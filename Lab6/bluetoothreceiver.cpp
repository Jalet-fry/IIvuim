#include "bluetoothreceiver.h"
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QDataStream>

BluetoothReceiver::BluetoothReceiver(BluetoothLogger *logger, QObject *parent)
    : QObject(parent)
    , logger(logger)
    , currentConnection(nullptr)
    , mediaPlayer(nullptr)
    , audioOutput(nullptr)
    , autoPlayEnabled(true)
    , receiving(false)
    , expectedFileSize(0)
    , bytesReceived(0)
    , receivedFile(nullptr)
{
    // Создаем медиаплеер для автовоспроизведения
    mediaPlayer = new QMediaPlayer(this);
    
    // В Qt 5.5 используем setVolume напрямую
    mediaPlayer->setVolume(80);
    
    if (logger) {
        logger->info("Receiver", "BluetoothReceiver инициализирован");
        logger->info("Receiver", "✓ Автовоспроизведение ВКЛЮЧЕНО");
    }
}

BluetoothReceiver::~BluetoothReceiver()
{
    stopListening();
    if (receivedFile) {
        receivedFile->close();
        delete receivedFile;
    }
}

void BluetoothReceiver::startListening(BluetoothConnection *connection)
{
    if (!connection) {
        if (logger) {
            logger->error("Receiver", "Нет подключения для прослушивания!");
        }
        return;
    }
    
    currentConnection = connection;
    
    if (logger) {
        logger->info("Receiver", "═══════════════════════════════════════");
        logger->info("Receiver", "ЗАПУСК ПРОСЛУШИВАНИЯ ВХОДЯЩИХ ДАННЫХ");
        logger->info("Receiver", "═══════════════════════════════════════");
        logger->success("Receiver", "✓ Готов к приему файлов");
        logger->info("Receiver", "");
    }
    
    // Подключаемся к сигналу данных
    connect(currentConnection, &BluetoothConnection::dataReceived,
            this, &BluetoothReceiver::onDataReceived);
            
    // Периодически проверяем входящие данные
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BluetoothReceiver::checkForIncomingData);
    timer->start(100);  // Проверка каждые 100ms
}

void BluetoothReceiver::stopListening()
{
    if (currentConnection) {
        disconnect(currentConnection, &BluetoothConnection::dataReceived,
                   this, &BluetoothReceiver::onDataReceived);
        currentConnection = nullptr;
    }
    
    if (logger) {
        logger->info("Receiver", "Прослушивание остановлено");
    }
}

void BluetoothReceiver::checkForIncomingData()
{
    if (!currentConnection) return;
    
    QByteArray data = currentConnection->receiveData(4096);
    if (!data.isEmpty()) {
        onDataReceived(data);
    }
}

void BluetoothReceiver::onDataReceived(const QByteArray &data)
{
    if (!receiving) {
        // Начало приема - читаем заголовок
        logger->info("Receiver", "═══════════════════════════════════════");
        logger->info("Receiver", "ПРИЕМ ФАЙЛА НАЧАТ!");
        logger->info("Receiver", "═══════════════════════════════════════");
        
        QDataStream stream(data);
        stream.setVersion(QDataStream::Qt_5_5);
        
        quint32 headerSize;
        stream >> headerSize;
        
        logger->debug("Receiver", QString("Размер заголовка: %1 байт").arg(headerSize));
        
        stream >> receivedFileName;
        stream >> expectedFileSize;
        
        logger->info("Receiver", QString("Имя файла: %1").arg(receivedFileName));
        logger->info("Receiver", QString("Ожидаемый размер: %1 байт (%.2f MB)")
            .arg(expectedFileSize)
            .arg(expectedFileSize / 1024.0 / 1024.0));
        logger->info("Receiver", "");
        
        // Создаем файл для сохранения
        QString receivePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        receivePath += "/Lab6_ReceivedFiles";
        
        QDir dir(receivePath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        
        QString fullPath = receivePath + "/" + receivedFileName;
        receivedFile = new QFile(fullPath);
        
        if (!receivedFile->open(QIODevice::WriteOnly)) {
            logger->error("Receiver", QString("Не удалось создать файл: %1").arg(receivedFile->errorString()));
            emit receiveFailed("Не удалось создать файл");
            return;
        }
        
        logger->success("Receiver", QString("✓ Файл создан: %1").arg(fullPath));
        logger->info("Receiver", "Начало приема данных...");
        logger->info("Receiver", "");
        
        receiving = true;
        bytesReceived = 0;
        
        // Остальные данные в этом пакете - это начало файла
        int headerBytes = sizeof(quint32) + receivedFileName.toUtf8().size() + sizeof(qint64);
        if (data.size() > headerBytes) {
            QByteArray fileData = data.mid(headerBytes);
            receivedFile->write(fileData);
            bytesReceived += fileData.size();
        }
        
    } else {
        // Продолжение приема
        receivedFile->write(data);
        bytesReceived += data.size();
        
        int progress = (bytesReceived * 100) / expectedFileSize;
        
        if (bytesReceived % (40960) == 0) {  // Каждые 40 KB
            logger->debug("Receiver", QString("Получено: %1/%2 байт (%3%)")
                .arg(bytesReceived)
                .arg(expectedFileSize)
                .arg(progress));
        }
        
        emit receiveProgress(bytesReceived, expectedFileSize);
        
        // Проверяем завершение
        if (bytesReceived >= expectedFileSize) {
            receivedFile->close();
            
            QString filePath = receivedFile->fileName();
            
            logger->success("Receiver", "");
            logger->success("Receiver", "✓✓✓ ФАЙЛ ПОЛУЧЕН ПОЛНОСТЬЮ! ✓✓✓");
            logger->success("Receiver", QString("Сохранен: %1").arg(filePath));
            logger->info("Receiver", "");
            
            delete receivedFile;
            receivedFile = nullptr;
            receiving = false;
            
            emit fileReceived(filePath);
            
            // Автовоспроизведение
            if (autoPlayEnabled && isAudioFile(filePath)) {
                autoPlayFile(filePath);
            }
        }
    }
}

void BluetoothReceiver::autoPlayFile(const QString &filePath)
{
    logger->info("Receiver", "═══════════════════════════════════════");
    logger->info("Receiver", "АВТОВОСПРОИЗВЕДЕНИЕ");
    logger->info("Receiver", "═══════════════════════════════════════");
    logger->info("Receiver", QString("Файл: %1").arg(filePath));
    logger->info("Receiver", "");
    
    mediaPlayer->setMedia(QUrl::fromLocalFile(filePath));
    mediaPlayer->play();
    
    logger->success("Receiver", "✓ Воспроизведение началось!");
    logger->info("Receiver", "");
    
    emit playbackStarted(QFileInfo(filePath).fileName());
}

bool BluetoothReceiver::isAudioFile(const QString &filePath)
{
    QStringList audioExtensions;
    audioExtensions << "mp3" << "wav" << "ogg" << "flac" << "m4a" << "wma" << "aac";
    
    QString ext = QFileInfo(filePath).suffix().toLower();
    return audioExtensions.contains(ext);
}

