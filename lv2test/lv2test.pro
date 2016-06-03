QT += core sql network xml
QT -= gui

TARGET = lv2test
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    lv2test.cpp

INCLUDEPATH += ../lv2 ${QMAKE_FILE_OUT_PATH}/../lv2
LIBS += -lsnmp -L../lv2 -llv2

HEADERS += \
    lv2test.h

