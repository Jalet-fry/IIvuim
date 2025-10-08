#include "pciwindow.h"
#include <conio.h>
#include <QDebug>
#include <cstdlib>

extern "C" {
    #include "(PCI_DEVS)pci_codes.h"
}

PCIWindow::PCIWindow(const QPixmap &background, QWidget *parent)
    : QWidget(parent), m_background(background)
{
    setWindowTitle("PCI Configuration Space Scanner");
    showFullScreen();

    setWindowIcon(QIcon(":/img/ik-ic.ico"));

    ALLOW_IO_OPERATIONS;

    setupUI();
    scanPCI();
}

void PCIWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 50);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addStretch();

    QPushButton *backBtn = new QPushButton("Back to the menu");
    backBtn->setStyleSheet(
        "QPushButton { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #b3e5fc, stop:1 #81d4fa); "
        "color: #002b5c; "
        "font-weight: bold; "
        "border: 2px solid rgba(180, 230, 255, 0.8); "
        "padding: 10px 25px; "
        "border-radius: 15px;"
        "font-size: 16px;"
        "font-family: 'Comic Sans MS', 'Segoe UI';"
        "} "
        "QPushButton:hover { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #e0f7fa, stop:1 #b3e5fc); "
        "color: #004080; "
        "border: 2px solid #cceeff; "
        "} "
        "QPushButton:pressed { "
        "background: #81d4fa; "
        "color: white; "
        "border: 2px solid #ffffff;"
        "}"
    );
    backBtn->setFixedSize(220, 60);
    connect(backBtn, SIGNAL(clicked()), this, SLOT(goBack()));

    topLayout->addWidget(backBtn);
    mainLayout->addLayout(topLayout);

    // === Title ===
    QLabel *titleLabel = new QLabel("Ice King's PCI Scanner");
    titleLabel->setStyleSheet(
        "font-size: 30px; "
        "font-weight: bold; "
        "color: #e0f7ff; "
        "background: rgba(0, 30, 60, 0.5); "
        "border: 2px solid rgba(200, 230, 255, 0.5); "
        "border-radius: 15px; "
        "padding: 20px; "
        "font-family: 'Comic Sans MS', 'Segoe UI'; "
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFixedHeight(90);
    mainLayout->addWidget(titleLabel);

    // === Table ===
    QHBoxLayout *tableLayout = new QHBoxLayout();
    tableLayout->addStretch();

    table = new QTableWidget();
    table->setColumnCount(6);  // 6 columns now

    QStringList headers;
    headers << "Vendor ID" << "Device ID" << "Vendor Name" << "Device Name" << "Class" << "Device Path";
    table->setHorizontalHeaderLabels(headers);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnWidth(0, 120);
    table->setColumnWidth(1, 120);
    table->setColumnWidth(2, 200);
    table->setColumnWidth(3, 250);
    table->setColumnWidth(4, 250);

    table->setStyleSheet(
        "QTableWidget { "
        "background: rgba(240, 250, 255, 0.8); "
        "alternate-background-color: rgba(210, 235, 255, 0.8); "
        "font-size: 13px; "
        "font-family: 'Segoe UI', 'Comic Sans MS';"
        "color: #003366; "
        "border: 3px solid rgba(180, 230, 255, 0.8); "
        "border-radius: 15px;"
        "}"
        "QHeaderView::section { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #b3e5fc, stop:1 #81d4fa); "
        "padding: 12px; "
        "border: 1px solid #a0d0f0; "
        "font-weight: bold; "
        "font-size: 14px; "
        "color: #003366;"
        "}"
        "QTableWidget::item:selected { "
        "background: rgba(120, 200, 255, 0.7); "
        "color: black; "
        "}"
        "QScrollBar:vertical { "
        "background: rgba(230, 245, 255, 0.6); "
        "width: 14px; "
        "border-radius: 7px;"
        "}"
        "QScrollBar::handle:vertical { "
        "background: rgba(160, 220, 255, 0.8); "
        "border-radius: 7px;"
        "}"
    );

    table->setMaximumWidth(1920);
    table->setMinimumWidth(1900);
    table->setMinimumHeight(1200);
    table->setMaximumHeight(1280);

    tableLayout->addWidget(table);
    tableLayout->addStretch();

    mainLayout->addLayout(tableLayout);
    mainLayout->addStretch();

    // === Background Frost ===
    this->setStyleSheet(
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #002b5c, stop:0.5 #003f7f, stop:1 #005599); "
        "border: none;"
        "}"
    );
}

void PCIWindow::goBack()
{
    close();
}

unsigned long PCIWindow::readPCIConfig(int bus, int device, int function, int reg)
{
    unsigned long configAddress = calculate_address(bus, device, function, reg);
    unsigned long regData;

    __asm
    {
        mov eax, configAddress
        mov dx, 0CF8h
        out dx, eax
        mov dx, 0CFCh
        in eax, dx
        mov regData, eax
    }

    return regData;
}

unsigned long PCIWindow::calculate_address(int bus, int device, int function, int reg)
{
    return (1u << 31) | (bus << 16) | (device << 11) | (function << 8) | (reg & 0x3F);
}

bool PCIWindow::deviceExists(int bus, int device, int function)
{
    unsigned long vendorDevice = readPCIConfig(bus, device, function, 0x00);
    return (vendorDevice != 0xFFFFFFFF) && ((vendorDevice & 0xFFFF) != 0xFFFF);
}

