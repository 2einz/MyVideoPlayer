cmake_minimum_required(VERSION 3.19)

# 定义项目名称
project(my_video_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出路径到 bin 文件夹
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# Qt 自动处理 (QML 模式下不再需要 AUTOUIC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 找到 Qt 模块 (必须包含 Quick 和 Multimedia)
find_package(
    Qt6
    REQUIRED
    COMPONENTS Core
               Gui
               Qml
               Quick
               Multimedia
)

# 配置 FFmpeg 路径
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg)

# spdlog
set(SPDLOG_DIR ${CMAKE_SOURCE_DIR}/3rdparty/spdlog)

if(EXISTS ${SPDLOG_DIR}/CMakeLists.txt)
    # 阻止 spdlog 触发 install 命令
    set(SPDLOG_INSTALL
        OFF
        CACHE BOOL
              ""
              FORCE
    )
    add_subdirectory(${SPDLOG_DIR})
else()
    message(FATAL_ERROR "spdlog not found in ${SPDLOG_DIR}, please clone it first.")
endif()

# 进入子目录
add_subdirectory(src/my_video_player)

# 拷贝ffmpeg dll
if(WIN32)
    file(
        GLOB
        FFMPEG_DLLS
        "${FFMPEG_DIR}/bin/*.dll"
    )
    file(COPY ${FFMPEG_DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()
