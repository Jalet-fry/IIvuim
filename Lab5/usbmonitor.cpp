#include "usbmonitor.h"
#include <QDebug>
#include <dbt.h>

// Инициализация статического указателя
USBMonitor* USBMonitor::instance = nullptr;

USBMonitor::USBMonitor(QObject *parent)
    : QThread(parent), stopFlag(false), hWnd(NULL)
{
    instance = this;
}

USBMonitor::~USBMonitor()
{
    stop();
    wait();
    instance = nullptr;
}

void USBMonitor::stop()
{
    stopFlag = true;
    if (hWnd)
    {
        PostMessage(hWnd, WM_QUIT, 0, 0);
    }
}

QVector<USBDevice> USBMonitor::getCurrentDevices() const
{
    return USBDevice::devices;
}

bool USBMonitor::ejectDevice(int index)
{
    if (index < 0 || index >= USBDevice::devices.size())
    {
        emit logMessage(QString("Неверный индекс устройства: %1").arg(index));
        return false;
    }
    
    USBDevice& device = USBDevice::devices[index];
    
    if (!device.isEjectable())
    {
        emit logMessage(QString("Устройство '%1' нельзя извлечь").arg(device.getName()));
        emit ejectFailed(device.getName());
        return false;
    }
    
    bool success = device.eject();
    emit deviceEjected(device.getName(), success);
    
    if (success)
    {
        emit logMessage(QString("✓ Устройство '%1' безопасно извлечено").arg(device.getName()));
    }
    else
    {
        emit logMessage(QString("✗ Ошибка при извлечении устройства '%1'").arg(device.getName()));
        emit ejectFailed(device.getName());
    }
    
    return success;
}

