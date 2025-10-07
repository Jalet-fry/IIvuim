#ifndef PCI_CODES_H
#define PCI_CODES_H

#include <QString>

struct PciClassCodeEntry {
    unsigned int BaseClass;
    unsigned int SubClass;
    unsigned int ProgIf;
    const char* BaseDesc;
    const char* SubDesc;
};

extern const PciClassCodeEntry PciClassCodeTable[];
extern const int PciClassCodeTableLen;

#endif // PCI_CODES_H
