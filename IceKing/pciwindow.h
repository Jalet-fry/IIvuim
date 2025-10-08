#ifndef PCIWINDOW_H
#define PCIWINDOW_H

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVector>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include <QPainter>

extern "C" {
    #include "hexioctrl.h"
}

struct PCI_Device {
    int bus;
    int device;
    int function;
    unsigned long deviceID;
    unsigned long vendorID;
    unsigned long baseClass;
    unsigned long subClass;
    unsigned long progIf;
    unsigned long revisionID;
    unsigned long subsysID;
    unsigned long subsysVendID;
    QString vendorName;
    QString deviceName;
    QString className;
};

class PCIWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PCIWindow(const QPixmap &background, QWidget *parent = 0);

private slots:
    void scanPCI();
    void goBack();

private:
    QTableWidget *table;
    QPixmap m_background;

    void setupUI();
    unsigned long readPCIConfig(int bus, int device, int function, int reg);
    bool deviceExists(int bus, int device, int function);
    void device_info(int bus, int dev, int func);
    void parse_device_name(unsigned long DeviceId, unsigned long VendorId,
                          unsigned long BaseClass, unsigned long SubClass, unsigned long ProgIf,
                          QString &vendorName, QString &deviceName, QString &className);
    unsigned long calculate_address(int bus, int device, int function, int reg);
    QString format_device_path(int bus, int device, int function, unsigned long vendorID,
                              unsigned long deviceID, unsigned long subsysID, unsigned long subsysVendID,
                              unsigned long revisionID);

};

#endif // PCIWINDOW_H
