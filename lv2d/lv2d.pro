#-------------------------------------------------
#
# Project created by QtCreator 2011-10-16T08:13:01
#
#-------------------------------------------------


QT       += core sql xml network

QT       -= gui

TARGET = lv2d
CONFIG   += console
CONFIG   -= app_bundle
#CONFIG   += debug
CONFIG -= debug_and_release

TEMPLATE = app


SOURCES += lv2d.cpp
HEADERS += lv2d.h
INCLUDEPATH += ../lv2
LIBS += -L../lv2 -llv2
unix:LIBS += -lsnmp

 TRANSLATIONS    = lv2d_hu.ts \
                   lv2d_en.ts

 CODECFORSRC     = UTF-8
