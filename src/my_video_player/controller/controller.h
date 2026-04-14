#ifndef MY_VIDEO_PLAYER_CONTROLLER_CONTROLLER_H_
#define MY_VIDEO_PLAYER_CONTROLLER_CONTROLLER_H_

#include <QObject>
#include <QString>
#include <QTimer>
#include <QPointer>

#include <thread>
#include <atomic>

#include "player/player.h"
#include "player/output/qt/video_renderer_qimage.h"

namespace my_video_player {
class Controller : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ is_playing NOTIFY IsPlayingChanged)
    Q_PROPERTY(double progress READ progress WRITE SetProgress NOTIFY ProgressChanged)
    Q_PROPERTY(QString currentTime READ current_time NOTIFY TimeChanged)
    Q_PROPERTY(QString totalTime READ total_time NOTIFY TimeChanged)

public:
    explicit Controller(QObject* parent = nullptr);
    ~Controller();

    Q_INVOKABLE void TogglePlay();
    Q_INVOKABLE void OpenFile(const QString& url);
    Q_INVOKABLE void Stop();

    // Getters
    bool is_playing() const;
    double progress() const { return progress_; };
    QString current_time() const { return current_time_; }
    QString total_time() const { return total_time_; }
    QmlImageProvider* provider() const { return provider_; }

    // Setters
    void SetProgress(double p);
    void SetImageProvider(QmlImageProvider* provider);

signals:
    void IsPlayingChanged();
    void ProgressChanged();
    void TimeChanged();
    void frameReady(); // 用于通知 QML 刷新 Image 控件

private:
    QString FormatTime(double seconds);
    void Clear();

private:
    std::atomic<bool> thread_running_{false};
    std::thread play_thread_;

    double progress_ = 0.0; // 播放进度条
    QString current_time_ = "00:00:00";
    QString total_time_ = "00:00:00";

    QmlImageProvider* provider_ = nullptr;
    VideoRendererQImage renderer_;
    Player player_; // Controller 拥有 Player 实例
};
} // namespace my_video_player

#endif // MY_VIDEO_PLAYER_CONTROLLER_CONTROLLER_H_