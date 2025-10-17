#include "obexfilesender.h"
#include "bluetoothlogger.h"
#include <QFile>
#include <QFileInfo>
#include <QDataStream>

ObexFileSender::ObexFileSender(BluetoothLogger *logger, QObject *parent)
    : QObject(parent)
    , logger(logger)
    , obexSocket(INVALID_SOCKET)
    , connected(false)
    , connectionId(0)
{
}

ObexFileSender::~ObexFileSender()
{
    if (obexSocket != INVALID_SOCKET) {
        closesocket(obexSocket);
        cleanupWinsock();
    }
}

bool ObexFileSender::sendFileViaObex(const QString &filePath, const QString &deviceAddress, const QString &deviceName)
{
    if (!logger) return false;
    
    logger->info("OBEX", "═══════════════════════════════════════");
    logger->info("OBEX", "ОТПРАВКА ФАЙЛА НА КОМПЬЮТЕР");
    logger->info("OBEX", "═══════════════════════════════════════");
    logger->info("OBEX", QString("Устройство: %1").arg(deviceName));
    logger->info("OBEX", QString("MAC: %1").arg(deviceAddress));
    logger->info("OBEX", QString("Файл: %1").arg(filePath));
    logger->info("OBEX", "");
    
    // Определяем тип устройства
    bool isComputer = deviceName.contains("COMPIK", Qt::CaseInsensitive) || 
                     deviceName.contains("PC", Qt::CaseInsensitive) ||
                     deviceName.contains("Computer", Qt::CaseInsensitive);
    
    if (isComputer) {
        logger->info("OBEX", "🎯 ОБНАРУЖЕН КОМПЬЮТЕР - используем RFCOMM");
        logger->info("OBEX", "Прямая передача данных без OBEX");
        logger->info("OBEX", "");
        return sendFileViaRfcomm(filePath, deviceAddress, deviceName);
    } else {
        logger->info("OBEX", "📱 ОБНАРУЖЕНО МОБИЛЬНОЕ УСТРОЙСТВО - используем OBEX");
        logger->info("OBEX", "Стандартный OBEX протокол");
        logger->info("OBEX", "");
    }
    
    // Открываем файл
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        logger->error("OBEX", QString("Не удалось открыть файл: %1").arg(file.errorString()));
        return false;
    }
    
    QFileInfo fileInfo(filePath);
    QByteArray fileData = file.readAll();
    file.close();
    
    logger->info("OBEX", QString("Размер файла: %1 байт (%2 MB)")
        .arg(fileData.size())
        .arg(fileData.size() / 1024.0 / 1024.0, 0, 'f', 2));
    logger->info("OBEX", "");
    
    emit transferStarted(fileInfo.fileName());
    
    // Инициализация Winsock
    if (!initWinsock()) {
        emit transferFailed("Ошибка инициализации Winsock");
        return false;
    }
    
    // Создание RFCOMM сокета
    logger->info("OBEX", "ШАГ 1: Создание Bluetooth сокета");
    obexSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    
    if (obexSocket == INVALID_SOCKET) {
        logger->error("OBEX", QString("Не удалось создать сокет: %1").arg(getLastSocketError()));
        cleanupWinsock();
        emit transferFailed("Не удалось создать сокет");
        return false;
    }
    
    logger->success("OBEX", "✓ Bluetooth сокет создан");
    logger->info("OBEX", "");
    
    // Парсинг MAC адреса
    logger->info("OBEX", "ШАГ 2: Парсинг MAC адреса");
    BLUETOOTH_ADDRESS btAddr;
    if (!parseMacAddress(deviceAddress, btAddr)) {
        logger->error("OBEX", "Неверный формат MAC адреса");
        closesocket(obexSocket);
        obexSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit transferFailed("Неверный формат MAC адреса");
        return false;
    }
    
    logger->success("OBEX", "✓ MAC адрес распознан");
    logger->info("OBEX", "");
    
    // Настройка адреса для OBEX Push (Object Push Profile)
    logger->info("OBEX", "ШАГ 3: Подключение к OBEX Push сервису");
    logger->debug("OBEX", "UUID: 00001105-0000-1000-8000-00805f9b34fb (Object Push)");
    
    SOCKADDR_BTH sockAddrBth;
    ZeroMemory(&sockAddrBth, sizeof(sockAddrBth));
    sockAddrBth.addressFamily = AF_BTH;
    sockAddrBth.btAddr = btAddr.ullLong;
    sockAddrBth.serviceClassId = OBEX_PUSH_SERVICE_UUID;  // Object Push Profile
    sockAddrBth.port = BT_PORT_ANY;
    
    logger->info("OBEX", "Подключение к устройству...");
    logger->warning("OBEX", "⏱ Это может занять 5-15 секунд...");
    
    int connectResult = ::connect(obexSocket, (SOCKADDR*)&sockAddrBth, sizeof(sockAddrBth));
    
    if (connectResult == SOCKET_ERROR) {
        QString error = getLastSocketError();
        logger->error("OBEX", QString("Ошибка подключения: %1").arg(error));
        closesocket(obexSocket);
        obexSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit transferFailed(QString("Не удалось подключиться к OBEX сервису: %1").arg(error));
        return false;
    }
    
    logger->success("OBEX", "✓ Подключено к OBEX Push сервису!");
    logger->info("OBEX", "");
    
    // Делаем сокет неблокирующим
    u_long nonBlocking = 1;
    ioctlsocket(obexSocket, FIONBIO, &nonBlocking);
    logger->debug("OBEX", "✓ Сокет переведен в неблокирующий режим");
    logger->info("OBEX", "");
    
    // OBEX CONNECT
    logger->info("OBEX", "ШАГ 4: OBEX CONNECT");
    if (!obexConnect()) {
        closesocket(obexSocket);
        obexSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit transferFailed("Ошибка OBEX CONNECT");
        return false;
    }
    
    logger->success("OBEX", "✓ OBEX сессия установлена");
    logger->info("OBEX", "");
    
    // OBEX PUT
    logger->info("OBEX", "ШАГ 5: OBEX PUT (отправка файла)");
    logger->info("OBEX", QString("Имя файла: %1").arg(fileInfo.fileName()));
    logger->info("OBEX", QString("Размер данных: %1 байт").arg(fileData.size()));
    logger->info("OBEX", "");
    
    if (!obexPut(fileInfo.fileName(), fileData)) {
        obexDisconnect();
        closesocket(obexSocket);
        obexSocket = INVALID_SOCKET;
        cleanupWinsock();
        emit transferFailed("Ошибка OBEX PUT");
        return false;
    }
    
    logger->success("OBEX", "✓ Файл отправлен через OBEX!");
    logger->info("OBEX", "");
    
    // OBEX DISCONNECT
    logger->info("OBEX", "ШАГ 6: OBEX DISCONNECT");
    obexDisconnect();
    
    logger->success("OBEX", "");
    logger->success("OBEX", "✓✓✓ ФАЙЛ УСПЕШНО ОТПРАВЛЕН ЧЕРЕЗ OBEX! ✓✓✓");
    logger->success("OBEX", "");
    logger->info("OBEX", "На телефоне должен был появиться диалог 'Принять файл?'");
    logger->info("OBEX", "");
    
    // Закрываем соединение
    closesocket(obexSocket);
    obexSocket = INVALID_SOCKET;
    cleanupWinsock();
    
    emit transferCompleted(fileInfo.fileName());
    return true;
}

