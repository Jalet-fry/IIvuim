#include "bluetoothwindow.h"
#include "ui_bluetoothwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTimer>

BluetoothWindow::BluetoothWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BluetoothWindow)
    , bluetoothManager(nullptr)
    , logger(nullptr)
    , fileSender(nullptr)
    , btConnection(nullptr)
    , btReceiver(nullptr)
    , selectedDeviceIndex(-1)
    , isDeviceConnected(false)
{
    ui->setupUi(this);
    setupUI();

    // Создаем систему логирования
    logger = new BluetoothLogger(this);
    connect(logger, &BluetoothLogger::logToUI,
            this, &BluetoothWindow::addLogMessage);
    
    logger->info("UI", "═══════════════════════════════════════");
    logger->info("UI", "BLUETOOTH WINDOW ИНИЦИАЛИЗАЦИЯ");
    logger->info("UI", "═══════════════════════════════════════");
    logger->info("UI", "");
    
    // Создаем приемник файлов с автовоспроизведением
    btReceiver = new BluetoothReceiver(logger, this);
    btReceiver->setAutoPlay(true);  // Включаем автовоспроизведение!
    
    connect(btReceiver, &BluetoothReceiver::fileReceived,
            this, [this](const QString &filePath) {
                logger->success("UI", QString("✓ Файл получен: %1").arg(filePath));
            });
    connect(btReceiver, &BluetoothReceiver::playbackStarted,
            this, [this](const QString &fileName) {
                logger->success("UI", QString("🎵 Автовоспроизведение: %1").arg(fileName));
            });
    
    // Создаем RFCOMM подключение
    btConnection = new BluetoothConnection(logger, this);
    connect(btConnection, &BluetoothConnection::connectionEstablished,
            this, [this](const QString &deviceName) {
                isDeviceConnected = true;
                updateButtonStates();
                logger->success("UI", QString("✓ Подключено к: %1").arg(deviceName));
                
                // Запускаем прослушивание входящих данных
                btReceiver->startListening(btConnection);
            });
    connect(btConnection, &BluetoothConnection::connectionFailed,
            this, [this](const QString &error) {
                isDeviceConnected = false;
                updateButtonStates();
                logger->error("UI", QString("✗ Ошибка подключения: %1").arg(error));
            });
    connect(btConnection, &BluetoothConnection::disconnected,
            this, [this]() {
                isDeviceConnected = false;
                updateButtonStates();
                logger->info("UI", "Отключено от устройства");
                btReceiver->stopListening();
            });
    
    // Создаем OBEX sender для Android
    obexSender = new ObexFileSender(logger, this);
    connect(obexSender, &ObexFileSender::transferStarted,
            this, [this](const QString &fileName) {
                logger->info("Transfer", QString("OBEX: Начало передачи: %1").arg(fileName));
                ui->progressBar->setVisible(true);
                ui->progressBar->setValue(0);
            });
    connect(obexSender, &ObexFileSender::transferProgress,
            this, [this](qint64 bytesSent, qint64 totalBytes) {
                if (totalBytes > 0) {
                    int progress = (bytesSent * 100) / totalBytes;
                    ui->progressBar->setValue(progress);
                }
            });
    connect(obexSender, &ObexFileSender::transferCompleted,
            this, [this](const QString &fileName) {
                logger->success("Transfer", QString("OBEX: Файл передан: %1").arg(fileName));
                ui->progressBar->setValue(100);
            });
    connect(obexSender, &ObexFileSender::transferFailed,
            this, [this](const QString &error) {
                logger->error("Transfer", QString("OBEX: Ошибка: %1").arg(error));
                ui->progressBar->setVisible(false);
            });
    
    // Создаем Bluetooth сервер для приема файлов ПК-ПК
    btServer = new BluetoothServer(logger, this);
    connect(btServer, &BluetoothServer::serverStarted,
            this, [this]() {
                logger->success("Server", "✓ Сервер запущен");
                ui->serverStatusLabel->setText("Сервер: Запущен");
                ui->serverStatusLabel->setStyleSheet("font-size: 12px; padding: 5px; color: #4CAF50; font-weight: bold;");
                ui->startServerButton->setEnabled(false);
                ui->stopServerButton->setEnabled(true);
            });
    connect(btServer, &BluetoothServer::serverStopped,
            this, [this]() {
                logger->info("Server", "Сервер остановлен");
                ui->serverStatusLabel->setText("Сервер: Остановлен");
                ui->serverStatusLabel->setStyleSheet("font-size: 12px; padding: 5px; color: #666;");
                ui->startServerButton->setEnabled(true);
                ui->stopServerButton->setEnabled(false);
            });
    connect(btServer, &BluetoothServer::fileReceived,
            this, [this](const QString &fileName) {
                logger->success("Server", QString("✓ Файл получен: %1").arg(fileName));
            });
    connect(btServer, &BluetoothServer::transferProgress,
            this, [this](qint64 bytesReceived, qint64 totalBytes) {
                if (totalBytes > 0) {
                    int progress = (bytesReceived * 100) / totalBytes;
                    ui->progressBar->setValue(progress);
                }
            });
    connect(btServer, &BluetoothServer::transferCompleted,
            this, [this](const QString &fileName) {
                logger->success("Server", QString("✓ Передача завершена: %1").arg(fileName));
                ui->progressBar->setValue(100);
                QTimer::singleShot(2000, [this]() {
                    ui->progressBar->setVisible(false);
                });
            });
    connect(btServer, &BluetoothServer::transferFailed,
            this, [this](const QString &error) {
                logger->error("Server", QString("Ошибка сервера: %1").arg(error));
                ui->progressBar->setVisible(false);
            });
    
    // Создаем файлсендер (для RFCOMM)
    fileSender = new BluetoothFileSender(logger, this);
    connect(fileSender, &BluetoothFileSender::transferStarted,
            this, [this](const QString &fileName) {
                logger->info("Transfer", QString("Начало передачи: %1").arg(fileName));
                ui->progressBar->setVisible(true);
                ui->progressBar->setValue(0);
            });
    connect(fileSender, &BluetoothFileSender::transferProgress,
            this, [this](qint64 bytesSent, qint64 totalBytes) {
                if (totalBytes > 0) {
                    int progress = (bytesSent * 100) / totalBytes;
                    ui->progressBar->setValue(progress);
                }
            });
    connect(fileSender, &BluetoothFileSender::transferCompleted,
            this, [this](const QString &fileName) {
                logger->success("Transfer", QString("Файл передан: %1").arg(fileName));
                ui->progressBar->setValue(100);
            });
    connect(fileSender, &BluetoothFileSender::transferFailed,
            this, [this](const QString &error) {
                logger->error("Transfer", QString("Ошибка: %1").arg(error));
                ui->progressBar->setVisible(false);
            });
    
    // Создаем Windows Bluetooth менеджер
    logger->info("UI", "Создание WindowsBluetoothManager...");
    bluetoothManager = new WindowsBluetoothManager(this);

    // Подключаем сигналы от менеджера
    connect(bluetoothManager, &WindowsBluetoothManager::deviceDiscovered,
            this, &BluetoothWindow::onDeviceDiscovered);
    connect(bluetoothManager, &WindowsBluetoothManager::discoveryFinished,
            this, &BluetoothWindow::onDiscoveryFinished);
    connect(bluetoothManager, &WindowsBluetoothManager::discoveryError,
            this, &BluetoothWindow::onDiscoveryError);

    connect(bluetoothManager, &WindowsBluetoothManager::logMessage,
            this, &BluetoothWindow::onLogMessage);

    // Подключаем кнопки
    connect(ui->scanButton, &QPushButton::clicked,
            this, &BluetoothWindow::onScanButtonClicked);
    connect(ui->clearLogButton, &QPushButton::clicked,
            this, &BluetoothWindow::onClearLogButtonClicked);
    connect(ui->refreshAdapterButton, &QPushButton::clicked,
            this, &BluetoothWindow::onRefreshAdapterButtonClicked);
    
    // Подключаем кнопки управления устройствами
    connect(ui->connectButton, &QPushButton::clicked,
            this, &BluetoothWindow::onConnectButtonClicked);
    connect(ui->disconnectButton, &QPushButton::clicked,
            this, &BluetoothWindow::onDisconnectButtonClicked);
    connect(ui->sendFileButton, &QPushButton::clicked,
            this, &BluetoothWindow::onSendFileButtonClicked);
    connect(ui->startServerButton, &QPushButton::clicked,
            this, &BluetoothWindow::onStartServerButtonClicked);
    connect(ui->stopServerButton, &QPushButton::clicked,
            this, &BluetoothWindow::onStopServerButtonClicked);
    
    // Подключаем сигнал выбора устройства в таблице
    connect(ui->devicesTable, &QTableWidget::itemSelectionChanged,
            this, &BluetoothWindow::onDeviceSelectionChanged);
    
    // Начальное состояние кнопок
    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(false);
    ui->sendFileButton->setEnabled(false);
    ui->startServerButton->setEnabled(true);
    ui->stopServerButton->setEnabled(false);

    // Логируем информацию о локальном адаптере
    logger->info("Adapter", "═══════════════════════════════════════");
    logger->info("Adapter", "ИНФОРМАЦИЯ О BLUETOOTH АДАПТЕРЕ");
    logger->info("Adapter", "═══════════════════════════════════════");
    logger->info("Adapter", QString("Имя: %1").arg(bluetoothManager->getLocalDeviceName()));
    logger->info("Adapter", QString("MAC: %1").arg(bluetoothManager->getLocalDeviceAddress()));
    logger->info("Adapter", "");

    if (!bluetoothManager->isBluetoothAvailable()) {
        logger->error("Adapter", "✗ Bluetooth адаптер НЕ НАЙДЕН!");
        logger->warning("Adapter", "Проверьте, что Bluetooth включен в Windows");
        logger->info("Adapter", "");
        QMessageBox::warning(this, "Bluetooth",
                             "Bluetooth адаптер не обнаружен. Проверьте, что Bluetooth включен в Windows.");
    } else {
        logger->success("Adapter", "✓ Bluetooth адаптер АКТИВЕН!");
        logger->info("Adapter", "");
    }
    
    // Проверяем поддержку OBEX
    logger->info("Features", "Проверка функциональности...");
    bool obexSupported = fileSender->isObexSupported();
    if (!obexSupported) {
        logger->warning("Features", "Отправка файлов может работать некорректно");
    }
    logger->info("Features", "");
    
    logger->success("UI", "✓ Bluetooth Window готово к работе!");
    logger->info("UI", "");
}

