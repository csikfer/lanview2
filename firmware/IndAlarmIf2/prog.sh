#! /bin/sh

sudo avrdude -c avrispmkII -p m1280 -P usb: -e -U flash:w:indalarmif2.hex -U eeprom:w:indalarmif2.eep

