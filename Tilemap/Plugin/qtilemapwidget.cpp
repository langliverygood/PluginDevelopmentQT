/**********************************************/
/* This text coding format is UTF-8           */
/**********************************************/

#include "qtilemapwidget.h"

const double QTMW_EARTH_RADIUS = 6378137;  //地球半径
const int QTMW_TIMER_TICK = 1000;          //计时器工作间隔ms

bool QTileMapWidget::m_mode_Reversal = false;
QColor QTileMapWidget::m_backgroundColor = Qt::white;

int QTileMapWidget::ConvertTileX_Normal2Reversal(
    int x,
    int z)  //正常模式的瓦片X坐标转到翻转模式的瓦片X坐标
{
    int ret = x;
    int maxCount = qPow(2, z);
    int start = qFloor(maxCount / 2);
    if (start > 0) {
        ret = (start + x) % (maxCount);
    }
    return ret;
}

void QTileMapWidget::setBackgroundColor(QColor _color) {
    m_backgroundColor = _color;
    ShowDefaultBackground();
    UpdateDisplayMapBuffer_All();
}

void QTileMapWidget::set_CallBackFunc(CallBackFunc _func) {
    m_callBackFunc = _func;
}

QPoint QTileMapWidget::getWidgetPoint_LL2Widget(Coord_LL _pos, bool *_ok) {
    QPoint ret = ConvertCoord_LL2Widget(_pos, _ok);
    return ret;
}

void QTileMapWidget::DrawCallBackFunc() {
    if (m_callBackFunc != nullptr) {
        m_callBackFunc(this);
    }
}

QHash<QString, QByteArray> QTMW_AsyncPreload::static_PreloadLoadImageBuffer;

QTileMapWidget::QTileMapWidget(QWidget *parent) : QFrame(parent) {
    initMapBuffer();
    SQLiteCreate();
    setMouseTracking(true);  //设置鼠标不点击也触发鼠标数据
    connect(&m_AsyncPreload, SIGNAL(finished()), this, SLOT(slot_PreloadFinished()));

    m_AsyncLoadMgr = new QTMW_AsyncLoadMgr();
    m_AsyncLoadMgr->setParent(this);
    m_AsyncLoadMgr->start();

    connect(&m_timer_AutoLoadDataFromDB, SIGNAL(timeout()), this,
            SLOT(slot_timeout_AutoLoadDataFromDB()));
    m_timer_AutoLoadDataFromDB.start(QTMW_TIMER_TICK);
}

void QTileMapWidget::ConnectSQLiteDB(QString _path) {
    m_SQLiteDBPath = _path;
    m_isSQLiteDBValid = false;
    m_SQLiteDB.setDatabaseName(_path);
    if (!m_SQLiteDB.open()) {
        m_LastError = m_SQLiteDB.lastError().text();
    } else {
        m_LimitMaxLevel = -1;     //数据库最大有效等级
        m_LimitMinLevel = -1;     //数据库最小有效等级
        m_DBFormatVaild = false;  //数据库瓦片数量
        CleanAsyncLoadImageBuffer();
        LoadDBInfo();

        LimitLevel(m_DB_Info.minZoom, m_DB_Info.maxZoom);
        if (m_DBFormatVaild) {
            m_isSQLiteDBValid = true;

            m_CurrentMapIndex_Z = m_LimitMinLevel;
            m_tilemapIndexInBuffer0_X = 0;
            m_tilemapIndexInBuffer0_Y = 0;

            ShowDefaultBackground();
            UpdateDisplayMapBuffer_All();
            FixXY2WidgetSize();

            m_LastError.clear();
        }
    }
}

void QTileMapWidget::LoadDBInfo() {
    QSqlQuery que(m_SQLiteDB);

    //获取数据库瓦片的最小/最大等级
    QString minStr = QString("select value from metadata where name = 'minzoom'");
    que.prepare(minStr);
    que.exec();
    if (que.next()) {
        m_DB_Info.minZoom = que.value(0).toInt();
        if (m_DB_Info.minZoom == 0) {
            m_DB_Info.minZoom = 1;
        }
    } else {
        m_LastError = QString("metadata[minzoom] - ") + que.lastError().text();
        return;
    }

    QString maxStr = QString("select value from metadata where name = 'maxzoom'");
    que.prepare(maxStr);
    que.exec();
    if (que.next()) {
        m_DB_Info.maxZoom = que.value(0).toInt();
    } else {
        m_LastError = QString("metadata[maxzoom] - ") + que.lastError().text();
        return;
    }

    //获取瓦片尺寸
    QString str_tile_width = QString("select value from metadata where name = 'tile_width'");
    que.prepare(str_tile_width);
    que.exec();
    if (que.next()) {
        m_DB_Info.tile_width = que.value(0).toInt();
        m_SingleTilePixel = m_DB_Info.tile_width;
    } else {
        m_LastError = QString("metadata[tile_width] - ") + que.lastError().text();
        return;
    }

    QString str_tile_height = QString("select value from metadata where name = 'tile_height'");
    que.prepare(str_tile_height);
    que.exec();
    if (que.next()) {
        m_DB_Info.tile_height = que.value(0).toInt();
        m_SingleTilePixel = m_DB_Info.tile_height;

    } else {
        m_LastError = QString("metadata[tile_height] - ") + que.lastError().text();
        return;
    }

    //获取瓦片格式
    QString str_tileSize = QString("select value from metadata where name = 'format'");
    que.prepare(str_tileSize);
    que.exec();
    if (que.next()) {
        m_DB_Info.format = que.value(0).toString();
    } else {
        m_LastError = QString("metadata[format] - ") + que.lastError().text();
        return;
    }

    //判断查询是否有瓦片，主要是查询格式判断
    QString str_tileCount = QString(
        "select * from map where zoom_level >=0 and tile_column >=0 and tile_row >=0 Limit 1");
    que.prepare(str_tileCount);
    que.exec();
    if (que.next()) {
        m_DBFormatVaild = true;
    } else {
        if (que.lastError().text().length() <= 1) {
            m_LastError = QString("map[select] - select nothing");
        } else {
            m_LastError = QString("map[select] - ") + que.lastError().text();
        }
    }
}

Coord_LL QTileMapWidget::GetMousePosCoord_LL(bool *_ok) {
    Coord_LL ret;
    bool _retIsVaild = false;

    ret = ConvertCoord_Widget2LL(m_mousetCurrentPos, &_retIsVaild);

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }
    if (!_retIsVaild) {
        ret.lon = 0;
        ret.lat = 0;
    }

    return ret;
}

Coord_LLMS QTileMapWidget::GetMousePosCoord_LLMS(bool *_ok) {
    Coord_LLMS ret;
    bool _retIsVaild = false;
    ret = ConvertCoord_Widget2LLMS(m_mousetCurrentPos, &_retIsVaild);

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }

    if (!_retIsVaild) {
        ret.lonD = 0;
        ret.lonM = 0;
        ret.lonS = 0;
        ret.latD = 0;
        ret.latM = 0;
        ret.latS = 0;
    }
    return ret;
}

int QTileMapWidget::GetTileMapMaxLevelInDB() {
    return m_DB_Info.maxZoom;
}

int QTileMapWidget::GetTileMapMinLevelInDB() {
    return m_DB_Info.minZoom;
}

int QTileMapWidget::GetTileMapMaxLevelInLimit() {
    return m_LimitMaxLevel;
}

int QTileMapWidget::GetTileMapMinLevelInLimit() {
    return m_LimitMinLevel;
}

void QTileMapWidget::SetReversalMode(bool _reversal) {
    bool _ok = false;
    Coord_LL _WidgetCenter =
        ConvertCoord_Widget2LL(QPoint(this->width() / 2, this->height() / 2), &_ok);
    int _currentLevel = m_CurrentMapIndex_Z;

    m_mode_Reversal = _reversal;
    ConnectSQLiteDB(m_SQLiteDBPath);
    MoveToPoint(_WidgetCenter, _currentLevel);
}

bool QTileMapWidget::GetIsReversalMode() {
    return m_mode_Reversal;
}

QString QTileMapWidget::GetLastError() {
    return m_LastError;
}

int QTileMapWidget::GetCurrentLevelIndex() {
    return this->m_CurrentMapIndex_Z;
}

QTMW_DrawGraph *QTileMapWidget::AddGraph() {
    QTMW_DrawGraph *_newGraph = new QTMW_DrawGraph();
    _newGraph->setParent(this);
    int key = m_DrawGraphBuffer.count();
    m_DrawGraphBuffer.insert(key, _newGraph);
    return _newGraph;
}

void QTileMapWidget::SetFrozenDrag(bool frozen) {
    m_FrozenDrag = frozen;
}

void QTileMapWidget::LimitLevel(int minLevel, int maxLevel) {
    m_LimitMinLevel = minLevel;
    if (m_LimitMinLevel < 0) {
        m_LimitMinLevel = 0;
    }

    m_LimitMaxLevel = maxLevel;
    if (m_LimitMaxLevel > 20) {
        m_LimitMinLevel = 20;
    }
}

void QTileMapWidget::MoveToPoint(Coord_LL point) {
    MoveToPoint(point, m_CurrentMapIndex_Z);
}

void QTileMapWidget::MoveToPoint(Coord_LL point, int level) {
    m_Point_MoveToFocus = point;
    if (m_isSQLiteDBValid) {
        if (level < m_LimitMinLevel) {
            level = m_LimitMinLevel;
        }

        if (level > m_LimitMaxLevel) {
            level = m_LimitMaxLevel;
        }

        bool _ok = false;
        Coord_Tile _center = ConvertCoord_LL2Tile_Mercator(point, level, &_ok);
        if (_ok) {
            if (level != m_CurrentMapIndex_Z) {
                m_CurrentMapIndex_Z = level;
            }

            //把瓦片放到视口的最后，计算Buffer0
            int pixelX = this->width();
            int pixelY = this->height();
            int _tileCountX = pixelX / m_SingleTilePixel;
            int _tileCountY = pixelY / m_SingleTilePixel;

            while (_tileCountX * m_SingleTilePixel + this->width() / 2 >
                   (m_DisplayMapBufferSize_X - 1) * m_SingleTilePixel) {
                _tileCountX--;
            };
            while (_tileCountY * m_SingleTilePixel + this->height() / 2 >
                   (m_DisplayMapBufferSize_Y - 1) * m_SingleTilePixel) {
                _tileCountY--;
            };

            m_tilemapIndexInBuffer0_X = _center.x - _tileCountX;
            m_tilemapIndexInBuffer0_Y = _center.y - _tileCountY;

            if (m_tilemapIndexInBuffer0_X < 0) {
                m_tilemapIndexInBuffer0_X = 0;
                _tileCountX = _center.x;
            }
            if (m_tilemapIndexInBuffer0_Y < 0) {
                m_tilemapIndexInBuffer0_Y = 0;
                _tileCountY = _center.y;
            }

            ShowDefaultBackground();
            UpdateDisplayMapBuffer_All();
            FixXY2WidgetSize();

            //移动视口
            int newOffsetX = (m_SingleTilePixel * _tileCountX + _center.offsetLT_x) - pixelX / 2;
            int newOffsetY = (m_SingleTilePixel * _tileCountY + _center.offsetLT_y) - pixelY / 2;

            m_WidgetOffsetImage.setX(newOffsetX);
            m_WidgetOffsetImage.setY(newOffsetY);
        }
    }
}

