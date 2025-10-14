#include "bluetoothconnection.h"
#include "bluetoothlogger.h"
#include <QDebug>

#pragma comment(lib, "ws2_32.lib")

BluetoothConnection::BluetoothConnection(BluetoothLogger *logger, QObject *parent)
    : QObject(parent)
    , logger(logger)
    , btSocket(INVALID_SOCKET)
    , connected(false)
{
    if (logger) {
        logger->info("Connection", "BluetoothConnection инициализирован");
    }
}

BluetoothConnection::~BluetoothConnection()
{
    disconnect();
    cleanupWinsock();
}

bool BluetoothConnection::initializeWinsock()
{
    if (logger) {
        logger->debug("Connection", "═══════════════════════════════════════");
        logger->debug("Connection", "ИНИЦИАЛИЗАЦИЯ WINSOCK");
        logger->debug("Connection", "═══════════════════════════════════════");
    }
    
    WSADATA wsaData;
    
    if (logger) {
        logger->logApiCall("WSAStartup", "MAKEWORD(2,2)");
    }
    
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    if (result != 0) {
        if (logger) {
            logger->logApiResult("WSAStartup", QString("FAILED - Error: %1").arg(result), false);
            logger->error("Connection", QString("WSAStartup failed: %1").arg(result));
        }
        return false;
    }
    
    if (logger) {
        logger->logApiResult("WSAStartup", "SUCCESS", true);
        logger->success("Connection", QString("✓ Winsock версия: %1.%2")
            .arg(LOBYTE(wsaData.wVersion))
            .arg(HIBYTE(wsaData.wVersion)));
        logger->debug("Connection", "");
    }
    
    return true;
}

void BluetoothConnection::cleanupWinsock()
{
    if (logger) {
        logger->debug("Connection", "Очистка Winsock...");
        logger->logApiCall("WSACleanup", "");
    }
    
    WSACleanup();
    
    if (logger) {
        logger->debug("Connection", "✓ Winsock очищен");
    }
}

bool BluetoothConnection::parseMacAddress(const QString &address, BLUETOOTH_ADDRESS &btAddr)
{
    if (logger) {
        logger->debug("Connection", QString("Парсинг MAC адреса: %1").arg(address));
    }
    
    // Формат: XX:XX:XX:XX:XX:XX
    QStringList bytes = address.split(":");
    
    if (bytes.size() != 6) {
        if (logger) {
            logger->error("Connection", QString("Неверный формат MAC адреса! Ожидается 6 байт, получено: %1").arg(bytes.size()));
        }
        return false;
    }
    
    ZeroMemory(&btAddr, sizeof(btAddr));
    
    for (int i = 0; i < 6; ++i) {
        bool ok;
        quint8 byte = bytes[i].toUInt(&ok, 16);
        if (!ok) {
            if (logger) {
                logger->error("Connection", QString("Неверный байт в позиции %1: %2").arg(i).arg(bytes[i]));
            }
            return false;
        }
        // Bluetooth адрес в обратном порядке
        btAddr.rgBytes[5 - i] = byte;
    }
    
    if (logger) {
        logger->success("Connection", "✓ MAC адрес распознан");
        logger->debug("Connection", QString("Байты: %1 %2 %3 %4 %5 %6")
            .arg(btAddr.rgBytes[5], 2, 16, QChar('0'))
            .arg(btAddr.rgBytes[4], 2, 16, QChar('0'))
            .arg(btAddr.rgBytes[3], 2, 16, QChar('0'))
            .arg(btAddr.rgBytes[2], 2, 16, QChar('0'))
            .arg(btAddr.rgBytes[1], 2, 16, QChar('0'))
            .arg(btAddr.rgBytes[0], 2, 16, QChar('0')).toUpper());
    }
    
    return true;
}

