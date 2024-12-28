#ifndef QTILEMAPWIDGET_H
#define QTILEMAPWIDGET_H

// include <QSqlXXX> need to add QT += sql in .pro
// 引用 <QSqlXXX> 需要在.pro中增加 QT += sql
#include <QApplication>
#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QMouseEvent>
#include <QMutex>
#include <QPainter>
#include <QQueue>
#include <QRunnable>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QString>
#include <QStyleOption>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QtMath>

typedef void (*CallBackFunc)(QWidget*);

// Coord
struct Coord_LL {
    double lon;
    double lat;

    Coord_LL() {
        lon = 180;
        lat = 180;
    }
    Coord_LL(double _lon, double _lat) {
        lon = _lon;
        lat = _lat;
    }
};
struct Coord_LLMS {
    int lonD;
    int lonM;
    double lonS;

    int latD;
    int latM;
    double latS;

    Coord_LLMS() {
        lonD = 0;
        lonM = 0;
        lonS = 0;
        latD = 0;
        latM = 0;
        latS = 0;
    }
};
struct Coord_Tile {
    int z;  //瓦片索引Z
    int x;  //瓦片索引X
    int y;  //瓦片索引Y

    int offsetLT_x;  //在瓦片内的横向偏移量,距离瓦片左上角的像素
    int offsetLT_y;  //在瓦片内的纵向偏移量,距离瓦片左上角的像素

    Coord_Tile() {
        z = 0;
        x = 0;
        y = 0;
        offsetLT_x = 0;
        offsetLT_y = 0;
    }
};

// DB
struct DB_Info {
    int minZoom;
    int maxZoom;
    int tile_width;
    int tile_height;
    QString format;

    DB_Info() {
        minZoom = -1;
        maxZoom = -1;
        tile_width = 0;
        tile_height = 0;
    }
};

// Class Define
class QTileMapWidget;
class QTMW_DrawGraph;
class QTMW_DrawItem_Point;
class QTMW_DrawItem_Text;
class QTMW_DrawItem_Image;
class QTMW_DrawItem_Line;
class QTMW_DrawItem_Path;
class QTMW_DrawItem_Circle;
class QTMW_AsyncLoadImage;
class QTMW_AsyncLoadMgr;

// Async LoadImage
class QTMW_AsyncLoadImage : public QThread {
    Q_OBJECT
public:
    void setParm(QString _dbPath, int _buffer_x, int _buffer_y, int _tile_z, int _tile_x,
                 int _tile_y);
    void getParm(int* _buffer_x, int* _buffer_y, int* _tile_z, int* _tile_x, int* _tile_y);

protected:
    void run();
signals:
    void loaded(int _buffer_x, int _buffer_y, int _tile_z, int _tile_x, int _tile_y,
                QByteArray _imageArray);

private:
    int buffer_x = 0;
    int buffer_y = 0;
    int tile_z = -1;
    int tile_x = -1;
    int tile_y = -1;
    QString db_path;
    QByteArray tile_imageAray;
};
class QTMW_AsyncLoadMgr : public QThread {
    friend class QTileMapWidget;

protected:
    void run();

private:
    void setParent(QTileMapWidget* parent);

private:
    QTileMapWidget* m_ptr_QTileMapWidget;
};
enum QTMW_AsyncLoadItemState {
    QTMW_AsyncLoadItemState_None,
    QTMW_AsyncLoadItemState_Loading,
    QTMW_AsyncLoadItemState_Loaded
};
struct QTMW_AsyncLoadItem {
    QTMW_AsyncLoadItemState state = QTMW_AsyncLoadItemState_None;
    QByteArray imageArray;
};

// Async Preload
class QTMW_AsyncPreload : public QThread {
    Q_OBJECT
    friend class QTileMapWidget;

public:
    void setParm(QString _dbPath, QVector<Coord_Tile> _vecData);
    void getPreloadProgress(int* current, int* max);  //获取预加载进度
protected:
    void run();
    QString m_dbPath;
    QVector<Coord_Tile> m_data;

    static QHash<QString, QByteArray> static_PreloadLoadImageBuffer;
    int m_PreloadProgressCurrent = 0;
    int m_PreloadProgressMax = 0;
};

