#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QPluginLoader>

#include "smodinterface.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    SMod::SModInterface *pluginLoader(const QString &name);
signals:

public slots:
};

#endif  // MAINWINDOW_H
