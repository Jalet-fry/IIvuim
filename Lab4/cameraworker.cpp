#include "cameraworker.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QCameraInfo>
#include <QImageWriter>
#include <QBuffer>

// Windows API includes для информации о камере
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>

// Определяем GUID_DEVCLASS_CAMERA если не определён
#ifndef GUID_DEVCLASS_CAMERA
DEFINE_GUID(GUID_DEVCLASS_CAMERA, 0xca3e7ab9, 0xb4c3, 0x4ae6, 0x82, 0x51, 0x57, 0x9e, 0xf9, 0x33, 0x89, 0x0f);
#endif

CameraWorker::CameraWorker(QObject *parent)
    : QObject(parent),
      camera(nullptr),
      imageCapture(nullptr),
      captureTimer(nullptr),
      videoFrameTimer(nullptr),
      isPreviewActive(false),
      isRecordingVideo(false),
      isCameraInitialized(false),
      videoFrameCount(0)
{
    // Таймеры создадим при необходимости
}

CameraWorker::~CameraWorker()
{
    stopAll();
    releaseCamera();
}

bool CameraWorker::initializeCamera()
{
    if (isCameraInitialized) {
        return true;
    }
    
    try {
        // Получаем список доступных камер
        QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
        
        if (cameras.isEmpty()) {
            emit errorOccurred("Не найдено ни одной камеры!");
            return false;
        }
        
        // Используем первую доступную камеру
        QCameraInfo cameraInfo = cameras.first();
        qDebug() << "Инициализация камеры:" << cameraInfo.description();
        
        camera = new QCamera(cameraInfo);
        
        // Обработка ошибок камеры
        connect(camera, static_cast<void(QCamera::*)(QCamera::Error)>(&QCamera::error),
                this, &CameraWorker::onCameraError);
        
        // Создаем объект для захвата изображений
        imageCapture = new QCameraImageCapture(camera);
        imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);
        
        // Подключаем сигналы
        connect(imageCapture, &QCameraImageCapture::imageCaptured,
                this, &CameraWorker::onImageCaptured);
        connect(imageCapture, &QCameraImageCapture::imageSaved,
                this, &CameraWorker::onImageSaved);
        connect(imageCapture, static_cast<void(QCameraImageCapture::*)(int, QCameraImageCapture::Error, const QString&)>(&QCameraImageCapture::error),
                this, &CameraWorker::onCaptureError);
        
        isCameraInitialized = true;
        qDebug() << "Камера успешно инициализирована";
        return true;
    }
    catch (...) {
        emit errorOccurred("Ошибка инициализации камеры");
        return false;
    }
}

void CameraWorker::releaseCamera()
{
    if (camera) {
        if (camera->state() == QCamera::ActiveState) {
            camera->stop();
        }
        camera->deleteLater();
        camera = nullptr;
    }
    
    if (imageCapture) {
        imageCapture->deleteLater();
        imageCapture = nullptr;
    }
    
    if (captureTimer) {
        captureTimer->stop();
        captureTimer->deleteLater();
        captureTimer = nullptr;
    }
    
    if (videoFrameTimer) {
        videoFrameTimer->stop();
        videoFrameTimer->deleteLater();
        videoFrameTimer = nullptr;
    }
    
    isCameraInitialized = false;
}

void CameraWorker::startPreview()
{
    if (!initializeCamera()) {
        return;
    }
    
    isPreviewActive = true;
    
    if (camera->state() != QCamera::ActiveState) {
        camera->start();
    }
    
    qDebug() << "Превью запущено";
}

void CameraWorker::stopPreview()
{
    isPreviewActive = false;
    
    if (!isRecordingVideo && camera && camera->state() == QCamera::ActiveState) {
        camera->stop();
    }
    
    qDebug() << "Превью остановлено";
}

void CameraWorker::takePhoto()
{
    if (!initializeCamera()) {
        return;
    }
    
    // Запускаем камеру если не запущена
    if (camera->state() != QCamera::ActiveState) {
        camera->start();
    }
    
    // Генерируем путь для сохранения
    QString outputDir = getOutputDirectory();
    QString fileName = generateFileName("photo", "jpg");
    QString fullPath = outputDir + "/" + fileName;
    
    // Устанавливаем путь сохранения
    imageCapture->capture(fullPath);
    
    qDebug() << "Захват фото:" << fullPath;
}

