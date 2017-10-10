
#bison definition
bison.name = Bison
bison.input = BISONSOURCES
bison.output = ${QMAKE_FILE_BASE}.c
bison.commands = bison -d -o ${QMAKE_FILE_OUT} -v --report-file=bison_report.txt ${QMAKE_FILE_IN}
bison.clean =
bison.CONFIG += target_predeps
bison.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += bison

#m4 definition
m4.name = m4
m4.input = M4SOURCES
m4.output = ${QMAKE_FILE_BASE}.h
m4.commands = m4 <${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
m4.clean =
m4.CONFIG += target_predeps
m4.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += m4

CONFIG += avrcompile
CONFIG -= avrdude

INCLUDEPATH += /usr/lib/avr/include
INCLUDEPATH += /usr/lib/gcc/avr/4.3.5/include
INCLUDEPATH += /usr/lib/gcc/avr/4.3.5/include-fixed

MMCU  = atmega1280
F_CPU = 14745600L
include(avrcompile.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= qt

M4SOURCES = portbits.m4
OTHER_FILES += $$M4SOURCES
BISONSOURCES = console.y
OTHER_FILES += $$BISONSOURCES

SOURCES += \
    serial.c \
    misc.c \
    indalarmif2.c \
    eeconf.c \
    OWIIntFunctions.c \
    OWIcrc.c \
    diag.c \
    OWIStateMachine.c

HEADERS += \
    serial.h \
    misc.h \
    indalarmif2_gl.h \
    indalarmif2.h \
    eeconf.h \
    buffmac.h \
    ../IndAlarmIf1/indalarmif1_gl.h \
    OWIIntFunctions.h \
    OWIInterruptDriven.h \
    OWIDeviceSpecific.h \
    OWIdefs.h \
    OWIcrc.h \
    diag.h \
    OWIStateMachine.h

