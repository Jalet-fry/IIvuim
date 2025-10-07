#include "pciworker.h"
#include <QDebug>
#include <QString>

PCIWorker::PCIWorker(QObject *parent) : QObject(parent)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PCIWorker::performScan);
    isScanning = false;
}

PCIWorker::~PCIWorker()
{
    stopScan();
    disablePortAccess();
}

void PCIWorker::startScan()
{
    qDebug() << "PCIWorker: Starting PCI scan";
    
    if (!enablePortAccess()) {
        emit errorOccurred("Не удалось получить доступ к портам ввода-вывода. Требуются права администратора.");
        return;
    }
    
    devices.clear();
    isScanning = true;
    
    // Выполняем сканирование в отдельном потоке через таймер
    timer->start(100); // Небольшая задержка для UI
}

void PCIWorker::stopScan()
{
    if (timer && timer->isActive()) {
        timer->stop();
    }
    isScanning = false;
    qDebug() << "PCIWorker: Scan stopped";
}

void PCIWorker::performScan()
{
    if (!isScanning) {
        writeToLogFile("PCIWorker: Scan not active, returning");
        return;
    }
    
    timer->stop();
    
    writeToLogFile("=== PCI WORKER PERFORMING SCAN ===");
    writeToLogFile("PCIWorker: Performing PCI bus scan");
    
    bool success = scanPCIBus();
    
    isScanning = false;
    
    if (success) {
        emit scanCompleted(devices.count());
        writeToLogFile(QString("PCIWorker: Scan completed successfully, found %1 devices").arg(devices.count()));
    } else {
        writeToLogFile("PCIWorker: Scan failed");
        emit errorOccurred("Ошибка при сканировании PCI шины. Проверьте права доступа.");
    }
}

bool PCIWorker::scanPCIBus()
{
    PCI_CONFIG_ADDRESS cfg;
    unsigned long val = 0;
    int deviceCount = 0;
    
    qDebug() << "=== STARTING PCI BUS SCAN ===";
    qDebug() << "PCIWorker: Starting PCI bus scan";
    
    // Включаем доступ к портам
    qDebug() << "PCIWorker: Attempting to enable port access...";
    if (!enablePortAccess()) {
        qDebug() << "PCIWorker: CRITICAL ERROR - Failed to enable port access";
        qDebug() << "PCIWorker: Cannot proceed with PCI scan";
        emit errorOccurred("Не удалось получить доступ к портам ввода-вывода. Запустите приложение от имени администратора.");
        return false;
    }
    
    qDebug() << "PCIWorker: Port access enabled successfully, starting scan";
    
    int totalChecked = 0;
    int devicesFound = 0;
    
    for (unsigned short bus = 0; bus < PCI_MAX_BUSES; bus++) {
        for (unsigned short dev = 0; dev < PCI_MAX_DEVICES; dev++) {
            for (unsigned short func = 0; func < PCI_MAX_FUNCTIONS; func++) {
                totalChecked++;
                
                if (totalChecked % 50 == 0) {
                    qDebug() << "PCI Worker: Checked" << totalChecked << "slots, found" << devicesFound << "devices";
                }
                
                // Настраиваем адрес конфигурации
                cfg.u1.s1.Enable = 1;
                cfg.u1.s1.BusNumber = bus;
                cfg.u1.s1.DeviceNumber = dev;
                cfg.u1.s1.FunctionNumber = func;
                cfg.u1.s1.RegisterNumber = 0;
                
                qDebug() << QString("Checking Bus:%1 Dev:%2 Func:%3").arg(bus).arg(dev).arg(func);
                qDebug() << "Writing address:" << QString("0x%1").arg(cfg.u1.Value, 8, 16, QChar('0'));
                
                // Читаем Device ID и Vendor ID
                outpd(PCI_ADDRESS_PORT, cfg.u1.Value);
                val = inpd(PCI_DATA_PORT);
                
                qDebug() << "Read value:" << QString("0x%1").arg(val, 8, 16, QChar('0'));
                
                // Проверяем, есть ли устройство (0xFFFF означает отсутствие устройства)
                if (val == 0xFFFFFFFF) {
                    qDebug() << "No device (0xFFFFFFFF)";
                    if (func == 0) break; // Если функция 0 не существует, остальные тоже не существуют
                    continue;
                } else if (val == 0x00000000) {
                    qDebug() << "No device (0x00000000)";
                    continue;
                }
                
                qDebug() << "*** DEVICE FOUND! *** Reading detailed information...";
                
                // Читаем информацию об устройстве
                PCIDevice device = readPCIDevice(bus, dev, func);
                
                if (device.DeviceID != 0 && device.VendorID != 0) {
                    devices.append(device);
                    emit deviceFound(device);
                    deviceCount++;
                    devicesFound++;
                    
                    qDebug() << "*** DEVICE ADDED ***" 
                             << QString("Bus:%1 Dev:%2 Func:%3")
                                .arg(bus).arg(dev).arg(func)
                             << QString("VID:0x%1 DID:0x%2")
                                .arg(device.VendorID, 4, 16, QChar('0'))
                                .arg(device.DeviceID, 4, 16, QChar('0'));
                }
            }
        }
    }
    
    qDebug() << "=== SCAN COMPLETED ===";
    qDebug() << "Total slots checked:" << totalChecked;
    qDebug() << "Devices found:" << devicesFound;
    
    return true;
}

