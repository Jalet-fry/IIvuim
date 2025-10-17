#include "lab4_logger.h"
#include <QDebug>
#include <QCoreApplication>

Lab4Logger* Lab4Logger::s_instance = nullptr;

Lab4Logger* Lab4Logger::instance()
{
    if (!s_instance) {
        s_instance = new Lab4Logger();
    }
    return s_instance;
}

Lab4Logger::Lab4Logger(QObject *parent)
    : QObject(parent),
      m_logLevel(INFO_LEVEL),
      m_enabled(true),
      m_logToConsole(true)
{
    // Создаем папку для логов
    m_logDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Lab4_DetailedLogs";
    QDir dir;
    if (!dir.exists(m_logDirectory)) {
        dir.mkpath(m_logDirectory);
    }
    
    qDebug() << "Lab4Logger initialized. Log directory:" << m_logDirectory;
    
    // Логируем инициализацию
    logSystemEvent("Lab4Logger initialized successfully");
}

Lab4Logger::~Lab4Logger()
{
    logSystemEvent("Lab4Logger shutting down");
    
    // Закрываем все лог файлы
    for (auto it = m_logStreams.begin(); it != m_logStreams.end(); ++it) {
        if (it.value()) {
            delete it.value();
        }
    }
    
    for (auto it = m_logFiles.begin(); it != m_logFiles.end(); ++it) {
        if (it.value()) {
            it.value()->close();
            delete it.value();
        }
    }
    
    qDebug() << "Lab4Logger destroyed";
}

void Lab4Logger::log(LogCategory category, LogLevel level, const QString &message)
{
    if (!m_enabled || level < m_logLevel) {
        return;
    }
    
    QMutexLocker locker(&m_logMutex);
    
    // Обеспечиваем существование лог файла
    ensureLogFileExists(category);
    
    // Форматируем сообщение
    QString formattedMessage = formatMessage(category, level, message);
    
    // Записываем в файл
    if (m_logStreams.contains(category) && m_logStreams[category]) {
        *m_logStreams[category] << formattedMessage << endl;
        m_logStreams[category]->flush();
    }
    
    // Выводим в консоль если включено
    if (m_logToConsole) {
        qDebug() << formattedMessage;
    }
    
    // Отправляем сигнал
    emit logMessageGenerated(getCategoryName(category), getLevelName(level), message);
}

void Lab4Logger::logDebug(LogCategory category, const QString &message)
{
    log(category, DEBUG_LEVEL, message);
}

void Lab4Logger::logInfo(LogCategory category, const QString &message)
{
    log(category, INFO_LEVEL, message);
}

void Lab4Logger::logWarning(LogCategory category, const QString &message)
{
    log(category, WARNING_LEVEL, message);
}

void Lab4Logger::logError(LogCategory category, const QString &message)
{
    log(category, ERROR_LEVEL, message);
}

void Lab4Logger::logCameraEvent(const QString &message)
{
    logInfo(CAMERA, message);
}

void Lab4Logger::logStealthModeEvent(const QString &message)
{
    logInfo(STEALTH_MODE, message);
}

void Lab4Logger::logStealthDaemonEvent(const QString &message)
{
    logInfo(STEALTH_DAEMON, message);
}

void Lab4Logger::logAutomaticModeEvent(const QString &message)
{
    logInfo(AUTOMATIC_MODE, message);
}

void Lab4Logger::logJakeEvent(const QString &message)
{
    logInfo(JAKE_WARNING, message);
}

void Lab4Logger::logHotkeyEvent(const QString &message)
{
    logInfo(HOTKEYS, message);
}

void Lab4Logger::logUIEvent(const QString &message)
{
    logInfo(UI_EVENTS, message);
}

void Lab4Logger::logFileEvent(const QString &message)
{
    logInfo(FILE_OPERATIONS, message);
}

void Lab4Logger::logSystemEvent(const QString &message)
{
    logInfo(SYSTEM_EVENTS, message);
}

QString Lab4Logger::getLogDirectory() const
{
    return m_logDirectory;
}

QString Lab4Logger::getLogFilePath(LogCategory category) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd");
    QString categoryName = getCategoryName(category);
    return QString("%1/%2_%3.log").arg(m_logDirectory).arg(categoryName).arg(timestamp);
}

void Lab4Logger::cleanupOldLogs(int daysToKeep)
{
    QDir logDir(m_logDirectory);
    if (!logDir.exists()) {
        return;
    }
    
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-daysToKeep);
    
    QStringList filters;
    filters << "*.log";
    
    QFileInfoList files = logDir.entryInfoList(filters, QDir::Files);
    
    int deletedCount = 0;
    for (const QFileInfo &fileInfo : files) {
        if (fileInfo.lastModified() < cutoffDate) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                deletedCount++;
            }
        }
    }
    
    logSystemEvent(QString("Cleaned up %1 old log files").arg(deletedCount));
}

void Lab4Logger::ensureLogFileExists(LogCategory category)
{
    if (m_logFiles.contains(category) && m_logFiles[category]) {
        return; // Файл уже открыт
    }
    
    QString filePath = getLogFilePath(category);
    
    QFile *file = new QFile(filePath);
    if (file->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logFiles[category] = file;
        m_logStreams[category] = new QTextStream(file);
        
        // Записываем заголовок при первом открытии
        if (file->size() == 0) {
            *m_logStreams[category] << QString("=== %1 LOG STARTED ===").arg(getCategoryName(category).toUpper()) << endl;
            *m_logStreams[category] << QString("Started at: %1").arg(QDateTime::currentDateTime().toString()) << endl;
            *m_logStreams[category] << QString("Application: %1").arg(QCoreApplication::applicationName()) << endl;
            *m_logStreams[category] << QString("Version: %1").arg(QCoreApplication::applicationVersion()) << endl;
            *m_logStreams[category] << "===========================================" << endl;
            m_logStreams[category]->flush();
        }
    } else {
        delete file;
        qWarning() << "Failed to open log file:" << filePath;
    }
}

QString Lab4Logger::getCategoryName(LogCategory category) const
{
    switch (category) {
        case GENERAL: return "GENERAL";
        case CAMERA: return "CAMERA";
        case STEALTH_MODE: return "STEALTH_MODE";
        case STEALTH_DAEMON: return "STEALTH_DAEMON";
        case AUTOMATIC_MODE: return "AUTOMATIC_MODE";
        case JAKE_WARNING: return "JAKE_WARNING";
        case HOTKEYS: return "HOTKEYS";
        case UI_EVENTS: return "UI_EVENTS";
        case FILE_OPERATIONS: return "FILE_OPERATIONS";
        case SYSTEM_EVENTS: return "SYSTEM_EVENTS";
        default: return "UNKNOWN";
    }
}

QString Lab4Logger::getLevelName(LogLevel level) const
{
    switch (level) {
        case DEBUG_LEVEL: return "DEBUG";
        case INFO_LEVEL: return "INFO";
        case WARNING_LEVEL: return "WARNING";
        case ERROR_LEVEL: return "ERROR";
        default: return "UNKNOWN";
    }
}

QString Lab4Logger::formatMessage(LogCategory category, LogLevel level, const QString &message) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString categoryName = getCategoryName(category);
    QString levelName = getLevelName(level);
    
    return QString("[%1] [%2] [%3] %4")
           .arg(timestamp)
           .arg(categoryName)
           .arg(levelName)
           .arg(message);
}
