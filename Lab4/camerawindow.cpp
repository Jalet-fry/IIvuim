#include "camerawindow.h"
#include "cameraworker.h"
#include "jakecamerawarning.h"
#include "lab4_logger.h"
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>
#include <QDebug>
#include <QCloseEvent>
#include <QProcess>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRadioButton>
#include <windows.h>

CameraWindow::CameraWindow(QWidget *parent, QWidget *mainWin)
    : QWidget(parent),
      cameraWorker(nullptr),
      isRecording(false),
      isPreviewEnabled(true),
      recordingIndicatorVisible(false),
      globalHotkeysRegistered(false),
      isStealthMode(false),
      stealthPhotoMode(true),
      stealthTimer(nullptr),
      mainWindow(mainWin),
      stealthWindow(nullptr)
{
    setWindowTitle("ЛР 4: Работа с веб-камерой (DirectShow API)");
    resize(1000, 700);
    
    // Инициализируем логгер
    Lab4Logger::instance()->logSystemEvent("CameraWindow constructor started");
    
    setupUI();
    
    // Создаем worker для работы с камерой
    cameraWorker = new CameraWorker();
    Lab4Logger::instance()->logCameraEvent("CameraWorker created");
    
    // Создаем Jake предупреждение
    jakeWarning = new JakeCameraWarning(this);
    Lab4Logger::instance()->logJakeEvent("JakeCameraWarning created");
    
    
    // Создаем автоматический режим
    
    // Подключаем сигналы
    connect(cameraWorker, &CameraWorker::videoRecordingStarted, this, &CameraWindow::onVideoRecordingStarted);
    connect(cameraWorker, &CameraWorker::videoRecordingStopped, this, &CameraWindow::onVideoRecordingStopped);
    connect(cameraWorker, &CameraWorker::photoSaved, this, &CameraWindow::onPhotoSaved);
    connect(cameraWorker, &CameraWorker::errorOccurred, this, &CameraWindow::onError);
    connect(cameraWorker, &CameraWorker::cameraInfoReady, this, &CameraWindow::onCameraInfoReady);
    connect(cameraWorker, &CameraWorker::frameReady, this, &CameraWindow::onFrameReady);
    
    
    // Подключаем сигналы автоматического режима
    
    // Настраиваем горячие клавиши
    setupHotkeys();
    
    // Инициализируем состояния
    isRecording = false;
    isPreviewEnabled = false;
    isVideoRecording = false;
    
    // Таймер для мигающего индикатора записи
    recordingBlinkTimer = new QTimer(this);
    connect(recordingBlinkTimer, &QTimer::timeout, this, [this]() {
        recordingIndicatorVisible = !recordingIndicatorVisible;
        if (recordingIndicatorVisible) {
            recordingIndicator->setStyleSheet("QLabel { background-color: red; color: white; padding: 5px; border-radius: 3px; font-weight: bold; }");
            recordingIndicator->setText("● REC");
        } else {
            recordingIndicator->setStyleSheet("QLabel { background-color: transparent; color: transparent; padding: 5px; }");
            recordingIndicator->setText("      ");
        }
    });
    
    // Запускаем превью
    if (cameraWorker->isInitialized()) {
        cameraWorker->startPreview();
        statusLabel->setText("Статус: Превью активно (DirectShow API)");
        
        // Jake предупреждает о включении камеры
        QTimer::singleShot(500, this, [this]() {
            jakeWarning->showWarning(JakeCameraWarning::CAMERA_STARTED);
        });
    } else {
        statusLabel->setText("Статус: Ошибка - камера не найдена");
    }
}

CameraWindow::~CameraWindow()
{
    qDebug() << "CameraWindow destructor called";
    
    // Отменяем регистрацию глобальных горячих клавиш
    unregisterGlobalHotkeys();
    
    if (cameraWorker) {
        qDebug() << "Stopping camera worker...";
        cameraWorker->stopAll();
        delete cameraWorker;
        cameraWorker = nullptr;
    }
    
    // Останавливаем скрытый режим
    if (isStealthMode) {
        qDebug() << "Stopping stealth mode in destructor...";
        stopStealthMode();
    }
    
    // Останавливаем автоматический режим
    
    qDebug() << "CameraWindow destroyed";
}

void CameraWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "=== CAMERA WINDOW CLOSE EVENT ===";
    Lab4Logger::instance()->logStealthModeEvent("CameraWindow closeEvent called");
    
    // Останавливаем запись если идет
    if (isRecording || isVideoRecording) {
        qDebug() << "Stopping recording on close...";
        cameraWorker->stopVideoRecording();
    }
    
    // Останавливаем превью и камеру
    if (cameraWorker) {
        qDebug() << "Stopping all camera operations...";
        cameraWorker->stopAll();
    }
    
    // Скрываем Jake
    if (jakeWarning) {
        jakeWarning->hideWarning();
    }
    
    // Отменяем глобальные горячие клавиши
    unregisterGlobalHotkeys();
    
    
    qDebug() << "CameraWindow closing properly";
    
    event->accept();
}

void CameraWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Заголовок
    QLabel *titleLabel = new QLabel("Управление веб-камерой (DirectShow API - Windows нативный)");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel { font-size: 20px; font-weight: bold; color: #1976D2; padding: 10px; }"
    );
    mainLayout->addWidget(titleLabel);
    
    // Основной контент - сплиттер с превью и информацией
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // Левая часть - превью камеры
    QWidget *previewWidget = new QWidget();
    QVBoxLayout *previewLayout = new QVBoxLayout(previewWidget);
    
    QGroupBox *previewGroup = new QGroupBox("Превью камеры");
    QVBoxLayout *previewGroupLayout = new QVBoxLayout(previewGroup);
    
    // QLabel для отображения камеры
    previewLabel = new QLabel("Ожидание кадров...");
    previewLabel->setMinimumSize(640, 480);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet("QLabel { background-color: #000000; border: 2px solid #1976D2; color: white; }");
    previewLabel->setScaledContents(false); // Сохраняем пропорции
    previewGroupLayout->addWidget(previewLabel);
    
    // Индикатор записи
    recordingIndicator = new QLabel("");
    recordingIndicator->setAlignment(Qt::AlignCenter);
    recordingIndicator->setStyleSheet("QLabel { background-color: transparent; }");
    recordingIndicator->setFixedHeight(30);
    previewGroupLayout->addWidget(recordingIndicator);
    
    previewLayout->addWidget(previewGroup);
    splitter->addWidget(previewWidget);
    
    // Правая часть - информация и управление
    QWidget *controlWidget = new QWidget();
    QVBoxLayout *controlLayout = new QVBoxLayout(controlWidget);
    
    // Информация о камере
    QGroupBox *infoGroup = new QGroupBox("Информация о камере");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    infoTextEdit = new QTextEdit();
    infoTextEdit->setReadOnly(true);
    infoTextEdit->setMaximumHeight(150);
    infoTextEdit->setPlaceholderText("Нажмите 'Получить информацию' для отображения данных камеры");
    infoLayout->addWidget(infoTextEdit);
    
    getCameraInfoBtn = new QPushButton("Получить информацию о камере");
    getCameraInfoBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; padding: 8px; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
    );
    connect(getCameraInfoBtn, &QPushButton::clicked, this, &CameraWindow::onGetCameraInfo);
    infoLayout->addWidget(getCameraInfoBtn);
    
    controlLayout->addWidget(infoGroup);
    
    // Управление
    QGroupBox *controlGroup = new QGroupBox("Управление");
    QVBoxLayout *controlGroupLayout = new QVBoxLayout(controlGroup);
    
    takePhotoBtn = new QPushButton("📷 Сделать фото");
    takePhotoBtn->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; padding: 10px; border-radius: 4px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:pressed { background-color: #0D47A1; }"
    );
    connect(takePhotoBtn, &QPushButton::clicked, this, &CameraWindow::onTakePhoto);
    controlGroupLayout->addWidget(takePhotoBtn);
    
    startStopVideoBtn = new QPushButton("🎥 Начать запись видео");
    startStopVideoBtn->setStyleSheet(
        "QPushButton { background-color: #FF5722; color: white; padding: 10px; border-radius: 4px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #E64A19; }"
        "QPushButton:pressed { background-color: #D84315; }"
    );
    connect(startStopVideoBtn, &QPushButton::clicked, this, &CameraWindow::onStartStopVideo);
    controlGroupLayout->addWidget(startStopVideoBtn);
    
    // Примечание о видео
    QLabel *videoNote = new QLabel("ℹ️ Видеозапись: упрощенная версия (серия кадров)");
    videoNote->setStyleSheet("QLabel { color: #666; font-size: 10px; padding: 2px; }");
    videoNote->setWordWrap(true);
    controlGroupLayout->addWidget(videoNote);
    
    togglePreviewBtn = new QPushButton("⏸ Остановить превью");
    togglePreviewBtn->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; padding: 8px; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #F57C00; }"
        "QPushButton:pressed { background-color: #E65100; }"
    );
    connect(togglePreviewBtn, &QPushButton::clicked, this, &CameraWindow::onTogglePreview);
    controlGroupLayout->addWidget(togglePreviewBtn);
    
    controlLayout->addWidget(controlGroup);
    
    // Скрытый режим
    QGroupBox *stealthGroup = new QGroupBox("🕵️ Скрытый режим");
    QVBoxLayout *stealthLayout = new QVBoxLayout(stealthGroup);
    
    // Выбор режима скрытого наблюдения
    QHBoxLayout *modeLayout = new QHBoxLayout();
    
    QRadioButton *photoModeRadio = new QRadioButton("📸 Скрытое фото");
    photoModeRadio->setChecked(true);
    photoModeRadio->setStyleSheet("QRadioButton { font-weight: bold; }");
    
    QRadioButton *videoModeRadio = new QRadioButton("🎥 Скрытое видео");
    videoModeRadio->setStyleSheet("QRadioButton { font-weight: bold; }");
    
    modeLayout->addWidget(photoModeRadio);
    modeLayout->addWidget(videoModeRadio);
    modeLayout->addStretch();
    
    stealthLayout->addLayout(modeLayout);
    
    // Кнопка запуска скрытого режима
    QPushButton *startStealthBtn = new QPushButton("🚀 Запустить скрытый режим");
    startStealthBtn->setStyleSheet(
        "QPushButton { background-color: #E91E63; color: white; padding: 12px; border-radius: 6px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #C2185B; }"
        "QPushButton:pressed { background-color: #AD1457; }"
    );
    
    connect(startStealthBtn, &QPushButton::clicked, this, [this, photoModeRadio, videoModeRadio]() {
        bool isPhotoMode = photoModeRadio->isChecked();
        startStealthMode(isPhotoMode);
    });
    
    stealthLayout->addWidget(startStealthBtn);
    
    // Информация о скрытом режиме
    QLabel *stealthInfo = new QLabel(
        "⚠️ Скрытый режим:\n"
        "• Полностью скрывает все окна приложения\n"
        "• Работает в фоновом режиме\n"
        "• Горячие клавиши:\n"
        "  - Ctrl+Shift+Q: Показать окна\n"
        "  - Ctrl+Shift+E: Остановить режим\n"
        "  - Ctrl+Shift+X: Принудительное завершение\n\n"
        "⚠️ ВНИМАНИЕ: Используйте только в образовательных целях!"
    );
    stealthInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; padding: 8px; background-color: #FFF3E0; border-radius: 4px; }");
    stealthInfo->setWordWrap(true);
    stealthLayout->addWidget(stealthInfo);
    
    controlLayout->addWidget(stealthGroup);
    
    // Автоматический режим (ваш вариант)
    
    // Статус
    statusLabel = new QLabel("Статус: Готов");
    statusLabel->setStyleSheet(
        "QLabel { background-color: #E3F2FD; padding: 10px; border-radius: 4px; border: 1px solid #1976D2; }"
    );
    statusLabel->setWordWrap(true);
    controlLayout->addWidget(statusLabel);
    
    controlLayout->addStretch();
    
    splitter->addWidget(controlWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
}

