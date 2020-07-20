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

# Get/Compile simple-mail :
# $git clone https://github.com/cutelyst/simple-mail.git
# $cd simple-mail
# $mkdir build
# $cd build
# $cmake ..
# $make
# $cd ../..
# $mkdir simple-mail-build
# $cd simple-mail-build
# $ln -s ../simple-mail/build/src src

unix {
    exists(/usr/include/net-snmp) {
        LIBS += -lsnmp
        DEFINES += SNMP_IS_EXISTS
    }
}

exists(../../simple-mail/src) {
    DEFINES += SYSCRON
    SOURCES += syscronthread.cpp
    INCLUDEPATH += ../../simple-mail/src
    LIBS += -L../../simple-mail/build/src -lSimpleMail2Qt5
}


 TRANSLATIONS    = lv2d_hu.ts \
                   lv2d_en.ts

 CODECFORSRC     = UTF-8
