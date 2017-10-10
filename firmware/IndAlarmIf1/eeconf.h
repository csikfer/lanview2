#ifndef EECONF_H
#define EECONF_H

#include <avr/eeprom.h>

struct eep_config {
    enum {
        UNKNOWN = 0,        ///< Az eszköz nincs konfigurálva
        MASTER,             ///< Master eszköz
        SLAVE               ///< Slave eszköz
    }   type;           ///< Az eszköz típusa
    uint8_t     id;     ///< IIC slave ill. RS485 id, ill. cím (0 < id < 64)
    struct sLight {
        unsigned  k1:2;   ///< Kontakt érzékelő 1. LED (piros) fényereje
        unsigned  k2:2;   ///< Kontakt érzékelő 2. LED (zöld) fényereje
    }   light ;         ///< Kontakt érzékelő LED fényerő
    struct sDisabled {
        uint8_t     k;      ///< Kontakt érzékelők tiltása 1: tiltott, LSB:0. ... MSB:7.
        unsigned    rev:4;  ///< Kontakt érzékelő párok fordított bekötés 1: fordított, LSB:1,2
        unsigned    s1:1;   ///< S1 kapcsoló tiltása 1: tiltott
        unsigned    s2:1;   ///< S2 kapcsoló tiltása 1: tiltott
    }   disabled;           ///< Risztások tíltása
};

extern struct eep_config   config;
extern struct eep_config   eep_config EEMEM;

/// A konfigot beolvassa az eeprom-ból a config struktúrába.
static inline void rdConfig(void) { eeprom_read_block(&config, &eep_config, sizeof(struct eep_config)); }
/// A konfigot kiírja az eeprom-ba a config struktúrából.
static inline void wrConfig(void) { eeprom_update_block(&config, &eep_config, sizeof(struct eep_config)); }
/// Ha az EEPROM configuráció még nincs beállítva, akkor nem nullával tér vissza
static inline int8_t   unConfigured() { return (config.type != MASTER && config.type != SLAVE) || config.id == 0; }

/*
ISP csatlakozó, mint jumper tüskesor:
 1 o o  2    (MOSI/Vcc)  unused
 3 o o  4    (-/GND)     unused
 5 o o  6    (Reset/GND) close: Reset
 7 o o  8    (SCK/GND)   open: Master; close: Slave (Ha setup van)
 9 o o 10    (MISO/GND)  open: normál müködés; close: Setup

CON5 Setup állapotban szintén jumperként funkcionál
A Slave vagy Master azonosítót adja meg (az ID nem lehet nulla!):
 1 o o  2    ID 0.bit:  close=1, open=0
 3 o o  4    ID 1.bit:  close=1, open=0
 5 o o  6    ID 2.bit:  close=1, open=0
 7 o o  8    ID 3.bit:  close=1, open=0
 9 o o 10    ID 4.bit:  close=1, open=0
11 o o 12    ID 5.bit:  close=1, open=0
13 o o 14
15 o o 16
*/
extern void setup() __ATTR_NORETURN__;
/*
ISP csatlakozó, mint jumper tüskesor:
 1 o o  2    (MOSI/Vcc)  unused
 3 o o  4    (-/GND)     unused
 5 o o  6    (Reset/GND) close: Reset
 7 o o  8    (SCK/GND)   open: normál müködés; close: setup2
 9 o o 10    (MISO/GND)  open: nem Setup; close: setup

CON5 Setup2 állapotban is jumperként funkcionál
A kontakt érzékelők megjétét adja meg
 1 o o  2   close=0.érzékelő tíltva, open=engedélyezve
 3 o o  4   close=1.érzékelő tíltva, open=engedélyezve
 5 o o  6   close=2.érzékelő tíltva, open=engedélyezve
 7 o o  8   close=3.érzékelő tíltva, open=engedélyezve
 9 o o 10   close=4.érzékelő tíltva, open=engedélyezve
11 o o 12   close=5.érzékelő tíltva, open=engedélyezve
13 o o 14   close=6.érzékelő tíltva, open=engedélyezve
15 o o 16   close=7.érzékelő tíltva, open=engedélyezve
JP3:        close=S1 kapcsoló tíltva, open=engedélyezve
JP4:        close=S2 kapcsoló tíltva, open=engedélyezve
*/
extern void setup2() __ATTR_NORETURN__;

extern void prnConf();
#endif