bool ObexFileSender::sendFileViaRfcomm(const QString &filePath, const QString &deviceAddress, const QString &deviceName)
{
    if (!logger) return false;
    
    logger->info("RFCOMM", "═══════════════════════════════════════");
    logger->info("RFCOMM", "ПРЯМАЯ ПЕРЕДАЧА ПК-ПК ЧЕРЕЗ RFCOMM");
    logger->info("RFCOMM", "═══════════════════════════════════════");
    logger->info("RFCOMM", QString("Устройство: %1").arg(deviceName));
    logger->info("RFCOMM", QString("MAC: %1").arg(deviceAddress));
    logger->info("RFCOMM", QString("Файл: %1").arg(filePath));
    logger->info("RFCOMM", "");
    
    // Открываем файл
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        logger->error("RFCOMM", QString("Не удалось открыть файл: %1").arg(file.errorString()));
        emit transferFailed("Не удалось открыть файл");
        return false;
    }
    
    QFileInfo fileInfo(filePath);
    qint64 fileSize = file.size();
    
    logger->info("RFCOMM", QString("Размер файла: %1 байт (%2 MB)")
        .arg(fileSize)
        .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 2));
    logger->info("RFCOMM", "");
    
    emit transferStarted(fileInfo.fileName());
    
    // Инициализация Winsock
    if (!initWinsock()) {
        emit transferFailed("Ошибка инициализации Winsock");
        file.close();
        return false;
    }
    
    // Создание Bluetooth сокета
    logger->info("RFCOMM", "ШАГ 1: Создание Bluetooth сокета");
    SOCKET btSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    
    if (btSocket == INVALID_SOCKET) {
        logger->error("RFCOMM", QString("Не удалось создать сокет: %1").arg(getLastSocketError()));
        cleanupWinsock();
        file.close();
        emit transferFailed("Не удалось создать Bluetooth сокет");
        return false;
    }
    
    logger->success("RFCOMM", "✓ Bluetooth сокет создан");
    logger->info("RFCOMM", "");
    
    // Парсинг MAC адреса
    logger->info("RFCOMM", "ШАГ 2: Парсинг MAC адреса");
    BLUETOOTH_ADDRESS btAddr;
    if (!parseMacAddress(deviceAddress, btAddr)) {
        logger->error("RFCOMM", "Неверный формат MAC адреса");
        closesocket(btSocket);
        cleanupWinsock();
        file.close();
        emit transferFailed("Неверный формат MAC адреса");
        return false;
    }
    
    logger->success("RFCOMM", "✓ MAC адрес распознан");
    logger->info("RFCOMM", "");
    
    // Настройка адреса для RFCOMM подключения
    logger->info("RFCOMM", "ШАГ 3: Настройка RFCOMM подключения");
    SOCKADDR_BTH remoteAddress;
    ZeroMemory(&remoteAddress, sizeof(remoteAddress));
    remoteAddress.addressFamily = AF_BTH;
    remoteAddress.btAddr = btAddr.ullLong;
    remoteAddress.serviceClassId = RFCOMM_PROTOCOL_UUID;  // RFCOMM протокол
    remoteAddress.port = 11;  // Стандартный порт для RFCOMM
    
    logger->debug("RFCOMM", "Параметры подключения:");
    logger->debug("RFCOMM", QString("  • addressFamily: AF_BTH"));
    logger->debug("RFCOMM", QString("  • btAddr: 0x%1").arg(btAddr.ullLong, 12, 16, QChar('0')));
    logger->debug("RFCOMM", QString("  • serviceClassId: RFCOMM_PROTOCOL_UUID"));
    logger->debug("RFCOMM", QString("  • port: 11"));
    logger->info("RFCOMM", "");
    
    // Подключение к удаленному устройству
    logger->info("RFCOMM", "ШАГ 4: Подключение к устройству");
    logger->warning("RFCOMM", "⏱ Это может занять 5-15 секунд...");
    
    int connectResult = ::connect(btSocket, (SOCKADDR*)&remoteAddress, sizeof(remoteAddress));
    
    if (connectResult == SOCKET_ERROR) {
        QString error = getLastSocketError();
        logger->error("RFCOMM", QString("Ошибка подключения: %1").arg(error));
        logger->warning("RFCOMM", "ВОЗМОЖНЫЕ ПРИЧИНЫ:");
        logger->warning("RFCOMM", "1. Устройство не запущено в режиме сервера");
        logger->warning("RFCOMM", "2. RFCOMM сервис недоступен на устройстве");
        logger->warning("RFCOMM", "3. Устройство не в режиме приема файлов");
        logger->warning("RFCOMM", "4. Проблемы с Bluetooth драйверами");
        logger->warning("RFCOMM", "");
        logger->info("RFCOMM", "РЕКОМЕНДАЦИИ:");
        logger->info("RFCOMM", "1. Убедитесь что на ПК запущен сервер приема");
        logger->info("RFCOMM", "2. Проверьте что устройство видимо для других");
        logger->info("RFCOMM", "3. Попробуйте перезапустить Bluetooth на устройстве");
        
        closesocket(btSocket);
        cleanupWinsock();
        file.close();
        emit transferFailed(QString("Не удалось подключиться к устройству: %1").arg(error));
        return false;
    }
    
    logger->success("RFCOMM", "✓ Подключено к устройству!");
    logger->info("RFCOMM", "");
    
    // Делаем сокет неблокирующим
    u_long nonBlocking = 1;
    ioctlsocket(btSocket, FIONBIO, &nonBlocking);
    logger->debug("RFCOMM", "✓ Сокет переведен в неблокирующий режим");
    logger->info("RFCOMM", "");
    
    // Отправка файла
    logger->info("RFCOMM", "ШАГ 5: Отправка файла");
    logger->info("RFCOMM", QString("Начинаем передачу: %1").arg(fileInfo.fileName()));
    logger->info("RFCOMM", "");
    
    const int bufferSize = 1024;
    char buffer[bufferSize];
    qint64 totalSent = 0;
    int chunkNumber = 0;
    
    while (!file.atEnd()) {
        qint64 bytesRead = file.read(buffer, bufferSize);
        if (bytesRead <= 0) break;
        
        qint64 bytesSent = 0;
        while (bytesSent < bytesRead) {
            int result = ::send(btSocket, buffer + bytesSent, bytesRead - bytesSent, 0);
            
            if (result == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    // Буфер полон - ждем
                    Sleep(10);
                    continue;
                } else {
                    logger->error("RFCOMM", QString("Ошибка отправки: %1").arg(getLastSocketError()));
                    closesocket(btSocket);
                    cleanupWinsock();
                    file.close();
                    emit transferFailed("Ошибка отправки данных");
                    return false;
                }
            }
            
            bytesSent += result;
            totalSent += result;
        }
        
        chunkNumber++;
        
        // Логируем прогресс каждые 10 блоков
        if (chunkNumber % 10 == 0) {
            int progress = (totalSent * 100) / fileSize;
            logger->debug("RFCOMM", QString("Отправлено: %1/%2 байт (%3%)")
                .arg(totalSent).arg(fileSize).arg(progress));
            emit transferProgress(totalSent, fileSize);
        }
    }
    
    file.close();
    
    // Ждем завершения передачи
    logger->info("RFCOMM", "ШАГ 6: Завершение передачи");
    logger->info("RFCOMM", "⏱ Ожидание завершения...");
    Sleep(2000);  // 2 секунды
    
    // Закрываем соединение
    closesocket(btSocket);
    cleanupWinsock();
    
    logger->success("RFCOMM", "");
    logger->success("RFCOMM", "✓✓✓ ФАЙЛ УСПЕШНО ОТПРАВЛЕН! ✓✓✓");
    logger->success("RFCOMM", QString("Отправлено блоков: %1").arg(chunkNumber));
    logger->success("RFCOMM", QString("Всего байт: %1").arg(totalSent));
    logger->info("RFCOMM", "");
    logger->info("RFCOMM", "На принимающем ПК должен появиться файл");
    logger->info("RFCOMM", "");
    
    emit transferCompleted(fileInfo.fileName());
    return true;
}

