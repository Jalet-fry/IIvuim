#include "cameraworker.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QBuffer>
#include <QThread>

// Windows API includes для информации о камере
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <dshow.h>
#include <qedit.h>

// Для работы с AVI файлами
#include <vfw.h>
#include <mmreg.h>

// Линковка библиотек задана в IIvuim.pro
// MinGW не поддерживает #pragma comment

// Определяем GUID_DEVCLASS_CAMERA если не определён
#ifndef GUID_DEVCLASS_CAMERA
DEFINE_GUID(GUID_DEVCLASS_CAMERA, 0xca3e7ab9, 0xb4c3, 0x4ae6, 0x82, 0x51, 0x57, 0x9e, 0xf9, 0x33, 0x89, 0x0f);
#endif

// ISampleGrabber interface (qedit.h может не содержать все определения)
EXTERN_C const CLSID CLSID_SampleGrabber;
EXTERN_C const CLSID CLSID_NullRenderer;
EXTERN_C const IID IID_ISampleGrabber;

// Forward declaration вспомогательной функции
void FreeMediaType(AM_MEDIA_TYPE& mt);

CameraWorker::CameraWorker(QObject *parent)
    : QObject(parent),
      m_pGraph(nullptr),
      m_pCapture(nullptr),
      m_pMediaControl(nullptr),
      m_pVideoCapture(nullptr),
      m_pGrabber(nullptr),
      m_pGrabberF(nullptr),
      m_captureTimer(nullptr),
      m_videoFrameTimer(nullptr),
      m_initialized(false),
      m_isPreviewActive(false),
      m_isRecordingVideo(false),
      m_videoFrameCount(0),
      m_videoFrameWidth(640),
      m_videoFrameHeight(480)
{
    // Инициализируем COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    // Инициализируем VFW для работы с AVI
    AVIFileInit();
    
    // Создаем таймеры
    m_captureTimer = new QTimer(this);
    connect(m_captureTimer, &QTimer::timeout, this, &CameraWorker::captureFrame);
    
    m_videoFrameTimer = new QTimer(this);
    connect(m_videoFrameTimer, &QTimer::timeout, this, &CameraWorker::captureFrame);
    
    // Инициализируем DirectShow
    if (!initializeDirectShow()) {
        qDebug() << "Failed to initialize DirectShow";
        emit errorOccurred("Не удалось инициализировать камеру");
    }
}

CameraWorker::~CameraWorker()
{
    stopAll();
    releaseDirectShow();
    
    // Освобождаем VFW
    AVIFileExit();
    
    CoUninitialize();
}

