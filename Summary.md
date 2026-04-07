
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

### 2.1 解码核心流程图解

FFmpeg 的解码过程遵循严格的流水线模式。以下流程是开发音视频程序的基础：

| 宏观流程 (Logic Flow)                                        | 详细 API 调用 (API Pipeline)                                 |
| :----------------------------------------------------------- | :----------------------------------------------------------- |
| ![流程1](D:\Workspace\AudioVideo\MyVideoPlayer\img\ffmpeg-decode-flow.jpg) | ![流程2](D:\Workspace\AudioVideo\MyVideoPlayer\img\ffmpeg-decoding.png) |
| *图 2.1-A：数据流转示意图*                                   | *图 2.1-B：核心函数调用链*                                   |

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
---