void QTileMapWidget::Release() {
    StopPreload();
    m_isSQLiteDBValid = false;
    m_AsyncLoadMgr->terminate();
    m_AsyncLoadMgr->wait();
    m_SQLiteDB.close();
}

QPoint QTileMapWidget::GetMousePosCoord_Widget() {
    QPoint ret = m_mousetCurrentPos;
    return ret;
}

Coord_Tile QTileMapWidget::GetMousePosCoord_Tile(bool *_ok) {
    Coord_Tile ret;
    bool _retIsVaild = false;
    ret = ConvertCoord_Widget2Tile(m_mousetCurrentPos, &_retIsVaild);

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }
    if (!_retIsVaild) {
        ret.z = -1;
        ret.x = -1;
        ret.y = -1;
        ret.offsetLT_x = 0;
        ret.offsetLT_y = 0;
    }
    return ret;
}

QString QTileMapWidget::GetCurrentSQLiteDBPath() {
    return m_SQLiteDBPath;
}

void QTileMapWidget::ShowTileMapGrid(bool _show, QColor _color) {
    m_showTileMapGrid = _show;
    m_colorTileMapGrid = _color;
}

void QTileMapWidget::ShowTileMapCoord(bool _show, QColor _color) {
    m_showTileMapCoord = _show;
    m_colorTileMapCoord = _color;
}

void QTileMapWidget::ShowCenterCrossLine(bool _show, QColor _color) {
    m_showCenterCrossLine = _show;
    m_colorCenterCrossLine = _color;
}

bool QTileMapWidget::GetIsSQLiteDBValid() {
    return m_isSQLiteDBValid;
}

void QTileMapWidget::initMapBuffer() {
    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        QVector<QPixmap> _VecY;
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            QPixmap _pixmap;
            _VecY.push_back(_pixmap);
        }
        m_DisplayMapBuffer.push_back(_VecY);
    }
}

void QTileMapWidget::paintEvent(QPaintEvent *e) {
    RespondStyleSheet();  //修正QWidget派生类setStyleSheet无效问题

    if (m_isSQLiteDBValid) {
        if (!m_DisplayMapBuffer.isEmpty()) {
            QPainter painter(this);

            bool _ok = false;
            Coord_LL _WidgetCenter =
                ConvertCoord_Widget2LL(QPoint(this->width() / 2, this->height() / 2), &_ok);
            if (_ok) {
                m_Point_WidgetCenter = _WidgetCenter;
            }

            if (!m_FrozenDrag) {
                if (qPow(2, m_CurrentMapIndex_Z) * m_SingleTilePixel < this->width()) {
                    int centerX =
                        (this->width() - (qPow(2, m_CurrentMapIndex_Z) * m_SingleTilePixel)) * 0.5;
                    m_WidgetOffsetImage.setX(-centerX);
                }

                if (qPow(2, m_CurrentMapIndex_Z) * m_SingleTilePixel < this->height()) {
                    int centerY =
                        (this->height() - (qPow(2, m_CurrentMapIndex_Z) * m_SingleTilePixel)) * 0.5;
                    m_WidgetOffsetImage.setY(-centerY);
                }
            }

            for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
                for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
                    QPixmap _tmpPixmap = m_DisplayMapBuffer[x][y];

                    QPoint _p_Image(m_SingleTilePixel * x, m_SingleTilePixel * y);
                    QPoint _p_Widget = ConvertCoord_Image2Widget(_p_Image);
                    painter.drawPixmap(_p_Widget.x(), _p_Widget.y(), m_SingleTilePixel,
                                       m_SingleTilePixel, _tmpPixmap);
                }
            }
        }
    }

    // paint DrawGraph
    for (QMap<int, QTMW_DrawGraph *>::iterator _iterLoopGraph = m_DrawGraphBuffer.begin();
         _iterLoopGraph != m_DrawGraphBuffer.end(); _iterLoopGraph++) {
        _iterLoopGraph.value()->draw();
    }

    if (m_showTileMapGrid) {
        DrawTileMapGrid();
    }

    if (m_showTileMapCoord) {
        DrawTileCoord();
    }

    if (m_showCenterCrossLine) {
        DrawCenterCrossLine();
    }

    DrawCallBackFunc();  //调用外部的回调绘制函数
    DrawDebugInfo();     //调试打印信息

    update();
}

void QTileMapWidget::wheelEvent(QWheelEvent *event) {
    emit signal_wheelEvent(event);
    if (!m_isSQLiteDBValid) {
        return;
    }

    bool _ok = false;
    Coord_Tile _coordTileNow;

    //滚动之后，通过移动图片使得鼠标对应位置的经纬度保持不变
    _coordTileNow = ConvertCoord_Widget2Tile(m_mousetCurrentPos, &_ok);  //鼠标位置的瓦片坐标

    if (!_ok) {
        //滚动之后，鼠标位置异常则使用中心缩放
        QPoint _centerPos;
        _centerPos.setX(this->width() / 2);
        _centerPos.setY(this->height() / 2);
        _coordTileNow = ConvertCoord_Widget2Tile(_centerPos, &_ok);
    }

    if (m_FrozenDrag)  //如果是冻结拖拽，以聚焦点缩放
    {
        QPoint _FocusPos = ConvertCoord_LL2Widget(m_Point_MoveToFocus, &_ok);
        _coordTileNow = ConvertCoord_Widget2Tile(_FocusPos, &_ok);
    }

    if (_ok) {
        int newOffsetX = 0;
        int newOffsetY = 0;
        int Level_BeforeWheel = m_CurrentMapIndex_Z;
        int Level_AfterWheel = Level_BeforeWheel;

        if (event->delta() > 0)  //上滚
        {
            if (m_CurrentMapIndex_Z >= 0 && m_CurrentMapIndex_Z < m_LimitMaxLevel) {
                Level_AfterWheel = Level_BeforeWheel + 1;

                //----- 情况1 如果升级之前，缓冲区已经填充满了,则更新缓存区
                if (qPow(2, Level_BeforeWheel) >= m_DisplayMapBufferSize_X) {
                    newOffsetX = FixX_LevelUp_BeforeFullAfterFull(_coordTileNow);
                };

                if (qPow(2, Level_BeforeWheel) >= m_DisplayMapBufferSize_Y) {
                    newOffsetY = FixY_LevelUp_BeforeFullAfterFull(_coordTileNow);
                };

                //----- 情况2 如果升级之后，缓冲区仍然填充不满或刚好填满,继续填充
                if (qPow(2, Level_AfterWheel) <= m_DisplayMapBufferSize_X) {
                    newOffsetX = FixX_LevelUp_BeforeEmptyAfterEmpty(_coordTileNow);
                };
                if (qPow(2, Level_AfterWheel) <= m_DisplayMapBufferSize_Y) {
                    newOffsetY = FixY_LevelUp_BeforeEmptyAfterEmpty(_coordTileNow);
                };

                //----- 情况3 如果升级之前，缓冲区未满，升级之后，缓冲区满了
                if ((qPow(2, Level_BeforeWheel) < m_DisplayMapBufferSize_X) &&
                    (qPow(2, Level_AfterWheel) > m_DisplayMapBufferSize_X)) {
                    newOffsetX = FixX_LevelUp_BeforeEmptyAfterFull(_coordTileNow);
                };

                if ((qPow(2, Level_BeforeWheel) < m_DisplayMapBufferSize_Y) &&
                    (qPow(2, Level_AfterWheel) > m_DisplayMapBufferSize_Y)) {
                    newOffsetY = FixY_LevelUp_BeforeEmptyAfterFull(_coordTileNow);
                };

                //更新瓦片
                async_CleanCurrentThread();
                m_CurrentMapIndex_Z = Level_AfterWheel;
                UpdateDisplayMapBuffer_All_Gradual();

                m_WidgetOffsetImage.setX(newOffsetX);
                m_WidgetOffsetImage.setY(newOffsetY);
                FixXY2WidgetSize();
                QTimer::singleShot(QTMW_TIMER_TICK, this, SLOT(slot_SingleShotInWheelEvent()));
            }
        } else  //下滚
        {
            if (m_CurrentMapIndex_Z > m_LimitMinLevel) {
                Level_AfterWheel = Level_BeforeWheel - 1;

                //----- 情况4 如果降级之后，缓冲区仍满填充满了,则更新缓存区
                if (qPow(2, Level_AfterWheel) >= m_DisplayMapBufferSize_X) {
                    newOffsetX = FixX_LevelDown_BeforeFullAfterFull(_coordTileNow);
                };

                if (qPow(2, Level_AfterWheel) >= m_DisplayMapBufferSize_Y) {
                    newOffsetY = FixY_LevelDown_BeforeFullAfterFull(_coordTileNow);
                }

                //----- 情况5 如果降级之前，缓冲区已经填充不满了或刚好满了
                if (qPow(2, Level_BeforeWheel) <= m_DisplayMapBufferSize_X) {
                    newOffsetX = FixX_LevelDown_BeforeEmptyAfterEmpty(_coordTileNow);
                };

                if (qPow(2, Level_BeforeWheel) <= m_DisplayMapBufferSize_Y) {
                    newOffsetY = FixY_LevelDown_BeforeEmptyAfterEmpty(_coordTileNow);
                }

                //----- 情况6 如果降级之前，缓冲区是满的，降级之后缓冲区不满了
                if ((qPow(2, Level_BeforeWheel) > m_DisplayMapBufferSize_X) &&
                    (qPow(2, Level_AfterWheel) < m_DisplayMapBufferSize_X)) {
                    newOffsetX = FixX_LevelDown_BeforeFullAfterEmpty(_coordTileNow);
                };

                if ((qPow(2, Level_BeforeWheel) > m_DisplayMapBufferSize_Y) &&
                    (qPow(2, Level_AfterWheel) < m_DisplayMapBufferSize_Y)) {
                    newOffsetY = FixY_LevelDown_BeforeFullAfterEmpty(_coordTileNow);
                };

                //更新瓦片
                async_CleanCurrentThread();
                m_CurrentMapIndex_Z = Level_AfterWheel;
                UpdateDisplayMapBuffer_All();

                //修正Widget位置
                m_WidgetOffsetImage.setX(newOffsetX);
                m_WidgetOffsetImage.setY(newOffsetY);
                FixXY2WidgetSize();
            }
        }
    }

    if (m_FrozenDrag)  //如果是冻结拖拽，以中心点缩放
    {
        MoveToPoint(m_Point_MoveToFocus);
    }
}

void QTileMapWidget::mouseMoveEvent(QMouseEvent *event) {
    emit signal_mouseMoveEvent(event);
    m_mousetPreviousPos = m_mousetCurrentPos;
    m_mousetCurrentPos = event->pos();

    if (!m_FrozenDrag && m_mouseLeftButtonDown) {
        DragMoveProcess();
    }
}

void QTileMapWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    emit signal_mouseDoubleClickEvent(event);
}

void QTileMapWidget::mousePressEvent(QMouseEvent *event) {
    emit signal_mousePressEvent(event);

    if (event->button() == Qt::LeftButton) {
        m_mouseLeftButtonDown = true;
    } else if (event->button() == Qt::RightButton) {
        m_mouseRightButtonDown = true;
    };
}

void QTileMapWidget::mouseReleaseEvent(QMouseEvent *event) {
    emit signal_mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton) {
        m_mouseLeftButtonDown = false;
    } else if (event->button() == Qt::RightButton) {
        m_mouseRightButtonDown = false;
    };
}