bool CameraWorker::initializeDirectShow()
{
    if (m_initialized) {
        return true;
    }
    
    HRESULT hr;
    
    // Создаем Filter Graph Manager
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                         IID_IGraphBuilder, (void**)&m_pGraph);
    if (FAILED(hr)) {
        qDebug() << "Failed to create FilterGraph:" << hr;
        return false;
    }
    
    // Создаем Capture Graph Builder
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
                         IID_ICaptureGraphBuilder2, (void**)&m_pCapture);
    if (FAILED(hr)) {
        qDebug() << "Failed to create CaptureGraphBuilder2:" << hr;
        m_pGraph->Release();
        m_pGraph = nullptr;
            return false;
        }
        
    // Связываем Capture Graph с Filter Graph
    hr = m_pCapture->SetFiltergraph(m_pGraph);
    if (FAILED(hr)) {
        qDebug() << "Failed to set filter graph:" << hr;
        releaseDirectShow();
        return false;
    }
    
    // Получаем интерфейс Media Control
    hr = m_pGraph->QueryInterface(IID_IMediaControl, (void**)&m_pMediaControl);
    if (FAILED(hr)) {
        qDebug() << "Failed to get MediaControl:" << hr;
        releaseDirectShow();
        return false;
    }
    
    // Создаем Video Capture Device
    ICreateDevEnum *pDevEnum = nullptr;
    IEnumMoniker *pEnum = nullptr;
    
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
    if (FAILED(hr)) {
        qDebug() << "Failed to create DeviceEnumerator:" << hr;
        releaseDirectShow();
        return false;
    }
    
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    if (hr == S_OK) {
        IMoniker *pMoniker = nullptr;
        if (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
            hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&m_pVideoCapture);
            pMoniker->Release();
        }
        pEnum->Release();
    }
    pDevEnum->Release();
    
    if (m_pVideoCapture == nullptr) {
        qDebug() << "No video capture device found";
        emit errorOccurred("Камера не найдена");
        releaseDirectShow();
        return false;
    }
    
    // Добавляем Video Capture в граф
    hr = m_pGraph->AddFilter(m_pVideoCapture, L"Video Capture");
    if (FAILED(hr)) {
        qDebug() << "Failed to add video capture filter:" << hr;
        releaseDirectShow();
        return false;
    }
    
    // Создаем Sample Grabber для захвата кадров
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
                         IID_IBaseFilter, (void**)&m_pGrabberF);
    if (FAILED(hr)) {
        qDebug() << "Failed to create SampleGrabber filter:" << hr;
        // Продолжаем без grabber'а (не критично)
    } else {
        hr = m_pGraph->AddFilter(m_pGrabberF, L"Sample Grabber");
        if (SUCCEEDED(hr)) {
            hr = m_pGrabberF->QueryInterface(IID_ISampleGrabber, (void**)&m_pGrabber);
            if (SUCCEEDED(hr)) {
                // Настраиваем Sample Grabber
                AM_MEDIA_TYPE mt;
                ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
                mt.majortype = MEDIATYPE_Video;
                mt.subtype = MEDIASUBTYPE_RGB24;
                m_pGrabber->SetMediaType(&mt);
                m_pGrabber->SetBufferSamples(TRUE);
                m_pGrabber->SetOneShot(FALSE);
            }
        }
    }
    
    // Создаем NULL Renderer (чтобы граф работал без отображения)
    IBaseFilter *pNullRenderer = nullptr;
    hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
                         IID_IBaseFilter, (void**)&pNullRenderer);
    if (SUCCEEDED(hr)) {
        m_pGraph->AddFilter(pNullRenderer, L"Null Renderer");
        
        // Соединяем фильтры
        if (m_pGrabberF) {
            m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
                                    m_pVideoCapture, m_pGrabberF, pNullRenderer);
        } else {
            m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
                                    m_pVideoCapture, nullptr, pNullRenderer);
        }
        
        pNullRenderer->Release();
    }
    
    m_initialized = true;
    qDebug() << "DirectShow initialized successfully";
    
    return true;
}

void CameraWorker::releaseDirectShow()
{
    if (m_pMediaControl) {
        m_pMediaControl->Stop();
        m_pMediaControl->Release();
        m_pMediaControl = nullptr;
    }
    
    if (m_pGrabber) {
        m_pGrabber->Release();
        m_pGrabber = nullptr;
    }
    
    if (m_pGrabberF) {
        m_pGraph->RemoveFilter(m_pGrabberF);
        m_pGrabberF->Release();
        m_pGrabberF = nullptr;
    }
    
    if (m_pVideoCapture) {
        m_pGraph->RemoveFilter(m_pVideoCapture);
        m_pVideoCapture->Release();
        m_pVideoCapture = nullptr;
    }
    
    if (m_pCapture) {
        m_pCapture->Release();
        m_pCapture = nullptr;
    }
    
    if (m_pGraph) {
        m_pGraph->Release();
        m_pGraph = nullptr;
    }
    
    m_initialized = false;
}

void CameraWorker::startPreview()
{
    if (!m_initialized) {
        if (!initializeDirectShow()) {
        return;
        }
    }
    
    if (m_pMediaControl) {
        HRESULT hr = m_pMediaControl->Run();
        if (SUCCEEDED(hr)) {
            m_isPreviewActive = true;
            m_captureTimer->start(33); // ~30 FPS
            qDebug() << "Preview started";
        } else {
            qDebug() << "Failed to start preview:" << hr;
            emit errorOccurred("Не удалось запустить превью");
        }
    }
}

