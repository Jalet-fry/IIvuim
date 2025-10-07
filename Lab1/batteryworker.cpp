#include "batteryworker.h"
#include <QDebug>
#include <QString>
#include <vector>

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
    updateBatteryInfo();
    timer->start(3000);
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
    qDebug() << "BATTERY WORKER: I AM WORKING";
    getBatteryInfo();
    emit batteryInfoUpdated(batteryInfo);
}

void BatteryWorker::getBatteryInfo()
{
    SYSTEM_POWER_STATUS status;

    if (GetSystemPowerStatus(&status)) {

        switch (status.ACLineStatus) {
        case 0:
            batteryInfo.powerType = "Battery";
            break;
        case 1:
            batteryInfo.powerType = "AC supply";
            break;
        default: batteryInfo.powerType = "Unknown"; break;
        }

        batteryInfo.chargeLevel = (status.BatteryLifePercent == 255) ?
            -1 : status.BatteryLifePercent;

        batteryInfo.batteryChemistry = getBatteryChemistry();

        batteryInfo.powerSaveMode = getPowerSaveMode();

        if (status.ACLineStatus == 1 || status.BatteryLifeTime == 0xFFFFFFFF) {
            batteryInfo.batteryLifeTime = -1;
        } else {
            batteryInfo.batteryLifeTime = status.BatteryLifeTime;
        }

        if (status.ACLineStatus == 1 || status.BatteryLifeTime == 0xFFFFFFFF) {
            batteryInfo.batteryFullLifeTime = -1;
        } else {
            batteryInfo.batteryFullLifeTime = static_cast<qint64>((100.0/status.BatteryLifePercent)*status.BatteryLifeTime);
        }

    } else {
        qDebug() << "ERROR: retreiving system power status failed:" << GetLastError();
    }
}

QString BatteryWorker::getBatteryChemistry()
{
    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVICE_BATTERY, 0, 0,
                                       DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev == INVALID_HANDLE_VALUE) {
        qDebug() << "SetupDiGetClassDevs failed:" << GetLastError();
        return "Unknown";
    }

    SP_DEVICE_INTERFACE_DATA did = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    if (!SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVICE_BATTERY, 0, &did)) {
        SetupDiDestroyDeviceInfoList(hdev);
        qDebug() << "SetupDiEnumDeviceInterfaces failed:" << GetLastError();
        return "Unknown";
    }

    DWORD needed = 0;
    SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &needed, 0);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        SetupDiDestroyDeviceInfoList(hdev);
        qDebug() << "SetupDiGetDeviceInterfaceDetail size failed:" << GetLastError();
        return "Unknown";
    }

    std::vector<BYTE> buffer(needed);
    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd =
        (PSP_DEVICE_INTERFACE_DETAIL_DATA)&buffer[0];
    pdidd->cbSize = sizeof(*pdidd);

    if (!SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, needed, 0, 0)) {
        SetupDiDestroyDeviceInfoList(hdev);
        qDebug() << "SetupDiGetDeviceInterfaceDetail failed:" << GetLastError();
        return "Unknown";
    }

    HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hBattery == INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(hdev);
        qDebug() << "CreateFile failed:" << GetLastError();
        return "Unknown";
    }

    BATTERY_QUERY_INFORMATION bqi = {0};
    DWORD dwWait = 0;
    DWORD dwOut;

    if (!DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG,
                        &dwWait, sizeof(dwWait),
                        &bqi.BatteryTag, sizeof(bqi.BatteryTag),
                        &dwOut, NULL) || !bqi.BatteryTag) {
        CloseHandle(hBattery);
        SetupDiDestroyDeviceInfoList(hdev);
        qDebug() << "IOCTL_BATTERY_QUERY_TAG failed:" << GetLastError();
        return "Неизвестно";
    }

    BATTERY_INFORMATION bi = {0};
    bqi.InformationLevel = BatteryInformation;

    if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION,
                       &bqi, sizeof(bqi), &bi, sizeof(bi), &dwOut, NULL)) {
        QString chemistry;

        char chemChars[5];
        memcpy(chemChars, &bi.Chemistry, 4);
        chemChars[4] = '\0';

        if (strcmp(chemChars, "LIon") == 0 || strcmp(chemChars, "Li-I") == 0) {
            chemistry = "Li-ion";
        } else if (strcmp(chemChars, "LiP") == 0) {
            chemistry = "Li-Poly";
        } else if (strcmp(chemChars, "NiCd") == 0) {
            chemistry = "NiCd";
        } else if (strcmp(chemChars, "NiMH") == 0) {
            chemistry = "NiMH";
        } else if (strcmp(chemChars, "PBat") == 0) {
            chemistry = "Pb";
        } else {
            chemistry = QString("Unknown %1").arg(chemChars);
        }

        CloseHandle(hBattery);
        SetupDiDestroyDeviceInfoList(hdev);
        return chemistry;
    } else {
        qDebug() << "IOCTL_BATTERY_QUERY_INFORMATION failed:" << GetLastError();
    }

    CloseHandle(hBattery);
    SetupDiDestroyDeviceInfoList(hdev);
    return "Unknown";
}

QString BatteryWorker::getPowerSaveMode() {
    GUID* schemeGuid = nullptr;
        if (PowerGetActiveScheme(nullptr, &schemeGuid) != ERROR_SUCCESS || !schemeGuid) return "—";

        DWORD size = 0;
        PowerReadFriendlyName(nullptr, schemeGuid, nullptr, nullptr, nullptr, &size);

        QString out = "—";
        if (size > 2) {
            QByteArray buf;
            buf.resize(size);
            if (PowerReadFriendlyName(nullptr, schemeGuid, nullptr, nullptr,
                                      reinterpret_cast<UCHAR*>(buf.data()), &size) == ERROR_SUCCESS) {
                out = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(buf.constData()));
            }
        }
        LocalFree(schemeGuid);
        return out;
}