PCIDevice PCIWorker::readPCIDevice(unsigned short bus, unsigned short device, unsigned short function)
{
    PCIDevice pciDevice;
    memset(&pciDevice, 0, sizeof(PCIDevice));
    
    pciDevice.Bus = bus;
    pciDevice.Device = device;
    pciDevice.Function = function;
    
    // Читаем Device ID и Vendor ID (регистр 0)
    unsigned int reg0 = readPCIRegister(bus, device, function, 0);
    pciDevice.DeviceID = reg0 >> 16;
    pciDevice.VendorID = reg0 & 0xFFFF;
    
    // Читаем Revision ID и Class Code (регистр 2)
    unsigned int reg2 = readPCIRegister(bus, device, function, 2);
    pciDevice.Revision = reg2 & 0xFF;
    pciDevice.ClassCode = (reg2 >> 8) & 0xFF;
    pciDevice.SubClass = (reg2 >> 16) & 0xFF;
    pciDevice.ProgIF = (reg2 >> 24) & 0xFF;
    
    // Получаем описание устройства
    pciDevice.DeviceName = getDeviceDescription(pciDevice.ClassCode, pciDevice.SubClass, pciDevice.ProgIF);
    
    return pciDevice;
}

unsigned int PCIWorker::readPCIRegister(unsigned short bus, unsigned short device, unsigned short function, unsigned char registerOffset)
{
    PCI_CONFIG_ADDRESS cfg;
    
    cfg.u1.s1.Enable = 1;
    cfg.u1.s1.BusNumber = bus;
    cfg.u1.s1.DeviceNumber = device;
    cfg.u1.s1.FunctionNumber = function;
    cfg.u1.s1.RegisterNumber = registerOffset >> 2; // Регистры выравнены по 4 байта
    
    outpd(PCI_ADDRESS_PORT, cfg.u1.Value);
    return inpd(PCI_DATA_PORT);
}

QString PCIWorker::getDeviceDescription(unsigned int classCode, unsigned int subClass, unsigned int progIF)
{
    for (int i = 0; i < PciClassCodeTableLen; i++) {
        if (PciClassCodeTable[i].BaseClass == classCode && 
            PciClassCodeTable[i].SubClass == subClass &&
            PciClassCodeTable[i].ProgIf == progIF) {
            
            QString result = QString("%1 %2")
                .arg(PciClassCodeTable[i].BaseDesc)
                .arg(PciClassCodeTable[i].SubDesc);
            return result;
        }
    }
    
    return QString("Unknown Device (Class: %1, SubClass: %2, ProgIF: %3)")
        .arg(classCode, 2, 16, QChar('0'))
        .arg(subClass, 2, 16, QChar('0'))
        .arg(progIF, 2, 16, QChar('0'));
}