QString PCIWindow::format_device_path(int bus, int device, int function, unsigned long vendorID,
                                     unsigned long deviceID, unsigned long subsysID, unsigned long subsysVendID,
                                     unsigned long revisionID)
{
    return QString("PCI\\VEN_%1&DEV_%2&SUBSYS_%3%4&REV_%5\\%6&%7&%8&%9")
        .arg(vendorID, 4, 16, QChar('0')).toUpper()
        .arg(deviceID, 4, 16, QChar('0')).toUpper()
        .arg(subsysVendID, 4, 16, QChar('0')).toUpper()
        .arg(subsysID, 4, 16, QChar('0')).toUpper()
        .arg(revisionID, 2, 16, QChar('0')).toUpper()
        .arg(bus, 2, 16, QChar('0')).toUpper()
        .arg(device, 2, 16, QChar('0')).toUpper()
        .arg(function, 1, 16, QChar('0')).toUpper()
        .arg(0, 2, 16, QChar('0')).toUpper();
}

void PCIWindow::parse_device_name(unsigned long DeviceId, unsigned long VendorId,
                                 unsigned long BaseClass, unsigned long SubClass, unsigned long ProgIf,
                                 QString &vendorName, QString &deviceName, QString &className)
{
    vendorName = "Unknown Vendor";
    deviceName = "Unknown Device";
    className = "Unknown Class";

    for(int i = 0; i < PCI_CLASSCODETABLE_LEN; i++)
    {
        if(PciClassCodeTable[i].BaseClass == BaseClass &&
           PciClassCodeTable[i].SubClass == SubClass &&
           PciClassCodeTable[i].ProgIf == ProgIf)
        {
            className = QString("%1 (%2 %3)")
                .arg(PciClassCodeTable[i].BaseDesc)
                .arg(PciClassCodeTable[i].SubDesc)
                .arg(PciClassCodeTable[i].ProgDesc);
            break;
        }
    }

    for(int i = 0; i < PCI_DEVTABLE_LEN; i++)
    {
        if(PciDevTable[i].VenId == VendorId && PciDevTable[i].DevId == DeviceId)
        {
            deviceName = QString("%1, %2")
                .arg(PciDevTable[i].Chip)
                .arg(PciDevTable[i].ChipDesc);
            break;
        }
    }

    for(int i = 0; i < PCI_VENTABLE_LEN; i++)
    {
        if(PciVenTable[i].VenId == VendorId)
        {
            vendorName = QString(PciVenTable[i].VenFull);
            break;
        }
    }
}

void PCIWindow::device_info(int bus, int dev, int func)
{
    unsigned long RegData;
    unsigned short VendorID, DeviceID;
    unsigned char  BaseClassCode, SubClassCode, ProgInterface, RevisionID;
    unsigned short SubsysVendID, SubsysID;

    // --- [0x00] Vendor and Device IDs ---
    RegData = readPCIConfig(bus, dev, func, 0x00);

    if (RegData == 0xFFFFFFFF)
        return;

    VendorID = (unsigned short)(RegData & 0xFFFF);
    DeviceID = (unsigned short)((RegData >> 16) & 0xFFFF);

    // --- [0x08] Class codes and Revision ---
    RegData = readPCIConfig(bus, dev, func, 0x08);
    RevisionID    = (unsigned char)(RegData & 0xFF);
    ProgInterface = (unsigned char)((RegData >> 8) & 0xFF);
    SubClassCode  = (unsigned char)((RegData >> 16) & 0xFF);
    BaseClassCode = (unsigned char)((RegData >> 24) & 0xFF);

    // --- [0x2C] Subsystem Vendor and Device IDs ---
    RegData = readPCIConfig(bus, dev, func, 0x2C);
    SubsysVendID = (unsigned short)(RegData & 0xFFFF);
    SubsysID     = (unsigned short)((RegData >> 16) & 0xFFFF);

    QString devicePath = format_device_path(bus, dev, func, VendorID, DeviceID,
                                           SubsysID, SubsysVendID, RevisionID);

    QString vendorName, deviceName, className;
    parse_device_name(DeviceID, VendorID, BaseClassCode, SubClassCode, ProgInterface,
                     vendorName, deviceName, className);

    int row = table->rowCount();
    table->insertRow(row);

    QTableWidgetItem *vendorItem = new QTableWidgetItem(QString("0x%1").arg(VendorID, 4, 16, QChar('0')).toUpper());
    vendorItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 0, vendorItem);

    QTableWidgetItem *deviceItem = new QTableWidgetItem(QString("0x%1").arg(DeviceID, 4, 16, QChar('0')).toUpper());
    deviceItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, deviceItem);

    QTableWidgetItem *vendorNameItem = new QTableWidgetItem(vendorName);
    vendorNameItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 2, vendorNameItem);

    // Column 3: Device Name
    QTableWidgetItem *deviceNameItem = new QTableWidgetItem(deviceName);
    deviceNameItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 3, deviceNameItem);

    // Column 4: Class Name
    QTableWidgetItem *classItem = new QTableWidgetItem(className);
    classItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 4, classItem);

    // Column 5: Device Path
    QTableWidgetItem *pathItem = new QTableWidgetItem(devicePath);
    pathItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->setItem(row, 5, pathItem);
}

void PCIWindow::scanPCI()
{
    table->setRowCount(0);

    for(int bus = 0; bus < 256; bus++)
        for(int dev = 0; dev < 32; dev++)
            for(int func = 0; func < 8; func++)
                device_info(bus, dev, func);

    table->resizeColumnsToContents();

    table->horizontalHeader()->setStretchLastSection(true);

    table->setColumnWidth(0, 150);
    table->setColumnWidth(1, 150);
}
