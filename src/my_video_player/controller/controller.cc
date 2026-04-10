#include "controller/controller.h"

#include <QDebug>

namespace my_video_player {
Controller::Controller(QObject* parent) : QObject(parent) {}

void Controller::TogglePlay() {
    is_playing_ = !is_playing_;
    qDebug() << "Toggle Play, Status is: " << is_playing_;
    emit IsPlayingChanged();
}

void Controller::OpenFile(const QString& url) {
    qDebug() << "Open file:" << url;
    QString local_path = url;
    if (local_path.startsWith("file:///")) {
        local_path = local_path.mid(8);
    }

    qDebug() << "Controller: Opening" << local_path;

    // 调用 Player 层的 Open
    if (player_.Open(local_path.toStdString()) == 0) {
        qDebug() << "Controller: Video opened and test decoded successfully.";
    } else {
        qCritical() << "Controller: Failed to open video.";
    }
}

void Controller::SetProgress(double p) {
    if (progress_ != p) {
        progress_ = p;
        emit ProgressChanged();
    }
}

void Controller::SetVideoSink(QVideoSink* sink) {
    if (video_sink_ != sink) {
        video_sink_ = sink;
        emit VideoSinkChanged();
        qDebug() << "Video Sink has been registered\n";
    }
}
} // namespace my_video_player