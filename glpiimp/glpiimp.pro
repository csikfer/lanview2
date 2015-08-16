#-------------------------------------------------
#
# Project created by QtCreator 2015-08-16T11:07:20
#
#-------------------------------------------------

QT       += core sql xml network

QT       -= gui

TARGET = glpiimp
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    glpiimp.cpp
INCLUDEPATH += ../lv2 ${QMAKE_FILE_OUT_PATH}/../lv2
LIBS += -lsnmp -L../lv2 -llv2

HEADERS += \
    glpiimp.h
