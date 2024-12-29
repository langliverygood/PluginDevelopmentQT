#include "menubutton.h"

MenuButton::MenuButton(QWidget *parent) : QPushButton(parent) {
    menu_ = new QMenu(this);
    menu_->setWindowFlags(menu_->windowFlags() | Qt::FramelessWindowHint |
                          Qt::NoDropShadowWindowHint);
    menu_->setAttribute(Qt::WA_TranslucentBackground, true);
}

QAction *MenuButton::addAction(const QString &text, const QString &command_args) {
    QAction *action = menu_->addAction(text);
    if (menu_->width() < text.length() * 16) {
        menu_->setFixedWidth(text.length() * 16);
    }
    connect(action, &QAction::triggered, this, [=]() {
        QStringList strs = command_args.trimmed().split(" ");
        QStringList args;
        for (int i = 0; i < strs.length(); i++) {
            if (strs.at(i).isEmpty() || i == 0) {
                continue;
            }
            args << strs.at(i);
        }
        CommandLine *cmd = new CommandLine;
        cmd->runexe(strs.at(0), args);
    });

    return action;
}

void MenuButton::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    if (event->type() == QEvent::MouseButtonPress && menu_->actions().size() != 0) {
        menu_->popup(mapToGlobal(QPoint(width() + 10, 0)));

    } else {
        QPushButton::mousePressEvent(event);
    }
}

void MenuButton::paintEvent(QPaintEvent *) {
    QStyleOption opt;
    opt.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}
