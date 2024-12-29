#ifndef MENUBUTTON_H
#define MENUBUTTON_H

#include <QDebug>
#include <QDesktopServices>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QStyleOption>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "commandline.h"

class MenuButton : public QPushButton {
    Q_OBJECT
public:
    explicit MenuButton(QWidget *parent = nullptr);
    QAction *addAction(const QString &text, const QString &command_args);

protected:
    // 重载鼠标点击事件来弹出菜单
    void mousePressEvent(QMouseEvent *event) override;

private:
    QMenu *menu_;
    void paintEvent(QPaintEvent *event) override;
};

#endif  // MENUBUTTON_H
