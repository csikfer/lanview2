CONFIG -= debug_and_release
CONFIG += C++14

# A git-nek a path-on kell lennie Windows-nál is!!
REVISION = $$system(git rev-list --count HEAD)
DEFINES += REVISION=$$REVISION

#bison definition
bison.name = Bison
bison.input = BISONSOURCES
bison.output = ${QMAKE_FILE_BASE}_yy.cpp
# Ez itt gányolás, bocsi, de nem találtam olyan bisont, ami érti a win. path-nevet, és  a név konvertió sem müködik
#msvc {
#    bison.commands = bison -d -o ${QMAKE_FILE_OUT} -v --report-file=bison_report.txt ../../lanview2/lv2/import_parser.yy
#} else {
    bison.commands = bison -d -o ${QMAKE_FILE_OUT} -v --report-file=bison_report.txt ${QMAKE_FILE_IN}
#}
bison.clean = rm ${QMAKE_FILE_OUT}
bison.CONFIG += target_predeps
bison.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += bison
msvc:INCLUDEPATH += "."

#m4 definition
m4h.name = m4h
m4h.input = M4HEADER
m4h.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.h
# És jobb híján újra csak gányolunk
#msvc {
#    m4h.commands = m4 -I../../lanview2/lv2 <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
#} else {
    m4h.commands = m4 -I${QMAKE_FILE_IN_PATH} <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
#}
m4h.clean = rm ${QMAKE_FILE_OUT}
m4h.CONFIG += target_predeps
m4h.variable_out = HEADERS
m4h.depends  += ${QMAKE_FILE_IN_PATH}/lv2dict.m4
QMAKE_EXTRA_COMPILERS += m4h

m4c.name = m4c
m4c.input = M4SOURCE
m4c.output =${QMAKE_FILE_BASE}.cpp
# gányolás !!! ...
#msvc {
#    m4c.commands = m4 -I../../lanview2/lv2 <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
#} else {
    m4c.commands = m4 -I${QMAKE_FILE_IN_PATH} <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
#}
m4c.clean = rm ${QMAKE_FILE_OUT}
m4c.CONFIG += target_predeps
m4c.variable_out = SOURCES
m4c.depends  += ${QMAKE_FILE_IN_PATH}/lv2dict.m4
QMAKE_EXTRA_COMPILERS +=  m4c

BISONSOURCES += import_parser.yy
M4HEADER      = lv2dict.m4h
M4SOURCE      = lv2dict.m4c
DEPENDPATH   += $$TARGETPATH

OTHER_FILES += $$BISONSOURCES \
    $$M4HEADER $$M4SOURCE lv2dict.m4

QT += network \
    sql \
    xml
QT -= gui

TARGET = lv2
TEMPLATE = lib

DEFINES += LV2_LIBRARY
SOURCES += \
    lanview.cpp \
    cdebug.cpp \
    cerror.cpp \
    lv2tooldata.cpp \
    lv2types.cpp \
    others.cpp \
    lv2xml.cpp \
    lv2sql.cpp \
    lv2datab.cpp \
    lv2data.cpp \
    lv2user.cpp \
    srvdata.cpp \
    vardata.cpp \
    lv2service.cpp \
    qsnmp.cpp \
    guidata.cpp \
    import_parser.cpp \
    lv2daterr.cpp \
    lv2link.cpp \
    export.cpp \
    lv2html.cpp \
    report.cpp \
    lv2mariadb.cpp \
    lv2glpi.cpp \
    qtelnet.cpp

HEADERS += \
    lv2_global.h \
    lanview.h \
    cdebug.h \
    cerror.h \
    errcodes.h \
    LV2_errcodes.h \
    lv2tooldata.h \
    lv2types.h \
    others.h \
    lv2xml.h \
    lv2sql.h \
    lv2datab.h \
    lv2cont.h \
    lv2data.h \
    lv2user.h \
    srvdata.h \
    vardata.h \
    lv2service.h \
    qsnmp.h \
    guidata.h \
    import_parser.h \
    lv2daterr.h \
    lv2link.h \
    export.h \
    lv2html.h \
    report.h \
    lv2mariadb.h \
    lv2glpi.h \
    qtelnet.h \
    scan.h

#FORMS +=

unix:{
    SOURCES += usignal.cpp
    HEADERS += usignal.h
    exists(/usr/include/net-snmp) {
        SOURCES += scan.cpp
        LIBS += -lsnmp
        DEFINES += SNMP_IS_EXISTS
    }
}
else {
    LIBS += -lSecur32 -lWtsapi32
}

TRANSLATIONS    = lv2lib_hu.ts \
                  lv2lib_en.ts

CODECFORSRC     = UTF-8

