✅ 打开视频文件（FFmpeg）
✅ 解码视频帧
✅ 转 RGB
✅ 显示到 Qt（哪怕很卡）
✅ 加线程
✅ 加播放/暂停
✅ 加音频
✅ 做同步

硬解码（NVDEC / VAAPI）
GPU 渲染（OpenGL / Vulkan）
音视频同步（AVSync）
播放列表（你已经有UI了）
字幕（ASS）


```text
my_video_player/
├── CMakeLists.txt                  # 构建入口
├── .cmake-format.json              # CMake 格式化配置
├── .clang-format                   # C++ 代码风格配置
├── bin/                            # 最终输出：exe、dll 等运行时文件
├── build/                          # CMake 编译临时文件目录
├── third_party/                    # 第三方依赖库
│   ├── spdlog/
│   └── ffmpeg/                     # FFmpeg SDK（include / lib / bin）
└── src/                            # 源码主目录
    └── my_video_player/            # 顶级命名空间
        ├── main.cc                 # 程序入口
        ├── controller/             # 【控制层】QML ↔ C++ 交互
        │   ├── player_controller.h
        │   └── player_controller.cc
        ├── ui/                     # 【视图层】QML 界面与资源
        │   ├── assets/             # 图标、图片等资源
        │   └── main.qml            # 主界面
        ├── log/                    # 日志工具
        │   ├── my_log_ini.h/       # 日志工具宏
        │   └── my_spdlog.h
        └── player/                 # 【业务层】播放器中枢
            ├── media_state.h       # 播放器状态枚举
            ├── player.h            # 对外暴露接口
            ├── player.cc           # 组合引擎与输出模块
            ├── engine/             # 【核心引擎】FFmpeg 解码逻辑
            │   ├── common/
            │   │   ├── ffmpeg_raii.h
            │   │   ├── engine_types.h
            │   │   ├── error_util.h
            │   │   └── thread_safe_queue.h
            │   ├── demux/
            │   │   ├── demuxer.h
            │   │   └── demuxer.cc
            │   ├── decode/
            │   │   ├── video_decoder.h
            │   │   ├── video_decoder.cc
            │   │   ├── audio_decoder.h
            │   │   └── audio_decoder.cc
            │   ├── buffer/
            │   │   ├── video_packet_queue.h
            │   │   ├── audio_packet_queue.h
            │   │   ├── video_frame_queue.h
            │   │   └── audio_frame_queue.h
            │   ├── convert/
            │   │   ├── video_converter.h
            │   │   ├── video_converter.cc
            │   │   ├── audio_resampler.h
            │   │   └── audio_resampler.cc
            │   ├── sync/
            │   │   ├── av_clock.h
            │   │   ├── av_clock.cc
            │   │   ├── av_sync.h
            │   │   └── av_sync.cc
            │   ├── thread/
            │   │   ├── demux_thread.h
            │   │   ├── demux_thread.cc
            │   │   ├── video_decode_thread.h
            │   │   ├── video_decode_thread.cc
            │   │   ├── audio_decode_thread.h
            │   │   ├── audio_decode_thread.cc
            │   │   ├── video_render_thread.h
            │   │   └── video_render_thread.cc
            │   ├── subtitle/
            │   │   ├── subtitle_decoder.h
            │   │   └── subtitle_decoder.cc
            │   └── filter/
            │       ├── video_filter.h
            │       └── video_filter.cc
            └── output/             # 【渲染输出】
                ├── qt/
                │   ├── video_renderer_qt.h
                │   ├── video_renderer_qt.cc
                │   ├── audio_renderer_qt.h
                │   └── audio_renderer_qt.cc
                └── opengl/
                    ├── video_renderer_gl.h
                    └── video_renderer_gl.cc
```

DemuxThread
    ↓
Audio/Video Packet Queue
    ↓
DecodeThread
    ↓
Frame Queue
    ↓
Convert
    ↓
AVSync（决定时间）
    ↓
Renderer（Qt/OpenGL）

### 架构设计：三层模型

我们要采用 **解耦设计**，确保 UI 线程永远不会被繁重的解码任务卡死。

#### 1. UI 层 (QML)

- **职责**：只负责绘制。显示播放/暂停按钮、进度条、视频画面。
- **交互**：通过 playerCtrl 属性与 C++ 通信。

#### 2. 控制层 (C++ PlayerController)

- **职责**：状态机管理。处理 QML 传来的 play(), seek(), setVolume() 指令。
- **桥梁**：将底层的解码数据（每一帧）推送到 QML 的 VideoSink。

#### 3. 引擎层 (C++ FFmpegEngine) —— 这是你练习 FFmpeg 的核心

