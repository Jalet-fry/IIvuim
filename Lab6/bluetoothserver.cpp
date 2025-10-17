#include "bluetoothserver.h"
#include "bluetoothlogger.h"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <bthdef.h>  // Для системного RFCOMM_PROTOCOL_UUID

// RFCOMM Protocol UUID для ПК-ПК передачи (используем системный RFCOMM_PROTOCOL_UUID)

BluetoothServer::BluetoothServer(BluetoothLogger *logger, QObject *parent)
    : QObject(parent)
    , logger(logger)
    , serverThread(nullptr)
    , running(false)
    , shouldStop(false)
    , serverSocket(INVALID_SOCKET)
{
}

BluetoothServer::~BluetoothServer()
{
    stopServer();
}

void BluetoothServer::startServer()
{
    if (running) {
        logger->warning("Server", "Сервер уже запущен");
        return;
    }
    
    shouldStop = false;
    running = true;
    
    // Создаем поток для сервера
    serverThread = new QThread(this);
    
    // Перемещаем сервер в поток
    moveToThread(serverThread);
    
    // Подключаем сигналы
    connect(serverThread, &QThread::started, this, &BluetoothServer::runServer);
    connect(serverThread, &QThread::finished, serverThread, &QThread::deleteLater);
    
    // Запускаем поток
    serverThread->start();
    
    emit serverStarted();
}

void BluetoothServer::stopServer()
{
    if (!running) return;
    
    shouldStop = true;
    running = false;
    
    // Закрываем сокет если он открыт
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    // Останавливаем поток
    if (serverThread && serverThread->isRunning()) {
        serverThread->quit();
        serverThread->wait(5000); // Ждем 5 секунд
    }
    
    emit serverStopped();
}

void BluetoothServer::runServer()
{
    logger->info("Server", "═══════════════════════════════════════");
    logger->info("Server", "ЗАПУСК BLUETOOTH СЕРВЕРА");
    logger->info("Server", "═══════════════════════════════════════");
    logger->info("Server", "");
    
    // Инициализация Winsock
    if (!initWinsock()) {
        logger->error("Server", "Ошибка инициализации Winsock");
        emit transferFailed("Ошибка инициализации Winsock");
        running = false;
        return;
    }
    
    // Создание серверного сокета
    logger->info("Server", "ШАГ 1: Создание серверного сокета");
    serverSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    
    if (serverSocket == INVALID_SOCKET) {
        logger->error("Server", QString("Не удалось создать сокет: %1").arg(getLastSocketError()));
        cleanupWinsock();
        emit transferFailed("Не удалось создать серверный сокет");
        running = false;
        return;
    }
    
    logger->success("Server", "✓ Серверный сокет создан");
    logger->info("Server", "");
    
    // Настройка адреса сервера
    logger->info("Server", "ШАГ 2: Настройка адреса сервера");
    SOCKADDR_BTH serverAddress;
    ZeroMemory(&serverAddress, sizeof(serverAddress));
    serverAddress.addressFamily = AF_BTH;
    serverAddress.port = 11;  // Стандартный порт для RFCOMM
    serverAddress.serviceClassId = RFCOMM_PROTOCOL_UUID;
    
    logger->debug("Server", "Параметры сервера:");
    logger->debug("Server", "  • addressFamily: AF_BTH");
    logger->debug("Server", "  • port: 11");
    logger->debug("Server", "  • serviceClassId: RFCOMM_PROTOCOL_UUID");
    logger->info("Server", "");
    
    // Привязка сокета к адресу
    logger->info("Server", "ШАГ 3: Привязка сокета к адресу");
    if (bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        logger->error("Server", QString("Ошибка привязки: %1").arg(getLastSocketError()));
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit transferFailed("Ошибка привязки сокета");
        running = false;
        return;
    }
    
    logger->success("Server", "✓ Сокет привязан к адресу");
    logger->info("Server", "");
    
    // Перевод сокета в режим прослушивания
    logger->info("Server", "ШАГ 4: Запуск прослушивания");
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        logger->error("Server", QString("Ошибка запуска прослушивания: %1").arg(getLastSocketError()));
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit transferFailed("Ошибка запуска прослушивания");
        running = false;
        return;
    }
    
    logger->success("Server", "✓ Сервер запущен и ожидает подключений");
    logger->info("Server", "Порт: 11 (RFCOMM)");
    logger->info("Server", "Готов к приему файлов...");
    logger->info("Server", "");
    
    // Основной цикл сервера
    while (!shouldStop && running) {
        logger->info("Server", "⏳ Ожидание подключения клиента...");
        
        // Ожидание подключения клиента
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        
        if (clientSocket == INVALID_SOCKET) {
            if (shouldStop) {
                logger->info("Server", "Сервер остановлен");
                break;
            }
            
            int error = WSAGetLastError();
            if (error == WSAEINTR) {
                // Прервано сигналом - нормально при остановке
                break;
            }
            
            logger->warning("Server", QString("Ошибка принятия соединения: %1").arg(error));
            continue;
        }
        
        logger->success("Server", "✓ Клиент подключился!");
        logger->info("Server", "");
        
        // Принимаем файл от клиента
        receiveFile(clientSocket);
        
        // Закрываем клиентский сокет
        closesocket(clientSocket);
        
        if (!shouldStop) {
            logger->info("Server", "Готов к следующему подключению...");
            logger->info("Server", "");
        }
    }
    
    // Очистка ресурсов
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    cleanupWinsock();
    
    logger->info("Server", "Сервер остановлен");
    running = false;
}

