#include "pluginsmanager.h"
#include <QDir>
#include <QApplication>
#include <QDebug>

Q_GLOBAL_STATIC(PluginsManager,pluginsmanager)

PluginsManager::PluginsManager(QObject *parent) : QObject(parent)
{
    init();
}

PluginsManager *PluginsManager::instance()
{
    return  pluginsmanager;
}

void PluginsManager::init()
{
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cd("core");
    pluginPath = dir.absolutePath();
}

int PluginsManager::loadCores()
{
    cachePlugins.clear();
    QDir dir(pluginPath);
    const QStringList entries = dir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader *loader = new QPluginLoader(dir.absoluteFilePath(fileName));
        if(loader->load()){
            SMod::SModInterface *smod = qobject_cast<SMod::SModInterface *>(loader->instance());
            if(smod){
                cachePlugins.insert(smod->key(),loader);
                connect(loader->instance(),SIGNAL(send(const SMod::ModMetaData &)),this,SLOT(receive(const SMod::ModMetaData &)));
            }
        }else{
            delete loader;
            loader = nullptr;
        }
    }
    return  cachePlugins.size();
}

SMod::SModInterface * PluginsManager::load(const QString &key)
{
    if(!cachePlugins.contains(key)){
        QDir dir(pluginPath);
        QPluginLoader *loader = new QPluginLoader(dir.absoluteFilePath(key));
        if(loader->load()){
            SMod::SModInterface *smod = qobject_cast<SMod::SModInterface *>(loader->instance());
            if(smod){
                cachePlugins.insert(smod->key(),loader);
                connect(loader->instance(),SIGNAL(send(const SMod::ModMetaData &)),this,SLOT(receive(const SMod::ModMetaData &)));
                return smod;
            }
        }else{
            delete loader;
            loader = nullptr;
        }
        return nullptr;
    }
    return nullptr;
}

void PluginsManager::unloadCores()
{
    foreach (auto var, cachePlugins)
        var->unload();
    cachePlugins.clear();
}

bool PluginsManager::unload(const QString &key)
{
    if(cachePlugins.contains(key)){
        cachePlugins.value(key)->unload();
        cachePlugins.remove(key);
        return true;
    }
    return false;
}

QStringList PluginsManager::keys()
{
    return cachePlugins.keys();
}

QList<SMod::SModInterface *> PluginsManager::all()
{
    QList<SMod::SModInterface *> mods;
    foreach (auto var, cachePlugins)
        mods.append(qobject_cast<SMod::SModInterface *>(var->instance()));
    return mods;
}

SMod::SModInterface *PluginsManager::find(const QString &key)
{
    if(cachePlugins.contains(key))
        return qobject_cast<SMod::SModInterface *>(cachePlugins.value(key)->instance());
    return nullptr;
}

void PluginsManager::receive(const SMod::ModMetaData &datagram)
{
//    emit pluginsReceive(datagram);

    if(cachePlugins.contains(datagram.dest)){
        auto loader = cachePlugins.value(datagram.dest);
        auto smod = qobject_cast<SMod::SModInterface *>(loader->instance());
        smod->receive(datagram);
    }

    else if(QString("main").contains(datagram.dest)){
        emit send(datagram);
    }

}

