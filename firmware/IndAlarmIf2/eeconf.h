#ifndef EECONF_H
#define EECONF_H

#include <avr/eeprom.h>

struct eep_config {
    uint8_t     id;             ///< RS485 id, ill. cím (0 < id < 64)
    uint16_t    disabled;       ///< Kontakt érzékelők tiltása 1: tiltott, LSB:0. ... MSB:7.
    uint16_t    rev;            ///< Kontakt érzékelő fordított bekötés 1: fordított, LSB:0, ...
    uint8_t     s_disabled;     ///< kapcsolók tiltása LSB= S1, S2, "SCK", J0...J4
    uint8_t     s_rev;          ///< kapcsolók polaritása: 0: L=ok, H=alarm; 1: H=ok, L=alarm
    unsigned    light1:3;       ///< Kontakt érzékelő 1. LED (piros) fényereje
    unsigned    light2:3;       ///< Kontakt érzékelő 2. LED (zöld) fényereje
    unsigned    lightc1:3;      ///< RJ45-2 színű LEDek fényereje, piros
    unsigned    lightc2:3;      ///< RJ45-2 színű LEDek fényereje, zöld
    unsigned    adSpeed:2;      ///< A/D konverter sebessége (0:115kHz; 1:230kHz; 2:460kHz; 3:920kHz)
    unsigned    t0Speed:1;      ///< T0 órajel (0:800Hz; 1:1600Hz)
    uint8_t     autoSetRev;     ///< Auto set rev bits, 0: no, >0, auto set and write *300 secs (5m).
};

extern struct eep_config   config;
extern struct eep_config   eep_config EEMEM;
/// A konfigot beolvassa az eeprom-ból a config struktúrába.
static inline void rdConfig(void) { eeprom_read_block(&config, &eep_config, sizeof(struct eep_config)); }
/// A konfigot kiírja az eeprom-ba a config struktúrából.
static inline void wrConfig(void) { eeprom_update_block(&config, &eep_config, sizeof(struct eep_config)); }
/// Ha az EEPROM configuráció még nincs beállítva, akkor nem nullával tér vissza
static inline int8_t   unConfigured() { return config.id == 0; }

/*
  Feltételes szetup a jumperekkel
*/
extern void setup();

extern void prnConf();

/// Ha értéke nem nulla, akkor a konfig módosítva lett
extern uint16_t  confIsMod;
/// Számláló a konfig autómatikus kiírásához.
extern uint32_t cnfWrCt;

static inline void invRevConfM(uint16_t m)
{
    config.rev ^= m;
    confIsMod  ^= m;
}

static inline void invRevConf(uint8_t i)
{
    invRevConfM(((uint16_t)1) << i);
}

static inline void autoConfInit()
{
    cnfWrCt = config.autoSetRev * 300;
}

extern void autoConfClk();
extern void loop_conf();

#endif
