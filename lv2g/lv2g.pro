#-------------------------------------------------
#
# Project created by QtCreator 2012-12-22T10:22:13
#
#-------------------------------------------------

QT       += network sql gui xml widgets

TARGET = lv2g
TEMPLATE = lib
win32:CONFIG += debug_and_release
unix:CONFIG += debug
#kell ahhoz, hogy a debug dll neveben ott legyen a d a vegen
CONFIG(debug, debug|release) {
     win32: TARGET = $$join(TARGET,,,d)
}
DEFINES += LV2G_LIBRARY

SOURCES += \
    lv2g.cpp \
    record_dialog.cpp \
    lv2widgets.cpp \
    lv2models.cpp \
    lv2validator.cpp \
    record_table.cpp \
    record_table_model.cpp \
    cerrormessagebox.cpp \
    menu.cpp \
    setup.cpp \
    gparse.cpp \
    logon.cpp \
    onlinealarm.cpp \
    record_tree_model.cpp \
    record_tree.cpp \
    imagedrv.cpp \
    mainwindow.cpp

HEADERS +=\
        lv2g_global.h \
    lv2g.h \
    record_table_model.h \
    record_dialog.h \
    lv2widgets.h \
    lv2models.h \
    lv2validator.h \
    record_table.h \
    cerrormessagebox.h \
    menu.h \
    setup.h \
    gparse.h \
    logon.h \
    LV2G_errcodes.h \
    onlinealarm.h \
    record_tree_model.h \
    record_tree.h \
    imagedrv.h \
    mainwindow.h

FORMS += \
    column_filter.ui \
    setup_logl.ui \
    setup.ui \
    gparse.ui \
    logindialog.ui \
    acknowledge.ui \
    polygoned.ui \
    arrayed.ui \
    fkeyed.ui
INCLUDEPATH += ../lv2
unix:LIBS += -lsnmp
LIBS += -L../lv2 -llv2


 TRANSLATIONS    = lv2glib_hu.ts \
                   lv2glib_en.ts

 CODECFORSRC     = UTF-8