bool BluetoothConnection::connectToDevice(const QString &deviceAddress, const QString &deviceName)
{
    if (logger) {
        logger->info("Connection", "═══════════════════════════════════════");
        logger->info("Connection", "ПОДКЛЮЧЕНИЕ К BLUETOOTH УСТРОЙСТВУ");
        logger->info("Connection", "═══════════════════════════════════════");
        logger->info("Connection", QString("Устройство: %1").arg(deviceName));
        logger->info("Connection", QString("MAC: %1").arg(deviceAddress));
        logger->info("Connection", "");
    }
    
    // Шаг 1: Инициализация Winsock
    if (!initializeWinsock()) {
        emit connectionFailed("Не удалось инициализировать Winsock");
        return false;
    }
    
    // Шаг 2: Создание Bluetooth сокета
    if (logger) {
        logger->info("Connection", "ШАГ 1: Создание Bluetooth сокета");
        logger->logApiCall("socket", "AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM");
    }
    
    btSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    
    if (btSocket == INVALID_SOCKET) {
        QString error = getLastSocketError();
        if (logger) {
            logger->logApiResult("socket", QString("FAILED - %1").arg(error), false);
            logger->error("Connection", QString("Не удалось создать сокет: %1").arg(error));
        }
        cleanupWinsock();
        emit connectionFailed("Не удалось создать Bluetooth сокет");
        return false;
    }
    
    if (logger) {
        logger->logApiResult("socket", QString("SUCCESS - Socket Handle = 0x%1").arg((quintptr)btSocket, 0, 16), true);
        logger->success("Connection", "✓ Bluetooth сокет создан");
        logger->debug("Connection", QString("Socket handle: 0x%1").arg((quintptr)btSocket, 0, 16));
        logger->debug("Connection", "");
    }
    
    // Шаг 3: Парсинг MAC адреса
    if (logger) {
        logger->info("Connection", "ШАГ 2: Парсинг MAC адреса устройства");
    }
    
    BLUETOOTH_ADDRESS btAddr;
    if (!parseMacAddress(deviceAddress, btAddr)) {
        closesocket(btSocket);
        btSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit connectionFailed("Неверный формат MAC адреса");
        return false;
    }
    
    if (logger) {
        logger->debug("Connection", "");
    }
    
    // Шаг 4: Настройка адреса для подключения
    if (logger) {
        logger->info("Connection", "ШАГ 3: Настройка параметров подключения");
    }
    
    SOCKADDR_BTH sockAddrBth;
    ZeroMemory(&sockAddrBth, sizeof(sockAddrBth));
    sockAddrBth.addressFamily = AF_BTH;
    sockAddrBth.btAddr = btAddr.ullLong;
    sockAddrBth.serviceClassId = RFCOMM_PROTOCOL_UUID;  // Serial Port Profile
    sockAddrBth.port = BT_PORT_ANY;  // Автоматический выбор порта
    
    if (logger) {
        logger->debug("Connection", "Параметры:");
        logger->debug("Connection", "  • addressFamily: AF_BTH");
        logger->debug("Connection", QString("  • btAddr: 0x%1").arg(btAddr.ullLong, 0, 16));
        logger->debug("Connection", "  • serviceClassId: RFCOMM_PROTOCOL_UUID");
        logger->debug("Connection", "  • port: BT_PORT_ANY (автоматически)");
        logger->debug("Connection", "");
    }
    
    // Шаг 5: Подключение
    if (logger) {
        logger->info("Connection", "ШАГ 4: Инициация подключения");
        logger->warning("Connection", "⏱ Это может занять 5-30 секунд...");
        logger->logApiCall("connect", QString("socket=0x%1, addr=%2")
            .arg((quintptr)btSocket, 0, 16)
            .arg(deviceAddress));
    }
    
    int connectResult = ::connect(btSocket, (SOCKADDR*)&sockAddrBth, sizeof(sockAddrBth));
    
    if (connectResult == SOCKET_ERROR) {
        QString error = getLastSocketError();
        
        if (logger) {
            logger->logApiResult("connect", QString("FAILED - %1").arg(error), false);
            logger->error("Connection", "");
            logger->error("Connection", "✗ ПОДКЛЮЧЕНИЕ НЕ УДАЛОСЬ");
            logger->error("Connection", "");
            logger->error("Connection", QString("Ошибка: %1").arg(error));
            logger->error("Connection", "");
            logger->warning("Connection", "ВОЗМОЖНЫЕ ПРИЧИНЫ:");
            logger->warning("Connection", "1. Устройство не поддерживает Serial Port Profile (SPP)");
            logger->warning("Connection", "2. Устройство не в режиме обнаружения");
            logger->warning("Connection", "3. Bluetooth выключен на устройстве");
            logger->warning("Connection", "4. Устройство отклонило подключение");
            logger->warning("Connection", "5. Таймаут подключения");
            logger->warning("Connection", "");
        }
        
        closesocket(btSocket);
        btSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit connectionFailed(QString("Не удалось подключиться: %1").arg(error));
        return false;
    }
    
    // Успех!
    connected = true;
    connectedDeviceName = deviceName;
    connectedDeviceAddress = deviceAddress;
    
    // КРИТИЧНО: Делаем сокет НЕБЛОКИРУЮЩИМ для предотвращения зависания UI!
    u_long nonBlocking = 1;  // 1 = неблокирующий режим, 0 = блокирующий
    int ioctlResult = ioctlsocket(btSocket, FIONBIO, &nonBlocking);
    
    if (ioctlResult == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (logger) {
            logger->warning("Connection", QString("⚠ Не удалось установить неблокирующий режим: %1").arg(error));
            logger->warning("Connection", "recv() будет блокирующим - возможно зависание UI!");
        }
    } else {
        if (logger) {
            logger->success("Connection", "✓ Сокет переведен в НЕБЛОКИРУЮЩИЙ режим");
            logger->debug("Connection", "recv() не будет блокировать UI поток");
        }
    }
    
    if (logger) {
        logger->logApiResult("connect", "SUCCESS - Подключено!", true);
        logger->success("Connection", "");
        logger->success("Connection", "✓✓✓ ПОДКЛЮЧЕНИЕ УСПЕШНО! ✓✓✓");
        logger->success("Connection", "");
        logger->success("Connection", QString("Установлено соединение с: %1").arg(deviceName));
        logger->success("Connection", QString("Адрес: %1").arg(deviceAddress));
        logger->success("Connection", "Можно отправлять данные!");
        logger->success("Connection", "");
    }
    
    emit connectionEstablished(deviceName);
    return true;
}

