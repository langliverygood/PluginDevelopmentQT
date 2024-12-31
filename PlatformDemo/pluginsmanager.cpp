#include "pluginsmanager.h"

PluginsManager *PluginsManager::instance_ = nullptr;

PluginsManager::PluginsManager(QObject *parent) : QObject(parent) {
    // 饿汉式单例会出问题
    // QCoreApplication::applicationDirPath: Please instantiate the QApplication object first
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cd("core");
    plugin_path_ = dir.absolutePath();
}

PluginsManager *PluginsManager::instance() {
    if (instance_ == nullptr) {
        instance_ = new PluginsManager();
    }
    return instance_;
}

int PluginsManager::loadCores() {
    cache_plugins_.clear();
    QDir dir(plugin_path_);
    const QStringList entries = dir.entryList(QDir::Files);
    for (const QString &file_name : entries) {
        QPluginLoader *loader = new QPluginLoader(dir.absoluteFilePath(file_name));
        if (loader->load()) {
            SMod::SModInterface *smod = qobject_cast<SMod::SModInterface *>(loader->instance());
            if (smod) {
                cache_plugins_.insert(smod->key(), loader);
                connect(loader->instance(), SIGNAL(send(const SMod::ModMetaData &)), this,
                        SLOT(receive(const SMod::ModMetaData &)));
            }
        } else {
            delete loader;
            loader = nullptr;
        }
    }
    return cache_plugins_.size();
}

SMod::SModInterface *PluginsManager::load(const QString &key) {
    if (!cache_plugins_.contains(key)) {
        QDir dir(plugin_path_);
        QPluginLoader *loader = new QPluginLoader(dir.absoluteFilePath(key));
        if (loader->load()) {
            SMod::SModInterface *smod = qobject_cast<SMod::SModInterface *>(loader->instance());
            if (smod) {
                cache_plugins_.insert(smod->key(), loader);
                connect(loader->instance(), SIGNAL(send(const SMod::ModMetaData &)), this,
                        SLOT(receive(const SMod::ModMetaData &)));
                return smod;
            }
        } else {
            delete loader;
            loader = nullptr;
        }
        return nullptr;
    }
    return nullptr;
}

void PluginsManager::unloadCores() {
    foreach (QPluginLoader *var, cache_plugins_) { var->unload(); }
    cache_plugins_.clear();
}

bool PluginsManager::unload(const QString &key) {
    if (cache_plugins_.contains(key)) {
        cache_plugins_.value(key)->unload();
        cache_plugins_.remove(key);
        return true;
    }
    return false;
}

QStringList PluginsManager::keys() {
    return cache_plugins_.keys();
}

QList<SMod::SModInterface *> PluginsManager::all() {
    QList<SMod::SModInterface *> mods;
    foreach (QPluginLoader *var, cache_plugins_) {
        mods.append(qobject_cast<SMod::SModInterface *>(var->instance()));
    }
    return mods;
}

SMod::SModInterface *PluginsManager::find(const QString &key) {
    if (cache_plugins_.contains(key)) {
        return qobject_cast<SMod::SModInterface *>(cache_plugins_.value(key)->instance());
    }
    return nullptr;
}

void PluginsManager::receive(const SMod::ModMetaData &datagram) {
    if (cache_plugins_.contains(datagram.dest)) {
        QPluginLoader *loader = cache_plugins_.value(datagram.dest);
        SMod::SModInterface *smod = qobject_cast<SMod::SModInterface *>(loader->instance());
        smod->receive(datagram);
    }

    else if (QString(PluginsManagerKey).contains(datagram.dest)) {
        emit send(datagram);
    }
}
