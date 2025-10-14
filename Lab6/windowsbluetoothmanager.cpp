#include "windowsbluetoothmanager.h"
#include <QDebug>
#include <QDateTime>
#include <winsock2.h>
#include <ws2bth.h>

// ============================================================================
// BluetoothDeviceData - вспомогательные функции
// ============================================================================

QString BluetoothDeviceData::getDeviceTypeString() const
{
    // Извлекаем Major Device Class (биты 8-12)
    DWORD majorClass = (deviceClass >> 8) & 0x1F;
    
    switch (majorClass) {
    case 0x01: return "Компьютер";
    case 0x02: return "Телефон";
    case 0x03: return "LAN/Network";
    case 0x04: return "Аудио/Видео";
    case 0x05: return "Периферия";
    case 0x06: return "Изображение";
    case 0x07: return "Носимое устройство";
    case 0x08: return "Игрушка";
    case 0x09: return "Здоровье";
    default: return "Неизвестно";
    }
}

BluetoothDeviceCapabilities BluetoothDeviceData::getCapabilities() const
{
    BluetoothDeviceCapabilities caps;
    
    // Извлекаем Major Device Class (биты 8-12)
    DWORD majorClass = (deviceClass >> 8) & 0x1F;
    
    // Извлекаем Minor Device Class (биты 2-7)
    DWORD minorClass = (deviceClass >> 2) & 0x3F;
    
    // По умолчанию
    caps.canReceiveFiles = false;
    caps.canSendFiles = false;
    caps.supportsOBEX = false;
    caps.supportsRFCOMM = false;
    caps.isAudioDevice = false;
    caps.isInputDevice = false;
    caps.recommendedMethod = "";
    caps.blockReason = "";
    
    switch (majorClass) {
    case 0x01: // Компьютер (Desktop, Laptop, Server, etc.)
        caps.canReceiveFiles = true;
        caps.canSendFiles = true;
        caps.supportsOBEX = true;
        caps.supportsRFCOMM = true;
        caps.recommendedMethod = "RFCOMM (прямое подключение)";
        break;
        
    case 0x02: // Телефон (Cellular, Cordless, Smartphone)
        caps.canReceiveFiles = true;
        caps.canSendFiles = true;
        caps.supportsOBEX = true;  // Телефоны обычно поддерживают OBEX Push
        caps.supportsRFCOMM = false; // Нет нашего RFCOMM сервера
        caps.recommendedMethod = "OBEX (прямой протокол)";
        break;
        
    case 0x04: // Аудио/Видео (наушники, колонки, микрофоны)
        caps.isAudioDevice = true;
        caps.canReceiveFiles = false;
        caps.canSendFiles = false;
        caps.blockReason = "Аудио устройство не может принимать файлы";
        
        // Проверяем Minor Class для деталей
        if ((minorClass >= 0x01 && minorClass <= 0x06) || minorClass == 0x0E) {
            // Наушники, гарнитура, колонки
            caps.blockReason = "Наушники/колонки предназначены только для воспроизведения аудио";
        }
        break;
        
    case 0x05: // Периферия (мышь, клавиатура, джойстик)
        caps.isInputDevice = true;
        caps.canReceiveFiles = false;
        caps.canSendFiles = false;
        
        // Проверяем Minor Class
        if ((minorClass & 0x30) == 0x10) {
            // Клавиатура
            caps.blockReason = "Клавиатура предназначена только для ввода";
        } else if ((minorClass & 0x30) == 0x20) {
            // Мышь
            caps.blockReason = "Мышь предназначена только для управления курсором";
        } else {
            caps.blockReason = "Устройство ввода не может принимать файлы";
        }
        break;
        
    case 0x06: // Изображение (принтер, сканер, камера)
        // Принтеры могут принимать файлы для печати
        if (minorClass == 0x02) {  // Printer
            caps.canReceiveFiles = true;
            caps.supportsOBEX = true;
            caps.recommendedMethod = "OBEX (для печати)";
        } else {
            caps.blockReason = "Устройство не поддерживает прием файлов";
        }
        break;
        
    default:
        caps.blockReason = "Неизвестный тип устройства";
        break;
    }
    
    return caps;
}

