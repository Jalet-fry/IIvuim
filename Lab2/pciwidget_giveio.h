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
#include <QLabel>
#include <QMovie>
#include "pciscanner_giveio.h"

class PCIWidget_GiveIO : public QWidget
{
    Q_OBJECT

public:
    explicit PCIWidget_GiveIO(QWidget *parent = nullptr);
    ~PCIWidget_GiveIO();
    
    void showAndStart();

private slots:
    void onScanClicked();
    void onClearClicked();
    void onDeviceSelected();
    void onSaveToFileClicked();

    // Слоты для подключения к сигналам scanner
    void onScannerLog(const QString &msg, bool isError);
    void onScannerDeviceFound(const PCI_Device_GiveIO &dev);
    void onScannerProgress(int value, int maximum);
    void onScannerFinished(bool anyFound);

private:
    // UI компоненты
    QTableWidget *tableWidget;
    QPushButton *scanButton;
    QPushButton *clearButton;
    QPushButton *saveButton;
    QProgressBar *progressBar;
    QTextEdit *logTextEdit;
    QSplitter *mainSplitter;
    QLabel *jakeAnimationLabel;
    QMovie *jakeAnimation;
    
    // Для хранения всех сообщений лога
    QStringList allLogMessages;

    void initializeUI();
    void addDeviceToTable(const PCI_Device_GiveIO &pciDevice);
    void logMessage(const QString &message, bool isError = false);
    
    // Сканер PCI (отвечает за всю логику работы с PCI)
    PciScannerGiveIO *scanner;
    
    // Локальная копия устройств для UI взаимодействия
    QList<PCI_Device_GiveIO> pciDevices;
};

#endif // PCIWIDGET_GIVEIO_H

