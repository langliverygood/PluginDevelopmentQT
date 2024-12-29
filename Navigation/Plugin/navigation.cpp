#include "navigation.h"

Navigation::Navigation(QObject *parent) : QObject(parent) {
    // 注册字体
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/navigationResources/font/HarmonyOS_Sans_SC_Regular.ttf"));
}

Navigation::~Navigation() {
    if (main_widget_) {
        main_widget_->deleteLater();
    }
    main_widget_ = nullptr;
}

QString Navigation::key() {
    return metaObject()->className();
}

QWidget *Navigation::widget(int) {
    main_widget_ = new QWidget;
    main_widget_->setObjectName("main_widget");
    main_widget_->setStyleSheet(loadStyleSheet());

    QVBoxLayout *main_layout = new QVBoxLayout(main_widget_);
    main_layout->setContentsMargins(0, 32, 0, 41);
    // logo
    QVBoxLayout *logo_layout = new QVBoxLayout;
    MenuButton *logo_btn = new MenuButton();
    logo_btn->setObjectName(QString("logo_btn"));
    logo_layout->addWidget(logo_btn);
    logo_layout->setAlignment(logo_btn, Qt::AlignHCenter);
    // logo_layout->addSpacing(10);
    QLabel *logo_label = new QLabel();
    logo_label->setObjectName(QString("logo_label"));
    logo_label->setText(QString("logo"));
    logo_layout->addWidget(logo_label);
    logo_layout->setAlignment(logo_label, Qt::AlignHCenter);
    main_layout->addLayout(logo_layout);
    main_layout->setAlignment(logo_layout, Qt::AlignHCenter);
    main_layout->addStretch();

    // 标题1
    QVBoxLayout *layout_1 = new QVBoxLayout;
    MenuButton *btn_1 = new MenuButton();
    btn_1->setObjectName(QString("label1_btn"));
    layout_1->addWidget(btn_1);
    layout_1->setAlignment(btn_1, Qt::AlignHCenter);
    layout_1->addSpacing(10);
    QLabel *label_1 = new QLabel();
    label_1->setObjectName(QString("label1"));
    label_1->setText(QString("标题1"));
    layout_1->addWidget(label_1);
    layout_1->setAlignment(label_1, Qt::AlignHCenter);
    main_layout->addLayout(layout_1);
    main_layout->setAlignment(layout_1, Qt::AlignHCenter);
    main_layout->addSpacing(40);

    // 标题2
    QVBoxLayout *layout_2 = new QVBoxLayout;
    MenuButton *btn_2 = new MenuButton();
    btn_2->setObjectName(QString("label2_btn"));
    layout_2->addWidget(btn_2);
    layout_2->setAlignment(btn_2, Qt::AlignHCenter);
    layout_2->addSpacing(10);
    QLabel *label_2 = new QLabel();
    label_2->setObjectName(QString("label2"));
    label_2->setText(QString("标题2"));
    layout_2->addWidget(label_2);
    layout_2->setAlignment(label_2, Qt::AlignHCenter);
    main_layout->addLayout(layout_2);
    main_layout->setAlignment(layout_2, Qt::AlignHCenter);
    main_layout->addSpacing(40);

    // 标题3
    QVBoxLayout *layout_3 = new QVBoxLayout;
    MenuButton *btn_3 = new MenuButton();
    btn_3->setObjectName(QString("label3_btn"));
    layout_3->addWidget(btn_3);
    layout_3->setAlignment(btn_3, Qt::AlignHCenter);
    layout_3->addSpacing(10);
    QLabel *label_3 = new QLabel();
    label_3->setObjectName(QString("label3"));
    label_3->setText(QString("标题3"));
    layout_3->addWidget(label_3);
    layout_3->setAlignment(label_3, Qt::AlignHCenter);
    main_layout->addLayout(layout_3);
    main_layout->setAlignment(layout_3, Qt::AlignHCenter);
    main_layout->addSpacing(40);

    // 标题4
    QVBoxLayout *layout_4 = new QVBoxLayout;
    MenuButton *btn_4 = new MenuButton();
    btn_4->setObjectName(QString("label4_btn"));
    layout_4->addWidget(btn_4);
    layout_4->setAlignment(btn_4, Qt::AlignHCenter);
    layout_4->addSpacing(10);
    QLabel *label_4 = new QLabel();
    label_4->setObjectName(QString("label4"));
    label_4->setText(QString("标题4"));
    layout_4->addWidget(label_4);
    layout_4->setAlignment(label_4, Qt::AlignHCenter);
    main_layout->addLayout(layout_4);
    main_layout->setAlignment(layout_4, Qt::AlignHCenter);
    main_layout->addSpacing(40);

    // 标题5
    QVBoxLayout *layout_5 = new QVBoxLayout;
    MenuButton *btn_5 = new MenuButton();
    btn_5->setObjectName(QString("label5_btn"));
    layout_5->addWidget(btn_5);
    layout_5->setAlignment(btn_5, Qt::AlignHCenter);
    layout_5->addSpacing(10);
    QLabel *label_5 = new QLabel();
    label_5->setObjectName(QString("label5"));
    label_5->setText(QString("标题5"));
    layout_5->addWidget(label_5);
    layout_5->setAlignment(label_5, Qt::AlignHCenter);
    main_layout->addLayout(layout_5);
    main_layout->setAlignment(layout_5, Qt::AlignHCenter);
    main_layout->addStretch();

    // 设置
    MenuButton *setting_btn = new MenuButton;
    setting_btn->setObjectName(QString("setting_btn"));
    main_layout->addWidget(setting_btn);
    main_layout->setAlignment(setting_btn, Qt::AlignHCenter);
    connect(setting_btn, &MenuButton::clicked, this, &Navigation::onSettingButtonClick);

    // 测试代码
    btn_4->addAction("画图", "mspaint.exe");
    btn_5->addAction("记事本", "notepad");
    btn_5->addAction("计算", "Calc.exe");
    btn_3->addAction("计算gggggggggggggggggdsf嘎嘎嘎嘎嘎嘎嘎", "Calc.exe");
    btn_3->addAction("计    算", "Calc.exe");
    btn_3->addAction("计dasd算", "Calc.exe");
    btn_3->addAction("计", "Calc.exe");
    btn_3->addAction("ping", "ping -t 127.0.0.1");
    return main_widget_;
}