BluetoothWindow::~BluetoothWindow()
{
    delete ui;
}

void BluetoothWindow::setupUI()
{
    setWindowTitle("Лабораторная работа 6 - Bluetooth (Windows Native API)");
    setMinimumSize(1100, 750);  // Увеличено для лучшего отображения таблицы

    // Настройка таблицы устройств
    ui->devicesTable->setColumnCount(5);
    ui->devicesTable->setHorizontalHeaderLabels({"№", "Имя устройства", "MAC адрес", "Тип", "Статус"});
    ui->devicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->devicesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->devicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->devicesTable->horizontalHeader()->setStretchLastSection(true);
    ui->devicesTable->setColumnWidth(0, 50);
    ui->devicesTable->setColumnWidth(1, 250);   // Увеличено для длинных имён
    ui->devicesTable->setColumnWidth(2, 150);
    ui->devicesTable->setColumnWidth(3, 120);
    // Колонка 4 (Статус) растягивается автоматически
    
    // Скрываем прогресс-бар по умолчанию
    ui->progressBar->setVisible(false);
}

void BluetoothWindow::addLogMessage(const QString &message, const QString &color)
{
    QString timestamp = getCurrentTimestamp();
    QString htmlMessage = QString("<font color='gray'>%1</font> <font color='%2'>%3</font>")
                              .arg(timestamp)
                              .arg(color)
                              .arg(message);
    ui->logTextEdit->append(htmlMessage);
}

