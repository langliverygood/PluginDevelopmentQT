#include "commandline.h"

Q_GLOBAL_STATIC(CommandLine, commandlineInstance)

CommandLine::CommandLine(QObject *parent) : QObject(parent) {
    // 连接 readyReadStandardOutput() 信号以异步读取标准输出
    QObject::connect(&process, &QProcess::readyReadStandardOutput, [&]() {
        // 读取标准输出并打印到控制台
        output = process.readAllStandardOutput();
        //    qDebug() << "Output: " << QString::fromUtf8(output);
    });

    // 连接 finished() 信号以获取进程完成信号
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [&](int exitCode, QProcess::ExitStatus exitStatus) {
                         //        qDebug() << "Process finished with exit code:" << exitCode;
                         //        qDebug() << "Exit status:" << exitStatus;
                         emit finished(output);
                     });
}

CommandLine &CommandLine::instance() {
    return *commandlineInstance();
}

void CommandLine::runexe(const QString &exe, QStringList args) {
    output.clear();
    // 在Windows平台上，使用 cmd.exe 来执行命令
    QString program = "cmd.exe";  // CMD命令提示符
    QStringList arguments;
    arguments.append("/C");
    arguments.append(exe);
    arguments.append(args);

    // 设置命令和参数
    process.start(program, arguments);
}
