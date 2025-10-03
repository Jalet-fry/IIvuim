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
    batterywidget.cpp \
    batteryworker.cpp \
    jakewidget.cpp

HEADERS += \
    mainwindow.h \
    batterywidget.h \
    batteryworker.h \
    jakewidget.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

win32: LIBS += -luser32 -lpowrprof -ladvapi32
