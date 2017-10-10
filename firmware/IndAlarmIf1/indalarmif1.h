#ifndef	INDALARMIF1_H
#define	INDALARMIF1_H
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay_basic.h>
#include <util/atomic.h>

#include "indalarmif1_gl.h"

/* System clock in Hz.*/
#define F_CPU      14745600L

#define PRNC(c)                 putchar(c)
#define PRNS(s)                 fputs_P(s, stdout)
#define _PRNP1(f,p)             printf_P(f, p)
#define PRNP0(s)                fputs_P(PSTR(s), stdout)
#define PRNP1(f,p)              printf_P(PSTR(f),p)
#define PRNP2(f,p1,p2)          printf_P(PSTR(f),p1,p2)
#define PRNP3(f,p1,p2,p3)       printf_P(PSTR(f),p1,p2,p3)
#define PRNP4(f,p1,p2,p3,p4)    printf_P(PSTR(f),p1,p2,p3,p4)
#define PRNDMP(b,s)             dump(stdout, b, s);
#define PRNCHR(c)               prnc(stdout, c)

#define DEBUG   1

#if DEBUG
#define DEBC(c)                 rs232_putchar(c, NULL)
#define DEBS(s)                 fputs_P(s, &rs232_str)
#define DEBP0(s)                fputs_P(PSTR(s), &rs232_str)
#define DEBP1(f,p)              fprintf_P(&rs232_str, PSTR(f),p)
#define DEBP2(f,p1,p2)          fprintf_P(&rs232_str, PSTR(f),p1,p2)
#define DEBP3(f,p1,p2,p3)       fprintf_P(&rs232_str, PSTR(f),p1,p2,p3)
#define DEBP4(f,p1,p2,p3,p4)    fprintf_P(&rs232_str, PSTR(f),p1,p2,p3,p4)
#define DEBDMP(b,s)             dump(&rs232_str, b, s);
#define DEBCHR(c)               prnc(&rs232_str, c)
#else   /* DEBUG */
#define DEBC(c)
#define DEBS(s)
#define DEBP0(s)
#define DEBP1(f,p)
#define DEBP2(f,p1,p2)
#define DEBP3(f,p1,p2,p3)
#define DEBP4(f,p1,p2,p3,p4)
#define DEBDMP(b,s)
#define DEBCHR(c)
#endif  /* DEBUG */

#include "serial.h"

extern const char version1[] PROGMEM;
extern const char version2[] PROGMEM;
extern const char version3[] PROGMEM;
extern struct vtm {
	char dt[11];
	char sp;
	char tm[8];
    char endl[2];
} vtm PROGMEM;
#define	version4	v4.dt

/* ***************************************************************** */
// LED1 (Red, negatív logika! L:On;H:Off)
#define L1_DDR  DDRD
#define L1_PIN  PIND
#define L1_POU  PORTD
#define L1_BIT  7
// LED2 (Green, negatív logika! L:On;H:Off)
#define L2_DDR  DDRB
#define L2_PIN  PINB
#define L2_POU  PORTB
#define L2_BIT  4
// OE (RS485 output enable (pozitív logika))
#define OE_DDR  DDRD
#define OE_PIN  PIND
#define OE_POU  PORTD
#define OE_BIT  6
// JP3 jumper
#define J3_DDR  DDRD
#define J3_PIN  PIND
#define J3_POU  PORTD
#define J3_BIT  4
// JP3 jumper
#define J4_DDR  DDRD
#define J4_PIN  PIND
#define J4_POU  PORTD
#define J4_BIT  5
// SCK Jumperként hasznélható
#define SC_DDR  DDRB
#define SC_PIN  PINB
#define SC_POU  PORTB
#define SC_BIT  7
// MISO Jumperként hasznélható
#define MI_DDR  DDRB
#define MI_PIN  PINB
#define MI_POU  PORTB
#define MI_BIT  6
// MOSI Jumperként hasznélható
#define MO_DDR  DDRB
#define MO_PIN  PINB
#define MO_POU  PORTB
#define MO_BIT  5
// S1 Kapcsoló (fedél nyitott)
#define S1_DDR  DDRC
#define S1_PIN  PINC
#define S1_POU  PORTC
#define S1_BIT  2
// S2 Kapcsoló (NYÁKOT ELMOZDÍTOTTÁK)
#define S2_DDR  DDRC
#define S2_PIN  PINC
#define S2_POU  PORTC
#define S2_BIT  3

