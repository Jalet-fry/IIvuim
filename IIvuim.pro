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
    Animation/jakewidget.cpp

HEADERS += \
    mainwindow.h \
    Lab1/batterywidget.h \
    Lab1/batteryworker.h \
    Lab3/storagescanner.h \
    Lab3/storagewindow.h \
    Animation/jakewidget.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

win32: LIBS += -luser32 -lpowrprof -ladvapi32 -lsetupapi -lole32 -loleaut32 -lwbemuuid
