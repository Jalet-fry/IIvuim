#ifndef STORAGESCANNER_H
#define STORAGESCANNER_H

#include <QString>
#include <QStringList>
#include <vector>
#include <cstdint>
#include <map>
#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>

struct StorageDevice {
    QString model;
    QString manufacturer;
    QString serialNumber;
    QString firmwareVersion;
    QString interfaceType;
    QString driveType;
    uint64_t totalSize;
    uint64_t freeSpace;
    uint64_t usedSpace;
    bool isSystemDrive;
    QStringList supportedModes;
};

// Структура для хранения информации о BusType
struct BusTypeInfo {
    uint16_t code;
    QString name;
    QString icon;
};

class StorageScanner {
public:
    StorageScanner();
    ~StorageScanner();

    bool initialize();
    std::vector<StorageDevice> scanStorageDevices();
    void cleanup();

private:
    IWbemLocator *m_pLoc;
    IWbemServices *m_pSvc;
    IWbemServices *m_pStorageSvc;  // Подключение к Storage namespace
    bool m_initialized;
    bool m_comInitialized;

    // Константы и таблицы
    static const std::map<uint16_t, BusTypeInfo> BUS_TYPES;
    static const std::map<QString, QString> MANUFACTURER_PATTERNS;
    static const std::map<QString, QString> DRIVE_TYPE_PATTERNS;

    // Основные функции
    QString variantToString(const VARIANT& var);
    uint64_t variantToUInt64(const VARIANT& var);
    
    // Упрощённые функции определения типов
    QString determineInterfaceType(int physicalDriveNumber, const QString& model, const QString& fallbackInterface);
    QString determineDriveType(const QString& model, const QString& mediaType, const QString& interfaceType);
    QString determineManufacturer(const QString& model, const QString& fallbackManufacturer);
    
    // Получение информации о диске
    void getDiskSpaceInfo(const QString& deviceID, uint64_t& totalSize, uint64_t& freeSpace, uint64_t& usedSpace);
    int getPhysicalDriveNumber(const QString& deviceID);
    uint64_t getDriveSpaceByPhysicalNumber(int driveNumber);
    
    // WMI запросы
    std::map<int, uint16_t> getAllBusTypes();  // Получает BusType для всех дисков одним запросом
    QString busTypeCodeToString(uint16_t busType);
    
    // Валидация и форматирование
    QString formatSerialNumber(const QString& serial);
    bool validateDeviceData(const StorageDevice& device);
};

#endif // STORAGESCANNER_H

