#ifndef SMOD_H
#define SMOD_H

#include <QByteArray>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QWidget>
#include <QtPlugin>

namespace SMod {

typedef struct _mod_meta_data {
    QString from;     // 消息来源 -> 插件 key(UUID)
    QString dest;     // 消息去向 -> 插件 key
    QString method;   // 请求函数名
    QByteArray data;  // 数据
} ModMetaData;

class SModInterface {
public:
    virtual ~SModInterface() {}
    virtual QString key() = 0;
    virtual QWidget *widget(int index = 0) = 0;
    virtual QStringList help() = 0;
    virtual void receive(const ModMetaData &) = 0;  //接收请求
signals:
    virtual void send(const ModMetaData &) = 0;  //发送请求
};

}  // namespace SMod

QT_BEGIN_NAMESPACE

Q_DECLARE_METATYPE(SMod::ModMetaData)

#define SMODINTERFACE_IID "xxx.modules.SModInterface"

Q_DECLARE_INTERFACE(SMod::SModInterface, SMODINTERFACE_IID)

QT_END_NAMESPACE

#endif  // SMOD_H
