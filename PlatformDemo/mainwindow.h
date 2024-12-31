#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFontDatabase>
#include <QLayout>
#include <QMainWindow>

#include "pluginsmanager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    QString loadStyleSheet();
};

#endif  // MAINWINDOW_H