// Kontakt érzékelő kimenet A (AD konverter felöli oldal)
#define KOA_DDR     DDRB
#define KOA_POU     PORTB
#define KOA01_BIT   0
#define KOA23_BIT   1
#define KOA45_BIT   2
#define KOA67_BIT   3
#define KOA_BITS 0x0f
// Kontak érzékelő kimenet B (AD ellen oldal, kis ellenállású rész)
#define KOB_DDR     DDRC
#define KOB_POU     PORTC
#define KOB01_BIT   7
#define KOB23_BIT   6
#define KOB45_BIT   5
#define KOB67_BIT   4
#define KOB_BITS 0xf0

/// @def SETBIT(PN,BN) Egy port bit beállítása (1-be)
/// @param PN Port azonosító név
/// @param BN Bit azonosító név
#define SETBIT(PN,BN)   PN ## _POU |= _BV(BN ## _BIT)

/// @def CLRBIT(PN,BN) Egy port bit törlése (0-ba)
/// @param PN Port azonosító név
/// @param BN Bit azonosító név
#define CLRBIT(PN,BN)   PN ## _POU &= ~_BV(BN ## _BIT)

/// @def SETK(PN,BN) Kontakt érzékelő kimeneti bit beállítása
/// @param PN Kontakt érzékelő pólus azonosító A vagy B
/// @param BN Bit azonosító szám: 01,23,45,67
#define SETK(PN,BN)   KO ## PN ## _POU |= _BV(KO ## PN ## BN ## _BIT)

/// @def CLRBIT(PN,BN) Kontakt érzékelő kimeneti bit törlése (0-ba)
/// @param PN Kontakt érzékelő pólus azonosító A vagy B
/// @param BN Bit azonosító szám: 01,23,45,67
#define CLRK(PN,BN)   KO ## PN ## _POU &= ~ _BV(KO ## PN ## BN ## _BIT)

/// @def ADMUXMSK   AD konverter multiplexer bitek maszkja
#define ADMUXMSK    0x1f

/// @def SETADMUX(c)   AD konverter multiplexer beállítása
#define SETADMUX(c) ADMUX    = (ADMUX & ~ADMUXMSK) | (c)

/// @def GETADMUX()   AD konverter multiplexer kiolvasása
#define GETADMUX() (ADMUX & ADMUXMSK)

/* ***************************************************************** */
#include "twi.h"
#include "eeconf.h"
#include "misc.h"
/* ***************************************************************** */

/// @def GETJP(JP)
/// Get jumper port bit value
/// @param JP Bit name
/// @return 0 if jumper is open, and 1 if jumper is close.
#define GETJP(JP)   (JP ## _PIN & _BV(JP ## _BIT) ? 0 : 1)
/// @def LEDON(L)
/// LED kigyújtása (negatív logika!)
/// @param L LED neve (sorszáma)
#define LEDON(L)    L ## _POU &= ~_BV(L ## _BIT)
/// @def LEDOFF(L)
/// LED kioltása (negatív logika!)
/// @param L LED neve
#define LEDOFF(L)    L ## _POU |= _BV(L ## _BIT)
/// @def LEDINV(L)
/// LED állapot invertálása
/// @param L LED neve
#define LEDINV(L)    L ## _PIN = _BV(L ## _BIT)

/*********************  Állapot válltozók *************************/
/// Kontakt érzékelők száma
#define KNUM    8

/// Jumper status data type
union jumpers {
    uint8_t all;
    struct {
        unsigned    jp3:1;
        unsigned    jp4:1;
        unsigned    sck:1;  // Setup: Open:Master;Close:Slave
        unsigned    miso:1; // Close:Setup
        unsigned    mosi:1;
    }       bits;
};
extern union jumpers jumpers;

/// @enum   eKLedSt
/// Kontakt érzékelők állapota (hogyan világítanak a LED-ek)
enum eKLedSt {
    KLED1 = 1,  ///< Bit maszk, 1LED viágít, bit = 1
    KLED2 = 2   ///< Bit maszk, 2LED viágít, bit = 1
};