// QTileMapWidget
class QTileMapWidget : public QFrame {
    Q_OBJECT
    friend class QTMW_DrawGraph;
    friend class QTMW_AsyncLoadMgr;
    friend class QTMW_AsyncPreload;
    friend class QTMW_AsyncLoadImage;

public:                                                  //接口
    explicit QTileMapWidget(QWidget* parent = nullptr);  //构造函数

    //初始化相关(数据源信息相关)
    void ConnectSQLiteDB(QString _path);  //连接到指定数据库
    bool GetIsSQLiteDBValid();            //判断连接是否有效
    QString GetCurrentSQLiteDBPath();     //获取连接的数据库路径
    int GetTileMapMaxLevelInDB();         //获取数据库中最大瓦片层级
    int GetTileMapMinLevelInDB();         //获取数据库中最小瓦片层级
    QString GetLastError();               //获取错误信息

    //显示相关
    void ShowTileMapGrid(bool _show, QColor _color = Qt::red);        //显示瓦片网格
    void ShowTileMapCoord(bool _show, QColor _color = Qt::red);       //显示瓦片坐标
    void ShowCenterCrossLine(bool _show, QColor _color = Qt::green);  //显示中央十字线

    //鼠标当前坐标相关
    QPoint GetMousePosCoord_Widget();                       //获取鼠标当前的窗口坐标
    Coord_Tile GetMousePosCoord_Tile(bool* _ok = nullptr);  //获取鼠标当前的瓦片坐标
    Coord_LL GetMousePosCoord_LL(bool* _ok = nullptr);  //获取鼠标当前的经纬度(度格式)坐标
    Coord_LLMS GetMousePosCoord_LLMS(bool* _ok = nullptr);  //获取鼠标当前的经纬度(度分秒格式)坐标

    //绘图相关
    QTMW_DrawGraph* AddGraph();  //添加绘图图层

    //聚焦相关
    void SetFrozenDrag(bool frozen);              //设置是否冻结拖拽
    void MoveToPoint(Coord_LL point);             //移动当前层级的指定经纬度
    void MoveToPoint(Coord_LL point, int level);  //移动指定层级的指定经纬度
    void LimitLevel(int minLevel, int maxLevel);  //设置图层限制（限制最大最小图层）
    int GetTileMapMaxLevelInLimit();              //获取限制的最大层级
    int GetTileMapMinLevelInLimit();              //获取限制的最小层级
    int GetCurrentLevelIndex();                   //获取当前地图缩放级别

    //图片预加载
    // 注意事项，图片预加载是把图片预加载到内存中
    // QtCreator中的32位的编译器只能申请到2GB左右的内存
    // 如果需要更大的内存，需要更换64位的编译器，比如说vs2013_64bit
    void PreloadTileMapImages(Coord_LL _leftTop, Coord_LL _rightBottom, int _minZoom,
                              int _maxZoom);          //预加载两个经纬度之间的图片
    void GetPreloadProgress(int* current, int* max);  //获取预加载进度
    void StopPreload();

    //坐标转换
    // ConvertCoord_LL2LLMS - 经纬度LL格式转LLMS格式（度转度分秒）
    // ConvertCoord_LL2Tile_Mercator - 经纬度LL转瓦片坐标 （度转xyz）
    // ConvertCoord_LL2Widget - 经纬度LL转Widget坐标 （度转xy）
    // ConvertCoord_LLMS2LL - 经纬度LLMS格式转LL格式（度分秒转度）
    // ConvertCoord_Tile2LL_Mercator 瓦片坐标转经纬度LL （xyz转度）
    // ConvertCoord_Tile2Widget - 瓦片坐标转Widget坐标 （xyz转xy）
    // ConvertCoord_Widget2LL - Widget坐标转经纬度LL（xy转度）
    // ConvertCoord_Widget2LLMS - Widget坐标转经纬度LLMS（xy转度分秒）
    // ConvertCoord_Widget2Tile -  Widget坐标转瓦片坐标（xy转xyz）
    Coord_LLMS ConvertCoord_LL2LLMS(Coord_LL _pos);
    Coord_Tile ConvertCoord_LL2Tile_Mercator(Coord_LL _pos, int _level,
                                             bool* _ok = nullptr);  //经纬度转瓦片坐标（墨卡托坐标）
    QPoint ConvertCoord_LL2Widget(Coord_LL _pos, bool* _ok = nullptr);
    Coord_LL ConvertCoord_LLMS2LL(Coord_LLMS _pos);
    Coord_LL ConvertCoord_Tile2LL_Mercator(Coord_Tile _pos,
                                           bool* _ok = nullptr);  //瓦片坐标转经纬度（墨卡托坐标）
    QPoint ConvertCoord_Tile2Widget(Coord_Tile _pos, bool* _ok = nullptr);
    Coord_LL ConvertCoord_Widget2LL(QPoint _pos, bool* _ok = nullptr);
    Coord_LLMS ConvertCoord_Widget2LLMS(QPoint _pos, bool* _ok = nullptr);
    Coord_Tile ConvertCoord_Widget2Tile(QPoint _pos, bool* _ok = nullptr);

