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

#Cross compiler not supported !!!
message($$QMAKE_HOST.arch)
unix {
    contains(QMAKE_HOST.arch,x86_\d+):{
    message("Unix, x86_??")
        LIBS += -lsnmp
        contains(QMAKE_HOST.arch,x86_64):{
            message("Unix, x86_64")
            SOURCES += syscronthread.cpp
            INCLUDEPATH += ../../simple-mail/src
            LIBS += -L../../simple-mail-build/src -lsimplemail-qt5
        }
        else:message("Unix x86_32")
    }
    else: message("Unix ARM (?)")
}
else: message("Windows (?)")


 TRANSLATIONS    = lv2d_hu.ts \
                   lv2d_en.ts

 CODECFORSRC     = UTF-8
