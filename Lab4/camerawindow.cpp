#include "camerawindow.h"
#include "cameraworker.h"
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>
#include <QCamera>

CameraWindow::CameraWindow(QWidget *parent)
    : QWidget(parent),
      cameraWorker(nullptr),
      isRecording(false),
      isPreviewEnabled(true),
      isStealthMode(false),
      recordingIndicatorVisible(false)
{
    setWindowTitle("ЛР 4: Работа с веб-камерой (Qt Multimedia)");
    resize(1000, 700);
    
    setupUI();
    
    // Создаем worker для работы с камерой
    cameraWorker = new CameraWorker();
    
    // Подключаем сигналы
    connect(cameraWorker, &CameraWorker::videoRecordingStarted, this, &CameraWindow::onVideoRecordingStarted);
    connect(cameraWorker, &CameraWorker::videoRecordingStopped, this, &CameraWindow::onVideoRecordingStopped);
    connect(cameraWorker, &CameraWorker::photoSaved, this, &CameraWindow::onPhotoSaved);
    connect(cameraWorker, &CameraWorker::errorOccurred, this, &CameraWindow::onError);
    connect(cameraWorker, &CameraWorker::cameraInfoReady, this, &CameraWindow::onCameraInfoReady);
    
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
    
    // Подключаем камеру к viewfinder и запускаем
    QCamera *camera = cameraWorker->getCamera();
    if (camera) {
        camera->setViewfinder(viewfinder);
        cameraWorker->startPreview();
        statusLabel->setText("Статус: Превью активно (Qt Multimedia)");
    } else {
        statusLabel->setText("Статус: Ошибка - камера не найдена");
    }
}

CameraWindow::~CameraWindow()
{
    if (cameraWorker) {
        cameraWorker->stopAll();
        delete cameraWorker;
    }
}

void CameraWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Заголовок
    QLabel *titleLabel = new QLabel("Управление веб-камерой (Qt Multimedia - без OpenCV!)");
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
    
    // QCameraViewfinder для отображения камеры
    viewfinder = new QCameraViewfinder();
    viewfinder->setMinimumSize(480, 360);
    viewfinder->setStyleSheet("QCameraViewfinder { background-color: #000000; border: 2px solid #1976D2; }");
    previewGroupLayout->addWidget(viewfinder);
    
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
    
    startStopVideoBtn = new QPushButton("🎥 Запись видео (не поддерживается)");
    startStopVideoBtn->setEnabled(false);
    startStopVideoBtn->setStyleSheet(
        "QPushButton { background-color: #999; color: white; padding: 10px; border-radius: 4px; font-size: 14px; font-weight: bold; }"
    );
    connect(startStopVideoBtn, &QPushButton::clicked, this, &CameraWindow::onStartStopVideo);
    controlGroupLayout->addWidget(startStopVideoBtn);
    
    // Примечание о видео
    QLabel *videoNote = new QLabel("⚠️ Видеозапись требует системные кодеки.\nИспользуйте захват фото вместо видео.");
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
            "Используйте 'Сделать фото' перед скрытием,\n"
            "затем нажмите эту кнопку.\n\n"
            "Для возврата закройте приложение через\n"
            "диспетчер задач или Alt+F4.");
        this->hide();
    });
    stealthLayout->addWidget(hideWindowBtn);
    
    QLabel *stealthInfo = new QLabel(
        "⚠️ Скрытый режим:\n"
        "• Нажмите кнопку выше чтобы скрыть окно\n"
        "• Камера продолжит работать\n"
        "• Для фото используйте таймер (Alt+Tab)"
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
        statusLabel->setText("Статус: Запись видео (упрощенная)...");
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
    }
}

void CameraWindow::onToggleStealthMode()
{
    // Убрана checkbox, используется кнопка
}

void CameraWindow::onVideoRecordingStarted()
{
    isRecording = true;
    updateVideoButtonText();
    recordingBlinkTimer->start(500);
    statusLabel->setText("Статус: ⏺ ЗАПИСЬ (упрощенная реализация)");
}

void CameraWindow::onVideoRecordingStopped()
{
    isRecording = false;
    updateVideoButtonText();
    recordingBlinkTimer->stop();
    recordingIndicator->clear();
    recordingIndicator->setStyleSheet("QLabel { background-color: transparent; }");
    statusLabel->setText("Статус: Запись остановлена");
}

void CameraWindow::onPhotoSaved(const QString &path)
{
    statusLabel->setText(QString("Статус: ✓ Фото сохранено: %1").arg(path));
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

