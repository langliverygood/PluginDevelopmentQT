#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QDebug>
#include <QGlobalStatic>
#include <QObject>
#include <QProcess>

// #define cmd CommandLine::instance()

class CommandLine : public QObject {
    Q_OBJECT
public:
    explicit CommandLine(QObject *parent = nullptr);
    static CommandLine &instance();

    void runexe(const QString &exe, QStringList args = QString("").split(","));

private:
    QProcess process;
    QByteArray output;
signals:
    void finished(const QByteArray &data);
public slots:
};

#endif  // COMMANDLINE_H
