#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTime>
#include <QMap>
#include <windows.h>

// Функция проверки прав администратора (добавьте в отдельный файл или в начаeло)
extern bool isRunningAsAdmin();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    tableWidget(0),
    scanButton(0),
    clearButton(0),
    centralWidget(0),
    progressBar(0),
    logTextEdit(0),
    mainSplitter(0),
    giveioHandle(INVALID_HANDLE_VALUE),
    giveioInitialized(false)
{
    ui->setupUi(this);
    initializeUI();

    if (!isRunningAsAdmin()) {
        logMessage("ПРЕДУПРЕЖДЕНИЕ: Программа запущена без прав администратора", true);
        logMessage("Прямой доступ к PCI может быть ограничен", true);
    } else {
        logMessage("Программа запущена с правами администратора");
    }
}

MainWindow::~MainWindow()
{
    shutdownGiveIO();
    delete ui;
}

void MainWindow::initializeUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    scanButton = new QPushButton("Сканировать PCI устройства", this);
    clearButton = new QPushButton("Очистить лог", this);
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(clearButton);
    mainLayout->addLayout(buttonLayout);

    // Прогресс бар
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    mainLayout->addWidget(progressBar);

    // Сплиттер для таблицы и лога
    mainSplitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(mainSplitter);

    // Таблица устройств
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(7);
    QStringList headers;
    headers << "Bus" << "Device" << "Function" << "VendorID" << "DeviceID" << "Vendor" << "Device";
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainSplitter->addWidget(tableWidget);

    // Лог
    logTextEdit = new QTextEdit(this);
    logTextEdit->setMaximumHeight(200);
    logTextEdit->setReadOnly(true);
    mainSplitter->addWidget(logTextEdit);

    // Подключаем сигналы
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::scanPCI_devices);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearLog);

    resize(1000, 700);
    setWindowTitle("PCI Devices Scanner - GiveIO");
}

void MainWindow::logMessage(const QString &message, bool isError)
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

void MainWindow::clearLog()
{
    logTextEdit->clear();
}

// GiveIO implementation
bool MainWindow::giveioInitialize()
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

void MainWindow::giveioShutdown()
{
    if (giveioHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(giveioHandle);
        giveioHandle = INVALID_HANDLE_VALUE;
    }
    giveioInitialized = false;
}

void MainWindow::giveioOutPortDword(WORD port, DWORD value)
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
        asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
    #endif
}

DWORD MainWindow::giveioInPortDword(WORD port)
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
        asm volatile ("in %1, %0" : "=a"(result) : "Nd"(port));
        return result;
    #endif
}

bool MainWindow::initializeGiveIO()
{
    logMessage("Инициализация драйвера GiveIO...");
    return giveioInitialize();
}

void MainWindow::shutdownGiveIO()
{
    giveioShutdown();
    logMessage("GiveIO выгружен");
}

bool MainWindow::writePortDword(WORD port, DWORD value)
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

DWORD MainWindow::readPortDword(WORD port)
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

bool MainWindow::testPCIAccess()
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

bool MainWindow::scanPCI_GiveIO()
{
    logMessage("=== НАЧАЛО СКАНИРОВАНИЯ PCI ЧЕРЕЗ GiveIO ===");

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

                    pciDevices.append(dev);
                    addDeviceToTable(dev);
                    foundDevices++;

                    logMessage(QString("Найдено: Bus=0x%1 Dev=0x%2 Func=0x%3 VID=0x%4 DID=0x%5 - %6 %7")
                              .arg(bus, 2, 16, QChar('0'))
                              .arg(device, 2, 16, QChar('0'))
                              .arg(function, 1, 16, QChar('0'))
                              .arg(vendorID, 4, 16, QChar('0'))
                              .arg(deviceID, 4, 16, QChar('0'))
                              .arg(dev.vendorName)
                              .arg(dev.deviceName));
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
    logMessage(QString("Сканирование завершено. Найдено устройств: %1").arg(foundDevices));

    return foundDevices > 0;
}

QString MainWindow::getVendorName(quint16 vendorID)
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

QString MainWindow::getDeviceName(quint16 vendorID, quint16 deviceID)
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

void MainWindow::addDeviceToTable(const PCI_Device &pciDevice)
{
    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);

    tableWidget->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(pciDevice.bus, 2, 16, QChar('0')).toUpper()));
    tableWidget->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(pciDevice.device, 2, 16, QChar('0')).toUpper()));
    tableWidget->setItem(row, 2, new QTableWidgetItem(QString("0x%1").arg(pciDevice.function, 1, 16, QChar('0')).toUpper()));
    tableWidget->setItem(row, 3, new QTableWidgetItem(QString("0x%1").arg(pciDevice.vendorID, 4, 16, QChar('0')).toUpper()));
    tableWidget->setItem(row, 4, new QTableWidgetItem(QString("0x%1").arg(pciDevice.deviceID, 4, 16, QChar('0')).toUpper()));
    tableWidget->setItem(row, 5, new QTableWidgetItem(pciDevice.vendorName));
    tableWidget->setItem(row, 6, new QTableWidgetItem(pciDevice.deviceName));
}

void MainWindow::scanPCI()
{
    pciDevices.clear();
    tableWidget->setRowCount(0);

    statusBar()->showMessage("Сканирование PCI через GiveIO...");
    logMessage("=== ЗАПУСК СКАНИРОВАНИЯ ЧЕРЕЗ GiveIO ===");

    bool success = scanPCI_GiveIO();

    if (success) {
        QString message = QString("Успешно! Найдено устройств: %1").arg(pciDevices.size());
        statusBar()->showMessage(message);
        logMessage(message);
    } else {
        statusBar()->showMessage("Сканирование не удалось");
        logMessage("ПРЕДУПРЕЖДЕНИЕ: Устройства не найдены или доступ ограничен", true);
    }

    shutdownGiveIO();
}

void MainWindow::scanPCI_devices()
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
