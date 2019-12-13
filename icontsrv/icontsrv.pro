#-------------------------------------------------
#
# Project created by QtCreator 2011-09-07T09:51:31
#
#-------------------------------------------------

QT       += core sql xml network serialport
#QT       -= gui

TARGET = icontsrv
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += debug

TEMPLATE = app


SOURCES += \
    icontsrv.cpp

HEADERS += \
    icontsrv.h \
    ../firmware/IndAlarmIf2/indalarmif2_gl.h \
    ../firmware/IndAlarmIf1/indalarmif1_gl.h

INCLUDEPATH += ../../lanview2/lv2
LIBS += -L../lv2 -llv2

 TRANSLATIONS    = icontsrv_hu.ts \
                   icontsrv_en.ts

 CODECFORSRC     = UTF-8
