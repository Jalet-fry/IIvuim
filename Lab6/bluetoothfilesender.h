#ifndef BLUETOOTHFILESENDER_H
#define BLUETOOTHFILESENDER_H

#include <QObject>
#include <QString>
#include <Windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlobj.h>

class BluetoothLogger;
class BluetoothConnection;

// Класс для отправки файлов через Bluetooth
class BluetoothFileSender : public QObject
{
    Q_OBJECT
    
public:
    explicit BluetoothFileSender(BluetoothLogger *logger, QObject *parent = nullptr);
    ~BluetoothFileSender();
    
    // Отправка файла на устройство
    bool sendFile(const QString &filePath, const QString &deviceAddress, const QString &deviceName, BluetoothConnection *connection = nullptr);
    
    // Проверка поддержки OBEX
    bool isObexSupported();
    
    // Отправка через fsquirt (для телефонов и других OBEX устройств)
    bool sendViaFsquirt(const QString &filePath);
    
signals:
    void transferStarted(const QString &fileName);
    void transferProgress(qint64 bytesSent, qint64 totalBytes);
    void transferCompleted(const QString &fileName);
    void transferFailed(const QString &error);
    
private:
    BluetoothLogger *logger;
    
    // Прямая отправка через RFCOMM
    bool sendFileDirectly(BluetoothConnection *connection, const QString &filePath);

    
    // Попытка отправки через Windows Shell
    bool sendViaWindowsShell(const QString &filePath, const QString &deviceAddress);
    
    // Форматирование MAC адреса
    QString formatMacAddress(const QString &address);
};

#endif // BLUETOOTHFILESENDER_H