void QTileMapWidget::resizeEvent(QResizeEvent *event) {
    if (m_isSQLiteDBValid) {
        if (m_FrozenDrag) {
            MoveToPoint(m_Point_WidgetCenter);
        } else {
            //缩放后，修正Widget位置让屏幕显示的区域不变化
            int width_Change = event->size().width() - event->oldSize().width();
            int height_Change = event->size().height() - event->oldSize().height();
            int newOffsetX = m_WidgetOffsetImage.x() - width_Change / 2;
            int newOffsetY = m_WidgetOffsetImage.y() - height_Change / 2;
            m_WidgetOffsetImage.setX(newOffsetX);
            m_WidgetOffsetImage.setY(newOffsetY);
            FixXY2WidgetSize();
        }
    }
}

void QTileMapWidget::SQLiteCreate() {
    m_SQLiteDB = QSqlDatabase::addDatabase("QSQLITE", "QTMW");
}

QPixmap QTileMapWidget::LoadTileMapImage(int z, int x, int y) {
    QPixmap _retPixMap;
    QSqlQuery que(m_SQLiteDB);

    //坐标修正
    if (m_mode_Reversal) {
        x = ConvertTileX_Normal2Reversal(x, z);
    }
    y = qPow(2, z) - 1 - y;
    QString selectStr = QString(
                            "select tile_data from images where tile_id=(select tile_id from "
                            "map where tile_column=%1 and tile_row=%2 and zoom_level=%3)")
                            .arg(x)
                            .arg(y)
                            .arg(z);
    que.prepare(selectStr);
    que.exec();

    if (que.next()) {
        QByteArray image = que.value(0).toByteArray();
        _retPixMap.loadFromData(image);
    }
    return _retPixMap;
}

void QTileMapWidget::AsynLoadTileMapImage(int buffer_x, int buffer_y, int tile_z, int tile_x,
                                          int tile_y) {
    if (QTMW_AsyncLoadItemState_None == m_AsyncLoadImageBuffer[tile_z][tile_x][tile_y].state) {
        m_AsyncLoadImageBuffer[tile_z][tile_x][tile_y].state = QTMW_AsyncLoadItemState_Loading;
        QTMW_AsyncLoadImage *_AsyncLoadImage = new QTMW_AsyncLoadImage();
        connect(_AsyncLoadImage, SIGNAL(loaded(int, int, int, int, int, QByteArray)), this,
                SLOT(slot_AsyncLoadImageLoaded(int, int, int, int, int, QByteArray)));
        _AsyncLoadImage->setParm(m_SQLiteDBPath, buffer_x, buffer_y, tile_z, tile_x, tile_y);
        async_AddThread(_AsyncLoadImage);
    }
}

void QTileMapWidget::ShowDefaultBackground() {
    int r = m_backgroundColor.red();
    int g = m_backgroundColor.green();
    int b = m_backgroundColor.blue();
    QString backgroundStyleSheet =
        QString("background-color: rgb(%1, %2, %3)").arg(r).arg(g).arg(b);
    setStyleSheet(backgroundStyleSheet);
}

void QTileMapWidget::RespondStyleSheet() {
    //修正QWidget派生类setStyleSheet无效问题
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void QTileMapWidget::DrawTileMapGrid() {
    QPainter painter(this);
    //绘制坐标系
    QPen pen(m_colorTileMapGrid, 1);
    //绘制区间
    painter.setPen(pen);

    for (int x = -m_DisplayMapBufferSize_X; x < m_DisplayMapBufferSize_X; x++) {
        QPoint _p_Image(m_SingleTilePixel * x, 0);
        QPoint _p_Widget = ConvertCoord_Image2Widget(_p_Image);
        painter.drawLine(_p_Widget.x(), 0, _p_Widget.x(), this->height());
    }

    for (int y = -m_DisplayMapBufferSize_Y; y < m_DisplayMapBufferSize_Y; y++) {
        QPoint _p_Image(0, m_SingleTilePixel * y);
        QPoint _p_Widget = ConvertCoord_Image2Widget(_p_Image);
        painter.drawLine(0, _p_Widget.y(), this->width(), _p_Widget.y());
    }
}

void QTileMapWidget::DrawTileCoord() {
    QPainter painter(this);
    //绘制坐标系
    QPen pen(m_colorTileMapGrid, 1);
    //绘制区间
    painter.setPen(pen);

    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            Coord_Tile _coordTile;
            _coordTile.z = m_CurrentMapIndex_Z;
            _coordTile.x = m_tilemapIndexInBuffer0_X + x;
            _coordTile.y = m_tilemapIndexInBuffer0_Y + y;
            _coordTile.offsetLT_x = m_SingleTilePixel / 2;
            _coordTile.offsetLT_y = m_SingleTilePixel / 2;

            bool _ok = false;
            QPoint _coordWidget = ConvertCoord_Tile2Widget(_coordTile, &_ok);
            if (_ok) {
                if (m_mode_Reversal) {
                    _coordTile.x = ConvertTileX_Normal2Reversal(_coordTile.x, _coordTile.z);
                }

                QString str_coord;
                str_coord.append(QString::number(_coordTile.z));
                str_coord.append("_");
                str_coord.append(QString::number(_coordTile.x));
                str_coord.append("_");
                str_coord.append(QString::number(_coordTile.y));

                int _strOffset = str_coord.length() * 3;
                QPoint _fix = _coordWidget;
                _fix.setX(_fix.x() - _strOffset);
                painter.drawText(_fix, str_coord);
            }
        }
    }
}

void QTileMapWidget::DrawCenterCrossLine() {
    QPainter painter(this);
    //绘制坐标系
    QPen pen(m_colorCenterCrossLine, 1);
    //绘制区间
    painter.setPen(pen);

    //中线
    painter.drawLine(0, this->height() / 2, this->width(), this->height() / 2);
    painter.drawLine(this->width() / 2, 0, this->width() / 2, this->height());
}

void QTileMapWidget::PreloadTileMapImages(Coord_LL _leftTop, Coord_LL _rightBottom, int _minZoom,
                                          int _maxZoom) {
    QVector<Coord_Tile> vec_PreloadCoord;
    for (int z = _minZoom; z <= _maxZoom; z++) {
        bool _ok = false;
        Coord_Tile _LT, _RB;
        _LT = ConvertCoord_LL2Tile_Mercator(_leftTop, z, &_ok);
        _RB = ConvertCoord_LL2Tile_Mercator(_rightBottom, z, &_ok);
        if (_ok) {
            int _minX, _minY, _maxX, _maxY;
            if (_LT.x > _RB.x) {
                _minX = _RB.x;
                _maxX = _LT.x;
            } else {
                _minX = _LT.x;
                _maxX = _RB.x;
            }

            if (_LT.y > _RB.y) {
                _minY = _RB.y;
                _maxY = _LT.y;
            } else {
                _minY = _LT.y;
                _maxY = _RB.y;
            }

            for (int x = _minX; x <= _maxX; x++) {
                for (int y = _minY; y <= _maxY; y++) {
                    Coord_Tile _preloadCoord;
                    _preloadCoord.x = x;
                    _preloadCoord.y = y;
                    _preloadCoord.z = z;
                    vec_PreloadCoord.push_back(_preloadCoord);
                }
            }
        }
    }
    m_AsyncPreload.setParm(m_SQLiteDBPath, vec_PreloadCoord);
    m_AsyncPreload.start();
}

void QTileMapWidget::GetPreloadProgress(int *current, int *max) {
    int _current = 0;
    int _max = 0;
    m_AsyncPreload.getPreloadProgress(&_current, &_max);
    *current = _current;
    *max = _max;
}

void QTileMapWidget::StopPreload() {
    if (!m_AsyncPreload.isFinished()) {
        m_AsyncPreload.terminate();
        m_AsyncPreload.wait();
    }
}

void QTileMapWidget::slot_PreloadFinished() {
    QHash<QString, QByteArray> _preloadData = m_AsyncPreload.static_PreloadLoadImageBuffer;
    QHash<QString, QByteArray>::iterator _iterLoop = _preloadData.begin();
    for (; _iterLoop != _preloadData.end(); _iterLoop++) {
        QString _strKey = _iterLoop.key();
        QStringList _strList = _strKey.split('_');
        if (_strList.count() > 2) {
            int z, x, y;
            z = _strList.at(0).toInt();
            x = _strList.at(1).toInt();
            y = _strList.at(2).toInt();
            m_AsyncLoadImageBuffer[z][x][y].imageArray = _iterLoop.value();
            m_AsyncLoadImageBuffer[z][x][y].state = QTMW_AsyncLoadItemState_Loaded;
        }
    }
}

void QTileMapWidget::DrawDebugInfo() {
    return;
    QPainter painter(this);
    //绘制坐标系
    QPen pen(Qt::magenta, 1);
    //绘制区间
    painter.setPen(pen);

    QString str_Debug;
    str_Debug.append("m_WidgetOffsetImage=");
    str_Debug.append(QString::number(m_WidgetOffsetImage.x()));
    str_Debug.append(",");
    str_Debug.append(QString::number(m_WidgetOffsetImage.y()));
    painter.drawText(QPoint(20, 20), str_Debug);

    str_Debug.clear();
    str_Debug.append("m_tilemapIndexInBuffer0_X&Y=");
    str_Debug.append(QString::number(m_tilemapIndexInBuffer0_X));
    str_Debug.append(",");
    str_Debug.append(QString::number(m_tilemapIndexInBuffer0_Y));
    painter.drawText(QPoint(20, 40), str_Debug);
}

void QTileMapWidget::UpdateDisplayMapBuffer_All() {
    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            int _mapZ, _mapX, _mapY;
            _mapZ = m_CurrentMapIndex_Z;
            _mapX = m_tilemapIndexInBuffer0_X + x;
            _mapY = m_tilemapIndexInBuffer0_Y + y;

            //不能只存放有效的图，即使没有图片，也要存进去，因为确实存在某一刻瓦片没有图片的情况
            m_DisplayMapBuffer[x][y].fill(m_backgroundColor);
            if (TileXYisValid(_mapZ, _mapX, _mapY)) {
                if (QTMW_AsyncLoadItemState_Loaded ==
                    m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].state) {
                    QByteArray tmpByteArray =
                        m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray;
                    QPixmap _tmpPixmap;
                    _tmpPixmap.loadFromData(tmpByteArray);
                    m_DisplayMapBuffer[x][y] = _tmpPixmap;
                } else {
                    AsynLoadTileMapImage(x, y, _mapZ, _mapX, _mapY);
                }
            }
        }
    }
}

