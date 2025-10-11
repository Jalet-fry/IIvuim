#ifndef CAMERAWINDOW_H
#define CAMERAWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTimer>
#include <QShortcut>
#include <QAbstractNativeEventFilter>

class CameraWorker;
class JakeCameraWarning;

class CameraWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CameraWindow(QWidget *parent = nullptr);
    ~CameraWindow();
    
protected:
    // Для обработки глобальных горячих клавиш Windows
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    
    // Обработка закрытия окна
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onGetCameraInfo();
    void onTakePhoto();
    void onStartStopVideo();
    void onTogglePreview();
    void onVideoRecordingStarted();
    void onVideoRecordingStopped();
    void onPhotoSaved(const QString &path);
    void onError(const QString &error);
    void onCameraInfoReady(const QString &info);
    void onFrameReady(const QImage &frame);

private:
    void setupUI();
    void updateVideoButtonText();
    void setupHotkeys();
    void registerGlobalHotkeys();
    void unregisterGlobalHotkeys();

    // UI элементы
    QLabel *previewLabel;           // Для отображения камеры
    QTextEdit *infoTextEdit;
    QPushButton *getCameraInfoBtn;
    QPushButton *takePhotoBtn;
    QPushButton *startStopVideoBtn;
    QPushButton *togglePreviewBtn;
    QLabel *statusLabel;
    QLabel *recordingIndicator;
    
    // Логика камеры
    CameraWorker *cameraWorker;
    
    // Jake предупреждение
    JakeCameraWarning *jakeWarning;
    
    // Состояния
    bool isRecording;
    bool isPreviewEnabled;
    bool isVideoRecording;
    
    // Таймер для индикатора записи
    QTimer *recordingBlinkTimer;
    bool recordingIndicatorVisible;
    
    // ID для глобальных горячих клавиш Windows
    static constexpr int HOTKEY_START_RECORDING = 1;
    static constexpr int HOTKEY_STOP_RECORDING = 2;
    static constexpr int HOTKEY_TAKE_PHOTO = 3;
    static constexpr int HOTKEY_SHOW_WINDOW = 4;
    bool globalHotkeysRegistered;
};

#endif // CAMERAWINDOW_H
