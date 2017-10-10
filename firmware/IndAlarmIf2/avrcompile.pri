 
avrcompile{

isEmpty(MMCU) : error(MMCU  is empty, example: MMCU  = atmega8)
isEmpty(F_CPU): error(F_CPU is empty, example: F_CPU = 8000000UL)

## add includes
#INCLUDEPATH += /usr/lib/avr/include
#INCLUDEPATH += /usr/lib/gcc/avr/4.3.5/include
#INCLUDEPATH += /usr/lib/gcc/avr/4.3.5/include-fixed

## Remove wrong C FLAGS
CONFIG	-= qt warn_on release incremental link_prl
#CONFIG   += console
#CONFIG   -= app_bundle
QT	-= core gui
QMAKE_CFLAGS   -= -m32
QMAKE_CXXFLAGS -= -m32
QMAKE_LFLAGS   -= -m32
INCLUDEPATH -= /usr/share/qt4/mkspecs/linux-g++-32
DEFINES     -= REENTRANT QT_WEBKIT QT_NO_DEBUG


## Cross Compiler for AVR
QMAKE_CC                = avr-gcc
QMAKE_CXX               = avr-g++
QMAKE_LINK              = avr-g++
QMAKE_LINK_SHLIB        = avr-g++
# modifications to linux.conf
QMAKE_AR                = avr-ar cqs
QMAKE_OBJCOPY           = avr-objcopy
QMAKE_STRIP             = avr-strip

message(avr-cc $$TARGET)

## remove unused code
QMAKE_CFLAGS   += -std=gnu99
QMAKE_CFLAGS   += -ffunction-sections -fdata-sections
QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
QMAKE_LFLAGS   += -Wl,-gc-sections

## compiler flags
DEFINES += F_CPU=$$F_CPU
CFLAGS_MINUS += -D_REENTRANT -DQT_WEBKIT -DQT_NO_DEBUG
CFLAGS_PLUS  += -mmcu=$$MMCU -DF_CPU=$$F_CPU \
                -g2 -gstabs -Os -fpack-struct \
                -fshort-enums -funsigned-char -funsigned-bitfields
#               -MMD -MP -MF"main.d" -MT"main.d"

QMAKE_CFLAGS   -= $$CFLAGS_MINUS
QMAKE_CXXFLAGS -= $$CFLAGS_MINUS
QMAKE_CFLAGS   += $$CFLAGS_PLUS
QMAKE_CXXFLAGS += $$CFLAGS_PLUS
QMAKE_LFLAGS -= -L/usr/lib/i386-linux-gnu -lpthread


avrLinker.target = $${TARGET}.elf
QMAKE_CLEAN += $${TARGET}.map
#avrLinker.commands = echo; echo Invoking: AVR Linker;
avrLinker.commands += avr-gcc ${LFLAGS} -Wl,-Map,$${TARGET}.map -mmcu=$$MMCU -o $${TARGET}.elf  ${OBJECTS};
avrLinker.depends = ${OBJECTS}
QMAKE_EXTRA_TARGETS += avrLinker


avrListing.target = $${TARGET}.lss
QMAKE_CLEAN += $${TARGET}.lss
#avrListing.commands  = echo; echo "Invoking: AVR Create Extended Listing";
avrListing.commands += avr-objdump -h -S $${TARGET}.elf  > $${TARGET}.lss;
avrListing.depends = $${TARGET}.elf
QMAKE_EXTRA_TARGETS += avrListing


avrFlashImage.target = $${TARGET}.hex
QMAKE_CLEAN += $${TARGET}.hex
#avrFlashImage.commands  = echo; echo "Create Flash image (ihex format)";
avrFlashImage.commands += avr-objcopy -R .eeprom -O ihex $${TARGET}.elf  $${TARGET}.hex;
avrFlashImage.depends = $${TARGET}.elf
QMAKE_EXTRA_TARGETS += avrFlashImage

avrEEPromImage.target = $${TARGET}.eep
QMAKE_CLEAN += $${TARGET}.eep
avrFlashImage.commands += avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $${TARGET}.elf  $${TARGET}.eep;
avrEEPromImage.depends = $${TARGET}.elf
QMAKE_EXTRA_TARGETS += avrEEPromImage


avrSize.target = avrsize
#avrSize.commands = echo; echo "Invoking: Print Size";
avrSize.commands += avr-size --format=avr --mcu=$$MMCU $${TARGET}.elf;
avrSize.depends = $${TARGET}.elf
QMAKE_EXTRA_TARGETS += avrSize


#avrdude {
#avrDude.target = avrdude
#avrDude.commands = avrdude -pm8 -cavrisp2 -Pusb -F -V -Uflash:w:$${TARGET}.hex:a;
##avrDude.commands+= touch avrdude.uploaded;
#avrDude.depends = $${TARGET}.hex
#QMAKE_EXTRA_TARGETS += avrDude
#}

#POST_TARGETDEPS += $${TARGET}.elf $${TARGET}.hex avrsize avrdude
POST_TARGETDEPS += $${TARGET}.elf $${TARGET}.hex $${TARGET}.eep


avrcompiler {

SOURCES_AVR += main.c
CFLAGS_AVR  += -mmcu=$$MMCU \
               -g2 -gstabs -Os -fpack-struct \
               -ffunction-sections -fdata-sections \
               -fshort-enums -funsigned-char -funsigned-bitfields \
               -MMD -MP -MF"main.d" -MT"main.d"

## AVR QMAKE Compiler
avrgcc.name  = avrgcc
avrgcc.input = SOURCES_AVR
avrgcc.dependency_type = TYPE_C
avrgcc.variable_out    = OBJECTS
avrgcc.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_IN_BASE}$${first(QMAKE_EXT_OBJ)}
avrgcc.commands = $${QMAKE_C} $(CFLAGS_AVR) -O0 $(INCPATH) \
                  -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} # Note the -O0
QMAKE_EXTRA_COMPILERS += avrgcc
message("QMAKE AVR COMPILER")
}
}