void CameraWorker::stopPreview()
{
    m_captureTimer->stop();
    
    if (!m_isRecordingVideo && m_pMediaControl) {
        m_pMediaControl->Stop();
    }
    
    m_isPreviewActive = false;
    qDebug() << "Preview stopped";
}

void CameraWorker::takePhoto()
{
    if (!m_initialized) {
        emit errorOccurred("Камера не инициализирована");
        return;
    }
    
    bool wasPreviewActive = m_isPreviewActive;
    
    // Запускаем граф и таймер если не запущены
    if (!m_isPreviewActive && m_pMediaControl) {
        m_pMediaControl->Run();
        m_captureTimer->start(33); // Запускаем таймер для захвата кадров
        QThread::msleep(500); // Даём камере прогреться
    }
    
    // Захватываем текущий кадр
    QImage frame = captureCurrentFrame();
    
    if (frame.isNull()) {
        // Останавливаем если запускали
        if (!wasPreviewActive) {
            m_captureTimer->stop();
            if (m_pMediaControl) {
                m_pMediaControl->Stop();
            }
        }
        emit errorOccurred("Не удалось захватить кадр");
        return;
    }
    
    // Генерируем путь для сохранения
    QString outputDir = getOutputDirectory();
    QString fileName = generateFileName("photo", "jpg");
    QString fullPath = outputDir + "/" + fileName;
    
    // Сохраняем кадр
    if (saveFrame(frame, fullPath)) {
        emit photoSaved(fullPath);
        qDebug() << "Photo saved:" << fullPath;
    } else {
        emit errorOccurred("Не удалось сохранить фото");
    }
    
    // Останавливаем граф и таймер если превью не было активно
    if (!wasPreviewActive) {
        m_captureTimer->stop();
        if (m_pMediaControl) {
            m_pMediaControl->Stop();
        }
    }
}

void CameraWorker::startVideoRecording()
{
    if (!m_initialized) {
        emit errorOccurred("Камера не инициализирована");
        return;
    }
    
    // Очищаем буфер кадров
    m_videoFrames.clear();
    m_videoFrameCount = 0;
    
    // Генерируем путь для видео
    QString outputDir = getOutputDirectory();
    QString fileName = generateFileName("video", "avi");
    m_currentVideoPath = outputDir + "/" + fileName;
    
    qDebug() << "Will save video to:" << m_currentVideoPath;
    
    // Запускаем граф если не запущен
    if (m_pMediaControl) {
        HRESULT hr = m_pMediaControl->Run();
        if (FAILED(hr)) {
            qDebug() << "Failed to start media control, hr:" << hr;
        } else {
            qDebug() << "Media control started successfully";
        }
    }
    
    // Запускаем таймер захвата кадров для видео
    m_isRecordingVideo = true;
    m_videoFrameTimer->start(33); // ~30 FPS
    qDebug() << "Video frame timer started";
    
    emit videoRecordingStarted();
    qDebug() << "Video recording started";
}

void CameraWorker::stopVideoRecording()
{
    if (!m_isRecordingVideo) {
        qDebug() << "stopVideoRecording called but not recording";
        return;
    }
    
    qDebug() << "Stopping video recording...";
    m_videoFrameTimer->stop();
    m_isRecordingVideo = false;
    
    qDebug() << "Captured" << m_videoFrames.count() << "frames";
    
    // Сохраняем видео в настоящий AVI файл
    if (!m_videoFrames.isEmpty()) {
        qDebug() << "Saving video to AVI...";
        bool success = saveVideoToAVI(m_currentVideoPath, m_videoFrames, m_videoFrameWidth, m_videoFrameHeight);
        if (success) {
            qDebug() << "✅ Video saved as AVI:" << m_currentVideoPath << "-" << m_videoFrames.count() << "frames";
            emit videoRecordingStopped(); // Отправляем сигнал только если успешно
        } else {
            qDebug() << "❌ Failed to save video as AVI, falling back to frames";
            // Fallback: сохраняем первый кадр
            QString firstFramePath = m_currentVideoPath;
            firstFramePath.replace(".avi", "_frame_000.jpg");
            if (saveFrame(m_videoFrames.first(), firstFramePath)) {
                qDebug() << "Saved first frame as fallback:" << firstFramePath;
            }
            emit videoRecordingStopped();
        }
    } else {
        qDebug() << "❌ No frames captured!";
        emit errorOccurred("Не удалось записать видео - нет кадров");
    }
    
    m_videoFrames.clear();
    
    qDebug() << "Video recording stopped";
    
    // Останавливаем граф если превью не активно
    if (!m_isPreviewActive && m_pMediaControl) {
        m_pMediaControl->Stop();
        qDebug() << "Media control stopped";
    }
}