void QTileMapWidget::UpdateDisplayMapBuffer_All_Gradual() {
    QVector<QVector<QPixmap>> _fatherCopy;
    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        QVector<QPixmap> _VecY;
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            QPixmap _pixmap;
            _VecY.push_back(_pixmap);
        }
        _fatherCopy.push_back(_VecY);
    }

    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            int _mapZ, _mapX, _mapY;
            _mapZ = m_CurrentMapIndex_Z;
            _mapX = m_tilemapIndexInBuffer0_X + x;
            _mapY = m_tilemapIndexInBuffer0_Y + y;

            if (TileXYisValid(_mapZ, _mapX, _mapY)) {
                if (QTMW_AsyncLoadItemState_Loaded ==
                    m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].state) {
                    QByteArray tmpByteArray =
                        m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray;
                    QPixmap _tmpPixmap;
                    _tmpPixmap.loadFromData(tmpByteArray);
                    m_DisplayMapBuffer[x][y] = _tmpPixmap;
                } else {
                    //上一级的瓦片坐标
                    int _fatherZ = _mapZ - 1;
                    int _fatherX = _mapX / 2;
                    int _fatherY = _mapY / 2;

                    if (m_AsyncLoadImageBuffer[_fatherZ][_fatherX][_fatherY].imageArray.length() >
                        0) {
                        if (m_AsyncLoadImageBuffer[_fatherZ][_fatherX][_fatherY]
                                .imageArray.length() > 0) {
                            QByteArray tmpByteArray =
                                m_AsyncLoadImageBuffer[_fatherZ][_fatherX][_fatherY].imageArray;
                            QPixmap _fatherPixmap;

                            _fatherPixmap.loadFromData(tmpByteArray);

                            QPixmap _partOfFather;
                            if (_mapX % 2 == 0 && _mapY % 2 == 0) {
                                _partOfFather = _fatherPixmap.copy(0, 0, m_SingleTilePixel / 2,
                                                                   m_SingleTilePixel / 2);
                            } else if (_mapX % 2 == 1 && _mapY % 2 == 0) {
                                _partOfFather = _fatherPixmap.copy(m_SingleTilePixel / 2, 0,
                                                                   m_SingleTilePixel / 2,
                                                                   m_SingleTilePixel / 2);
                            } else if (_mapX % 2 == 0 && _mapY % 2 == 1) {
                                _partOfFather = _fatherPixmap.copy(0, m_SingleTilePixel / 2,
                                                                   m_SingleTilePixel / 2,
                                                                   m_SingleTilePixel / 2);
                            } else if (_mapX % 2 == 1 && _mapY % 2 == 1) {
                                _partOfFather = _fatherPixmap.copy(
                                    m_SingleTilePixel / 2, m_SingleTilePixel / 2,
                                    m_SingleTilePixel / 2, m_SingleTilePixel / 2);
                            }
                            _partOfFather =
                                _partOfFather.scaled(m_SingleTilePixel, m_SingleTilePixel);
                            m_DisplayMapBuffer[x][y] = _partOfFather;

                            QByteArray _array;
                            QBuffer _buffer(&_array);
                            _buffer.open(QIODevice::ReadWrite);
                            _partOfFather.save(&_buffer, "jpg");
                            m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray = _array;
                        }
                    }
                }
            }
        }
    }
}

void QTileMapWidget::UpdateDisplayMapBuffer_AddX() {
    //先直接从内存中移动已经加载好的图片，在加载需要更新的新图片
    for (int x = 1; x < m_DisplayMapBufferSize_X; x++) {
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            m_DisplayMapBuffer[x - 1][y] = m_DisplayMapBuffer[x][y];
        }
    }

    for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
        int _mapZ, _mapX, _mapY;
        _mapZ = m_CurrentMapIndex_Z;
        _mapX = m_tilemapIndexInBuffer0_X + m_DisplayMapBufferSize_X - 1;
        _mapY = m_tilemapIndexInBuffer0_Y + y;

        if (TileXYisValid(_mapZ, _mapX, _mapY)) {
            if (QTMW_AsyncLoadItemState_Loaded ==
                m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].state) {
                QByteArray tmpByteArray = m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray;
                QPixmap _tmpPixmap;
                _tmpPixmap.loadFromData(tmpByteArray);
                m_DisplayMapBuffer[m_DisplayMapBufferSize_X - 1][y] = _tmpPixmap;
            } else {
                QPixmap _tmpPixmap = LoadTileMapImage(_mapZ, _mapX, _mapY);
                m_DisplayMapBuffer[m_DisplayMapBufferSize_X - 1][y] = _tmpPixmap;
            }
        }
    }
}

void QTileMapWidget::UpdateDisplayMapBuffer_SubX() {
    //先直接从内存中移动已经加载好的图片，在加载需要更新的新图片
    for (int x = m_DisplayMapBufferSize_X - 1; x > 0; x--) {
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            m_DisplayMapBuffer[x][y] = m_DisplayMapBuffer[x - 1][y];
        }
    }

    for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
        int _mapZ, _mapX, _mapY;
        _mapZ = m_CurrentMapIndex_Z;
        _mapX = m_tilemapIndexInBuffer0_X;
        _mapY = m_tilemapIndexInBuffer0_Y + y;

        if (TileXYisValid(_mapZ, _mapX, _mapY)) {
            if (QTMW_AsyncLoadItemState_Loaded ==
                m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].state) {
                QByteArray tmpByteArray = m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray;
                QPixmap _tmpPixmap;
                _tmpPixmap.loadFromData(tmpByteArray);
                m_DisplayMapBuffer[0][y] = _tmpPixmap;
            } else {
                QPixmap _tmpPixmap = LoadTileMapImage(_mapZ, _mapX, _mapY);
                m_DisplayMapBuffer[0][y] = _tmpPixmap;
            }
        }
    }
}

void QTileMapWidget::UpdateDisplayMapBuffer_AddY() {
    //先直接从内存中移动已经加载好的图片，在加载需要更新的新图片
    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        for (int y = 1; y < m_DisplayMapBufferSize_Y; y++) {
            m_DisplayMapBuffer[x][y - 1] = m_DisplayMapBuffer[x][y];
        }
    }

    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        int _mapZ, _mapX, _mapY;
        _mapZ = m_CurrentMapIndex_Z;
        _mapX = m_tilemapIndexInBuffer0_X + x;
        _mapY = m_tilemapIndexInBuffer0_Y + m_DisplayMapBufferSize_Y - 1;

        //不能只存放有效的图，即使没有图片，也要存进去，因为确实存在某一刻瓦片没有图片的情况
        if (TileXYisValid(_mapZ, _mapX, _mapY)) {
            if (QTMW_AsyncLoadItemState_Loaded ==
                m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].state) {
                QByteArray tmpByteArray = m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray;
                QPixmap _tmpPixmap;
                _tmpPixmap.loadFromData(tmpByteArray);
                m_DisplayMapBuffer[x][m_DisplayMapBufferSize_Y - 1] = _tmpPixmap;
            } else {
                QPixmap _tmpPixmap = LoadTileMapImage(_mapZ, _mapX, _mapY);
                m_DisplayMapBuffer[x][m_DisplayMapBufferSize_Y - 1] = _tmpPixmap;
            }
        }
    }
}

void QTileMapWidget::UpdateDisplayMapBuffer_SubY() {
    //先直接从内存中移动已经加载好的图片，在加载需要更新的新图片
    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        for (int y = m_DisplayMapBufferSize_Y - 1; y > 0; y--) {
            m_DisplayMapBuffer[x][y] = m_DisplayMapBuffer[x][y - 1];
        }
    }

    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        int _mapZ, _mapX, _mapY;
        _mapZ = m_CurrentMapIndex_Z;
        _mapX = m_tilemapIndexInBuffer0_X + x;
        _mapY = m_tilemapIndexInBuffer0_Y;

        //不能只存放有效的图，即使没有图片，也要存进去，因为确实存在某一刻瓦片没有图片的情况
        if (TileXYisValid(_mapZ, _mapX, _mapY)) {
            if (QTMW_AsyncLoadItemState_Loaded ==
                m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].state) {
                QByteArray tmpByteArray = m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray;
                QPixmap _tmpPixmap;
                _tmpPixmap.loadFromData(tmpByteArray);
                m_DisplayMapBuffer[x][0] = _tmpPixmap;
            } else {
                QPixmap _tmpPixmap = LoadTileMapImage(_mapZ, _mapX, _mapY);
                m_DisplayMapBuffer[x][0] = _tmpPixmap;
            }
        }
    }
}

int QTileMapWidget::FixX_LevelUp_BeforeFullAfterFull(Coord_Tile _pos) {
    int Level_After = _pos.z + 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);  //鼠标位置的经纬度
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(
            _coordLL, Level_After, &_ok);  //层级变化后，对应经纬度的瓦片坐标
        if (_ok) {
            int tileCountX = _pos.x - m_tilemapIndexInBuffer0_X;  //当前瓦片在缓存中的偏差量
            int LLinImageX_Now = (tileCountX * m_SingleTilePixel) +
                                 _pos.offsetLT_x;  //当前经纬度在当前层级的Image中的坐标
            int LLinImageX_New = (tileCountX * m_SingleTilePixel) +
                                 _coordTileNew.offsetLT_x;  //当前经纬度在新层级的Image中的坐标
            m_tilemapIndexInBuffer0_X =
                _coordTileNew.x - tileCountX;   //层级变化后，新瓦片的索引起点
            if (m_tilemapIndexInBuffer0_X < 0)  //如果层级低于一定的时候，考虑容错
            {
                m_tilemapIndexInBuffer0_X = 0;
                LLinImageX_New = (_coordTileNew.x * m_SingleTilePixel) + _coordTileNew.offsetLT_x;
            }

            int ret = m_WidgetOffsetImage.x() + LLinImageX_New -
                      LLinImageX_Now;  //通过偏移量进行二次修正,使得经纬度与鼠标位置对应
            while (ret + this->width() > (m_DisplayMapBufferSize_X - 1) * m_SingleTilePixel) {
                m_tilemapIndexInBuffer0_X++;
                ret = ret - m_SingleTilePixel;
            }

            return ret;
        }
    }
}

int QTileMapWidget::FixY_LevelUp_BeforeFullAfterFull(Coord_Tile _pos) {
    int Level_After = _pos.z + 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            int tileCountY = _pos.y - m_tilemapIndexInBuffer0_Y;
            int LLinImageY_Now = (tileCountY * m_SingleTilePixel) + _pos.offsetLT_y;
            int LLinImageY_New = (tileCountY * m_SingleTilePixel) + _coordTileNew.offsetLT_y;
            m_tilemapIndexInBuffer0_Y = _coordTileNew.y - tileCountY;
            if (m_tilemapIndexInBuffer0_Y < 0) {
                m_tilemapIndexInBuffer0_Y = 0;
                LLinImageY_New = (_coordTileNew.y * m_SingleTilePixel) + _coordTileNew.offsetLT_y;
            }

            int ret = m_WidgetOffsetImage.y() + LLinImageY_New - LLinImageY_Now;

            while (ret + this->height() > (m_DisplayMapBufferSize_Y - 1) * m_SingleTilePixel) {
                m_tilemapIndexInBuffer0_Y++;
                ret = ret - m_SingleTilePixel;
            }
            return ret;
        }
    }
}

int QTileMapWidget::FixX_LevelUp_BeforeEmptyAfterEmpty(Coord_Tile _pos) {
    int Level_After = _pos.z + 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            m_tilemapIndexInBuffer0_X = 0;
            int LLinImageX_New = (_coordTileNew.x * m_SingleTilePixel) + _coordTileNew.offsetLT_x;
            int ret = 0;
            if (qPow(2, Level_After) * m_SingleTilePixel > this->width()) {
                if (LLinImageX_New > this->width()) {
                    ret = LLinImageX_New - m_mousetCurrentPos.x();
                    while (ret + this->width() > m_DisplayMapBufferSize_X * m_SingleTilePixel) {
                        m_tilemapIndexInBuffer0_X++;
                        ret = ret - m_SingleTilePixel;
                    }
                    if (qPow(2, Level_After) * m_SingleTilePixel < ret + this->width()) {
                        ret = qPow(2, Level_After) * m_SingleTilePixel - this->width();
                    }
                } else {
                    ret = LLinImageX_New - m_mousetCurrentPos.x();
                    if (ret < 0) {
                        ret = 0;
                    }
                }
            } else {
            }  //如果该层级像素全部加起来都没有撑满可视区域，不作处理，自动居中显示
            return ret;
        }
    }
}