void CameraWindow::onGetCameraInfo()
{
    infoTextEdit->setText("Получение информации о камере...");
    cameraWorker->getCameraInfo();
}

void CameraWindow::onTakePhoto()
{
    statusLabel->setText("Статус: Захват фото...");
    cameraWorker->takePhoto();
}

void CameraWindow::onStartStopVideo()
{
    if (!isRecording) {
        statusLabel->setText("Статус: Запись видео...");
        cameraWorker->startVideoRecording();
    } else {
        statusLabel->setText("Статус: Остановка записи...");
        cameraWorker->stopVideoRecording();
    }
}

void CameraWindow::onTogglePreview()
{
    isPreviewEnabled = !isPreviewEnabled;
    
    if (isPreviewEnabled) {
        cameraWorker->startPreview();
        togglePreviewBtn->setText("⏸ Остановить превью");
        statusLabel->setText("Статус: Превью активно");
    } else {
        cameraWorker->stopPreview();
        togglePreviewBtn->setText("▶ Запустить превью");
        statusLabel->setText("Статус: Превью остановлено");
        previewLabel->setText("Превью остановлено");
    }
}

void CameraWindow::onVideoRecordingStarted()
{
    isRecording = true;
    isVideoRecording = true;
    updateVideoButtonText();
    recordingBlinkTimer->start(500);
    statusLabel->setText("Статус: ⏺ ЗАПИСЬ ВИДЕО");
    
    // Jake танцует при записи!
    jakeWarning->showWarning(JakeCameraWarning::RECORDING_STARTED);
}

