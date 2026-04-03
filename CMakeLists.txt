cmake_minimum_required(VERSION 3.19)

# 1. 定义项目名称
project(my_video_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 设置输出路径到 bin 文件夹
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# 3. Qt 自动处理 (QML 模式下不再需要 AUTOUIC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 4. 找到 Qt 模块 (必须包含 Quick 和 Multimedia)
find_package(
    Qt6 REQUIRED
    COMPONENTS Core
               Gui
               Qml
               Quick
               Multimedia
)

set(PROJECT_SOURCES
    src/my_video_player/main.cc
    src/my_video_player/controller.cc
    src/my_video_player/controller.h
)

# 生成可执行程序并注册 QML 模块
# 在 Qt 6 中，推荐使用 qt_add_qml_module
qt_add_executable(
    ${PROJECT_NAME}
    WIN32
    ${PROJECT_SOURCES}
)

# 将 QML 文件打包
# 这样你的 main.qml 就会被自动编译进 exe 的资源文件中
qt_add_qml_module(
    ${PROJECT_NAME}
    URI my.video.player
    VERSION 1.0
    QML_FILES src/my_video_player/main.qml
)

# 在目标创建之后，设置头文件包含路径 (Google 风格)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR})

# 链接 Qt 库 (增加 Quick 和 Multimedia)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::Multimedia
)

# 自动化部署
if(WIN32)
    find_program(
        WINDEPLOYQT_EXECUTABLE windeployqt HINTS "D:/Workspace/Env/Qt/6.11.0/msvc2022_64/bin"
    )

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND
                "${WINDEPLOYQT_EXECUTABLE}" "--no-compiler-runtime" "--debug" "--qmldir"
                "${CMAKE_SOURCE_DIR}/src/my_video_player" # 告知 QML 文件位置
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt libraries using windeployqt..."
        )
    endif()
endif()
