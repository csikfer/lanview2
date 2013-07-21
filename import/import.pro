#-------------------------------------------------
#
# Project created by QtCreator 2011-07-13T14:36:59
#
#-------------------------------------------------


QT       += core sql xml network

QT       -= gui

TARGET = import
CONFIG += console
CONFIG -= app_bundle
CONFIG += debug

TEMPLATE = app


SOURCES += import.cpp
HEADERS += import.h
INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

 TRANSLATIONS    = import_hu.ts \
                   import_en.ts

 CODECFORSRC     = UTF-8
