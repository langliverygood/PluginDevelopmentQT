#ifndef TILEMAP_H
#define TILEMAP_H

#include <QDebug>
#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPushButton>

#include "qtilemapwidget.h"
#include "smodinterface.h"

class Tilemap : public QObject, public SMod::SModInterface {
    Q_OBJECT
    Q_INTERFACES(SMod::SModInterface)
    Q_PLUGIN_METADATA(IID SMODINTERFACE_IID FILE "tilemap.json")
public:
    explicit Tilemap(QObject *parent = nullptr);
    ~Tilemap();

    QString key() override;
    QWidget *widget(int index = 0) override;
    QStringList help() override;
    void receive(const SMod::ModMetaData &) override;

private:
    QWidget *main_widget_;
    QStringList helps_;

private:
    QString loadStyleSheet();

signals:
    void send(const SMod::ModMetaData &) override;
};

#endif  // TILEMAP_H