void CameraWindow::onVideoRecordingStopped()
{
    isRecording = false;
    isVideoRecording = false;
    updateVideoButtonText();
    recordingBlinkTimer->stop();
    recordingIndicator->clear();
    recordingIndicator->setStyleSheet("QLabel { background-color: transparent; }");
    statusLabel->setText("Статус: Запись остановлена");
    
    // Скрываем Jake когда запись остановлена
    jakeWarning->hideWarning();
}

void CameraWindow::onPhotoSaved(const QString &path)
{
    Lab4Logger::instance()->logCameraEvent(QString("Photo saved: %1").arg(path));
    Lab4Logger::instance()->logFileEvent(QString("Photo file created: %1").arg(path));
    
    statusLabel->setText(QString("Статус: ✓ Фото сохранено: %1").arg(path));
    
    // Jake подмигивает при фото!
    jakeWarning->showWarning(JakeCameraWarning::PHOTO_TAKEN);
    Lab4Logger::instance()->logJakeEvent("Jake showed PHOTO_TAKEN warning");
    
    QMessageBox::information(this, "Фото сохранено", "Фотография успешно сохранена:\n" + path);
    Lab4Logger::instance()->logUIEvent("Photo saved message box shown");
}

void CameraWindow::onError(const QString &error)
{
    statusLabel->setText("Статус: ❌ Ошибка: " + error);
    QMessageBox::critical(this, "Ошибка", error);
}

void CameraWindow::onCameraInfoReady(const QString &info)
{
    infoTextEdit->setHtml(info);
}

