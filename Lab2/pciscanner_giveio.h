#ifndef PCISCANNER_GIVEIO_H
#define PCISCANNER_GIVEIO_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <windows.h>

struct PCI_Device_GiveIO {
    quint16 vendorID;
    quint16 deviceID;
    quint8 bus;
    quint8 device;
    quint8 function;
    QString vendorName;
    QString deviceName;

    quint8 classCode;      // Offset 0x0B - Class Code
    quint8 subClass;       // Offset 0x0A - SubClass
    quint8 progIF;         // Offset 0x09 - Programming Interface
    quint8 revisionID;     // Offset 0x08 - Revision ID
    quint8 headerType;     // Offset 0x0E - Header Type
    quint16 subsysVendorID; // Offset 0x2C - Subsystem Vendor ID
    quint16 subsysID;      // Offset 0x2E - Subsystem ID
};

// PciScannerGiveIO отвечает только за работу с PCI устройствами и низкоуровневый
// доступ через GiveIO. Не взаимодействует с UI напрямую - использует сигналы
// для передачи прогресса и найденных устройств.
class PciScannerGiveIO : public QObject
{
    Q_OBJECT
public:
    explicit PciScannerGiveIO(QObject *parent = nullptr);
    ~PciScannerGiveIO() override;

    // Запуск полного сканирования. Синхронный; испускает сигналы во время работы.
    bool scan();

    // Быстрый тест доступа к GiveIO и PCI конфигурационному пространству.
    bool testAccess();

    // Доступ к последним найденным устройствам
    QList<PCI_Device_GiveIO> devices() const;
    
    // Вспомогательные методы для получения информации об устройствах
    QString getVendorName(quint16 vendorID) const;
    QString getDeviceName(quint16 vendorID, quint16 deviceID) const;
    QString getClassString(quint8 classCode) const;
    QString getSubClassString(quint8 classCode, quint8 subClass) const;
    QString getProgIFString(quint8 classCode, quint8 subClass, quint8 progIF) const;
    
    // Проверка прав администратора
    bool isRunningAsAdmin() const;

signals:
    void logMessage(const QString &msg, bool isError = false);
    void deviceFound(const PCI_Device_GiveIO &device);
    void progress(int value, int maximum);
    void finished(bool anyFound);

private:
    // Низкоуровневые методы работы с GiveIO
    bool giveioInitialize();
    void giveioShutdown();
    void giveioOutPortDword(WORD port, DWORD value);
    DWORD giveioInPortDword(WORD port);

    bool writePortDword(WORD port, DWORD value);
    DWORD readPortDword(WORD port);
    DWORD readPCIConfigDword(quint8 bus, quint8 device, quint8 function, quint8 offset);

    // Внутренняя процедура сканирования
    bool scanInternal();

    HANDLE giveioHandle;
    bool giveioInitialized;

    QList<PCI_Device_GiveIO> m_devices;
};

#endif // PCISCANNER_GIVEIO_H

