#
# $Id: Makefile,v 1.1 2002/12/18 22:35:38 joerg_wunsch Exp $
#
CC= avr-gcc
MCU=atmega1280


CFLAGS=  -O -g -Wall -std=c99 -ffreestanding -mmcu=$(MCU) -Wa,-ahlms=$(<:.c=.lst)
LDFLAGS= -Wl,-Map=indalarmif2.map
OBJS=indalarmif2.o serial.o eeconf.o console.o misc.o diag.o OWIIntFunctions.o OWIcrc.o OWIStateMachine.o
#override CFLAGS += -D MOD_SER=$(MOD_SER)

.SUFFIXES: .s .bin .out .hex .srec .eep

all:	indalarmif2.bin indalarmif2.hex indalarmif2.eep

console.c:	console.y
	bison $< -o $@

.c.s:
	$(CC) $(CFLAGS) -S $<

.cpp.s:
	$(CC) $(CFLAGS) -S $<

.S.o:
	$(CC) $(ASFLAGS) -c $<

.o.out:
	$(CC) $(CFLAGS) -o $@ $<

.out.bin:
	avr-objcopy -O binary $< $@

.out.hex:
	avr-objcopy -O ihex -R .eeprom $< $@

.out.eep:
	avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $< $@

.out.srec:
	avr-objcopy -O srec $< $@


portbits.h:	portbits.m4
	m4 $< >$@

indalarmif2.o: 	indalarmif2.h serial.h misc.h indalarmif2_gl.h indalarmif2.h eeconf.h buffmac.h ../IndAlarmIf1/indalarmif1_gl.h portbits.h diag.h
console.o:	indalarmif2.h misc.h diag.h
serial.o:	serial.h indalarmif2.h
misch.o:	indalarmif2.h misc.h
eeconf.o:	diag.h eeconf.h indalarmif2.h
diag.o:		diag.h indalarmif2.h


indalarmif2.out: $(OBJS)
	$(CC) -o indalarmif2.out $(CFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f *~ *.out *.bin *.hex *.srec *.s *.o *.pdf *core *.eep console.c