void BluetoothConnection::disconnect()
{
    if (btSocket != INVALID_SOCKET) {
        if (logger) {
            logger->info("Connection", "Закрытие соединения...");
            logger->logApiCall("closesocket", QString("0x%1").arg((quintptr)btSocket, 0, 16));
        }
        
        closesocket(btSocket);
        btSocket = INVALID_SOCKET;
        
        if (logger) {
            logger->logApiResult("closesocket", "SUCCESS", true);
            logger->info("Connection", "✓ Соединение закрыто");
        }
    }
    
    if (connected) {
        connected = false;
        emit disconnected();
    }
}

qint64 BluetoothConnection::sendData(const QByteArray &data)
{
    if (!connected || btSocket == INVALID_SOCKET) {
        if (logger) {
            logger->error("Connection", "Попытка отправки без подключения!");
        }
        return -1;
    }
    
    if (logger) {
        logger->debug("Connection", QString("Отправка %1 байт...").arg(data.size()));
        logger->logApiCall("send", QString("socket=0x%1, size=%2").arg((quintptr)btSocket, 0, 16).arg(data.size()));
    }
    
    int bytesSent = ::send(btSocket, data.constData(), data.size(), 0);
    
    if (bytesSent == SOCKET_ERROR) {
        int error = WSAGetLastError();
        
        // WSAEWOULDBLOCK (10035) - буфер отправки полон, НЕ фатальная ошибка
        if (error == WSAEWOULDBLOCK || error == 10035) {
            if (logger) {
                logger->logApiResult("send", "WOULD_BLOCK - Буфер полон", false);
                logger->debug("Connection", "Буфер отправки полон, повторить позже");
            }
            return 0;  // Возвращаем 0 = "попробуй позже"
        }
        
        // Реальная ошибка
        QString errorStr = getLastSocketError();
        if (logger) {
            logger->logApiResult("send", QString("FAILED - %1").arg(errorStr), false);
            logger->error("Connection", QString("Ошибка отправки: %1").arg(errorStr));
        }
        return -1;  // Возвращаем -1 = фатальная ошибка
    }
    
    if (logger) {
        logger->logApiResult("send", QString("SUCCESS - Отправлено %1 байт").arg(bytesSent), true);
        logger->debug("Connection", QString("✓ Отправлено: %1 байт").arg(bytesSent));
    }
    
    return bytesSent;
}

