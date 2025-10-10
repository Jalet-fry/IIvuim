#ifndef STORAGESCANNER_H
#define STORAGESCANNER_H

#include <QString>
#include <QStringList>
#include <vector>
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
    bool m_initialized;
    bool m_comInitialized;

    QString variantToString(const VARIANT& var);
    uint64_t variantToUInt64(const VARIANT& var);
    QString determineInterfaceType(const QString& interfaceType);
    QString determineDriveType(const QString& mediaType, const QString& model);
    void getDiskSpaceInfo(const QString& deviceID, uint64_t& totalSize, uint64_t& freeSpace, uint64_t& usedSpace);
    
    // Системные вызовы Windows API для получения информации о дисках
    int getPhysicalDriveNumber(const QString& deviceID);
    uint64_t getDriveSpaceByPhysicalNumber(int driveNumber);
    
    // Получение точного BusType через Storage namespace (NVMe/SATA/USB/etc)
    QString getAccurateBusType(int physicalDriveNumber);
    QString busTypeCodeToString(uint16_t busType);
};

#endif // STORAGESCANNER_H

