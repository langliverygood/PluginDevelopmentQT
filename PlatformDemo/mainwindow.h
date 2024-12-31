#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFont>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    QWidget* initUI();
    QString loadStyleSheet();
    QFont loadFonts();
    void addPlugins();
    void jump2Page(int index=0);
    void changeStacked(int index=0);

};

#endif // MAINWINDOW_H
