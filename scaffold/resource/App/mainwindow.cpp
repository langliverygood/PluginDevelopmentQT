#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    SMod::SModInterface *plugin = pluginLoader("ExamplePlugin");
    qDebug() << "Plugin key:" << plugin->key();

    this->setCentralWidget(plugin->widget());
}

SMod::SModInterface *MainWindow::pluginLoader(const QString &name) {
    // 插件所在目录（在实际项目中需要根据路径调整）
    QString pluginPath = qApp->applicationDirPath() + QString("/%1.dll").arg(name);  // Windows
    // QString pluginPath = QCoreApplication::applicationDirPath() + "/libMyPlugin.so"; // Linux

    // 加载插件
    QPluginLoader loader(pluginPath);
    QObject *pluginInstance = loader.instance();

    if (!pluginInstance) {
        qWarning() << "Failed to load plugin:" << loader.errorString();
        return nullptr;
    }

    // 转换为接口类型
    SMod::SModInterface *plugin = qobject_cast<SMod::SModInterface *>(pluginInstance);
    if (!plugin) {
        qWarning() << "Failed to cast plugin instance to PluginInterface";
        return nullptr;
    }

    return plugin;
}
