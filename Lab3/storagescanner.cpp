#include "storagescanner.h"
#include <QDebug>
#include <windows.h>
#include <fileapi.h>
#include <initguid.h>
#include <winioctl.h>
#include <ntddscsi.h>

// Define CLSID_WbemLocator
DEFINE_GUID(CLSID_WbemLocator, 0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);

StorageScanner::StorageScanner() 
    : m_pLoc(nullptr), m_pSvc(nullptr), m_initialized(false), m_comInitialized(false) {
}

StorageScanner::~StorageScanner() {
    cleanup();
}

bool StorageScanner::initialize() {
    HRESULT hres;

    // Initialize COM
    hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hres)) {
        // If COM is already initialized, that's OK
        if (hres == RPC_E_CHANGED_MODE || hres == S_FALSE) {
            qDebug() << "COM already initialized, continuing...";
            m_comInitialized = false; // Don't uninitialize on cleanup
        } else {
            qDebug() << "Failed to initialize COM library. Error code =" << hres;
            return false;
        }
    } else {
        m_comInitialized = true;
    }

    // Set general COM security levels
    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        qDebug() << "Failed to initialize security. Error code =" << hres;
        if (m_comInitialized) {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    // Obtain the initial locator to WMI
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&m_pLoc
    );

    if (FAILED(hres)) {
        qDebug() << "Failed to create IWbemLocator object. Error code =" << hres;
        if (m_comInitialized) {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    // Connect to WMI
    BSTR strNetworkResource = SysAllocString(L"ROOT\\CIMV2");
    hres = m_pLoc->ConnectServer(
        strNetworkResource,
        NULL,
        NULL,
        0,
        0L,
        0,
        0,
        &m_pSvc
    );
    SysFreeString(strNetworkResource);

    if (FAILED(hres)) {
        qDebug() << "Could not connect to WMI. Error code =" << hres;
        m_pLoc->Release();
        m_pLoc = nullptr;
        if (m_comInitialized) {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    // Set security levels on the proxy
    hres = CoSetProxyBlanket(
        m_pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        qDebug() << "Could not set proxy blanket. Error code =" << hres;
        m_pSvc->Release();
        m_pSvc = nullptr;
        m_pLoc->Release();
        m_pLoc = nullptr;
        if (m_comInitialized) {
            CoUninitialize();
            m_comInitialized = false;
        }
        return false;
    }

    m_initialized = true;
    return true;
}

std::vector<StorageDevice> StorageScanner::scanStorageDevices() {
    std::vector<StorageDevice> devices;

    if (!m_initialized) {
        qDebug() << "StorageScanner not initialized!";
        return devices;
    }

    qDebug() << "╔════════════════════════════════════════════════════════════╗";
    qDebug() << "║  СКАНИРОВАНИЕ НАКОПИТЕЛЕЙ - СИСТЕМНЫЕ ВЫЗОВЫ WINDOWS API  ║";
    qDebug() << "╠════════════════════════════════════════════════════════════╣";
    qDebug() << "║  1. WMI (Win32_DiskDrive) - информация о накопителях      ║";
    qDebug() << "║  2. DeviceIoControl + IOCTL_STORAGE_GET_DEVICE_NUMBER     ║";
    qDebug() << "║  3. GetDiskFreeSpaceExW - получение свободного места      ║";
    qDebug() << "║  4. GetLogicalDrives - список логических дисков           ║";
    qDebug() << "║  5. CreateFileW - открытие дескриптора устройства         ║";
    qDebug() << "╚════════════════════════════════════════════════════════════╝";

    // Query for physical disk drives
    IEnumWbemClassObject* pEnumerator = nullptr;
    BSTR strQueryLanguage = SysAllocString(L"WQL");
    BSTR strQuery = SysAllocString(L"SELECT * FROM Win32_DiskDrive");
    HRESULT hres = m_pSvc->ExecQuery(
        strQueryLanguage,
        strQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    SysFreeString(strQuery);
    SysFreeString(strQueryLanguage);

    if (FAILED(hres)) {
        qDebug() << "Query for Win32_DiskDrive failed. Error code =" << hres;
        return devices;
    }

    IWbemClassObject *pclsObj = nullptr;
    ULONG uReturn = 0;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn) {
            break;
        }

        StorageDevice device;
        VARIANT vtProp;

        // Get Model
        hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            device.model = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        // Get Manufacturer
        hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            device.manufacturer = variantToString(vtProp);
            VariantClear(&vtProp);
        }
        
        // Enhanced manufacturer detection from model name
        if (device.manufacturer == "N/A" || 
            device.manufacturer.contains("Standard", Qt::CaseInsensitive) ||
            device.manufacturer.contains("Стандартные", Qt::CaseInsensitive) ||
            device.manufacturer.isEmpty() ||
            device.manufacturer == "(Standard disk drives)") {
            QString modelLower = device.model.toLower();
            QString modelUpper = device.model.toUpper();
            
            // Priority detection - check model patterns
            if (modelUpper.contains("MICRON_") || modelLower.contains("micron")) 
                device.manufacturer = "Micron";
            else if (modelLower.contains("samsung")) 
                device.manufacturer = "Samsung";
            else if (modelLower.contains("wd_") || modelLower.contains("wd ") || 
                     modelLower.contains("western digital")) 
                device.manufacturer = "Western Digital";
            else if (modelLower.contains("seagate") || modelLower.contains("st")) 
                device.manufacturer = "Seagate";
            else if (modelLower.contains("kingston")) 
                device.manufacturer = "Kingston";
            else if (modelLower.contains("crucial")) 
                device.manufacturer = "Crucial";
            else if (modelLower.contains("intel")) 
                device.manufacturer = "Intel";
            else if (modelLower.contains("sandisk")) 
                device.manufacturer = "SanDisk";
            else if (modelLower.contains("toshiba")) 
                device.manufacturer = "Toshiba";
            else if (modelLower.contains("corsair")) 
                device.manufacturer = "Corsair";
            else if (modelLower.contains("adata")) 
                device.manufacturer = "ADATA";
            else if (modelLower.contains("hitachi") || modelLower.contains("hgst")) 
                device.manufacturer = "HGST/Hitachi";
        }

        // Get Serial Number with enhanced formatting
        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            device.serialNumber = variantToString(vtProp).trimmed();
            // Clean up serial number - remove excess whitespace
            device.serialNumber = device.serialNumber.simplified();
            // If serial number is too long or looks like hex data, format it nicely
            if (device.serialNumber.length() > 50) {
                // Keep original format for very long serial numbers
                device.serialNumber = device.serialNumber.replace(" ", "_");
            }
            // Add trailing dot for consistency with system info display
            if (!device.serialNumber.isEmpty() && !device.serialNumber.endsWith(".")) {
                device.serialNumber += ".";
            }
            VariantClear(&vtProp);
        }

        // Get Firmware Version
        hr = pclsObj->Get(L"FirmwareRevision", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            device.firmwareVersion = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        // Get Interface Type
        hr = pclsObj->Get(L"InterfaceType", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            QString rawInterface = variantToString(vtProp);
            device.interfaceType = determineInterfaceType(rawInterface);
            VariantClear(&vtProp);
        }

        // Get Media Type (to determine HDD vs SSD)
        hr = pclsObj->Get(L"MediaType", 0, &vtProp, 0, 0);
        QString mediaType;
        if (SUCCEEDED(hr)) {
            mediaType = variantToString(vtProp);
            VariantClear(&vtProp);
        }
        
        // Determine drive type using both media type and model
        device.driveType = determineDriveType(mediaType, device.model);
        
        // Smart interface detection: if model indicates NVMe but interface is SCSI or unknown
        if (device.driveType.contains("NVMe", Qt::CaseInsensitive)) {
            if (device.interfaceType == "SCSI" || 
                device.interfaceType == "Unknown" || 
                device.interfaceType.isEmpty()) {
                device.interfaceType = "NVMe";
            }
        }
        
        // Additional NVMe detection based on model patterns
        QString modelUpper = device.model.toUpper();
        if ((modelUpper.contains("MICRON_") || 
             device.model.toLower().contains("nvme") ||
             modelUpper.contains("_NVMe")) && 
            device.interfaceType != "NVMe") {
            device.interfaceType = "NVMe";
            if (!device.driveType.contains("NVMe", Qt::CaseInsensitive)) {
                device.driveType = "SSD (NVMe)";
            }
        }

        // Get Size
        hr = pclsObj->Get(L"Size", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            device.totalSize = variantToUInt64(vtProp);
            VariantClear(&vtProp);
        }

        // Get Device ID for partition info
        QString deviceID;
        hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            deviceID = variantToString(vtProp);
            VariantClear(&vtProp);
        }

        // Get free space information
        getDiskSpaceInfo(deviceID, device.totalSize, device.freeSpace, device.usedSpace);

        // Get Capabilities (supported modes)
        hr = pclsObj->Get(L"CapabilityDescriptions", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == (VT_ARRAY | VT_BSTR)) {
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

        // Debug output for verification
        qDebug() << "\n┌─────────────────────────────────────────────────────────────┐";
        qDebug() << "│ ОБНАРУЖЕНО УСТРОЙСТВО:";
        qDebug() << "├─────────────────────────────────────────────────────────────┤";
        qDebug() << QString("│  Модель: %1").arg(device.model);
        qDebug() << QString("│  Производитель: %1").arg(device.manufacturer);
        qDebug() << QString("│  Серийный номер: %1").arg(device.serialNumber);
        qDebug() << QString("│  Прошивка: %1").arg(device.firmwareVersion);
        qDebug() << QString("│  Тип: %1").arg(device.driveType);
        qDebug() << QString("│  Интерфейс: %1").arg(device.interfaceType);
        qDebug() << QString("│  Размер: %1 bytes").arg(device.totalSize);
        qDebug() << QString("│  Свободно: %1 bytes").arg(device.freeSpace);
        qDebug() << QString("│  Занято: %1 bytes").arg(device.usedSpace);
        qDebug() << QString("│  Системный: %1").arg(device.isSystemDrive ? "Да" : "Нет");
        qDebug() << "└─────────────────────────────────────────────────────────────┘";

        devices.push_back(device);
        pclsObj->Release();
    }

    pEnumerator->Release();
    return devices;
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

QString StorageScanner::determineInterfaceType(const QString& interfaceType) {
    if (interfaceType.contains("NVMe", Qt::CaseInsensitive)) {
        return "NVMe";
    } else if (interfaceType.contains("USB", Qt::CaseInsensitive)) {
        return "USB";
    } else if (interfaceType.contains("1394", Qt::CaseInsensitive)) {
        return "FireWire";
    } else if (interfaceType.contains("SATA", Qt::CaseInsensitive)) {
        return "SATA";
    } else if (interfaceType.contains("IDE", Qt::CaseInsensitive)) {
        return "IDE";
    } else if (interfaceType.contains("SCSI", Qt::CaseInsensitive)) {
        // SCSI может быть SATA или NVMe - нужна дополнительная проверка
        return "SCSI";
    }
    return interfaceType.isEmpty() ? "Unknown" : interfaceType;
}

QString StorageScanner::determineDriveType(const QString& mediaType, const QString& model) {
    // Check model first - more reliable
    QString modelLower = model.toLower();
    QString modelUpper = model.toUpper();
    
    // NVMe drives detection - check model patterns
    if (modelLower.contains("nvme") || 
        modelUpper.contains("MICRON_") || 
        modelLower.contains("micron 2") ||
        modelLower.contains("samsung 970") || 
        modelLower.contains("samsung 980") ||
        modelLower.contains("samsung 990") ||
        modelLower.contains("wd black sn") || 
        modelLower.contains("wd_black sn") ||
        modelLower.contains("kingston nv") ||
        modelLower.contains("crucial p") ||
        modelLower.contains("intel 660p") ||
        modelLower.contains("seagate firecuda")) {
        return "SSD (NVMe)";
    }
    
    // Check for common SSD model indicators (SATA SSDs)
    if (modelLower.contains("ssd") || 
        modelLower.contains("solid state") ||
        modelLower.contains("samsung 8") || 
        modelLower.contains("crucial mx") ||
        modelLower.contains("kingston sa") ||
        modelLower.contains("adata su") ||
        modelLower.contains("sandisk ultra")) {
        return "SSD";
    }
    
    // Check media type
    if (mediaType.contains("SSD", Qt::CaseInsensitive) ||
        mediaType.contains("Solid State", Qt::CaseInsensitive)) {
        return "SSD";
    }
    
    // Check for HDD indicators in media type
    if (mediaType.contains("Fixed", Qt::CaseInsensitive) ||
        mediaType.contains("hard disk", Qt::CaseInsensitive) ||
        mediaType.contains("HDD", Qt::CaseInsensitive)) {
        return "HDD";
    }
    
    // If no clear indicator, return Unknown
    return "Unknown";
}

void StorageScanner::getDiskSpaceInfo(const QString& deviceID, uint64_t& totalSize, uint64_t& freeSpace, uint64_t& usedSpace) {
    qDebug() << "=== СИСТЕМНЫЙ ВЫЗОВ: Getting disk space for:" << deviceID;
    
    // Извлекаем номер физического диска
    int physicalDriveNumber = getPhysicalDriveNumber(deviceID);
    
    if (physicalDriveNumber < 0) {
        qDebug() << "Could not extract physical drive number from" << deviceID;
        freeSpace = 0;
        usedSpace = totalSize;
        return;
    }
    
    // Используем системный вызов DeviceIoControl с IOCTL_STORAGE_GET_DEVICE_NUMBER
    // для сопоставления логических дисков с физическими
    uint64_t totalFree = getDriveSpaceByPhysicalNumber(physicalDriveNumber);
    
    freeSpace = totalFree;
    usedSpace = totalSize > freeSpace ? totalSize - freeSpace : 0;
    
    qDebug() << "=== РЕЗУЛЬТАТ: Free:" << freeSpace << "Used:" << usedSpace;
}

int StorageScanner::getPhysicalDriveNumber(const QString& deviceID) {
    // Извлекаем номер диска из строки типа "\\\\.\\PHYSICALDRIVE0"
    QString deviceIDUpper = deviceID.toUpper();
    if (deviceIDUpper.contains("PHYSICALDRIVE")) {
        int idx = deviceIDUpper.indexOf("PHYSICALDRIVE") + 13;
        QString numStr = deviceIDUpper.mid(idx);
        bool ok;
        int num = numStr.toInt(&ok);
        if (ok) {
            qDebug() << "Extracted physical drive number:" << num;
            return num;
        }
    }
    return -1;
}

uint64_t StorageScanner::getDriveSpaceByPhysicalNumber(int driveNumber) {
    qDebug() << "Getting space for physical drive:" << driveNumber;
    
    uint64_t totalFree = 0;
    
    // Перебираем все логические диски
    DWORD driveMask = GetLogicalDrives();
    
    for (int i = 0; i < 26; i++) {
        if (!(driveMask & (1 << i))) continue;
        
        wchar_t drivePath[8];
        swprintf(drivePath, 8, L"\\\\.\\%c:", L'A' + i);
        
        // Открываем логический диск для получения информации
        HANDLE hDevice = CreateFileW(
            drivePath,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        
        if (hDevice == INVALID_HANDLE_VALUE) {
            continue;
        }
        
        // Используем системный вызов IOCTL_STORAGE_GET_DEVICE_NUMBER
        STORAGE_DEVICE_NUMBER deviceNumber;
        DWORD bytesReturned;
        
        if (DeviceIoControl(
            hDevice,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            NULL,
            0,
            &deviceNumber,
            sizeof(deviceNumber),
            &bytesReturned,
            NULL
        )) {
            qDebug() << "Drive" << (char)('A' + i) << ":" 
                     << "is on physical drive" << deviceNumber.DeviceNumber;
            
            // Если это наш физический диск
            if ((int)deviceNumber.DeviceNumber == driveNumber) {
                CloseHandle(hDevice);
                
                // Получаем свободное место используя системный вызов
                wchar_t rootPath[4];
                swprintf(rootPath, 4, L"%c:\\", L'A' + i);
                
                ULARGE_INTEGER freeBytesAvailable;
                ULARGE_INTEGER totalNumberOfBytes;
                ULARGE_INTEGER totalNumberOfFreeBytes;
                
                if (GetDiskFreeSpaceExW(rootPath,
                                        &freeBytesAvailable,
                                        &totalNumberOfBytes,
                                        &totalNumberOfFreeBytes)) {
                    qDebug() << "  Drive" << (char)('A' + i) << ": Total:"
                             << totalNumberOfBytes.QuadPart
                             << "Free:" << totalNumberOfFreeBytes.QuadPart;
                    
                    totalFree += totalNumberOfFreeBytes.QuadPart;
                }
            }
        }
        
        CloseHandle(hDevice);
    }
    
    qDebug() << "Total free space for drive" << driveNumber << ":" << totalFree;
    return totalFree;
}

void StorageScanner::cleanup() {
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

