#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QLayout>

#include "smodinterface.h"
#include "videohandler.h"

class VideoPlayer : public QObject, public SMod::SModInterface {
    Q_OBJECT
    Q_INTERFACES(SMod::SModInterface)
    Q_PLUGIN_METADATA(IID SMODINTERFACE_IID FILE "videoplayer.json")
public:
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer();
    QString key() override;
    QWidget *widget(int index = 0) override;
    QStringList help() override;
    void receive(const SMod::ModMetaData &) override;

private:
    QWidget *main_widget_;
    QStringList helps_;

    QString loadStyleSheet();
signals:
    void send(const SMod::ModMetaData &) override;
};

#endif  // VIDEOPLAYER_H
