#-------------------------------------------------
#
# Project created by QtCreator 2012-12-22T10:22:13
#
#-------------------------------------------------

QT       += network sql gui xml widgets multimedia printsupport

TARGET = lv2g
TEMPLATE = lib
CONFIG -= debug_and_release
CONFIG += C++14
DEFINES += LV2G_LIBRARY

SOURCES += \
    glpisync.cpp \
    input_dialog.cpp \
    lv2g.cpp \
    record_dialog.cpp \
    lv2widgets.cpp \
    lv2models.cpp \
    lv2validator.cpp \
    record_table.cpp \
    record_table_model.cpp \
    cerrormessagebox.cpp \
    menu.cpp \
    reportwidget.cpp \
    setup.cpp \
    gparse.cpp \
    logon.cpp \
    onlinealarm.cpp \
    record_tree_model.cpp \
    record_tree.cpp \
    imagedrv.cpp \
    mainwindow.cpp \
    apierrcodes.cpp \
    record_link.cpp \
    record_link_model.cpp \
    gsetupwidget.cpp \
    hsoperate.cpp \
    phslinkform.cpp \
    findbymac.cpp \
    tool_table.cpp \
    workstation.cpp \
    object_dialog.cpp \
    deducepatch.cpp \
    tableexportdialog.cpp \
    linkgraph.cpp \
    snmpdevquery.cpp \
    popupreport.cpp \
    gexports.cpp \
    translator.cpp \
    changeswitch.cpp

HEADERS +=\
    glpisync.h \
    input_dialog.h \
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
    reportwidget.h \
    setup.h \
    gparse.h \
    logon.h \
    LV2G_errcodes.h \
    onlinealarm.h \
    record_tree_model.h \
    record_tree.h \
    imagedrv.h \
    mainwindow.h \
    apierrcodes.h \
    record_link.h \
    record_link_model.h \
    gsetupwidget.h \
    hsoperate.h \
    phslinkform.h \
    findbymac.h \
    tool_table.h \
    tool_table_model.h \
    workstation.h \
    object_dialog.h \
    deducepatch.h \
    tableexportdialog.h \
    linkgraph.h \
    snmpdevquery.h \
    popupreport.h \
    gexports.h \
    translator.h \
    changeswitch.h

FORMS += \
    column_filter.ui \
    glpisync.ui \
    reportwidget.ui \
    setup_logl.ui \
    setup.ui \
    gparse.ui \
    logindialog.ui \
    acknowledge.ui \
    polygoned.ui \
    arrayed.ui \
    fkeyed.ui \
    no_rights.ui \
    apierrcodes.ui \
    fkeyarrayed.ui \
    gsetup.ui \
    edit_field.ui \
    hsoperate.ui \
    phslinkform.ui \
    findbymac.ui \
    wstform.ui \
    dialogpatchsimple.ui \
    edit_enum_vals.ui \
    exports.ui \
    deducepatch.ui \
    tableexportdialog.ui \
    snmpdevquery.ui \
    edit_ip.ui \
    place_ed.ui \
    rplace_ed.ui \
    port_ed.ui \
    datetimedialog.ui \
    translator.ui \
    changeswitch.ui
INCLUDEPATH += ../lv2

LIBS += -L../lv2 -llv2

#exists(../../zodiacgraph) {
#    INCLUDEPATH += ../../zodiacgraph
#    LIBS += -L../../zodiacgraph.build -lzodiacgraph
#    DEFINES += ZODIACGRAPH
#}

unix:{
    exists(/usr/include/net-snmp) {
        DEFINES += SNMP_IS_EXISTS
    }
}
else {
    LIBS += -lSecur32 -lWtsapi32
}


TRANSLATIONS    = lv2glib_hu.ts \
                  lv2glib_en.ts

CODECFORSRC     = UTF-8

RESOURCES += \
    lv2g.qrc