void CameraWorker::getCameraInfo()
{
    QString info = getCameraInfoWindows();
    
    if (info.isEmpty()) {
        info = "<p><b>Информация через Windows API недоступна</b></p>";
    }
    
    // Добавляем информацию от DirectShow
    info += "<hr><p><b>Информация от DirectShow:</b></p>";
    
    if (m_pVideoCapture) {
        info += "<p><b>Статус:</b> Камера подключена</p>";
        info += "<p><b>API:</b> DirectShow (Windows нативный)</p>";
        info += QString("<p><b>Разрешение:</b> %1x%2</p>").arg(m_videoFrameWidth).arg(m_videoFrameHeight);
    } else {
        info += "<p>Камера не найдена</p>";
    }
    
    emit cameraInfoReady(info);
}

void CameraWorker::stopAll()
{
    if (m_isRecordingVideo) {
        stopVideoRecording();
    }
    
    stopPreview();
}

void CameraWorker::captureFrame()
{
    if (!m_pGrabber) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    long bufferSize = 0;
    HRESULT hr = m_pGrabber->GetCurrentBuffer(&bufferSize, NULL);
    if (FAILED(hr) || bufferSize == 0) {
        return;
    }
    
    char *pBuffer = new char[bufferSize];
    hr = m_pGrabber->GetCurrentBuffer(&bufferSize, (long*)pBuffer);
    
    if (SUCCEEDED(hr)) {
        // Получаем формат кадра
        AM_MEDIA_TYPE mt;
        hr = m_pGrabber->GetConnectedMediaType(&mt);
        if (SUCCEEDED(hr) && mt.formattype == FORMAT_VideoInfo) {
            VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)mt.pbFormat;
            
            int width = pVih->bmiHeader.biWidth;
            int height = abs(pVih->bmiHeader.biHeight);
            
            m_videoFrameWidth = width;
            m_videoFrameHeight = height;
            
            // Создаем QImage из буфера
            QImage frame(width, height, QImage::Format_RGB888);
            
            // Копируем данные (BGR -> RGB и переворачиваем)
            for (int y = 0; y < height; y++) {
                uchar *dest = frame.scanLine(height - 1 - y);
                const uchar *src = (const uchar*)(pBuffer + y * width * 3);
                
                for (int x = 0; x < width; x++) {
                    dest[x * 3 + 0] = src[x * 3 + 2]; // R
                    dest[x * 3 + 1] = src[x * 3 + 1]; // G
                    dest[x * 3 + 2] = src[x * 3 + 0]; // B
                }
            }
            
            m_currentFrame = frame;
            
            // Отправляем кадр для превью
            if (m_isPreviewActive) {
                emit frameReady(frame);
            }
            
            // Сохраняем кадр для видео
            if (m_isRecordingVideo) {
                m_videoFrames.append(frame);
                m_videoFrameCount++;
                
                if (m_videoFrameCount % 30 == 0) {
                    qDebug() << "Video: captured" << m_videoFrameCount << "frames";
                }
                
                // Ограничиваем количество кадров (чтобы не забить память)
                if (m_videoFrames.count() > 300) { // ~10 секунд при 30 FPS
                    m_videoFrames.removeFirst();
                }
            }
            
            FreeMediaType(mt);
        }
    }
    
    delete[] pBuffer;
}

