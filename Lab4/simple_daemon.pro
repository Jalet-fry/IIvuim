QT += core

CONFIG += c++11

TARGET = simple_daemon
TEMPLATE = app

# Скрываем консольное окно
CONFIG += console
win32:CONFIG -= console

# Исходные файлы
SOURCES += simple_daemon.cpp

# Выходная папка
DESTDIR = ../release
