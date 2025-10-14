#include "usbdevice.h"
#include <QDebug>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "Cfgmgr32.lib")

// Инициализация статического вектора устройств
QVector<USBDevice> USBDevice::devices;

// Конструктор по умолчанию
USBDevice::USBDevice()
    : ejectable(false), devInst(0), safelyEjected(false)
{
}

// Вспомогательный метод для определения типа устройства
QString USBDevice::determineDeviceType(HDEVINFO deviceList, SP_DEVINFO_DATA& deviceInfo)
{
    WCHAR buffer[1024] = { 0 };
    
    // Пытаемся получить класс устройства
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_CLASS, 
                                          NULL, (PBYTE)buffer, sizeof(buffer), NULL))
    {
        QString deviceClass = QString::fromWCharArray(buffer);
        
        // Определяем тип по классу
        if (deviceClass.contains("HIDClass", Qt::CaseInsensitive))
            return "HID-устройство";
        else if (deviceClass.contains("DiskDrive", Qt::CaseInsensitive))
            return "Дисковый накопитель";
        else if (deviceClass.contains("USB", Qt::CaseInsensitive))
            return "USB-устройство";
        else if (deviceClass.contains("Mouse", Qt::CaseInsensitive))
            return "Мышь";
        else if (deviceClass.contains("Keyboard", Qt::CaseInsensitive))
            return "Клавиатура";
        else if (deviceClass.contains("Image", Qt::CaseInsensitive))
            return "Устройство обработки изображений";
        else if (deviceClass.contains("Media", Qt::CaseInsensitive))
            return "Медиа-устройство";
        else if (deviceClass.contains("Bluetooth", Qt::CaseInsensitive))
            return "Bluetooth-устройство";
        else if (deviceClass.contains("Camera", Qt::CaseInsensitive))
            return "Камера";
        else
            return deviceClass;
    }
    
    return "USB-устройство";
}

// Конструктор для создания устройства из уведомления системы
USBDevice::USBDevice(PDEV_BROADCAST_DEVICEINTERFACE info, HWND hWnd)
    : ejectable(false), devInst(0), safelyEjected(false)
{
    // Создаем список устройств
    HDEVINFO deviceList = SetupDiCreateDeviceInfoList(nullptr, nullptr);
    
    // Открываем интерфейс устройства
    SetupDiOpenDeviceInterfaceW(deviceList, (LPCWSTR)info->dbcc_name, NULL, NULL);
    
    // Создаем структуру для информации об устройстве
    SP_DEVINFO_DATA deviceInfo = { sizeof(SP_DEVINFO_DATA) };
    
    // Получаем информацию о первом устройстве
    SetupDiEnumDeviceInfo(deviceList, 0, &deviceInfo);
    
    // Используем другой конструктор для получения полной информации
    *this = USBDevice(deviceList, deviceInfo, hWnd);
}