bool ObexFileSender::initWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    if (result != 0) {
        if (logger) {
            logger->error("OBEX", QString("Ошибка WSAStartup: %1").arg(result));
        }
        return false;
    }
    
    if (logger) {
        logger->debug("OBEX", "✓ Winsock инициализирован");
    }
    
    return true;
}

void ObexFileSender::cleanupWinsock()
{
    WSACleanup();
    if (logger) {
        logger->debug("OBEX", "✓ Winsock очищен");
    }
}

bool ObexFileSender::parseMacAddress(const QString &address, BLUETOOTH_ADDRESS &btAddr)
{
    ZeroMemory(&btAddr, sizeof(btAddr));
    
    QStringList bytes = address.split(":");
    if (bytes.size() != 6) {
        return false;
    }
    
    for (int i = 0; i < 6; i++) {
        bool ok;
        btAddr.rgBytes[5 - i] = bytes[i].toUInt(&ok, 16);
        if (!ok) return false;
    }
    
    return true;
}

bool ObexFileSender::obexConnect()
{
    logger->debug("OBEX", "Формирование OBEX CONNECT пакета...");
    
    QByteArray packet = buildConnectPacket();
    
    logger->debug("OBEX", QString("Размер CONNECT пакета: %1 байт").arg(packet.size()));
    logger->debug("OBEX", "Отправка OBEX CONNECT...");
    
    if (!sendObexPacket(packet)) {
        logger->error("OBEX", "Ошибка отправки OBEX CONNECT");
        return false;
    }
    
    logger->debug("OBEX", "Ожидание ответа...");
    QByteArray response = receiveObexResponse();
    
    if (response.isEmpty() || (unsigned char)response[0] != OBEX_RSP_SUCCESS) {
        if (!response.isEmpty()) {
            logger->error("OBEX", QString("OBEX CONNECT отклонен: 0x%1").arg((unsigned char)response[0], 2, 16, QChar('0')));
        } else {
            logger->error("OBEX", "Нет ответа от OBEX сервера");
        }
        return false;
    }
    
    logger->success("OBEX", "✓ OBEX CONNECT SUCCESS (0xA0)");
    
    // Извлекаем Connection ID если есть
    if (response.size() >= 7) {
        // Проверяем наличие Connection ID header (0xCB)
        for (int i = 3; i < response.size() - 4; i++) {
            if ((unsigned char)response[i] == OBEX_HDR_CONNECTION) {
                connectionId = ((unsigned char)response[i+1] << 24) |
                              ((unsigned char)response[i+2] << 16) |
                              ((unsigned char)response[i+3] << 8) |
                              ((unsigned char)response[i+4]);
                logger->debug("OBEX", QString("Connection ID: 0x%1").arg(connectionId, 8, 16, QChar('0')));
                break;
            }
        }
    }
    
    connected = true;
    return true;
}

