#include "videohandler.h"

VideoWidget::VideoWidget(const QString &video, QWidget *parent)
    : QVideoWidget(parent), video_path_(video) {
    media_player_ = new QMediaPlayer();
    media_player_->setVideoOutput(this);
    media_player_->setMedia(QUrl::fromLocalFile(video));
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setAspectRatioMode(Qt::IgnoreAspectRatio);  // 缩放适应videoWidget的大小
                                                      //    media_player_->setVolume(0);
    media_player_->play();
    //    QThread::msleep(50);
    //    media_player_->pause();

    connect(media_player_, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::EndOfMedia) {
                    emit videoFinished();  // 视频播放结束信号
                    media_player_->play();
                }
            });

    timer_ = new QTimer(this);
    // 区分单击还是双击
    connect(timer_, SIGNAL(timeout()), this, SLOT(mouseClick()));
}

VideoWidget::~VideoWidget() {}

void VideoWidget::stopVideo() {
    media_player_->stop();
    return;
}

void VideoWidget::startVideo() {
    media_player_->play();
    return;
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *) {
    timer_->stop();
    emit this->onDoubleClick();
    QDesktopServices::openUrl(QUrl(video_path_));
}

void VideoWidget::mousePressEvent(QMouseEvent *) {
    timer_->start(300);
}

void VideoWidget::mouseClick() {
    timer_->stop();
    if (media_player_->state() == QMediaPlayer::PlayingState) {
        media_player_->pause();
    } else {
        media_player_->play();
    }
    return;
}
