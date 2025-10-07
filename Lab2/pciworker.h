#ifndef PCIWORKER_H
#define PCIWORKER_H

#include <QObject>
#include <QTimer>
#include <QList>
#include "pci_codes.h"
#include "portio.h"

// PCI константы
#define PCI_ADDRESS_PORT        0x0CF8
#define PCI_DATA_PORT           0x0CFC
#define PCI_MAX_BUSES           128
#define PCI_MAX_DEVICES         32
#define PCI_MAX_FUNCTIONS       8

#pragma pack(1)
typedef struct _PCI_CONFIG_ADDRESS {
    union {
        struct {
            unsigned int Zero:2;
            unsigned int RegisterNumber:6;
            unsigned int FunctionNumber:3;
            unsigned int DeviceNumber:5;
            unsigned int BusNumber:8;
            unsigned int Reserved:7;
            unsigned int Enable:1;
        } s1;
        unsigned int Value;
    } u1;
} PCI_CONFIG_ADDRESS;
#pragma pack()

struct PCIDevice {
    unsigned int DeviceID;
    unsigned int VendorID;
    unsigned int ClassCode;
    unsigned int SubClass;
    unsigned int ProgIF;
    unsigned int Revision;
    unsigned short Bus;
    unsigned short Device;
    unsigned short Function;
    QString DeviceName;
    QString VendorName;
};

class PCIWorker : public QObject
{
    Q_OBJECT

public:
    explicit PCIWorker(QObject *parent = nullptr);
    ~PCIWorker();

signals:
    void deviceFound(const PCIDevice &device);
    void scanCompleted(int deviceCount);
    void errorOccurred(const QString &error);

public slots:
    void startScan();
    void stopScan();

private slots:
    void performScan();

private:
    QTimer *timer;
    QList<PCIDevice> devices;
    bool isScanning;
    
    bool scanPCIBus();
    PCIDevice readPCIDevice(unsigned short bus, unsigned short device, unsigned short function);
    QString getDeviceDescription(unsigned int classCode, unsigned int subClass, unsigned int progIF);
    unsigned int readPCIRegister(unsigned short bus, unsigned short device, unsigned short function, unsigned char registerOffset);
};

#endif // PCIWORKER_H