QString BluetoothWindow::getCurrentTimestamp() const
{
    return QDateTime::currentDateTime().toString("[HH:mm:ss]");
}

// Слоты для кнопок

void BluetoothWindow::onScanButtonClicked()
{
    logger->info("Scan", "═══════════════════════════════════════");
    logger->info("Scan", "НАЧАЛО СКАНИРОВАНИЯ УСТРОЙСТВ");
    logger->info("Scan", "═══════════════════════════════════════");
    logger->debug("Scan", "Очистка таблицы устройств");
    
    ui->devicesTable->setRowCount(0);
    discoveredDevices.clear();
    selectedDeviceIndex = -1;
    updateButtonStates();
    
    logger->info("Scan", "Запуск сканирования через WinAPI...");
    bluetoothManager->startDeviceDiscovery();
    
    ui->scanButton->setEnabled(false);
    ui->scanButton->setText("Сканирование...");
    logger->info("Scan", "");
}

void BluetoothWindow::onClearLogButtonClicked()
{
    ui->logTextEdit->clear();
    addLogMessage("Журнал очищен", "gray");
}

void BluetoothWindow::onRefreshAdapterButtonClicked()
{
    addLogMessage("=== Обновление информации об адаптере ===", "blue");
    addLogMessage("Локальное устройство: " + bluetoothManager->getLocalDeviceName());
    addLogMessage("Адрес: " + bluetoothManager->getLocalDeviceAddress());

    if (bluetoothManager->isBluetoothAvailable()) {
        addLogMessage("✓ Bluetooth адаптер активен", "green");
        
        // Показываем подключенные устройства
        QList<BluetoothDeviceData> connected = bluetoothManager->getConnectedDevices();
        if (!connected.isEmpty()) {
            addLogMessage(QString("Подключенных устройств: %1").arg(connected.count()), "green");
            for (const BluetoothDeviceData &dev : connected) {
                addLogMessage(QString("  • %1 (%2)").arg(dev.name).arg(dev.address), "darkgreen");
            }
        } else {
            addLogMessage("Нет подключенных устройств", "gray");
        }
    } else {
        addLogMessage("✗ Bluetooth адаптер недоступен!", "red");
    }
}

