#ifndef PLUGINSMANAGER_H
#define PLUGINSMANAGER_H

#include <QApplication>
#include <QDir>
#include <QHash>
#include <QPluginLoader>

#include "smodinterface.h"

#define Manager PluginsManager::instance()

#define PluginsManagerKey "main"

class PluginsManager : public QObject {
    Q_OBJECT
public:
    static PluginsManager *instance();

    // 加载核心插件
    int loadCores();
    // 加载指定插件
    SMod::SModInterface *load(const QString &key);
    // 卸载所有插件
    void unloadCores();
    // 卸载指定插件
    bool unload(const QString &key);
    // 获取所有插件 key
    QStringList keys();
    // 获取所有插件
    QList<SMod::SModInterface *> all();
    // 查找插件
    SMod::SModInterface *find(const QString &key);

private:
    static PluginsManager *instance_;
    QString plugin_path_;
    QHash<QString, QPluginLoader *> cache_plugins_;

    explicit PluginsManager(QObject *parent = nullptr);

private slots:
    void receive(const SMod::ModMetaData &datagram);

signals:
    void send(const SMod::ModMetaData &datagram);
};

#endif  // PLUGINSMANAGER_H
