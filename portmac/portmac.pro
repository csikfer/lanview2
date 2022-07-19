#-------------------------------------------------
#
# Project created by QtCreator 2014-02-26T12:22:21
#
#-------------------------------------------------

# Buils debug:  ../../lanview2.debug/portmac
# Build release:../../lanview2.release/portmac

QT       += core sql network

QT       -= gui

TARGET = portmac
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += c++17
CONFIG -= debug_and_release

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

