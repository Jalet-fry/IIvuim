#include "camerawindow.h"
#include "cameraworker.h"
#include "jakecamerawarning.h"
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>
#include <QDebug>
#include <QCloseEvent>
#include <windows.h>

CameraWindow::CameraWindow(QWidget *parent)
    : QWidget(parent),
      cameraWorker(nullptr),
      isRecording(false),
      isPreviewEnabled(true),
      recordingIndicatorVisible(false),
      globalHotkeysRegistered(false)
{
    setWindowTitle("ЛР 4: Работа с веб-камерой (DirectShow API)");
    resize(1000, 700);
    
    setupUI();
    
    // Создаем worker для работы с камерой
    cameraWorker = new CameraWorker();
    
    // Создаем Jake предупреждение
    jakeWarning = new JakeCameraWarning(this);
    
    // Подключаем сигналы
    connect(cameraWorker, &CameraWorker::videoRecordingStarted, this, &CameraWindow::onVideoRecordingStarted);
    connect(cameraWorker, &CameraWorker::videoRecordingStopped, this, &CameraWindow::onVideoRecordingStopped);
    connect(cameraWorker, &CameraWorker::photoSaved, this, &CameraWindow::onPhotoSaved);
    connect(cameraWorker, &CameraWorker::errorOccurred, this, &CameraWindow::onError);
    connect(cameraWorker, &CameraWorker::cameraInfoReady, this, &CameraWindow::onCameraInfoReady);
    connect(cameraWorker, &CameraWorker::frameReady, this, &CameraWindow::onFrameReady);
    
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
    
    qDebug() << "CameraWindow destroyed";
}

void CameraWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "CameraWindow closeEvent called";
    
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
    
    // Режим невидимости
    QGroupBox *stealthGroup = new QGroupBox("🕵️ Скрытый режим");
    QVBoxLayout *stealthLayout = new QVBoxLayout(stealthGroup);
    
    QPushButton *hideWindowBtn = new QPushButton("Скрыть окно (скрытый режим)");
    hideWindowBtn->setStyleSheet(
        "QPushButton { background-color: #9C27B0; color: white; padding: 8px; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #7B1FA2; }"
    );
    connect(hideWindowBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Скрытый режим",
            "Окно будет скрыто.\n"
            "Камера продолжит работать.\n\n"
            "🔥 ГЛОБАЛЬНЫЕ ГОРЯЧИЕ КЛАВИШИ:\n"
            "• Ctrl+Shift+R - НАЧАТЬ запись видео\n"
            "• Ctrl+Shift+S - ОСТАНОВИТЬ запись видео\n"
            "• Ctrl+Shift+P - Сделать фото\n"
            "• Ctrl+Shift+Q - Показать окно (+ автостоп записи)\n\n"
            "Работают ВЕЗДЕ в Windows!\n\n"
            "Для принудительного выхода используйте\n"
            "диспетчер задач (Ctrl+Shift+Esc)");
        
        // Регистрируем глобальные горячие клавиши при скрытии
        registerGlobalHotkeys();
        
        // Jake предупреждает о скрытом режиме!
        jakeWarning->showWarning(JakeCameraWarning::STEALTH_MODE);
        
        this->hide();
    });
    stealthLayout->addWidget(hideWindowBtn);
    
    QLabel *stealthInfo = new QLabel(
        "⚠️ Скрытый режим:\n"
        "• Нажмите кнопку выше чтобы скрыть окно\n"
        "• Камера продолжит работать в фоне\n"
        "• Можно делать фото/видео незаметно"
    );
    stealthInfo->setStyleSheet("QLabel { color: #666; font-size: 11px; padding: 5px; }");
    stealthInfo->setWordWrap(true);
    stealthLayout->addWidget(stealthInfo);
    
    controlLayout->addWidget(stealthGroup);
    
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
    statusLabel->setText(QString("Статус: ✓ Фото сохранено: %1").arg(path));
    
    // Jake подмигивает при фото!
    jakeWarning->showWarning(JakeCameraWarning::PHOTO_TAKEN);
    
    QMessageBox::information(this, "Фото сохранено", "Фотография успешно сохранена:\n" + path);
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
                    statusLabel->setText("Статус: Окно восстановлено");
                    return true;
            }
        }
    }
    
    return QWidget::nativeEvent(eventType, message, result);
}