void CameraWindow::onFrameReady(const QImage &frame)
{
    if (frame.isNull()) {
        return;
    }
    
    // Масштабируем изображение с сохранением пропорций
    QPixmap pixmap = QPixmap::fromImage(frame);
    QPixmap scaled = pixmap.scaled(previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    previewLabel->setPixmap(scaled);
}

void CameraWindow::updateVideoButtonText()
{
    if (isRecording) {
        startStopVideoBtn->setText("⏹ Остановить запись");
        startStopVideoBtn->setStyleSheet(
            "QPushButton { background-color: #D32F2F; color: white; padding: 10px; border-radius: 4px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: #B71C1C; }"
            "QPushButton:pressed { background-color: #8B0000; }"
        );
    } else {
        startStopVideoBtn->setText("🎥 Начать запись видео");
        startStopVideoBtn->setStyleSheet(
            "QPushButton { background-color: #FF5722; color: white; padding: 10px; border-radius: 4px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: #E64A19; }"
            "QPushButton:pressed { background-color: #D84315; }"
        );
    }
}

void CameraWindow::setupHotkeys()
{
    // Эта функция больше не используется
    // Глобальные горячие клавиши регистрируются в registerGlobalHotkeys()
}

void CameraWindow::registerGlobalHotkeys()
{
    if (globalHotkeysRegistered) {
        return; // Уже зарегистрированы
    }
    
    HWND hwnd = (HWND)winId();
    
    // Регистрируем глобальные горячие клавиши Windows
    // MOD_CONTROL | MOD_SHIFT = Ctrl+Shift
    
    // Ctrl+Shift+R - начать запись
    if (RegisterHotKey(hwnd, HOTKEY_START_RECORDING, MOD_CONTROL | MOD_SHIFT, 'R')) {
        qDebug() << "✅ Зарегистрирована горячая клавиша: Ctrl+Shift+R (начать запись)";
    } else {
        qDebug() << "❌ Не удалось зарегистрировать Ctrl+Shift+R";
    }
    
    // Ctrl+Shift+S - остановить запись
    if (RegisterHotKey(hwnd, HOTKEY_STOP_RECORDING, MOD_CONTROL | MOD_SHIFT, 'S')) {
        qDebug() << "✅ Зарегистрирована горячая клавиша: Ctrl+Shift+S (остановить)";
    } else {
        qDebug() << "❌ Не удалось зарегистрировать Ctrl+Shift+S";
    }
    
    // Ctrl+Shift+P - сделать фото
    if (RegisterHotKey(hwnd, HOTKEY_TAKE_PHOTO, MOD_CONTROL | MOD_SHIFT, 'P')) {
        qDebug() << "✅ Зарегистрирована горячая клавиша: Ctrl+Shift+P (фото)";
    } else {
        qDebug() << "❌ Не удалось зарегистрировать Ctrl+Shift+P";
    }
    
    // Ctrl+Shift+Q - показать окно
    if (RegisterHotKey(hwnd, HOTKEY_SHOW_WINDOW, MOD_CONTROL | MOD_SHIFT, 'Q')) {
        qDebug() << "✅ Зарегистрирована горячая клавиша: Ctrl+Shift+Q (показать окно)";
    } else {
        qDebug() << "❌ Не удалось зарегистрировать Ctrl+Shift+Q";
    }
    
    globalHotkeysRegistered = true;
}

void CameraWindow::unregisterGlobalHotkeys()
{
    if (!globalHotkeysRegistered) {
        return;
    }
    
    HWND hwnd = (HWND)winId();
    
    UnregisterHotKey(hwnd, HOTKEY_START_RECORDING);
    UnregisterHotKey(hwnd, HOTKEY_STOP_RECORDING);
    UnregisterHotKey(hwnd, HOTKEY_TAKE_PHOTO);
    UnregisterHotKey(hwnd, HOTKEY_SHOW_WINDOW);
    
    qDebug() << "Глобальные горячие клавиши отменены";
    
    globalHotkeysRegistered = false;
}


bool CameraWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG*>(message);
        
        if (msg->message == WM_HOTKEY) {
            int hotkeyId = msg->wParam;
            
            qDebug() << "Получена горячая клавиша:" << hotkeyId;
            
            switch (hotkeyId) {
                case HOTKEY_START_RECORDING:
                    qDebug() << "Ctrl+Shift+R - НАЧАТЬ запись видео";
                    if (!isRecording && !isVideoRecording) {
                        statusLabel->setText("Статус: Запись видео (горячая клавиша)...");
                        cameraWorker->startVideoRecording();
                    } else {
                        qDebug() << "Запись уже идет!";
                    }
                    return true;
                
                case HOTKEY_STOP_RECORDING:
                    qDebug() << "Ctrl+Shift+S - ОСТАНОВИТЬ запись";
                    qDebug() << "isRecording:" << isRecording << "isVideoRecording:" << isVideoRecording;
                    // Напрямую останавливаем запись
                    if (isRecording || isVideoRecording) {
                        statusLabel->setText("Статус: Остановка записи (горячая клавиша)...");
                        cameraWorker->stopVideoRecording();
                    } else {
                        qDebug() << "Запись не идет!";
                    }
                    return true;
                    
                case HOTKEY_TAKE_PHOTO:
                    qDebug() << "Ctrl+Shift+P - сделать фото";
                    statusLabel->setText("Статус: Захват фото (горячая клавиша)...");
                    onTakePhoto();
                    return true;
                    
                case HOTKEY_SHOW_WINDOW:
                    qDebug() << "Ctrl+Shift+Q - показать окно";
                    
                    // Автоматически останавливаем запись при показе окна
                    if (isRecording || isVideoRecording) {
                        qDebug() << "Автоматически останавливаем запись при показе окна";
                        cameraWorker->stopVideoRecording();
                        // Обновляем UI
                        isRecording = false;
                        isVideoRecording = false;
                        updateVideoButtonText();
                        recordingBlinkTimer->stop();
                        recordingIndicator->clear();
                        recordingIndicator->setStyleSheet("QLabel { background-color: transparent; }");
                    }
                    
                    // Скрываем Jake
                    jakeWarning->hideWarning();
                    
                    unregisterGlobalHotkeys(); // Отменяем глобальные клавиши
                    this->show();
                    this->raise();
                    this->activateWindow();
                    
                    if (mainWindow) {
                        mainWindow->show();
                        mainWindow->raise();
                        mainWindow->activateWindow();
                    }
                    
                    // Если в скрытом режиме, останавливаем его
                    if (isStealthMode) {
                        stopStealthMode();
                    }
                    
                    statusLabel->setText("Статус: Окно восстановлено");
                    return true;
                    
                case HOTKEY_STOP_STEALTH:
                    qDebug() << "Ctrl+Shift+E - остановить скрытый режим";
                    if (isStealthMode) {
                        stopStealthMode();
                    }
                    return true;
                    
                case HOTKEY_FORCE_QUIT:
                    qDebug() << "Ctrl+Shift+X - принудительное завершение приложения";
                    forceQuitApplication();
                    return true;
            }
        }
    }
    
    return QWidget::nativeEvent(eventType, message, result);
}

