#ifndef MY_VIDEO_PLAYER_PLAYER_OUTPUT_QT_VIDEO_RENDERER_QIMAGE_H_
#define MY_VIDEO_PLAYER_PLAYER_OUTPUT_QT_VIDEO_RENDERER_QIMAGE_H_

#include "player/engine/convert/video_converter.h"
#include "player/output/qt/qml_image_provider.h"

namespace my_video_player {
class VideoRendererQImage {
public:
    void SetProvider(QmlImageProvider* provide) { provider_ = provide; };

    int Init(int w, int h, AVPixelFormat fmt);

    void Render(const AVFrame* frame);

private:
    QmlImageProvider* provider_ = nullptr;
    VideoConverter converter_;
    int width_ = 0;
    int height_ = 0;
};
} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_PLAYER_OUTPUT_QT_VIDEO_RENDERER_QIMAGE_H_