bool BluetoothDeviceData::canSendFilesTo() const
{
    BluetoothDeviceCapabilities caps = getCapabilities();
    return caps.canReceiveFiles;
}

// ============================================================================
// BluetoothScanWorker - поток для сканирования
// ============================================================================

BluetoothScanWorker::BluetoothScanWorker(QObject *parent)
    : QThread(parent)
    , shouldStop(false)
{
}

BluetoothScanWorker::~BluetoothScanWorker()
{
    stopScanning();
    wait();
}

void BluetoothScanWorker::stopScanning()
{
    shouldStop = true;
}

QString BluetoothScanWorker::formatBluetoothAddress(const BLUETOOTH_ADDRESS &btAddr) const
{
    return QString("%1:%2:%3:%4:%5:%6")
        .arg(btAddr.rgBytes[5], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[4], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[3], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[2], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[1], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[0], 2, 16, QChar('0')).toUpper();
}

QString BluetoothScanWorker::getDeviceClassName(DWORD classOfDevice) const
{
    DWORD majorClass = (classOfDevice >> 8) & 0x1F;
    
    switch (majorClass) {
    case 0x01: return "Компьютер";
    case 0x02: return "Телефон";
    case 0x03: return "LAN/Network";
    case 0x04: return "Аудио/Видео";
    case 0x05: return "Периферия";
    case 0x06: return "Изображение";
    case 0x07: return "Носимое устройство";
    case 0x08: return "Игрушка";
    case 0x09: return "Здоровье";
    default: return "Неизвестно";
    }
}

