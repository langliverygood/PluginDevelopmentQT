TEMPLATE = lib
TARGET = VideoPlayerPlugin

QT += multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT -= gui

# 定义插件相关的宏
DEFINES += QT_PLUGIN \
           QT_SHARED

CONFIG += plugin

# 头文件
HEADERS += \
    videohandler.h \
    videoplayer.h \
    smodinterface.h

# 源代码文件
SOURCES += \
    videohandler.cpp \
    videoplayer.cpp

# 资源文件
RESOURCES += \
    res.qrc

# 插件的输出目录
DESTDIR = ../bin

# 插件配置文件
DISTFILES += videoplayer.json

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
