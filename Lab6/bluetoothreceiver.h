#ifndef BLUETOOTHRECEIVER_H
#define BLUETOOTHRECEIVER_H

#include <QObject>
#include <QFile>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "bluetoothconnection.h"
#include "bluetoothlogger.h"

// Класс для приема файлов через Bluetooth и автовоспроизведения
class BluetoothReceiver : public QObject
{
    Q_OBJECT
    
public:
    explicit BluetoothReceiver(BluetoothLogger *logger, QObject *parent = nullptr);
    ~BluetoothReceiver();
    
    // Запуск прослушивания входящих данных
    void startListening(BluetoothConnection *connection);
    void stopListening();
    
    // Авто воспроизведение
    void setAutoPlay(bool enabled) { autoPlayEnabled = enabled; }
    bool isAutoPlayEnabled() const { return autoPlayEnabled; }
    
signals:
    void fileReceived(const QString &filePath);
    void receiveProgress(qint64 bytesReceived, qint64 totalBytes);
    void receiveFailed(const QString &error);
    void playbackStarted(const QString &fileName);
    
private slots:
    void onDataReceived(const QByteArray &data);
    void checkForIncomingData();
    
private:
    BluetoothLogger *logger;
    BluetoothConnection *currentConnection;
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    
    bool autoPlayEnabled;
    bool receiving;
    
    // Состояние приема
    QString receivedFileName;
    qint64 expectedFileSize;
    qint64 bytesReceived;
    QFile *receivedFile;
    
    // Автовоспроизведение
    void autoPlayFile(const QString &filePath);
    bool isAudioFile(const QString &filePath);
};

#endif // BLUETOOTHRECEIVER_H

