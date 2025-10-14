#ifndef USBDEVICE_H
#define USBDEVICE_H

#include <QString>
#include <QVector>
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <Usbiodef.h>
#include <Cfgmgr32.h>
#include <dbt.h>

// Класс для представления USB-устройства
class USBDevice
{
public:
    // Конструктор для создания устройства из уведомления системы
    USBDevice(PDEV_BROADCAST_DEVICEINTERFACE info, HWND hWnd);
    
    // Конструктор для создания устройства из списка устройств
    USBDevice(HDEVINFO deviceList, SP_DEVINFO_DATA deviceInfo, HWND hWnd);
    
    // Конструктор по умолчанию
    USBDevice();
    
    // Метод для безопасного извлечения устройства
    bool eject();
    
    // Геттеры
    QString getName() const { return name; }
    QString getPID() const { return pid; }
    QString getVID() const { return vid; }
    QString getManufacturer() const { return manufacturer; }
    QString getDeviceType() const { return deviceType; }
    QString getHardwareID() const { return hardwareID; }
    QString getDevicePath() const { return devicePath; }
    bool isEjectable() const { return ejectable; }
    bool isSafelyEjected() const { return safelyEjected; }
    DEVINST getDevInst() const { return devInst; }
    
    // Оператор сравнения
    bool operator==(const USBDevice& other) const;
    
    // Статический список всех подключенных устройств
    static QVector<USBDevice> devices;

private:
    QString hardwareID;      // Идентификатор оборудования
    QString name;            // Имя устройства
    QString devicePath;      // Путь к устройству
    QString pid;             // Product ID
    QString vid;             // Vendor ID
    QString manufacturer;    // Производитель
    QString deviceType;      // Тип устройства (HID, Mass Storage и т.д.)
    bool ejectable;          // Можно ли извлечь
    DEVINST devInst;         // Идентификатор устройства
    bool safelyEjected;      // Было ли безопасно извлечено
    
    // Вспомогательный метод для определения типа устройства
    QString determineDeviceType(HDEVINFO deviceList, SP_DEVINFO_DATA& deviceInfo);
};

#endif // USBDEVICE_H