bool ObexFileSender::obexPut(const QString &fileName, const QByteArray &fileData)
{
    logger->debug("OBEX", "Формирование OBEX PUT пакета...");
    logger->debug("OBEX", QString("Имя файла: %1").arg(fileName));
    logger->debug("OBEX", QString("Размер данных: %1 байт").arg(fileData.size()));
    
    const int MAX_OBEX_PACKET = 32000;  // Максимальный размер OBEX пакета (32 KB)
    
    // Если файл маленький - отправляем за один раз
    if (fileData.size() <= MAX_OBEX_PACKET) {
        logger->info("OBEX", "Файл маленький - отправка одним пакетом");
        
        // Формируем PUT FINAL пакет
        QByteArray packet;
        
        // Opcode: PUT FINAL (0x82)
        packet.append((char)OBEX_PUT_FINAL);
        
        // Packet Length (будет заполнено позже)
        packet.append((char)0x00);
        packet.append((char)0x00);
        
        // Connection ID header (если есть)
        if (connectionId != 0) {
            packet.append((char)OBEX_HDR_CONNECTION);
            writeUInt32BE(packet, connectionId);
        }
        
        // Name header
        QByteArray nameUnicode = encodeUnicode(fileName);
        packet.append((char)OBEX_HDR_NAME);
        writeUInt16BE(packet, nameUnicode.size() + 3);
        packet.append(nameUnicode);
        
        // Length header
        packet.append((char)OBEX_HDR_LENGTH);
        writeUInt32BE(packet, fileData.size());
        
        // End of Body header
        packet.append((char)OBEX_HDR_END_OF_BODY);
        writeUInt16BE(packet, fileData.size() + 3);
        packet.append(fileData);
        
        // Заполняем длину пакета
        quint16 totalLength = packet.size();
        packet[1] = (totalLength >> 8) & 0xFF;
        packet[2] = totalLength & 0xFF;
        
        logger->debug("OBEX", QString("Размер PUT пакета: %1 байт").arg(packet.size()));
        logger->info("OBEX", "Отправка OBEX PUT...");
        logger->info("OBEX", "⏱ На телефоне должен появиться диалог 'Принять файл?'");
        logger->info("OBEX", "");
        
        if (!sendObexPacket(packet)) {
            logger->error("OBEX", "Ошибка отправки OBEX PUT");
            return false;
        }
        
        logger->debug("OBEX", "Ожидание ответа от телефона...");
        logger->info("OBEX", "💡 Если вы ПРИНЯЛИ файл на телефоне - ждите ответа...");
        logger->info("OBEX", "");
        
        QByteArray response = receiveObexResponse();
        
        if (response.isEmpty()) {
            logger->error("OBEX", "");
            logger->error("OBEX", "✗ НЕТ ОТВЕТА ОТ ТЕЛЕФОНА");
            logger->error("OBEX", "");
            logger->warning("OBEX", "ВОЗМОЖНЫЕ ПРИЧИНЫ:");
            logger->warning("OBEX", "1. Вы ПРИНЯЛИ файл, но ответ не дошел (проблема Bluetooth)");
            logger->warning("OBEX", "2. Проверьте телефон - файл может быть там!");
            logger->warning("OBEX", "3. Таймаут ожидания истек (30 секунд)");
            logger->warning("OBEX", "");
            return false;
        }
        
        unsigned char responseCode = (unsigned char)response[0];
        
        if (responseCode != OBEX_RSP_SUCCESS) {
            logger->error("OBEX", QString("OBEX PUT отклонен: 0x%1").arg(responseCode, 2, 16, QChar('0')));
            
            if (responseCode == OBEX_RSP_FORBIDDEN) {
                logger->warning("OBEX", "Пользователь ОТКЛОНИЛ файл на телефоне");
            }
            return false;
        }
        
        logger->success("OBEX", "✓ OBEX PUT SUCCESS (0xA0)");
        logger->success("OBEX", "Файл принят телефоном!");
        
        return true;
    } else {
        // Файл большой - отправляем по частям
        logger->info("OBEX", "Файл большой - многопакетная отправка");
        logger->info("OBEX", QString("Будет отправлено пакетов: %1").arg((fileData.size() / MAX_OBEX_PACKET) + 1));
        logger->info("OBEX", "");
        
        qint64 offset = 0;
        int packetNum = 0;
        
        while (offset < fileData.size()) {
            qint64 chunkSize = qMin((qint64)MAX_OBEX_PACKET, fileData.size() - offset);
            bool isLast = (offset + chunkSize >= fileData.size());
            
            packetNum++;
            logger->debug("OBEX", QString("Пакет %1: offset=%2, size=%3, last=%4")
                .arg(packetNum).arg(offset).arg(chunkSize).arg(isLast ? "ДА" : "НЕТ"));
            
            QByteArray packet;
            
            // Opcode: PUT или PUT FINAL
            packet.append((char)(isLast ? OBEX_PUT_FINAL : OBEX_PUT));
            packet.append((char)0x00);
            packet.append((char)0x00);
            
            // Connection ID
            if (connectionId != 0) {
                packet.append((char)OBEX_HDR_CONNECTION);
                writeUInt32BE(packet, connectionId);
            }
            
            // Первый пакет - добавляем имя и размер
            if (offset == 0) {
                QByteArray nameUnicode = encodeUnicode(fileName);
                packet.append((char)OBEX_HDR_NAME);
                writeUInt16BE(packet, nameUnicode.size() + 3);
                packet.append(nameUnicode);
                
                packet.append((char)OBEX_HDR_LENGTH);
                writeUInt32BE(packet, fileData.size());
            }
            
            // Body или End of Body
            QByteArray chunk = fileData.mid(offset, chunkSize);
            packet.append((char)(isLast ? OBEX_HDR_END_OF_BODY : OBEX_HDR_BODY));
            writeUInt16BE(packet, chunk.size() + 3);
            packet.append(chunk);
            
            // Длина пакета
            quint16 totalLength = packet.size();
            packet[1] = (totalLength >> 8) & 0xFF;
            packet[2] = totalLength & 0xFF;
            
            logger->debug("OBEX", QString("Отправка пакета %1 (%2 байт)...").arg(packetNum).arg(packet.size()));
            
            if (!sendObexPacket(packet)) {
                logger->error("OBEX", QString("Ошибка отправки пакета %1").arg(packetNum));
                return false;
            }
            
            // Ждем ответ
            QByteArray response = receiveObexResponse();
            
            if (response.isEmpty()) {
                logger->error("OBEX", QString("Нет ответа на пакет %1").arg(packetNum));
                return false;
            }
            
            unsigned char responseCode = (unsigned char)response[0];
            
            if (isLast) {
                // Последний пакет - ждем SUCCESS
                if (responseCode != OBEX_RSP_SUCCESS) {
                    logger->error("OBEX", QString("OBEX PUT отклонен: 0x%1").arg(responseCode, 2, 16, QChar('0')));
                    return false;
                }
                logger->success("OBEX", "✓ OBEX PUT FINAL SUCCESS!");
            } else {
                // Промежуточный пакет - ждем CONTINUE
                if (responseCode != OBEX_RSP_CONTINUE && responseCode != OBEX_RSP_SUCCESS) {
                    logger->error("OBEX", QString("Ошибка на пакете %1: 0x%2").arg(packetNum).arg(responseCode, 2, 16, QChar('0')));
                    return false;
                }
                logger->debug("OBEX", QString("✓ Пакет %1 принят (0x%2)").arg(packetNum).arg(responseCode, 2, 16, QChar('0')));
            }
            
            offset += chunkSize;
            
            // Прогресс
            emit transferProgress(offset, fileData.size());
        }
        
        logger->success("OBEX", "");
        logger->success("OBEX", "✓ Все пакеты отправлены успешно!");
        logger->success("OBEX", "Файл принят телефоном!");
        logger->success("OBEX", "");
        
        return true;
    }
}

