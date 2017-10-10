#include "indalarmif2.h"


struct eep_config   config           = { 0 };   // ID = 0, unconfigured
struct eep_config   eep_config EEMEM = {
    1,      // ID
    0,      // Sensors disabled
    0,      // Sensors reverse
    0xfc,   // Switchs disabled
    0,      // Switchs inverted
    7,      // Sensors red leds light
    7,      // Sensors green leds light
    7,      // Sensors connectors red leds light
    7,      // Sensors connectors green leds light
    2,      // A/D Speed (2 = 460kHz)
    0,      // T0 ótajel (0 = 800Hz)
    0       // Autómatikus baállítása a rev biteknek, kiírásuk 2x300mp-enként (10p)
};

uint16_t confIsMod = 0;
uint32_t cnfWrCt = 0;

void prnConf()
{
    if (unConfigured()) {
        PRNP0("CONF:UNCOFIGURED\n");
        return;
    }
    PRNP1("ID:%d/K:", config.id);
    {
        uint16_t i;
        for (i = 1; i; i <<= 1) {
            if      (i & config.disabled) PRNC('D');    // Letiltva
            else if (i & config.rev)      PRNC('R');    // Engedélyezve, fordított bekötés
            else                          PRNC('E');    // Engedélyezve
        }
    }
    PRNP2("/L:%d.%d ", config.light1 +1, config.light2 +1);    // Érzékelő fényerő
    PRNP0("/S:");
    {
        uint8_t i;
        for (i = 1; i; i <<= 1) {
            if (i & config.s_disabled) PRNC('D');    // Letiltva
            else if (i & config.s_rev) PRNC('R');    // Engedélyezve, fordított logika
            else                       PRNC('E');    // Engedélyezve
        }
    }
    PRNP2(" /L:%d.%d\n", config.lightc1 +1, config.lightc2 +1);    // Érzékelő fényerő
    PRNP1("A/D Clock Speed: (#%d) ", config.adSpeed);
    switch (config.adSpeed) {
    case 0: PRNP0("115");   break;
    case 1: PRNP0("230");   break;
    case 2: PRNP0("460");   break;
    case 3: PRNP0("920");   break;
    }
    PRNP2("kHz; T0 Clock: (#%d) %dHz; ", config.t0Speed , config.t0Speed ? 1600 : 800);
    PRNP1("Autoconf: %d\n", config.autoSetRev);
}

/// ID beállítása
/// J0 Closed: SETUP
/// Setup típusa: J1,J2,J3 (LOW...HIGH, CLOSE = 1, OPEN = 0)
/// 0 (J1-J3 open)  id = (J4. J5, J6, scl, spa, spb, S1, S2) / SDA = close, akkor DIAGNOSZTIKA!!!
/// 1 (J1 close)    Érzékelők LED-jei light1 = (J4, J5. J6), light2 = (spa, spb, S1, sda)
/// 2 (j2 close)    disable sensors low ( 0:J5...7:sda)
/// 3 (j1,j2 close) disable sensors high ( 8:J5...15:sda)
/// 4 (j3 close)    disable switch ( 0:J5...7:sda)
/// 5 (j1,j3 close) inverz switch( 0:J5...7:sda)
/// 6 (j2,J3 close) kétszínű LED-ek light1 = (J4, J5. J6), light2 = (spa, spb, S1, sda)
/// 7 (j1-j3 close) A/D clock : (J4,5), T0 clock : spa, Auto conf : spb
void setup()
{
    if (!jumpers.bits.jp0) return;
    uint8_t t = (jumpers.all >> 1) & 7;
    if (t == 0 && jumpers.bits.sda) {
        diags();
        return;
    }
    DEBP1("Setup %d...\n", t);
    LEDON(RED1);
    LEDON(RED2);
    LEDON(BLUE);
    switch (t) {
    case 0: config.id = jumpers.all >> 5;           break;
    case 1: config.light1 = jumpers.all >> 5;   config.light2 = jumpers.all >> 9;           break;
    case 2: config.disabled = (config.disabled & 0xff00U) | ((jumpers.all >> 5) & 0x00ffU); break;
    case 3: config.disabled = (config.disabled & 0x00ffU) | ((jumpers.all << 3) & 0xff00U); break;
    case 4: config.s_disabled = jumpers.all >> 5;   break;
    case 5: config.s_rev      = jumpers.all >> 5;   break;
    case 6: config.lightc1 = jumpers.all >> 5;  config.lightc2 = jumpers.all >> 9;; break;
    case 7: config.adSpeed = jumpers.all >> 5;  config.t0Speed = jumpers.bits.spa;  config.autoSetRev = jumpers.bits.spb;   break;
    }
    wrConfig(); // Kiírjuk az EEPROM-ba
    prnConf();
    stop();
}

static uint8_t confWrFl = 0;

void autoConfClk()
{
    if (cnfWrCt) {
        // DEBC('*');
        cnfWrCt--;
        if (cnfWrCt == 0) {
            autoConfInit();
            if (confIsMod) {
                confWrFl  = 1;
                confIsMod = 0;
            }
        }
    }
    //else {
    //    DEBC('.');
    //}
}

void loop_conf()
{
    if (confWrFl) {
        confWrFl = 0;
        wrConfig();
        PRNP0("Config auto writed : \n");
        prnConf();
    }
}
