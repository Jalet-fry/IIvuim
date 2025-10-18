QT += core widgets

TARGET = test_stealth_mode
TEMPLATE = app

SOURCES += test_stealth_mode.cpp

# Настройки для Windows
win32 {
    CONFIG += console
    CONFIG -= app_bundle
}

# Настройки компилятора
QMAKE_CXXFLAGS += -std=c++11

# Отключаем предупреждения
QMAKE_CXXFLAGS += -Wno-unused-parameter