bool ObexFileSender::obexDisconnect()
{
    if (!connected) return true;
    
    logger->debug("OBEX", "Отправка OBEX DISCONNECT...");
    
    QByteArray packet = buildDisconnectPacket();
    
    if (!sendObexPacket(packet)) {
        logger->warning("OBEX", "Ошибка отправки DISCONNECT (не критично)");
        return false;
    }
    
    // Не обязательно ждать ответ
    logger->success("OBEX", "✓ OBEX DISCONNECT отправлен");
    
    connected = false;
    return true;
}

bool ObexFileSender::sendObexPacket(const QByteArray &packet)
{
    if (obexSocket == INVALID_SOCKET) {
        return false;
    }
    
    logger->logApiCall("send", QString("OBEX пакет, размер=%1").arg(packet.size()));
    
    int sent = ::send(obexSocket, packet.constData(), packet.size(), 0);
    
    if (sent == SOCKET_ERROR) {
        QString error = getLastSocketError();
        logger->logApiResult("send", QString("FAILED - %1").arg(error), false);
        return false;
    }
    
    logger->logApiResult("send", QString("SUCCESS - %1 байт").arg(sent), true);
    
    return (sent == packet.size());
}

QByteArray ObexFileSender::receiveObexResponse()
{
    if (obexSocket == INVALID_SOCKET) {
        return QByteArray();
    }
    
    // Ждем данных с таймаутом
    char buffer[1024];
    int totalReceived = 0;
    int attempts = 0;
    const int maxAttempts = 300;  // 30 секунд (300 * 100ms) - больше времени для телефона
    
    logger->debug("OBEX", "Ожидание ответа от телефона...");
    
    while (attempts < maxAttempts) {
        int received = ::recv(obexSocket, buffer + totalReceived, sizeof(buffer) - totalReceived, 0);
        
        if (received == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Нет данных пока - ждем
                Sleep(100);
                attempts++;
                
                // Логируем каждые 5 секунд
                if (attempts % 50 == 0) {
                    logger->debug("OBEX", QString("Ожидание... (%1 сек)").arg(attempts / 10));
                }
                continue;
            }
            
            logger->error("OBEX", QString("Ошибка recv: %1").arg(error));
            return QByteArray();
        }
        
        if (received > 0) {
            totalReceived += received;
            logger->debug("OBEX", QString("Получено %1 байт ответа (всего: %2)").arg(received).arg(totalReceived));
            
            // Логируем первые байты для отладки
            if (totalReceived >= 1) {
                logger->debug("OBEX", QString("Response Code: 0x%1").arg((unsigned char)buffer[0], 2, 16, QChar('0')));
            }
            
            // Проверяем что получили полный ответ (минимум 3 байта: opcode + length)
            if (totalReceived >= 3) {
                quint16 packetLength = ((unsigned char)buffer[1] << 8) | (unsigned char)buffer[2];
                logger->debug("OBEX", QString("Ожидаемая длина пакета: %1 байт").arg(packetLength));
                
                if (totalReceived >= packetLength) {
                    logger->success("OBEX", QString("✓ Получен полный ответ (%1 байт)").arg(packetLength));
                    
                    // Проверяем response code
                    unsigned char responseCode = (unsigned char)buffer[0];
                    if (responseCode == OBEX_RSP_SUCCESS) {
                        logger->success("OBEX", "✓ Response: SUCCESS (0xA0)");
                    } else if (responseCode == OBEX_RSP_CONTINUE) {
                        logger->info("OBEX", "→ Response: CONTINUE (0x90)");
                    } else if (responseCode == OBEX_RSP_FORBIDDEN) {
                        logger->warning("OBEX", "⚠ Response: FORBIDDEN (0xC3) - Файл отклонен пользователем");
                    } else {
                        logger->warning("OBEX", QString("⚠ Response: 0x%1").arg(responseCode, 2, 16, QChar('0')));
                    }
                    
                    return QByteArray(buffer, packetLength);
                }
            }
        }
    }
    
    logger->warning("OBEX", "Таймаут ожидания ответа");
    return QByteArray();
}

