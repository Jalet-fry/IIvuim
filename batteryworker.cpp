#include "batteryworker.h"
#include <QDebug>
#include <QString>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <iostream>
    
    // Интегрированные классы из вашего кода
    class BatteryInfo {
    public:
        std::string powerType;
        std::string batteryChemistry;
        int chargeLevel;
        std::string powerSaveMode;
        int batteryLifeTime;      // Оставшееся время работы в секундах
        int batteryFullLifeTime;  // Полное время работы от полного заряда в секундах
    };
    
    class BatteryManager {
    private:
        SYSTEM_POWER_STATUS powerStatus;
    
    public:
        BatteryInfo getBatteryInfo() {
            BatteryInfo info;
            
            if (GetSystemPowerStatus(&powerStatus)) {
                // Тип питания
                if (powerStatus.ACLineStatus == 1) {
                    info.powerType = "AC supply";
                } else if (powerStatus.ACLineStatus == 0) {
                    info.powerType = "Battery";
                } else {
                    info.powerType = "Unknown";
                }

                // Уровень заряда
                info.chargeLevel = powerStatus.BatteryLifePercent;
                if (info.chargeLevel == 255) info.chargeLevel = 0;

                // Химический состав батареи
                switch (powerStatus.BatteryFlag) {
                    case 1: info.batteryChemistry = "High"; break;
                    case 2: info.batteryChemistry = "Low"; break;
                    case 4: info.batteryChemistry = "Critical"; break;
                    case 8: info.batteryChemistry = "Charging"; break;
                    default: info.batteryChemistry = "Unknown";
                }

                // Режим энергосбережения
                info.powerSaveMode = (powerStatus.BatteryFlag & 8) ? "On" : "Off";

                // Расчет времени работы батареи
                if (powerStatus.ACLineStatus == 0) { // На батарее
                    // Оставшееся время работы
                    info.batteryLifeTime = powerStatus.BatteryLifeTime;
                    if (info.batteryLifeTime == -1) {
                        info.batteryLifeTime = -1; // Расчет недоступен
                    }

                    // Расчет полного времени работы от полного заряда
                    if (info.chargeLevel > 0 && info.batteryLifeTime != -1) {
                        // Формула: (текущее_время * 100) / уровень_заряда
                        info.batteryFullLifeTime = (info.batteryLifeTime * 100) / info.chargeLevel;
                    } else {
                        info.batteryFullLifeTime = -1;
                    }
                } else {
                    // На питании от сети
                    info.batteryLifeTime = -1;
                    info.batteryFullLifeTime = -1;
                }
            }
            
            return info;
        }
    };
    
    // Глобальный экземпляр менеджера батареи
    static BatteryManager batteryManager;
    
#endif

BatteryWorker::BatteryWorker(QObject *parent) : QObject(parent)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BatteryWorker::updateBatteryInfo);
}

BatteryWorker::~BatteryWorker()
{
    stopMonitoring();
}

void BatteryWorker::startMonitoring()
{
    updateBatteryInfo(); // Немедленное обновление при запуске
    timer->start(3000); // Обновление каждые 3 секунды
    qDebug() << "Battery monitoring started";
}

void BatteryWorker::stopMonitoring()
{
    if(timer && timer->isActive()) {
        timer->stop();
        qDebug() << "Battery monitoring stopped";
    }
}

void BatteryWorker::updateBatteryInfo()
{
    qDebug() << "Обновление информации о батарее...";

#ifdef Q_OS_WIN
    // Используем интегрированный BatteryManager
    BatteryInfo info = batteryManager.getBatteryInfo();
    
    // Конвертируем std::string в QString
    QString powerType = QString::fromStdString(info.powerType);
    QString batteryChemistry = QString::fromStdString(info.batteryChemistry);
    QString powerSaveMode = QString::fromStdString(info.powerSaveMode);
    
    emit batteryInfoUpdated(powerType, batteryChemistry, info.chargeLevel, 
                           powerSaveMode, info.batteryLifeTime, info.batteryFullLifeTime);
#else
    // Заглушка для других ОС
    emit batteryInfoUpdated("Unknown", "Unknown", 0, "Off", -1, -1);
#endif
}
