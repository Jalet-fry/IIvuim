#include "pciwidget_giveio.h"
#include <QApplication>
#include <QTime>
#include <QMap>
#include <windows.h>

PCIWidget_GiveIO::PCIWidget_GiveIO(QWidget *parent) :
    QWidget(parent),
    tableWidget(nullptr),
    scanButton(nullptr),
    clearButton(nullptr),
    progressBar(nullptr),
    logTextEdit(nullptr),
    mainSplitter(nullptr),
    jakeAnimationLabel(nullptr),
    jakeAnimation(nullptr),
    giveioHandle(INVALID_HANDLE_VALUE),
    giveioInitialized(false)
{
    initializeUI();
    
    if (!isRunningAsAdmin()) {
        logMessage("ПРЕДУПРЕЖДЕНИЕ: Программа запущена без прав администратора", true);
        logMessage("Прямой доступ к PCI может быть ограничен", true);
    } else {
        logMessage("Программа запущена с правами администратора");
    }
}

PCIWidget_GiveIO::~PCIWidget_GiveIO()
{
    shutdownGiveIO();
    
    if (jakeAnimation) {
        jakeAnimation->stop();
        delete jakeAnimation;
    }
}

void PCIWidget_GiveIO::initializeUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Верхняя панель с кнопками и анимацией
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);

    // Jake анимация (слева)
    jakeAnimationLabel = new QLabel(this);
    jakeAnimationLabel->setFixedSize(120, 120);
    jakeAnimationLabel->setAlignment(Qt::AlignCenter);
    jakeAnimationLabel->setStyleSheet("background-color: white; border: 2px solid #4A90E2; border-radius: 8px;");
    jakeAnimation = new QMovie(":/Animation/jake with cicrle.gif");
    jakeAnimationLabel->setMovie(jakeAnimation);
    jakeAnimationLabel->setScaledContents(true);
    jakeAnimationLabel->setVisible(false);
    topLayout->addWidget(jakeAnimationLabel);

    // Кнопки (справа от анимации)
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(8);
    
    scanButton = new QPushButton("🔍 Сканировать PCI устройства", this);
    scanButton->setMinimumHeight(50);
    scanButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4A90E2;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    padding: 10px 20px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #357ABD;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2E6BA8;"
        "}"
    );
    
    clearButton = new QPushButton("🗑️ Очистить лог", this);
    clearButton->setMinimumHeight(40);
    clearButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #E8F4F8;"
        "    color: #4A90E2;"
        "    border: 2px solid #4A90E2;"
        "    border-radius: 8px;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    padding: 8px 20px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #D0E9F5;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #B8DDF0;"
        "}"
    );
    
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    
    topLayout->addLayout(buttonLayout);
    topLayout->addStretch();
    
    mainLayout->addLayout(topLayout);

    // Прогресс бар
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setMinimumHeight(25);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #4A90E2;"
        "    border-radius: 8px;"
        "    text-align: center;"
        "    background-color: white;"
        "    color: #333;"
        "    font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4A90E2, stop:1 #67B8F7);"
        "    border-radius: 6px;"
        "}"
    );
    mainLayout->addWidget(progressBar);

    // Сплиттер для таблицы и лога
    mainSplitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(mainSplitter);

    // Таблица устройств
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(11);
    QStringList headers;
    headers << "Bus" << "Device" << "Function" 
            << "VendorID" << "DeviceID" 
            << "Vendor" << "Device"
            << "Class Code" << "SubClass"
            << "Prog IF" << "Header";
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setStyleSheet(
        "QTableWidget {"
        "    background-color: white;"
        "    alternate-background-color: #F5F9FC;"
        "    border: 2px solid #4A90E2;"
        "    border-radius: 8px;"
        "    gridline-color: #E0E0E0;"
        "}"
        "QTableWidget::item {"
        "    padding: 5px;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #4A90E2;"
        "    color: white;"
        "}"
        "QHeaderView::section {"
        "    background-color: #4A90E2;"
        "    color: white;"
        "    padding: 8px;"
        "    border: none;"
        "    font-weight: bold;"
        "}"
    );
    mainSplitter->addWidget(tableWidget);

    // Лог
    logTextEdit = new QTextEdit(this);
    logTextEdit->setMaximumHeight(200);
    logTextEdit->setReadOnly(true);
    logTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: white;"
        "    border: 2px solid #4A90E2;"
        "    border-radius: 8px;"
        "    padding: 8px;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 11px;"
        "}"
    );
    mainSplitter->addWidget(logTextEdit);

    // Общий стиль для виджета
    setStyleSheet(
        "QWidget {"
        "    background-color: #F8FBFD;"
        "}"
    );

    // Подключаем сигналы
    connect(scanButton, SIGNAL(clicked()), this, SLOT(scanPCI_devices()));
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clearLog()));
    connect(tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onDeviceSelected()));

    resize(1000, 700);
    setWindowTitle("PCI Devices Scanner - GiveIO");
}

