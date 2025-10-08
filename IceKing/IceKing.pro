#-------------------------------------------------
#
# Project created by QtCreator 2025-10-04T00:59:05
#
#-------------------------------------------------

QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = IceKing
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp \
    pciwindow.cpp \
    storagewindow.cpp \
    mainWindow.cpp

HEADERS  += \
    (PCI_DEVS)pci_codes.h \
    hexioctrl.h \
    pciwindow.h \
    storagewindow.h \
    mainWindow.h

RESOURCES += \
    resources.qrc \
    power.qrc \
    lost.qrc \
    hugs.qrc \
    drums.qrc

win32 {
    LIBS += $$PWD/hexiosupp.lib
    LIBS += -luser32
    LIBS += -ladvapi32
}