/// Kontakt érzékelők állapota (hogyan világítanak a LED-ek) érzékelő páronként
/// 2 bites mezők, bitek értelmezése az eKLedSt maszk alaján
union kLedSt {
    uint8_t all;
    struct {
        unsigned    k01:2;
        unsigned    k23:2;
        unsigned    k45:2;
        unsigned    k67:2;
    }   bits;
};

/// Kontakt érzékelők LED-jeinek a fényereje (mennyire világitson)
/// Az összes érzékelőre, 1 és 2-es LED-re külön megadva.
/// A 0 egyszeres, az 1 kétszeres, a 2 háromszoros és a 3 négyszeres fényerőt jelent.
struct kLight {
    unsigned    l1:2;       ///< 1. LED fényereje
    unsigned    l2:2;       ///< 2. LED fényereje
    unsigned    phase:1;    ///< Utoljára kigyujtott LED (0:LED1, 1:LED2)
};

/// A kontakt érzékelő LED-ek állapota (páronként)
extern union kLedSt kLedSt;
/// A kontakt érzékelő LED-ek fényereje
extern struct kLight kLight;
/// A kontakt érzékelőkön mért értékek 1. LED "világít"
extern uint8_t kVal1[KNUM];
/// A kontakt érzékelőkön mért értékek 2. LED "világít"
extern uint8_t kVal2[KNUM];
/// Kontakt érzékelők riasztási állapota
extern uint8_t  kStat[KNUM];
/// Kapcsolók riasztási állapota
extern uint8_t  sStat[2];

extern uint8_t  oeCt;
#define OE_DELAY	10

#define ALIVE_CTC0  1
#define ALIVE_ADC   2
#define ALIVE_CTC2  4
#define ALIVE_SER0  8
#define ALIVE_SER1 16
#define ALIVE_TWI  32
#define ALIVE_ALL   (ALIVE_CTC0 | ALIVE_ADC | ALIVE_CTC2)
extern uint8_t alive;

union errstat {
	uint16_t		all;
	struct {
        unsigned    wdt_occur:1;        ///< A watchdog ujraindította a kontrolert
        unsigned    bor_occur:1;        ///< Brown-out reset volt
        unsigned    exr_occur:1;        ///< Volt egy külső reset
        unsigned    pon_occur:1;        ///< Power on reset volt
        unsigned    tx0buffoverfl:1;    ///< Adó buffer túlcsordulás történt (RS485)
        unsigned    rx0buffoverfl:1;    ///< Vevő buffer túlcsorduéás történt (RS485)
        unsigned    rx0_fre:1;          ///< Vételi formátum hiba (RS485)
        unsigned    tx1buffoverfl:1;    ///< Adó buffer túlcsordulás történt (RS232)
        unsigned    rx1buffoverfl:1;    ///< Vevő buffer túlcsorduéás történt (RS232)
        unsigned    rx1_fre:1;          ///< Vételi formátum hiba (RS232)
        unsigned    linbuffoverfl:1;    ///< Line buffer túlcsorduéás történt (RS232)
        unsigned    wordbuffoverfl:1;   ///< Parser szó buffer túlcsordulás történt (RS232)
        unsigned    twi:1;              ///< I2C hiba volt
        unsigned    adc:1;              ///< ADC hiba volt
        unsigned    s1:1;               ///< S1 is open
        unsigned    s2:1;               ///< S2 is open
    }           bits;
};

extern	union errstat	errstat;

/********************************** INLINE **************************************/

static inline void prnJumpers()
{
    PRNP0("JP:");
    PRNC(jumpers.bits.jp3  ? 'C' : 'O');
    PRNC(jumpers.bits.jp4  ? 'C' : 'O');
    PRNC(jumpers.bits.sck  ? 'C' : 'O');
    PRNC(jumpers.bits.miso ? 'C' : 'O');
    PRNC(jumpers.bits.mosi ? 'C' : 'O');
    PRNC('\n');
}


