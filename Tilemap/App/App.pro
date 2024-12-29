TEMPLATE = app
TARGET = Tilemap

QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 源代码文件
SOURCES += \
    main.cpp \
    mainwindow.cpp

# 头文件
HEADERS += \
    mainwindow.h \
    smodinterface.h

# 应用程序的输出目录
DESTDIR = ../bin

# 设置动态加载插件路径
#QMAKE_POST_LINK += $$quote(cp -r ../bin $$DESTDIR/plugins)

#指定编译产生的文件目录
MOC_DIR     = temp/moc
RCC_DIR     = temp/rcc
UI_DIR      = temp/ui
OBJECTS_DIR = temp/obj

#关闭警告提示
CONFIG += warn_off
#开启大资源支持
CONFIG += resources_big
#打印信息控制台输出
#CONFIG += console
#不生成空的 debug release 目录
CONFIG -= debug_and_release

win32:CONFIG(debug, debug|release){
    contains(DEFINES, WIN64) {
    } else {
    }
} else:win32:CONFIG(release, debug|release){
    contains(DEFINES, WIN64) {
    } else {
    }
}