    //地图中心翻转
    void SetReversalMode(bool _reversal);
    bool GetIsReversalMode();

    //背景色
    void setBackgroundColor(QColor _color);  //设置背景色

    //回调函数-扩展paintEvent绘制
    void set_CallBackFunc(CallBackFunc _func);  //设置回调函数
    QPoint getWidgetPoint_LL2Widget(Coord_LL _pos, bool* _ok = nullptr);

    //析构相关
    void Release();  //释放相关资源

protected:                                           //窗体事件触发
    void paintEvent(QPaintEvent* e);                 //绘图事件
    void wheelEvent(QWheelEvent* event);             //鼠标滚动
    void mouseMoveEvent(QMouseEvent* event);         //鼠标移动
    void mouseDoubleClickEvent(QMouseEvent* event);  //鼠标双击
    void mousePressEvent(QMouseEvent* event);        //鼠标按下
    void mouseReleaseEvent(QMouseEvent* event);      //鼠标抬起
    void resizeEvent(QResizeEvent* event);           //窗口大小变化

    /************************************************************/
    //功能-初始化
private:                           // Func
    void ShowDefaultBackground();  //没有瓦片地图，显示默认背景
    void RespondStyleSheet();  // QWidget的派生类会有不响应StyleSheep的问题，需要在paintEvent中修正
private:                       // Variable
    QString m_LastError;

    /************************************************************/
    //功能-数据源连接(数据库连接访问)
private:  // Func
    void SQLiteCreate();
    void LoadDBInfo();

private:  // Variable
    QString m_SQLiteDBPath;
    QSqlDatabase m_SQLiteDB;
    DB_Info m_DB_Info;
    bool m_isSQLiteDBValid = false;  //数据库是否有效（是否有数据）
    int m_CurrentMapIndex_Z = -1;    //当前图层等级 0为最顶层
    int m_LimitMaxLevel = -1;        //数据库中地图最大有效等级
    int m_LimitMinLevel = -1;        //数据库中地图最小有效等级
    bool m_DBFormatVaild = false;    //数据库瓦片格式查询有效
    int m_SingleTilePixel = 256;     //单个瓦片的像素大小，一般为256，但也有512的

    /************************************************************/
    //功能-图片显示
private:  // Func
    void initMapBuffer();
    QPixmap LoadTileMapImage(int z, int x, int y);

private:                                           // Variable
    int m_tilemapIndexInBuffer0_X = 0;             //缓冲区第一个瓦片的索引X
    int m_tilemapIndexInBuffer0_Y = 0;             //缓冲区第一个瓦片的索引Y
    QVector<QVector<QPixmap>> m_DisplayMapBuffer;  //显示瓦片的缓存 QVector[x][y]
    int m_DisplayMapBufferSize_X = 17;             //缓存的容量X,按照4K屏 (3840/256)+2 = 17
    int m_DisplayMapBufferSize_Y = 10;             //缓存的容量Y,按照4K屏  (2160/256)+2 = 10