// Методы скрытого режима
void CameraWindow::startStealthMode(bool photoMode)
{
    qDebug() << "=== START STEALTH MODE ===";
    Lab4Logger::instance()->logStealthModeEvent("Starting stealth mode");
    
    if (isStealthMode) {
        qDebug() << "Stealth mode already active!";
        QMessageBox::information(this, "Скрытый режим", "Скрытый режим уже активен!");
        return;
    }
    
    qDebug() << "Setting stealth mode parameters...";
    stealthPhotoMode = photoMode;
    isStealthMode = true;
    
    qDebug() << "Hiding windows...";
    Lab4Logger::instance()->logStealthModeEvent("Hiding windows");
    
    // Создаем невидимое окно-заглушку для поддержки работы приложения
    qDebug() << "Creating stealth window...";
    createStealthWindow();
    
    // Проверяем, что невидимое окно действительно создано и видимо
    if (stealthWindow && stealthWindow->isVisible()) {
        qDebug() << "Stealth window successfully created and visible";
        Lab4Logger::instance()->logStealthModeEvent("Stealth window created successfully");
    } else {
        qDebug() << "ERROR: Stealth window not created or not visible!";
        qDebug() << "Stealth window pointer:" << stealthWindow;
        if (stealthWindow) {
            qDebug() << "Stealth window visible:" << stealthWindow->isVisible();
        }
        Lab4Logger::instance()->logStealthModeEvent("ERROR: Stealth window creation failed");
        
        // Если не удалось создать окно-заглушку, отменяем скрытый режим
        isStealthMode = false;
        QMessageBox::critical(this, "Ошибка", "Не удалось создать невидимое окно-заглушку!\nСкрытый режим отменен.");
        return;
    }
    
    // Скрываем все окна
    this->hide();
    if (mainWindow) {
        mainWindow->hide();
        qDebug() << "Main window hidden";
    }
    qDebug() << "Camera window hidden";
    
    // Регистрируем дополнительные горячие клавиши для скрытого режима
    qDebug() << "Registering stealth hotkeys...";
    RegisterHotKey((HWND)this->winId(), HOTKEY_SHOW_WINDOW, MOD_CONTROL | MOD_SHIFT, 'Q');
    RegisterHotKey((HWND)this->winId(), HOTKEY_STOP_STEALTH, MOD_CONTROL | MOD_SHIFT, 'E');
    RegisterHotKey((HWND)this->winId(), HOTKEY_FORCE_QUIT, MOD_CONTROL | MOD_SHIFT, 'X');
    qDebug() << "Stealth hotkeys registered";
    
    // Создаем таймер для периодического захвата
    qDebug() << "Creating stealth timer...";
    stealthTimer = new QTimer(this);
    connect(stealthTimer, &QTimer::timeout, this, &CameraWindow::onStealthTimer);
    
    // Запускаем таймер (каждые 5 секунд)
    stealthTimer->start(5000);
    qDebug() << "Stealth timer started (5 seconds interval)";
    
    // Показываем предупреждение от Джейка
    qDebug() << "Showing stealth warning...";
    showStealthWarning();
    
    statusLabel->setText(QString("Статус: 🕵️ Скрытый режим активен (%1)").arg(photoMode ? "фото" : "видео"));
    
    Lab4Logger::instance()->logStealthDaemonEvent(QString("Stealth mode started: %1").arg(photoMode ? "photo" : "video"));
    
    qDebug() << "Showing stealth mode information dialog...";
    QMessageBox::information(this, "Скрытый режим", 
        QString("🕵️ Скрытый режим запущен!\n\n"
               "Режим: %1\n"
               "• Все окна скрыты\n"
               "• Захват каждые 5 секунд\n"
               "• Горячие клавиши:\n"
               "  - Ctrl+Shift+Q: Показать окна\n"
               "  - Ctrl+Shift+E: Остановить режим\n\n"
               "⚠️ ВНИМАНИЕ: Используйте только в образовательных целях!")
        .arg(photoMode ? "Скрытое фото" : "Скрытое видео"));
    
    qDebug() << "=== STEALTH MODE STARTED SUCCESSFULLY ===";
    Lab4Logger::instance()->logStealthModeEvent("Stealth mode started successfully");
}

