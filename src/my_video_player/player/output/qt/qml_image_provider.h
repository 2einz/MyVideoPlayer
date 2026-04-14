#ifndef MY_VIDEO_PLAYER_OUTPUT_QT_QML_IMAGE_PROVIDER_H_
#define MY_VIDEO_PLAYER_OUTPUT_QT_QML_IMAGE_PROVIDER_H_

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

namespace my_video_player {
class QmlImageProvider : public QQuickImageProvider {
public:
    QmlImageProvider();
    ~QmlImageProvider() override = default;

    QImage requestImage(const QString& id, QSize* size, const QSize& requestdSize) override;

    void UpdateImage(const QImage& image);

private:
    QImage image_;
    mutable QMutex mutex_;
};
} // namespace my_video_player
#endif // MY_VIDEO_PLAYER_OUTPUT_QT_QML_IMAGE_PROVIDER_H_