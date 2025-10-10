// Implementation file for PCI Database
// This ensures the PCI tables are defined only once

#include "(PCI_DEVS)pci_codes.h"
#include <cstddef>

// Functions to return table sizes
size_t getPciVenTableSize()
{
    return sizeof(PciVenTable) / sizeof(PCI_VENTABLE);
}

size_t getPciDevTableSize()
{
    return sizeof(PciDevTable) / sizeof(PCI_DEVTABLE);
}

