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
    // TODO:打开文件流程，ffmpeg解码初始化
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