void BluetoothServer::receiveFile(SOCKET clientSocket)
{
    logger->info("Server", "═══════════════════════════════════════");
    logger->info("Server", "ПРИЕМ ФАЙЛА ОТ КЛИЕНТА");
    logger->info("Server", "═══════════════════════════════════════");
    logger->info("Server", "");
    
    // Создаем уникальное имя файла с временной меткой
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_hhmmss");
    QString fileName = QString("received_file_%1.bin").arg(timestamp);
    QString filePath = QDir::currentPath() + "/" + fileName;
    
    logger->info("Server", QString("Файл будет сохранен как: %1").arg(fileName));
    logger->info("Server", "");
    
    // Открываем файл для записи
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        logger->error("Server", QString("Не удалось создать файл: %1").arg(file.errorString()));
        emit transferFailed("Не удалось создать файл для записи");
        return;
    }
    
    logger->success("Server", "✓ Файл создан для записи");
    logger->info("Server", "");
    
    // Прием данных
    logger->info("Server", "ШАГ 1: Прием данных от клиента");
    
    const int bufferSize = 1024;
    char buffer[bufferSize];
    int totalReceived = 0;
    int chunkNumber = 0;
    
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
        
        if (bytesReceived == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Нет данных пока - ждем
                Sleep(10);
                continue;
            } else {
                logger->error("Server", QString("Ошибка получения данных: %1").arg(error));
                file.close();
                emit transferFailed("Ошибка получения данных");
                return;
            }
        }
        
        if (bytesReceived == 0) {
            // Соединение закрыто клиентом
            logger->info("Server", "Клиент закрыл соединение");
            break;
        }
        
        // Записываем полученные данные в файл
        qint64 written = file.write(buffer, bytesReceived);
        if (written != bytesReceived) {
            logger->error("Server", "Ошибка записи в файл");
            file.close();
            emit transferFailed("Ошибка записи в файл");
            return;
        }
        
        totalReceived += bytesReceived;
        chunkNumber++;
        
        // Логируем прогресс каждые 10 блоков
        if (chunkNumber % 10 == 0) {
            logger->debug("Server", QString("Принято: %1 байт (блоков: %2)")
                .arg(totalReceived).arg(chunkNumber));
            emit transferProgress(totalReceived, -1); // -1 означает неизвестный общий размер
        }
    }
    
    file.close();
    
    logger->success("Server", "");
    logger->success("Server", "✓✓✓ ФАЙЛ УСПЕШНО ПРИНЯТ! ✓✓✓");
    logger->success("Server", QString("Имя файла: %1").arg(fileName));
    logger->success("Server", QString("Размер: %1 байт").arg(totalReceived));
    logger->success("Server", QString("Блоков получено: %1").arg(chunkNumber));
    logger->info("Server", QString("Путь к файлу: %1").arg(filePath));
    logger->info("Server", "");
    
    emit fileReceived(fileName);
    emit transferCompleted(fileName);
}

bool BluetoothServer::initWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    if (result != 0) {
        if (logger) {
            logger->error("Server", QString("Ошибка WSAStartup: %1").arg(result));
        }
        return false;
    }
    
    if (logger) {
        logger->debug("Server", "✓ Winsock инициализирован");
    }
    
    return true;
}

void BluetoothServer::cleanupWinsock()
{
    WSACleanup();
    if (logger) {
        logger->debug("Server", "✓ Winsock очищен");
    }
}

QString BluetoothServer::getLastSocketError()
{
    int error = WSAGetLastError();
    
    switch (error) {
    case WSAETIMEDOUT:
        return QString("WSAETIMEDOUT (%1): Таймаут").arg(error);
    case WSAECONNREFUSED:
        return QString("WSAECONNREFUSED (%1): Подключение отклонено").arg(error);
    case WSAEWOULDBLOCK:
        return QString("WSAEWOULDBLOCK (%1): Операция заблокирована").arg(error);
    case WSAEINTR:
        return QString("WSAEINTR (%1): Прервано сигналом").arg(error);
    default:
        return QString("WSA Error %1").arg(error);
    }
}
