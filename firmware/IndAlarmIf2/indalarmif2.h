#ifndef	INDALARMIF2_H
#define	INDALARMIF2_H
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

#include "indalarmif2_gl.h"
#include "portbits.h"	// Ld.: portbits.m4
#include "serial.h"

#include "diag.h"

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

#define AD1V    (10*256/50)
#define AD2V    (20*256/50)
#define AD3V    (30*256/50)
#define AD4V    (40*256/50)

extern volatile uint8_t div1s;
extern volatile uint8_t biLeds1;
extern volatile uint8_t biLeds2;
extern volatile uint8_t silentCt;

extern const char perr_str1[] PROGMEM;
extern const char perr_str2[] PROGMEM;
#define PERR()  { \
    fputs_P(perr_str1, &rs232_str); \
    fputs_P(PSTR(__FILE__), &rs232_str); \
    fprintf_P(&rs232_str, perr_str2, __LINE__); \
}

/// Bejelentkező üzenet 1.sor
extern const char version1[] PROGMEM;
/// Bejelentkező üzenet 2.sor
extern const char version2[] PROGMEM;
/// Bejelentkező üzenet 3.sor
extern const char version3[] PROGMEM;
/// Bejelentkező üzenet 4.sor
extern struct vtm {
    char dt[11];        ///< Fordítás dátuma
    char sp;            ///< szóköz
    char tm[8];         ///< Fordítás időpontja
    char endl[2];       ///< Soremelés
} vtm PROGMEM;
#define	version4	v4.dt

/* ***************************************************************** */

/// @def SETBIT(PR) Egy port bit beállítása (1-be)
/// @param PR Port és Bit azonosító név előtag
#define SETBIT(PR)   PR ## _POU |=  _BV(PR ## _BIT)

/// @def CLRBIT(PR) Egy port bit törlése (0-ba)
/// @param PR Port és Bit azonosító név előtag
#define CLRBIT(PR)   PR ## _POU &= ~_BV(PR ## _BIT)

/// @def STPJP(JP)
/// Setup jumper port bit: Bit is input, and pull up resistor is on.
/// @param JP Jumper name
#define STPJP(JP)   JP ## _DDR &= ~_BV(JP ## _BIT); JP ## _POU |=  _BV(JP ## _BIT)

/// @def STPLED(L)
/// Setup LED port bit: Bit is output
/// @param L LED name
#define STPLED(L)   L ## _DDR |=  _BV(L ## _BIT)

/// @def GETBIT(PR)
/// Get port bit value
/// @param PR Bit name
/// @return 0 if port L, and 1 if port H.
#define GETBIT(PR)  (PR ## _PIN & _BV(PR ## _BIT) ? 1 : 0)

/// @def GETJP(JP)
/// Get jumper port bit value    jumpers.bits.jp3 = GETJP(J3);
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

/// AD konverter multiplexer beállítása
static inline void setADMux(uint8_t ch) {
    ADMUX  = (ADMUX  &  0xe0) | (ch & 0x07);
    ADCSRB = (ADCSRB & ~_BV(MUX5)) | (ch & _BV(MUX5));
}

/// @def GETADMUX()   AD konverter multiplexer kiolvasása
static inline uint8_t getADMux() {
    return (ADMUX & 0x07) | (ADCSRB & _BV(MUX5));
}


/* ***************************************************************** */
#include "eeconf.h"
#include "misc.h"
/// Kontakt érzékelők száma
#define KNUM    IAIF2PORTS
/// Kapcsolók, TTL bemenetek, mint érzékelők száma
#define SNUM    IAIF2SWS
/* ***************************************************************** */

union u_byte {
    uint8_t byte;
    struct {
        unsigned bit0:1;
        unsigned bit1:1;
        unsigned bit2:1;
        unsigned bit3:1;
        unsigned bit4:1;
        unsigned bit5:1;
        unsigned bit6:1;
        unsigned bit7:1;
    }       bits;
};

union u_word {
    uint16_t    word;
    struct {
        uint8_t     low;
        uint8_t     high;
    }           bytes;
    struct {
        unsigned bit0:1;
        unsigned bit1:1;
        unsigned bit2:1;
        unsigned bit3:1;
        unsigned bit4:1;
        unsigned bit5:1;
        unsigned bit6:1;
        unsigned bit7:1;
        unsigned bit8:1;
        unsigned bit9:1;
        unsigned bit10:1;
        unsigned bit11:1;
        unsigned bit12:1;
        unsigned bit13:1;
        unsigned bit14:1;
        unsigned bit15:1;
    }       bits;
};

#define BIT(v, n)   v.bits.bit##n

/*********************  Állapot válltozók *************************/