QStringList Navigation::help() {
    return helps_;
}

void Navigation::receive(const SMod::ModMetaData &m) {
    // QMetaObject::invokeMethod(this, m.data, Q_ARG(QByteArray, m.data));
    QJsonDocument doc = QJsonDocument::fromJson(m.data);
    if (!doc.isObject()) {
        qDebug() << " Not an object";
        return;
    }
    QJsonObject obj = doc.object();
    QStringList keys = obj.keys();

    QString type, type_chinese, name, path;
    // 获取 key - value
    for (int i = 0; i < keys.size(); i++) {
        QString key = keys[i];
        QJsonValue value = obj.value(key);
        if (value.isString()) {
            if (key == "type") {
                type = value.toString();
            } else if (key == "name") {
                name = value.toString();
            } else if (key == "path") {
                path = value.toString();
            }
        }
    }
    if (!type.isEmpty() && !name.isEmpty() && !path.isEmpty()) {
        MenuButton *button = main_widget_->findChild<MenuButton *>(type);
        if (button != nullptr) {
            button->addAction(name, path);
        }
    }
    return;
}

QString Navigation::loadStyleSheet() {
    QString sheet;
    QFile file(":/navigationResources/style/main.qss");
    if (file.open(QIODevice::ReadOnly)) {
        sheet = file.readAll();
    }
    file.close();
    return sheet;
}

void Navigation::onSettingButtonClick() {
    SMod::ModMetaData data;
    data.from = this->key();  // 消息来源 -> 插件 key(UUID)
    data.dest = "main";       // 消息去向 -> 主界面
    data.data = QString("").toUtf8();
    if (open_main_) {
        data.method = "mainBodyPageShow";
    } else {
        data.method = "naviPlugManager";
    }
    open_main_ = !open_main_;
    emit this->send(data);
}
