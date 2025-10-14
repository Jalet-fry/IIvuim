#ifndef OBEXFILESENDER_H
#define OBEXFILESENDER_H

#include <QObject>
#include <QString>
#include <winsock2.h>
#include <ws2bth.h>
#include <BluetoothAPIs.h>

class BluetoothLogger;

// OBEX OpCodes
#define OBEX_CONNECT    0x80
#define OBEX_DISCONNECT 0x81
#define OBEX_PUT        0x02
#define OBEX_PUT_FINAL  0x82
#define OBEX_GET        0x03
#define OBEX_SETPATH    0x85

// OBEX Response Codes
#define OBEX_RSP_SUCCESS         0xA0  // 160 - OK, Success
#define OBEX_RSP_CONTINUE        0x90  // 144 - Continue
#define OBEX_RSP_BAD_REQUEST     0xC0  // 192 - Bad Request
#define OBEX_RSP_UNAUTHORIZED    0xC1  // 193 - Unauthorized
#define OBEX_RSP_FORBIDDEN       0xC3  // 195 - Forbidden
#define OBEX_RSP_NOT_FOUND       0xC4  // 196 - Not Found

// OBEX Header IDs
#define OBEX_HDR_NAME        0x01  // Unicode text (null terminated)
#define OBEX_HDR_TYPE        0x42  // Byte sequence (ASCII)
#define OBEX_HDR_LENGTH      0xC3  // 4-byte value
#define OBEX_HDR_BODY        0x48  // Byte sequence
#define OBEX_HDR_END_OF_BODY 0x49  // Byte sequence (final)
#define OBEX_HDR_CONNECTION  0xCB  // 4-byte connection ID

// OBEX FTP Service UUID
static const GUID OBEX_PUSH_SERVICE_UUID = 
    { 0x00001105, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };

// Класс для прямой отправки файлов через OBEX протокол
class ObexFileSender : public QObject
{
    Q_OBJECT
    
public:
    explicit ObexFileSender(BluetoothLogger *logger, QObject *parent = nullptr);
    ~ObexFileSender();
    
    // Отправка файла через OBEX напрямую на Android/телефон
    bool sendFileViaObex(const QString &filePath, const QString &deviceAddress, const QString &deviceName);
    
signals:
    void transferStarted(const QString &fileName);
    void transferProgress(qint64 bytesSent, qint64 totalBytes);
    void transferCompleted(const QString &fileName);
    void transferFailed(const QString &error);
    
private:
    BluetoothLogger *logger;
    SOCKET obexSocket;
    bool connected;
    quint32 connectionId;
    
    // Парсинг MAC адреса
    bool parseMacAddress(const QString &address, BLUETOOTH_ADDRESS &btAddr);
    
    // Инициализация Winsock
    bool initWinsock();
    void cleanupWinsock();
    
    // OBEX протокол
    bool obexConnect();
    bool obexPut(const QString &fileName, const QByteArray &fileData);
    bool obexDisconnect();
    
    // Отправка/прием OBEX пакетов
    bool sendObexPacket(const QByteArray &packet);
    QByteArray receiveObexResponse();
    
    // Формирование OBEX пакетов
    QByteArray buildConnectPacket();
    QByteArray buildPutPacket(const QString &fileName, qint64 fileSize, bool final = false);
    QByteArray buildDisconnectPacket();
    
    // Вспомогательные функции
    void writeUInt16BE(QByteArray &data, quint16 value);  // Big-endian 16-bit
    void writeUInt32BE(QByteArray &data, quint32 value);  // Big-endian 32-bit
    QByteArray encodeUnicode(const QString &str);
    
    QString getLastSocketError();
};

#endif // OBEXFILESENDER_H

