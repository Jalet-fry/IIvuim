#-------------------------------------------------
#
# Project created by QtCreator 2025-09-26T14:28:18
#
#-------------------------------------------------

QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SimpleLabsMenu
TEMPLATE = app

#SOURCES += main.cpp \
#    mainwindow.cpp

HEADERS += mainwindow.h
# Добавьте эти строки, если их нет
SOURCES += $$files(*.cpp, true)
HEADERS += $$files(*.h, true)
FORMS += $$files(*.ui, true)
RESOURCES += $$files(*.qrc, true)

# Для совместимости с Qt 5.5.1
CONFIG += c++11
