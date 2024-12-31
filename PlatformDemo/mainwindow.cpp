#include "mainwindow.h"
#include <QLayout>
#include <QFontDatabase>
#include <QFile>
#include <QStackedWidget>
#include <QTimer>
#include <QDebug>
#include <QLabel>

#include "pluginsmanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QStackedWidget *stack_widget = new QStackedWidget;
    stack_widget->setObjectName("stack_widget");
    this->setCentralWidget(stack_widget);
    QFont HarmonyOS_Sans_SC_Regular = loadFonts();
    this->setFont(HarmonyOS_Sans_SC_Regular);
    this->setStyleSheet(loadStyleSheet());
    this->setMinimumSize(800,600);
    this->showFullScreen();

    QWidget *lodding_widget = new QWidget;
    lodding_widget->setObjectName("lodding_widget");
    stack_widget->addWidget(lodding_widget);
    QVBoxLayout *lodding_layout = new QVBoxLayout(lodding_widget);
    lodding_layout->addStretch(1);
    QLabel *sws_zh_label = new QLabel;
    sws_zh_label->setObjectName("sws_zh_label");
    sws_zh_label->setText("海上武器控制与测量数字化平台");
    sws_zh_label->setFont(HarmonyOS_Sans_SC_Regular);
    lodding_layout->addWidget(sws_zh_label,0,Qt::AlignHCenter);
    QLabel *sws_en_label = new QLabel;
    sws_en_label->setObjectName("sws_en_label");
    sws_en_label->setFont(HarmonyOS_Sans_SC_Regular);
    sws_en_label->setText("SEA WEAPON CONTROL AND SUSVEY DIGTAL PLATFLAM");
    QFont label_font = sws_en_label->font();
    label_font.setLetterSpacing(QFont::AbsoluteSpacing,10);
    sws_en_label->setFont(label_font);
    lodding_layout->addWidget(sws_en_label,0,Qt::AlignHCenter);
    lodding_layout->addStretch(2);

    QWidget *main_widget = initUI();
    stack_widget->addWidget(main_widget);

    // 加载插件
    addPlugins();


    // 模拟加载
    QTimer::singleShot(3000, this,[=](){
        stack_widget->setCurrentIndex(2);
    });

    connect(Manager,&PluginsManager::send,this,[=](const SMod::ModMetaData &datagram){
        //        QMetaObject::invokeMethod(this, functionName.toUtf8().data());

        qDebug() << datagram.from << datagram.method;
        if(datagram.from == "LoginMod" && datagram.method == "loginSucceedSignal"){
            jump2Page(1);
        }
        if(datagram.from == "Navigation" && datagram.method == "naviPlugManager"){
            changeStacked(1);
        }
        if(datagram.from == "Navigation" && datagram.method == "mainBodyPageShow"){
            changeStacked(0);
        }
    });
}

MainWindow::~MainWindow()
{
}

QWidget* MainWindow::initUI()
{
    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(24,24,24,24);
    QWidget *left_menu_widget = new QWidget;
    left_menu_widget->setFixedWidth(114);
    QHBoxLayout *menu_layout = new QHBoxLayout(left_menu_widget);
    menu_layout->setContentsMargins(0,0,0,0);
    menu_layout->setObjectName("menu_layout");
    layout->addWidget(left_menu_widget);

    QStackedWidget *center_stackedwidget = new QStackedWidget;
    center_stackedwidget->setObjectName("center_stackedwidget");
    layout->addWidget(center_stackedwidget);
    QWidget *right_center_widget = new QWidget;
    center_stackedwidget->addWidget(right_center_widget);

    QHBoxLayout *right_center_layout = new QHBoxLayout(right_center_widget);
    right_center_layout->setContentsMargins(0,0,0,0);
    QVBoxLayout *center_layout = new QVBoxLayout;
    center_layout->setObjectName("center_layout");
    center_layout->setContentsMargins(16,0,16,0);
    center_layout->setSpacing(10);
    right_center_layout->addLayout(center_layout);
    QWidget *right_widget = new QWidget;
    right_widget->setFixedWidth(450);
    QVBoxLayout *right_layout = new QVBoxLayout(right_widget);
    right_layout->setSpacing(10);
    right_layout->setObjectName("right_layout");
    right_layout->setContentsMargins(0,0,0,0);
    right_center_layout->addWidget(right_widget);

    return widget;
}

QString MainWindow::loadStyleSheet()
{
    QString sheet;
    QFile file(":/resource/style/def.qss");
    if(file.open(QIODevice::ReadOnly))
        sheet = file.readAll();
    file.close();
    return sheet;
}

QFont MainWindow::loadFonts()
{
    QFont HarmonyOS_Sans_SC_Regular;
    // 加载字体
    int fontId = QFontDatabase::addApplicationFont(":/resource/fonts/HarmonyOS_Sans_SC_Regular.ttf");
    if (fontId != -1) {
        QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
        // 设置字体
        HarmonyOS_Sans_SC_Regular = QFont (fontFamily, 12);
    }
    return HarmonyOS_Sans_SC_Regular;
}

void MainWindow::addPlugins()
{
    // 加载插件
    int plugins_size = Manager->loadCores();
    qDebug() << QString("已加载 %1 个插件").arg(plugins_size);
    foreach (auto var, Manager->all()) {
        qDebug()<<var->key();

    }
    // 登录
    auto login_plugin =  Manager->find("LoginMod");
    if(login_plugin)
        this->findChild<QStackedWidget *>("stack_widget")->addWidget(login_plugin->widget());

    // 装载菜单
    auto nav_plugin =  Manager->find("Navigation");
    if(nav_plugin)
        this->findChild<QHBoxLayout *>("menu_layout")->addWidget(nav_plugin->widget());

    // 模拟海图
    auto tilemap_plugin =  Manager->find("TileMapMod");
    if(tilemap_plugin){
        qDebug() << "sdfsdfds";
        this->findChild<QVBoxLayout *>("center_layout")->addWidget(tilemap_plugin->widget());
    }
    // MDS
    auto sys_plugin =  Manager->find("SysstemResourves");
    if(sys_plugin){
        QWidget *mds_8 = sys_plugin->widget(8);
        mds_8->setFixedHeight(260);
        this->findChild<QVBoxLayout *>("center_layout")->addWidget(mds_8);
        this->findChild<QVBoxLayout *>("center_layout")->addWidget(sys_plugin->widget(9));
    }

    // 视频信息
    auto video_plugin =  Manager->find("MulMediaVideo");
    if(video_plugin){
        QWidget *video_widget = video_plugin->widget();
        video_widget->setFixedHeight(623);
        this->findChild<QVBoxLayout *>("right_layout")->addWidget(video_widget);
    }

    // 模块信息
    auto list_plugin =  Manager->find("PlugManager");
    if(list_plugin){
        this->findChild<QVBoxLayout *>("right_layout")->addWidget(list_plugin->widget(1));
        this->findChild<QStackedWidget *>("center_stackedwidget")->addWidget(list_plugin->widget());
    }
}

void MainWindow::jump2Page(int index)
{
    QStackedWidget *stack_widget = this->findChild<QStackedWidget *>("stack_widget");
    if(index <= stack_widget->depth())
        stack_widget->setCurrentIndex(index);
}

void MainWindow::changeStacked(int index)
{
    QStackedWidget *stack_widget = this->findChild<QStackedWidget *>("center_stackedwidget");
    if(index <= stack_widget->depth())
        stack_widget->setCurrentIndex(index);
}
