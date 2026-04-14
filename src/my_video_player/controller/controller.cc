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
        ms->Dispatch(Action::kUserStop);
    }

    // 开始加载
    ms->Dispatch(Action::kOpen);

    QString local_path = url;
    if (local_path.startsWith("file:///"))
        local_path = local_path.mid(8);

    qDebug() << "Controller: Opening" << local_path;

    // 调用 Player 层的 Open
    if (player_.Open(local_path.toStdString()) == 0) {
        qDebug() << "Controller: Video opened and test decoded successfully.";
        // 获取视频参数并初始化 Renderer
        AVCodecParameters* params = player_.GetDemuxer().GetVideoCodecParameters();

        if (params) {
            renderer_.Init(params->width, params->height, static_cast<AVPixelFormat>(params->format));

            // 准备就绪
            ms->Dispatch(Action::kPrepared);

            // 开始后台播放
            ms->Dispatch(Action::kUserPlay);
            emit IsPlayingChanged();

            thread_running_ = true;
            play_thread_ = std::thread(&Controller::PlayLoop, this);
        }

    } else {
        ms->Dispatch(Action::kError);
        qCritical() << "Controller: Failed to open video.";
    }
}

void Controller::PlayLoop() {
    auto* ms = player_.GetMediaState();
    auto& demuxer = player_.GetDemuxer();
    auto& decoder = player_.GetVideoDecoder();

    PacketItem pkt;
    FrameItem frame;
    double last_pts = -1.0; // 标记为第一帧

    AVRational tb = demuxer.format_context()->streams[demuxer.video_stream_index()]->time_base;

    qDebug() << "Controller: PlayLoop Start\n";
    while (thread_running_) {
        State current_state = ms->GetState();

        // 停止和错误处理
        if (current_state == State::kStopped || current_state == State::kError)
            break;

        // 处理暂停
        if (current_state == State::kPaused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        // 处理Seek
        if (ms->HasSeekRequest()) {
            double target = ms->ConsumeSeekPosition();
            // TODO: av_seek_frame(...)
            ms->Dispatch(Action::kReachedSeekTarget);
            continue;
        }

        // 读取并解码
        if (demuxer.ReadPacket(&pkt) < 0) {
            ms->Dispatch(Action::kReachedEnd); // 播放结束
            break;
        }

        if (pkt.stream_index == demuxer.video_stream_index()) {
            if (decoder.SendPacket(pkt) == 0) {
                while (decoder.ReceiveFrame(&frame, tb) == 0) {
                    static int frame_count = 0;
                    if (++frame_count % 30 == 0) {
                        qDebug() << "Decoding Frame:" << frame_count << "PTS:" << frame.pts;
                    }
                    // 渲染
                    renderer_.Render(frame.frame.get());
                    emit frameReady();

                    // 简易同步
                    if (last_pts >= 0) { // 不是第一帧
                        double diff = frame.pts - last_pts;
                        if (diff > 0 && diff < 1.0) { // 正常的帧间隔（小于1秒）
                            std::this_thread::sleep_for(std::chrono::milliseconds((int)(diff * 1000)));
                        }
                    }
                    last_pts = frame.pts; // 无论是否睡眠，必须更新基准 PTS
                }
            }
        }
    }
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

void Controller::SetProgress(double p) {
    if (progress_ != p) {
        progress_ = p;
        emit ProgressChanged();
    }
}

void Controller::SetImageProvider(QmlImageProvider* provider) {
    provider_ = provider;
    renderer_.SetProvider(provider);
}

} // namespace my_video_player