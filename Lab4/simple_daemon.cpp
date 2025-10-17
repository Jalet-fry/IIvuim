#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <windows.h>

// Простой демон для Lab4 - каждые 5 секунд снимает видео на 4 секунды
class SimpleDaemon : public QObject
{
    Q_OBJECT

public:
    SimpleDaemon(QObject *parent = nullptr) : QObject(parent)
    {
        // Создаем таймер для периодических видео
        m_videoTimer = new QTimer(this);
        m_videoTimer->setSingleShot(false);
        m_videoTimer->setInterval(5000); // 5 секунд между видео
        connect(m_videoTimer, &QTimer::timeout, this, &SimpleDaemon::startVideo);
        
        // Создаем таймер для остановки видео
        m_stopTimer = new QTimer(this);
        m_stopTimer->setSingleShot(true);
        m_stopTimer->setInterval(4000); // 4 секунды записи
        connect(m_stopTimer, &QTimer::timeout, this, &SimpleDaemon::stopVideo);
        
        // Создаем лог файл
        createLogFile();
        
        logMessage("Simple Daemon started - Lab4 mode: 5s interval, 4s video duration");
    }
    
    void start()
    {
        logMessage("Starting video timer...");
        m_videoTimer->start();
        logMessage("Daemon is now running in background");
    }
    
    void stop()
    {
        m_videoTimer->stop();
        m_stopTimer->stop();
        logMessage("Daemon stopped");
    }

private slots:
    void startVideo()
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        logMessage(QString("Starting video recording at %1").arg(timestamp));
        
        // Здесь должна быть логика запуска записи видео
        // Пока просто логируем
        
        // Запускаем таймер остановки
        m_stopTimer->start();
    }
    
    void stopVideo()
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        logMessage(QString("Stopping video recording at %1").arg(timestamp));
        
        // Здесь должна быть логика остановки записи видео
        // Пока просто логируем
    }

private:
    void createLogFile()
    {
        QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QString logDir = documentsPath + "/Lab4_StealthLogs";
        
        QDir dir;
        if (!dir.exists(logDir)) {
            dir.mkpath(logDir);
        }
        
        QString logPath = logDir + "/simple_daemon_" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".log";
        m_logFile = new QFile(logPath, this);
        
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_logStream = new QTextStream(m_logFile);
            *m_logStream << "\n=== SIMPLE DAEMON STARTED ===" << endl;
            *m_logStream << "Time: " << QDateTime::currentDateTime().toString() << endl;
            *m_logStream << "Mode: Lab4 - 5s interval, 4s video duration" << endl;
            m_logStream->flush();
        }
    }
    
    void logMessage(const QString &message)
    {
        if (m_logStream) {
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
            *m_logStream << QString("[%1] %2").arg(timestamp).arg(message) << endl;
            m_logStream->flush();
        }
        qDebug() << message;
    }
    
    QTimer *m_videoTimer;
    QTimer *m_stopTimer;
    QFile *m_logFile;
    QTextStream *m_logStream;
};

// Функция для скрытия консольного окна
void hideConsoleWindow()
{
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) {
        ShowWindow(hwnd, SW_HIDE);
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
        out << "Simple Daemon is running\n";
        out << "Started at: " << QDateTime::currentDateTime().toString() << "\n";
        out << "PID: " << QCoreApplication::applicationPid() << "\n";
        out << "Mode: Lab4 - 5s interval, 4s video duration\n";
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

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Скрываем консольное окно для фоновой работы
    hideConsoleWindow();
    
    qDebug() << "Simple Daemon starting...";
    
    // Создаем файл-маркер для отслеживания работы демона
    createDaemonMarker();
    
    // Создаем и запускаем демон
    SimpleDaemon daemon;
    daemon.start();
    
    qDebug() << "Simple Daemon started successfully in background";
    
    // Ждем завершения приложения
    int result = app.exec();
    
    // Удаляем файл-маркер
    removeDaemonMarker();
    
    qDebug() << "Simple Daemon finished";
    return result;
}

#include "simple_daemon.moc"
