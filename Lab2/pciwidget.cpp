#include "pciwidget.h"
#include <QDebug>

PCIWidget::PCIWidget(QWidget *parent) : QWidget(parent), deviceCount(0)
{
    setupUI();
    pciWorker = new PCIWorker(this);
    
    connect(pciWorker, &PCIWorker::deviceFound, this, &PCIWidget::onDeviceFound);
    connect(pciWorker, &PCIWorker::scanCompleted, this, &PCIWidget::onScanCompleted);
    connect(pciWorker, &PCIWorker::errorOccurred, this, &PCIWidget::onErrorOccurred);
    
    setWindowTitle("Лабораторная работа 2 - PCI устройства");
    setFixedSize(800, 600);
}

PCIWidget::~PCIWidget()
{
    if (pciWorker) {
        pciWorker->stopScan();
    }
}

void PCIWidget::showAndStart()
{
    show();
    raise();
    activateWindow();
}

void PCIWidget::closeEvent(QCloseEvent *event)
{
    if (pciWorker) {
        pciWorker->stopScan();
    }
    event->accept();
}

void PCIWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // Заголовок
    QLabel *titleLabel = new QLabel("Список PCI устройств");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: blue;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Панель управления
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    scanButton = new QPushButton("Сканировать PCI");
    scanButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px; font-weight: bold; }");
    connect(scanButton, &QPushButton::clicked, this, &PCIWidget::onScanButtonClicked);
    
    clearButton = new QPushButton("Очистить");
    clearButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px; font-weight: bold; }");
    connect(clearButton, &QPushButton::clicked, this, &PCIWidget::onClearButtonClicked);
    
    controlLayout->addWidget(scanButton);
    controlLayout->addWidget(clearButton);
    controlLayout->addStretch();
    
    mainLayout->addLayout(controlLayout);
    
    // Прогресс бар
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    mainLayout->addWidget(progressBar);
    
    // Таблица устройств
    deviceTable = new QTableWidget();
    deviceTable->setColumnCount(7);
    deviceTable->setHorizontalHeaderLabels({
        "Bus:Dev:Func",
        "Vendor ID",
        "Device ID", 
        "Class",
        "SubClass",
        "ProgIF",
        "Описание"
    });
    
    // Настройка таблицы
    deviceTable->setAlternatingRowColors(true);
    deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceTable->setSortingEnabled(true);
    deviceTable->horizontalHeader()->setStretchLastSection(true);
    deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    mainLayout->addWidget(deviceTable);
    
    // Статус бар
    QHBoxLayout *statusLayout = new QHBoxLayout();
    
    statusLabel = new QLabel("Готов к сканированию");
    statusLabel->setStyleSheet("color: green; font-weight: bold;");
    
    deviceCountLabel = new QLabel("Устройств: 0");
    deviceCountLabel->setStyleSheet("color: blue; font-weight: bold;");
    
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(deviceCountLabel);
    
    mainLayout->addLayout(statusLayout);
}

void PCIWidget::onDeviceFound(const PCIDevice &device)
{
    addDeviceToTable(device);
    deviceCount++;
    deviceCountLabel->setText(QString("Устройств: %1").arg(deviceCount));
}

void PCIWidget::onScanCompleted(int deviceCount)
{
    scanButton->setEnabled(true);
    progressBar->setVisible(false);
    statusLabel->setText(QString("Сканирование завершено. Найдено устройств: %1").arg(deviceCount));
    statusLabel->setStyleSheet("color: green; font-weight: bold;");
}

void PCIWidget::onErrorOccurred(const QString &error)
{
    scanButton->setEnabled(true);
    progressBar->setVisible(false);
    statusLabel->setText("Ошибка сканирования");
    statusLabel->setStyleSheet("color: red; font-weight: bold;");
    
    QMessageBox::warning(this, "Ошибка PCI сканирования", error);
}

void PCIWidget::onScanButtonClicked()
{
    qDebug() << "PCIWidget: Starting scan";
    
    // Защита от повторного нажатия
    if (!scanButton->isEnabled()) {
        qDebug() << "PCIWidget: Scan already in progress, ignoring click";
        return;
    }
    
    deviceTable->setRowCount(0);
    deviceCount = 0;
    deviceCountLabel->setText("Устройств: 0");
    
    scanButton->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0); // Неопределенный прогресс
    statusLabel->setText("Сканирование PCI шины...");
    statusLabel->setStyleSheet("color: orange; font-weight: bold;");
    
    pciWorker->startScan();
}

void PCIWidget::onClearButtonClicked()
{
    deviceTable->setRowCount(0);
    deviceCount = 0;
    deviceCountLabel->setText("Устройств: 0");
    statusLabel->setText("Таблица очищена");
    statusLabel->setStyleSheet("color: gray; font-weight: bold;");
}

void PCIWidget::addDeviceToTable(const PCIDevice &device)
{
    int row = deviceTable->rowCount();
    deviceTable->insertRow(row);
    
    // Bus:Dev:Func
    QString location = QString("%1:%2:%3")
        .arg(device.Bus, 2, 16, QChar('0'))
        .arg(device.Device, 2, 16, QChar('0'))
        .arg(device.Function);
    
    deviceTable->setItem(row, 0, new QTableWidgetItem(location));
    
    // Vendor ID
    deviceTable->setItem(row, 1, new QTableWidgetItem(formatHex(device.VendorID)));
    
    // Device ID
    deviceTable->setItem(row, 2, new QTableWidgetItem(formatHex(device.DeviceID)));
    
    // Class
    deviceTable->setItem(row, 3, new QTableWidgetItem(formatHex(device.ClassCode, 2)));
    
    // SubClass
    deviceTable->setItem(row, 4, new QTableWidgetItem(formatHex(device.SubClass, 2)));
    
    // ProgIF
    deviceTable->setItem(row, 5, new QTableWidgetItem(formatHex(device.ProgIF, 2)));
    
    // Описание
    deviceTable->setItem(row, 6, new QTableWidgetItem(device.DeviceName));
    
    // Автоматическое изменение размера столбцов
    deviceTable->resizeColumnsToContents();
}

QString PCIWidget::formatHex(unsigned int value, int width)
{
    return QString("0x%1").arg(value, width, 16, QChar('0')).toUpper();
}