void PCIWidget_GiveIO::logMessage(const QString &message, bool isError)
{
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp).arg(message);

    if (isError) {
        logTextEdit->setTextColor(Qt::red);
    } else {
        logTextEdit->setTextColor(Qt::blue);
    }

    logTextEdit->append(logEntry);
    logTextEdit->setTextColor(Qt::black);

    // Обеспечиваем обновление интерфейса
    QApplication::processEvents();
}

void PCIWidget_GiveIO::clearLog()
{
    logTextEdit->clear();
}

void PCIWidget_GiveIO::onDeviceSelected()
{
    QList<QTableWidgetItem*> selectedItems = tableWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = tableWidget->currentRow();
    if (row < 0 || row >= pciDevices.size()) {
        return;
    }
    
    const PCI_Device& dev = pciDevices[row];
    
    QString details = QString(
        "═══════════════════════════════════════════════════════════\n"
        "  ПОЛНОЕ КОНФИГУРАЦИОННОЕ ПРОСТРАНСТВО PCI УСТРОЙСТВА\n"
        "═══════════════════════════════════════════════════════════\n\n"
        "📍 ФИЗИЧЕСКОЕ РАСПОЛОЖЕНИЕ:\n"
        "   Bus: %1 (0x%2)  |  Device: %3 (0x%4)  |  Function: %5 (0x%6)\n\n"
        "───────────────────────────────────────────────────────────\n"
        "🔑 ИДЕНТИФИКАЦИЯ УСТРОЙСТВА (Offset 0x00):\n"
        "   Vendor ID:    %7 (0x%8)\n"
        "   Device ID:    %9 (0x%10)\n"
        "   Vendor Name:  %11\n"
        "   Device Name:  %12\n\n"
        "───────────────────────────────────────────────────────────\n"
        "📊 КЛАССИФИКАЦИЯ УСТРОЙСТВА (Offset 0x08):\n"
        "   Class Code:   0x%13 - %14\n"
        "   SubClass:     0x%15 - %16\n"
        "   Prog IF:      %17\n"
        "   Revision ID:  0x%18\n\n"
        "───────────────────────────────────────────────────────────\n"
        "🔧 ЗАГОЛОВОК КОНФИГУРАЦИИ (Offset 0x0C-0x0F):\n"
        "   Header Type:  0x%19 %20\n\n"
    ).arg(dev.bus).arg(dev.bus, 2, 16, QChar('0')).toUpper()
     .arg(dev.device).arg(dev.device, 2, 16, QChar('0')).toUpper()
     .arg(dev.function).arg(dev.function, 1, 16, QChar('0')).toUpper()
     .arg(dev.vendorID).arg(dev.vendorID, 4, 16, QChar('0')).toUpper()
     .arg(dev.deviceID).arg(dev.deviceID, 4, 16, QChar('0')).toUpper()
     .arg(dev.vendorName)
     .arg(dev.deviceName)
     .arg(dev.classCode, 2, 16, QChar('0')).toUpper()
     .arg(getClassString(dev.classCode))
     .arg(dev.subClass, 2, 16, QChar('0')).toUpper()
     .arg(getSubClassString(dev.classCode, dev.subClass))
     .arg(getProgIFString(dev.classCode, dev.subClass, dev.progIF))
     .arg(dev.revisionID, 2, 16, QChar('0')).toUpper()
     .arg(dev.headerType, 2, 16, QChar('0')).toUpper()
     .arg((dev.headerType & 0x7F) == 0x00 ? "(Standard PCI Device)" : 
          (dev.headerType & 0x7F) == 0x01 ? "(PCI-to-PCI Bridge)" :
          (dev.headerType & 0x7F) == 0x02 ? "(CardBus Bridge)" : "(Unknown)");
    
    // Добавляем информацию о подсистеме для обычных устройств
    if ((dev.headerType & 0x7F) == 0x00 && (dev.subsysVendorID != 0 || dev.subsysID != 0)) {
        details += QString(
            "───────────────────────────────────────────────────────────\n"
            "🏷️  ПОДСИСТЕМА (Offset 0x2C):\n"
            "   Subsystem Vendor ID:  0x%1\n"
            "   Subsystem ID:         0x%2\n\n"
        ).arg(dev.subsysVendorID, 4, 16, QChar('0')).toUpper()
         .arg(dev.subsysID, 4, 16, QChar('0')).toUpper();
    }
    
    details += "═══════════════════════════════════════════════════════════\n";
    
    logMessage("═══ Показана детальная информация об устройстве ═══");
    logMessage(QString("Bus %1, Device %2, Function %3: %4 %5")
               .arg(dev.bus).arg(dev.device).arg(dev.function)
               .arg(dev.vendorName).arg(dev.deviceName));
    
    // Можно вывести в отдельное окно или в лог
    QMessageBox::information(this, "Детальная информация о PCI устройстве", details);
}

