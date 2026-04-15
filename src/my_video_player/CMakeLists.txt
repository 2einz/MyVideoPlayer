set(ENGINE_DIR player/engine)
set(RENDERER_DIR player/output)

set(PROJECT_SOURCES
    main.cc
    controller/controller.cc
    controller/controller.h
    player/player.cc
    player/player.h
    player/media_state.hpp
    ${ENGINE_DIR}/common/error_util.h
    ${ENGINE_DIR}/common/ffmpeg_raii.h
    ${ENGINE_DIR}/common/engine_types.h
    ${ENGINE_DIR}/utils/thread_safe_queue.hpp
    ${ENGINE_DIR}/demux/demuxer.cc
    ${ENGINE_DIR}/demux/demuxer.h
    ${ENGINE_DIR}/thread/demux_thread.h
    ${ENGINE_DIR}/thread/demux_thread.cc
    ${ENGINE_DIR}/thread/video_decode_thread.h
    ${ENGINE_DIR}/thread/video_decode_thread.cc
    ${ENGINE_DIR}/decode/video_decoder.h
    ${ENGINE_DIR}/decode/video_decoder.cc
    ${ENGINE_DIR}/decode/audio_decoder.h
    ${ENGINE_DIR}/decode/audio_decoder.cc
    ${ENGINE_DIR}/convert/video_converter.h
    ${ENGINE_DIR}/convert/video_converter.cc
    ${RENDERER_DIR}/qt/qml_image_provider.h
    ${RENDERER_DIR}/qt/qml_image_provider.cc
    ${RENDERER_DIR}/qt/video_renderer_qimage.h
    ${RENDERER_DIR}/qt/video_renderer_qimage.cc
)

# 生成可执行程序
qt_add_executable(
    ${PROJECT_NAME} # WIN32
    ${PROJECT_SOURCES}
)

# 配置 QML 模块
qt_add_qml_module(
    ${PROJECT_NAME}
    URI my.video.player
    VERSION 1.0
    QML_FILES ui/main.qml
    RESOURCES ui/assets/player_menu.svg
              ui/assets/player_next.svg
              ui/assets/player_pause.svg
              ui/assets/player_prior.svg
              ui/assets/player_play.svg
              ui/assets/player_fullscreen.svg
              ui/assets/player_close.svg
              ui/assets/player_minimize.svg
              ui/assets/player_maximize.svg
              ui/assets/player_resize.svg
              ui/assets/player_resize_black.svg
              ui/assets/player_close_black.svg
              ui/assets/player_minimize_black.svg
              ui/assets/player_maximize_black.svg
              ui/assets/player_menu_black.svg
              ui/assets/player_next_black.svg
              ui/assets/player_pause_black.svg
              ui/assets/player_prior_black.svg
              ui/assets/player_play_black.svg
              ui/assets/player_fullscreen_black.svg
)

#  包含与链接
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE "${CMAKE_SOURCE_DIR}/src" "${FFMPEG_DIR}/include"
)

target_link_directories(
    ${PROJECT_NAME}
    PRIVATE
    "${FFMPEG_DIR}/lib"
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::Multimedia
            avformat
            avcodec
            avutil
            swscale
            swresample
)

# 自动部署
if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" "--no-compiler-runtime" "--debug" "--qmldir"
                "${CMAKE_CURRENT_SOURCE_DIR}" "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Deploying Qt libraries using windeployqt..."
    )
endif()
