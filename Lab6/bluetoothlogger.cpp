#include "bluetoothlogger.h"
#include <QStandardPaths>
#include <QDebug>

BluetoothLogger::BluetoothLogger(QObject *parent)
    : QObject(parent)
    , mainLogFile(nullptr)
    , mainLogStream(nullptr)
    , scanLogFile(nullptr)
    , scanLogStream(nullptr)
    , connectLogFile(nullptr)
    , connectLogStream(nullptr)
    , sendLogFile(nullptr)
    , sendLogStream(nullptr)
    , apiLogFile(nullptr)
    , apiLogStream(nullptr)
{
    initializeLogFile();
    initializeCategoryLogs();
}

BluetoothLogger::~BluetoothLogger()
{
    // Закрываем все файлы
    auto closeLog = [](QTextStream *&stream, QFile *&file) {
        if (stream) {
            stream->flush();
            delete stream;
            stream = nullptr;
        }
        if (file) {
            file->close();
            delete file;
            file = nullptr;
        }
    };
    
    closeLog(mainLogStream, mainLogFile);
    closeLog(scanLogStream, scanLogFile);
    closeLog(connectLogStream, connectLogFile);
    closeLog(sendLogStream, sendLogFile);
    closeLog(apiLogStream, apiLogFile);
}

void BluetoothLogger::initializeLogFile()
{
    // Создаем уникальный ID сессии
    sessionId = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    
    // Путь к папке логов
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString logsBasePath = documentsPath + "/Lab6_BluetoothLogs";
    
    // Создаем папку для логов, если её нет
    QDir logsDir(logsBasePath);
    if (!logsDir.exists()) {
        logsDir.mkpath(".");
    }
    
    // Создаем папку для этой сессии
    sessionPath = logsBasePath + "/Session_" + sessionId;
    QDir sessionDir(sessionPath);
    if (!sessionDir.exists()) {
        sessionDir.mkpath(".");
    }
    
    // Путь к главному файлу лога
    logFilePath = sessionPath + "/main.log";
    
    // Открываем главный файл
    mainLogFile = new QFile(logFilePath);
    if (mainLogFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        mainLogStream = new QTextStream(mainLogFile);
        mainLogStream->setCodec("UTF-8");
        
        // Заголовок лога
        *mainLogStream << "╔═══════════════════════════════════════════════════════════════╗\n";
        *mainLogStream << "║          BLUETOOTH LAB6 - MAIN LOG                            ║\n";
        *mainLogStream << "╚═══════════════════════════════════════════════════════════════╝\n";
        *mainLogStream << "\n";
        *mainLogStream << "Session ID: " << sessionId << "\n";
        *mainLogStream << "Started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
        *mainLogStream << "Session Path: " << sessionPath << "\n";
        *mainLogStream << "\n";
        *mainLogStream << "LOG FILES:\n";
        *mainLogStream << "  • main.log       - Все логи (этот файл)\n";
        *mainLogStream << "  • scan.log       - Сканирование устройств\n";
        *mainLogStream << "  • connect.log    - Подключения к устройствам\n";
        *mainLogStream << "  • send.log       - Отправка файлов\n";
        *mainLogStream << "  • api_calls.log  - Все Windows API вызовы\n";
        *mainLogStream << "\n";
        *mainLogStream << "═══════════════════════════════════════════════════════════════\n\n";
        mainLogStream->flush();
        
        // Лог в UI
        emit logToUI("═══════════════════════════════════════", "blue");
        emit logToUI("СИСТЕМА ЛОГИРОВАНИЯ ИНИЦИАЛИЗИРОВАНА", "green");
        emit logToUI("═══════════════════════════════════════", "blue");
        emit logToUI("Session ID: " + sessionId, "gray");
        emit logToUI("Папка логов: " + sessionPath, "gray");
        emit logToUI("Создано 5 файлов логов:", "gray");
        emit logToUI("  • main.log (все)", "gray");
        emit logToUI("  • scan.log (сканирование)", "gray");
        emit logToUI("  • connect.log (подключения)", "gray");
        emit logToUI("  • send.log (отправка)", "gray");
        emit logToUI("  • api_calls.log (API вызовы)", "gray");
        emit logToUI("", "black");
    } else {
        qWarning() << "Не удалось создать файл лога:" << logFilePath;
    }
}

