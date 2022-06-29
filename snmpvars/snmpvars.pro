
QT       -= gui
QT       += core network sql xml

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app


SOURCES += \
    snmpvars.cpp
HEADERS += \
    snmpvars.h
INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

TRANSLATIONS    = snmpvars_hu.ts \
                  snmpvars_en.ts

CODECFORSRC     = UTF-8

