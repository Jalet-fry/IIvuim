#ifndef BLUETOOTHCONNECTION_H
#define BLUETOOTHCONNECTION_H

#include <QObject>
#include <Windows.h>
#include <winsock2.h>
#include <ws2bth.h>
#include <BluetoothAPIs.h>

class BluetoothLogger;

// Класс для настоящего RFCOMM подключения к Bluetooth устройству
class BluetoothConnection : public QObject
{
    Q_OBJECT
    
public:
    explicit BluetoothConnection(BluetoothLogger *logger, QObject *parent = nullptr);
    ~BluetoothConnection();
    
    // Подключение к устройству
    bool connectToDevice(const QString &deviceAddress, const QString &deviceName);
    
    // Отключение
    void disconnect();
    
    // Проверка состояния
    bool isConnected() const { return connected; }
    
    // Отправка данных
    qint64 sendData(const QByteArray &data);
    
    // Прием данных
    QByteArray receiveData(int maxSize = 4096);
    
signals:
    void connectionEstablished(const QString &deviceName);
    void connectionFailed(const QString &error);
    void disconnected();
    void dataReceived(const QByteArray &data);
    
private:
    BluetoothLogger *logger;
    SOCKET btSocket;
    bool connected;
    QString connectedDeviceName;
    QString connectedDeviceAddress;
    
    // Инициализация Winsock
    bool initializeWinsock();
    void cleanupWinsock();
    
    // Парсинг MAC адреса
    bool parseMacAddress(const QString &address, BLUETOOTH_ADDRESS &btAddr);
    
    // Форматирование ошибок
    QString getLastSocketError();
};

#endif // BLUETOOTHCONNECTION_H

