# -------------------------------------------------
# Project created by QtCreator 2010-02-13T18:45:35
# -------------------------------------------------

QT += network \
    sql \
    xml
QT -= gui
TARGET = lv2
TEMPLATE = lib
win32:CONFIG += debug_and_release
unix:CONFIG += debug
#kell ahhoz, hogy a debug dll neveben ott legyen a d a vegen
CONFIG(debug, debug|release) {
     win32: TARGET = $$join(TARGET,,,d)
}
DEFINES += LV2_LIBRARY
SOURCES += lanview.cpp \
    strings.cpp \
    cdebug.cpp \
    cerror.cpp \
    usignal.cpp \
    lv2types.cpp \
    lv2sql.cpp \
    lv2datab.cpp \
    lv2data.cpp \
    lv2user.cpp \
    lv2xml.cpp \
    scan.cpp \
    ping.cpp \
    lv2service.cpp \
    others.cpp \
    qsnmp.cpp \
    guidata.cpp
HEADERS += lanview.h \
    lv2_global.h \
    strings.h \
    qsnmp.h \
    errcodes.h \
    cerror.h \
    cdebug.h \
    usignal.h \
    lv2types.h \
    lv2sql.h \
    lv2xml.h \
    LV2_errcodes.h \
    scan.h \
    ping.h \
    lv2service.h \
    others.h \
    lv2datab.h \
    lv2data.h \
    lv2user.h \
    guidata.h \
    lv2cont.h \
    doxydoc.h
FORMS += 
OTHER_FILES +=
unix:LIBS += -lsnmp

# CXXFLAGS += -Wconversion

 TRANSLATIONS    = lv2lib_hu.ts \
                   lv2lib_en.ts

 CODECFORSRC     = UTF-8
