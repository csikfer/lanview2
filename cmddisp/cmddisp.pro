
QT       += core sql xml network

QT       -= gui

TARGET = cmddisp
CONFIG += console
CONFIG -= app_bundle
CONFIG -= debug_and_release

TEMPLATE = app

SOURCES += \
    cmddisp.cpp

HEADERS += \
    cmddisp.h

INCLUDEPATH += ../lv2
LIBS += -L../lv2 -llv2

TRANSLATIONS = cmddisp_hu.ts \
               cmddisp_en.ts

CODECFORSRC     = UTF-8

