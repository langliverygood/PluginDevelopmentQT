#include "example.h"

Example::Example(QObject *parent) : QObject(parent) {}

Example::~Example() {
    if (main_widget_) {
        main_widget_->deleteLater();
    }
    main_widget_ = nullptr;
}

QString Example::key() {
    return metaObject()->className();
}

QWidget *Example::widget(int index) {
    main_widget_ = new QWidget;
    main_widget_->setObjectName("main_widget");
    main_widget_->setStyleSheet(loadStyleSheet());

    return main_widget_;
}

QStringList Example::help() {
    return helps_;
}

void Example::receive(const SMod::ModMetaData &m) {
    // QMetaObject::invokeMethod(this, m.data, Q_ARG(QByteArray, m.data));
}

QString Example::loadStyleSheet() {
    QString sheet;
    QFile file(":/ExampleResources/style/main.qss");
    if (file.open(QIODevice::ReadOnly)) {
        sheet = file.readAll();
    }
    file.close();
    return sheet;
}