void CameraWindow::stopStealthMode()
{
    qDebug() << "=== STOP STEALTH MODE ===";
    Lab4Logger::instance()->logStealthModeEvent("Stopping stealth mode");
    
    if (!isStealthMode) {
        qDebug() << "Stealth mode not active, nothing to stop";
        return;
    }
    
    isStealthMode = false;
    qDebug() << "Stealth mode stopped, app quit allowed";
    
    // Останавливаем таймер
    if (stealthTimer) {
        stealthTimer->stop();
        stealthTimer->deleteLater();
        stealthTimer = nullptr;
    }
    
    // Отменяем регистрацию горячих клавиш скрытого режима
    UnregisterHotKey((HWND)this->winId(), HOTKEY_SHOW_WINDOW);
    UnregisterHotKey((HWND)this->winId(), HOTKEY_STOP_STEALTH);
    UnregisterHotKey((HWND)this->winId(), HOTKEY_FORCE_QUIT);
    
    // Уничтожаем невидимое окно-заглушку
    qDebug() << "Destroying stealth window...";
    destroyStealthWindow();
    
    // Показываем окна
    this->show();
    this->raise();
    this->activateWindow();
    
    if (mainWindow) {
        mainWindow->show();
        mainWindow->raise();
        mainWindow->activateWindow();
    }
    
    statusLabel->setText("Статус: Скрытый режим остановлен");
    
    Lab4Logger::instance()->logStealthDaemonEvent("Stealth mode stopped");
    
    QMessageBox::information(this, "Скрытый режим", "Скрытый режим остановлен!");
}

