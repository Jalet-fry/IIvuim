#include "usbwindow.h"
#include "ui_usbwindow.h"
#include <QDateTime>
#include <QMessageBox>
#include <QHeaderView>

USBWindow::USBWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::USBWindow)
    , monitor(nullptr)
{
    ui->setupUi(this);
    
    // Настройка таблицы устройств
    ui->devicesTable->horizontalHeader()->setStretchLastSection(false);
    ui->devicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->devicesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->devicesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->devicesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    
    // Стилизация
    setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            border: 2px solid #3498db;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        QPushButton {
            background-color: #3498db;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #2980b9;
        }
        QPushButton:pressed {
            background-color: #21618c;
        }
        QPushButton:disabled {
            background-color: #bdc3c7;
        }
        QTableWidget {
            border: 1px solid #bdc3c7;
            border-radius: 3px;
        }
        QTextEdit {
            border: 1px solid #bdc3c7;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
        }
    )");
    
    // Создание и запуск монитора
    monitor = new USBMonitor(this);
    setupConnections();
    monitor->start();
    
    addLogMessage("=== Система мониторинга USB-устройств запущена ===");
    addLogMessage("Инструкция:");
    addLogMessage("1. Подключите USB-устройство (мышь, флешку)");
    addLogMessage("2. Событие отобразится в журнале");
    addLogMessage("3. Выберите устройство и нажмите 'Безопасно извлечь'");
    addLogMessage("4. Отключите устройство физически");
    addLogMessage("");
}

USBWindow::~USBWindow()
{
    if (monitor)
    {
        monitor->stop();
        monitor->wait();
    }
    delete ui;
}

void USBWindow::setupConnections()
{
    // Подключение сигналов от монитора
    connect(monitor, &USBMonitor::deviceConnected, 
            this, &USBWindow::onDeviceConnected);
    connect(monitor, &USBMonitor::deviceDisconnected, 
            this, &USBWindow::onDeviceDisconnected);
    connect(monitor, &USBMonitor::deviceEjected, 
            this, &USBWindow::onDeviceEjected);
    connect(monitor, &USBMonitor::ejectFailed, 
            this, &USBWindow::onEjectFailed);
    connect(monitor, &USBMonitor::logMessage, 
            this, &USBWindow::onLogMessage);
}

void USBWindow::updateDevicesTable()
{
    ui->devicesTable->setRowCount(0);
    
    QVector<USBDevice> devices = monitor->getCurrentDevices();
    
    for (int i = 0; i < devices.size(); ++i)
    {
        const USBDevice& device = devices[i];
        
        int row = ui->devicesTable->rowCount();
        ui->devicesTable->insertRow(row);
        
        // Номер
        QTableWidgetItem* numItem = new QTableWidgetItem(QString::number(i + 1));
        numItem->setTextAlignment(Qt::AlignCenter);
        ui->devicesTable->setItem(row, 0, numItem);
        
        // Имя устройства
        QTableWidgetItem* nameItem = new QTableWidgetItem(device.getName());
        ui->devicesTable->setItem(row, 1, nameItem);
        
        // PID
        QTableWidgetItem* pidItem = new QTableWidgetItem(device.getPID());
        pidItem->setTextAlignment(Qt::AlignCenter);
        ui->devicesTable->setItem(row, 2, pidItem);
        
        // Извлекаемое
        QTableWidgetItem* ejectableItem = new QTableWidgetItem(
            device.isEjectable() ? "Да" : "Нет"
        );
        ejectableItem->setTextAlignment(Qt::AlignCenter);
        if (device.isEjectable())
        {
            ejectableItem->setForeground(QBrush(QColor(46, 204, 113))); // Зеленый
        }
        else
        {
            ejectableItem->setForeground(QBrush(QColor(231, 76, 60))); // Красный
        }
        ui->devicesTable->setItem(row, 3, ejectableItem);
    }
    
    ui->statusLabel->setText(QString("Статус: Подключено устройств: %1").arg(devices.size()));
}

void USBWindow::addLogMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    ui->logTextEdit->append(timestamp + message);
    
    // Автопрокрутка вниз
    QTextCursor cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logTextEdit->setTextCursor(cursor);
}

void USBWindow::on_refreshButton_clicked()
{
    updateDevicesTable();
    addLogMessage("Список устройств обновлен");
}

void USBWindow::on_ejectButton_clicked()
{
    int currentRow = ui->devicesTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", 
                           "Пожалуйста, выберите устройство для извлечения");
        return;
    }
    
    QVector<USBDevice> devices = monitor->getCurrentDevices();
    if (currentRow >= devices.size())
    {
        QMessageBox::warning(this, "Ошибка", "Неверный индекс устройства");
        return;
    }
    
    const USBDevice& device = devices[currentRow];
    
    if (!device.isEjectable())
    {
        QMessageBox::information(this, "Информация", 
                               QString("Устройство '%1' нельзя безопасно извлечь.\n"
                                      "Это может быть системное устройство.")
                                      .arg(device.getName()));
        return;
    }
    
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Подтверждение", 
                                 QString("Извлечь устройство '%1'?")
                                 .arg(device.getName()),
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes)
    {
        addLogMessage(QString("Попытка извлечения: %1").arg(device.getName()));
        monitor->ejectDevice(currentRow);
        
        // Обновляем таблицу через небольшую задержку
        QTimer::singleShot(500, this, &USBWindow::updateDevicesTable);
    }
}

void USBWindow::on_clearLogButton_clicked()
{
    ui->logTextEdit->clear();
    addLogMessage("Журнал очищен");
}

void USBWindow::on_backButton_clicked()
{
    close();
}

void USBWindow::on_devicesTable_itemSelectionChanged()
{
    int currentRow = ui->devicesTable->currentRow();
    
    if (currentRow >= 0)
    {
        QVector<USBDevice> devices = monitor->getCurrentDevices();
        if (currentRow < devices.size())
        {
            ui->ejectButton->setEnabled(devices[currentRow].isEjectable());
        }
        else
        {
            ui->ejectButton->setEnabled(false);
        }
    }
    else
    {
        ui->ejectButton->setEnabled(false);
    }
}

void USBWindow::onDeviceConnected(const QString& deviceName, const QString& pid)
{
    addLogMessage(QString("✓ ПОДКЛЮЧЕНО: %1 [PID: %2]").arg(deviceName).arg(pid));
    updateDevicesTable();
}

void USBWindow::onDeviceDisconnected(const QString& deviceName, const QString& pid)
{
    addLogMessage(QString("✗ ОТКЛЮЧЕНО: %1 [PID: %2]").arg(deviceName).arg(pid));
    updateDevicesTable();
}

void USBWindow::onDeviceEjected(const QString& deviceName, bool success)
{
    if (success)
    {
        addLogMessage(QString("✓ БЕЗОПАСНО ИЗВЛЕЧЕНО: %1").arg(deviceName));
    }
    else
    {
        addLogMessage(QString("✗ ОШИБКА ИЗВЛЕЧЕНИЯ: %1").arg(deviceName));
    }
}

void USBWindow::onEjectFailed(const QString& deviceName)
{
    addLogMessage(QString("⚠ ОТКАЗ В ИЗВЛЕЧЕНИИ: %1").arg(deviceName));
    QMessageBox::warning(this, "Отказ в извлечении", 
                       QString("Не удалось безопасно извлечь устройство '%1'.\n"
                              "Устройство может использоваться системой или приложением.")
                              .arg(deviceName));
}

void USBWindow::onLogMessage(const QString& message)
{
    addLogMessage(message);
}

