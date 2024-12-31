#ifndef PLUGINSMANAGER_H
#define PLUGINSMANAGER_H

#include <QObject>
#include <QGlobalStatic>
#include <QHash>
#include <QPluginLoader>
#include "smodinterface.h"

#define Manager PluginsManager::instance()

class PluginsManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginsManager(QObject *parent = nullptr);

    static PluginsManager* instance();
public:
    // 加载核心插件
    int loadCores();
    // 加载指定插件
    SMod::SModInterface * load(const QString &key);
    // 卸载所有插件
    void unloadCores();
    // 卸载指定插件
    bool unload(const QString &key);
    // 获取所有插件 key
    QStringList keys();
    // 获取所有插件
    QList<SMod::SModInterface *> all();
    SMod::SModInterface *find(const QString &key);

private:
    QString pluginPath;
    QHash<QString,QPluginLoader *> cachePlugins;

private:
    void init();

private slots:
    void receive(const SMod::ModMetaData &datagram);

signals:
    void send(const SMod::ModMetaData &datagram);

};

#endif // PLUGINSMANAGER_H
