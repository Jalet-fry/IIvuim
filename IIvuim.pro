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
    Lab3/storagescanner.cpp \
    Lab3/storagewindow.cpp \
    Lab4/camerawindow.cpp \
    Lab4/cameraworker.cpp \
    Animation/jakewidget.cpp

HEADERS += \
    mainwindow.h \
    Lab1/batterywidget.h \
    Lab1/batteryworker.h \
    Lab3/storagescanner.h \
    Lab3/storagewindow.h \
    Lab4/camerawindow.h \
    Lab4/cameraworker.h \
    Animation/jakewidget.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Lab4 использует DirectShow API (Windows нативный API)
# Низкоуровневый доступ к камере без высокоуровневых библиотек
# Qt Multimedia НЕ используется!

win32: LIBS += -luser32 -lpowrprof -ladvapi32 -lsetupapi -lole32 -loleaut32 -lwbemuuid -lstrmiids -lvfw32
