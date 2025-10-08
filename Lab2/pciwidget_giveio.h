#ifndef PCIWIDGET_GIVEIO_H
#define PCIWIDGET_GIVEIO_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QSplitter>
#include <QMap>
#include <QLabel>
#include <QMovie>
#include <windows.h>

struct PCI_Device {
    quint16 vendorID;
    quint16 deviceID;
    quint8 bus;
    quint8 device;
    quint8 function;
    QString vendorName;
    QString deviceName;
};

class PCIWidget_GiveIO : public QWidget
{
    Q_OBJECT

public:
    explicit PCIWidget_GiveIO(QWidget *parent = nullptr);
    ~PCIWidget_GiveIO();
    
    void showAndStart();

private slots:
    void scanPCI_devices();
    void clearLog();

private:
    QTableWidget *tableWidget;
    QPushButton *scanButton;
    QPushButton *clearButton;
    QProgressBar *progressBar;
    QTextEdit *logTextEdit;
    QSplitter *mainSplitter;
    QLabel *jakeAnimationLabel;
    QMovie *jakeAnimation;

    void initializeUI();
    void scanPCI();
    bool testPCIAccess();
    void addDeviceToTable(const PCI_Device &pciDevice);
    void logMessage(const QString &message, bool isError = false);
    QList<PCI_Device> pciDevices;

    // GiveIO методы
    bool initializeGiveIO();
    void shutdownGiveIO();
    bool scanPCI_GiveIO();
    bool writePortDword(WORD port, DWORD value);
    DWORD readPortDword(WORD port);

    // Вспомогательные методы
    QString getVendorName(quint16 vendorID);
    QString getDeviceName(quint16 vendorID, quint16 deviceID);

    // GiveIO реализация
    bool giveioInitialize();
    void giveioShutdown();
    void giveioOutPortDword(WORD port, DWORD value);
    DWORD giveioInPortDword(WORD port);

    HANDLE giveioHandle;
    bool giveioInitialized;
    
    // Проверка прав администратора
    bool isRunningAsAdmin();
};

#endif // PCIWIDGET_GIVEIO_H