int QTileMapWidget::FixY_LevelUp_BeforeEmptyAfterEmpty(Coord_Tile _pos) {
    int Level_After = _pos.z + 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            m_tilemapIndexInBuffer0_Y = 0;
            int LLinImageY_New = (_coordTileNew.y * m_SingleTilePixel) + _coordTileNew.offsetLT_y;
            int ret = 0;
            if (qPow(2, Level_After) * m_SingleTilePixel > this->height()) {
                if (LLinImageY_New > this->height()) {
                    ret = LLinImageY_New - m_mousetCurrentPos.y();
                    while (ret + this->height() > m_DisplayMapBufferSize_Y * m_SingleTilePixel) {
                        m_tilemapIndexInBuffer0_Y++;
                        ret = ret - m_SingleTilePixel;
                    }
                    if (qPow(2, Level_After) * m_SingleTilePixel < ret + this->height()) {
                        ret = qPow(2, Level_After) * m_SingleTilePixel - this->height();
                    }
                } else {
                    ret = LLinImageY_New - m_mousetCurrentPos.y();
                    if (ret < 0) {
                        ret = 0;
                    }
                }
            } else {
            }  //如果该层级像素全部加起来都没有撑满可视区域，不作处理，自动居中显示
            return ret;
        }
    }
}

int QTileMapWidget::FixX_LevelUp_BeforeEmptyAfterFull(Coord_Tile _pos) {
    int Level_After = _pos.z + 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            int ret = 0;
            if (_coordTileNew.x < m_DisplayMapBufferSize_X - 1)  //_coordTile的坐标从0开始
            {
                ret = FixX_LevelUp_BeforeEmptyAfterEmpty(_pos);
            } else {
                int tileCountX = 0;
                if (_pos.x == qPow(2, _pos.z) - 1) {
                    tileCountX = m_DisplayMapBufferSize_X - 1;
                } else {
                    //与远界的判断
                    int offsetMax = qPow(2, _coordTileNew.z) - _coordTileNew.x - 1;
                    if (offsetMax < m_DisplayMapBufferSize_X) {
                        tileCountX = m_DisplayMapBufferSize_X - offsetMax;
                    }
                }
                m_tilemapIndexInBuffer0_X = _coordTileNew.x - tileCountX;  //算出起始瓦片

                if (m_tilemapIndexInBuffer0_X < 0) {
                    tileCountX = _coordTileNew.x;
                }

                int LLinImageX_New = (tileCountX * m_SingleTilePixel) +
                                     _coordTileNew.offsetLT_x;  //当前经纬度在新层级的Image中的坐标
                ret = LLinImageX_New - m_mousetCurrentPos.x();

                if (ret > m_DisplayMapBufferSize_X * m_SingleTilePixel - this->width()) {
                    ret = m_DisplayMapBufferSize_X * m_SingleTilePixel - this->width();
                }

                if (ret < 0) {
                    ret = 0;
                }
            }
            return ret;
        }
    }
}

int QTileMapWidget::FixY_LevelUp_BeforeEmptyAfterFull(Coord_Tile _pos) {
    int Level_After = _pos.z + 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            int ret = 0;
            if (_coordTileNew.y < m_DisplayMapBufferSize_Y - 1)  //_coordTile的坐标从0开始
            {
                ret = FixY_LevelUp_BeforeEmptyAfterEmpty(_pos);
            } else {
                int tileCountY = 0;
                if (_pos.y == qPow(2, _pos.z) - 1) {
                    tileCountY = m_DisplayMapBufferSize_Y - 1;
                } else {
                    //与远界的判断
                    int offsetMax = qPow(2, _coordTileNew.z) - _coordTileNew.y - 1;
                    if (offsetMax < m_DisplayMapBufferSize_Y) {
                        tileCountY = m_DisplayMapBufferSize_Y - offsetMax;
                    }
                }

                m_tilemapIndexInBuffer0_Y = _coordTileNew.y - tileCountY;  //算出起始瓦片
                if (m_tilemapIndexInBuffer0_Y < 0) {
                    tileCountY = _coordTileNew.y;
                }

                int LLinImageY_New = (tileCountY * m_SingleTilePixel) +
                                     _coordTileNew.offsetLT_y;  //当前经纬度在新层级的Image中的坐标
                ret = LLinImageY_New - m_mousetCurrentPos.y();

                if (ret > m_DisplayMapBufferSize_Y * m_SingleTilePixel - this->height()) {
                    ret = m_DisplayMapBufferSize_Y * m_SingleTilePixel - this->height();
                }

                if (ret < 0) {
                    ret = 0;
                }
            }
            return ret;
        }
    }
}

int QTileMapWidget::FixX_LevelDown_BeforeFullAfterFull(Coord_Tile _pos) {
    int Level_After = _pos.z - 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);  //鼠标位置的经纬度
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(
            _coordLL, Level_After, &_ok);  //层级变化后，对应经纬度的瓦片坐标
        if (_ok) {
            int tileCountX = _pos.x - m_tilemapIndexInBuffer0_X;  //当前瓦片在缓存中的偏差量
            int LLinImageX_Now = (tileCountX * m_SingleTilePixel) +
                                 _pos.offsetLT_x;  //当前经纬度在当前层级的Image中的坐标

            //边界判断，如果降级之后的远界小于缓存区
            int _tmp_Buffer0 = _coordTileNew.x - tileCountX;
            int _maxCheck = qPow(2, Level_After) - _tmp_Buffer0;
            if (_maxCheck < m_DisplayMapBufferSize_X) {
                tileCountX = tileCountX + (m_DisplayMapBufferSize_X - _maxCheck);
            }
            int LLinImageX_New = (tileCountX * m_SingleTilePixel) +
                                 _coordTileNew.offsetLT_x;  //当前经纬度在新层级的Image中的坐标
            m_tilemapIndexInBuffer0_X =
                _coordTileNew.x - tileCountX;  //层级变化后，新瓦片的索引起点

            if (m_tilemapIndexInBuffer0_X < 0)  //如果层级低于一定的时候，考虑容错
            {
                m_tilemapIndexInBuffer0_X = 0;
                LLinImageX_New = (_coordTileNew.x * m_SingleTilePixel) + _coordTileNew.offsetLT_x;
            }
            int ret = m_WidgetOffsetImage.x() + LLinImageX_New -
                      LLinImageX_Now;  //通过偏移量进行二次修正,使得经纬度与鼠标位置对应
            return ret;
        }
    }
}

int QTileMapWidget::FixY_LevelDown_BeforeFullAfterFull(Coord_Tile _pos) {
    int Level_After = _pos.z - 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            int tileCountY = _pos.y - m_tilemapIndexInBuffer0_Y;
            int LLinImageY_Now = (tileCountY * m_SingleTilePixel) + _pos.offsetLT_y;

            //边界判断，如果降级之后的远界小于缓存区
            int _tmp_Buffer0 = _coordTileNew.y - tileCountY;
            int _maxCheck = qPow(2, Level_After) - _tmp_Buffer0;
            if (_maxCheck < m_DisplayMapBufferSize_Y) {
                tileCountY = tileCountY + (m_DisplayMapBufferSize_Y - _maxCheck);
            }
            int LLinImageY_New = (tileCountY * m_SingleTilePixel) + _coordTileNew.offsetLT_y;
            m_tilemapIndexInBuffer0_Y = _coordTileNew.y - tileCountY;
            if (m_tilemapIndexInBuffer0_Y < 0) {
                m_tilemapIndexInBuffer0_Y = 0;
                LLinImageY_New = (_coordTileNew.y * m_SingleTilePixel) + _coordTileNew.offsetLT_y;
            }

            int _offetY = m_WidgetOffsetImage.y() + LLinImageY_New - LLinImageY_Now;
            return _offetY;
        }
    }
}

int QTileMapWidget::FixX_LevelDown_BeforeEmptyAfterEmpty(Coord_Tile _pos) {
    int Level_After = _pos.z - 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            m_tilemapIndexInBuffer0_X = 0;
            int LLinImageX_New = (_coordTileNew.x * m_SingleTilePixel) + _coordTileNew.offsetLT_x;
            int ret = 0;
            if (qPow(2, Level_After) * m_SingleTilePixel > this->width()) {
                if (LLinImageX_New > this->width()) {
                    ret = LLinImageX_New - m_mousetCurrentPos.x();
                    if (qPow(2, Level_After) * m_SingleTilePixel < ret + this->width()) {
                        ret = qPow(2, Level_After) * m_SingleTilePixel - this->width();
                    }
                } else {
                    ret = LLinImageX_New - m_mousetCurrentPos.x();
                    if (ret < 0) {
                        ret = 0;
                    }
                }
            } else {
            }  //如果该层级像素全部加起来都没有撑满可视区域，不作处理，自动居中显示
            return ret;
        }
    }
}

int QTileMapWidget::FixY_LevelDown_BeforeEmptyAfterEmpty(Coord_Tile _pos) {
    int Level_After = _pos.z - 1;
    if (Level_After >= 0) {
        bool _ok = false;
        Coord_LL _coordLL = ConvertCoord_Tile2LL_Mercator(_pos, &_ok);
        Coord_Tile _coordTileNew = ConvertCoord_LL2Tile_Mercator(_coordLL, Level_After, &_ok);
        if (_ok) {
            m_tilemapIndexInBuffer0_Y = 0;
            int LLinImageY_New = (_coordTileNew.y * m_SingleTilePixel) + _coordTileNew.offsetLT_y;
            int ret = 0;
            if (qPow(2, Level_After) * m_SingleTilePixel > this->height()) {
                if (LLinImageY_New > this->height()) {
                    ret = LLinImageY_New - m_mousetCurrentPos.y();
                    if (qPow(2, Level_After) * m_SingleTilePixel < ret + this->height()) {
                        ret = qPow(2, Level_After) * m_SingleTilePixel - this->height();
                    }
                } else {
                    ret = LLinImageY_New - m_mousetCurrentPos.y();
                    if (ret < 0) {
                        ret = 0;
                    }
                }
            } else {
            }  //如果该层级像素全部加起来都没有撑满可视区域，不作处理，自动居中显示
            return ret;
        }
    }
}

int QTileMapWidget::FixX_LevelDown_BeforeFullAfterEmpty(Coord_Tile _pos) {
    int ret = FixX_LevelDown_BeforeEmptyAfterEmpty(_pos);
    return ret;
}

int QTileMapWidget::FixY_LevelDown_BeforeFullAfterEmpty(Coord_Tile _pos) {
    int ret = FixY_LevelDown_BeforeEmptyAfterEmpty(_pos);
    return ret;
}