void BluetoothScanWorker::run()
{
    emit logMessage("═══════════════════════════════════════════");
    emit logMessage("=== WINDOWS BLUETOOTH SCAN STARTED ===");
    emit logMessage("═══════════════════════════════════════════");
    emit logMessage("");
    
    emit logMessage("DEBUG: BluetoothScanWorker::run() начат");
    
    // Параметры поиска
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams;
    ZeroMemory(&searchParams, sizeof(searchParams));
    searchParams.dwSize = sizeof(searchParams);
    searchParams.fReturnAuthenticated = TRUE;   // Сопряженные устройства
    searchParams.fReturnRemembered = TRUE;       // Запомненные устройства
    searchParams.fReturnConnected = TRUE;        // Подключенные устройства
    searchParams.fReturnUnknown = TRUE;          // Неизвестные устройства
    searchParams.fIssueInquiry = TRUE;           // Выполнить активное сканирование
    searchParams.cTimeoutMultiplier = 4;         // Таймаут (4 * 1.28 сек = ~5 сек)
    
    emit logMessage("DEBUG: Параметры поиска установлены");
    
    // Информация об устройстве
    BLUETOOTH_DEVICE_INFO deviceInfo;
    ZeroMemory(&deviceInfo, sizeof(deviceInfo));
    deviceInfo.dwSize = sizeof(deviceInfo);
    
    emit logMessage("DEBUG: Структура deviceInfo подготовлена");
    
    emit logMessage("ПАРАМЕТРЫ СКАНИРОВАНИЯ:");
    emit logMessage(QString("  • dwSize: %1").arg(searchParams.dwSize));
    emit logMessage(QString("  • Таймаут: %1 сек").arg(searchParams.cTimeoutMultiplier * 1.28));
    emit logMessage("  • Поиск сопряженных: ДА");
    emit logMessage("  • Поиск подключенных: ДА");
    emit logMessage("  • Активное сканирование: ДА");
    emit logMessage("");
    emit logMessage("Поиск устройств...");
    emit logMessage("Вызов BluetoothFindFirstDevice()...");
    emit logMessage("─────────────────────────────────────────");
    emit logMessage("");
    
    // Начинаем поиск
    emit logMessage("DEBUG: Вызов BluetoothFindFirstDevice...");
    HBLUETOOTH_DEVICE_FIND hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    
    emit logMessage(QString("DEBUG: BluetoothFindFirstDevice вернул HANDLE = 0x%1").arg((quintptr)hFind, 0, 16));
    
    if (hFind == NULL) {
        DWORD error = GetLastError();
        emit logMessage(QString("DEBUG: GetLastError() = 0x%1").arg(error, 0, 16));
        
        if (error == ERROR_NO_MORE_ITEMS) {
            emit logMessage("⚠ Устройства не найдены (ERROR_NO_MORE_ITEMS)");
            emit logMessage("");
            emit logMessage("ВОЗМОЖНЫЕ ПРИЧИНЫ:");
            emit logMessage("  1. Нет Bluetooth устройств в радиусе действия");
            emit logMessage("  2. Все устройства уже подключены (не в режиме обнаружения)");
            emit logMessage("  3. Bluetooth выключен на устройствах");
            emit logMessage("");
            emit scanCompleted(0);
        } else {
            QString errorMsg = QString("Ошибка BluetoothFindFirstDevice: 0x%1").arg(error, 0, 16);
            emit logMessage("ERROR: " + errorMsg);
            
            // Расшифровываем код ошибки
            switch (error) {
            case ERROR_INVALID_PARAMETER:
                emit logMessage("  → ERROR_INVALID_PARAMETER (87): Неверный параметр");
                break;
            case ERROR_REVISION_MISMATCH:
                emit logMessage("  → ERROR_REVISION_MISMATCH: Несоответствие версии");
                break;
            default:
                emit logMessage(QString("  → Неизвестная ошибка: %1").arg(error));
                break;
            }
            
            emit scanError(errorMsg);
        }
        return;
    }
    
    emit logMessage("DEBUG: BluetoothFindFirstDevice успешен! Перебор устройств...");
    
    int deviceCount = 0;
    
    // Обрабатываем первое устройство
    do {
        if (shouldStop) {
            emit logMessage("DEBUG: shouldStop = true, прерывание");
            emit logMessage("Сканирование прервано пользователем");
            break;
        }
        
        emit logMessage(QString("DEBUG: Обработка устройства #%1...").arg(deviceCount + 1));
        
        // Заполняем структуру данных
        BluetoothDeviceData device;
        device.name = QString::fromWCharArray(deviceInfo.szName);
        device.address = formatBluetoothAddress(deviceInfo.Address);
        device.deviceClass = deviceInfo.ulClassofDevice;
        device.isConnected = deviceInfo.fConnected != 0;
        device.isPaired = deviceInfo.fAuthenticated != 0;
        device.isRemembered = deviceInfo.fRemembered != 0;
        device.lastSeen = deviceInfo.stLastSeen;
        device.lastUsed = deviceInfo.stLastUsed;
        
        emit logMessage(QString("DEBUG: Устройство: %1, MAC: %2").arg(device.name).arg(device.address));
        emit logMessage(QString("DEBUG: Connected=%1 Paired=%2 Remembered=%3")
            .arg(device.isConnected).arg(device.isPaired).arg(device.isRemembered));
        
        // НОВОЕ: Логируем возможности устройства
        BluetoothDeviceCapabilities caps = device.getCapabilities();
        emit logMessage(QString("  → Тип: %1").arg(device.getDeviceTypeString()));
        emit logMessage(QString("  → Может принимать файлы: %1").arg(caps.canReceiveFiles ? "ДА" : "НЕТ"));
        if (!caps.canReceiveFiles && !caps.blockReason.isEmpty()) {
            emit logMessage(QString("  → Причина: %1").arg(caps.blockReason));
        }
        if (caps.canReceiveFiles && !caps.recommendedMethod.isEmpty()) {
            emit logMessage(QString("  → Метод: %1").arg(caps.recommendedMethod));
        }
        emit logMessage("");
        
        emit deviceFound(device);
        deviceCount++;
        
        // Подготовка для следующего устройства
        ZeroMemory(&deviceInfo, sizeof(deviceInfo));
        deviceInfo.dwSize = sizeof(deviceInfo);
        
        emit logMessage("DEBUG: Вызов BluetoothFindNextDevice...");
        
    } while (BluetoothFindNextDevice(hFind, &deviceInfo) && !shouldStop);
    
    emit logMessage(QString("DEBUG: Цикл завершен, найдено устройств: %1").arg(deviceCount));
    
    // Закрываем поиск
    emit logMessage("DEBUG: Вызов BluetoothFindDeviceClose...");
    BluetoothFindDeviceClose(hFind);
    emit logMessage("DEBUG: BluetoothFindDeviceClose выполнен");
    
    emit logMessage("");
    emit logMessage("═══════════════════════════════════════════");
    emit logMessage(QString("=== НАЙДЕНО УСТРОЙСТВ: %1 ===").arg(deviceCount));
    emit logMessage("═══════════════════════════════════════════");
    
    emit scanCompleted(deviceCount);
    emit logMessage("DEBUG: BluetoothScanWorker::run() завершен");
}

