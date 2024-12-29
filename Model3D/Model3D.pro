TEMPLATE = subdirs
#CONFIG += ordered

SUBDIRS += \
    App \
    Plugin

# 子项目
SUBDIRS += App Plugin

# 设置编译顺序
App.depends = Plugin

