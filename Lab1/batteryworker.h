#ifndef BATTERYWORKER_H
#define BATTERYWORKER_H

// Определяем версию Windows для совместимости
#ifndef _WIN32_WINNT
    #ifdef _WIN32_WINNT_0x0501  // Windows XP
        #define _WIN32_WINNT 0x0501
    #else
        #define _WIN32_WINNT 0x0600  // Windows Vista и выше
    #endif
#endif

// Для Windows XP совместимости
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <QObject>
#include <QTimer>

// Включаем заголовки в зависимости от версии Windows
#if _WIN32_WINNT >= 0x0600
    #include <powrprof.h>
#endif
#include <setupapi.h>
#include <batclass.h>
#include <initguid.h>

// Определяем недостающие константы для Windows XP совместимости
#ifndef FILE_DEVICE_BATTERY
#define FILE_DEVICE_BATTERY 0x00000029
#endif

#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED 0
#endif

#ifndef FILE_READ_ACCESS
#define FILE_READ_ACCESS 0x0001
#endif

#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

// Определяем IOCTL коды для батареи
#ifndef IOCTL_BATTERY_QUERY_TAG
#define IOCTL_BATTERY_QUERY_TAG \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x10, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#ifndef IOCTL_BATTERY_QUERY_INFORMATION
#define IOCTL_BATTERY_QUERY_INFORMATION \
    CTL_CODE(FILE_DEVICE_BATTERY, 0x11, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif


#ifndef GUID_DEVICE_BATTERY
DEFINE_GUID(GUID_DEVICE_BATTERY, 0x72631e54, 0x78a4, 0x11d0, 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a);
#endif

class BatteryWorker : public QObject
{
    Q_OBJECT

public:
    explicit BatteryWorker(QObject *parent = nullptr);
    ~BatteryWorker();

    struct BatteryInfo {
        QString powerType;
        QString batteryChemistry;
        int chargeLevel;
        QString powerSaveMode;
        qint64 batteryLifeTime;
        qint64 batteryFullLifeTime;
    };

signals:
    void batteryInfoUpdated(const BatteryInfo &info);
    void powerStatusChanged(bool isOnBattery);

public slots:
    void startMonitoring();
    void stopMonitoring();

private slots:
    void updateBatteryInfo();

private:
    QTimer *timer;
    BatteryInfo batteryInfo;
    void getBatteryInfo();
    QString getBatteryChemistry();
    QString getPowerSaveMode();
};

#endif // BATTERYWORKER_H
