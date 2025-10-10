#ifndef PCI_DATABASE_H
#define PCI_DATABASE_H

#include <QString>

// Forward declare the PCI structures and tables
typedef struct _PCI_VENTABLE
{
    unsigned short VenId;
    char *VenShort;
    char *VenFull;
} PCI_VENTABLE, *PPCI_VENTABLE;

typedef struct _PCI_DEVTABLE
{
    unsigned short VenId;
    unsigned short DevId;
    char *Chip;
    char *ChipDesc;
} PCI_DEVTABLE, *PPCI_DEVTABLE;

// External references to tables (defined in pci_database.cpp)
extern PCI_VENTABLE PciVenTable[];
extern PCI_DEVTABLE PciDevTable[];

// Table size functions (sizes are computed in pci_database.cpp)
extern size_t getPciVenTableSize();
extern size_t getPciDevTableSize();

class PCIDatabase
{
public:
    // Find vendor name by VendorID
    static QString findVendorName(unsigned short vendorId)
    {
        size_t tableSize = getPciVenTableSize();
        for (size_t i = 0; i < tableSize; i++) {
            if (PciVenTable[i].VenId == vendorId) {
                QString shortName = QString::fromLocal8Bit(PciVenTable[i].VenShort);
                QString fullName = QString::fromLocal8Bit(PciVenTable[i].VenFull);
                
                // Return short name if available, otherwise full name
                if (!shortName.isEmpty()) {
                    return shortName;
                } else if (!fullName.isEmpty()) {
                    return fullName;
                } else {
                    return QString("Vendor 0x%1").arg(vendorId, 4, 16, QChar('0')).toUpper();
                }
            }
        }
        return QString("Vendor 0x%1").arg(vendorId, 4, 16, QChar('0')).toUpper();
    }
    
    // Find device name by VendorID and DeviceID
    static QString findDeviceName(unsigned short vendorId, unsigned short deviceId)
    {
        // Get table length
        size_t tableLen = getPciDevTableSize();
        
        for (size_t i = 0; i < tableLen; i++) {
            if (PciDevTable[i].VenId == vendorId && PciDevTable[i].DevId == deviceId) {
                QString chip = QString::fromLocal8Bit(PciDevTable[i].Chip);
                QString desc = QString::fromLocal8Bit(PciDevTable[i].ChipDesc);
                
                // Build device name from available fields
                QString result;
                if (!chip.isEmpty() && !desc.isEmpty()) {
                    result = chip + " - " + desc;
                } else if (!chip.isEmpty()) {
                    result = chip;
                } else if (!desc.isEmpty()) {
                    result = desc;
                }
                
                if (!result.isEmpty()) {
                    return result;
                }
            }
        }
        
        // Not found in database, return hex format
        return QString("Device 0x%1").arg(deviceId, 4, 16, QChar('0')).toUpper();
    }
};

#endif // PCI_DATABASE_H

