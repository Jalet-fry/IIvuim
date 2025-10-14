#ifndef WINDOWSBLUETOOTHMANAGER_H
#define WINDOWSBLUETOOTHMANAGER_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <Windows.h>
#include <BluetoothAPIs.h>

#pragma comment(lib, "Bthprops.lib")
#pragma comment(lib, "ws2_32.lib")

// Возможности Bluetooth устройства
struct BluetoothDeviceCapabilities {
    bool canReceiveFiles;      // Может принимать файлы
    bool canSendFiles;         // Может отправлять файлы
    bool supportsOBEX;         // Поддерживает OBEX FTP
    bool supportsRFCOMM;       // Поддерживает SPP (Serial Port Profile)
    bool isAudioDevice;        // Только аудио устройство
    bool isInputDevice;        // Устройство ввода (мышь, клавиатура)
    QString recommendedMethod; // Рекомендуемый метод отправки
    QString blockReason;       // Причина блокировки (если нельзя)
};

// Структура для хранения информации об устройстве
struct BluetoothDeviceData {
    QString name;
    QString address;
    quint32 deviceClass;
    bool isConnected;
    bool isPaired;
    bool isRemembered;
    SYSTEMTIME lastSeen;
    SYSTEMTIME lastUsed;
    
    // Вспомогательная функция для получения типа устройства
    QString getDeviceTypeString() const;
    
    // НОВОЕ: Определение возможностей устройства
    BluetoothDeviceCapabilities getCapabilities() const;
    
    // НОВОЕ: Можно ли отправлять файлы на это устройство
    bool canSendFilesTo() const;
};

// Регистрация типа для использования в Qt сигналах/слотах
Q_DECLARE_METATYPE(BluetoothDeviceData)

// Поток для сканирования устройств (чтобы не блокировать UI)
class BluetoothScanWorker : public QThread
{
    Q_OBJECT
    
public:
    explicit BluetoothScanWorker(QObject *parent = nullptr);
    ~BluetoothScanWorker();
    
    void stopScanning();
    
signals:
    void deviceFound(const BluetoothDeviceData &device);
    void scanCompleted(int deviceCount);
    void scanError(const QString &error);
    void logMessage(const QString &message);
    
protected:
    void run() override;
    
private:
    volatile bool shouldStop;
    QString formatBluetoothAddress(const BLUETOOTH_ADDRESS &btAddr) const;
    QString getDeviceClassName(DWORD classOfDevice) const;
};

// Основной менеджер Bluetooth
class WindowsBluetoothManager : public QObject
{
    Q_OBJECT
    
public:
    explicit WindowsBluetoothManager(QObject *parent = nullptr);
    ~WindowsBluetoothManager();
    
    // Управление сканированием
    void startDeviceDiscovery();
    void stopDeviceDiscovery();
    bool isScanning() const;
    
    // Информация о локальном адаптере
    QString getLocalDeviceName() const;
    QString getLocalDeviceAddress() const;
    bool isBluetoothAvailable() const;
    
    // Получение списка уже подключенных устройств
    QList<BluetoothDeviceData> getConnectedDevices() const;
    
signals:
    // Сигналы обнаружения устройств
    void deviceDiscovered(const BluetoothDeviceData &device);
    void discoveryFinished(int deviceCount);
    void discoveryError(const QString &errorString);
    
    // Логирование
    void logMessage(const QString &message);
    
private slots:
    void onDeviceFound(const BluetoothDeviceData &device);
    void onScanCompleted(int deviceCount);
    void onScanError(const QString &error);
    void onScanLogMessage(const QString &message);
    
private:
    BluetoothScanWorker *scanWorker;
    bool scanning;
    
    // Кэш информации о локальном адаптере
    mutable QString cachedLocalName;
    mutable QString cachedLocalAddress;
    mutable bool cachedAvailability;
    mutable bool cacheValid;
    
    void updateLocalDeviceCache() const;
    QString formatBluetoothAddress(const BLUETOOTH_ADDRESS &btAddr) const;
};

#endif // WINDOWSBLUETOOTHMANAGER_H

