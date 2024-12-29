#include "tilemap.h"

Tilemap::Tilemap(QObject *parent) : QObject(parent) {}

Tilemap::~Tilemap() {
    if (main_widget_) {
        main_widget_->deleteLater();
    }
    main_widget_ = nullptr;
}

QString Tilemap::key() {
    return metaObject()->className();
}

QWidget *Tilemap::widget(int) {
    main_widget_ = new QWidget;
    main_widget_->setObjectName("main_widget");
    main_widget_->setStyleSheet(loadStyleSheet());
    QVBoxLayout *main_layout = new QVBoxLayout(main_widget_);
    main_layout->setMargin(0);

    // 加载卫星地图
    QTileMapWidget *tilemap = new QTileMapWidget();
    QString tilemap_file = QApplication::applicationDirPath() +
                           "/tilemap/卫星(影像).谷歌-永暑岛-L0-L18-分辨率0.59米.mbtiles";
    float centerPointLon = 112.865964;
    float centerPointLat = 9.555737;
    int minLevel = 15;
    int maxLevel = 18;
    tilemap->SetFrozenDrag(false);
    tilemap->SetReversalMode(true);
    tilemap->ConnectSQLiteDB(tilemap_file);
    tilemap->MoveToPoint(Coord_LL(centerPointLon, centerPointLat), minLevel);
    tilemap->LimitLevel(minLevel, maxLevel);

    main_layout->addWidget(tilemap);
    return main_widget_;
}

QStringList Tilemap::help() {
    return helps_;
}

void Tilemap::receive(const SMod::ModMetaData &) {
    // QMetaObject::invokeMethod(this, m.data, Q_ARG(QByteArray, m.data));
}

QString Tilemap::loadStyleSheet() {
    QString sheet;
    QFile file(":/tilemapResources/style/main.qss");
    if (file.open(QIODevice::ReadOnly)) {
        sheet = file.readAll();
    }
    file.close();
    return sheet;
}
