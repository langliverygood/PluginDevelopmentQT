#ifndef MODEL3D_H
#define MODEL3D_H

#include <QDebug>
#include <QFile>
#include <QLayout>

#include "appGLWidget.h"
#include "smodinterface.h"

class Model3D : public QObject, public SMod::SModInterface {
    Q_OBJECT
    Q_INTERFACES(SMod::SModInterface)
    Q_PLUGIN_METADATA(IID SMODINTERFACE_IID FILE "model3d.json")
public:
    explicit Model3D(QObject *parent = nullptr);
    ~Model3D();
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

#endif  // MODEL3D_H
