#include "indalarmif1.h"


struct eep_config   config           = { UNKNOWN, 0 };
struct eep_config   eep_config EEMEM = { SLAVE,   1, { 3, 3 }, { 0, 0xA, 1, 1 } };


void prnConf()
{
    uint8_t i;
    if (config.type == UNKNOWN || config.id == 0) {
        PRNP0("CONF:UNCOFIGURED\n");
        return;
    }
    switch (config.type) {
        case MASTER:    PRNP1("CONF:M/%d:K/", config.id);  break;
        case SLAVE:     PRNP1("CONF:S/%d:K/", config.id);  break;
        default:        ERROR("PRG");                      break;
    }
    for (i = 1; i; i <<= 1) {
        PRNC(i & config.disabled.k ? 'D' : 'E');
    }
    PRNC('/');
    for (i = 1; i < 0x10; i <<= 1) {
        PRNC(i & config.disabled.rev ? 'R' : '-');
    }
    PRNP1(":S1/%c", config.disabled.s1 ? 'D' : 'E');
    PRNP1(":S2/%c\n", config.disabled.s2 ? 'D' : 'E');
}

void setup()
{
    uint8_t k;
    DEBP0("Setup...\n");
    LEDON(L1);
    LEDON(L2);
    config.type  = jumpers.bits.sck ? SLAVE : MASTER;  // open = MASTER, close = SLAVE
    DDRA = 0;   // PA bemenet
    // OA = Lowstatic inline void twi_init(uint8_t sla, eventPtr pFn)
    KOA_DDR |=  KOA_BITS;
    KOA_POU &= ~KOA_BITS;
    // OB = Hihg ( Nagy áram : 4 x 25 mA !)
    KOB_DDR |=  KOB_BITS;
    KOB_POU |=  KOB_BITS;
    // Várunk egy picit
    _delay_loop_1(10);
    // Read ID
    config.id = PINA;
    // OA = High ( Nagy áramfelvétel vége )
    KOA_POU |=  KOA_BITS;
    if (config.id == 0) {   // A nulla ID nem megengedett
        DEBP0("Invalid ID!\n");
        fatal();
    }
    k = 3;  // Érzékelő LED-ek max fényerő
    if (jumpers.bits.jp3) k  = 2;
    if (jumpers.bits.jp4) k -= 2;
    config.light.k2 = config.light.k1 = k;
    wrConfig(); // Kiírjuk az EEPROM-ba
    prnConf();
    stop();
}

void setup2()
{
    DEBP0("Setup2...\n");
    LEDON(L1);
    LEDON(L2);
    DDRA = 0;   // PA bemenet
    // OA = Lowstatic inline void twi_init(uint8_t sla, eventPtr pFn)
    KOA_DDR |=  KOA_BITS;
    KOA_POU &= ~KOA_BITS;
    // OB = Hihg ( Nagy áram : 4 x 25 mA !)
    KOB_DDR |=  KOB_BITS;
    KOB_POU |=  KOB_BITS;
    // Várunk egy picit
    _delay_loop_1(10);
    // Read ID
    config.disabled.k = PINA;
    // OA = High ( Nagy áramfelvétel vége )
    KOA_POU |=  KOA_BITS;
    config.disabled.s1 = jumpers.bits.jp3;
    config.disabled.s2 = jumpers.bits.jp4;
    wrConfig(); // Kiírjuk az EEPROM-ba
    prnConf();
    stop();
}


