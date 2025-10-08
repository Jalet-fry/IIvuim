#include "storagewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QFont>
#include <QTextStream>
#include <windows.h>
#include "hexioctrl.h"

using namespace std;

extern "C" {
    BYTE InPort(WORD port);
    WORD InPortW(WORD port);
    void OutPort(WORD port, BYTE value);
    void OutPortW(WORD port, WORD value);
}

#define IDENTIFY_PACKET_DEVICE 0xA1
#define IDENTIFY_DEVICE 0xEC

const WORD AS[2] = {0x3F6, 0x376};
const WORD DR[2] = {0x1F0, 0x170};
const WORD DH[2] = {0x1F6, 0x176};
const WORD SR[2] = {0x1F7, 0x177};
const WORD CR[2] = {0x1F7, 0x177};

WORD ataData[256];

void WaitBusy(int channel);
BOOL WaitReady(int channel);
BOOL GetATADevice(int channel, int device);

StorageWindow::StorageWindow(const QPixmap &background, QWidget *parent)
    : QWidget(parent), m_background(background)
{
    setWindowTitle("Storage Devices Information");
    showFullScreen();

    ALLOW_IO_OPERATIONS;

    setupUI();
    getATAStorageInfo();
}

void StorageWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    QPixmap scaledBg = m_background.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    painter.drawPixmap(0, 0, scaledBg);

    QWidget::paintEvent(event);
}

void StorageWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 50);

    QHBoxLayout *topLayout = new QHBoxLayout();

    topLayout->addStretch();

    QPushButton *backBtn = new QPushButton("Back");
    backBtn->setStyleSheet(
        "QPushButton { "
        "background: rgba(150,0,0,200); "
        "color: white; "
        "font-weight: bold; "
        "border: 1px solid #222; "
        "padding: 10px 20px; "
        "border-radius: 5px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover { "
        "background: rgba(150,0,0,255); "
        "color: #ffff66;"
        "}"
    );
    backBtn->setFixedSize(120, 50);
    connect(backBtn, SIGNAL(clicked()), this, SLOT(goBack()));

    topLayout->addWidget(backBtn);
    mainLayout->addLayout(topLayout);

    mainLayout->addStretch();

    QHBoxLayout *contentLayout = new QHBoxLayout();

    contentLayout->addStretch();

    QVBoxLayout *textLayout = new QVBoxLayout();

    QLabel *titleLabel = new QLabel("Storage Devices Information");
    titleLabel->setStyleSheet(
        "font-size: 24px; "
        "font-weight: bold; "
        "color: white; "
        "background: rgba(0,0,0,150); "
        "padding: 15px; "
        "border-radius: 8px 8px 0px 0px;"
        "margin-bottom: 0px;"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFixedHeight(70);

    textLayout->addWidget(titleLabel);

    textEdit = new QTextEdit();
    textEdit->setStyleSheet(
        "QTextEdit { "
        "background: rgba(255,255,255,230); "
        "font-size: 12px; "
        "border: 2px solid #a0a0a0; "
        "border-radius: 0px 0px 8px 8px;"
        "border-top: none;"
        "padding: 10px;"
        "}"
    );
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Courier New", 10));

    textEdit->setMaximumWidth(1000);
    textEdit->setMinimumWidth(800);
    textEdit->setMinimumHeight(600);
    textEdit->setMaximumHeight(700);

    textLayout->addWidget(textEdit);
    textLayout->setSpacing(0);
    textLayout->setContentsMargins(0, 0, 0, 0);

    contentLayout->addLayout(textLayout);

    contentLayout->addStretch();

    mainLayout->addLayout(contentLayout);

    mainLayout->addStretch();
}

void StorageWindow::goBack()
{
    close();
}

void StorageWindow::getATAStorageInfo()
{
    ALLOW_IO_OPERATIONS;

    int channel = 0;
    bool foundDevices = false;

    for (int device = 0; device < 2; ++device)
    {
        if (GetATADevice(channel, device))
        {
            QString info = getDeviceInfo(channel, device);
            printToConsole(info);
            foundDevices = true;
            break;
        }
    }

    if (!foundDevices)
    {
        printToConsole("No storage devices detected!");
    }
}

QString StorageWindow::getManufacturerFromATA()
{
    QString manufacturer;

    for (int i = 27; i <= 46; ++i) {
        manufacturer.append(QChar(ataData[i] >> 8));
        manufacturer.append(QChar(ataData[i] & 0x00FF));
    }
    manufacturer = manufacturer.trimmed();

    if (!manufacturer.isEmpty()) {
        QString vendor = manufacturer.section(' ', 0, 0).trimmed();
        if (!vendor.isEmpty()) {
            return vendor;
        }
    }

    return "Unknown";
}

QString StorageWindow::getDiskUsageInfo()
{
    QString usageInfo;
    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;

    if (GetDiskFreeSpaceExA("C:\\", &freeBytes, &totalBytes, &totalFreeBytes)) {
        double totalGB = totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double freeGB = freeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double usedGB = totalGB - freeGB;

        usageInfo = QString("  Total: %1 GB\n").arg(totalGB, 0, 'f', 2)
                   + QString("  Used: %1 GB\n").arg(usedGB, 0, 'f', 2)
                   + QString("  Free: %1 GB").arg(freeGB, 0, 'f', 2);
    } else {
        usageInfo = "  Information not available";
    }

    return usageInfo;
}

QString StorageWindow::getDriveTypeFromATA()
{
    WORD rotationRate = ataData[217];

    if (rotationRate == 0x0001 || rotationRate == 0xFFFF) {
        return "SSD";
    }

    return "HDD";
}

