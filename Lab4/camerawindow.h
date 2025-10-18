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
class AutomaticStealthMode;
class Lab4Logger;

class CameraWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CameraWindow(QWidget *parent = nullptr, QWidget *mainWin = nullptr);
    ~CameraWindow();
    
    // Геттер для проверки скрытого режима
    bool getIsStealthMode() const { return isStealthMode; }
    
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
    
    
    // Автоматический режим

private:
    void setupUI();
    void updateVideoButtonText();
    void setupHotkeys();
    void registerGlobalHotkeys();
    void unregisterGlobalHotkeys();
    
    // Скрытый режим
    void startStealthMode(bool photoMode);
    void stopStealthMode();
    void onStealthTimer();
    void showStealthWarning();
    void createStealthWindow();
    void destroyStealthWindow();
    void forceQuitApplication(); // Принудительное завершение приложения
    
    // Управление поведением завершения приложения
    void enableStealthQuitBehavior(); // Включает поведение для скрытого режима
    void disableStealthQuitBehavior(); // Отключает поведение для скрытого режима

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
    
    
    // Автоматический режим
    
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
    static constexpr int HOTKEY_STOP_STEALTH = 5;
    static constexpr int HOTKEY_FORCE_QUIT = 6;
    bool globalHotkeysRegistered;
    
    // Скрытый режим
    bool isStealthMode;
    bool stealthPhotoMode; // true = фото, false = видео
    QTimer *stealthTimer;
    QWidget *mainWindow; // Ссылка на главное окно для скрытия
    QWidget *stealthWindow; // Невидимое окно-заглушка для поддержки работы приложения
    
    // Состояние автоматического режима
};

#endif // CAMERAWINDOW_H
