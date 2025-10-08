QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = untitled2
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS += mainwindow.h

FORMS += mainwindow.ui

# Добавьте эти строки для линковки с Windows API
LIBS += -lsetupapi
LIBS += -ladvapi32
LIBS += -lshell32

# Для компиляции в Windows XP
win32 {
    DEFINES += WINVER=0x0501
    DEFINES += _WIN32_WINNT=0x0501
}

DISTFILES += \
    app.manifest.txt \
    app.rc.txt
