#! /bin/sh

avrdude -c stk200 -p m644p -e -U flash:w:indalarmif1.hex -U eeprom:w:master1.eep