QByteArray BluetoothConnection::receiveData(int maxSize)
{
    if (!connected || btSocket == INVALID_SOCKET) {
        return QByteArray();
    }
    
    char buffer[4096];
    int bytesToRead = qMin(maxSize, (int)sizeof(buffer));
    
    // НЕ логируем вызов recv() каждый раз - слишком много записей
    // Логируем только когда есть данные или ошибки
    
    int bytesReceived = ::recv(btSocket, buffer, bytesToRead, 0);
    
    if (bytesReceived == SOCKET_ERROR) {
        int error = WSAGetLastError();
        
        // WSAEWOULDBLOCK (10035) - это НОРМАЛЬНО для неблокирующего сокета
        // Означает что данных пока нет, просто возвращаем пустой массив
        if (error == WSAEWOULDBLOCK || error == 10035) {
            // Данных пока нет - это не ошибка
            return QByteArray();
        }
        
        // Реальная ошибка
        QString errorStr = QString("Error %1").arg(error);
        if (logger) {
            logger->logApiResult("recv", QString("FAILED - %1").arg(errorStr), false);
            logger->error("Connection", QString("Ошибка приема данных: %1").arg(errorStr));
        }
        return QByteArray();
    }
    
    if (bytesReceived == 0) {
        // Соединение закрыто удаленной стороной
        if (logger) {
            logger->warning("Connection", "recv() вернул 0 - соединение закрыто удаленной стороной");
        }
        return QByteArray();
    }
    
    // Успешно получены данные - ТЕПЕРЬ логируем
    if (logger) {
        logger->logApiCall("recv", QString("socket=0x%1, maxSize=%2").arg((quintptr)btSocket, 0, 16).arg(bytesToRead));
        logger->logApiResult("recv", QString("SUCCESS - Получено %1 байт").arg(bytesReceived), true);
    }
    
    return QByteArray(buffer, bytesReceived);
}

QString BluetoothConnection::getLastSocketError()
{
    int error = WSAGetLastError();
    
    QString errorMsg;
    switch (error) {
    case WSAETIMEDOUT:
        errorMsg = "WSAETIMEDOUT (10060): Таймаут подключения";
        break;
    case WSAECONNREFUSED:
        errorMsg = "WSAECONNREFUSED (10061): Подключение отклонено";
        break;
    case WSAEHOSTUNREACH:
        errorMsg = "WSAEHOSTUNREACH (10065): Устройство недоступно";
        break;
    case WSAENOTCONN:
        errorMsg = "WSAENOTCONN (10057): Сокет не подключен";
        break;
    case WSAENOTSOCK:
        errorMsg = "WSAENOTSOCK (10038): Неверный сокет";
        break;
    default:
        errorMsg = QString("WSA Error %1").arg(error);
        break;
    }
    
    if (logger) {
        logger->debug("Connection", QString("WSAGetLastError(): 0x%1 - %2").arg(error, 0, 16).arg(errorMsg));
    }
    
    return errorMsg;
}