QString StorageWindow::getDeviceInfo(int channel, int device)
{
    QString result;
    QTextStream stream(&result);

    QString model;
    for (int i = 27; i <= 46; ++i) {
        model.append(QChar(ataData[i] >> 8));
        model.append(QChar(ataData[i] & 0x00FF));
    }
    stream << QString("%1:").arg("Model", -30) << model.trimmed();

    stream << "\n" << QString("%1:").arg("Manufacturer", -30) << getManufacturerFromATA();

    stream << "\n" << QString("%1:").arg("Drive Type", -30) << getDriveTypeFromATA();

    QString serial;
    for (int i = 10; i <= 19; ++i) {
        serial.append(QChar(ataData[i] >> 8));
        serial.append(QChar(ataData[i] & 0x00FF));
    }
    stream << "\n" << QString("%1:").arg("Serial Number", -30) << serial.trimmed();

    QString firmware;
    for (int i = 23; i <= 26; ++i) {
        firmware.append(QChar(ataData[i] >> 8));
        firmware.append(QChar(ataData[i] & 0x00FF));
    }
    stream << "\n" << QString("%1:").arg("Firmware Version", -30) << firmware.trimmed();

    stream << "\n\n";

    stream << "Memory Information:\n";
    stream << getDiskUsageInfo();

    stream << "\n\n";

    stream << QString("%1:").arg("Interface", -30);
    WORD interf = (ataData[168] & 0x000F);
    switch (interf) {
    case 0x0001: stream << "Parallel ATA (PATA)"; break;
    case 0x0002: stream << "Serial ATA (SATA)"; break;
    case 0x0003: stream << "Serial Attached SCSI (SAS)"; break;
    default: stream << "Unknown interface";
    }

    stream << "\n\nPIO Support:";
    stream << "\n  (" << ((ataData[64] & 0x1) ? "+" : "-") << ") PIO 3";
    stream << "\n  (" << ((ataData[64] & 0x2) ? "+" : "-") << ") PIO 4";

    stream << "\n\nMultiword DMA Support:";
    stream << "\n  (" << ((ataData[63] & 0x1) ? "+" : "-") << ") MWDMA 0";
    stream << "\n  (" << ((ataData[63] & 0x2) ? "+" : "-") << ") MWDMA 1";
    stream << "\n  (" << ((ataData[63] & 0x4) ? "+" : "-") << ") MWDMA 2";

    stream << "\n\nUltra DMA Support:";
    stream << "\n  (" << ((ataData[88] & 0x1) ? "+" : "-") << ") UDMA 0";
    stream << "\n  (" << ((ataData[88] & 0x2) ? "+" : "-") << ") UDMA 1";
    stream << "\n  (" << ((ataData[88] & 0x4) ? "+" : "-") << ") UDMA 2";
    stream << "\n  (" << ((ataData[88] & 0x8) ? "+" : "-") << ") UDMA 3";
    stream << "\n  (" << ((ataData[88] & 0x10) ? "+" : "-") << ") UDMA 4";
    stream << "\n  (" << ((ataData[88] & 0x20) ? "+" : "-") << ") UDMA 5";

    stream << "\n\nATA Version Support:";
    stream << "\n  (" << ((ataData[80] & 0x2) ? "+" : "-") << ") ATA 1";
    stream << "\n  (" << ((ataData[80] & 0x4) ? "+" : "-") << ") ATA 2";
    stream << "\n  (" << ((ataData[80] & 0x8) ? "+" : "-") << ") ATA 3";
    stream << "\n  (" << ((ataData[80] & 0x10) ? "+" : "-") << ") ATA 4";
    stream << "\n  (" << ((ataData[80] & 0x20) ? "+" : "-") << ") ATA 5";
    stream << "\n  (" << ((ataData[80] & 0x40) ? "+" : "-") << ") ATA 6";
    stream << "\n  (" << ((ataData[80] & 0x80) ? "+" : "-") << ") ATA 7";

    return result;
}

void StorageWindow::printToConsole(const QString &text)
{
    textEdit->append(text);
}

void WaitBusy(int channel)
{
    BYTE state;
    do
        state = InPort(AS[channel]);
    while (state & 0x80);
}

BOOL WaitReady(int channel)
{
    for (int i = 0; i < 1000; ++i)
        if (InPort(AS[channel]) & 0x40)
            return true;
    return false;
}

BOOL GetATADevice(int channel, int device)
{
    const BYTE commands[] = { IDENTIFY_DEVICE };

    for (int i = 0; i < sizeof(commands); ++i) {
        WaitBusy(channel);
        OutPort(DH[channel], (BYTE)((device << 4) | 0xE0));

        if (!WaitReady(channel))
            return false;

        OutPort(CR[channel], commands[i]);
        WaitBusy(channel);

        if (!(InPort(SR[channel]) & 0x08)) {
            if (i == 0)
                return false;
            continue;
        } else
            break;
    }

    for (int i = 0; i < 256; ++i)
        ataData[i] = InPortW(DR[channel]);

    return true;
}

BYTE InPort(WORD port)
{
    BYTE result;
    __asm {
            mov DX, port
            in AL, DX
            mov result, AL
    }
    return result;
}

WORD InPortW(WORD port)
{
    WORD result;
    __asm {
            mov DX, port
            in AX, DX
            mov result, AX
    }
    return result;
}

void OutPort(WORD port, BYTE value)
{
    __asm {
            mov DX, port
            mov AL, value
            out DX, AL
    }
}

void OutPortW(WORD port, WORD value)
{
    __asm {
            mov DX, port
            mov AX, value
            out DX, AX
    }
}
