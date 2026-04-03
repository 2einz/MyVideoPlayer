cmake_minimum_required(VERSION 3.19)

# 1. 定义项目名称
project(my_video_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 设置输出路径（直接生成到项目根目录）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# 3. Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 自动找到 Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# 4. 指定源文件列表 (Google 风格：.cc 后缀，路径清晰)
set(PROJECT_SOURCES
    src/my_video_player/main.cc
    src/my_video_player/widget.cc
    src/my_video_player/widget.h
    src/my_video_player/widget.ui
)

# 5. 【关键】首先生成可执行程序
# 这里使用 ${PROJECT_NAME} 变量，即 my_video_player
qt_add_executable(${PROJECT_NAME}
    WIN32
    ${PROJECT_SOURCES}
)

# 6. 【关键】在目标创建之后，再设置头文件包含路径
# 这样设置后，代码里可以写 #include "src/my_video_player/widget.h"
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR})

# 7. 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)


if(WIN32)
    # 找到 windeployqt 程序的路径
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "D:/Workspace/Env/Qt/6.11.0/msvc2022_64/bin")

    if(WINDEPLOYQT_EXECUTABLE)
        # 在编译完成后，自动运行 windeployqt
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    "--no-compiler-runtime"
                    "--debug"
                    "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt libraries using windeployqt..."
        )
    endif()
endif()