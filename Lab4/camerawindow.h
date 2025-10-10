#ifndef CAMERAWINDOW_H
#define CAMERAWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QCheckBox>
#include <QTimer>
#include <QCameraViewfinder>

class CameraWorker;

class CameraWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CameraWindow(QWidget *parent = nullptr);
    ~CameraWindow();

private slots:
    void onGetCameraInfo();
    void onTakePhoto();
    void onStartStopVideo();
    void onTogglePreview();
    void onToggleStealthMode();
    void onVideoRecordingStarted();
    void onVideoRecordingStopped();
    void onPhotoSaved(const QString &path);
    void onError(const QString &error);
    void onCameraInfoReady(const QString &info);

private:
    void setupUI();
    void updateVideoButtonText();

    // UI элементы
    QCameraViewfinder *viewfinder;  // Для отображения камеры
    QTextEdit *infoTextEdit;
    QPushButton *getCameraInfoBtn;
    QPushButton *takePhotoBtn;
    QPushButton *startStopVideoBtn;
    QPushButton *togglePreviewBtn;
    QLabel *statusLabel;
    QLabel *recordingIndicator;
    
    // Логика камеры
    CameraWorker *cameraWorker;
    
    // Состояния
    bool isRecording;
    bool isPreviewEnabled;
    bool isStealthMode;
    
    // Таймер для индикатора записи
    QTimer *recordingBlinkTimer;
    bool recordingIndicatorVisible;
};

#endif // CAMERAWINDOW_H
