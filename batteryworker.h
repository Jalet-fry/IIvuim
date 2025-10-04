#ifndef BATTERYWORKER_H
#define BATTERYWORKER_H

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <QObject>
#include <QTimer>
#include <Windows.h>
#include <powrprof.h>
#include <setupapi.h>
#include <batclass.h>
#include <initguid.h>


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