void CameraWorker::startVideoRecording()
{
    // Qt 5.5 с MinGW не поддерживает полноценную запись видео без установленных кодеков
    // Для записи видео нужен QMediaRecorder + системные DirectShow кодеки
    emit errorOccurred(
        "Запись видео в Qt 5.5 требует:\n"
        "1. Установленные системные кодеки (K-Lite Codec Pack)\n"
        "2. QMediaRecorder (требует дополнительной реализации)\n\n"
        "Текущая версия поддерживает только захват фото.\n"
        "Для скрытого режима используйте фото с таймером."
    );
    
    qDebug() << "Запись видео не поддерживается в текущей конфигурации";
}

void CameraWorker::stopVideoRecording()
{
    // Заглушка
    isRecordingVideo = false;
    emit videoRecordingStopped();
}

void CameraWorker::getCameraInfo()
{
    QString info = getCameraInfoWindows();
    
    if (info.isEmpty()) {
        info = "<p><b>Информация через Windows API недоступна</b></p>";
    }
    
    // Добавляем информацию от Qt
    info += "<hr><p><b>Информация от Qt Multimedia:</b></p>";
    
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    
    if (cameras.isEmpty()) {
        info += "<p>Камеры не найдены</p>";
    } else {
        for (int i = 0; i < cameras.size(); ++i) {
            const QCameraInfo &cameraInfo = cameras[i];
            info += QString("<p><b>Камера #%1:</b></p>").arg(i + 1);
            info += QString("<p><b>Название:</b> %1</p>").arg(cameraInfo.description());
            info += QString("<p><b>Устройство:</b> %1</p>").arg(cameraInfo.deviceName());
            info += QString("<p><b>Позиция:</b> %1</p>").arg(
                cameraInfo.position() == QCamera::FrontFace ? "Фронтальная" :
                cameraInfo.position() == QCamera::BackFace ? "Задняя" : "Неизвестно"
            );
            info += QString("<p><b>Ориентация:</b> %1°</p>").arg(cameraInfo.orientation());
            
            if (i == 0) {
                info += "<p><i>(используется по умолчанию)</i></p>";
            }
            
            if (i < cameras.size() - 1) {
                info += "<hr>";
            }
        }
    }
    
    emit cameraInfoReady(info);
}

void CameraWorker::stopAll()
{
    if (isRecordingVideo) {
        stopVideoRecording();
    }
    
    stopPreview();
}

void CameraWorker::onImageCaptured(int id, const QImage &preview)
{
    Q_UNUSED(id);
    
    // Отправляем превью фото
    if (!preview.isNull()) {
        emit frameReady(preview);
    }
}

void CameraWorker::onImageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    emit photoSaved(fileName);
    qDebug() << "Фото сохранено:" << fileName;
}

void CameraWorker::onCaptureError(int id, QCameraImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    emit errorOccurred("Ошибка захвата изображения: " + errorString);
}

void CameraWorker::onCameraError(QCamera::Error error)
{
    QString errorMsg;
    switch (error) {
        case QCamera::NoError:
            return;
        case QCamera::CameraError:
            errorMsg = "Общая ошибка камеры";
            break;
        case QCamera::InvalidRequestError:
            errorMsg = "Неверный запрос к камере";
            break;
        case QCamera::ServiceMissingError:
            errorMsg = "Сервис камеры недоступен";
            break;
        case QCamera::NotSupportedFeatureError:
            errorMsg = "Функция не поддерживается";
            break;
        default:
            errorMsg = "Неизвестная ошибка камеры";
    }
    
    emit errorOccurred(errorMsg);
}

void CameraWorker::captureVideoFrame()
{
    // Эта функция была бы использована для захвата кадров видео
    // В Qt 5.5 без дополнительных модулей это сложно реализовать
    // Оставляем заглушку для возможного расширения
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
            qDebug() << "Создана папка для сохранения:" << outputPath;
        } else {
            qWarning() << "Не удалось создать папку:" << outputPath;
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
                result += "<p><b>Информация через Windows API:</b></p>";
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