    /************************************************************/
    //功能-拖拽及缩放
private:                     // Func
    void DragMoveProcess();  //拖拽逻辑
    void UpdateDisplayMapBuffer_All();
    void UpdateDisplayMapBuffer_All_Gradual();  //利用父级图层进行渐变加载，在图层升级时使用
    void UpdateDisplayMapBuffer_AddX();
    void UpdateDisplayMapBuffer_SubX();
    void UpdateDisplayMapBuffer_AddY();
    void UpdateDisplayMapBuffer_SubY();
    int FixX_LevelUp_BeforeFullAfterFull(Coord_Tile _pos);  //如果层架变化前后，缓冲区都是满的
    int FixY_LevelUp_BeforeFullAfterFull(Coord_Tile _pos);  //如果层架变化前后，缓冲区都是满的
    int FixX_LevelUp_BeforeEmptyAfterEmpty(
        Coord_Tile _pos);  //如果升级之后，缓冲区仍然填充不满,继续填充
    int FixY_LevelUp_BeforeEmptyAfterEmpty(
        Coord_Tile _pos);  //如果升级之后，缓冲区仍然填充不满,继续填充
    int FixX_LevelUp_BeforeEmptyAfterFull(
        Coord_Tile _pos);  //如果升级之前，缓冲区未满，升级之后，缓冲区
    int FixY_LevelUp_BeforeEmptyAfterFull(
        Coord_Tile _pos);  //如果升级之前，缓冲区未满，升级之后，缓冲区
    int FixX_LevelDown_BeforeFullAfterFull(Coord_Tile _pos);  //如果层架变化前后，缓冲区都是满的
    int FixY_LevelDown_BeforeFullAfterFull(Coord_Tile _pos);  //如果层架变化前后，缓冲区都是满的
    int FixX_LevelDown_BeforeEmptyAfterEmpty(
        Coord_Tile _pos);  //如果降级之前，缓冲区已经填充不满,继续减少
    int FixY_LevelDown_BeforeEmptyAfterEmpty(
        Coord_Tile _pos);  //如果降级之前，缓冲区已经填充不满,继续减少
    int FixX_LevelDown_BeforeFullAfterEmpty(
        Coord_Tile _pos);  //如果降级之前，缓冲区满，降级之后，缓冲区不满
    int FixY_LevelDown_BeforeFullAfterEmpty(
        Coord_Tile _pos);  //如果降级之前，缓冲区满，降级之后，缓冲区不满
    void FixXY2WidgetSize();

private:                                        // Variable
    QPoint m_WidgetOffsetImage = QPoint(0, 0);  // Widget和图片缓冲区的偏差量
    bool m_mouseLeftButtonDown = false;         //鼠标左键是否被按下
    bool m_mouseRightButtonDown = false;        //鼠标右键是否被按下
    QPoint m_mousetCurrentPos = QPoint(0, 0);   //当前鼠标位置
    QPoint m_mousetPreviousPos = QPoint(0, 0);  //上一帧的鼠标位置
    QTimer m_timer_AutoLoadDataFromDB;          //拖拽效果判定计时器
    Coord_Tile m_Flag_AutoLoadDataFromDB;       //拖拽前后的基准点位置
private slots:                                  // Slots
    void slot_SingleShotInWheelEvent();
    void slot_timeout_AutoLoadDataFromDB();

    /************************************************************/
    //功能-坐标转换
private:  // Func
    bool TileXYisValid(int z, int x, int y);
    QPoint ConvertCoord_Widget2Image(QPoint _pos);  // Widget窗口坐标转ImageBuffer坐标
    QPoint ConvertCoord_Image2Widget(QPoint _pos);  // ImageBuffer坐标转Widget窗口坐标
    /************************************************************/
    //功能-异步加载
private:  // Func
    void AsynLoadTileMapImage(int buffer_x, int buffer_y, int tile_z, int tile_x,
                              int tile_y);  //异步加载瓦片地图
    void async_AddThread(QTMW_AsyncLoadImage* _ptr);
    void async_CleanCurrentThread();
    void CleanAsyncLoadImageBuffer();

private:  // Variable
    QQueue<QTMW_AsyncLoadImage*> m_AsyncLoadImageThreadQueue;
    QMutex m_mutex_AsyncLoadImage;
    QMap<int, QMap<int, QMap<int, QTMW_AsyncLoadItem>>> m_AsyncLoadImageBuffer;
    QTMW_AsyncLoadMgr* m_AsyncLoadMgr = nullptr;
private slots:  // Slots
    void slot_AsyncLoadImageLoaded(int _buffer_x, int _buffer_y, int _tile_z, int _tile_x,
                                   int _tile_y, QByteArray _imageArray);

    /************************************************************/
    //功能-聚焦定位
private:                        // Variable
    bool m_FrozenDrag = false;  // 冻结拖拽
    Coord_LL m_Point_MoveToFocus;
    Coord_LL m_Point_WidgetCenter;
    QMap<int, QTMW_DrawGraph*> m_DrawGraphBuffer;

