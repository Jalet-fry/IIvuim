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
    
    // Подключаем сигнал выбора устройства в таблице
    connect(ui->devicesTable, &QTableWidget::itemSelectionChanged,
            this, &BluetoothWindow::onDeviceSelectionChanged);
    
    // Начальное состояние кнопок
    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(false);
    ui->sendFileButton->setEnabled(false);

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
    // 3. Либо есть RFCOMM подключение (для ноутбуков), либо устройство поддерживает OBEX (для телефонов)
    if (deviceSelected && canSendFiles) {
        const BluetoothDeviceData &device = discoveredDevices[selectedDeviceIndex];
        BluetoothDeviceCapabilities caps = device.getCapabilities();
        
        // Для устройств с RFCOMM (ноутбуки) - нужно подключение
        // Для устройств с OBEX (телефоны) - можно без подключения
        if (caps.supportsRFCOMM) {
            ui->sendFileButton->setEnabled(isDeviceConnected);
        } else if (caps.supportsOBEX) {
            ui->sendFileButton->setEnabled(true);  // OBEX работает через fsquirt без подключения
        } else {
            ui->sendFileButton->setEnabled(false);
        }
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
    bool useRFCOMM = false;
    bool useOBEX = false;
    
    if (caps.supportsRFCOMM && isDeviceConnected) {
        // Есть RFCOMM подключение - используем прямую отправку (для ноутбуков)
        useRFCOMM = true;
        logger->success("Send", "✓ RFCOMM ПОДКЛЮЧЕНИЕ АКТИВНО!");
        logger->info("Send", "Будет использована ПРЯМАЯ отправка через сокет");
    } else if (caps.supportsOBEX) {
        // Устройство поддерживает OBEX - используем ПРЯМОЙ OBEX протокол (для телефонов)
        useOBEX = true;
        logger->success("Send", "✓ Устройство поддерживает OBEX");
        logger->info("Send", "Будет использована ПРЯМАЯ отправка через OBEX протокол");
        logger->info("Send", "БЕЗ fsquirt - напрямую через сокет!");
    } else if (caps.supportsRFCOMM && !isDeviceConnected) {
        // Ноутбук но не подключено
        logger->error("Send", "");
        logger->error("Send", "✗ НЕТ RFCOMM ПОДКЛЮЧЕНИЯ!");
        logger->error("Send", "");
        logger->warning("Send", "Это устройство требует RFCOMM подключения:");
        logger->info("Send", "1. Нажмите кнопку 'Подключить'");
        logger->info("Send", "2. Дождитесь подключения (5-30 сек)");
        logger->info("Send", "3. Повторите отправку");
        logger->info("Send", "");
        
        QMessageBox::critical(this, "Нет подключения",
            QString("Устройство %1 требует RFCOMM подключения!\n\n"
                    "Что делать:\n"
                    "1. Нажмите кнопку 'Подключить'\n"
                    "2. Дождитесь подключения\n"
                    "3. Повторите отправку")
            .arg(device.name));
        return;
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
    
    if (useRFCOMM) {
        // ПРЯМАЯ ОТПРАВКА через RFCOMM (ноутбук → ноутбук)
        logger->info("Send", "Метод: ПРЯМАЯ ОТПРАВКА через RFCOMM сокет");
        logger->info("Send", "");
        
        ui->progressBar->setVisible(true);
        ui->progressBar->setValue(0);
        
        success = fileSender->sendFile(fileName, device.address, device.name, btConnection);
        
        if (success) {
            logger->success("Send", "");
            logger->success("Send", "✓✓✓ ФАЙЛ УСПЕШНО ОТПРАВЛЕН! ✓✓✓");
            logger->info("Send", QString("Файл: %1 (%2 MB)")
                .arg(fileInfo.fileName())
                .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2));
            logger->info("Send", "Метод: RFCOMM (прямое подключение)");
            logger->info("Send", "");
            
            QMessageBox::information(this, "Успешно!",
                QString("✓ Файл успешно отправлен!\n\n"
                        "Файл: %1 (%2 MB)\n"
                        "Устройство: %2\n\n"
                        "Метод: Прямая отправка через\n"
                        "RFCOMM сокет (ноутбук → ноутбук)")
                .arg(fileInfo.fileName())
                .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2)
                .arg(device.name));
                
            QTimer::singleShot(2000, [this]() {
                ui->progressBar->setVisible(false);
            });
        } else {
            ui->progressBar->setVisible(false);
            
            logger->error("Send", "");
            logger->error("Send", "✗✗✗ ОШИБКА ОТПРАВКИ ✗✗✗");
            logger->error("Send", "Смотрите send.log для деталей");
            logger->error("Send", "");
            
            QMessageBox::critical(this, "Ошибка отправки",
                QString("Не удалось отправить файл через RFCOMM.\n\n"
                        "Проверьте:\n"
                        "• Подключение не разорвалось\n"
                        "• На другом ноутбуке запущен Lab6\n"
                        "• Достаточно памяти\n\n"
                        "Детали в send.log"));
        }
        
    } else if (useOBEX) {
        // ПРЯМАЯ ОТПРАВКА через OBEX протокол для телефонов (БЕЗ fsquirt!)
        logger->info("Send", "Метод: OBEX протокол (прямая отправка на Android)");
        logger->info("Send", "БЕЗ FSQUIRT - только OBEX через сокет!");
        logger->info("Send", "");
        
        ui->progressBar->setVisible(true);
        ui->progressBar->setValue(0);
        
        // Используем OBEX напрямую для телефонов
        success = obexSender->sendFileViaObex(fileName, device.address, device.name);
        
        if (success) {
            logger->success("Send", "");
            logger->success("Send", "✓✓✓ ФАЙЛ УСПЕШНО ОТПРАВЛЕН! ✓✓✓");
            logger->info("Send", QString("Файл: %1 (%2 MB)")
                .arg(fileInfo.fileName())
                .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2));
            logger->info("Send", "Метод: OBEX (прямой протокол для Android)");
            logger->info("Send", "БЕЗ fsquirt - напрямую через сокет!");
            logger->info("Send", "");
            
            QMessageBox::information(this, "Успешно!",
                QString("✓ Файл успешно отправлен на телефон!\n\n"
                        "Файл: %1 (%2 MB)\n"
                        "Устройство: %3\n\n"
                        "Метод: OBEX протокол\n"
                        "(прямая отправка БЕЗ fsquirt!)\n\n"
                        "На телефоне файл должен был быть принят.")
                .arg(fileInfo.fileName())
                .arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2)
                .arg(device.name));
                
            QTimer::singleShot(2000, [this]() {
                ui->progressBar->setVisible(false);
            });
        } else {
            ui->progressBar->setVisible(false);
            
            logger->error("Send", "");
            logger->error("Send", "✗✗✗ ОШИБКА ОТПРАВКИ ЧЕРЕЗ OBEX ✗✗✗");
            logger->error("Send", "");
            logger->warning("Send", "ВОЗМОЖНЫЕ ПРИЧИНЫ:");
            logger->warning("Send", "1. Телефон отклонил передачу файла");
            logger->warning("Send", "2. Телефон не поддерживает OBEX Push");
            logger->warning("Send", "3. Bluetooth не активен на телефоне");
            logger->warning("Send", "");
            logger->info("Send", "Смотрите детали в send.log и api_calls.log");
            logger->info("Send", "");
            
            QMessageBox::critical(this, "Ошибка OBEX отправки",
                QString("Не удалось отправить файл через OBEX.\n\n"
                        "Возможные причины:\n"
                        "• Вы отклонили файл на телефоне\n"
                        "• Телефон не поддерживает OBEX Push\n"
                        "• Bluetooth выключен на телефоне\n\n"
                        "Детали в send.log"));
        }
    }
}

