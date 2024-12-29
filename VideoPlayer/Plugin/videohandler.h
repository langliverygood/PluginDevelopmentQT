#ifndef VIDEOHANDLER_H
#define VIDEOHANDLER_H

#include <QDesktopServices>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QTimer>
#include <QUrl>
#include <QVideoWidget>

class VideoWidget : public QVideoWidget {
    Q_OBJECT
public:
    explicit VideoWidget(const QString &video, QWidget *parent = nullptr);
    ~VideoWidget();

    void stopVideo();
    void startVideo();

private:
    QMediaPlayer *media_player_;
    QMediaPlaylist *media_play_list_;
    QTimer *timer_;  // 区分单击还是双击
    QString video_path_;

    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
signals:
    void videoPlaying();
    void videoFinished();
    void onDoubleClick();

public slots:
    void mouseClick();
};

#endif  // VIDEOHANDLER_H
