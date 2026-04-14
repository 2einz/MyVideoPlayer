#include "player/output/qt/qml_image_provider.h"

namespace my_video_player {
QmlImageProvider::QmlImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage QmlImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    QMutexLocker locker(&mutex_);
    qDebug() << "QML is requesting image, id:" << id;

    if (image_.isNull()) {
        return QImage(requestedSize.isValid() ? requestedSize : QSize(100, 100), QImage::Format_RGB32);
    }

    if (size)
        *size = image_.size();
    return image_; // 返回副本，确保线程安全
}

void QmlImageProvider::UpdateImage(const QImage& img) {
    QMutexLocker locker(&mutex_);
    image_ = img;
}
} // namespace my_video_player