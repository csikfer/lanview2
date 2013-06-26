#-------------------------------------------------
#
# Project created by QtCreator 2011-07-13T14:36:59
#
#-------------------------------------------------

#bison definition
bison.name = Bison
bison.input = BISONSOURCES
bison.output = ${QMAKE_FILE_BASE}.cpp
bison.commands = bison -d -o ${QMAKE_FILE_OUT} -v --report-file=bison_report.txt ${QMAKE_FILE_IN}
bison.clean =
bison.CONFIG += target_predeps
bison.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += bison

BISONSOURCES += import_parser.yy

OTHER_FILES += $$BISONSOURCES

# nem mintha kéne nekünk widget, de elszáll a QPolygonF típusnál, ha ki akarjuk szedni egy QVariant-ból
QT       += core gui sql xml network widgets

#QT       -= gui

TARGET = import
CONFIG += console
CONFIG -= app_bundle
CONFIG += debug

TEMPLATE = app


SOURCES += import.cpp
HEADERS += import.h \
    import_parser_yacc.h

INCLUDEPATH += ../lv2
LIBS += -lsnmp -L../lv2 -llv2

 TRANSLATIONS    = import_hu.ts \
                   import_en.ts

 CODECFORSRC     = UTF-8