void CameraWindow::onStealthTimer()
{
    if (!isStealthMode) {
        return;
    }
    
    if (stealthPhotoMode) {
        // Скрытое фото
        statusLabel->setText("Статус: 🕵️ Скрытое фото...");
        cameraWorker->takePhoto();
        
        // Показываем предупреждение от Джейка
        showStealthWarning();
        
        Lab4Logger::instance()->logStealthDaemonEvent("Stealth photo taken");
    } else {
        // Скрытое видео (короткое - 3 секунды)
        if (!isRecording) {
            statusLabel->setText("Статус: 🕵️ Скрытое видео...");
            cameraWorker->startVideoRecording();
            
            // Показываем предупреждение от Джейка
            showStealthWarning();
            
            // Останавливаем запись через 3 секунды
            QTimer::singleShot(3000, this, [this]() {
                if (isRecording) {
                    cameraWorker->stopVideoRecording();
                }
            });
            
            Lab4Logger::instance()->logStealthDaemonEvent("Stealth video recorded");
        }
    }
}

void CameraWindow::showStealthWarning()
{
    if (jakeWarning) {
        // Показываем предупреждение от Джейка о скрытом наблюдении
        jakeWarning->showWarning(JakeCameraWarning::STEALTH_MODE);
    }
}

void CameraWindow::createStealthWindow()
{
    qDebug() << "Creating stealth window...";
    
    // Создаем невидимое окно-заглушку с правильными флагами для Qt 5.5.1
    stealthWindow = new QWidget();
    
    // Устанавливаем флаги окна для невидимости, но поддержки работы приложения
    stealthWindow->setWindowFlags(
        Qt::Tool | 
        Qt::FramelessWindowHint | 
        Qt::WindowStaysOnTopHint |
        Qt::WindowDoesNotAcceptFocus
    );
    
    // Делаем окно полностью прозрачным
    stealthWindow->setAttribute(Qt::WA_TranslucentBackground, true);
    stealthWindow->setAttribute(Qt::WA_NoSystemBackground, true);
    stealthWindow->setAttribute(Qt::WA_ShowWithoutActivating, true);
    
    // Минимальный размер и позиция за экраном
    stealthWindow->setFixedSize(1, 1);
    stealthWindow->move(-1000, -1000);
    stealthWindow->setWindowTitle("Stealth Window");
    
    // КРИТИЧЕСКИ ВАЖНО: Делаем окно видимым для Qt, но невидимым для пользователя
    stealthWindow->setVisible(true);
    stealthWindow->show();
    
    // Дополнительно убеждаемся, что окно остается активным
    stealthWindow->raise();
    
    qDebug() << "Stealth window created and shown";
    qDebug() << "Stealth window is visible:" << stealthWindow->isVisible();
    qDebug() << "Stealth window geometry:" << stealthWindow->geometry();
    qDebug() << "Stealth window windowFlags:" << stealthWindow->windowFlags();
}

void CameraWindow::destroyStealthWindow()
{
    qDebug() << "Destroying stealth window...";
    
    if (stealthWindow) {
        // Сначала скрываем окно
        stealthWindow->hide();
        stealthWindow->setVisible(false);
        
        // Затем удаляем его
        stealthWindow->deleteLater();
        stealthWindow = nullptr;
        
        qDebug() << "Stealth window destroyed";
    } else {
        qDebug() << "No stealth window to destroy";
    }
}

void CameraWindow::forceQuitApplication()
{
    qDebug() << "=== FORCE QUIT APPLICATION ===";
    Lab4Logger::instance()->logStealthModeEvent("Force quit application requested");
    
    // Останавливаем скрытый режим
    if (isStealthMode) {
        stopStealthMode();
    }
    
    // Останавливаем камеру
    if (cameraWorker) {
        cameraWorker->stopAll();
    }
    
    // Принудительно завершаем приложение
    qDebug() << "Forcing application quit...";
    QCoreApplication::quit();
}











