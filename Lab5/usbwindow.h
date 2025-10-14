#ifndef USBWINDOW_H
#define USBWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QMovie>
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

protected:
    void resizeEvent(QResizeEvent *event) override;

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
    
    // Слот для скрытия анимации Jake
    void onAnimationHide();

private:
    Ui::USBWindow *ui;
    USBMonitor *monitor;
    
    // Jake анимационный виджет
    QLabel *jakeAnimationLabel;
    QMovie *jakeMovie;
    QTimer *animationTimer;
    
    // Вспомогательные методы
    void updateDevicesTable();
    void addLogMessage(const QString& message);
    void setupConnections();
    void setupJakeAnimation();
    void showJakeAnimation(const QString& gifPath, int duration = 3000);
    void hideJakeAnimation();
};

#endif // USBWINDOW_H