void QTileMapWidget::FixXY2WidgetSize() {
    int newOffsetX = m_WidgetOffsetImage.x();
    int newOffsetY = m_WidgetOffsetImage.y();

    int ImageLevelPixels = qPow(2, m_CurrentMapIndex_Z) * m_SingleTilePixel;
    int ImageBufferPixels_X = m_DisplayMapBufferSize_X * m_SingleTilePixel;
    int ImageBufferPixels_Y = m_DisplayMapBufferSize_Y * m_SingleTilePixel;
    if (ImageBufferPixels_X > ImageLevelPixels) {
        ImageBufferPixels_X = ImageLevelPixels;
    }
    if (ImageBufferPixels_Y > ImageLevelPixels) {
        ImageBufferPixels_Y = ImageLevelPixels;
    }

    if (newOffsetX > ImageBufferPixels_X - this->width()) {
        newOffsetX = ImageBufferPixels_X - this->width();
    }

    if (newOffsetY > ImageBufferPixels_Y - this->height()) {
        newOffsetY = ImageBufferPixels_Y - this->height();
    }

    if (newOffsetX < 0) {
        newOffsetX = 0;
    }
    if (newOffsetY < 0) {
        newOffsetY = 0;
    }

    m_WidgetOffsetImage.setX(newOffsetX);
    m_WidgetOffsetImage.setY(newOffsetY);
}

void QTileMapWidget::DragMoveProcess() {
    int _offsetInFrame_X = m_mousetCurrentPos.x() - m_mousetPreviousPos.x();
    int _offsetInFrame_Y = m_mousetCurrentPos.y() - m_mousetPreviousPos.y();

    int ImageAreaWidth = m_DisplayMapBufferSize_X * m_SingleTilePixel;
    int ImageAreaHeight = m_DisplayMapBufferSize_Y * m_SingleTilePixel;

    //如果层级比较低，填充不满缓冲区
    int VaildImageCount = qPow(2, m_CurrentMapIndex_Z);
    if (VaildImageCount < m_DisplayMapBufferSize_X) {
        ImageAreaWidth = VaildImageCount * m_SingleTilePixel;
    }
    if (VaildImageCount < m_DisplayMapBufferSize_Y) {
        ImageAreaHeight = VaildImageCount * m_SingleTilePixel;
    }

    //左边界
    if (_offsetInFrame_X > 0) {
        if (m_WidgetOffsetImage.x() - _offsetInFrame_X > 0) {
            int _newOffset = m_WidgetOffsetImage.x() - _offsetInFrame_X;
            m_WidgetOffsetImage.setX(_newOffset);
        } else {
            int _newOffset = 0;
            m_WidgetOffsetImage.setX(_newOffset);
        }
    }

    //上边界
    if (_offsetInFrame_Y > 0) {
        if (m_WidgetOffsetImage.y() - _offsetInFrame_Y > 0) {
            int _newOffset = m_WidgetOffsetImage.y() - _offsetInFrame_Y;
            m_WidgetOffsetImage.setY(_newOffset);
        } else {
            int _newOffset = 0;
            m_WidgetOffsetImage.setY(_newOffset);
        }
    }

    //右边界
    if (_offsetInFrame_X < 0) {
        if (m_WidgetOffsetImage.x() + this->width() - _offsetInFrame_X < ImageAreaWidth) {
            int _newOffset = m_WidgetOffsetImage.x() - _offsetInFrame_X;
            m_WidgetOffsetImage.setX(_newOffset);
        } else {
            int _newOffset = ImageAreaWidth - this->width();
            m_WidgetOffsetImage.setX(_newOffset);
        }
    }

    //下边界
    if (_offsetInFrame_Y < 0) {
        if (m_WidgetOffsetImage.y() + this->height() - _offsetInFrame_Y < ImageAreaHeight) {
            int _newOffset = m_WidgetOffsetImage.y() - _offsetInFrame_Y;
            m_WidgetOffsetImage.setY(_newOffset);
        } else {
            int _newOffset = ImageAreaHeight - this->height();
            m_WidgetOffsetImage.setY(_newOffset);
        }
    }

    //当Widget靠近Image边界时，判断是否可以扩展更新
    //右边界更新
    if (m_WidgetOffsetImage.x() + this->width() >= ImageAreaWidth - m_SingleTilePixel) {
        if (m_tilemapIndexInBuffer0_X + m_DisplayMapBufferSize_X < VaildImageCount) {
            m_tilemapIndexInBuffer0_X++;
            UpdateDisplayMapBuffer_AddX();
            m_WidgetOffsetImage.setX(m_WidgetOffsetImage.x() - m_SingleTilePixel);
        }
    }

    //左边界更新
    if (m_WidgetOffsetImage.x() <= m_SingleTilePixel) {
        if (m_tilemapIndexInBuffer0_X > 0) {
            m_tilemapIndexInBuffer0_X--;
            UpdateDisplayMapBuffer_SubX();
            m_WidgetOffsetImage.setX(m_WidgetOffsetImage.x() + m_SingleTilePixel);
        }
    }

    //上边界更新
    if (m_WidgetOffsetImage.y() <= m_SingleTilePixel) {
        if (m_tilemapIndexInBuffer0_Y > 0) {
            m_tilemapIndexInBuffer0_Y--;
            UpdateDisplayMapBuffer_SubY();
            m_WidgetOffsetImage.setY(m_WidgetOffsetImage.y() + m_SingleTilePixel);
        }
    }

    //下边界更新
    if (m_WidgetOffsetImage.y() + this->height() >= ImageAreaHeight - m_SingleTilePixel) {
        if (m_tilemapIndexInBuffer0_Y + m_DisplayMapBufferSize_Y < VaildImageCount) {
            m_tilemapIndexInBuffer0_Y++;
            UpdateDisplayMapBuffer_AddY();
            m_WidgetOffsetImage.setY(m_WidgetOffsetImage.y() - m_SingleTilePixel);
        }
    }
}

Coord_Tile QTileMapWidget::ConvertCoord_Widget2Tile(QPoint _pos, bool *_ok) {
    Coord_Tile ret;
    bool _retIsVaild = false;

    if (m_isSQLiteDBValid) {
        QPoint _p_Image = ConvertCoord_Widget2Image(_pos);
        ret.z = m_CurrentMapIndex_Z;
        ret.x = m_tilemapIndexInBuffer0_X + (_p_Image.x() / m_SingleTilePixel);
        ret.y = m_tilemapIndexInBuffer0_Y + (_p_Image.y() / m_SingleTilePixel);
        ret.offsetLT_x = _p_Image.x() % m_SingleTilePixel;
        ret.offsetLT_y = _p_Image.y() % m_SingleTilePixel;

        //边界修正
        if (ret.x == qPow(2, ret.z) && ret.offsetLT_x == 0) {
            ret.x = qPow(2, ret.z) - 1;
            ret.offsetLT_x = m_SingleTilePixel;
        }

        //边界修正
        if (ret.y == qPow(2, ret.z) && ret.offsetLT_y == 0) {
            ret.y = qPow(2, ret.z) - 1;
            ret.offsetLT_y = m_SingleTilePixel;
        }

        _retIsVaild = true;

        if (ret.x < 0 || ret.y < 0) {
            _retIsVaild = false;
        }

        if (_p_Image.x() < 0 || _p_Image.y() < 0) {
            _retIsVaild = false;
        }

        if (ret.x > qPow(2, m_CurrentMapIndex_Z) - 1 || ret.y > qPow(2, m_CurrentMapIndex_Z) - 1) {
            _retIsVaild = false;
        }
    }

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }

    if (!_retIsVaild) {
        ret.z = -1;
        ret.x = -1;
        ret.y = -1;
        ret.offsetLT_x = 0;
        ret.offsetLT_y = 0;
    }
    return ret;
}

Coord_LL QTileMapWidget::ConvertCoord_Widget2LL(QPoint _pos, bool *_ok) {
    Coord_LL ret;
    bool _retIsVaild = false;
    Coord_Tile _coordTile = ConvertCoord_Widget2Tile(_pos, &_retIsVaild);
    if (_retIsVaild) {
        ret = ConvertCoord_Tile2LL_Mercator(_coordTile, &_retIsVaild);
    }

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }
    if (!_retIsVaild) {
        ret.lon = 0;
        ret.lat = 0;
    }
    return ret;
}

Coord_LLMS QTileMapWidget::ConvertCoord_Widget2LLMS(QPoint _pos, bool *_ok) {
    Coord_LLMS ret;
    bool _retIsVaild = false;

    Coord_LL _coordLL = ConvertCoord_Widget2LL(_pos, &_retIsVaild);
    if (_retIsVaild) {
        ret = ConvertCoord_LL2LLMS(_coordLL);
    }

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }

    if (!_retIsVaild) {
        ret.lonD = 0;
        ret.lonM = 0;
        ret.lonS = 0;
        ret.latD = 0;
        ret.latM = 0;
        ret.latS = 0;
    }
    return ret;
}

Coord_LLMS QTileMapWidget::ConvertCoord_LL2LLMS(Coord_LL _pos) {
    Coord_LLMS ret;
    memset(&ret, 0, sizeof(ret));

    double _positive = 0;
    int _degree, _minute;
    double _second;

    // Lon
    if (_pos.lon >= 0) {
        _positive = _pos.lon;
    } else {
        _positive = -_pos.lon;
    }
    _degree = floor(_positive);
    _minute = floor((_positive - _degree) * 60);
    _second = (_positive * 3600) - (_degree * 3600) - (_minute * 60);

    // double类型会出现 107.8° = 107°47′59.999999999989768″，所以这里需要进行一下容错处理
    if (abs(60 - _second) < 0.00001) {
        _minute = _minute + 1;
        _second = abs((_positive * 3600) - (_degree * 3600) - (_minute * 60));
    }
    ret.lonD = _degree;
    ret.lonM = _minute;
    ret.lonS = _second;
    if (_pos.lon < 0) {
        ret.lonD = -ret.lonD;
    }

    // Lat
    if (_pos.lat >= 0) {
        _positive = _pos.lat;
    } else {
        _positive = -_pos.lat;
    }
    _degree = floor(_positive);
    _minute = floor((_positive - _degree) * 60);
    _second = (_positive * 3600) - (_degree * 3600) - (_minute * 60);

    // double类型会出现 107.8° = 107°47′59.999999999989768″，所以这里需要进行一下容错处理
    if (abs(60 - _second) < 0.00001) {
        _minute = _minute + 1;
        _second = abs((_positive * 3600) - (_degree * 3600) - (_minute * 60));
    }
    ret.latD = _degree;
    ret.latM = _minute;
    ret.latS = _second;
    if (_pos.lat < 0) {
        ret.latD = -ret.latD;
    }
    return ret;
}

Coord_LL QTileMapWidget::ConvertCoord_LLMS2LL(Coord_LLMS _pos) {
    Coord_LL ret;
    double _D_min, _D_sec;

    // Lon
    _D_min = (double) _pos.lonM / 60;
    _D_sec = (double) _pos.lonS / 3600;
    ret.lon = _pos.lonD + _D_min + _D_sec;

    // Lat
    _D_min = (double) _pos.latM / 60;
    _D_sec = (double) _pos.latS / 3600;
    ret.lat = _pos.latD + _D_min + _D_sec;

    return ret;
}

QPoint QTileMapWidget::ConvertCoord_Widget2Image(QPoint _pos) {
    QPoint ret;
    ret.setX(_pos.x() + m_WidgetOffsetImage.x());
    ret.setY(_pos.y() + m_WidgetOffsetImage.y());
    return ret;
}

