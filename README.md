# 配置环境

- qt6
- ffmpeg8.1


# 项目结构

my_video_player/
├── CMakeLists.txt               # 你现有的，我帮你适配好
├── 3rdparty/
│   └── ffmpeg/                  # 你的 FFmpeg 库
├── bin/                         # 输出 exe
└── src/
    └── my_video_player/
        ├── main.cc               # 程序入口
        ├── core/                 # 核心 FFmpeg 逻辑
        │   ├── ffmpeg_define.h   # FFmpeg 头文件包裹
        │   ├── decoder.h         # 抽象解码器
        │   ├── video_decoder.h   # 视频解码器
        │   ├── audio_decoder.h   # 音频解码器
        
        │   └── sync.h            # 音画同步
        ├── engine/                # 播放器引擎
        │   └── player_engine.h    # 总控制：播放、暂停、进度、音量
        ├── controller/            # QML 交互控制器（你已有的）
        │   └── controller.h
        └── ui/
            └── main.qml           # 你的界面