// Слоты для событий Bluetooth

void BluetoothWindow::onDeviceDiscovered(const BluetoothDeviceData &device)
{
    // Добавляем устройство в список
    discoveredDevices.append(device);

    int row = ui->devicesTable->rowCount();
    ui->devicesTable->insertRow(row);

    // Номер
    ui->devicesTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));

    // Имя
    QString name = device.name.isEmpty() ? "Без имени" : device.name;
    ui->devicesTable->setItem(row, 1, new QTableWidgetItem(name));

    // MAC адрес
    ui->devicesTable->setItem(row, 2, new QTableWidgetItem(device.address));

    // Тип устройства
    ui->devicesTable->setItem(row, 3, new QTableWidgetItem(device.getDeviceTypeString()));
    
    // Статус
    QString status;
    if (device.isConnected) status += "🔵 Подключено ";
    if (device.isPaired) status += "🔗 Сопряжено ";
    if (device.isRemembered) status += "💾 Запомнено ";
    if (status.isEmpty()) status = "Обнаружено";
    
    ui->devicesTable->setItem(row, 4, new QTableWidgetItem(status.trimmed()));
}

void BluetoothWindow::onDiscoveryFinished(int deviceCount)
{
    addLogMessage("=== Сканирование завершено ===", "blue");
    addLogMessage(QString("Найдено устройств: %1").arg(deviceCount), "green");
    ui->scanButton->setEnabled(true);
    ui->scanButton->setText("Сканировать");
    updateButtonStates();
}

void BluetoothWindow::onDiscoveryError(const QString &errorString)
{
    addLogMessage("ОШИБКА: " + errorString, "red");
    ui->scanButton->setEnabled(true);
    ui->scanButton->setText("Сканировать");
}

void BluetoothWindow::onLogMessage(const QString &message)
{
    addLogMessage(message);
}

void BluetoothWindow::onDeviceSelectionChanged()
{
    selectedDeviceIndex = ui->devicesTable->currentRow();
    updateButtonStates();
}