// ============================================================================
// WindowsBluetoothManager - основной менеджер
// ============================================================================

WindowsBluetoothManager::WindowsBluetoothManager(QObject *parent)
    : QObject(parent)
    , scanWorker(nullptr)
    , scanning(false)
    , cacheValid(false)
{
    // Регистрируем тип для передачи через сигналы/слоты между потоками
    qRegisterMetaType<BluetoothDeviceData>("BluetoothDeviceData");
    
    emit logMessage("═══════════════════════════════════════════");
    emit logMessage("=== WINDOWS BLUETOOTH MANAGER INIT ===");
    emit logMessage("═══════════════════════════════════════════");
    emit logMessage("");
    
    // Проверяем доступность Bluetooth
    updateLocalDeviceCache();
    
    if (cachedAvailability) {
        emit logMessage("✓ Windows Bluetooth API доступен");
        emit logMessage("");
        emit logMessage("ИНФОРМАЦИЯ О ЛОКАЛЬНОМ АДАПТЕРЕ:");
        emit logMessage("  Имя: " + (cachedLocalName.isEmpty() ? "(не указано)" : cachedLocalName));
        emit logMessage("  MAC: " + (cachedLocalAddress.isEmpty() ? "(недоступен)" : cachedLocalAddress));
        emit logMessage("");
        
        // Получаем список уже подключенных устройств
        emit logMessage("ПРОВЕРКА ПОДКЛЮЧЕННЫХ УСТРОЙСТВ:");
        QList<BluetoothDeviceData> connected = getConnectedDevices();
        if (!connected.isEmpty()) {
            emit logMessage(QString("  Найдено: %1").arg(connected.count()));
            for (int i = 0; i < connected.count(); ++i) {
                const BluetoothDeviceData &dev = connected[i];
                emit logMessage(QString("  %1. %2 (%3)")
                    .arg(i + 1)
                    .arg(dev.name)
                    .arg(dev.address));
            }
        } else {
            emit logMessage("  (Нет активных подключений через Windows API)");
        }
        emit logMessage("");
        
    } else {
        emit logMessage("✗ Bluetooth адаптер НЕ НАЙДЕН");
        emit logMessage("");
        emit logMessage("ВОЗМОЖНЫЕ ПРИЧИНЫ:");
        emit logMessage("  1. Bluetooth выключен в системе");
        emit logMessage("  2. Отсутствуют драйверы");
        emit logMessage("  3. Нет Bluetooth адаптера");
        emit logMessage("");
    }
    
    emit logMessage("═══════════════════════════════════════════");
    emit logMessage("=== ИНИЦИАЛИЗАЦИЯ ЗАВЕРШЕНА ===");
    emit logMessage("═══════════════════════════════════════════");
    emit logMessage("");
}

