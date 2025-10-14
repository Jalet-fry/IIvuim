#ifndef BLUETOOTHLOGGER_H
#define BLUETOOTHLOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QMutex>

// Класс для логирования в файл и UI одновременно
class BluetoothLogger : public QObject
{
    Q_OBJECT
    
public:
    enum LogLevel {
        Debug,      // Отладочная информация
        Info,       // Обычная информация
        Warning,    // Предупреждения
        Error,      // Ошибки
        Success     // Успешные операции
    };
    
    explicit BluetoothLogger(QObject *parent = nullptr);
    ~BluetoothLogger();
    
    // Основной метод логирования
    void log(LogLevel level, const QString &category, const QString &message);
    
    // Удобные методы
    void debug(const QString &category, const QString &message);
    void info(const QString &category, const QString &message);
    void warning(const QString &category, const QString &message);
    void error(const QString &category, const QString &message);
    void success(const QString &category, const QString &message);
    
    // Логирование API вызовов
    void logApiCall(const QString &apiName, const QString &params);
    void logApiResult(const QString &apiName, const QString &result, bool success);
    
    // Логирование данных устройства
    void logDeviceInfo(const QString &name, const QString &address, const QString &info);
    
    // Получить путь к файлу лога
    QString getLogFilePath() const { return logFilePath; }
    
signals:
    // Сигнал для UI (с цветом)
    void logToUI(const QString &message, const QString &color);
    
private:
    // Главный файл лога
    QFile *mainLogFile;
    QTextStream *mainLogStream;
    
    // Отдельные файлы для категорий
    QFile *scanLogFile;
    QTextStream *scanLogStream;
    
    QFile *connectLogFile;
    QTextStream *connectLogStream;
    
    QFile *sendLogFile;
    QTextStream *sendLogStream;
    
    QFile *apiLogFile;
    QTextStream *apiLogStream;
    
    QString logFilePath;
    QString sessionPath;
    QString sessionId;
    QMutex mutex;
    
    void initializeLogFile();
    void initializeCategoryLogs();
    QString getLevelString(LogLevel level) const;
    QString getLevelColor(LogLevel level) const;
    QString getCurrentTimestamp() const;
    void writeToFile(const QString &message);
    void writeToCategory(const QString &category, const QString &message);
};

#endif // BLUETOOTHLOGGER_H