void BluetoothWindow::updateButtonStates()
{
    bool deviceSelected = (selectedDeviceIndex >= 0 && selectedDeviceIndex < discoveredDevices.size());
    
    // Проверяем возможности выбранного устройства
    bool canSendFiles = false;
    if (deviceSelected) {
        const BluetoothDeviceData &device = discoveredDevices[selectedDeviceIndex];
        canSendFiles = device.canSendFilesTo();
        
        // Если нельзя отправлять файлы - меняем tooltip кнопки
        if (!canSendFiles) {
            BluetoothDeviceCapabilities caps = device.getCapabilities();
            ui->sendFileButton->setToolTip(QString("Невозможно: %1").arg(caps.blockReason));
        } else {
            ui->sendFileButton->setToolTip("Отправить файл на выбранное устройство");
        }
    }
    
    // Кнопка "Подключить" доступна если устройство выбрано и НЕ подключено
    // НО только если это устройство может принимать файлы!
    ui->connectButton->setEnabled(deviceSelected && !isDeviceConnected && canSendFiles);
    
    // Кнопка "Отключить" доступна если есть активное подключение
    ui->disconnectButton->setEnabled(isDeviceConnected);
    
    // Кнопка "Отправить файл" доступна если:
    // 1. Устройство выбрано
    // 2. Устройство может принимать файлы  
    // 3. Устройство поддерживает OBEX (телефоны И компьютеры)
    if (deviceSelected && canSendFiles) {
        const BluetoothDeviceData &device = discoveredDevices[selectedDeviceIndex];
        BluetoothDeviceCapabilities caps = device.getCapabilities();
        
        // OBEX работает для всех устройств без предварительного подключения
        ui->sendFileButton->setEnabled(caps.supportsOBEX);
    } else {
        ui->sendFileButton->setEnabled(false);
    }
}

void BluetoothWindow::onConnectButtonClicked()
{
    if (selectedDeviceIndex < 0 || selectedDeviceIndex >= discoveredDevices.size()) {
        logger->error("Connect", "Устройство не выбрано!");
        QMessageBox::warning(this, "Ошибка", "Выберите устройство для подключения");
        return;
    }
    
    const BluetoothDeviceData &device = discoveredDevices[selectedDeviceIndex];
    
    logger->info("Connect", "═══════════════════════════════════════");
    logger->info("Connect", "ПОПЫТКА RFCOMM ПОДКЛЮЧЕНИЯ");
    logger->info("Connect", "═══════════════════════════════════════");
    logger->info("Connect", QString("Целевое устройство: %1").arg(device.name.isEmpty() ? "(Без имени)" : device.name));
    logger->info("Connect", QString("MAC адрес: %1").arg(device.address));
    logger->info("Connect", QString("Тип: %1").arg(device.getDeviceTypeString()));
    logger->info("Connect", "");
    
    // Проверяем сопряжение
    if (!device.isPaired) {
        logger->error("Connect", "✗ УСТРОЙСТВО НЕ СОПРЯЖЕНО!");
        logger->warning("Connect", "Сначала сопрягите устройство через Windows");
        
        QMessageBox::critical(this, "Не сопряжено",
            QString("Устройство '%1' не сопряжено.\n\n"
                    "Откройте: Параметры → Bluetooth\n"
                    "Нажмите: Добавить устройство")
            .arg(device.name.isEmpty() ? device.address : device.name));
        return;
    }
    
    logger->success("Connect", "✓ Устройство сопряжено");
    logger->info("Connect", "");
    logger->warning("Connect", "⏱ Подключение может занять 5-30 секунд...");
    logger->info("Connect", "Пожалуйста подождите...");
    logger->info("Connect", "");
    
    // Блокируем UI на время подключения
    ui->connectButton->setEnabled(false);
    ui->connectButton->setText("Подключение...");
    
    // Пытаемся подключиться через RFCOMM
    logger->info("Connect", "Вызов BluetoothConnection::connectToDevice()...");
    bool success = btConnection->connectToDevice(device.address, device.name);
    
    // Восстанавливаем кнопку
    ui->connectButton->setText("Подключить");
    updateButtonStates();
    
    if (success) {
        logger->success("Connect", "");
        logger->success("Connect", "✓✓✓ ПОДКЛЮЧЕНИЕ УСТАНОВЛЕНО! ✓✓✓");
        logger->info("Connect", "");
        logger->info("Connect", "Теперь можно отправлять данные напрямую!");
        logger->info("Connect", "");
        
        QMessageBox::information(this, "Подключено!",
            QString("Успешно подключено к устройству:\n\n"
                    "%1\n"
                    "%2\n\n"
                    "Теперь можно отправлять файлы!")
            .arg(device.name)
            .arg(device.address));
    } else {
        logger->error("Connect", "");
        logger->error("Connect", "✗✗✗ ПОДКЛЮЧЕНИЕ НЕ УДАЛОСЬ ✗✗✗");
        logger->error("Connect", "");
        logger->warning("Connect", "Смотрите детали в файле connect.log");
        logger->info("Connect", "");
        logger->info("Connect", "АЛЬТЕРНАТИВА:");
        logger->info("Connect", "Можно отправлять файлы БЕЗ RFCOMM подключения");
        logger->info("Connect", "через кнопку 'Отправить файл' (использует fsquirt)");
        logger->info("Connect", "");
    }
}