QPoint QTileMapWidget::ConvertCoord_Image2Widget(QPoint _pos) {
    QPoint ret;
    ret.setX(_pos.x() - m_WidgetOffsetImage.x());
    ret.setY(_pos.y() - m_WidgetOffsetImage.y());
    return ret;
}

void QTileMapWidget::slot_AsyncLoadImageLoaded(int _buffer_x, int _buffer_y, int _tile_z,
                                               int _tile_x, int _tile_y, QByteArray _imageArray) {
    if (_tile_z == m_CurrentMapIndex_Z) {
        if (_buffer_x >= 0 && _buffer_y >= 0 && _buffer_x < m_DisplayMapBufferSize_X &&
            _buffer_y < m_DisplayMapBufferSize_Y) {
            m_DisplayMapBuffer[_buffer_x][_buffer_y].loadFromData(_imageArray);
        }
    }

    m_AsyncLoadImageBuffer[_tile_z][_tile_x][_tile_y].state = QTMW_AsyncLoadItemState_Loaded;
    m_AsyncLoadImageBuffer[_tile_z][_tile_x][_tile_y].imageArray = _imageArray;
}

void QTileMapWidget::slot_SingleShotInWheelEvent() {
    bool imageNotFound = false;
    for (int x = 0; x < m_DisplayMapBufferSize_X; x++) {
        for (int y = 0; y < m_DisplayMapBufferSize_Y; y++) {
            int _mapZ, _mapX, _mapY;
            _mapZ = m_CurrentMapIndex_Z;
            _mapX = m_tilemapIndexInBuffer0_X + x;
            _mapY = m_tilemapIndexInBuffer0_Y + y;

            if (TileXYisValid(_mapZ, _mapX, _mapY)) {
                if (QTMW_AsyncLoadItemState_Loaded ==
                    m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].state) {
                    QByteArray tmpByteArray =
                        m_AsyncLoadImageBuffer[_mapZ][_mapX][_mapY].imageArray;
                    QPixmap _tmpPixmap;
                    _tmpPixmap.loadFromData(tmpByteArray);
                    m_DisplayMapBuffer[x][y] = _tmpPixmap;
                } else {
                    imageNotFound = true;
                }
            }
        }
    }

    if (imageNotFound) {
        QTimer::singleShot(QTMW_TIMER_TICK, this, SLOT(slot_SingleShotInWheelEvent()));
    }
}

void QTileMapWidget::slot_timeout_AutoLoadDataFromDB() {
    bool _ok = false;
    Coord_Tile widgetCenter =
        ConvertCoord_Widget2Tile(QPoint(this->width() / 2, this->height() / 2), &_ok);
    if (_ok) {
        if (widgetCenter.z != m_Flag_AutoLoadDataFromDB.z ||
            widgetCenter.x != m_Flag_AutoLoadDataFromDB.x ||
            widgetCenter.y != m_Flag_AutoLoadDataFromDB.y) {
            m_Flag_AutoLoadDataFromDB.z = widgetCenter.z;
            m_Flag_AutoLoadDataFromDB.x = widgetCenter.x;
            m_Flag_AutoLoadDataFromDB.y = widgetCenter.y;

            //从中心向外围更新
            int range = m_DisplayMapBufferSize_X;  //外扩范围
            for (int r = 0; r <= range; r++) {
                int z = m_Flag_AutoLoadDataFromDB.z;
                int x = m_Flag_AutoLoadDataFromDB.x - r;
                int y = m_Flag_AutoLoadDataFromDB.y - r;

                for (int s = 0; s <= 8 * r; s++) {
                    if (TileXYisValid(z, x, y)) {
                        AsynLoadTileMapImage(-1, -1, z, x, y);
                    }

                    if (s < 2 * r) {
                        x++;
                    } else if (s < 4 * r) {
                        y++;
                    } else if (s < 6 * r) {
                        x--;
                    } else if (s < 8 * r) {
                        y--;
                    }
                }
            }
        }
    }
}

void QTileMapWidget::CleanAsyncLoadImageBuffer() {
    m_AsyncLoadImageBuffer.clear();
}

bool QTileMapWidget::TileXYisValid(int z, int x, int y) {
    if (z >= 0 && x >= 0 && y >= 0) {
        if (x < qPow(2, z) && y < qPow(2, z)) {
            return true;
        }
    }
    return false;
}

void QTileMapWidget::async_AddThread(QTMW_AsyncLoadImage *_ptr) {
    m_mutex_AsyncLoadImage.lock();
    m_AsyncLoadImageThreadQueue.push_back(_ptr);
    m_mutex_AsyncLoadImage.unlock();
}

void QTileMapWidget::async_CleanCurrentThread() {
    m_mutex_AsyncLoadImage.lock();
    while (m_AsyncLoadImageThreadQueue.count() > 1) {
        QTMW_AsyncLoadImage *_ptr = m_AsyncLoadImageThreadQueue.last();

        int buffer_x, buffer_y, tile_z, tile_x, tile_y;

        _ptr->getParm(&buffer_x, &buffer_y, &tile_z, &tile_x, &tile_y);

        m_AsyncLoadImageBuffer[tile_z][tile_x][tile_y].state = QTMW_AsyncLoadItemState_None;
        m_AsyncLoadImageThreadQueue.pop_back();
        _ptr->deleteLater();
    }
    m_mutex_AsyncLoadImage.unlock();
}

QPoint QTileMapWidget::ConvertCoord_Tile2Widget(Coord_Tile _pos, bool *_ok) {
    QPoint ret(-1, -1);
    bool _canConvert = true;
    bool _retIsVaild = false;

    if (!m_isSQLiteDBValid)  //数据库无效不同
    {
        _canConvert = false;
    }

    if (_pos.z != m_CurrentMapIndex_Z)  //层级不同
    {
        _canConvert = false;
    }

    if (_pos.x >= qPow(2, m_CurrentMapIndex_Z)) {
        _canConvert = false;
    }

    if (_pos.y >= qPow(2, m_CurrentMapIndex_Z)) {
        _canConvert = false;
    }

    if (_canConvert) {
        int _x = -1;
        int _y = -1;
        _x = ((_pos.x - m_tilemapIndexInBuffer0_X) * m_SingleTilePixel) + _pos.offsetLT_x;
        _y = ((_pos.y - m_tilemapIndexInBuffer0_Y) * m_SingleTilePixel) + _pos.offsetLT_y;

        QPoint _p_Image;
        _p_Image.setX(_x);
        _p_Image.setY(_y);

        ret = ConvertCoord_Image2Widget(_p_Image);
        _retIsVaild = true;
    }

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }
    if (!_retIsVaild) {
        ret.setX(-1);
        ret.setY(-1);
    }
    return ret;
}

QPoint QTileMapWidget::ConvertCoord_LL2Widget(Coord_LL _pos, bool *_ok) {
    QPoint ret;
    bool _retIsVaild = false;
    if (m_isSQLiteDBValid) {
        // LL -> Tile
        Coord_Tile _coordTile =
            ConvertCoord_LL2Tile_Mercator(_pos, m_CurrentMapIndex_Z, &_retIsVaild);

        // Tile -> Widget
        if (_retIsVaild) {
            ret = ConvertCoord_Tile2Widget(_coordTile, &_retIsVaild);
        }
    }

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }

    if (!_retIsVaild) {
        ret.setX(-1);
        ret.setY(-1);
    }
    return ret;
}

Coord_LL QTileMapWidget::ConvertCoord_Tile2LL_Mercator(Coord_Tile _pos, bool *_ok) {
    Coord_LL ret;
    bool _retIsVaild = false;

    if (m_mode_Reversal) {
        _pos.x = ConvertTileX_Normal2Reversal(_pos.x, _pos.z);
    }

    if (_pos.x >= 0 && _pos.y >= 0) {
        // picel X -> Lon
        double PixelX = _pos.x * m_SingleTilePixel + _pos.offsetLT_x;
        double Lon = PixelX * 360 / (256 << _pos.z) - 180;

        // picel Y -> Lat
        double PixelY = _pos.y * m_SingleTilePixel + _pos.offsetLT_y;
        double y = 2 * M_PI * (1 - PixelY / (128 << _pos.z));
        double z = qPow(M_E, y);
        double siny = (z - 1) / (z + 1);
        double Lat = qAsin(siny) * 180 / M_PI;

        ret.lon = Lon;
        ret.lat = Lat;
        _retIsVaild = true;
    }

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }
    if (!_retIsVaild) {
        ret.lon = 0;
        ret.lat = 0;
    }
    return ret;
}

Coord_Tile QTileMapWidget::ConvertCoord_LL2Tile_Mercator(Coord_LL _pos, int _level, bool *_ok) {
    Coord_Tile ret;
    bool _retIsVaild = false;

    if (_level >= 0) {
        ret.z = _level;

        double PixelX = (_pos.lon + 180) * (256 << _level) / 360;

        double siny = qSin(_pos.lat * M_PI / 180);
        double y = log((1 + siny) / (1 - siny));
        double PixelY = (128 << _level) * (1 - y / (2 * M_PI));

        int tileX = PixelX / m_SingleTilePixel;
        int tileY = PixelY / m_SingleTilePixel;
        ret.x = (int) floor(tileX);
        ret.y = (int) floor(tileY);
        ret.offsetLT_x = PixelX - tileX * m_SingleTilePixel;
        ret.offsetLT_y = PixelY - tileY * m_SingleTilePixel;
        //边界修正
        if (ret.x == qPow(2, ret.z) && ret.offsetLT_x == 0) {
            ret.x = qPow(2, ret.z) - 1;
            ret.offsetLT_x = m_SingleTilePixel;
        }

        //边界修正
        if (ret.y == qPow(2, ret.z) && ret.offsetLT_y == 0) {
            ret.y = qPow(2, ret.z) - 1;
            ret.offsetLT_y = m_SingleTilePixel;
        }
        _retIsVaild = true;
    }

    if (_ok != nullptr) {
        *(bool *) _ok = _retIsVaild;
    }
    if (!_retIsVaild) {
        ret.z = -1;
        ret.x = -1;
        ret.y = -1;
        ret.offsetLT_x = 0;
        ret.offsetLT_y = 0;
    }

    if (m_mode_Reversal) {
        ret.x = ConvertTileX_Normal2Reversal(ret.x, ret.z);
    }
    return ret;
}

void QTMW_DrawGraph::setParent(QTileMapWidget *parent) {
    m_ptr_QTileMapWidget = parent;
}

void QTMW_DrawGraph::clear() {
    m_mutex.lock();
    m_DrawItem_Points.clear();
    m_DrawItem_Lines.clear();
    for (int i = 0; i < m_DrawItem_Paths.size(); i++) {
        m_DrawItem_Paths[i].data.clear();
    }
    m_DrawItem_Paths.clear();
    m_mutex.unlock();
}

int QTMW_DrawGraph::addPoint(QTMW_DrawItem_Point point) {
    int index = -1;
    m_mutex.lock();
    index = m_DrawItem_Points.count();
    m_DrawItem_Points.push_back(point);
    m_mutex.unlock();
    return index;
}

int QTMW_DrawGraph::addLine(QTMW_DrawItem_Line line) {
    int index = -1;
    m_mutex.lock();
    index = m_DrawItem_Lines.count();
    m_DrawItem_Lines.push_back(line);
    m_mutex.unlock();
    return index;
}

