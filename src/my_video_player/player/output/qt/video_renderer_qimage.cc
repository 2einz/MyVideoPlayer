#include "player/output/qt/video_renderer_qimage.h"

namespace my_video_player {
int VideoRendererQImage::Init(int w, int h, AVPixelFormat fmt) {
    width_ = w;
    height_ = h;
    return converter_.init(w, h, fmt, w, h, AV_PIX_FMT_RGBA);
}

void VideoRendererQImage::Render(const AVFrame* frame) {
    if (!provider_ || !frame)
        return;

    // 创建 QImage
    QImage img(width_, height_, QImage::Format_RGBA8888);

    // 调用引擎层的 Converter 填充像素数据
    converter_.convert2buffer(frame, img.bits(), img.bytesPerLine());

    // 更新 Provider 内部图像
    provider_->UpdateImage(img);
}
} // namespace my_video_player