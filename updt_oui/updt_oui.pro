#-------------------------------------------------
#
# Project created by QtCreator 2014-06-23T08:44:10
#
#-------------------------------------------------

# Buils debug:  ../../lanview2.debug/updt_oui
# Build release:../../lanview2.release/updt_oui

QT       += core sql network

QT       -= gui

TARGET = updt_oui
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += c++17
CONFIG -= debug_and_release


TEMPLATE = app


SOURCES += \
    updt_oui.cpp
HEADERS += \
    updt_oui.h
INCLUDEPATH += ../lv2
LIBS += -L../lv2 -llv2

TRANSLATIONS    = updt_oui_hu.ts \
                  updt_oui_en.ts

CODECFORSRC     = UTF-8

