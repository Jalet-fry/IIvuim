#-------------------------------------------------
#
# Project created by QtCreator 2025-09-26T14:28:18
#
#-------------------------------------------------

QT += core gui widgets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SimpleLabsMenu
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Lab1/batterywidget.cpp \
    Lab1/batteryworker.cpp \
    Lab3/storagescanner.cpp \
    Lab3/storagewindow.cpp \
    Lab4/camerawindow.cpp \
    Lab4/cameraworker.cpp \
    Lab4/jakecamerawarning.cpp \
    Lab4/stealthdaemon.cpp \
    Lab4/automatic_stealth_mode.cpp \
    Lab4/lab4_logger.cpp \
    Lab5/usbdevice.cpp \
    Lab5/usbmonitor.cpp \
    Lab5/usbwindow.cpp \
    Lab6/windowsbluetoothmanager.cpp \
    Lab6/bluetoothwindow.cpp \
    Lab6/bluetoothlogger.cpp \
    Lab6/bluetoothfilesender.cpp \
    Lab6/bluetoothconnection.cpp \
    Lab6/bluetoothreceiver.cpp \
    Lab6/obexfilesender.cpp \
    Animation/jakewidget.cpp

HEADERS += \
    mainwindow.h \
    Lab1/batterywidget.h \
    Lab1/batteryworker.h \
    Lab3/storagescanner.h \
    Lab3/storagewindow.h \
    Lab4/camerawindow.h \
    Lab4/cameraworker.h \
    Lab4/jakecamerawarning.h \
    Lab4/stealthdaemon.h \
    Lab4/automatic_stealth_mode.h \
    Lab4/lab4_logger.h \
    Lab5/usbdevice.h \
    Lab5/usbmonitor.h \
    Lab5/usbwindow.h \
    Lab6/windowsbluetoothmanager.h \
    Lab6/bluetoothwindow.h \
    Lab6/bluetoothlogger.h \
    Lab6/bluetoothfilesender.h \
    Lab6/bluetoothconnection.h \
    Lab6/bluetoothreceiver.h \
    Lab6/obexfilesender.h \
    Animation/jakewidget.h

FORMS += \
    mainwindow.ui \
    Lab5/usbwindow.ui \
    Lab6/bluetoothwindow.ui

RESOURCES += \
    resources.qrc

# Lab4 использует DirectShow API (Windows нативный API)
# Низкоуровневый доступ к камере без высокоуровневых библиотек
# Qt Multimedia НЕ используется!

# Lab5 использует Windows API для мониторинга USB-устройств
# Библиотеки: setupapi.lib, Cfgmgr32.lib

# Lab6 использует Windows Bluetooth API (Native Windows API)
# Библиотеки: Bthprops.lib, ws2_32.lib

win32: LIBS += -luser32 -lpowrprof -ladvapi32 -lsetupapi -lole32 -loleaut32 -lwbemuuid -lstrmiids -lvfw32 -lCfgmgr32 -lBthprops -lws2_32
