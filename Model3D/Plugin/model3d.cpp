#include "model3d.h"

Model3D::Model3D(QObject *parent) : QObject(parent) {}

Model3D::~Model3D() {
    if (main_widget_) {
        main_widget_->deleteLater();
    }
    main_widget_ = nullptr;
}

QString Model3D::key() {
    return metaObject()->className();
}

QWidget *Model3D::widget(int) {
    main_widget_ = new QWidget;
    main_widget_->setObjectName("main_widget");
    main_widget_->setStyleSheet(loadStyleSheet());

    QVBoxLayout *main_layout = new QVBoxLayout(main_widget_);
    main_layout->setMargin(0);

    AppGLWidget *gl_widget = new AppGLWidget();
    gl_widget->loadFile(QApplication::applicationDirPath() + "/model/ship.stl");
    main_layout->addWidget(gl_widget);

    return main_widget_;
}

QStringList Model3D::help() {
    return helps_;
}

void Model3D::receive(const SMod::ModMetaData &m) {
    // QMetaObject::invokeMethod(this, m.data, Q_ARG(QByteArray, m.data));
}

QString Model3D::loadStyleSheet() {
    QString sheet;
    QFile file(":/Model3DResources/style/main.qss");
    if (file.open(QIODevice::ReadOnly)) {
        sheet = file.readAll();
    }
    file.close();
    return sheet;
}
