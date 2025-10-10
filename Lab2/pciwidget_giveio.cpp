#include "pciwidget_giveio.h"
#include <QApplication>
#include <QTime>

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
    scanner(new PciScannerGiveIO(this))
{
    initializeUI();
    
    // Подключаем сигналы кнопок к UI слотам
    connect(scanButton, &QPushButton::clicked, this, &PCIWidget_GiveIO::onScanClicked);
    connect(clearButton, &QPushButton::clicked, this, &PCIWidget_GiveIO::onClearClicked);
    connect(tableWidget, &QTableWidget::itemSelectionChanged, this, &PCIWidget_GiveIO::onDeviceSelected);

    // Подключаем сигналы от scanner к UI слотам
    connect(scanner, &PciScannerGiveIO::logMessage, this, &PCIWidget_GiveIO::onScannerLog);
    connect(scanner, &PciScannerGiveIO::deviceFound, this, &PCIWidget_GiveIO::onScannerDeviceFound);
    connect(scanner, &PciScannerGiveIO::progress, this, &PCIWidget_GiveIO::onScannerProgress);
    connect(scanner, &PciScannerGiveIO::finished, this, &PCIWidget_GiveIO::onScannerFinished);
    
    if (!scanner->isRunningAsAdmin()) {
        logMessage("ПРЕДУПРЕЖДЕНИЕ: Программа запущена без прав администратора", true);
        logMessage("Прямой доступ к PCI может быть ограничен", true);
    } else {
        logMessage("Программа запущена с правами администратора");
    }
}

PCIWidget_GiveIO::~PCIWidget_GiveIO()
{
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

void PCIWidget_GiveIO::addDeviceToTable(const PCI_Device_GiveIO &pciDevice)
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
    QTableWidgetItem *classItem = new QTableWidgetItem(scanner->getClassString(pciDevice.classCode));
    classItem->setToolTip(QString("Class Code: 0x%1").arg(pciDevice.classCode, 2, 16, QChar('0')).toUpper());
    tableWidget->setItem(row, 7, classItem);
    
    QTableWidgetItem *subClassItem = new QTableWidgetItem(scanner->getSubClassString(pciDevice.classCode, pciDevice.subClass));
    subClassItem->setToolTip(QString("SubClass: 0x%1").arg(pciDevice.subClass, 2, 16, QChar('0')).toUpper());
    tableWidget->setItem(row, 8, subClassItem);
    
    QTableWidgetItem *progIFItem = new QTableWidgetItem(scanner->getProgIFString(pciDevice.classCode, pciDevice.subClass, pciDevice.progIF));
    progIFItem->setTextAlignment(Qt::AlignCenter);
    progIFItem->setToolTip("Programming Interface");
    tableWidget->setItem(row, 9, progIFItem);
    
    QTableWidgetItem *headerItem = new QTableWidgetItem(QString("0x%1").arg(pciDevice.headerType, 2, 16, QChar('0')).toUpper());
    headerItem->setTextAlignment(Qt::AlignCenter);
    headerItem->setToolTip("Header Type");
    tableWidget->setItem(row, 10, headerItem);
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
    
    const PCI_Device_GiveIO& dev = pciDevices[row];
    
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
     .arg(scanner->getClassString(dev.classCode))
     .arg(dev.subClass, 2, 16, QChar('0')).toUpper()
     .arg(scanner->getSubClassString(dev.classCode, dev.subClass))
     .arg(scanner->getProgIFString(dev.classCode, dev.subClass, dev.progIF))
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
    
    QMessageBox::information(this, "Детальная информация о PCI устройстве", details);
}

void PCIWidget_GiveIO::onScannerLog(const QString &msg, bool isError)
{
    logMessage(msg, isError);
}

void PCIWidget_GiveIO::onScannerDeviceFound(const PCI_Device_GiveIO &dev)
{
    pciDevices.append(dev);
    addDeviceToTable(dev);
}

void PCIWidget_GiveIO::onScannerProgress(int value, int maximum)
{
    progressBar->setVisible(true);
    progressBar->setMaximum(maximum);
    progressBar->setValue(value);
}

void PCIWidget_GiveIO::onScannerFinished(bool anyFound)
{
    // Скрываем анимацию Jake и останавливаем её
    jakeAnimation->stop();
    jakeAnimationLabel->setVisible(false);
    
    progressBar->setVisible(false);
    
    if (anyFound) {
        logMessage(QString("Успешно! Найдено устройств: %1").arg(pciDevices.size()));
    } else {
        logMessage("ПРЕДУПРЕЖДЕНИЕ: Устройства не найдены или доступ ограничен", true);
    }
}

void PCIWidget_GiveIO::onScanClicked()
{
    // Очистим предыдущие результаты
    pciDevices.clear();
    tableWidget->setRowCount(0);
    
    logMessage("=== ЗАПУСК ПРОГРАММЫ С GiveIO ===");

    // Проверка доступа
    if (!scanner->testAccess()) {
        QMessageBox::warning(this, "Ошибка",
            "Не удалось получить доступ к PCI через GiveIO.\n"
            "Убедитесь, что:\n"
            "1. Программа запущена от администратора\n"
            "2. Драйвер GiveIO установлен в системе\n"
            "3. Система поддерживает прямой доступ к портам");
        return;
    }

    // Показываем анимацию Jake и запускаем её
    jakeAnimationLabel->setVisible(true);
    jakeAnimation->start();
    
    // Запускаем основной скан (синхронно)
    scanner->scan();
    
    logMessage("=== РАБОТА ЗАВЕРШЕНА ===");
}

void PCIWidget_GiveIO::onClearClicked()
{
    logTextEdit->clear();
}

void PCIWidget_GiveIO::showAndStart()
{
    show();
    raise();
    activateWindow();
}
