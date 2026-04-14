
---

# 开发环境与 FFmpeg 技术笔记

## 一、 Qt 环境配置与部署

### 1.1 自动化部署工具
在 Windows 平台下，使用 Qt 自带的 `windeployqt` 可以自动补全程序运行所需的动态库（DLL）。

**使用命令：**
```bash
# 建议在 Qt 命令行终端中运行，以确保环境变量正确
# 路径格式：[windeployqt路径] [可执行文件路径]
D:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe Project.exe
```

### 1.2 编译器选择（重要）
> [!CAUTION]
> **在 MSVC 环境下开发时，请勿使用 Clang 编译器。**
> 混合使用可能导致 ABI 不兼容，引发不可预知的链接错误或运行时崩溃。请统一使用原生 MSVC 编译器。

---

## 二、 FFmpeg 开发笔记

本章节记录 FFmpeg API 调用过程中的核心细节与坑点。

这一章节是 FFmpeg 开发的核心逻辑。为了让它更像一份专业的**技术手册**，我将其美化为：**流程图示 -> 步骤分解 -> 核心代码 -> 数据产出** 的逻辑结构。

---

### 2.1 解码核心流程全解析

FFmpeg 的解码过程遵循严格的流水线模式。理解以下流程是开发音视频程序的基础：

#### 2.1.1 流程图示
| 宏观逻辑 (Logic Flow)                                        | 详细 API 调用 (API Pipeline)                                 |
| :----------------------------------------------------------- | :----------------------------------------------------------- |
| ![流程1](D:\Workspace\AudioVideo\MyVideoPlayer\img\ffmpeg-decode-flow.jpg) | ![流程2](D:\Workspace\AudioVideo\MyVideoPlayer\img\ffmpeg-decoding.png) |
| *图 2.1-A：数据流转示意图*                                   | *图 2.1-B：核心函数调用链*                                   |

**核心路径总结：**
`文件 (File)` ➔ `解封装 (Demux)` ➔ `压缩包 (Packet)` ➔ `解码 (Decode)` ➔ `原始帧 (Frame)`

---

#### 2.1.2 步骤分解与 API 实现

我们可以将整个解码初始化与循环过程分为三个阶段：

##### 第一阶段：环境初始化与流定位
首先需要打开容器并确认我们要处理的媒体流（如视频流）。

```cpp
// 1. 打开视频文件 (Demux)
avformat_open_input(&fmt_ctx, url, nullptr, nullptr);

// 2. 探测流信息（填充 fmt_ctx->streams）
avformat_find_stream_info(fmt_ctx, nullptr);

// 3. 自动筛选最优的视频流索引
int video_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
```

##### 第二阶段：解码器准备
定位到流后，需要配置与之对应的解码器上下文。

```cpp
// 4. 获取流参数 (Codec Parameters)
AVStream *st = fmt_ctx->streams[video_index];
AVCodecParameters *codec_par = st->codecpar;

// 5. 查找对应的硬件/软件解码器
const AVCodec *codec = avcodec_find_decoder(codec_par->codec_id);

// 6. 创建解码器上下文并关联参数
AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
avcodec_parameters_to_context(codec_ctx, codec_par); // 将流参数拷贝至上下文

// 7. 激活解码器
avcodec_open2(codec_ctx, codec, nullptr);
```

##### 第三阶段：解码循环 (The Loop)
这是程序最核心的部分：不停地读取 Packet 并送入解码器获取 Frame。

```cpp
AVPacket *pkt = av_packet_alloc();
AVFrame *frame = av_frame_alloc();

// 8. 从文件中读取压缩数据包 (Demuxing)
while (av_read_frame(fmt_ctx, pkt) >= 0) {
    // 9. 仅处理我们选中的视频流
    if (pkt->stream_index == video_index) {

        // 10. 发送压缩包到解码器
        if (avcodec_send_packet(codec_ctx, pkt) == 0) {

            // 11. 循环接收解码后的原始帧 (一个 Packet 可能对应多个 Frame)
            while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                // 读取成功，此时 frame 包含原始数据
            }
        }
    }
    av_packet_unref(pkt); // 释放 Packet 引用
}
```

---

#### 2.1.3 解码产物说明

解码完成后，数据存储在 `AVFrame` 结构体中，不同类型的流产物不同：

| 媒体类型   | 数据形式 (`frame->data`)                     | 后续处理                                                     |
| :--------- | :------------------------------------------- | :----------------------------------------------------------- |
| **📽️ 视频** | `data[0-2]` 存放 **YUV** 或 **RGB** 像素数据 | 通常需要使用 `libswscale` 缩放或转换颜色空间后显示           |
| **🔊 音频** | `data[0]` 存放 **PCM** 原始采样点            | 通常需要使用 `libswresample` 进行**重采样**（转采样率、转声道数） |

---

> [!IMPORTANT]
> **关键点提醒：**
> 1. **内存释放**：在循环中必须调用 `av_packet_unref`，否则会导致严重的内存泄漏。
> 2. **返回值判断**：`avcodec_receive_frame` 返回 `AVERROR(EAGAIN)` 表示需要更多输入，返回 `AVERROR_EOF` 表示流结束，这些在健壮的代码中必须处理。
> 3. **音频重采样**：音频解码出的 PCM 格式往往与声卡要求的格式不一致（例如声卡要求 44100Hz 采样率），因此重采样是音频播放的必经步骤。