// Конструктор для создания устройства из списка устройств
USBDevice::USBDevice(HDEVINFO deviceList, SP_DEVINFO_DATA deviceInfo, HWND hWnd)
    : ejectable(false), devInst(0), safelyEjected(false)
{
    // Получаем идентификатор устройства
    devInst = deviceInfo.DevInst;
    
    // Буфер для чтения данных
    WCHAR buffer[1024] = { 0 };
    
    QString friendlyName;
    QString deviceDesc;
    
    // Пытаемся получить "дружественное имя" (обычно более информативное)
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_FRIENDLYNAME, 
                                          NULL, (PBYTE)buffer, sizeof(buffer), NULL))
    {
        friendlyName = QString::fromWCharArray(buffer);
    }
    
    // Очищаем буфер
    ZeroMemory(buffer, sizeof(buffer));
    
    // Получаем описание устройства
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_DEVICEDESC, 
                                          NULL, (PBYTE)buffer, sizeof(buffer), NULL))
    {
        deviceDesc = QString::fromWCharArray(buffer);
    }
    
    // Очищаем буфер
    ZeroMemory(buffer, sizeof(buffer));
    
    // Получаем производителя
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_MFG, 
                                          NULL, (PBYTE)buffer, sizeof(buffer), NULL))
    {
        manufacturer = QString::fromWCharArray(buffer);
    }
    
    // Очищаем буфер
    ZeroMemory(buffer, sizeof(buffer));
    
    // Получаем идентификатор оборудования
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_HARDWAREID, 
                                          nullptr, (PBYTE)buffer, sizeof(buffer), nullptr))
    {
        hardwareID = QString::fromWCharArray(buffer);
        
        // Извлекаем VID и PID из строки идентификатора
        if (!hardwareID.isEmpty())
        {
            int vidIndex = hardwareID.indexOf("VID_");
            if (vidIndex != -1)
            {
                vid = hardwareID.mid(vidIndex + 4, 4);
            }
            
            int pidIndex = hardwareID.indexOf("PID_");
            if (pidIndex != -1)
            {
                pid = hardwareID.mid(pidIndex + 4, 4);
            }
        }
    }
    
    // Определяем тип устройства
    deviceType = determineDeviceType(deviceList, deviceInfo);
    
    // Выбираем лучшее имя устройства
    // Приоритет: FriendlyName > DeviceDesc, но избегаем "Составное USB устройство"
    if (!friendlyName.isEmpty() && 
        !friendlyName.contains("Составное USB устройство", Qt::CaseInsensitive) &&
        !friendlyName.contains("Composite USB", Qt::CaseInsensitive))
    {
        name = friendlyName;
    }
    else if (!deviceDesc.isEmpty() && 
             !deviceDesc.contains("Составное USB устройство", Qt::CaseInsensitive) &&
             !deviceDesc.contains("Composite USB", Qt::CaseInsensitive))
    {
        name = deviceDesc;
    }
    else
    {
        // Если нет хорошего имени, формируем из типа устройства и производителя
        if (!manufacturer.isEmpty() && !manufacturer.contains("(Стандартные", Qt::CaseInsensitive))
        {
            name = QString("%1 (%2)").arg(deviceType).arg(manufacturer);
        }
        else if (!friendlyName.isEmpty())
        {
            name = friendlyName;
        }
        else if (!deviceDesc.isEmpty())
        {
            name = deviceDesc;
        }
        else
        {
            name = deviceType;
        }
    }
    
    // Получаем информацию о возможностях устройства
    DWORD properties = 0;
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_CAPABILITIES, 
                                          NULL, (PBYTE)&properties, sizeof(DWORD), NULL))
    {
        ejectable = (properties & CM_DEVCAP_REMOVABLE) != 0;
    }
    
    // Если устройство извлекаемое и есть дескриптор окна, получаем путь к устройству
    if (hWnd && ejectable)
    {
        SP_DEVICE_INTERFACE_DATA devInterfaceData = { sizeof(devInterfaceData) };
        
        if (SetupDiEnumDeviceInterfaces(deviceList, &deviceInfo, 
                                        &GUID_DEVINTERFACE_USB_DEVICE, 0, &devInterfaceData))
        {
            DWORD requiredLength = 0;
            
            // Получаем требуемый размер
            SetupDiGetDeviceInterfaceDetailW(deviceList, &devInterfaceData, 
                                            NULL, 0, &requiredLength, NULL);
            
            if (requiredLength > 0)
            {
                // Выделяем память для детальной информации
                auto devInterfaceDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA_W)malloc(requiredLength);
                if (devInterfaceDetailData)
                {
                    devInterfaceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA_W);
                    
                    // Получаем детальную информацию
                    if (SetupDiGetDeviceInterfaceDetailW(deviceList, &devInterfaceData, 
                                                         devInterfaceDetailData, requiredLength, 
                                                         NULL, &deviceInfo))
                    {
                        devicePath = QString::fromWCharArray(devInterfaceDetailData->DevicePath);
                    }
                    
                    free(devInterfaceDetailData);
                }
            }
        }
    }
}

// Метод для безопасного извлечения устройства
bool USBDevice::eject()
{
    if (!ejectable)
    {
        qDebug() << "Устройство" << name << "нельзя извлечь";
        return false;
    }
    
    CONFIGRET result = CM_Request_Device_EjectW(devInst, nullptr, nullptr, NULL, NULL);
    
    if (result == CR_SUCCESS)
    {
        safelyEjected = true;
        qDebug() << "Устройство" << name << "безопасно извлечено";
        return true;
    }
    else
    {
        qDebug() << "Ошибка при извлечении устройства" << name << "Код ошибки:" << result;
        return false;
    }
}

// Оператор сравнения
bool USBDevice::operator==(const USBDevice& other) const
{
    return (hardwareID == other.hardwareID && 
            name == other.name && 
            pid == other.pid);
}

