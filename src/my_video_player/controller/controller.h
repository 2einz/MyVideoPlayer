#ifndef SRC_MY_VIDEO_PLAYER_CONTROLLER_CONTROLLER_H_
#define SRC_MY_VIDEO_PLAYER_CONTROLLER_CONTROLLER_H_

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVideoSink>
#include <QPointer>

namespace video_player {
class Controller : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ is_playing NOTIFY IsPlayingChanged)
    Q_PROPERTY(double progress READ progress WRITE SetProgress NOTIFY ProgressChanged)
    Q_PROPERTY(QString currentTime READ current_time NOTIFY TimeChanged)
    Q_PROPERTY(QString totalTime READ total_time NOTIFY TimeChanged)
    Q_PROPERTY(QVideoSink* videoSink READ video_sink WRITE SetVideoSink NOTIFY VideoSinkChanged)

public:
    explicit Controller(QObject* parent = nullptr);

    Q_INVOKABLE void TogglePlay();
    Q_INVOKABLE void OpenFile(const QString& url);

    // Getters
    bool is_playing() const { return is_playing_; };
    double progress() const { return progress_; };
    QString current_time() const { return current_time_; }
    QString total_time() const { return total_time_; }
    QVideoSink* video_sink() const { return video_sink_.data(); }

    // Setters
    void SetProgress(double p);
    void SetVideoSink(QVideoSink* sink);
signals:
    void IsPlayingChanged();
    void ProgressChanged();
    void TimeChanged();
    void VideoSinkChanged();

private:
    bool is_playing_ = false; // 是否播放
    double progress_ = 0.0;   // 播放进度
    QString current_time_ = "00:00:00";
    QString total_time_ = "00:00:00";
    QPointer<QVideoSink> video_sink_;
};
} // namespace video_player

#endif // SRC_MY_VIDEO_PLAYER_CONTROLLER_CONTROLLER_H_