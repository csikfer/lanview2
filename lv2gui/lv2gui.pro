#-------------------------------------------------
#
# Project created by QtCreator 2011-11-04T11:21:21
#
#-------------------------------------------------

QT       += core gui sql network widgets

TARGET = lv2gui
CONFIG -= app_bundle
CONFIG += c++17
CONFIG -= debug_and_release

SOURCES += \
    lv2gui.cpp

HEADERS  += \
    lv2gui.h


INCLUDEPATH += ../lv2 ../lv2g
LIBS += -L../lv2 -L../lv2g -llv2 -llv2g
#unix:LIBS += -lsnmp

TRANSLATIONS    = lv2gui_hu.ts \
                  lv2gui_en.ts

CODECFORSRC     = UTF-8
msvc:RC_ICONS = ../lv2g/icons/LanView2.ico