/// Jumper status data type
union jumpers {
    uint16_t all;
    struct {
        unsigned    jp0:1;  // 0 SHORT: SETUP a jumperek alapján Lsd.: eeconf.c : void setup()
        unsigned    jp1:1;  // 1
        unsigned    jp2:1;  // 2
        unsigned    jp3:1;  // 3
        unsigned    jp4:1;  // 4
        unsigned    jp5:1;  // 5
        unsigned    jp6:1;  // 6
        unsigned    scl:1;  // 7/IIC:SCL
        unsigned    spa:1;  // 8/ SPEAKER A
        unsigned    spb:1;  // 9/ SPEAKER B
        unsigned    s1:1;   //10/S1 kapcsolóm
        unsigned    s2:1;   //11/S2 kapcsoló
        unsigned    sda:1;  //12/IIC:SDA
    }       bits;
};
extern volatile union jumpers jumpers;

/// Kontakt érzékelők állapota (hogyan világítanak a LED-ek) érzékelő páronként
struct kLedSt {
    uint16_t    kLed1;  ///< LED1-ek
    uint16_t    kLed2;  ///< LED2-k
    uint16_t    kLedRd; ///< LED Red (LED1, ill. inverz esetén LED2)
    uint16_t    kLedGr; ///< LED Green (LED2, ill. inverz esetén LED1)
};

/// A kontakt érzékelő LED-ek állapota
extern volatile struct kLedSt kLedSt;
/// A kontakt érzékelőkön mért értékek 1. LED "világít"
extern volatile uint8_t kVal1[KNUM];
/// A kontakt érzékelőkön mért értékek 2. LED "világít"
extern volatile uint8_t kVal2[KNUM];
/// Kontakt érzékelők riasztási állapota
extern volatile uint8_t  kStat[KNUM];
/// kapcsolók, beolvasott értékek
extern volatile union u_byte sVal;
/// Kapcsolók riasztási állapota
extern volatile uint8_t  sStat[SNUM];
/// Az érzékelők mérésénél a fázisok (állapotok) száma
#define KPHASES   (KNUM * 2)
/// Érzékelők mérésének fázisai
extern volatile uint8_t  kPhase;
/// Érzékelők mérésénél melyik LED van kigyújtva az érzékelőn, index
extern volatile uint8_t kLedPhase;
/// Érzékelők mérésénél melyik csatornát mérjük
extern volatile uint8_t kIxPhase;
/// Érzékelők mérésénél melyik LED van kigyújtva az érzékelőn, 16 bites maszk a
extern volatile uint16_t kMask;

extern volatile uint16_t cnt1s;

#define ALIVE_CTC0  1
#define ALIVE_ADC   2
#define ALIVE_CTC2  4
#define ALIVE_NET   8
#define ALIVE_DEB  16
#define ALIVE_ALL   (ALIVE_CTC0 | ALIVE_ADC | ALIVE_CTC2)
extern volatile uint8_t alive;

union errstat {
	uint16_t		all;
	struct {
        unsigned    wdt_occur:1;        ///< A watchdog ujraindította a kontrolert
        unsigned    bor_occur:1;        ///< Brown-out reset volt
        unsigned    exr_occur:1;        ///< Volt egy külső reset
        unsigned    pon_occur:1;        ///< Power on reset volt
        unsigned    txnbuffoverfl:1;    ///< Adó buffer túlcsordulás történt (RS485)
        unsigned    rxnbuffoverfl:1;    ///< Vevő buffer túlcsorduéás történt (RS485)
        unsigned    rxn_fre:1;          ///< Vételi formátum hiba (RS485)
        unsigned    txdbuffoverfl:1;    ///< Adó buffer túlcsordulás történt (RS232)
        unsigned    rxdbuffoverfl:1;    ///< Vevő buffer túlcsorduéás történt (RS232)
        unsigned    rxd_fre:1;          ///< Vételi formátum hiba (RS232)
        unsigned    linbuffoverfl:1;    ///< Line buffer túlcsorduéás történt (RS232)
        unsigned    wordbuffoverfl:1;   ///< Parser szó buffer túlcsordulás történt (RS232)
        unsigned    adc:1;              ///< ADC hiba volt
        unsigned    oeto:1;             ///< OE time out
        unsigned    perr:1;             ///< egyébb program hiba
    }           bits;
};

extern volatile union errstat	errstat;

union uIsOff {
    uint8_t     all;
    struct {
        unsigned    ad:1;       // Az AD konverzió tiltása
        unsigned    silent:1;   // Ha nincs forgalom, akkor a reset tiltása
        unsigned    diag:1;     // Diagnosztikai mód
    }   bits;
};

extern volatile union uIsOff isOff;

