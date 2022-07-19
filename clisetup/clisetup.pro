QT -= gui
QT += core sql network

TARGET = clisetup
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++17
CONFIG -= debug_and_release


TEMPLATE = app

SOURCES +=  main.cpp

INCLUDEPATH += ../lv2
LIBS += -L../lv2 -llv2

TRANSLATIONS    = clisetup_hu.ts \
                  clisetup_en.ts

CODECFORSRC     = UTF-8

