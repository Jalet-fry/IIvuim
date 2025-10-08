#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QStatusBar>
#include <QProgressBar>
#include <QTextEdit>
#include <QSplitter>
#include <QMap>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct PCI_Device {
    quint16 vendorID;
    quint16 deviceID;
    quint8 bus;
    quint8 device;
    quint8 function;
    QString vendorName;
    QString deviceName;
};

class MainWindow : public QMainWindow
{
    //Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

//private slots:
private:
    void scanPCI_devices();
    void clearLog();

private:
    Ui::MainWindow *ui;
    QTableWidget *tableWidget;
    QPushButton *scanButton;
    QPushButton *clearButton;
    QWidget *centralWidget;
    QProgressBar *progressBar;
    QTextEdit *logTextEdit;
    QSplitter *mainSplitter;

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
};

#endif // MAINWINDOW_H
