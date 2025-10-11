#ifndef USBMONITOR_H
#define USBMONITOR_H

#include <QThread>
#include <QVector>
#include <QString>
#include <windows.h>
#include "usbdevice.h"

// Класс для мониторинга USB-устройств
class USBMonitor : public QThread
{
    Q_OBJECT

public:
    explicit USBMonitor(QObject *parent = nullptr);
    ~USBMonitor();
    
    // Остановка мониторинга
    void stop();
    
    // Получить список текущих устройств
    QVector<USBDevice> getCurrentDevices() const;
    
    // Извлечь устройство по индексу
    bool ejectDevice(int index);

signals:
    // Сигналы для уведомления об событиях
    void deviceConnected(const QString& deviceName, const QString& pid);
    void deviceDisconnected(const QString& deviceName, const QString& pid);
    void deviceEjected(const QString& deviceName, bool success);
    void ejectFailed(const QString& deviceName);
    void logMessage(const QString& message);

protected:
    void run() override;

private:
    // Вспомогательные методы
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void handleDeviceArrival(PDEV_BROADCAST_DEVICEINTERFACE info, HWND hWnd);
    void handleDeviceRemoveComplete(PDEV_BROADCAST_DEVICEINTERFACE info);
    void handleDeviceQueryRemoveFailed();
    void loadInitialDevices(HWND hWnd);
    
    bool stopFlag;
    HWND hWnd;
    
    // Статический указатель на текущий экземпляр для доступа из WndProc
    static USBMonitor* instance;
};

#endif // USBMONITOR_H