WindowsBluetoothManager::~WindowsBluetoothManager()
{
    stopDeviceDiscovery();
}

void WindowsBluetoothManager::startDeviceDiscovery()
{
    emit logMessage("DEBUG: startDeviceDiscovery() вызван");
    
    if (scanning) {
        emit logMessage("⚠ Сканирование уже выполняется");
        return;
    }
    
    emit logMessage("DEBUG: Проверка доступности адаптера...");
    if (!isBluetoothAvailable()) {
        emit logMessage("✗ Bluetooth адаптер недоступен");
        emit discoveryError("Bluetooth адаптер недоступен");
        return;
    }
    
    emit logMessage("DEBUG: Создание потока сканирования...");
    
    // Создаем и запускаем поток сканирования
    scanWorker = new BluetoothScanWorker(this);
    
    emit logMessage("DEBUG: Подключение сигналов...");
    
    // Используем Qt::QueuedConnection для межпотоковой связи
    connect(scanWorker, &BluetoothScanWorker::deviceFound,
            this, &WindowsBluetoothManager::onDeviceFound, Qt::QueuedConnection);
    connect(scanWorker, &BluetoothScanWorker::scanCompleted,
            this, &WindowsBluetoothManager::onScanCompleted, Qt::QueuedConnection);
    connect(scanWorker, &BluetoothScanWorker::scanError,
            this, &WindowsBluetoothManager::onScanError, Qt::QueuedConnection);
    connect(scanWorker, &BluetoothScanWorker::logMessage,
            this, &WindowsBluetoothManager::onScanLogMessage, Qt::QueuedConnection);
    
    connect(scanWorker, &BluetoothScanWorker::finished,
            scanWorker, &QObject::deleteLater);
    
    emit logMessage("DEBUG: Запуск потока...");
    
    scanning = true;
    scanWorker->start();
    
    emit logMessage("DEBUG: Поток запущен, ожидание результатов...");
    emit logMessage("DEBUG: Поток должен эмитировать логи из своего run()...");
}

void WindowsBluetoothManager::stopDeviceDiscovery()
{
    if (scanWorker) {
        scanWorker->stopScanning();
        scanWorker->wait(3000); // Ждем до 3 секунд
        scanWorker = nullptr;
    }
    scanning = false;
}

bool WindowsBluetoothManager::isScanning() const
{
    return scanning;
}

QString WindowsBluetoothManager::getLocalDeviceName() const
{
    if (!cacheValid) {
        updateLocalDeviceCache();
    }
    return cachedLocalName;
}

QString WindowsBluetoothManager::getLocalDeviceAddress() const
{
    if (!cacheValid) {
        updateLocalDeviceCache();
    }
    return cachedLocalAddress;
}

bool WindowsBluetoothManager::isBluetoothAvailable() const
{
    if (!cacheValid) {
        updateLocalDeviceCache();
    }
    return cachedAvailability;
}

