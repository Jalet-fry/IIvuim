#ifndef BLUETOOTHWINDOW_H
#define BLUETOOTHWINDOW_H

#include <QWidget>
#include "windowsbluetoothmanager.h"
#include "bluetoothlogger.h"
#include "bluetoothfilesender.h"
#include "bluetoothconnection.h"
#include "bluetoothreceiver.h"
#include "obexfilesender.h"
#include "bluetoothserver.h"

namespace Ui {
class BluetoothWindow;
}

class BluetoothWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BluetoothWindow(QWidget *parent = nullptr);
    ~BluetoothWindow();

private slots:
    // Слоты для кнопок
    void onScanButtonClicked();
    void onClearLogButtonClicked();
    void onRefreshAdapterButtonClicked();
    void onConnectButtonClicked();
    void onDisconnectButtonClicked();
    void onSendFileButtonClicked();
    void onDeviceSelectionChanged();
    
    // Слоты для сервера
    void onStartServerButtonClicked();
    void onStopServerButtonClicked();

    // Слоты для обработки событий Bluetooth
    void onDeviceDiscovered(const BluetoothDeviceData &device);
    void onDiscoveryFinished(int deviceCount);
    void onDiscoveryError(const QString &errorString);

    void onLogMessage(const QString &message);

private:
    Ui::BluetoothWindow *ui;
    WindowsBluetoothManager *bluetoothManager;
    BluetoothLogger *logger;
    BluetoothFileSender *fileSender;
    BluetoothConnection *btConnection;
    BluetoothReceiver *btReceiver;
    ObexFileSender *obexSender;  // Для прямой OBEX отправки на Android
    BluetoothServer *btServer;   // Сервер для приема файлов ПК-ПК

    void setupUI();
    void addLogMessage(const QString &message, const QString &color = "black");
    QString getCurrentTimestamp() const;
    void updateButtonStates();

    // Хранение информации об устройствах
    QList<BluetoothDeviceData> discoveredDevices;
    int selectedDeviceIndex;
    
    // Состояние подключения
    bool isDeviceConnected;
};

#endif // BLUETOOTHWINDOW_H

