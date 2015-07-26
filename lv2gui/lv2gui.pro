#-------------------------------------------------
#
# Project created by QtCreator 2011-11-04T11:21:21
#
#-------------------------------------------------

QT       += core gui sql network xml widgets

TARGET = lv2gui
CONFIG -= app_bundle
CONFIG += debug

SOURCES += \
    lv2gui.cpp

HEADERS  += \
    lv2gui.h


INCLUDEPATH += ../lv2 ../lv2g
LIBS += -lsnmp -L../lv2 -L../lv2g -llv2 -llv2g

unix:LIBS += -lsnmp

TRANSLATIONS    = lv2gui_hu.ts \
                  lv2gui_en.ts

CODECFORSRC     = UTF-8


