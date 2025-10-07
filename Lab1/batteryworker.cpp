#include "batteryworker.h"
#include <QDebug>
#include <QString>
#include <vector>
#include <cstring>

BatteryWorker::BatteryWorker(QObject *parent) : QObject(parent)
{
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateBatteryInfo()));
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
        qDebug() << "BATTERY INFO: AC Line Status:" << status.ACLineStatus;
        qDebug() << "BATTERY INFO: Battery Life Percent:" << status.BatteryLifePercent;
        qDebug() << "BATTERY INFO: Battery Life Time:" << status.BatteryLifeTime;

        switch (status.ACLineStatus) {
        case 0:
            batteryInfo.powerType = "Battery";
            break;
        case 1:
            batteryInfo.powerType = "AC supply";
            break;
        case 255: // Неизвестно
            batteryInfo.powerType = "Unknown";
            break;
        default: 
            batteryInfo.powerType = "Unknown"; 
            break;
        }

        batteryInfo.chargeLevel = (status.BatteryLifePercent == 255) ?
            -1 : status.BatteryLifePercent;

        batteryInfo.batteryChemistry = getBatteryChemistry();

        batteryInfo.powerSaveMode = getPowerSaveMode();

        // Обработка времени работы батареи с проверкой на валидность
        if (status.ACLineStatus == 1 || status.BatteryLifeTime == 0xFFFFFFFF || status.BatteryLifeTime == 0) {
            batteryInfo.batteryLifeTime = -1;
        } else {
            batteryInfo.batteryLifeTime = status.BatteryLifeTime;
        }

        // Обработка полного времени работы батареи
        if (status.ACLineStatus == 1 || status.BatteryLifeTime == 0xFFFFFFFF || 
            status.BatteryLifeTime == 0 || status.BatteryLifePercent == 0 || status.BatteryLifePercent == 255) {
            batteryInfo.batteryFullLifeTime = -1;
        } else {
            batteryInfo.batteryFullLifeTime = static_cast<qint64>((100.0/status.BatteryLifePercent)*status.BatteryLifeTime);
        }

    } else {
        qDebug() << "ERROR: retrieving system power status failed:" << GetLastError();
        
        // Устанавливаем значения по умолчанию при ошибке
        batteryInfo.powerType = "Unknown";
        batteryInfo.chargeLevel = -1;
        batteryInfo.batteryChemistry = "Unknown";
        batteryInfo.powerSaveMode = "Unknown";
        batteryInfo.batteryLifeTime = -1;
        batteryInfo.batteryFullLifeTime = -1;
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

    SP_DEVICE_INTERFACE_DATA did;
    memset(&did, 0, sizeof(did));
    did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
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

    BATTERY_QUERY_INFORMATION bqi;
    memset(&bqi, 0, sizeof(bqi));
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

    BATTERY_INFORMATION bi;
    memset(&bi, 0, sizeof(bi));
    bqi.InformationLevel = BatteryInformation;

    if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION,
                       &bqi, sizeof(bqi), &bi, sizeof(bi), &dwOut, NULL)) {
        QString chemistry;

        char chemChars[5];
        memcpy(chemChars, &bi.Chemistry, 4);
        chemChars[4] = '\0';

        if (strcmp(chemChars, "LION") == 0 || strcmp(chemChars, "Li-I") == 0) {
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
#if _WIN32_WINNT >= 0x0600  // Windows Vista и выше
    // Проверяем, доступна ли функция PowerGetActiveScheme
    typedef DWORD (WINAPI *PowerGetActiveSchemeFunc)(HKEY, GUID**);
    typedef DWORD (WINAPI *PowerReadFriendlyNameFunc)(HKEY, GUID*, GUID*, GUID*, PUCHAR, LPDWORD);
    
    HMODULE hPowrProf = LoadLibraryA("powrprof.dll");
    if (hPowrProf) {
        PowerGetActiveSchemeFunc pPowerGetActiveScheme = 
            (PowerGetActiveSchemeFunc)GetProcAddress(hPowrProf, "PowerGetActiveScheme");
        PowerReadFriendlyNameFunc pPowerReadFriendlyName = 
            (PowerReadFriendlyNameFunc)GetProcAddress(hPowrProf, "PowerReadFriendlyName");
        
        if (pPowerGetActiveScheme && pPowerReadFriendlyName) {
            GUID* schemeGuid = nullptr;
            if (pPowerGetActiveScheme(nullptr, &schemeGuid) == ERROR_SUCCESS && schemeGuid) {
                DWORD size = 0;
                pPowerReadFriendlyName(nullptr, schemeGuid, nullptr, nullptr, nullptr, &size);

                QString out = "—";
                if (size > 2) {
                    QByteArray buf;
                    buf.resize(size);
                    if (pPowerReadFriendlyName(nullptr, schemeGuid, nullptr, nullptr,
                                              reinterpret_cast<UCHAR*>(buf.data()), &size) == ERROR_SUCCESS) {
                        out = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(buf.constData()));
                    }
                }
                LocalFree(schemeGuid);
                FreeLibrary(hPowrProf);
                return out;
            }
            if (schemeGuid) LocalFree(schemeGuid);
        }
        FreeLibrary(hPowrProf);
    }
    return "Схема питания недоступна";
#else  // Windows XP
    // Для Windows XP используем более простой подход
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                      "SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD activeScheme = 0;
        DWORD dataSize = sizeof(activeScheme);
        if (RegQueryValueExA(hKey, "ActivePowerScheme", nullptr, nullptr, 
                            reinterpret_cast<LPBYTE>(&activeScheme), &dataSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            switch (activeScheme) {
                case 0x00000000: return "Сбалансированная";
                case 0x00000001: return "Экономия энергии";
                case 0x00000002: return "Высокая производительность";
                default: return QString("Схема %1").arg(activeScheme, 8, 16, QChar('0'));
            }
        }
        RegCloseKey(hKey);
    }
    return "Windows XP (схема неизвестна)";
#endif
}
