#include "storagescanner.h"
#include <QDebug>
#include <windows.h>
#include <fileapi.h>
#include <initguid.h>
#include <winioctl.h>
#include <ntddscsi.h>

// Define CLSID_WbemLocator
DEFINE_GUID(CLSID_WbemLocator, 0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);

// Статические константы
const std::map<uint16_t, BusTypeInfo> StorageScanner::BUS_TYPES = {
    {0, {0, "Unknown", "❓"}},
    {1, {1, "SCSI", "🔌"}},
    {2, {2, "ATAPI", "🔌"}},
    {3, {3, "ATA", "🔌"}},
    {4, {4, "IEEE 1394", "🔥"}},
    {5, {5, "SSA", "🔌"}},
    {6, {6, "Fibre Channel", "🔌"}},
    {7, {7, "USB", "🔌"}},
    {8, {8, "RAID", "🔌"}},
    {9, {9, "iSCSI", "🌐"}},
    {10, {10, "SAS", "🔌"}},
    {11, {11, "SATA", "💾"}},
    {12, {12, "SD", "💳"}},
    {13, {13, "MMC", "💳"}},
    {14, {14, "Virtual", "💻"}},
    {15, {15, "File Backed Virtual", "💻"}},
    {16, {16, "Storage Spaces", "🗂️"}},
    {17, {17, "NVMe", "🚀"}},
    {18, {18, "Microsoft Reserved", "🔒"}}
};

const std::map<QString, QString> StorageScanner::MANUFACTURER_PATTERNS = {
    {"micron", "Micron"},
    {"samsung", "Samsung"},
    {"wd_", "Western Digital"}, {"wd ", "Western Digital"}, {"western digital", "Western Digital"},
    {"transcend", "Transcend"},
    {"kingston", "Kingston"},
    {"seagate", "Seagate"},
    {"crucial", "Crucial"},
    {"intel", "Intel"},
    {"sandisk", "SanDisk"},
    {"toshiba", "Toshiba"},
    {"corsair", "Corsair"},
    {"adata", "ADATA"},
    {"hitachi", "HGST/Hitachi"}, {"hgst", "HGST/Hitachi"}
};

const std::map<QString, QString> StorageScanner::DRIVE_TYPE_PATTERNS = {
    // NVMe patterns
    {"nvme", "SSD (NVMe)"},
    {"micron_", "SSD (NVMe)"},
    {"samsung 970", "SSD (NVMe)"}, {"samsung 980", "SSD (NVMe)"}, {"samsung 990", "SSD (NVMe)"},
    {"wd black sn", "SSD (NVMe)"}, {"wd_black sn", "SSD (NVMe)"},
    {"kingston nv", "SSD (NVMe)"},
    {"crucial p", "SSD (NVMe)"},
    {"intel 660p", "SSD (NVMe)"},
    {"seagate firecuda", "SSD (NVMe)"},
    
    // SSD patterns
    {"ssd", "SSD"},
    {"solid state", "SSD"},
    {"samsung 8", "SSD"},
    {"crucial mx", "SSD"},
    {"kingston sa", "SSD"},
    {"adata su", "SSD"},
    {"sandisk ultra", "SSD"}
};

StorageScanner::StorageScanner() 
    : m_pLoc(nullptr), m_pSvc(nullptr), m_pStorageSvc(nullptr), 
      m_initialized(false), m_comInitialized(false) {
}

StorageScanner::~StorageScanner() {
    cleanup();
}

