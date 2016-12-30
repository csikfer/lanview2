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
HEADERS += lv2d.h \
           syscronthread.h
INCLUDEPATH += ../lv2
LIBS += -L../lv2 -llv2

unix {
    contains(QMAKE_HOST.arch, x86_64) {
        message("Unix, x86")
        SOURCES += syscronthread.cpp
        # simple-mail : https://github.com/cutelyst/simple-mail.git
        INCLUDEPATH += ../../simple-mail/src
        unix:LIBS += -L../../simple-mail-build/src -lsimplemail-qt5
    }
    else: message("ARM or x86_32")
}


 TRANSLATIONS    = lv2d_hu.ts \
                   lv2d_en.ts

 CODECFORSRC     = UTF-8
