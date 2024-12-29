#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <QDebug>
#include <QFile>
#include <QLayout>

#include "smodinterface.h"

class Example : public QObject, public SMod::SModInterface {
    Q_OBJECT
    Q_INTERFACES(SMod::SModInterface)
    Q_PLUGIN_METADATA(IID SMODINTERFACE_IID FILE "example.json")
public:
    explicit Example(QObject *parent = nullptr);
    ~Example();
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

#endif  // EXAMPLE_H
