#-------------------------------------------------
#
# Project created by QtCreator 2011-10-20T13:01:06
#
#-------------------------------------------------


QT       += core sql xml network

QT       -= gui

TARGET = portstat
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app


SOURCES +=  portstat.cpp
HEADERS +=  portstat.h
INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

TRANSLATIONS    = portstat_hu.ts \
                  portstat_en.ts

CODECFORSRC     = UTF-8