QByteArray ObexFileSender::buildConnectPacket()
{
    QByteArray packet;
    
    // Opcode: CONNECT (0x80)
    packet.append((char)OBEX_CONNECT);
    
    // Packet Length (будет заполнено позже)
    packet.append((char)0x00);
    packet.append((char)0x00);
    
    // OBEX Version (1.0)
    packet.append((char)0x10);
    
    // Flags (0x00)
    packet.append((char)0x00);
    
    // Max packet length (0xFFFF = 65535)
    packet.append((char)0xFF);
    packet.append((char)0xFF);
    
    // Обновляем длину пакета
    quint16 totalLength = packet.size();
    packet[1] = (totalLength >> 8) & 0xFF;
    packet[2] = totalLength & 0xFF;
    
    return packet;
}

QByteArray ObexFileSender::buildPutPacket(const QString &fileName, qint64 fileSize, bool final)
{
    // Эта функция используется для построения PUT пакета
    // Но основную логику мы сделали в obexPut()
    return QByteArray();
}

QByteArray ObexFileSender::buildDisconnectPacket()
{
    QByteArray packet;
    
    // Opcode: DISCONNECT (0x81)
    packet.append((char)OBEX_DISCONNECT);
    
    // Packet Length
    packet.append((char)0x00);
    packet.append((char)0x03);  // Минимальный размер
    
    return packet;
}

