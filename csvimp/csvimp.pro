QT += core sql xml network
QT -= gui

TARGET = csvimp
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

INCLUDEPATH += ../lv2 ${QMAKE_FILE_OUT_PATH}/../lv2
LIBS += -lsnmp -L../lv2 -llv2

HEADERS += \
    csvimp.h