QList<BluetoothDeviceData> WindowsBluetoothManager::getConnectedDevices() const
{
    QList<BluetoothDeviceData> devices;
    
    // Параметры поиска только для подключенных устройств
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams;
    ZeroMemory(&searchParams, sizeof(searchParams));
    searchParams.dwSize = sizeof(searchParams);
    searchParams.fReturnAuthenticated = FALSE;
    searchParams.fReturnRemembered = FALSE;
    searchParams.fReturnConnected = TRUE;  // Только подключенные
    searchParams.fReturnUnknown = FALSE;
    searchParams.fIssueInquiry = FALSE;    // Без активного сканирования
    searchParams.cTimeoutMultiplier = 1;
    
    BLUETOOTH_DEVICE_INFO deviceInfo;
    ZeroMemory(&deviceInfo, sizeof(deviceInfo));
    deviceInfo.dwSize = sizeof(deviceInfo);
    
    HBLUETOOTH_DEVICE_FIND hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    
    if (hFind == NULL) {
        return devices;
    }
    
    do {
        BluetoothDeviceData device;
        device.name = QString::fromWCharArray(deviceInfo.szName);
        device.address = formatBluetoothAddress(deviceInfo.Address);
        device.deviceClass = deviceInfo.ulClassofDevice;
        device.isConnected = deviceInfo.fConnected != 0;
        device.isPaired = deviceInfo.fAuthenticated != 0;
        device.isRemembered = deviceInfo.fRemembered != 0;
        device.lastSeen = deviceInfo.stLastSeen;
        device.lastUsed = deviceInfo.stLastUsed;
        
        devices.append(device);
        
        ZeroMemory(&deviceInfo, sizeof(deviceInfo));
        deviceInfo.dwSize = sizeof(deviceInfo);
        
    } while (BluetoothFindNextDevice(hFind, &deviceInfo));
    
    BluetoothFindDeviceClose(hFind);
    
    return devices;
}

void WindowsBluetoothManager::onDeviceFound(const BluetoothDeviceData &device)
{
    emit logMessage("╔═══════════════════════════════════════╗");
    emit logMessage("║     ✓ НАЙДЕНО УСТРОЙСТВО!             ║");
    emit logMessage("╚═══════════════════════════════════════╝");
    emit logMessage("  Имя: " + (device.name.isEmpty() ? "(Без имени)" : device.name));
    emit logMessage("  MAC: " + device.address);
    emit logMessage("  Тип: " + device.getDeviceTypeString());
    
    QString status;
    if (device.isConnected) status += "🔵 Подключено ";
    if (device.isPaired) status += "🔗 Сопряжено ";
    if (device.isRemembered) status += "💾 Запомнено ";
    if (status.isEmpty()) status = "Обнаружено";
    
    emit logMessage("  Статус: " + status);
    emit logMessage("─────────────────────────────────────────");
    emit logMessage("");
    
    emit deviceDiscovered(device);
}

void WindowsBluetoothManager::onScanCompleted(int deviceCount)
{
    scanning = false;
    emit discoveryFinished(deviceCount);
}

void WindowsBluetoothManager::onScanError(const QString &error)
{
    scanning = false;
    emit discoveryError(error);
}

void WindowsBluetoothManager::onScanLogMessage(const QString &message)
{
    emit logMessage(message);
}

void WindowsBluetoothManager::updateLocalDeviceCache() const
{
    cachedLocalName.clear();
    cachedLocalAddress.clear();
    cachedAvailability = false;
    
    // Пытаемся найти первый локальный радио-адаптер
    BLUETOOTH_FIND_RADIO_PARAMS radioParams;
    radioParams.dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS);
    
    HANDLE hRadio;
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&radioParams, &hRadio);
    
    if (hFind == NULL) {
        cacheValid = true;
        return;
    }
    
    // Получаем информацию о радио
    BLUETOOTH_RADIO_INFO radioInfo;
    radioInfo.dwSize = sizeof(BLUETOOTH_RADIO_INFO);
    
    if (BluetoothGetRadioInfo(hRadio, &radioInfo) == ERROR_SUCCESS) {
        cachedLocalName = QString::fromWCharArray(radioInfo.szName);
        cachedLocalAddress = formatBluetoothAddress(radioInfo.address);
        cachedAvailability = true;
    }
    
    CloseHandle(hRadio);
    BluetoothFindRadioClose(hFind);
    
    cacheValid = true;
}

QString WindowsBluetoothManager::formatBluetoothAddress(const BLUETOOTH_ADDRESS &btAddr) const
{
    return QString("%1:%2:%3:%4:%5:%6")
        .arg(btAddr.rgBytes[5], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[4], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[3], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[2], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[1], 2, 16, QChar('0'))
        .arg(btAddr.rgBytes[0], 2, 16, QChar('0')).toUpper();
}