QImage CameraWorker::captureCurrentFrame()
{
    // Ждем захвата кадра
    for (int i = 0; i < 10; i++) {
        QThread::msleep(50);
        QMutexLocker locker(&m_mutex);
        if (!m_currentFrame.isNull()) {
            return m_currentFrame;
        }
    }
    
    return QImage();
}

bool CameraWorker::saveFrame(const QImage &frame, const QString &filePath)
{
    if (frame.isNull()) {
        return false;
    }
    
    return frame.save(filePath, "JPG", 90);
}

QString CameraWorker::generateFileName(const QString &prefix, const QString &extension)
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_HHmmss_zzz");
    return QString("%1_%2.%3").arg(prefix).arg(timestamp).arg(extension);
}

QString CameraWorker::getOutputDirectory()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString outputPath = documentsPath + "/Lab4_CameraCaptures";
    
    QDir dir;
    if (!dir.exists(outputPath)) {
        if (dir.mkpath(outputPath)) {
            qDebug() << "Created output directory:" << outputPath;
        } else {
            qWarning() << "Failed to create directory:" << outputPath;
            outputPath = QDir::currentPath();
        }
    }
    
    return outputPath;
}

QString CameraWorker::getCameraInfoWindows()
{
    QString result;
    
    try {
        SP_DEVINFO_DATA deviceInfoData;
        ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        
        HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
            &GUID_DEVCLASS_CAMERA,
            L"USB",
            NULL,
            DIGCF_PRESENT
        );
        
        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            deviceInfoSet = SetupDiGetClassDevs(
                &GUID_DEVCLASS_CAMERA,
                NULL,
                NULL,
                DIGCF_PRESENT
            );
            
            if (deviceInfoSet == INVALID_HANDLE_VALUE) {
                return QString();
            }
        }
        
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        
        int cameraCount = 0;
        for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
            cameraCount++;
            
            const int bufferSize = 1024;
            wchar_t deviceID[bufferSize];
            wchar_t deviceName[bufferSize];
            wchar_t companyName[bufferSize];
            
            ZeroMemory(deviceID, sizeof(deviceID));
            ZeroMemory(deviceName, sizeof(deviceName));
            ZeroMemory(companyName, sizeof(companyName));
            
            SetupDiGetDeviceInstanceId(deviceInfoSet, &deviceInfoData, deviceID, sizeof(deviceID), NULL);
            
            if (!SetupDiGetDeviceRegistryProperty(
                deviceInfoSet,
                &deviceInfoData,
                SPDRP_DEVICEDESC,
                NULL,
                (PBYTE)deviceName,
                sizeof(deviceName),
                NULL)) {
                wcscpy_s(deviceName, L"Неизвестно");
            }
            
            if (!SetupDiGetDeviceRegistryProperty(
                deviceInfoSet,
                &deviceInfoData,
                SPDRP_MFG,
                NULL,
                (PBYTE)companyName,
                sizeof(companyName),
                NULL)) {
                wcscpy_s(companyName, L"Неизвестно");
            }
            
            if (cameraCount == 1) {
                result += "<p><b>Информация через Windows Setup API:</b></p>";
            }
            
            result += QString("<hr><p><b>Устройство #%1</b></p>").arg(cameraCount);
            result += QString("<p><b>Название:</b> %1</p>").arg(QString::fromWCharArray(deviceName));
            result += QString("<p><b>Производитель:</b> %1</p>").arg(QString::fromWCharArray(companyName));
            
            QString deviceIdStr = QString::fromWCharArray(deviceID);
            if (deviceIdStr.contains("VID_")) {
                int vidPos = deviceIdStr.indexOf("VID_") + 4;
                QString vid = deviceIdStr.mid(vidPos, 4);
                result += QString("<p><b>Vendor ID:</b> %1</p>").arg(vid);
            }
            
            if (deviceIdStr.contains("PID_")) {
                int pidPos = deviceIdStr.indexOf("PID_") + 4;
                QString pid = deviceIdStr.mid(pidPos, 4);
                result += QString("<p><b>Product ID:</b> %1</p>").arg(pid);
            }
        }
        
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        
        if (cameraCount == 0) {
            result = "<p><b>Устройства не найдены через Windows API</b></p>";
        }
    }
    catch (...) {
        return QString();
    }
    
    return result;
}