void BluetoothLogger::initializeCategoryLogs()
{
    auto createCategoryLog = [this](QFile *&file, QTextStream *&stream, const QString &filename, const QString &title) {
        QString path = sessionPath + "/" + filename;
        file = new QFile(path);
        if (file->open(QIODevice::WriteOnly | QIODevice::Text)) {
            stream = new QTextStream(file);
            stream->setCodec("UTF-8");
            
            *stream << "╔═══════════════════════════════════════════════════════════════╗\n";
            *stream << QString("║  %1").arg(title).leftJustified(62) << " ║\n";
            *stream << "╚═══════════════════════════════════════════════════════════════╝\n";
            *stream << "\n";
            *stream << "Session ID: " << sessionId << "\n";
            *stream << "Started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
            *stream << "\n";
            *stream << "═══════════════════════════════════════════════════════════════\n\n";
            stream->flush();
        }
    };
    
    createCategoryLog(scanLogFile, scanLogStream, "scan.log", "СКАНИРОВАНИЕ УСТРОЙСТВ");
    createCategoryLog(connectLogFile, connectLogStream, "connect.log", "ПОДКЛЮЧЕНИЯ К УСТРОЙСТВАМ");
    createCategoryLog(sendLogFile, sendLogStream, "send.log", "ОТПРАВКА ФАЙЛОВ");
    createCategoryLog(apiLogFile, apiLogStream, "api_calls.log", "WINDOWS API ВЫЗОВЫ");
}

void BluetoothLogger::log(LogLevel level, const QString &category, const QString &message)
{
    QMutexLocker locker(&mutex);
    
    QString timestamp = getCurrentTimestamp();
    QString levelStr = getLevelString(level);
    QString color = getLevelColor(level);
    
    // Форматирование для файла
    QString fileMessage = QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(levelStr.rightJustified(7))
        .arg(category.leftJustified(15))
        .arg(message);
    
    // Форматирование для UI
    QString uiMessage = QString("[%1] [%2] %3")
        .arg(levelStr)
        .arg(category)
        .arg(message);
    
    // Записываем в главный файл
    writeToFile(fileMessage);
    
    // Записываем в файл категории
    writeToCategory(category, fileMessage);
    
    // Отправляем в UI
    emit logToUI(uiMessage, color);
}

void BluetoothLogger::debug(const QString &category, const QString &message)
{
    log(Debug, category, message);
}

void BluetoothLogger::info(const QString &category, const QString &message)
{
    log(Info, category, message);
}

void BluetoothLogger::warning(const QString &category, const QString &message)
{
    log(Warning, category, message);
}

void BluetoothLogger::error(const QString &category, const QString &message)
{
    log(Error, category, message);
}

void BluetoothLogger::success(const QString &category, const QString &message)
{
    log(Success, category, message);
}

void BluetoothLogger::logApiCall(const QString &apiName, const QString &params)
{
    QString message = QString("API CALL: %1(%2)").arg(apiName).arg(params);
    log(Debug, "WinAPI", message);
}

void BluetoothLogger::logApiResult(const QString &apiName, const QString &result, bool success)
{
    LogLevel level = success ? Success : Error;
    QString message = QString("API RESULT: %1 → %2").arg(apiName).arg(result);
    log(level, "WinAPI", message);
}

void BluetoothLogger::logDeviceInfo(const QString &name, const QString &address, const QString &info)
{
    log(Info, "Device", QString("%1 [%2]: %3").arg(name).arg(address).arg(info));
}

QString BluetoothLogger::getLevelString(LogLevel level) const
{
    switch (level) {
    case Debug:   return "DEBUG";
    case Info:    return "INFO";
    case Warning: return "WARNING";
    case Error:   return "ERROR";
    case Success: return "SUCCESS";
    default:      return "UNKNOWN";
    }
}

QString BluetoothLogger::getLevelColor(LogLevel level) const
{
    switch (level) {
    case Debug:   return "gray";
    case Info:    return "black";
    case Warning: return "orange";
    case Error:   return "red";
    case Success: return "green";
    default:      return "black";
    }
}

QString BluetoothLogger::getCurrentTimestamp() const
{
    return QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
}

void BluetoothLogger::writeToFile(const QString &message)
{
    if (mainLogStream) {
        *mainLogStream << message << "\n";
        mainLogStream->flush();
    }
}

void BluetoothLogger::writeToCategory(const QString &category, const QString &message)
{
    QTextStream *categoryStream = nullptr;
    
    if (category == "Scan" || category.contains("Scan", Qt::CaseInsensitive)) {
        categoryStream = scanLogStream;
    } else if (category == "Connect" || category == "Connection") {
        categoryStream = connectLogStream;
    } else if (category == "Send" || category == "FileSender" || category == "Transfer") {
        categoryStream = sendLogStream;
    } else if (category == "WinAPI") {
        categoryStream = apiLogStream;
    }
    
    if (categoryStream) {
        *categoryStream << message << "\n";
        categoryStream->flush();
    }
}

