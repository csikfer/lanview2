
QT       += core sql xml network

QT       -= gui

TARGET = portvlan
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app


SOURCES +=  portvlan.cpp
HEADERS +=  portvlan.h
INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

TRANSLATIONS    = portvlan_hu.ts \
                  portvlan_en.ts

CODECFORSRC     = UTF-8

