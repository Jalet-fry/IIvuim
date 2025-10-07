#-------------------------------------------------
#
# Project created by QtCreator 2025-09-26T14:28:18
#
#-------------------------------------------------

QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SimpleLabsMenu
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Lab1/batterywidget.cpp \
    Lab1/batteryworker.cpp \
    Lab2/pciwidget.cpp \
    Lab2/pciworker.cpp \
    Lab2/portio.cpp \
    Lab2/hexiowrapper_real.cpp \
    Lab2/pci_codes.cpp \
    Animation/jakewidget_xp.cpp

HEADERS += \
    mainwindow.h \
    Lab1/batterywidget.h \
    Lab1/batteryworker.h \
    Lab2/pciwidget.h \
    Lab2/pciworker.h \
    Lab2/portio.h \
    Lab2/hexioctrl_original.h \
    Lab2/pci_codes.h \
    Animation/jakewidget_xp.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

win32: LIBS += -luser32 -lpowrprof -ladvapi32 -lsetupapi

# Пока не используем hexiosupp.lib - делаем простую реализацию
# win32: LIBS += -L$$PWD/Lab2 -lhexiosupp
