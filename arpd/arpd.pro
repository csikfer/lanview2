#-------------------------------------------------
#
# Project created by QtCreator 2013-03-23T09:50:13
#
#-------------------------------------------------


QT       += core sql xml network

QT       -= gui

TARGET = arpd
CONFIG += console
CONFIG -= app_bundle
CONFIG += debug

TEMPLATE = app


SOURCES += \
    arpd.cpp

INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

 TRANSLATIONS    = arpd_hu.ts \
                   arpd_en.ts

 CODECFORSRC     = UTF-8

HEADERS += \
    arpd.h
