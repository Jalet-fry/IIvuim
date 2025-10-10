#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QCamera>
#include <QCameraImageCapture>
#include <QVideoFrame>
#include <QTimer>

class CameraWorker : public QObject
{
    Q_OBJECT

public:
    explicit CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

    // Получить объект камеры для превью
    QCamera* getCamera() const { return camera; }
    
    // Управление превью
    void startPreview();
    void stopPreview();
    
    // Захват фото
    void takePhoto();
    
    // Запись видео
    void startVideoRecording();
    void stopVideoRecording();
    
    // Получение информации о камере
    void getCameraInfo();
    
    // Остановка всего
    void stopAll();

signals:
    void frameReady(const QImage &frame);
    void videoRecordingStarted();
    void videoRecordingStopped();
    void photoSaved(const QString &path);
    void errorOccurred(const QString &error);
    void cameraInfoReady(const QString &info);

private slots:
    void onImageCaptured(int id, const QImage &preview);
    void onImageSaved(int id, const QString &fileName);
    void onCaptureError(int id, QCameraImageCapture::Error error, const QString &errorString);
    void onCameraError(QCamera::Error error);
    void captureVideoFrame();

private:
    // Инициализация камеры
    bool initializeCamera();
    void releaseCamera();
    
    // Генерация имени файла с датой и временем
    QString generateFileName(const QString &prefix, const QString &extension);
    
    // Создание папки для сохранения
    QString getOutputDirectory();
    
    // Получение информации о камере через Windows API
    QString getCameraInfoWindows();
    
    // Qt Multimedia объекты
    QCamera *camera;
    QCameraImageCapture *imageCapture;
    
    // Таймер для захвата кадров (для превью и видео)
    QTimer *captureTimer;
    QTimer *videoFrameTimer;
    
    // Состояния
    bool isPreviewActive;
    bool isRecordingVideo;
    bool isCameraInitialized;
    
    // Для записи видео (простая реализация через серию кадров)
    QList<QImage> videoFrames;
    QString currentVideoPath;
    int videoFrameCount;
};

#endif // CAMERAWORKER_H
