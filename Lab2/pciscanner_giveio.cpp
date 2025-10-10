#include "pciscanner_giveio.h"
#include <QThread>

PciScannerGiveIO::PciScannerGiveIO(QObject *parent)
    : QObject(parent),
      giveioHandle(INVALID_HANDLE_VALUE),
      giveioInitialized(false)
{
}

PciScannerGiveIO::~PciScannerGiveIO()
{
    giveioShutdown();
}

bool PciScannerGiveIO::giveioInitialize()
{
    emit logMessage("Попытка инициализации GiveIO...");

    giveioHandle = CreateFileA("\\\\.\\giveio",
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if (giveioHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        emit logMessage(QString("Ошибка открытия GiveIO: %1").arg(error), true);
        emit logMessage("Драйвер GiveIO не установлен или не запущен", true);
        return false;
    }

    giveioInitialized = true;
    emit logMessage("GiveIO успешно инициализирован");
    return true;
}

void PciScannerGiveIO::giveioShutdown()
{
    if (giveioHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(giveioHandle);
        giveioHandle = INVALID_HANDLE_VALUE;
    }
    giveioInitialized = false;
}

void PciScannerGiveIO::giveioOutPortDword(WORD port, DWORD value)
{
    if (!giveioInitialized) return;

    // Inline assembly для MSVC
    #if defined(_MSC_VER)
        __asm {
            mov dx, port
            mov eax, value
            out dx, eax
        }
    #else
        // Для MinGW и других компиляторов
        asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
    #endif
}

DWORD PciScannerGiveIO::giveioInPortDword(WORD port)
{
    if (!giveioInitialized) return 0xFFFFFFFF;

    #if defined(_MSC_VER)
        DWORD result;
        __asm {
            mov dx, port
            in eax, dx
            mov result, eax
        }
        return result;
    #else
        DWORD result;
        asm volatile ("inl %1, %0" : "=a"(result) : "Nd"(port));
        return result;
    #endif
}

bool PciScannerGiveIO::writePortDword(WORD port, DWORD value)
{
    if (!giveioInitialized) {
        emit logMessage("GiveIO не инициализирован", true);
        return false;
    }

    try {
        giveioOutPortDword(port, value);
        return true;
    } catch (...) {
        emit logMessage(QString("Ошибка записи в порт 0x%1").arg(port, 4, 16, QChar('0')), true);
        return false;
    }
}

DWORD PciScannerGiveIO::readPortDword(WORD port)
{
    if (!giveioInitialized) {
        emit logMessage("GiveIO не инициализирован", true);
        return 0xFFFFFFFF;
    }

    try {
        return giveioInPortDword(port);
    } catch (...) {
        emit logMessage(QString("Ошибка чтения из порта 0x%1").arg(port, 4, 16, QChar('0')), true);
        return 0xFFFFFFFF;
    }
}

DWORD PciScannerGiveIO::readPCIConfigDword(quint8 bus, quint8 device, quint8 function, quint8 offset)
{
    // Формируем адрес Configuration Space для указанного offset
    DWORD address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
    
    // Записываем адрес в порт 0xCF8
    if (!writePortDword(0x0CF8, address)) {
        return 0xFFFFFFFF;
    }
    
    // Читаем данные из порта 0xCFC
    return readPortDword(0x0CFC);
}

QString PciScannerGiveIO::getClassString(quint8 classCode) const
{
    switch (classCode) {
        case 0x00: return "Pre-2.0 PCI Specification Device";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Device";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Simple Communications Controller";
        case 0x08: return "Base Systems Peripheral";
        case 0x09: return "Input Device";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite Communication Controller";
        case 0x10: return "Encryption/Decryption Controller";
        case 0x11: return "Data Acquisition Controller";
        case 0xFF: return "Unknown Device";
        default: return QString("Class 0x%1").arg(classCode, 2, 16, QChar('0')).toUpper();
    }
}

QString PciScannerGiveIO::getSubClassString(quint8 classCode, quint8 subClass) const
{
    switch (classCode) {
        case 0x01: // Mass Storage
            switch (subClass) {
                case 0x00: return "SCSI";
                case 0x01: return "IDE";
                case 0x02: return "Floppy";
                case 0x03: return "IPI";
                case 0x04: return "RAID";
                case 0x05: return "ATA Controller";
                case 0x06: return "Serial ATA Controller";
                case 0x07: return "Serial Attached SCSI Controller";
                case 0x08: return "Non-Volatile Memory (NVMe)";
                case 0x80: return "Other";
                default: return QString("Storage 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
            }
        case 0x02: // Network
            switch (subClass) {
                case 0x00: return "Ethernet/WiFi";
                case 0x01: return "Token Ring";
                case 0x02: return "FDDI";
                case 0x03: return "ATM";
                case 0x80: return "Other";
                default: return QString("Network 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
            }
        case 0x03: // Display
            switch (subClass) {
                case 0x00: return "VGA Compatible";
                case 0x01: return "XGA";
                case 0x02: return "3D Graphics";
                case 0x80: return "Other";
                default: return QString("Display 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
            }
        case 0x04: // Multimedia
            switch (subClass) {
                case 0x00: return "Video Controller";
                case 0x01: return "Audio Controller";
                case 0x02: return "Telephony Controller";
                case 0x03: return "Audio Device";
                case 0x80: return "Other";
                default: return QString("Multimedia 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
            }
        case 0x05: // Memory
            switch (subClass) {
                case 0x00: return "RAM";
                case 0x01: return "Flash";
                case 0x80: return "Other";
                default: return QString("Memory 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
            }
        case 0x06: // Bridge
            switch (subClass) {
                case 0x00: return "Host/PCI";
                case 0x01: return "PCI/ISA";
                case 0x02: return "PCI/EISA";
                case 0x03: return "PCI/Micro Channel";
                case 0x04: return "PCI/PCI";
                case 0x05: return "PCI/PCMCIA";
                case 0x06: return "PCI/NuBus";
                case 0x07: return "PCI/CardBus";
                case 0x08: return "PCI/RACEway";
                case 0x09: return "PCI/PCI";
                case 0x0A: return "PCI/InfiniBand";
                case 0x80: return "Other";
                default: return QString("Bridge 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
            }
        case 0x0C: // Serial Bus
            switch (subClass) {
                case 0x00: return "Firewire (IEEE 1394)";
                case 0x01: return "ACCESS.bus";
                case 0x02: return "SSA (Serial Storage Archetecture)";
                case 0x03: return "USB Controller";
                case 0x04: return "Fibre Channel";
                case 0x05: return "SMbus";
                case 0x06: return "InfiniBand";
                case 0x07: return "IPMI (SMIC)";
                case 0x08: return "SERCOS";
                case 0x80: return "Other";
                default: return QString("Serial 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
            }
        default:
            return QString("SubClass 0x%1").arg(subClass, 2, 16, QChar('0')).toUpper();
    }
}

QString PciScannerGiveIO::getProgIFString(quint8 classCode, quint8 subClass, quint8 progIF) const
{
    // USB Programming Interface
    if (classCode == 0x0C && subClass == 0x03) {
        switch (progIF) {
            case 0x00: return "USB 1.1 UHCI";
            case 0x10: return "USB 1.1 OHCI";
            case 0x20: return "USB 2.0 EHCI";
            case 0x30: return "USB 3.0 XHCI";
            case 0xFE: return "USB Device";
            default: return QString("USB 0x%1").arg(progIF, 2, 16, QChar('0')).toUpper();
        }
    }
    
    // SATA Programming Interface
    if (classCode == 0x01 && subClass == 0x06) {
        switch (progIF) {
            case 0x00: return "Vendor Specific";
            case 0x01: return "AHCI 1.0";
            case 0x02: return "Serial Storage Bus";
            default: return QString("SATA 0x%1").arg(progIF, 2, 16, QChar('0')).toUpper();
        }
    }
    
    // Display VGA Programming Interface
    if (classCode == 0x03 && subClass == 0x00) {
        switch (progIF) {
            case 0x00: return "VGA";
            case 0x01: return "8514";
            default: return QString("VGA 0x%1").arg(progIF, 2, 16, QChar('0')).toUpper();
        }
    }
    
    return QString("0x%1").arg(progIF, 2, 16, QChar('0')).toUpper();
}

QString PciScannerGiveIO::getVendorName(quint16 vendorID) const
{
    static QMap<quint16, QString> vendorMap;
    if (vendorMap.isEmpty()) {
        vendorMap[0x8086] = "Intel";
        vendorMap[0x10DE] = "NVIDIA";
        vendorMap[0x1002] = "AMD";
        vendorMap[0x1B36] = "Red Hat";
        vendorMap[0x80EE] = "InnoTek";
        vendorMap[0x10EC] = "Realtek";
        vendorMap[0x14E4] = "Broadcom";
        vendorMap[0x106B] = "Apple";
        vendorMap[0x1106] = "VIA";
        vendorMap[0x1234] = "Technical";
    }

    return vendorMap.value(vendorID, QString("Vendor 0x%1").arg(vendorID, 4, 16, QChar('0')));
}

QString PciScannerGiveIO::getDeviceName(quint16 vendorID, quint16 deviceID) const
{
    if (vendorID == 0x8086) {
        switch (deviceID) {
            case 0x1237: return "440FX PCIset";
            case 0x100E: return "82540EM Ethernet";
            case 0x2415: return "AC'97 Audio";
            case 0x265C: return "SATA Controller";
            case 0x7000: return "PIIX3 ISA Bridge";
            case 0x7111: return "PIIX4 IDE";
            case 0x7113: return "PIIX4 ACPI";
            default: return "Intel Device";
        }
    } else if (vendorID == 0x10DE) {
        return "NVIDIA Graphics";
    } else if (vendorID == 0x1002) {
        return "AMD Graphics";
    } else if (vendorID == 0x80EE) {
        switch (deviceID) {
            case 0xBEEF: return "VirtualBox Graphics";
            case 0xCAFE: return "VirtualBox Service";
            default: return "VirtualBox Device";
        }
    }

    return QString("Device 0x%1").arg(deviceID, 4, 16, QChar('0'));
}

QList<PCI_Device_GiveIO> PciScannerGiveIO::devices() const
{
    return m_devices;
}

bool PciScannerGiveIO::isRunningAsAdmin() const
{
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;

    // Пытаемся открыть токен процесса
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }

    // Для Windows XP используем старый способ проверки прав
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = NULL;

    // Создаем SID для группы администраторов
    if (!AllocateAndInitializeSid(&NtAuthority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &AdministratorsGroup)) {
        if (hToken) CloseHandle(hToken);
        return false;
    }

    // Проверяем, есть ли у токена группа администраторов
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
        isAdmin = FALSE;
    }

    // Освобождаем ресурсы
    if (AdministratorsGroup) FreeSid(AdministratorsGroup);
    if (hToken) CloseHandle(hToken);

    return (isAdmin == TRUE);
}

bool PciScannerGiveIO::testAccess()
{
    emit logMessage("Проверка доступа к PCI через GiveIO...");

    if (!isRunningAsAdmin()) {
        emit logMessage("ОШИБКА: Требуются права администратора", true);
        return false;
    }

    if (!giveioInitialize()) {
        emit logMessage("ОШИБКА: Не удалось инициализировать GiveIO", true);
        return false;
    }

    // Тестирование доступа к портам
    emit logMessage("=== ТЕСТ ДОСТУПА К ПОРТАМ ===");

    // Тест порта 0x80 (обычно свободен)
    emit logMessage("Тест порта 0x0080...");
    if (!writePortDword(0x0080, 0x12345678)) {
        emit logMessage("ОШИБКА: Не удалось записать в порт 0x0080", true);
        giveioShutdown();
        return false;
    }

    DWORD testValue = readPortDword(0x0080);
    emit logMessage(QString("Порт 0x0080: ЧТЕНИЕ УСПЕШНО = 0x%1").arg(testValue, 8, 16, QChar('0')));

    // Тест PCI Configuration Space Access
    emit logMessage("Тест PCI Configuration Space...");

    // Записываем адрес для чтения Vendor ID устройства 0:0:0
    if (!writePortDword(0x0CF8, 0x80000000)) {
        emit logMessage("ОШИБКА: Не удалось записать в порт 0x0CF8", true);
        giveioShutdown();
        return false;
    }

    // Читаем Vendor ID и Device ID
    DWORD vendorDevice = readPortDword(0x0CFC);
    DWORD vendorID = vendorDevice & 0xFFFF;
    DWORD deviceID = (vendorDevice >> 16) & 0xFFFF;

    emit logMessage(QString("Порт 0x0CFC: ЧТЕНИЕ УСПЕШНО = 0x%1").arg(vendorDevice, 8, 16, QChar('0')));
    emit logMessage(QString("VendorID: 0x%1, DeviceID: 0x%2").arg(vendorID, 4, 16, QChar('0')).arg(deviceID, 4, 16, QChar('0')));

    if (vendorID == 0xFFFF || vendorID == 0x0000) {
        emit logMessage("ПРЕДУПРЕЖДЕНИЕ: VendorID невалиден, но доступ к портам работает", true);
    } else {
        emit logMessage("Доступ к PCI Configuration Space подтвержден");
    }

    emit logMessage("GiveIO готов к работе");
    giveioShutdown();
    return true;
}

bool PciScannerGiveIO::scanInternal()
{
    m_devices.clear();
    emit progress(0, 255);

    int foundDevices = 0;

    for (DWORD bus = 0; bus < 256; bus++) {
        for (DWORD device = 0; device < 32; device++) {
            for (DWORD function = 0; function < 8; function++) {
                // Формируем адрес Configuration Space
                DWORD address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8);

                // Записываем адрес в порт 0xCF8
                if (!writePortDword(0x0CF8, address)) {
                    continue;
                }

                // Читаем VendorID и DeviceID из порта 0xCFC
                DWORD vendorDevice = readPortDword(0x0CFC);
                DWORD vendorID = vendorDevice & 0xFFFF;
                DWORD deviceID = (vendorDevice >> 16) & 0xFFFF;

                if (vendorID != 0xFFFF && vendorID != 0x0000) {
                    PCI_Device_GiveIO dev;
                    dev.vendorID = static_cast<quint16>(vendorID);
                    dev.deviceID = static_cast<quint16>(deviceID);
                    dev.bus = static_cast<quint8>(bus);
                    dev.device = static_cast<quint8>(device);
                    dev.function = static_cast<quint8>(function);
                    dev.vendorName = getVendorName(dev.vendorID);
                    dev.deviceName = getDeviceName(dev.vendorID, dev.deviceID);

                    // Читаем регистр 0x08: Revision ID, Prog IF, SubClass, Class Code
                    DWORD classReg = readPCIConfigDword(bus, device, function, 0x08);
                    dev.revisionID = classReg & 0xFF;           // Биты 7-0
                    dev.progIF = (classReg >> 8) & 0xFF;        // Биты 15-8
                    dev.subClass = (classReg >> 16) & 0xFF;     // Биты 23-16
                    dev.classCode = (classReg >> 24) & 0xFF;    // Биты 31-24
                    
                    // Читаем регистр 0x0C: Cache Line, Latency Timer, Header Type, BIST
                    DWORD headerReg = readPCIConfigDword(bus, device, function, 0x0C);
                    dev.headerType = (headerReg >> 16) & 0xFF;  // Байт offset 0x0E
                    
                    // Читаем регистр 0x2C: Subsystem Vendor ID и Subsystem ID (только для Header Type 0)
                    if ((dev.headerType & 0x7F) == 0x00) {
                        DWORD subsysReg = readPCIConfigDword(bus, device, function, 0x2C);
                        dev.subsysVendorID = subsysReg & 0xFFFF;
                        dev.subsysID = (subsysReg >> 16) & 0xFFFF;
                    } else {
                        dev.subsysVendorID = 0;
                        dev.subsysID = 0;
                    }

                    m_devices.append(dev);
                    emit deviceFound(dev);
                    foundDevices++;

                    emit logMessage(QString("Найдено: Bus=0x%1 Dev=0x%2 Func=0x%3 VID=0x%4 DID=0x%5 - %6 [%7 / %8]")
                              .arg(bus, 2, 16, QChar('0'))
                              .arg(device, 2, 16, QChar('0'))
                              .arg(function, 1, 16, QChar('0'))
                              .arg(vendorID, 4, 16, QChar('0'))
                              .arg(deviceID, 4, 16, QChar('0'))
                              .arg(dev.vendorName)
                              .arg(getClassString(dev.classCode))
                              .arg(getSubClassString(dev.classCode, dev.subClass)));
                }
            }
        }

        if (bus % 32 == 0) {
            emit logMessage(QString("Прогресс: Bus 0x%1, найдено: %2 устройств")
                      .arg(bus, 2, 16, QChar('0'))
                      .arg(foundDevices));
            QThread::yieldCurrentThread();
        }

        emit progress(static_cast<int>(bus), 255);
    }

    emit progress(255, 255);
    emit finished(foundDevices > 0);
    return (foundDevices > 0);
}

bool PciScannerGiveIO::scan()
{
    emit logMessage("=== НАЧАЛО СКАНИРОВАНИЯ PCI ЧЕРЕЗ GiveIO ===");

    if (!giveioInitialize()) {
        emit logMessage("ОШИБКА: Не удалось инициализировать GiveIO", true);
        return false;
    }

    bool result = scanInternal();

    giveioShutdown();

    emit logMessage(QString("Сканирование завершено. Найдено устройств: %1").arg(m_devices.size()));
    return result;
}

