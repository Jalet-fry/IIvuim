#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>

void logToFile(const QString &message)
{
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IIvuim_App_Logs/main_app.log";
    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "] " << message << "\n";
    }
}

int main(int argc, char *argv[])
{
    logToFile("=== APPLICATION START ===");
    qDebug() << "=== APPLICATION START ===";
    
    QApplication a(argc, argv);
    logToFile("QApplication created");
    
    // НЕ устанавливаем setQuitOnLastWindowClosed(false) глобально
    // Это будет управляться динамически в CameraWindow
    logToFile("QuitOnLastWindowClosed will be managed dynamically");

    MainWindow w;
    logToFile("MainWindow created");
    
    w.show();
    logToFile("MainWindow shown");
    
    logToFile("Starting event loop...");
    int result = a.exec();
    
    logToFile(QString("Application finished with code: %1").arg(result));
    qDebug() << "Application finished with code:" << result;
    
    return result;
}