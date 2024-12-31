#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    this->setMinimumSize(800, 600);
    // 加载字体
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/platformdemoResources/font/HarmonyOS_Sans_SC_Regular.ttf"));

    QWidget *main_widget = new QWidget;
    this->setCentralWidget(main_widget);
    main_widget->setObjectName("main_widget");
    main_widget->setStyleSheet(loadStyleSheet());
    QHBoxLayout *main_layout = new QHBoxLayout(main_widget);
    main_layout->setMargin(0);
    // 加载插件
    int plugins_size = Manager->loadCores();
    qDebug() << QString("已加载 %1 个插件").arg(plugins_size);
    foreach (SMod::SModInterface *var, Manager->all()) { qDebug() << var->key(); }
    // 瓦片图
    SMod::SModInterface *tilemap_plugin = Manager->find("Tilemap");
    if (tilemap_plugin) {
        main_layout->addWidget(tilemap_plugin->widget());
    }
    QVBoxLayout *right_layout = new QVBoxLayout;
    right_layout->setMargin(0);
    main_layout->addLayout(right_layout);
    // 三维模型
    SMod::SModInterface *model3d_plugin = Manager->find("Model3D");
    if (model3d_plugin) {
        right_layout->addWidget(model3d_plugin->widget());
    }
    // 视频
    SMod::SModInterface *video_plugin = Manager->find("VideoPlayer");
    if (video_plugin) {
        QWidget *video_widget = video_plugin->widget();
        video_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        right_layout->addWidget(video_widget);
    }

    connect(Manager, &PluginsManager::send, this, [=](const SMod::ModMetaData &datagram) {});
}

MainWindow::~MainWindow() {}

QString MainWindow::loadStyleSheet() {
    QString sheet;
    QFile file(":/platformdemoResources/style/main.qss");
    if (file.open(QIODevice::ReadOnly)) {
        sheet = file.readAll();
    }
    file.close();
    return sheet;
}