// GiveIO implementation
bool PCIWidget_GiveIO::giveioInitialize()
{
    logMessage("Попытка инициализации GiveIO...");

    giveioHandle = CreateFileA("\\\\.\\giveio",
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if (giveioHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        logMessage(QString("Ошибка открытия GiveIO: %1").arg(error), true);
        logMessage("Драйвер GiveIO не установлен или не запущен", true);
        return false;
    }

    giveioInitialized = true;
    logMessage("GiveIO успешно инициализирован");
    return true;
}

void PCIWidget_GiveIO::giveioShutdown()
{
    if (giveioHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(giveioHandle);
        giveioHandle = INVALID_HANDLE_VALUE;
    }
    giveioInitialized = false;
}

void PCIWidget_GiveIO::giveioOutPortDword(WORD port, DWORD value)
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

DWORD PCIWidget_GiveIO::giveioInPortDword(WORD port)
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

bool PCIWidget_GiveIO::initializeGiveIO()
{
    logMessage("Инициализация драйвера GiveIO...");
    return giveioInitialize();
}

void PCIWidget_GiveIO::shutdownGiveIO()
{
    giveioShutdown();
    logMessage("GiveIO выгружен");
}

bool PCIWidget_GiveIO::writePortDword(WORD port, DWORD value)
{
    if (!giveioInitialized) {
        logMessage("GiveIO не инициализирован", true);
        return false;
    }

    try {
        giveioOutPortDword(port, value);
        return true;
    } catch (...) {
        logMessage(QString("Ошибка записи в порт 0x%1").arg(port, 4, 16, QChar('0')), true);
        return false;
    }
}

DWORD PCIWidget_GiveIO::readPortDword(WORD port)
{
    if (!giveioInitialized) {
        logMessage("GiveIO не инициализирован", true);
        return 0xFFFFFFFF;
    }

    try {
        return giveioInPortDword(port);
    } catch (...) {
        logMessage(QString("Ошибка чтения из порта 0x%1").arg(port, 4, 16, QChar('0')), true);
        return 0xFFFFFFFF;
    }
}

bool PCIWidget_GiveIO::testPCIAccess()
{
    logMessage("Проверка доступа к PCI через GiveIO...");

    if (!isRunningAsAdmin()) {
        logMessage("ОШИБКА: Требуются права администратора", true);
        return false;
    }

    if (!initializeGiveIO()) {
        logMessage("ОШИБКА: Не удалось инициализировать GiveIO", true);
        return false;
    }

    // Тестирование доступа к портам
    logMessage("=== ТЕСТ ДОСТУПА К ПОРТАМ ===");

    // Тест порта 0x80 (обычно свободен)
    logMessage("Тест порта 0x0080...");
    if (!writePortDword(0x0080, 0x12345678)) {
        logMessage("ОШИБКА: Не удалось записать в порт 0x0080", true);
        return false;
    }

    DWORD testValue = readPortDword(0x0080);
    logMessage(QString("Порт 0x0080: ЧТЕНИЕ УСПЕШНО = 0x%1").arg(testValue, 8, 16, QChar('0')));

    // Тест PCI Configuration Space Access
    logMessage("Тест PCI Configuration Space...");

    // Записываем адрес для чтения Vendor ID устройства 0:0:0
    if (!writePortDword(0x0CF8, 0x80000000)) {
        logMessage("ОШИБКА: Не удалось записать в порт 0x0CF8", true);
        return false;
    }

    // Читаем Vendor ID и Device ID
    DWORD vendorDevice = readPortDword(0x0CFC);
    DWORD vendorID = vendorDevice & 0xFFFF;
    DWORD deviceID = (vendorDevice >> 16) & 0xFFFF;

    logMessage(QString("Порт 0x0CFC: ЧТЕНИЕ УСПЕШНО = 0x%1").arg(vendorDevice, 8, 16, QChar('0')));
    logMessage(QString("VendorID: 0x%1, DeviceID: 0x%2").arg(vendorID, 4, 16, QChar('0')).arg(deviceID, 4, 16, QChar('0')));

    if (vendorID == 0xFFFF || vendorID == 0x0000) {
        logMessage("ПРЕДУПРЕЖДЕНИЕ: VendorID невалиден, но доступ к портам работает", true);
    } else {
        logMessage("Доступ к PCI Configuration Space подтвержден");
    }

    logMessage("GiveIO готов к работе");
    return true;
}

bool PCIWidget_GiveIO::scanPCI_GiveIO()
{
    logMessage("=== НАЧАЛО СКАНИРОВАНИЯ PCI ЧЕРЕЗ GiveIO ===");

    // Показываем анимацию Jake и запускаем её
    jakeAnimationLabel->setVisible(true);
    jakeAnimation->start();

    int foundDevices = 0;
    progressBar->setRange(0, 255);
    progressBar->setValue(0);
    progressBar->setVisible(true);

    logMessage("Сканирование PCI пространства через GiveIO...");

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
                    PCI_Device dev;
                    dev.vendorID = vendorID;
                    dev.deviceID = deviceID;
                    dev.bus = bus;
                    dev.device = device;
                    dev.function = function;
                    dev.vendorName = getVendorName(vendorID);
                    dev.deviceName = getDeviceName(vendorID, deviceID);

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

                    pciDevices.append(dev);
                    addDeviceToTable(dev);
                    foundDevices++;

                    logMessage(QString("Найдено: Bus=0x%1 Dev=0x%2 Func=0x%3 VID=0x%4 DID=0x%5 - %6 [%7 / %8]")
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

        progressBar->setValue(bus);

        if (bus % 32 == 0) {
            logMessage(QString("Прогресс: Bus 0x%1, найдено: %2 устройств")
                      .arg(bus, 2, 16, QChar('0'))
                      .arg(foundDevices));
            QApplication::processEvents();
        }
    }

    progressBar->setValue(255);
    progressBar->setVisible(false);
    
    // Скрываем анимацию Jake и останавливаем её
    jakeAnimation->stop();
    jakeAnimationLabel->setVisible(false);
    
    logMessage(QString("Сканирование завершено. Найдено устройств: %1").arg(foundDevices));

    return foundDevices > 0;
}

DWORD PCIWidget_GiveIO::readPCIConfigDword(quint8 bus, quint8 device, quint8 function, quint8 offset)
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

QString PCIWidget_GiveIO::getClassString(quint8 classCode)
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

QString PCIWidget_GiveIO::getSubClassString(quint8 classCode, quint8 subClass)
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

QString PCIWidget_GiveIO::getProgIFString(quint8 classCode, quint8 subClass, quint8 progIF)
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

QString PCIWidget_GiveIO::getVendorName(quint16 vendorID)
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

QString PCIWidget_GiveIO::getDeviceName(quint16 vendorID, quint16 deviceID)
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

void PCIWidget_GiveIO::addDeviceToTable(const PCI_Device &pciDevice)
{
    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);

    // Основная информация
    QTableWidgetItem *busItem = new QTableWidgetItem(QString("0x%1").arg(pciDevice.bus, 2, 16, QChar('0')).toUpper());
    busItem->setTextAlignment(Qt::AlignCenter);
    tableWidget->setItem(row, 0, busItem);
    
    QTableWidgetItem *deviceItem = new QTableWidgetItem(QString("0x%1").arg(pciDevice.device, 2, 16, QChar('0')).toUpper());
    deviceItem->setTextAlignment(Qt::AlignCenter);
    tableWidget->setItem(row, 1, deviceItem);
    
    QTableWidgetItem *funcItem = new QTableWidgetItem(QString("0x%1").arg(pciDevice.function, 1, 16, QChar('0')).toUpper());
    funcItem->setTextAlignment(Qt::AlignCenter);
    tableWidget->setItem(row, 2, funcItem);
    
    QTableWidgetItem *vidItem = new QTableWidgetItem(QString("0x%1").arg(pciDevice.vendorID, 4, 16, QChar('0')).toUpper());
    vidItem->setTextAlignment(Qt::AlignCenter);
    tableWidget->setItem(row, 3, vidItem);
    
    QTableWidgetItem *didItem = new QTableWidgetItem(QString("0x%1").arg(pciDevice.deviceID, 4, 16, QChar('0')).toUpper());
    didItem->setTextAlignment(Qt::AlignCenter);
    tableWidget->setItem(row, 4, didItem);
    
    tableWidget->setItem(row, 5, new QTableWidgetItem(pciDevice.vendorName));
    tableWidget->setItem(row, 6, new QTableWidgetItem(pciDevice.deviceName));
    
    // Новые колонки с классификацией
    QTableWidgetItem *classItem = new QTableWidgetItem(getClassString(pciDevice.classCode));
    classItem->setToolTip(QString("Class Code: 0x%1").arg(pciDevice.classCode, 2, 16, QChar('0')).toUpper());
    tableWidget->setItem(row, 7, classItem);
    
    QTableWidgetItem *subClassItem = new QTableWidgetItem(getSubClassString(pciDevice.classCode, pciDevice.subClass));
    subClassItem->setToolTip(QString("SubClass: 0x%1").arg(pciDevice.subClass, 2, 16, QChar('0')).toUpper());
    tableWidget->setItem(row, 8, subClassItem);
    
    QTableWidgetItem *progIFItem = new QTableWidgetItem(getProgIFString(pciDevice.classCode, pciDevice.subClass, pciDevice.progIF));
    progIFItem->setTextAlignment(Qt::AlignCenter);
    progIFItem->setToolTip("Programming Interface");
    tableWidget->setItem(row, 9, progIFItem);
    
    QTableWidgetItem *headerItem = new QTableWidgetItem(QString("0x%1").arg(pciDevice.headerType, 2, 16, QChar('0')).toUpper());
    headerItem->setTextAlignment(Qt::AlignCenter);
    headerItem->setToolTip("Header Type");
    tableWidget->setItem(row, 10, headerItem);
}