void ObexFileSender::writeUInt16BE(QByteArray &data, quint16 value)
{
    data.append((value >> 8) & 0xFF);  // High byte
    data.append(value & 0xFF);          // Low byte
}

void ObexFileSender::writeUInt32BE(QByteArray &data, quint32 value)
{
    data.append((value >> 24) & 0xFF);
    data.append((value >> 16) & 0xFF);
    data.append((value >> 8) & 0xFF);
    data.append(value & 0xFF);
}

QByteArray ObexFileSender::encodeUnicode(const QString &str)
{
    QByteArray result;
    
    // OBEX использует UTF-16 Big Endian с null terminator
    for (int i = 0; i < str.length(); i++) {
        quint16 ch = str[i].unicode();
        result.append((ch >> 8) & 0xFF);  // High byte
        result.append(ch & 0xFF);          // Low byte
    }
    
    // Null terminator
    result.append((char)0x00);
    result.append((char)0x00);
    
    return result;
}

QString ObexFileSender::getLastSocketError()
{
    int error = WSAGetLastError();
    
    switch (error) {
    case WSAETIMEDOUT:
        return QString("WSAETIMEDOUT (%1): Таймаут подключения").arg(error);
    case WSAECONNREFUSED:
        return QString("WSAECONNREFUSED (%1): Подключение отклонено").arg(error);
    case WSAEWOULDBLOCK:
        return QString("WSAEWOULDBLOCK (%1): Операция заблокирована").arg(error);
    default:
        return QString("WSA Error %1").arg(error);
    }
}

