#ifndef USBWINDOW_H
#define USBWINDOW_H

#include <QWidget>
#include <QTimer>
#include "usbmonitor.h"

namespace Ui {
class USBWindow;
}

class USBWindow : public QWidget
{
    Q_OBJECT

public:
    explicit USBWindow(QWidget *parent = nullptr);
    ~USBWindow();

private slots:
    // Слоты для обработки действий пользователя
    void on_refreshButton_clicked();
    void on_ejectButton_clicked();
    void on_clearLogButton_clicked();
    void on_backButton_clicked();
    void on_devicesTable_itemSelectionChanged();
    
    // Слоты для обработки событий от монитора
    void onDeviceConnected(const QString& deviceName, const QString& pid);
    void onDeviceDisconnected(const QString& deviceName, const QString& pid);
    void onDeviceEjected(const QString& deviceName, bool success);
    void onEjectFailed(const QString& deviceName);
    void onLogMessage(const QString& message);

private:
    Ui::USBWindow *ui;
    USBMonitor *monitor;
    
    // Вспомогательные методы
    void updateDevicesTable();
    void addLogMessage(const QString& message);
    void setupConnections();
};

#endif // USBWINDOW_H