void PCIWidget_GiveIO::scanPCI()
{
    pciDevices.clear();
    tableWidget->setRowCount(0);

    logMessage("=== ЗАПУСК СКАНИРОВАНИЯ ЧЕРЕЗ GiveIO ===");

    bool success = scanPCI_GiveIO();

    if (success) {
        QString message = QString("Успешно! Найдено устройств: %1").arg(pciDevices.size());
        logMessage(message);
    } else {
        logMessage("ПРЕДУПРЕЖДЕНИЕ: Устройства не найдены или доступ ограничен", true);
    }

    shutdownGiveIO();
}

void PCIWidget_GiveIO::scanPCI_devices()
{
    logMessage("=== ЗАПУСК ПРОГРАММЫ С GiveIO ===");

    if (!testPCIAccess()) {
        QMessageBox::warning(this, "Ошибка",
            "Не удалось получить доступ к PCI через GiveIO.\n"
            "Убедитесь, что:\n"
            "1. Программа запущена от администратора\n"
            "2. Драйвер GiveIO установлен в системе\n"
            "3. Система поддерживает прямой доступ к портам");
        return;
    }

    scanPCI();
    logMessage("=== РАБОТА ЗАВЕРШЕНА ===");
}

void PCIWidget_GiveIO::showAndStart()
{
    show();
    raise();
    activateWindow();
}

bool PCIWidget_GiveIO::isRunningAsAdmin()
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