int QTMW_DrawGraph::addPath(QTMW_DrawItem_Path path) {
    int index = -1;
    m_mutex.lock();
    index = m_DrawItem_Paths.count();
    m_DrawItem_Paths.push_back(path);
    m_mutex.unlock();
    return index;
}

bool QTMW_DrawGraph::updatePoint(int pointIndex, QTMW_DrawItem_Point point) {
    bool ret = false;
    if (pointIndex < m_DrawItem_Points.size()) {
        m_DrawItem_Points[pointIndex] = point;
        ret = true;
    }
    return ret;
}

bool QTMW_DrawGraph::updateLine(int lineIndex, QTMW_DrawItem_Line line) {
    bool ret = false;
    if (lineIndex < m_DrawItem_Lines.size()) {
        m_DrawItem_Lines[lineIndex] = line;
        ret = true;
    }
    return ret;
}

bool QTMW_DrawGraph::updatePath(int pathInedx, Coord_LL pathData) {
    bool ret = false;
    if (pathInedx < m_DrawItem_Paths.size()) {
        m_DrawItem_Paths[pathInedx].data.push_back(pathData);
        ret = true;
    }
    return ret;
}

int QTMW_DrawGraph::PointCount() {
    return m_DrawItem_Points.count();
}

int QTMW_DrawGraph::LineCount() {
    return m_DrawItem_Lines.count();
}

int QTMW_DrawGraph::PathCount() {
    return m_DrawItem_Paths.count();
}

void QTMW_DrawGraph::draw() {
    QPainter painter(m_ptr_QTileMapWidget);
    painter.setRenderHint(QPainter::Antialiasing, true);  //反锯齿

    QPen pen;
    QBrush brush(Qt::SolidPattern);
    QFont font;

    m_mutex.lock();
    for (QVector<QTMW_DrawItem_Point>::iterator _iterLoopPoint = m_DrawItem_Points.begin();
         _iterLoopPoint != m_DrawItem_Points.end(); _iterLoopPoint++) {
        Coord_LL _posLL = _iterLoopPoint->pos;
        bool _ok = false;
        QPoint _posWidget = m_ptr_QTileMapWidget->ConvertCoord_LL2Widget(_posLL, &_ok);
        if (_ok) {
            // Ellipse or Image
            if (_iterLoopPoint->Image.image.isNull()) {
                pen.setColor(_iterLoopPoint->color);
                brush.setColor(_iterLoopPoint->color);
                painter.setPen(pen);
                painter.setBrush(brush);

                int _size = _iterLoopPoint->size / 2;
                painter.drawEllipse(_posWidget, _size, _size);
            } else {
                int _size = _iterLoopPoint->Image.size;
                QRect _imageRect;
                _imageRect.setRect(_posWidget.x() - _size / 2, _posWidget.y() - _size / 2, _size,
                                   _size);
                painter.drawImage(_imageRect, _iterLoopPoint->Image.image);
            }

            QPoint _textPos = _posWidget + _iterLoopPoint->Text.offset;
            pen.setColor(_iterLoopPoint->Text.color);
            painter.setPen(pen);
            font.setPixelSize(_iterLoopPoint->Text.size);
            painter.setFont(font);
            // painter.drawText(_textPos,_iterLoopPoint->Text.text); //单行显示

            //多行显示
            QTextOption opt;
            opt.setWrapMode(QTextOption::WordWrap);
            QRect _textRect = m_ptr_QTileMapWidget->geometry();
            _textRect.setTopLeft(_textPos);
            painter.drawText(_textRect, _iterLoopPoint->Text.text, opt);
        }
    }

    for (QVector<QTMW_DrawItem_Line>::iterator _iterLoopLine = m_DrawItem_Lines.begin();
         _iterLoopLine != m_DrawItem_Lines.end(); _iterLoopLine++) {
        Coord_LL _posLL_From = _iterLoopLine->from;
        Coord_LL _posLL_To = _iterLoopLine->to;
        bool _ok = false;
        QPoint _posWidget_From = m_ptr_QTileMapWidget->ConvertCoord_LL2Widget(_posLL_From, &_ok);
        if (_ok) {
            QPoint _posWidget_To = m_ptr_QTileMapWidget->ConvertCoord_LL2Widget(_posLL_To, &_ok);
            if (_ok) {
                pen.setWidth(_iterLoopLine->width);
                pen.setColor(_iterLoopLine->color);
                brush.setColor(_iterLoopLine->color);
                painter.setPen(pen);
                painter.setBrush(brush);

                painter.drawLine(_posWidget_From, _posWidget_To);

                QPoint _textPos =
                    (_posWidget_From + _posWidget_To) / 2 + _iterLoopLine->Text.offset;
                pen.setColor(_iterLoopLine->Text.color);
                painter.setPen(pen);
                font.setPixelSize(_iterLoopLine->Text.size);
                painter.setFont(font);
                // painter.drawText(_textPos,_iterLoopLine->Text.text); //单行显示

                //多行显示
                QTextOption opt;
                opt.setWrapMode(QTextOption::WordWrap);
                QRect _textRect = m_ptr_QTileMapWidget->geometry();
                _textRect.setTopLeft(_textPos);
                painter.drawText(_textRect, _iterLoopLine->Text.text, opt);
            }
        }
    }

    for (QVector<QTMW_DrawItem_Path>::iterator _iterLoopPath = m_DrawItem_Paths.begin();
         _iterLoopPath != m_DrawItem_Paths.end(); _iterLoopPath++) {
        pen.setWidth(_iterLoopPath->width);
        pen.setColor(_iterLoopPath->color);
        brush.setColor(_iterLoopPath->color);
        painter.setPen(pen);
        painter.setBrush(brush);

        QVector<QPoint> pointVec;
        for (int i = 0; i < _iterLoopPath->data.size(); i++) {
            Coord_LL pos = _iterLoopPath->data.at(i);
            bool _ok = false;
            QPoint point = m_ptr_QTileMapWidget->ConvertCoord_LL2Widget(pos, &_ok);
            if (_ok) {
                pointVec.push_back(point);
            }
        }
        for (int i = 1; i < pointVec.size(); i++) {
            painter.drawLine(pointVec.at(i), pointVec.at(i - 1));
        }
    }
    m_mutex.unlock();
}

void QTMW_AsyncLoadImage::setParm(QString _dbPath, int _buffer_x, int _buffer_y, int _tile_z,
                                  int _tile_x, int _tile_y) {
    db_path = _dbPath;
    buffer_x = _buffer_x;
    buffer_y = _buffer_y;
    tile_z = _tile_z;
    tile_x = _tile_x;
    tile_y = _tile_y;
}

void QTMW_AsyncLoadImage::getParm(int *_buffer_x, int *_buffer_y, int *_tile_z, int *_tile_x,
                                  int *_tile_y) {
    *_buffer_x = buffer_x;
    *_buffer_y = buffer_y;
    *_tile_z = tile_z;
    *_tile_x = tile_x;
    *_tile_y = tile_y;
}

void QTMW_AsyncLoadImage::run() {
    QString str_index = QString("%1_%2_%3").arg(tile_z).arg(tile_x).arg(tile_y);
    QSqlDatabase DB = QSqlDatabase::addDatabase("QSQLITE", str_index);
    DB.setDatabaseName(db_path);
    QSqlQuery que(DB);
    if (DB.open()) {
        //坐标修正
        int z = tile_z;
        int x = tile_x;
        int y = qPow(2, z) - 1 - tile_y;
        if (QTileMapWidget::m_mode_Reversal) {
            x = QTileMapWidget::ConvertTileX_Normal2Reversal(x, z);
        }
        QString selectStr =
            QString(
                "select tile_data from images where tile_id=(select tile_id from map where "
                "tile_column=%1 and tile_row=%2 and zoom_level=%3)")
                .arg(x)
                .arg(y)
                .arg(z);
        que.prepare(selectStr);
        que.exec();
        QByteArray imageArray;
        if (que.next()) {
            imageArray = que.value(0).toByteArray();

        } else {
        }

        emit loaded(buffer_x, buffer_y, tile_z, tile_x, tile_y, imageArray);
    }
    DB.close();
}

void QTMW_AsyncLoadMgr::run() {
    msleep(100);
    if (m_ptr_QTileMapWidget != nullptr) {
        while (true) {
            msleep(1);

            m_ptr_QTileMapWidget->m_mutex_AsyncLoadImage.lock();
            if (!m_ptr_QTileMapWidget->m_AsyncLoadImageThreadQueue.isEmpty()) {
                if (m_ptr_QTileMapWidget->m_AsyncLoadImageThreadQueue.first()->isFinished()) {
                    QTMW_AsyncLoadImage *_ptr =
                        m_ptr_QTileMapWidget->m_AsyncLoadImageThreadQueue.first();
                    _ptr->deleteLater();
                    m_ptr_QTileMapWidget->m_AsyncLoadImageThreadQueue.pop_front();
                } else if (!m_ptr_QTileMapWidget->m_AsyncLoadImageThreadQueue.first()
                                ->isRunning()) {
                    m_ptr_QTileMapWidget->m_AsyncLoadImageThreadQueue.first()->start();
                }
            }
            m_ptr_QTileMapWidget->m_mutex_AsyncLoadImage.unlock();
        }
    }
}

void QTMW_AsyncLoadMgr::setParent(QTileMapWidget *parent) {
    m_ptr_QTileMapWidget = parent;
}

void QTMW_AsyncPreload::setParm(QString _dbPath, QVector<Coord_Tile> _vecData) {
    m_dbPath = _dbPath;
    m_data = _vecData;
}

void QTMW_AsyncPreload::getPreloadProgress(int *current, int *max) {
    *current = m_PreloadProgressCurrent;
    *max = m_PreloadProgressMax;
}

void QTMW_AsyncPreload::run() {
    static_PreloadLoadImageBuffer.clear();
    msleep(20);

    m_PreloadProgressCurrent = 0;
    m_PreloadProgressMax = 0;

    m_PreloadProgressMax = m_data.count();
    //异步预加载
    QString str_Name = QString("QTMW_AsyncPreload");
    QSqlDatabase DB = QSqlDatabase::addDatabase("QSQLITE", str_Name);
    DB.setDatabaseName(m_dbPath);
    QSqlQuery que(DB);
    if (DB.open()) {
        for (int i = 0; i < m_data.count(); i++) {
            m_PreloadProgressCurrent = i;
            Coord_Tile _preloadCoord = m_data.at(i);

            int z = _preloadCoord.z;
            int x = _preloadCoord.x;
            int y = qPow(2, z) - 1 - _preloadCoord.y;

            if (QTileMapWidget::m_mode_Reversal) {
                x = QTileMapWidget::ConvertTileX_Normal2Reversal(x, z);
            }
            QString selectStr =
                QString(
                    "select tile_data from images where tile_id=(select tile_id from map where "
                    "tile_column=%1 and tile_row=%2 and zoom_level=%3)")
                    .arg(x)
                    .arg(y)
                    .arg(z);
            que.prepare(selectStr);
            que.exec();
            QByteArray imageArray;
            if (que.next()) {
                imageArray = que.value(0).toByteArray();
                QString _key = QString("%1_%2_%3")
                                   .arg(_preloadCoord.z)
                                   .arg(_preloadCoord.x)
                                   .arg(_preloadCoord.y);
                static_PreloadLoadImageBuffer.insert(_key, imageArray);
            }
        }
    }
}
