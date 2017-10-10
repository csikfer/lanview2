#
# $Id: Makefile,v 1.1 2002/12/18 22:35:38 joerg_wunsch Exp $
#
CC= avr-gcc
MCU=atmega644p


CFLAGS=  -O -g -Wall -std=c99 -ffreestanding -mmcu=$(MCU) -Wa,-ahlms=$(<:.c=.lst)
LDFLAGS= -Wl,-Map=indalarmif1.map
OBJS=indalarmif1.o serial.o twi.o eeconf.o console.o misc.o

#override CFLAGS += -D MOD_SER=$(MOD_SER)

.SUFFIXES: .s .bin .out .hex .srec .eep

.y.c:
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

all:	indalarmif1.bin indalarmif1.hex indalarmif1.eep

indalarmif1.out: $(OBJS)
	$(CC) -o indalarmif1.out $(CFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f *~ *.out *.bin *.hex *.srec *.s *.o *.pdf *core *.eep console.c