---



### 2.2 错误处理：`av_err2str` vs `av_strerror`

FFmpeg 大部分函数返回 `int` 型错误码（负数为异常）。解析这些错误码时需注意内存安全。

#### 方案对比

| 特性         | `av_err2str(ret)`                          | `av_strerror(ret, buf, len)`     |
| :----------- | :----------------------------------------- | :------------------------------- |
| **本质**     | 宏 (Macro)                                 | 函数 (Function)                  |
| **内存分配** | 栈上的临时匿名数组（生命周期仅限当前行）   | 用户提供的缓冲区 (Buffer)        |
| **优点**     | 写法极简，适合快速打印调试                 | 内存安全，适合日志记录和逻辑传递 |
| **风险**     | **禁止**赋值给指针后续使用（会导致野指针） | 需要手动管理 `char` 数组空间     |

#### 代码范例
```cpp
// ❌ 错误做法：危险！指针指向的内存会在本行结束后销毁
const char* err = av_err2str(ret);
printf("%s", err);

// ✅ 正确做法 A：仅用于临时打印
printf("Operation failed: %s\n", av_err2str(ret));

// ✅ 正确做法 B：稳健的日志记录
char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
av_strerror(ret, err_buf, sizeof(err_buf));
qDebug() << "FFmpeg Log:" << err_buf;
```

---

### 2.3 媒体流与容器 (Streams & Containers)

#### 2.3.1 容器结构透视
一个多媒体文件（Container）本质上是多个数据流的复用体：
*   **📽️ 视频流**：可能存在多条（如不同分辨率、不同视角）。
*   **🔊 音频流**：多语言、多声道、解说词。
*   **📜 字幕流**：不同语言的内嵌字幕。

#### 2.3.2 查找流索引 (Find Stream Index)
在解码前，必须从 `AVFormatContext` 的流数组中筛选出我们需要的那一条。

##### 方案 A：手动遍历（不推荐）
仅能找到第一个匹配类型的流，无法识别质量优劣。
```cpp
int stream_idx = -1;
for (int i = 0; i < fmt_ctx->nb_streams; i++) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        stream_idx = i;
        break;
    }
}
```

##### 方案 B：官方推荐 API（推荐 ✅）
使用 `av_find_best_stream`，它会根据分辨率、码率和用户偏好自动筛选最优流。

```cpp
/**
 * 自动寻找最优的流
 * @param ic 格式上下文
 * @param type 流类型 (VIDEO/AUDIO/SUBTITLE)
 * @param wanted_stream_nb 指定流索引 (传 -1 自动选择)
 * @param related_stream 相关流索引 (如根据视频找对应音频)
 * @param decoder_ret 返回对应的解码器指针 (可选)
 */
int video_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

if (video_idx < 0) {
    fprintf(stderr, "错误: 无法在文件中找到视频流\n");
}
```

---

### 2.4 核心枚举：常用媒体类型 (`AVMediaType`)

在 FFmpeg 中，通过以下常量标识流的属性：

| 常量名                    | 物理含义 | 常见用途                 |
| :------------------------ | :------- | :----------------------- |
| `AVMEDIA_TYPE_VIDEO`      | **视频** | 图像序列、封面图         |
| `AVMEDIA_TYPE_AUDIO`      | **音频** | 音乐、对白               |
| `AVMEDIA_TYPE_SUBTITLE`   | **字幕** | 文本或位图字幕           |
| `AVMEDIA_TYPE_DATA`       | **数据** | GPS 信息、运动传感器数据 |
| `AVMEDIA_TYPE_ATTACHMENT` | **附件** | 字体文件、说明文档       |

---

> [!TIP]
> **下一节预告**：
> 在获取到 `stream_index` 后，接下来的步骤是根据 `codecpar` 寻找并打开对应的**解码器 (AVCodec)**。注意：FFmpeg 4.0 之后已经废弃了旧的 `codec` 字段，应统一使用 `codecpar`。
---

### 2.2 内存管理（待补充）

*   *提示：后续可在此处添加关于 av_malloc, av_frame_alloc 等内容...*

### 2.3 常用结构体说明（待补充）
*   *提示：后续可在此处添加关于 AVFormatContext, AVCodecContext 等内容...*

### 2.4 常见问题处理（FAQ）
*   **Q: 为什么某些格式无法解码？**
    *   A: 检查是否执行了网络初始化 `avformat_network_init()` 或检查配置编译脚本。

---

3.0 git 与ai结合
将cache中的部分读取出来，让ai生成git commit

```text
请根据用户提供的Git diff代码修改，完成以下任务：
1. 用简洁专业的中文总结本次修改内容
2. 按照工程规范生成Git commit message，支持 [feat]/[refactor]/[modify]/[fixed]/[chore] 等标准前缀
3. commit message 要求简洁、语义明确、可直接用于团队提交
4. 语言风格：专业、干练、工程化，不啰嗦、不口语化
```

```shell
git diff --cached > cached_diff.txt
```
---