#define AD1V    (14*256/50)
#define AD2V    (20*256/50)
#define AD3V    (30*256/50)
// A 4V nagyon határeset, lejebb vesszük 3.6V-ra
#define AD4V    (36*256/50)
/// Kontakt érzékelő mért érték kiértékelése
/// @param i A kontakt érzékelő sorszáma (0-7)
/// @return A kiértékelés eredménye ld.: enum eKStat
static inline char _kEval(uint8_t i)
{
    register uint8_t v1, v2;
    // Index ellenörzés, ha rosz program hiba
    if (i > 7) return KERR;
    // Ha tiltva van az érzékelő
    if (config.disabled.k & _BV(i)) return KDISA;
    v1 = kVal1[i];
    v2 = kVal2[i];
    // Ha nincs mért érték
    if (v1 == 0 && v2 == 0) return KNULL;
    // Ha mindkét érték 2V és 3V között akkor OK.
    if (AD2V < v1 && AD3V > v1 && AD2V < v2 && AD3V > v2) return KOK;
    // Ha v1 1V alatt, és v2 4V felett, akkor rövidzár
    if (AD1V > v1 && AD4V < v2) return KSHORT;
    // Ha v2 1V alatt, és v1 4V felett, akkor a vezeték szakadt vagy elvégták
    if (AD1V > v2 && AD4V < v1) return KCUT;
    // Ha v1 érték 2V és 3V között, v1 4V felett, akkor az érzékelő kapcsoló nem zárt
    if (AD1V > v2 && AD2V < v1 && AD3V > v1) return config.disabled.rev & _BV(i/2) ? KINV : KALERT;
    // Ha v2 érték 2V és 3V között, v1 4V felett, akkor az érzékelő kapcsoló nem zárt, fordított bekötés esetén
    if (AD4V < v1 && AD2V < v2 && AD3V > v2) return config.disabled.rev & _BV(i/2) ? KALERT : KINV;
    // Nem értékelhető
    return KANY;
}
/// Egymás után ennyi egyforma status kell, hogy elhiggyük
#define MINEQ  3
/// Ha egymás után ennyiszer nincs hihatő status, akkor az baj
#define MAXNE 15
static inline char kEval(uint8_t i)
{
    static   char       prevStats[IAIF1PORTS];
    static struct {
        unsigned cteq:3;    // Azonos értékek számlálója
        unsigned ctfl:5;    // Nincs hihető érték számláló
    }                   cnts[IAIF1PORTS];
    register char       r  = _kEval(i);
    register uint8_t    ct;

    if (prevStats[i] == r) {
        ct = cnts[i].cteq +1;
        if (MINEQ <= ct) {
            cnts[i].ctfl = 0;
            return r;
        }
        cnts[i].cteq = ct;
    }
    else {
        prevStats[i] = r;
    }
    ct = cnts[i].ctfl +1;
    if (MAXNE <= ct) return KERR;
    cnts[i].ctfl = ct;
    return KNULL;
}

/// Kapcsolók állapotának a kiértékelése
#define SEVAL(n)    ((S##n##_PIN & _BV(S##n##_BIT)) ? 1 : 0)

/// Az új riasztási állapot
/// @param nk A legújabb kiértékelés szerinti állapot (ls.: kEval(uint8_t i) )
/// @param ok A régi riasztási állapot
/// @return Az új riasztási állapot
static inline char newStat(char nk, char ok)
{
    if (ok == 0)     return KNULL;      // Eredményt (reset utáni első mérés) eldobjuk
    if (nk == KNULL) return ok;         // Nem hittük el az új eredményt, marad a régi.
    if (isAlert(ok)) {                  // Ha a régi érték is risztás volt
        if (isAlert(nk)) return nk;     // Ha mindkettő riasztás, akkor az új az aktuális
        return ok | KOLD;               // Már nem áll fenn a riasztási állapot, de a régi még aktív
    }
    return nk;
}

extern void fatal() __ATTR_NORETURN__;
extern void stop() __ATTR_NORETURN__;
extern void reset(void) __ATTR_NORETURN__;

#define ERROR(e)    { DEBP3("Fatal %S. %S[%d]\n", PSTR(e), PSTR(__FILE__), __LINE__); fatal(); }

#endif	/* INDALARMIF1_H */