/********************************** INLINE **************************************/

static inline void prnJumpers()
{
    PRNP0("JP:");
    PRNC(jumpers.bits.jp0 ? 'C' : 'O');
    PRNC(jumpers.bits.jp1 ? 'C' : 'O');
    PRNC(jumpers.bits.jp2 ? 'C' : 'O');
    PRNC(jumpers.bits.jp3 ? 'C' : 'O');
    PRNC(jumpers.bits.jp4 ? 'C' : 'O');
    PRNC(jumpers.bits.jp5 ? 'C' : 'O');
    PRNC(jumpers.bits.jp6 ? 'C' : 'O');
    PRNC(jumpers.bits.scl ? 'C' : 'O');
    PRNC(jumpers.bits.spa ? 'C' : 'O');
    PRNC(jumpers.bits.spb ? 'C' : 'O');
    PRNC(jumpers.bits.s1  ? 'C' : 'O');
    PRNC(jumpers.bits.s2  ? 'C' : 'O');
    PRNC(jumpers.bits.sda ? 'C' : 'O');
    PRNC('\n');
}


/// Az új riasztási állapot (kontakt érzékelő)
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

/// Az új riasztási állapot (kapcsoló (bit))
/// @param nk A legújabb kiértékelés szerinti állapot (1:OK, 0:Alarm)
/// @param ok A régi riasztási állapot
/// @return Az új riasztási állapot
static inline char newSwStat(uint8_t nk, char ok)
{
    if (ok == 0) return KNULL;          // Eredményt (reset utáni első mérés) eldobjuk
    if (!nk) return KALERT;             // Riasztás
    if (isAlert(ok)) return ok | KOLD;  // Már nem áll fenn a riasztási állapot, de a régi még aktív
    return KOK;
}

static inline uint8_t isNoAlertNow(enum eKStat s)
{
    return ((s == KOK) || (s & KOLD)) ? 1 : 0;
}


extern void fatal() __ATTR_NORETURN__;
extern void stop() __ATTR_NORETURN__;
extern void reset(void) __ATTR_NORETURN__;

#define ERROR(e)    { DEBP3("Fatal %S. %S[%d]\n", PSTR(e), PSTR(__FILE__), __LINE__); fatal(); }

static inline void t0init()
{
    DEBP0("I:T0\n");
    /* Timer0: IT */
    TCNT0 = 0;
    TCCR0A = _BV(WGM01);            // CTC mód
    TCCR0B = _BV(CS02) | _BV(CS00); // 14745600/1024 =  14.4kHz
    switch (config.t0Speed) {
    case 0: OCR0A  = 18;    break;  //    14400/  18 = 800 Hz
    case 1: OCR0A  =  9;    break;  //    14400/   9 =1600 Hz
    }
    TIMSK0 = _BV(OCIE0A);           //  Timer/Counter0 Output Compare Match A Interrupt Enable
}

static inline void t2init()
{
    DEBP0("I:T2\n");
    TCNT2  = 0;
    TCCR2A = _BV(WGM21);                        // CTC,mód
    TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); // 14745600/1024 =  14.4kHz
    OCR2A  =  144;                              //    14400/ 144 = 100   Hz
    TIMSK2 = _BV(OCIE2A);
}

static inline void adinit()
{
    /* AD konverter */
    DEBP0("I:A/D\n");
    isOff.bits.ad = 0;
    memset((void *)kVal1, 0, KNUM);
    memset((void *)kVal2, 0, KNUM);
    ADMUX  = _BV(REFS0) | _BV(ADLAR);    // Referencia a tápfesz, eredmény balra igazítva (8bit)
    // AD konverter engedélyezve, it is, előosztó ...
    // Javasolt órajel 200kHz, max 1000kHz
    switch (config.adSpeed) {
        // előosztó 128 (115.2 kHz)
    case 0: ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);   break;
        // előosztó 64 (230.4 kHz)
    case 1: ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS1) | _BV(ADPS2);   break;
        // előosztó 32 (460.8 kHz)
    case 2: ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS2);   break;
        // előosztó 16 (921.6 kHz)
    case 3: ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2);   break;
    }
    ADCSRB = 0;
    DIDR0  = 0xff;  // Digital input disable
    DIDR2  = 0xff;
    /* Kontakt érzékelőket tápláló port kimenetként állítása, H kimeneti szint mindegyikre */
    SAL_DDR = 0xff;
    SAH_DDR = 0xff;
    SBL_DDR = 0xff;
    SBH_DDR = 0xff;
    SAL_POU = 0xff;
    SAH_POU = 0xff;
    SBL_POU = 0xff;
    SBH_POU = 0xff;
}


#endif	/* INDALARMIF2_H */