bool StorageScanner::initialize() {
    HRESULT hres;

    // Initialize COM
    hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hres)) {
        if (hres == RPC_E_CHANGED_MODE || hres == S_FALSE) {
            qDebug() << "COM already initialized, continuing...";
            m_comInitialized = false;
        } else {
            qDebug() << "Failed to initialize COM library. Error code =" << hres;
            return false;
        }
    } else {
        m_comInitialized = true;
    }

    // Set general COM security levels
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        qDebug() << "Failed to initialize security. Error code =" << hres;
        if (m_comInitialized) {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    // Obtain the initial locator to WMI
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator, (LPVOID*)&m_pLoc);

    if (FAILED(hres)) {
        qDebug() << "Failed to create IWbemLocator object. Error code =" << hres;
        if (m_comInitialized) {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    // Connect to CIMV2 namespace
    BSTR strNetworkResource = SysAllocString(L"ROOT\\CIMV2");
    hres = m_pLoc->ConnectServer(strNetworkResource, NULL, NULL, 0, 0L, 0, 0, &m_pSvc);
    SysFreeString(strNetworkResource);

    if (FAILED(hres)) {
        qDebug() << "Could not connect to CIMV2. Error code =" << hres;
        cleanup();
        return false;
    }

    // Connect to Storage namespace (один раз!)
    BSTR strStorageNamespace = SysAllocString(L"ROOT\\Microsoft\\Windows\\Storage");
    hres = m_pLoc->ConnectServer(strStorageNamespace, NULL, NULL, 0, 0L, 0, 0, &m_pStorageSvc);
    SysFreeString(strStorageNamespace);

    if (FAILED(hres)) {
        qDebug() << "Could not connect to Storage namespace. Error code =" << hres;
        cleanup();
        return false;
    }

    // Set security levels on both proxies
    CoSetProxyBlanket(m_pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                     RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    
    CoSetProxyBlanket(m_pStorageSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                     RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    m_initialized = true;
    qDebug() << "✅ StorageScanner initialized successfully";
    return true;
}

std::vector<StorageDevice> StorageScanner::scanStorageDevices() {
    std::vector<StorageDevice> devices;

    if (!m_initialized) {
        qDebug() << "❌ StorageScanner not initialized!";
        return devices;
    }

    qDebug() << "🔍 Scanning storage devices...";

    // Получаем BusType для всех дисков одним запросом
    std::map<int, uint16_t> busTypes = getAllBusTypes();

    // Query for physical disk drives
    IEnumWbemClassObject* pEnumerator = nullptr;
    BSTR strQueryLanguage = SysAllocString(L"WQL");
    BSTR strQuery = SysAllocString(L"SELECT * FROM Win32_DiskDrive");
    HRESULT hres = m_pSvc->ExecQuery(strQueryLanguage, strQuery,
                                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                    NULL, &pEnumerator);
    SysFreeString(strQuery);
    SysFreeString(strQueryLanguage);

    if (FAILED(hres)) {
        qDebug() << "❌ Query for Win32_DiskDrive failed. Error code =" << hres;
        return devices;
    }

    IWbemClassObject *pclsObj = nullptr;
    ULONG uReturn = 0;

    while (pEnumerator) {
        pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) break;

        StorageDevice device;
        VARIANT vtProp;

        // Get basic information
        if (SUCCEEDED(pclsObj->Get(L"Model", 0, &vtProp, 0, 0))) {
            device.model = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        if (SUCCEEDED(pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0))) {
            QString fallbackManufacturer = variantToString(vtProp);
            device.manufacturer = determineManufacturer(device.model, fallbackManufacturer);
            VariantClear(&vtProp);
        }

        if (SUCCEEDED(pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0))) {
            device.serialNumber = formatSerialNumber(variantToString(vtProp));
            VariantClear(&vtProp);
        }

        if (SUCCEEDED(pclsObj->Get(L"FirmwareRevision", 0, &vtProp, 0, 0))) {
            device.firmwareVersion = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        if (SUCCEEDED(pclsObj->Get(L"Size", 0, &vtProp, 0, 0))) {
            device.totalSize = variantToUInt64(vtProp);
            VariantClear(&vtProp);
        }

        // Get Device ID and extract physical drive number
        QString deviceID;
        if (SUCCEEDED(pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0))) {
            deviceID = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        int physicalDriveNumber = getPhysicalDriveNumber(deviceID);
        
        // Get Media Type for drive type determination
        QString mediaType;
        if (SUCCEEDED(pclsObj->Get(L"MediaType", 0, &vtProp, 0, 0))) {
            mediaType = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        // Get fallback interface type
        QString fallbackInterface;
        if (SUCCEEDED(pclsObj->Get(L"InterfaceType", 0, &vtProp, 0, 0))) {
            fallbackInterface = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        // Упрощённое определение типа интерфейса
        device.interfaceType = determineInterfaceType(physicalDriveNumber, device.model, fallbackInterface);
        
        // Упрощённое определение типа накопителя
        device.driveType = determineDriveType(device.model, mediaType, device.interfaceType);

        // Get disk space information
        getDiskSpaceInfo(deviceID, device.totalSize, device.freeSpace, device.usedSpace);

        // Get capabilities
        if (SUCCEEDED(pclsObj->Get(L"CapabilityDescriptions", 0, &vtProp, 0, 0)) && 
            vtProp.vt == (VT_ARRAY | VT_BSTR)) {
            SAFEARRAY* psa = vtProp.parray;
            LONG lLower, lUpper;
            SafeArrayGetLBound(psa, 1, &lLower);
            SafeArrayGetUBound(psa, 1, &lUpper);

            for (LONG i = lLower; i <= lUpper; i++) {
                BSTR bstr;
                SafeArrayGetElement(psa, &i, &bstr);
                device.supportedModes.append(QString::fromWCharArray(bstr));
                SysFreeString(bstr);
            }
            VariantClear(&vtProp);
        }

        // Check if system drive
        device.isSystemDrive = deviceID.contains("PHYSICALDRIVE0", Qt::CaseInsensitive);

        // Validate device data
        if (validateDeviceData(device)) {
            devices.push_back(device);
            
            qDebug() << QString("✅ Found: %1 (%2) - %3 %4")
                        .arg(device.model)
                        .arg(device.manufacturer)
                        .arg(device.interfaceType)
                        .arg(device.driveType);
        } else {
            qDebug() << QString("⚠️ Skipped invalid device: %1").arg(device.model);
        }

        pclsObj->Release();
    }

    pEnumerator->Release();
    
    qDebug() << QString("🎯 Scan completed: %1 devices found").arg(devices.size());
    return devices;
}

std::map<int, uint16_t> StorageScanner::getAllBusTypes() {
    std::map<int, uint16_t> busTypes;
    
    if (!m_pStorageSvc) return busTypes;

    IEnumWbemClassObject* pEnumerator = nullptr;
    BSTR strQueryLanguage = SysAllocString(L"WQL");
    BSTR strQuery = SysAllocString(L"SELECT DeviceId, BusType FROM MSFT_PhysicalDisk");
    
    HRESULT hres = m_pStorageSvc->ExecQuery(strQueryLanguage, strQuery,
                                           WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                           NULL, &pEnumerator);
    
    SysFreeString(strQuery);
    SysFreeString(strQueryLanguage);

    if (SUCCEEDED(hres)) {
        IWbemClassObject *pclsObj = nullptr;
        ULONG uReturn = 0;
        
        while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn > 0) {
            VARIANT vtDeviceId, vtBusType;
            
            if (SUCCEEDED(pclsObj->Get(L"DeviceId", 0, &vtDeviceId, 0, 0)) &&
                SUCCEEDED(pclsObj->Get(L"BusType", 0, &vtBusType, 0, 0))) {
                
                int deviceId = 0;
                uint16_t busType = 0;
                
                if (vtDeviceId.vt == VT_I4) deviceId = vtDeviceId.lVal;
                if (vtBusType.vt == VT_I4) busType = static_cast<uint16_t>(vtBusType.lVal);
                
                busTypes[deviceId] = busType;
                
                VariantClear(&vtDeviceId);
                VariantClear(&vtBusType);
            }
            
            pclsObj->Release();
        }
        
        pEnumerator->Release();
    }
    
    return busTypes;
}

QString StorageScanner::determineInterfaceType(int physicalDriveNumber, const QString& model, const QString& fallbackInterface) {
    // Сначала пытаемся получить точный BusType
    std::map<int, uint16_t> busTypes = getAllBusTypes();
    
    if (busTypes.find(physicalDriveNumber) != busTypes.end()) {
        uint16_t busTypeCode = busTypes[physicalDriveNumber];
        auto it = BUS_TYPES.find(busTypeCode);
        if (it != BUS_TYPES.end()) {
            qDebug() << QString("🎯 Exact BusType for drive %1: %2").arg(physicalDriveNumber).arg(it->second.name);
            return it->second.name;
        }
    }
    
    // Fallback: анализ модели
    QString modelLower = model.toLower();
    QString modelUpper = model.toUpper();
    
    // NVMe detection from model
    if (modelLower.contains("nvme") || 
        modelUpper.contains("MICRON_") ||
        modelLower.contains("samsung 970") || modelLower.contains("samsung 980") ||
        modelLower.contains("wd black sn") || modelLower.contains("wd_black sn")) {
        return "NVMe";
    }
    
    // USB detection
    if (fallbackInterface.contains("USB", Qt::CaseInsensitive)) {
        return "USB";
    }
    
    // SATA detection
    if (fallbackInterface.contains("SATA", Qt::CaseInsensitive)) {
        return "SATA";
    }
    
    // Default fallback
    return fallbackInterface.isEmpty() ? "Unknown" : fallbackInterface;
}

QString StorageScanner::determineDriveType(const QString& model, const QString& mediaType, const QString& interfaceType) {
    QString modelLower = model.toLower();
    
    // Определяем по интерфейсу
    if (interfaceType == "NVMe") {
        return "SSD (NVMe)";
    }
    
    // Определяем по паттернам в модели
    for (const auto& pattern : DRIVE_TYPE_PATTERNS) {
        if (modelLower.contains(pattern.first, Qt::CaseInsensitive)) {
            return pattern.second;
        }
    }
    
    // Определяем по MediaType
    if (mediaType.contains("SSD", Qt::CaseInsensitive) || 
        mediaType.contains("Solid State", Qt::CaseInsensitive)) {
        return "SSD";
    }
    
    if (mediaType.contains("Fixed", Qt::CaseInsensitive) ||
        mediaType.contains("hard disk", Qt::CaseInsensitive)) {
        return "HDD";
    }
    
    return "Unknown";
}

QString StorageScanner::determineManufacturer(const QString& model, const QString& fallbackManufacturer) {
    // Если производитель уже определён и не является общим
    if (!fallbackManufacturer.isEmpty() && 
        !fallbackManufacturer.contains("Standard", Qt::CaseInsensitive) &&
        !fallbackManufacturer.contains("Стандартные", Qt::CaseInsensitive) &&
        fallbackManufacturer != "(Standard disk drives)" &&
        fallbackManufacturer != "N/A") {
        return fallbackManufacturer;
    }
    
    // Определяем по паттернам в модели
    QString modelLower = model.toLower();
    for (const auto& pattern : MANUFACTURER_PATTERNS) {
        if (modelLower.contains(pattern.first, Qt::CaseInsensitive)) {
            return pattern.second;
        }
    }
    
    return fallbackManufacturer.isEmpty() ? "Unknown" : fallbackManufacturer;
}

QString StorageScanner::formatSerialNumber(const QString& serial) {
    QString formatted = serial.trimmed().simplified();
    
    if (formatted.length() > 50) {
        formatted = formatted.replace(" ", "_");
    }
    
    if (!formatted.isEmpty() && !formatted.endsWith(".")) {
        formatted += ".";
    }
    
    return formatted;
}

bool StorageScanner::validateDeviceData(const StorageDevice& device) {
    // Проверяем минимально необходимые данные
    if (device.model.isEmpty() || device.model == "N/A") {
        return false;
    }
    
    if (device.totalSize == 0) {
        return false;
    }
    
    return true;
}

QString StorageScanner::variantToString(const VARIANT& var) {
    if (var.vt == VT_BSTR) {
        return QString::fromWCharArray(var.bstrVal);
    } else if (var.vt == VT_NULL || var.vt == VT_EMPTY) {
        return "N/A";
    }
    return "Unknown";
}

uint64_t StorageScanner::variantToUInt64(const VARIANT& var) {
    if (var.vt == VT_BSTR) {
        QString str = QString::fromWCharArray(var.bstrVal);
        return str.toULongLong();
    } else if (var.vt == VT_UI8) {
        return var.ullVal;
    } else if (var.vt == VT_I8) {
        return static_cast<uint64_t>(var.llVal);
    } else if (var.vt == VT_UI4) {
        return var.ulVal;
    } else if (var.vt == VT_I4) {
        return static_cast<uint64_t>(var.lVal);
    }
    return 0;
}

void StorageScanner::getDiskSpaceInfo(const QString& deviceID, uint64_t& totalSize, uint64_t& freeSpace, uint64_t& usedSpace) {
    int physicalDriveNumber = getPhysicalDriveNumber(deviceID);
    
    if (physicalDriveNumber < 0) {
        freeSpace = 0;
        usedSpace = totalSize;
        return;
    }
    
    uint64_t totalFree = getDriveSpaceByPhysicalNumber(physicalDriveNumber);
    freeSpace = totalFree;
    usedSpace = totalSize > freeSpace ? totalSize - freeSpace : 0;
}

int StorageScanner::getPhysicalDriveNumber(const QString& deviceID) {
    QString deviceIDUpper = deviceID.toUpper();
    if (deviceIDUpper.contains("PHYSICALDRIVE")) {
        int idx = deviceIDUpper.indexOf("PHYSICALDRIVE") + 13;
        QString numStr = deviceIDUpper.mid(idx);
        bool ok;
        int num = numStr.toInt(&ok);
        if (ok) return num;
    }
    return -1;
}

uint64_t StorageScanner::getDriveSpaceByPhysicalNumber(int driveNumber) {
    uint64_t totalFree = 0;
    
    DWORD driveMask = GetLogicalDrives();
    
    for (int i = 0; i < 26; i++) {
        if (!(driveMask & (1 << i))) continue;
        
        wchar_t drivePath[8];
        swprintf(drivePath, 8, L"\\\\.\\%c:", L'A' + i);
        
        HANDLE hDevice = CreateFileW(drivePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, 0, NULL);
        
        if (hDevice == INVALID_HANDLE_VALUE) continue;
        
        STORAGE_DEVICE_NUMBER deviceNumber;
        DWORD bytesReturned;
        
        if (DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                          &deviceNumber, sizeof(deviceNumber), &bytesReturned, NULL)) {
            
            if ((int)deviceNumber.DeviceNumber == driveNumber) {
                CloseHandle(hDevice);
                
                wchar_t rootPath[4];
                swprintf(rootPath, 4, L"%c:\\", L'A' + i);
                
                ULARGE_INTEGER freeBytesAvailable;
                ULARGE_INTEGER totalNumberOfBytes;
                ULARGE_INTEGER totalNumberOfFreeBytes;
                
                if (GetDiskFreeSpaceExW(rootPath, &freeBytesAvailable,
                                      &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                    totalFree += totalNumberOfFreeBytes.QuadPart;
                }
            }
        }
        
        CloseHandle(hDevice);
    }
    
    return totalFree;
}

QString StorageScanner::busTypeCodeToString(uint16_t busType) {
    auto it = BUS_TYPES.find(busType);
    if (it != BUS_TYPES.end()) {
        return it->second.name;
    }
    return QString("Unknown (%1)").arg(busType);
}

void StorageScanner::cleanup() {
    if (m_pStorageSvc) {
        m_pStorageSvc->Release();
        m_pStorageSvc = nullptr;
    }
    if (m_pSvc) {
        m_pSvc->Release();
        m_pSvc = nullptr;
    }
    if (m_pLoc) {
        m_pLoc->Release();
        m_pLoc = nullptr;
    }
    if (m_comInitialized) {
        CoUninitialize();
        m_comInitialized = false;
    }
    m_initialized = false;
}