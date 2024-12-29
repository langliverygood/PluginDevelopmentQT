#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <QDebug>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QLabel>
#include <QLayout>

#include "menubutton.h"
#include "smodinterface.h"

class Navigation : public QObject, public SMod::SModInterface {
    Q_OBJECT
    Q_INTERFACES(SMod::SModInterface)
    Q_PLUGIN_METADATA(IID SMODINTERFACE_IID FILE "navigation.json")
public:
    explicit Navigation(QObject *parent = nullptr);
    ~Navigation();
    QString key() override;
    QWidget *widget(int index = 0) override;
    QStringList help() override;
    void receive(const SMod::ModMetaData &) override;

private:
    QWidget *main_widget_;
    QStringList helps_;
    bool open_main_ = false;

    QString loadStyleSheet();
private slots:
    void onSettingButtonClick();
signals:
    void send(const SMod::ModMetaData &) override;
};

#endif  // NAVIGATION_H
