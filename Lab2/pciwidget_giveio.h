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
    
    // Дополнительные характеристики из Configuration Space
    quint8 classCode;      // Offset 0x0B - Class Code
    quint8 subClass;       // Offset 0x0A - SubClass
    quint8 progIF;         // Offset 0x09 - Programming Interface
    quint8 revisionID;     // Offset 0x08 - Revision ID
    quint8 headerType;     // Offset 0x0E - Header Type
    quint16 subsysVendorID; // Offset 0x2C - Subsystem Vendor ID
    quint16 subsysID;      // Offset 0x2E - Subsystem ID
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
    void onDeviceSelected();

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
    
    // Классификация устройств
    QString getClassString(quint8 classCode);
    QString getSubClassString(quint8 classCode, quint8 subClass);
    QString getProgIFString(quint8 classCode, quint8 subClass, quint8 progIF);
    
    // Чтение дополнительных регистров PCI
    DWORD readPCIConfigDword(quint8 bus, quint8 device, quint8 function, quint8 offset);

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