void BluetoothWindow::onDisconnectButtonClicked()
{
    logger->info("Connect", "═══════════════════════════════════════");
    logger->info("Connect", "ОТКЛЮЧЕНИЕ ОТ УСТРОЙСТВА");
    logger->info("Connect", "═══════════════════════════════════════");
    logger->info("Connect", "Закрытие RFCOMM соединения...");
    
    btConnection->disconnect();
    
    logger->success("Connect", "✓ Отключено");
    logger->info("Connect", "");
    
    QMessageBox::information(this, "Отключено", "Устройство отключено");
}

void BluetoothWindow::onSendFileButtonClicked()
{
    if (selectedDeviceIndex < 0 || selectedDeviceIndex >= discoveredDevices.size()) {
        logger->error("Send", "Устройство не выбрано!");
        QMessageBox::warning(this, "Ошибка", "Выберите устройство");
        return;
    }
    
    const BluetoothDeviceData &device = discoveredDevices[selectedDeviceIndex];
    
    logger->info("Send", "═══════════════════════════════════════");
    logger->info("Send", "ОТПРАВКА ФАЙЛА НА BLUETOOTH УСТРОЙСТВО");
    logger->info("Send", "═══════════════════════════════════════");
    logger->info("Send", QString("Целевое устройство: %1").arg(device.name));
    logger->info("Send", QString("MAC адрес: %1").arg(device.address));
    logger->debug("Send", QString("Подключено: %1").arg(device.isConnected ? "ДА" : "НЕТ"));
    logger->debug("Send", QString("Сопряжено: %1").arg(device.isPaired ? "ДА" : "НЕТ"));
    logger->info("Send", "");
    
    // Проверяем сопряжение
    if (!device.isPaired) {
        logger->error("Send", "✗ УСТРОЙСТВО НЕ СОПРЯЖЕНО!");
        logger->warning("Send", "Сначала сопрягите устройство через Windows");
        
        QMessageBox::critical(this, "Устройство не сопряжено",
            QString("Устройство '%1' не сопряжено с вашим ПК.\n\n"
                    "Для отправки файлов устройство ДОЛЖНО быть сопряжено.\n\n"
                    "Как сопрячь:\n"
                    "1. Откройте: Параметры Windows → Bluetooth\n"
                    "2. Нажмите: Добавить устройство\n"
                    "3. Выберите ваш %2")
            .arg(device.name.isEmpty() ? device.address : device.name)
            .arg(device.getDeviceTypeString()));
        return;
    }
    
    logger->success("Send", "✓ Устройство сопряжено");
    
    // Выбираем файл
    logger->info("Send", "Открытие диалога выбора файла...");
    
    QString fileName = QFileDialog::getOpenFileName(
        this,
        QString("Отправить файл на: %1").arg(device.name),
        QDir::homePath(),
        "Аудио файлы (*.mp3 *.wav *.ogg *.flac *.m4a);;Изображения (*.jpg *.jpeg *.png *.gif);;Видео (*.mp4 *.avi *.mkv);;Документы (*.pdf *.doc *.docx *.txt);;Все файлы (*)");
    
    if (fileName.isEmpty()) {
        logger->warning("Send", "Отправка отменена пользователем");
        return;
    }
    
    QFileInfo fileInfo(fileName);
    logger->success("Send", QString("✓ Выбран файл: %1").arg(fileInfo.fileName()));
    logger->debug("Send", QString("Полный путь: %1").arg(fileName));
    logger->debug("Send", QString("Размер: %1 байт (%2 MB)")
        .arg(fileInfo.size())
        .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2));
    logger->info("Send", "");
    
    // УМНОЕ ОПРЕДЕЛЕНИЕ МЕТОДА ОТПРАВКИ
    BluetoothDeviceCapabilities caps = device.getCapabilities();
    
    logger->info("Send", "═══════════════════════════════════════");
    logger->info("Send", "АНАЛИЗ УСТРОЙСТВА");
    logger->info("Send", "═══════════════════════════════════════");
    logger->info("Send", QString("Тип устройства: %1").arg(device.getDeviceTypeString()));
    logger->info("Send", QString("Может принимать файлы: %1").arg(caps.canReceiveFiles ? "ДА" : "НЕТ"));
    logger->info("Send", QString("Поддерживает RFCOMM: %1").arg(caps.supportsRFCOMM ? "ДА" : "НЕТ"));
    logger->info("Send", QString("Поддерживает OBEX: %1").arg(caps.supportsOBEX ? "ДА" : "НЕТ"));
    logger->info("Send", QString("Рекомендуемый метод: %1").arg(caps.recommendedMethod));
    logger->info("Send", "");
    
    // Определяем метод отправки
    bool useOBEX = false;
    
    if (caps.supportsOBEX) {
        // Все устройства с OBEX - телефоны И компьютеры
        useOBEX = true;
        logger->success("Send", "✓ Устройство поддерживает OBEX");
        logger->info("Send", "Будет использована ПРЯМАЯ отправка через OBEX протокол");
        logger->info("Send", "БЕЗ fsquirt - напрямую через сокет!");
    } else {
        logger->error("Send", "");
        logger->error("Send", "✗ ОТПРАВКА НЕ ВОЗМОЖНА!");
        logger->error("Send", QString("Причина: %1").arg(caps.blockReason));
        logger->error("Send", "");
        
        QMessageBox::critical(this, "Отправка невозможна",
            QString("Невозможно отправить файл на %1\n\n"
                    "Причина: %2")
            .arg(device.name)
            .arg(caps.blockReason));
        return;
    }
    
    // ВЫПОЛНЯЕМ ОТПРАВКУ выбранным методом
    logger->info("Send", "═══════════════════════════════════════");
    logger->info("Send", "ВЫПОЛНЕНИЕ ОТПРАВКИ");
    logger->info("Send", "═══════════════════════════════════════");
    logger->info("Send", "");
    
    bool success = false;
    
    if (useOBEX) {
        // ПРЯМАЯ ОТПРАВКА через OBEX протокол (для телефонов И компьютеров)
        QString deviceType = device.getDeviceTypeString();
        logger->info("Send", QString("Метод: OBEX протокол (прямая отправка на %1)").arg(deviceType));
        logger->info("Send", "БЕЗ FSQUIRT - только OBEX через сокет!");
        logger->info("Send", "");
        
        ui->progressBar->setVisible(true);
        ui->progressBar->setValue(0);
        
        // Выбираем метод в зависимости от типа устройства
        if (deviceType == "Компьютер") {
            // Для компьютеров используем RFCOMM (ПК-ПК передача)
            logger->info("Send", "Выбран метод: RFCOMM для ПК-ПК передачи");
            success = obexSender->sendFileViaRfcomm(fileName, device.address, device.name);
        } else {
            // Для телефонов используем OBEX
            logger->info("Send", "Выбран метод: OBEX для телефонов");
            success = obexSender->sendFileViaObex(fileName, device.address, device.name);
        }
        
        if (success) {
            logger->success("Send", "");
            logger->success("Send", "✓✓✓ ФАЙЛ УСПЕШНО ОТПРАВЛЕН! ✓✓✓");
            logger->info("Send", QString("Файл: %1 (%2 MB)")
                .arg(fileInfo.fileName())
                .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2));
            logger->info("Send", QString("Метод: OBEX (прямой протокол для %1)").arg(deviceType));
            logger->info("Send", "БЕЗ fsquirt - напрямую через сокет!");
            logger->info("Send", "");
            
            QString message;
            if (deviceType == "Компьютер") {
                message = QString("✓ Файл успешно отправлен!\n\n"
                        "Файл: %1 (%2 MB)\n"
                        "Устройство: %3 (Компьютер)\n\n"
                        "Метод: RFCOMM протокол (ПК-ПК)\n\n"
                        "На %3 должен быть запущен Bluetooth сервер!\n"
                        "Файл сохранится в текущую папку как received_file_*.bin\n\n"
                        "ВАЖНО: Убедитесь что на принимающем ПК:\n"
                        "1. Запущен Bluetooth сервер\n"
                        "2. Устройство видимо для других")
                    .arg(fileInfo.fileName())
                    .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2)
                    .arg(device.name);
            } else {
                message = QString("✓ Файл успешно отправлен!\n\n"
                        "Файл: %1 (%2 MB)\n"
                        "Устройство: %3 (%4)\n\n"
                        "Метод: OBEX протокол\n\n"
                        "Файл должен быть принят на устройстве.")
                    .arg(fileInfo.fileName())
                    .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2)
                    .arg(device.name)
                    .arg(deviceType);
            }
            
            QMessageBox::information(this, "Успешно!", message);
                
            QTimer::singleShot(2000, [this]() {
                ui->progressBar->setVisible(false);
            });
        } else {
            ui->progressBar->setVisible(false);
            
            logger->error("Send", "");
            logger->error("Send", "✗✗✗ ОШИБКА ОТПРАВКИ ЧЕРЕЗ OBEX ✗✗✗");
            logger->error("Send", "");
            if (deviceType == "Компьютер") {
                logger->warning("Send", "ВОЗМОЖНЫЕ ПРИЧИНЫ (ПК-ПК):");
                logger->warning("Send", "1. На принимающем ПК не запущен сервер");
                logger->warning("Send", "2. Устройство не видимо для других");
                logger->warning("Send", "3. RFCOMM сервис недоступен");
                logger->warning("Send", "4. Bluetooth драйверы устарели");
                logger->warning("Send", "");
                logger->info("Send", "РЕШЕНИЕ: Запустите сервер на принимающем ПК!");
            } else {
                logger->warning("Send", "ВОЗМОЖНЫЕ ПРИЧИНЫ (Телефон):");
                logger->warning("Send", "1. Телефон отклонил передачу файла");
                logger->warning("Send", "2. Телефон не поддерживает OBEX Push");
                logger->warning("Send", "3. Bluetooth не активен на телефоне");
                logger->warning("Send", "");
            }
            logger->info("Send", "Смотрите детали в send.log и api_calls.log");
            logger->info("Send", "");
            
            QString errorMessage;
            if (deviceType == "Компьютер") {
                errorMessage = QString("Не удалось отправить файл на ПК.\n\n"
                        "Возможные причины:\n"
                        "• На принимающем ПК не запущен сервер\n"
                        "• Устройство не видимо для других\n"
                        "• RFCOMM сервис недоступен\n\n"
                        "РЕШЕНИЕ: Запустите сервер на принимающем ПК!\n\n"
                        "Детали в send.log");
            } else {
                errorMessage = QString("Не удалось отправить файл на телефон.\n\n"
                        "Возможные причины:\n"
                        "• Вы отклонили файл на телефоне\n"
                        "• Телефон не поддерживает OBEX Push\n"
                        "• Bluetooth выключен на телефоне\n\n"
                        "Детали в send.log");
            }
            
            QMessageBox::critical(this, "Ошибка отправки", errorMessage);
        }
    }
}

void BluetoothWindow::onStartServerButtonClicked()
{
    logger->info("Server", "═══════════════════════════════════════");
    logger->info("Server", "ЗАПУСК BLUETOOTH СЕРВЕРА");
    logger->info("Server", "═══════════════════════════════════════");
    logger->info("Server", "");
    logger->info("Server", "Сервер будет принимать файлы от других ПК");
    logger->info("Server", "Порт: 11 (RFCOMM)");
    logger->info("Server", "");
    
    btServer->startServer();
}

void BluetoothWindow::onStopServerButtonClicked()
{
    logger->info("Server", "Остановка Bluetooth сервера...");
    btServer->stopServer();
}

