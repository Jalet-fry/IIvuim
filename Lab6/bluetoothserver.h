#ifndef BLUETOOTHSERVER_H
#define BLUETOOTHSERVER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <winsock2.h>
#include <ws2bth.h>

class BluetoothLogger;

// Класс для приема файлов через Bluetooth RFCOMM
class BluetoothServer : public QObject
{
    Q_OBJECT
    
public:
    explicit BluetoothServer(BluetoothLogger *logger, QObject *parent = nullptr);
    ~BluetoothServer();
    
    // Запуск сервера для приема файлов
    void startServer();
    
    // Остановка сервера
    void stopServer();
    
    // Проверка статуса сервера
    bool isRunning() const { return running; }
    
signals:
    void serverStarted();
    void serverStopped();
    void fileReceived(const QString &fileName);
    void transferProgress(qint64 bytesReceived, qint64 totalBytes);
    void transferCompleted(const QString &fileName);
    void transferFailed(const QString &error);
    
private slots:
    void runServer();
    
private:
    BluetoothLogger *logger;
    QThread *serverThread;
    bool running;
    bool shouldStop;
    SOCKET serverSocket;
    
    // Инициализация Winsock
    bool initWinsock();
    void cleanupWinsock();
    
    // Прием файла
    void receiveFile(SOCKET clientSocket);
    
    // Получение ошибки сокета
    QString getLastSocketError();
};

#endif // BLUETOOTHSERVER_H