    /************************************************************/
    //功能-辅助绘制
private:                             // Func
    void DrawDebugInfo();            //调试信息
    void DrawTileMapGrid();          //绘制瓦片网格
    void DrawTileCoord();            //绘制瓦片坐标
    void DrawCenterCrossLine();      //绘制中央十字线
private:                             // Variable
    bool m_showTileMapGrid = false;  //是否显示瓦片网格
    QColor m_colorTileMapGrid = Qt::red;
    bool m_showTileMapCoord = false;  //是否显示瓦片坐标
    QColor m_colorTileMapCoord = Qt::red;
    bool m_showCenterCrossLine = false;  //是否中央十字线
    QColor m_colorCenterCrossLine = Qt::green;

    /************************************************************/
    //功能-图片预加载
private slots:
    void slot_PreloadFinished();

private:
    QTMW_AsyncPreload m_AsyncPreload;

    /************************************************************/
    //功能-地图中心翻转
private:
    static bool m_mode_Reversal;
    static int ConvertTileX_Normal2Reversal(int x,
                                            int z);  //正常模式的瓦片X坐标转到翻转模式的瓦片X坐标

    /************************************************************/
    //功能-背景色
private:
    static QColor m_backgroundColor;

    /************************************************************/
    //功能-回调函数
private:
    void DrawCallBackFunc();  //回调绘制函数
private:
    CallBackFunc m_callBackFunc = nullptr;  //回调函数的函数指针

    /************************************************************/
    //功能-鼠标点击信号
signals:
    void signal_wheelEvent(QWheelEvent* event);             //鼠标滚动
    void signal_mouseMoveEvent(QMouseEvent* event);         //鼠标移动
    void signal_mousePressEvent(QMouseEvent* event);        //鼠标按下
    void signal_mouseReleaseEvent(QMouseEvent* event);      //鼠标抬起
    void signal_mouseDoubleClickEvent(QMouseEvent* event);  //鼠标双击
};

// QTMW_DrawItem
class QTMW_DrawItem_Text {
public:
    QString text;
    QPoint offset;
    QColor color;
    int size;

    QTMW_DrawItem_Text() {
        color = Qt::red;
        size = 12;
        offset.setX(5);
        offset.setY(0);
    }
};
class QTMW_DrawItem_Image {
    friend class QTMW_DrawItem_Point;

public:
    QImage image;
    int size;

    QTMW_DrawItem_Image() {
        size = 64;  //宽(高)像素
    }

private:
    float dir;
};
class QTMW_DrawItem_Point {
public:
    Coord_LL pos;
    QColor color;
    int size;

    QTMW_DrawItem_Text Text;
    QTMW_DrawItem_Image Image;

    QTMW_DrawItem_Point() {
        color = Qt::red;
        size = 4;  //直径像素
    }
};
class QTMW_DrawItem_Line {
public:
    Coord_LL from;
    Coord_LL to;
    QColor color;
    int width;

    QTMW_DrawItem_Text Text;

    QTMW_DrawItem_Line() {
        color = Qt::red;
        width = 1;
    }
};
class QTMW_DrawItem_Path {
public:
    QColor color;
    int width;
    QVector<Coord_LL> data;

    QTMW_DrawItem_Path() {
        color = Qt::red;
        width = 1;
    }
};
class QTMW_DrawGraph {
    friend class QTileMapWidget;

public:
    int addPoint(QTMW_DrawItem_Point point);
    int addLine(QTMW_DrawItem_Line line);
    int addPath(QTMW_DrawItem_Path path);

    bool updatePoint(int pointIndex, QTMW_DrawItem_Point point);
    bool updateLine(int lineIndex, QTMW_DrawItem_Line line);
    bool updatePath(int pathInedx, Coord_LL pathData);

    int PointCount();
    int LineCount();
    int PathCount();

    void clear();

private:
    void setParent(QTileMapWidget* parent);
    void draw();

private:
    QTileMapWidget* m_ptr_QTileMapWidget;
    QMutex m_mutex;
    QVector<QTMW_DrawItem_Point> m_DrawItem_Points;
    QVector<QTMW_DrawItem_Line> m_DrawItem_Lines;
    QVector<QTMW_DrawItem_Path> m_DrawItem_Paths;
};

#endif  // QTILEMAPWIDGET_H