void USBMonitor::run()
{
    emit logMessage("Запуск мониторинга USB-устройств...");
    
    // Регистрация класса окна
    WNDCLASSEXW wx = { 0 };
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = (WNDPROC)WndProc;
    wx.lpszClassName = L"USBMonitorClass";
    
    if (!RegisterClassExW(&wx))
    {
        emit logMessage("Ошибка регистрации класса окна");
        return;
    }
    
    // Создание скрытого окна для получения уведомлений
    hWnd = CreateWindowW(L"USBMonitorClass", L"USBMonitorWindow", 
                         WS_ICONIC, 0, 0, CW_USEDEFAULT, 0, 0, NULL, 
                         GetModuleHandleW(nullptr), NULL);
    
    if (!hWnd)
    {
        emit logMessage("Ошибка создания окна");
        return;
    }
    
    // Регистрация уведомлений о USB-устройствах
    DEV_BROADCAST_DEVICEINTERFACE_W filter = { 0 };
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    
    HDEVNOTIFY hDevNotify = RegisterDeviceNotificationW(hWnd, &filter, 
                                                        DEVICE_NOTIFY_WINDOW_HANDLE);
    
    if (!hDevNotify)
    {
        emit logMessage("Ошибка регистрации уведомлений об устройствах");
        DestroyWindow(hWnd);
        return;
    }
    
    // Загрузка списка текущих устройств
    loadInitialDevices(hWnd);
    
    emit logMessage("Мониторинг USB-устройств запущен");
    
    // Цикл обработки сообщений
    MSG msg;
    while (!stopFlag)
    {
        if (PeekMessageW(&msg, hWnd, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
                
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else
        {
            msleep(10); // Небольшая задержка для снижения нагрузки на CPU
        }
    }
    
    // Очистка
    if (hDevNotify)
    {
        UnregisterDeviceNotification(hDevNotify);
    }
    
    if (hWnd)
    {
        DestroyWindow(hWnd);
        hWnd = NULL;
    }
    
    UnregisterClassW(L"USBMonitorClass", GetModuleHandleW(nullptr));
    
    emit logMessage("Мониторинг USB-устройств остановлен");
}

void USBMonitor::loadInitialDevices(HWND hWnd)
{
    // Очистка списка устройств
    USBDevice::devices.clear();
    
    // Получение списка всех USB-устройств
    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DEVICE, 
                                                         nullptr, nullptr, 
                                                         DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        emit logMessage("Не удалось получить информацию об устройствах");
        return;
    }
    
    // Перебор всех устройств
    DWORD deviceIndex = 0;
    while (SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &devInfoData))
    {
        USBDevice device(deviceInfoSet, devInfoData, hWnd);
        
        if (!device.getName().isEmpty())
        {
            USBDevice::devices.append(device);
            emit logMessage(QString("Обнаружено устройство: %1 [PID: %2]")
                          .arg(device.getName())
                          .arg(device.getPID()));
        }
        
        deviceIndex++;
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    
    emit logMessage(QString("Загружено устройств: %1").arg(USBDevice::devices.size()));
}

LRESULT CALLBACK USBMonitor::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DEVICECHANGE && instance)
    {
        switch (wParam)
        {
            case DBT_DEVICEARRIVAL:
            {
                PDEV_BROADCAST_DEVICEINTERFACE info = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
                if (info->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE &&
                    info->dbcc_classguid == GUID_DEVINTERFACE_USB_DEVICE)
                {
                    instance->handleDeviceArrival(info, hWnd);
                }
                break;
            }
            
            case DBT_DEVICEREMOVECOMPLETE:
            {
                PDEV_BROADCAST_DEVICEINTERFACE info = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
                if (info->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                {
                    instance->handleDeviceRemoveComplete(info);
                }
                break;
            }
            
            case DBT_DEVICEQUERYREMOVEFAILED:
            {
                instance->handleDeviceQueryRemoveFailed();
                break;
            }
        }
    }
    
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void USBMonitor::handleDeviceArrival(PDEV_BROADCAST_DEVICEINTERFACE info, HWND hWnd)
{
    try
    {
        USBDevice device(info, hWnd);
        
        if (!device.getName().isEmpty())
        {
            // Проверяем, нет ли уже такого устройства
            bool exists = false;
            for (const USBDevice& existingDevice : USBDevice::devices)
            {
                if (existingDevice == device)
                {
                    exists = true;
                    break;
                }
            }
            
            if (!exists)
            {
                USBDevice::devices.append(device);
                
                emit logMessage(QString("→ Подключено: %1 [PID: %2]")
                              .arg(device.getName())
                              .arg(device.getPID()));
                              
                emit deviceConnected(device.getName(), device.getPID());
            }
        }
    }
    catch (...)
    {
        emit logMessage("Ошибка при обработке подключения устройства");
    }
}

void USBMonitor::handleDeviceRemoveComplete(PDEV_BROADCAST_DEVICEINTERFACE info)
{
    try
    {
        USBDevice tempDevice(info, hWnd);
        
        // Поиск и удаление устройства из списка
        for (int i = 0; i < USBDevice::devices.size(); ++i)
        {
            if (USBDevice::devices[i] == tempDevice)
            {
                USBDevice removedDevice = USBDevice::devices[i];
                USBDevice::devices.removeAt(i);
                
                if (removedDevice.isSafelyEjected())
                {
                    emit logMessage(QString("← Безопасно отключено: %1 [PID: %2]")
                                  .arg(removedDevice.getName())
                                  .arg(removedDevice.getPID()));
                }
                else
                {
                    emit logMessage(QString("⚠ Небезопасно отключено: %1 [PID: %2]")
                                  .arg(removedDevice.getName())
                                  .arg(removedDevice.getPID()));
                }
                
                emit deviceDisconnected(removedDevice.getName(), removedDevice.getPID());
                break;
            }
        }
    }
    catch (...)
    {
        emit logMessage("Ошибка при обработке отключения устройства");
    }
}

void USBMonitor::handleDeviceQueryRemoveFailed()
{
    emit logMessage("✗ Отказ в безопасном извлечении устройства");
}

