CONFIG -= debug_and_release

REVISION = $$system(git rev-list --count HEAD)
DEFINES += REVISION=$$REVISION

#bison definition
bison.name = Bison
bison.input = BISONSOURCES
bison.output = ${QMAKE_FILE_BASE}_yy.cpp
# Ez itt gányolás, bocsi, de nem találtam olyan bisont, ami érti a win. path-nevet, és  a név konvertió sem müködik
msvc {
    bison.commands = bison -d -o ${QMAKE_FILE_OUT} -v --report-file=bison_report.txt ../../lanview2/lv2/import_parser.yy
} else {
    bison.commands = bison -d -o ${QMAKE_FILE_OUT} -v --report-file=bison_report.txt ${QMAKE_FILE_IN}
}
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
msvc {
    m4h.commands = m4 -I../../lanview2/lv2 <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
} else {
    m4h.commands = m4 -I${QMAKE_FILE_IN_PATH} <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
}
m4h.clean = rm ${QMAKE_FILE_OUT}
m4h.CONFIG += target_predeps
m4h.variable_out = HEADERS
m4h.depends  += ${QMAKE_FILE_IN_PATH}/strings.m4
QMAKE_EXTRA_COMPILERS += m4h

m4c.name = m4c
m4c.input = M4SOURCE
m4c.output =${QMAKE_FILE_BASE}.cpp
# gányolás !!! ...
msvc {
    m4c.commands = m4 -I../../lanview2/lv2 <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
} else {
    m4c.commands = m4 -I${QMAKE_FILE_IN_PATH} <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
}
m4c.clean = rm ${QMAKE_FILE_OUT}
m4c.CONFIG += target_predeps
m4c.variable_out = SOURCES
m4c.depends  += ${QMAKE_FILE_IN_PATH}/strings.m4
QMAKE_EXTRA_COMPILERS +=  m4c

BISONSOURCES += import_parser.yy
M4HEADER      = strings.m4h
M4SOURCE      = strings.m4c
DEPENDPATH   += $$TARGETPATH

OTHER_FILES += $$BISONSOURCES $$M4HEADER $$M4SOURCE

QT += network \
    sql \
    xml
QT -= gui

TARGET = lv2
TEMPLATE = lib

DEFINES += LV2_LIBRARY
SOURCES += lanview.cpp \
    cdebug.cpp \
    cerror.cpp \
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
    guidata.cpp \
    import_parser.cpp \
    lv2daterr.cpp \
    qtelnet.cpp \
    srvdata.cpp \
    lv2link.cpp \
    syscronthread.cpp

HEADERS += lanview.h \
    lv2_global.h \
    qsnmp.h \
    errcodes.h \
    cerror.h \
    cdebug.h \
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
    doxydoc.h \
    import_parser.h \
    strings.m4 \
    lv2daterr.h \
    qtelnet.h \
    srvdata.h \
    lv2link.h \
    syscronthread.h

unix:SOURCES += usignal.cpp
unix:HEADERS += usignal.h

FORMS += 

unix:LIBS += -lsnmp

TRANSLATIONS    = lv2lib_hu.ts \
                  lv2lib_en.ts

CODECFORSRC     = UTF-8

