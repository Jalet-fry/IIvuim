#ifndef LAB4_LOGGER_H
#define LAB4_LOGGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QStandardPaths>
#include <QDir>

class Lab4Logger : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        DEBUG_LEVEL = 0,
        INFO_LEVEL = 1,
        WARNING_LEVEL = 2,
        ERROR_LEVEL = 3
    };

    enum LogCategory {
        GENERAL,           // Общие события Lab4
        CAMERA,            // События камеры
        STEALTH_MODE,      // Скрытый режим (горячие клавиши)
        STEALTH_DAEMON,    // Скрытый демон (keylogger)
        AUTOMATIC_MODE,    // Автоматический режим
        JAKE_WARNING,      // Jake предупреждения
        HOTKEYS,           // Глобальные горячие клавиши
        UI_EVENTS,         // UI события
        FILE_OPERATIONS,   // Операции с файлами
        SYSTEM_EVENTS      // Системные события
    };

    static Lab4Logger* instance();
    
    // Основные методы логирования
    void log(LogCategory category, LogLevel level, const QString &message);
    void logDebug(LogCategory category, const QString &message);
    void logInfo(LogCategory category, const QString &message);
    void logWarning(LogCategory category, const QString &message);
    void logError(LogCategory category, const QString &message);
    
    // Удобные методы для каждого режима
    void logCameraEvent(const QString &message);
    void logStealthModeEvent(const QString &message);
    void logStealthDaemonEvent(const QString &message);
    void logAutomaticModeEvent(const QString &message);
    void logJakeEvent(const QString &message);
    void logHotkeyEvent(const QString &message);
    void logUIEvent(const QString &message);
    void logFileEvent(const QString &message);
    void logSystemEvent(const QString &message);
    
    // Настройки
    void setLogLevel(LogLevel level) { m_logLevel = level; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void setLogToConsole(bool enabled) { m_logToConsole = enabled; }
    
    // Получение путей к лог файлам
    QString getLogDirectory() const;
    QString getLogFilePath(LogCategory category) const;
    
    // Очистка старых логов
    void cleanupOldLogs(int daysToKeep = 7);

signals:
    void logMessageGenerated(const QString &category, const QString &level, const QString &message);

private:
    explicit Lab4Logger(QObject *parent = nullptr);
    ~Lab4Logger();
    
    // Создание лог файлов
    void ensureLogFileExists(LogCategory category);
    QString getCategoryName(LogCategory category) const;
    QString getLevelName(LogLevel level) const;
    QString formatMessage(LogCategory category, LogLevel level, const QString &message) const;
    
    // Настройки
    LogLevel m_logLevel;
    bool m_enabled;
    bool m_logToConsole;
    
    // Лог файлы
    QHash<LogCategory, QFile*> m_logFiles;
    QHash<LogCategory, QTextStream*> m_logStreams;
    QMutex m_logMutex;
    
    // Путь к папке логов
    QString m_logDirectory;
    
    static Lab4Logger* s_instance;
};

// Макросы для удобного использования
#define LAB4_LOG_DEBUG(category, message) Lab4Logger::instance()->logDebug(Lab4Logger::category, message)
#define LAB4_LOG_INFO(category, message) Lab4Logger::instance()->logInfo(Lab4Logger::category, message)
#define LAB4_LOG_WARNING(category, message) Lab4Logger::instance()->logWarning(Lab4Logger::category, message)
#define LAB4_LOG_ERROR(category, message) Lab4Logger::instance()->logError(Lab4Logger::category, message)

#define LAB4_LOG_CAMERA(message) Lab4Logger::instance()->logCameraEvent(message)
#define LAB4_LOG_STEALTH_MODE(message) Lab4Logger::instance()->logStealthModeEvent(message)
#define LAB4_LOG_STEALTH_DAEMON(message) Lab4Logger::instance()->logStealthDaemonEvent(message)
#define LAB4_LOG_AUTOMATIC_MODE(message) Lab4Logger::instance()->logAutomaticModeEvent(message)
#define LAB4_LOG_JAKE(message) Lab4Logger::instance()->logJakeEvent(message)
#define LAB4_LOG_HOTKEY(message) Lab4Logger::instance()->logHotkeyEvent(message)
#define LAB4_LOG_UI(message) Lab4Logger::instance()->logUIEvent(message)
#define LAB4_LOG_FILE(message) Lab4Logger::instance()->logFileEvent(message)
#define LAB4_LOG_SYSTEM(message) Lab4Logger::instance()->logSystemEvent(message)

#endif // LAB4_LOGGER_H
