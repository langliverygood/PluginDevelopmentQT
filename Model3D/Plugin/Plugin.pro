TEMPLATE = lib
TARGET = Model3DPlugin

QT += opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT -= gui

# 定义插件相关的宏
DEFINES += QT_PLUGIN \
           QT_SHARED

CONFIG += plugin

# 头文件
HEADERS += \
    appGLWidget.h \
    model3d.h \
    smodinterface.h \
    stlasciimodel.h

# 源代码文件
SOURCES += \
    appGLWidget.cpp \
    model3d.cpp \
    stlasciimodel.cpp

# 资源文件
RESOURCES += \
    res.qrc

# 插件的输出目录
DESTDIR = ../bin

# 插件配置文件
DISTFILES += model3d.json

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
        LIBS+= -lOpengl32 -glu32
    }
} else:win32:CONFIG(release, debug|release){
    contains(DEFINES, WIN64) {
    } else {
        LIBS+= -lOpengl32 -glu32
    }
}
