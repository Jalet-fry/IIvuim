QT += core

CONFIG += c++11

TARGET = stealth_daemon
TEMPLATE = app

# Сборка как скрытое оконное приложение (без консоли)
win32:CONFIG += windows
CONFIG -= console

# Исходные файлы
SOURCES += \
    stealth_daemon_main.cpp \
    stealthdaemon.cpp \
    cameraworker.cpp \
    lab4_logger.cpp

HEADERS += \
    stealthdaemon.h \
    cameraworker.h \
    lab4_logger.h \
    qedit.h

# Windows библиотеки для DirectShow и keylogger
win32 {
    LIBS += -lole32 -loleaut32 -luuid -lstrmiids -lvfw32 -lsetupapi
    DEFINES += WIN32_LEAN_AND_MEAN
    DEFINES += NOMINMAX
}

# Компилятор
QMAKE_CXXFLAGS += -std=c++11

# Выходная папка
DESTDIR = ../release

# Иконка (опционально)
# RC_ICONS = daemon.ico
