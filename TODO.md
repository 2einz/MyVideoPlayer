# 阶段 1：搭建 Qt 基础界面（1–3 天）

新建 Qt Widgets 项目，做无边框主窗口
实现：标题栏（最小化 / 最大化 / 关闭）、播放控制条（按钮 + 滑块）、播放列表
掌握：Qt 信号槽、自定义控件、鼠标拖动窗口、样式表 QSS

# 阶段 2：实现基础交互逻辑（2–5 天）

打开文件对话框、添加文件到列表、双击播放
播放 / 暂停 / 停止、进度条拖动、音量滑块、全屏切换
键盘快捷键（空格、回车、方向键）

# 阶段 3：接入 FFmpeg 做音视频解码（3–7 天）

编译 / 配置 FFmpeg 环境
实现：打开文件 → 解封装 → 解码视频帧 → 解码音频帧
掌握：AVFormatContext、AVCodecContext、AVPacket、AVFrame

# 阶段 4：用 SDL 做渲染与播放（3–7 天）

SDL 初始化窗口、渲染器、音频设备
视频：YUV 转纹理 → 渲染到 Qt 控件
音频：重采样 → 播放
音视频同步（以音频为基准）

# 阶段 5：完善体验（2–5 天）

播放记忆、列表保存、画面比例自适应、鼠标隐藏、右键菜单



```text
my_video_player/
├── CMakeLists.txt              # 总构建脚本
├── .cmake-format.json          # 格式化配置
├── bin/                        # 存放生成的 exe 和 dll (运行环境)
├── build/                      # 编译中间产物
├── src/
│   └── my_video_player/        # 主命名空间
│       ├── main.cc             # 程序入口，初始化 QML 引擎
│       ├── player_controller.cc # 业务逻辑：播放、暂停、进度条逻辑
│       ├── player_controller.h
│       ├── main.qml            # UI 界面：QML 描述
│       └── engine/             # 【核心】FFmpeg 底层封装
│           ├── video_decoder.cc # 视频解码、颜色转换
│           ├── video_decoder.h
│           ├── audio_decoder.cc # 音频解码、重采样 (进阶)
│           ├── audio_decoder.h
│           ├── packet_queue.h   # 缓冲队列 (解复用后的数据包)
│           └── frame_queue.h    # 缓冲队列 (解码后的原始帧)
└── third_party/                # 存放 FFmpeg 的头文件和库
    └── ffmpeg/
```



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