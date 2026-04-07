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
├── .cmake-format.json              # 格式化 CMake
├── .clang-format                   # 格式化 C++
├── bin/                            # 最终生成产物 (.exe, .dll)
├── build/                          # 临时构建目录
├── third_party/                    # 第三方依赖
│   └── ffmpeg/                     # FFmpeg SDK (include/lib/bin)
├── src/                            # 源码根目录
│   └── my_video_player/            # 顶级命名空间 (Include 路径起点)
│       ├── main.cc                 # 程序入口
│       ├── controller/             # 【控制层】QML 交互桥梁
│       │   ├── player_controller.h
│       │   └── player_controller.cc
│       ├── ui/                     # 【视图层】QML 资源
│       │   ├── assets/             # 图片、图标 (.svg, .png)
│       │   └── main.qml            # 主界面
│       └── player/                 # 【业务层】逻辑中枢
│           ├── player.h            # 暴露给 Controller 的接口
│           ├── player.cc           # 组合 Engine 各个模块
│           └── engine/             # 【引擎层】FFmpeg 核心实现
│               ├── common/         # 通用工具
│               │   ├── ffmpeg_raii.h   # 内存安全包装
│               │   ├── engine_types.h  # 枚举、常量、状态定义
│               │   └── error_util.h    # 错误处理
│               ├── demux/          # 解封装
│               │   ├── demuxer.h
│               │   └── demuxer.cc
│               ├── decode/         # 解码 (音视频分离)
│               │   ├── video_decoder.h
│               │   ├── video_decoder.cc
│               │   ├── audio_decoder.h
│               │   └── audio_decoder.cc
│               ├── buffer/         # 缓冲 (同步/阻塞队列)
│               │   ├── packet_queue.h
│               │   ├── packet_queue.cc
│               │   ├── frame_queue.h
│               │   └── frame_queue.cc
│               ├── convert/        # 格式转换 (Sws/Swr)
│               │   ├── video_converter.h
│               │   ├── video_converter.cc
│               │   ├── audio_resampler.h
│               │   └── audio_resampler.cc
│               ├── sync/           # 同步控制
│               │   ├── av_clock.h  # 时间戳同步核心
│               │   └── av_clock.cc
│               └── render/         # 渲染适配 (Qt 专用)
│                   ├── video_renderer.h # 封装 QVideoSink
│                   ├── video_renderer.cc
│                   ├── audio_renderer.h # 封装 QAudioSink
│                   └── audio_renderer.cc
```

用户操作：ui/main.qml
→
→
 controller/player_controller.cc
业务调度：controller
→
→
 player/player.cc (控制 Start/Stop/Seek)
数据流向：
engine/demux/ (读取文件)
→
→
 buffer/packet_queue
engine/decode/ (多线程解码)
→
→
 buffer/frame_queue
engine/convert/ (颜色/采样率转换)
engine/sync/ (PTS 校准)
engine/render/ (转换为 Qt 帧并推送给 QVideoSink)


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
