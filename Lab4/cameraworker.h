#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QTimer>
#include <QMutex>
#include <windows.h>
#include <dshow.h>

// Forward declarations
struct ISampleGrabber;

class CameraWorker : public QObject
{
    Q_OBJECT

public:
    explicit CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

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
    
    // Проверка инициализации
    bool isInitialized() const { return m_initialized; }

signals:
    void frameReady(const QImage &frame);
    void videoRecordingStarted();
    void videoRecordingStopped();
    void photoSaved(const QString &path);
    void errorOccurred(const QString &error);
    void cameraInfoReady(const QString &info);

private slots:
    void captureFrame();

private:
    // Инициализация DirectShow
    bool initializeDirectShow();
    void releaseDirectShow();
    
    // Генерация имени файла с датой и временем
    QString generateFileName(const QString &prefix, const QString &extension);
    
    // Создание папки для сохранения
    QString getOutputDirectory();
    
    // Получение информации о камере через Windows API
    QString getCameraInfoWindows();
    
    // Захват текущего кадра из DirectShow
    QImage captureCurrentFrame();
    
    // Сохранение кадра в файл
    bool saveFrame(const QImage &frame, const QString &filePath);
    
    // Сохранение видео в AVI файл
    bool saveVideoToAVI(const QString &filePath, const QList<QImage> &frames, int width, int height);
    
    // DirectShow интерфейсы
    IGraphBuilder *m_pGraph;
    ICaptureGraphBuilder2 *m_pCapture;
    IMediaControl *m_pMediaControl;
    IBaseFilter *m_pVideoCapture;
    ISampleGrabber *m_pGrabber;
    IBaseFilter *m_pGrabberF;
    
    // Таймер для захвата кадров
    QTimer *m_captureTimer;
    QTimer *m_videoFrameTimer;
    
    // Мьютекс для потокобезопасности
    QMutex m_mutex;
    
    // Состояния
    bool m_initialized;
    bool m_isPreviewActive;
    bool m_isRecordingVideo;
    
    // Буфер для текущего кадра
    QImage m_currentFrame;
    
    // Для записи видео (упрощенная версия - сохранение кадров)
    QList<QImage> m_videoFrames;
    QString m_currentVideoPath;
    int m_videoFrameCount;
    int m_videoFrameWidth;
    int m_videoFrameHeight;
};

#endif // CAMERAWORKER_H
