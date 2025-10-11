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
    
    // Получаем описание устройства
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_DEVICEDESC, 
                                          NULL, (PBYTE)buffer, sizeof(buffer), NULL))
    {
        name = QString::fromWCharArray(buffer);
    }
    
    // Очищаем буфер
    ZeroMemory(buffer, sizeof(buffer));
    
    // Получаем идентификатор оборудования
    if (SetupDiGetDeviceRegistryPropertyW(deviceList, &deviceInfo, SPDRP_HARDWAREID, 
                                          nullptr, (PBYTE)buffer, sizeof(buffer), nullptr))
    {
        hardwareID = QString::fromWCharArray(buffer);
        
        // Извлекаем PID из строки идентификатора
        if (!hardwareID.isEmpty())
        {
            int pidIndex = hardwareID.indexOf("PID_");
            if (pidIndex != -1)
            {
                pid = hardwareID.mid(pidIndex + 4, 4);
            }
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