// Вспомогательная функция для освобождения AM_MEDIA_TYPE
void FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

// Сохранение видео в AVI файл
bool CameraWorker::saveVideoToAVI(const QString &filePath, const QList<QImage> &frames, int width, int height)
{
    if (frames.isEmpty()) {
        return false;
    }
    
    // Конвертируем QString в wchar_t для Windows API
    std::wstring wFilePath = filePath.toStdWString();
    
    // Создаем AVI файл
    PAVIFILE pAviFile = nullptr;
    HRESULT hr = AVIFileOpenW(&pAviFile, wFilePath.c_str(), OF_CREATE | OF_WRITE, nullptr);
    if (FAILED(hr)) {
        qDebug() << "Failed to create AVI file:" << filePath;
        return false;
    }
    
    // Настройка видео потока
    AVISTREAMINFO aviStreamInfo;
    memset(&aviStreamInfo, 0, sizeof(AVISTREAMINFO));
    aviStreamInfo.fccType = streamtypeVIDEO;
    aviStreamInfo.fccHandler = mmioFOURCC('M', 'J', 'P', 'G'); // MJPEG кодек
    aviStreamInfo.dwScale = 1;
    aviStreamInfo.dwRate = 30; // 30 FPS
    aviStreamInfo.dwSuggestedBufferSize = width * height * 3;
    aviStreamInfo.rcFrame.left = 0;
    aviStreamInfo.rcFrame.top = 0;
    aviStreamInfo.rcFrame.right = width;
    aviStreamInfo.rcFrame.bottom = height;
    
    PAVISTREAM pVideoStream = nullptr;
    hr = AVIFileCreateStreamW(pAviFile, &pVideoStream, &aviStreamInfo);
    if (FAILED(hr)) {
        qDebug() << "Failed to create video stream";
        AVIFileRelease(pAviFile);
        return false;
    }
    
    // Настройка формата видео (MJPEG)
    BITMAPINFOHEADER bih;
    memset(&bih, 0, sizeof(BITMAPINFOHEADER));
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = width * height * 3;
    
    hr = AVIStreamSetFormat(pVideoStream, 0, &bih, sizeof(bih));
    if (FAILED(hr)) {
        qDebug() << "Failed to set video format";
        AVIStreamRelease(pVideoStream);
        AVIFileRelease(pAviFile);
        return false;
    }
    
    // Записываем кадры
    for (int i = 0; i < frames.count(); ++i) {
        const QImage &frame = frames[i];
        
        // Для видео нужно дополнительно перевернуть (превью и видео имеют разную ориентацию)
        QImage flippedFrame = frame.mirrored(false, true);
        
        // Конвертируем в BGR24 формат (правильный порядок цветов для AVI)
        QImage bgrFrame = flippedFrame.convertToFormat(QImage::Format_RGB888);
        
        // Конвертируем RGB -> BGR для правильного отображения цветов
        for (int y = 0; y < bgrFrame.height(); ++y) {
            for (int x = 0; x < bgrFrame.width(); ++x) {
                QRgb pixel = bgrFrame.pixel(x, y);
                int r = qRed(pixel);
                int g = qGreen(pixel);
                int b = qBlue(pixel);
                bgrFrame.setPixel(x, y, qRgb(b, g, r)); // BGR порядок
            }
        }
        
        // Записываем кадр
        LONG written;
        hr = AVIStreamWrite(pVideoStream, i, 1, bgrFrame.bits(), 
                          bgrFrame.byteCount(), AVIIF_KEYFRAME, nullptr, &written);
        
        if (FAILED(hr)) {
            qDebug() << "Failed to write frame" << i;
            break;
        }
    }
    
    // Освобождаем ресурсы
    AVIStreamRelease(pVideoStream);
    AVIFileRelease(pAviFile);
    
    qDebug() << "AVI file saved successfully:" << filePath << "with" << frames.count() << "frames";
    return true;
}

