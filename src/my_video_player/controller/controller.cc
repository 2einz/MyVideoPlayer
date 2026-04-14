#include "controller/controller.h"

#include <QDebug>
#include <thread>

namespace my_video_player {
Controller::Controller(QObject* parent) : QObject(parent) {}

Controller::~Controller() {
    Stop();
}

bool Controller::is_playing() const {
    return player_.GetMediaState()->IsPlaying();
}

void Controller::OpenFile(const QString& url) {
    auto* ms = player_.GetMediaState();
    if (!ms->IsIdle()) {
        Stop(); // 停止旧线程
        Clear();
        ms->Dispatch(Action::kUserStop);
    }

    // 开始加载
    // ms->Dispatch(Action::kOpen);

    QString local_path = url;
    if (local_path.startsWith("file:///"))
        local_path = local_path.mid(8);

    qDebug() << "Controller: Opening" << local_path;

    // 调用 Player 层的 Open
    if (player_.Open(local_path.toStdString()) == 0) {
        qDebug() << "Controller: Video opened and test decoded successfully.";
        ms->Dispatch(Action::kPrepared);

        // 获取时间
        double total_sec = player_.GetDuration();
        total_time_ = FormatTime(total_sec);

        emit TimeChanged();

        // 获取视频参数并初始化 Renderer
        auto* params = player_.GetVideoCodecParams();

        if (params) {
            renderer_.Init(params->width, params->height, static_cast<AVPixelFormat>(params->format));

            // binding callback
            player_.SetFrameCallback([this, total_sec](const FrameItem& frame) {
                renderer_.Render(frame.frame.get());
                emit frameReady();
                QString time_str = FormatTime(frame.pts);
                if (time_str != current_time_) {
                    current_time_ = time_str;
                    emit TimeChanged();
                }

                if (total_sec > 0) {
                    double p = frame.pts / total_sec;
                    if (std::abs(progress_ - p) > 0.0001) {
                        progress_ = p;
                        emit ProgressChanged();
                    }
                }
            });

            player_.SetFinishCallback([this]() { emit IsPlayingChanged(); });

            // 开始后台播放
            ms->Dispatch(Action::kUserPlay);
            emit IsPlayingChanged();

            thread_running_ = true;
            play_thread_ = std::thread([this]() { player_.StartLoop(thread_running_); });
        }

    } else {
        ms->Dispatch(Action::kError);
        qCritical() << "Controller: Failed to open video.";
    }
}

QString Controller::FormatTime(double total_seconds) {
    int tt = static_cast<int>(total_seconds);
    int h = tt / 3600, m = (tt % 3600) / 60, s = tt % 60;

    return QString::asprintf("%02d:%02d:%02d", h, m, s);
}

void Controller::TogglePlay() {
    auto* ms = player_.GetMediaState();
    if (ms->IsPlaying())
        ms->Dispatch(Action::kUserPause);
    else
        ms->Dispatch(Action::kUserPlay);

    emit IsPlayingChanged();
}

void Controller::Stop() {
    thread_running_ = false;
    player_.GetMediaState()->Dispatch(Action::kUserStop);
    if (play_thread_.joinable()) {
        play_thread_.join();
    }
}

void Controller::Clear() {
    progress_ = 0.0;
    current_time_ = "00:00:00";
    total_time_ = "00:00:00";

    emit ProgressChanged();
    emit TimeChanged();
}

void Controller::SetProgress(double p) {
    // p: from 0.0 to 1.0
    auto* ms = player_.GetMediaState();
    double total_sec = player_.GetDuration();
    if (total_sec <= 0)
        return;

    double target_sec = p * total_sec;

    ms->Seek(target_sec);

    if (std::abs(progress_ - p) > 0.0001) {
        progress_ = p;
        emit ProgressChanged();

        current_time_ = FormatTime(target_sec);
        emit TimeChanged();
    }
}

void Controller::SetImageProvider(QmlImageProvider* provider) {
    provider_ = provider;
    renderer_.SetProvider(provider);
}

} // namespace my_video_player