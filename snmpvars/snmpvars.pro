QT       += core sql xml network
QT       -= gui

TARGET = snmpvars
CONFIG += c++11 console
CONFIG -= app_bundle

TEMPLATE = app


SOURCES += \
    snmpvars.cpp
HEADERS += \
    snmpvars.h
INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

TRANSLATIONS    = snmpvars_hu.ts \
                  snmpvars_en.ts

CODECFORSRC     = UTF-8




# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
