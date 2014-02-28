#-------------------------------------------------
#
# Project created by QtCreator 2014-02-26T12:22:21
#
#-------------------------------------------------

QT       += core sql xml network

QT       -= gui

TARGET = portmac
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    portmac.cpp

HEADERS += \
    portmac.h
INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

TRANSLATIONS    = portmac_hu.ts \
                  portmac_en.ts

CODECFORSRC     = UTF-8

