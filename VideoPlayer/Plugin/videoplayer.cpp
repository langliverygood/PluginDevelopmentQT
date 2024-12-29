#include "videoplayer.h"

VideoPlayer::VideoPlayer(QObject *parent) : QObject(parent) {}

VideoPlayer::~VideoPlayer() {
    if (main_widget_) {
        main_widget_->deleteLater();
    }
    main_widget_ = nullptr;
}

QString VideoPlayer::key() {
    return metaObject()->className();
}

QWidget *VideoPlayer::widget(int) {
    main_widget_ = new QWidget;
    main_widget_->setObjectName("main_widget");
    main_widget_->setStyleSheet(loadStyleSheet());
    QVBoxLayout *main_layout = new QVBoxLayout(main_widget_);
    main_layout->setMargin(0);

    VideoWidget *video_widget_top =
        new VideoWidget(QApplication::applicationDirPath() + "/video/1.mp4");
    main_layout->addWidget(video_widget_top);

    return main_widget_;
}

QStringList VideoPlayer::help() {
    return helps_;
}

void VideoPlayer::receive(const SMod::ModMetaData &m) {
    // QMetaObject::invokeMethod(this, m.data, Q_ARG(QByteArray, m.data));
}

QString VideoPlayer::loadStyleSheet() {
    QString sheet;
    QFile file(":/videoPlayerResources/style/main.qss");
    if (file.open(QIODevice::ReadOnly)) {
        sheet = file.readAll();
    }
    file.close();
    return sheet;
}
