#ifndef BATTERYWORKER_H
#define BATTERYWORKER_H

#include <QObject>
#include <QTimer>

class BatteryWorker : public QObject
{
    Q_OBJECT

public:
    explicit BatteryWorker(QObject *parent = 0);  // Используем 0 вместо nullptr
    ~BatteryWorker();

    void startMonitoring();
    void stopMonitoring();

signals:
    void batteryInfoUpdated(const QString &powerType, const QString &batteryChemistry,
                           int chargeLevel, const QString &powerSaveMode,
                           int batteryLifeTime, int batteryFullLifeTime);

private slots:
    void updateBatteryInfo();

private:
    QTimer *timer;
};

#endif // BATTERYWORKER_H
