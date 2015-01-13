# -------------------------------------------------
# Project created by QtCreator 2010-02-13T18:45:35
# -------------------------------------------------

#bison definition
bison.name = Bison
bison.input = BISONSOURCES
bison.output = ${QMAKE_FILE_BASE}_yy.cpp
bison.commands = bison -d -o ${QMAKE_FILE_OUT} -v --report-file=bison_report.txt ${QMAKE_FILE_IN}
bison.clean =
bison.CONFIG += target_predeps
bison.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += bison
msvc:INCLUDEPATH += "."

# Az M4-es makrók fordítása nem működik rendesen, kézzel kell fordítani!!!:
#< m4 strings.m4c >../../lanview2.debug/lv2/strings.cpp
#< m4 strings.m4h >strings.h

#m4 definition
m4h.name = m4h
m4h.input = M4HEADERS
m4h.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.h
m4h.commands = m4 -I${QMAKE_FILE_IN_PATH} ${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
m4h.clean = rm ${QMAKE_FILE_OUT}
m4h.CONFIG += target_predeps
m4h.variable_out = HEADERS
QMAKE_EXTRA_COMPILERS += m4h

m4c.name = m4c
m4c.input = M4SOURCES
m4c.output =${QMAKE_FILE_BASE}.cpp
m4c.commands = m4 -I${QMAKE_FILE_IN_PATH} ${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
m4c.clean = rm ${QMAKE_FILE_OUT}
m4c.CONFIG += target_predeps
m4c.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS +=  m4c

BISONSOURCES += import_parser.yy
M4HEADERS    += strings.m4h
M4SOURCES    += strings.m4c
DEPENDPATH   += $$TARGETPATH

OTHER_FILES += $$BISONSOURCES $$M4HEADERS $$M4SOURCES

QT += network \
    sql \
    xml
QT -= gui

TARGET = lv2
TEMPLATE = lib

msvc:CONFIG += debug_and_release
unix:CONFIG += debug

#kell ahhoz, hogy a debug dll neveben ott legyen a d a vegen
CONFIG(debug, debug|release) {
     msvc: TARGET = $$join(TARGET,,,d)
}
DEFINES += LV2_LIBRARY
SOURCES += lanview.cpp \
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
    guidata.cpp \
    import_parser.cpp \
    lv2daterr.cpp \
    qtelnet.cpp
HEADERS += lanview.h \
    lv2_global.h \
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
    doxydoc.h \
    import_parser.h \
    strings.m4 \
    lv2daterr.h \
    qtelnet.h

FORMS += 

unix:LIBS += -lsnmp

 TRANSLATIONS    = lv2lib_hu.ts \
                   lv2lib_en.ts

 CODECFORSRC     = UTF-8

# message( $$CONFIG )
# msvc: message( "msvc on" )
# linux: message( "linux on" )
# win32: message( "Win32 on" )
# message( $$INCLUDEPATH )
