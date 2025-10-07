#ifndef PCIWIDGET_H
#define PCIWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QCloseEvent>
#include "pciworker.h"

class PCIWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PCIWidget(QWidget *parent = nullptr);
    ~PCIWidget();

    void showAndStart();

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void onDeviceFound(const PCIDevice &device);
    void onScanCompleted(int deviceCount);
    void onErrorOccurred(const QString &error);

private slots:
    void onScanButtonClicked();
    void onClearButtonClicked();

private:
    void setupUI();
    void addDeviceToTable(const PCIDevice &device);
    QString formatHex(unsigned int value, int width = 4);

    QTableWidget *deviceTable;
    QLabel *statusLabel;
    QLabel *deviceCountLabel;
    QPushButton *scanButton;
    QPushButton *clearButton;
    QProgressBar *progressBar;
    
    PCIWorker *pciWorker;
    int deviceCount;
};

#endif // PCIWIDGET_H