- **线程模型**：必须是**多线程**。
  - **线程 A (Demuxer)**：读取视频文件，将 Packet（压缩数据）放入队列。
  - **线程 B (Video Decoder)**：从队列取 Packet，解码成 Frame（原始数据），转换颜色空间。
  - **线程 C (Audio Decoder)**：解码音频数据。



实现 `engine` 层是一个由浅入深的过程。虽然你的最终目标是多线程实时播放，但**千万不要一上来就写多线程和音视频同步**，否则 Debug 会让你非常痛苦。

按照**工业界增量开发**的逻辑，我建议你按照以下 **4 个阶段** 逐步实现：

---

### 第一阶段：基础设施与环境验证 (Foundation)
**目标**：确保 CMake 能找到 FFmpeg，且能打印出视频信息。

1.  **实现 `common/error_util.h`**：
    *   编写一个函数 `std::string AvErrorToString(int errnum)`。
    *   调用 FFmpeg 的 `av_strerror`。
    *   **原因**：FFmpeg 的报错全是负数（如 `-1094995529`），没有这个工具你根本不知道哪里出错了。
2.  **实现 `demux/demuxer.h` (初步)**：
    *   只实现一个 `Open(const std::string& url)` 函数。
    *   流程：`avformat_open_input` -> `avformat_find_stream_info` -> 打印视频宽、高、时长、编码格式。
3.  **验证**：在 `main.cc` 里实例化 `Demuxer` 并打开一个 `.mp4`。只要控制台输出了正确的分辨率，环境就通了。

---

### 第二阶段：单线程“抽帧”测试 (Static Frame)
**目标**：不考虑速度，只求从文件里解出一张图片并在 QML 显示出来。

1.  **实现 `common/ffmpeg_raii.h`**：
    *   定义 `AVPacket` 和 `AVFrame` 的智能指针包装。
    *   **原因**：从这一步开始涉及内存申请，必须保证不会内存泄漏。
2.  **实现 `decode/video_decoder.h` (核心)**：
    *   实现 `SendPacket()` 和 `ReceiveFrame()`。
    *   包装 `avcodec_send_packet` 和 `avcodec_receive_frame`。
3.  **实现 `convert/video_converter.h`**：
    *   使用 `sws_scale` 将解码出的 `YUV420P` 转换为 `RGBA`。
4.  **实现 `render/video_renderer.h`**：
    *   将 `RGBA` 数据封装进 `QVideoFrame`，通过 `QVideoSink::setVideoFrame` 发送。
5.  **验证**：点击 UI 上的播放按钮，屏幕上跳出一张静止的视频截图。

---

### 第三阶段：生产者-消费者流水线 (Pipeline)
**目标**：引入线程和队列，让视频“动起来”。

1.  **实现 `buffer/packet_queue.h`**：
    *   使用 `std::queue` + `std::mutex` + `std::condition_variable`。
    *   实现一个线程安全的阻塞队列。
2.  **实现线程循环**：
    *   **线程 A (Demux)**：循环 `av_read_frame`，把 `Packet` 塞进队列。
    *   **线程 B (Decode)**：从队列拿 `Packet`，解码，转换，然后立刻推给 `Renderer`。
3.  **验证**：视频开始疯狂快进播放（因为还没做同步，解码多快就播多快）。

---

### 第四阶段：灵魂注入：音视频同步 (A/V Sync)
**目标**：按照正确的时间播放视频，并加入音频。

1.  **实现 `sync/av_clock.h`**：
    *   学习什么是 **PTS (Presentation Time Stamp)**。
    *   实现一个简单的计时器。
2.  **实现同步逻辑**：
    *   在 `VideoDecoder` 推送帧之前，计算 `PTS` 与当前时钟的差值。
    *   如果来早了，就 `QThread::msleep` 睡一会。
3.  **实现 `audio_decoder` 和 `audio_resampler`**：
    *   处理音频流，并以此作为主时钟。

---

### 现在的具体行动建议：

**从 `src/my_video_player/player/engine/common/error_util.h` 开始。**

这是最简单的文件，但能帮你熟悉 Google 风格的头文件保护（Header Guard）和命名空间。

**代码示例 (`error_util.h`)：**
```cpp
#ifndef MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_
#define MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_

#include <string>
extern "C" {
#include <libavutil/error.h>
}

namespace my_video_player {
namespace engine {

inline std::string AvErrorToString(int errnum) {
  char errbuf[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
  return std::string(errbuf);
}

}  // namespace engine
}  // namespace my_video_player

#endif  // MY_VIDEO_PLAYER_PLAYER_ENGINE_COMMON_ERROR_UTIL_H_
```

写完这个，再去写 `Demuxer::Open()`。当你第一次在控制台看到 FFmpeg 打印出 `Video: h264, 1920x1080, 24fps` 的时候，你就已经跨过了最难的门槛！

**需要我帮你写出第一个 `Demuxer::Open` 的代码框架吗？**