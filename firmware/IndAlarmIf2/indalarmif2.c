#include "indalarmif2.h"
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <OWIStateMachine.h>

//Hiba stringek
const char perr_str1[] PROGMEM = "Fatal:";
const char perr_str2[] PROGMEM = "[%d]\n";
// Verzio �s (c)
const char version1[]  PROGMEM =        "IndAlarmIf2 ???\n";    // 1sor: Nem konfigur�lt
const char version1m[] PROGMEM =        "IndAlarmIf2 %02d\n";   // 1sor: param�ter az RS485 "c�m"
const char version2[]  PROGMEM =        "V:2.1 ";               // 2 sor +vtm
struct vtm vtm PROGMEM = { __DATE__, ' ', __TIME__, "\n" };     // A ford�t�s d�tuma a m�sodik sorban
const char version3[]  PROGMEM =        "(c)Indasoft bt.\n";    // 3 sor
// Jumperek �llapota
volatile union jumpers jumpers;
// �rz�kel� LED �llapotok
volatile struct kLedSt kLedSt;
// �rz�kel� utols� m�r�si eredm�nyek
volatile uint8_t kVal1[KNUM] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
volatile uint8_t kVal2[KNUM] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
// Kapcsol�k �llapota (�tfed�s van a jumperekkel !)
volatile union u_byte sVal;
// Modulok 'sz�vver�se' ha fut, 1-be billenti a flag-j�t, az ellen�rz�s pedig t�rli
volatile uint8_t  alive = 0;
// Sz�ml�l� (oszt�) az 1sec id� m�r�s�hez, a 100Hz-es IT (TIMER2) kezeli
volatile uint8_t div1s = 100;
// Sz�ml�l�, a bekapcsol�s �ta eltellt id� m�sodpercben
volatile uint16_t cnt1s = 0;
// M�ri az id�t, mennyi ideig nincs kommunik�ci� az RS485-�n, m�sodperc sz�ml�l�
volatile uint8_t  silentCt = 0;
// �rz�kel�k LED vez�rl�s/m�r�si f�zisok sz�ml�l�ja.
volatile uint8_t  kPhase    = KPHASES -1;   // utols� f�zis, inkrement�l�s ut�n a 0-val kezd�nk
// F�zis seg�d v�ltoz�, egy �rz�kel�n bell�l melyik LED-del foglakozunk �ppen (0/1)
volatile uint8_t  kLedPhase = 0;
// F�zis seg�d v�ltoz�, hanyadik LED-p�rral/�rz�kel�vel foglakozunk �ppen
volatile uint8_t  kIxPhase  = 0;
// F�zis seg�d v�ltoz�, egy egy bites maszk az �rz�kel� index�b�l.
volatile uint16_t kMask;
// Utols� m�rt h?m�rs�klet 0.5 fokonk�nt, 255 = invalid
volatile uint8_t    tmp;


/// Query message receive buffer
struct qMsg2     qMsg2 = { "??", "00", 'F', "0000", "00", "\r\n"};
/// Answer message send buffer
struct aMsg2    aMsg2 = { "..", "00", "0000000000000000", "00000000", "0000", "0000", "\r\n" };
struct cMsg2    cMsg2 = { "!!", "00", "0000", "00", "00", "7777", "\r\n" };
struct tMsg     tMsg;
char *          pMsg = NULL;

volatile union errstat   errstat;
// �rz�kel� port st�tuszok
volatile uint8_t  kStat[KNUM] = { KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL };
// Kapcsol� port st�tuszok
volatile uint8_t  sStat[SNUM] = { KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL };

/* Az RJ45-�s �rz�kel�k csatlakoz�in l�v� LED-ek �llapota */
volatile uint8_t biLeds1 = 0;
volatile uint8_t biLeds2 = 0;

volatile union uIsOff isOff;

void setupByMsg(void);

// A jumperek �llapot�nak a beolvas�sa. �s t�rol�sa a jumpers global strukt�r�ba.
static inline void getJumpers()
{
    jumpers.bits.jp0 = GETJP(J0);
    jumpers.bits.jp1 = GETJP(J1);
    jumpers.bits.jp2 = GETJP(J2);
    jumpers.bits.jp3 = GETJP(J3);
    jumpers.bits.jp4 = GETJP(J4);
    jumpers.bits.jp5 = GETJP(J5);
    jumpers.bits.jp6 = GETJP(J6);
    jumpers.bits.scl = GETJP(SCL);
    jumpers.bits.spa = GETJP(SPA);
    jumpers.bits.spb = GETJP(SPB);
    jumpers.bits.s1  = GETJP(S1);
    jumpers.bits.s2  = GETJP(S2);
    jumpers.bits.sda = GETJP(SDA);
}

// A csatlakoz�kon a k�t sz�n� LED-ek k�t f�zis�nak a be�ll�t�sa
// Azt, hogy mely LED-ek vil�g�tanak a biLeds1 �s biLeds2 glob�l tartalmazza
// A param�ter a f�zis (0/nem 0)
static inline void setBiLeds(uint8_t ledPhase)
{
    if (ledPhase) {
        LBL_POU = (LBL_POU & ~LBL_MSK) | (biLeds2 & LBL_MSK);
        LBH_POU = (LBH_POU & ~LBH_MSK) | (biLeds2 & LBH_MSK);
        CLRBIT(LA);
    }
    else {
        LBL_POU = (LBL_POU & ~LBL_MSK) | (~biLeds1 & LBL_MSK);
        LBH_POU = (LBH_POU & ~LBH_MSK) | (~biLeds1 & LBH_MSK);
        SETBIT(LA);
      }
}
// A csatlakoz�kon a k�t sz�n� LED-ek kikapcsol�sa
static inline void offBiLeds()
{
    LBL_POU &= ~LBL_MSK;
    LBH_POU &= ~LBH_MSK;
    CLRBIT(LA);
}

// �lltal�nos inicializ�l�rutin
static inline void prtinit(void)
{
    // Alap init
    MCUCR = _BV(JTD);    // PUD=0 (felh�z� ellen�ll�sok) enged�lyezve)
    MCUCR = _BV(JTD);    // JTAG tiltva (tilthat� FUSE-b�l is, de semmik�ppen nem kell.)
    PRR0  = _BV(PRTWI) | _BV(PRTIM1) | _BV(PRSPI);   // TWI, TIM1 and SPI power down
    PRR1  = _BV(PRTIM5) | _BV(PRTIM4) | _BV(PRTIM3) | _BV(PRUSART3);
    /* szingli LED-ek */
    STPLED(RED1);
    STPLED(GRN1);
    STPLED(RED2);
    STPLED(GRN2);
    STPLED(BLUE);
    /* K�t sz�n� LED-ek az RJ45 csatikon */
    STPLED(LA);
    LBL_DDR |= LBL_MSK;
    LBH_DDR |= LBH_MSK;
    /* All off */
    LEDOFF(RED1);
    LEDOFF(GRN1);
    LEDON(RED2);
    LEDOFF(GRN2);
    LEDOFF(BLUE);
    setBiLeds(0);
    
    /* Jumperek */
    STPJP(J0);
    STPJP(J1);
    STPJP(J2);
    STPJP(J3);
    STPJP(J4);
    STPJP(J5);
    STPJP(J6);
    STPJP(SCL);
    STPJP(SPA);
    STPJP(SPB);
    STPJP(SDA);
        /* �rz�kel� kapcsol�k */
    STPJP(S1);
    STPJP(S2);
    getJumpers();

    STPJP(SCK);
    STPJP(PDI);
    SETBIT(PDI);    // Felh�z�� ellen�ll�s be
    STPLED(PDO);    // Kimenet
    CLRBIT(PDO);    // PDO = L

}


/// Kontakt �rz�kel� m�rt �rt�k ki�rt�kel�se (nincs hihet�s�g vizsg�lat)
/// @param i A kontakt �rz�kel� sorsz�ma (0-15)
/// @return A ki�rt�kel�s eredm�nye ld.: enum eKStat
static inline char _kEval(uint8_t i)
{
    register uint8_t v1, v2;
    // Ha tiltva van az �rz�kel�
    if (config.disabled & _BV(i)) return KDISA;
    v2 = kVal1[i];
    v1 = kVal2[i];
    // Ha nincs m�rt �rt�k
    if (v1 == 0 || v2 == 0) return KNULL;
    // Ha mindk�t �rt�k 2V �s 3V k�z�tt akkor OK.
    if (AD2V < v1 && AD3V > v1 && AD2V < v2 && AD3V > v2) return KOK;
    // Ha v1 1V alatt, �s v2 4V felett, akkor r�vidz�r
    if (AD1V > v1 && AD4V < v2) return KSHORT;
    // Ha v2 1V alatt, �s v1 4V felett, akkor a vezet�k szakadt vagy elv�gt�k
    if (AD1V > v2 && AD4V < v1) return KCUT;
    // Ha v1 �rt�k 2V �s 3V k�z�tt, v1 4V felett, akkor az �rz�kel� kapcsol� nem z�rt
    if (AD1V > v2 && AD2V < v1 && AD3V > v1) return config.rev & _BV(i) ? KALERT : KINV;
    // Ha v2 �rt�k 2V �s 3V k�z�tt, v1 4V felett, akkor az �rz�kel� kapcsol� nem z�rt, ford�tott bek�t�s eset�n
    if (AD4V < v1 && AD2V < v2 && AD3V > v2) return config.rev & _BV(i) ? KINV : KALERT;
    // Nem �rt�kelhet�
    return KANY;
}

/// Egym�s ut�n ennyi egyforma status kell, hogy elhiggy�k
#define MINEQ  3
/// Ha egym�s ut�n ennyiszer nincs hihat� status, akkor az baj
#define MAXNE 15
/// Kontakt �rz�kel� m�rt �rt�k ki�rt�kel�se hihet�s�g vizsg�lattal
static inline char kEval(uint8_t i)
{
    static   char       prevStats[KNUM];
    static struct {
        unsigned cteq:3;    // Azonos �rt�kek sz�ml�l�ja
        unsigned ctfl:5;    // Nincs hihet� �rt�k sz�ml�l�
    }                   cnts[KNUM];
    register char       r  = _kEval(i);
    register uint8_t    ct;

    // Index ellen�rz�s, ha rosz program hiba
    if (i >= KNUM) {
        errstat.bits.perr = 1;
        return KERR;
    }

    if (prevStats[i] == r) {
        ct = cnts[i].cteq +1;
        if (MINEQ <= ct) {      // Az �rt�k hihet�
            cnts[i].ctfl = 0;
            if (config.autoSetRev && r == KINV) {    // Ford�tott bek�t�s �rz�kel�se !
                DEBP3("REV: A:%d, R:%c, I:%d", config.autoSetRev, r, i);
                invRevConf(i);
                r = prevStats[i] = KALERT;
            }
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

/* T0 IT 800Hz/1600Hz; */
ISR(TIMER0_COMPA_vect)
{
    // Maszk �rt�kek a LED ek kigy�jt�si �tem�re a f�nyer� be�ll�t�s�hoz.
    static uint16_t mskTab[] PROGMEM = { 0x0001, 0x0101, 0x1111, 0x5555, 0x5757, 0x7777, 0x7f7f, 0xFFFF };
    // Az aktu�lis maszkok a k�t sz�nhez.
    static uint16_t msk1, msk2;
    alive |= ALIVE_CTC0;
    // M�r�si f�zist l�ptetj�k
    ++kPhase;
    if (kPhase >= KPHASES) {
        kPhase = kLedPhase = kIxPhase = 0;
        msk1 = pgm_read_word_near(mskTab + config.light1);
        msk2 = pgm_read_word_near(mskTab + config.light2);
        kMask = 1;
    }
    else {
        kIxPhase   = kPhase >> 1;		// �rz�kel� indexe
        kLedPhase  = kPhase & 1;		// melyik LED �g
        if (kLedPhase == 0) kMask <<= 1;
    }
    if (kLedPhase == 0) {	/* 1-es LED-ek */
        SBL_POU = SBH_POU = 0xff;
        SAL_POU = ~( msk1 & kLedSt.kLed1);
        SAH_POU = ~((msk1 & kLedSt.kLed1) >> 8);
        if (0 == (kLedSt.kLed1 & kMask)) {	/* Ha nem �g a led ki kell gyujtani a m�r�s idej�re */
            if (kIxPhase < 8) SAL_POU &= ~((uint8_t) kMask);
            else              SAH_POU &= ~((uint8_t)(kMask >> 8));
        }
    }
    else {		/* 2-es LED-ek */
        SBL_POU = SBH_POU = 0;
        SAL_POU = msk2 & kLedSt.kLed2;
        SAH_POU =(msk2 & kLedSt.kLed2) >> 8;
        if (0 == (kLedSt.kLed2 & kMask)) {	/* Ha nem �g a led ki kell gyujtani a m�r�s idej�re */
            if (kIxPhase < 8) SAL_POU |= (uint8_t) kMask;
            else              SAH_POU |= (uint8_t)(kMask >> 8);
        }
        msk1 = (msk1 << 1) | (msk1 >> 15);	// Rot�ljuk a maszkokat a k�v. m�r�s p�rhoz
        msk2 = (msk2 << 1) | (msk2 >> 15);
    }
    setADMux(kIxPhase);
    _delay_loop_1(20);  // kb 4usec v�rakoz�s
    ADCSRA  |= _BV(ADSC);   // Ind�tjuk az A/D k�nverzi�t
}

/// 100 Hz
ISR(TIMER2_COMPA_vect, ISR_NOBLOCK)
{
    static uint8_t div = 1;
    static int8_t  ph  = 0;
    alive |= ALIVE_CTC2;
    // K�tsz�n� LED-ek hogyan vil�g�tsanak...
    if (isOff.bits.ad) {
        setBiLeds(div);
//      PRNC(div ? '.' : 8);
        div = !div;
    }
    else if (isOff.bits.diag == 0) {
        // A k�t sz�n� LED-ek m�k�dtet�se
        if (--div == 0) {
            uint16_t    red;
            uint16_t    grn;
            uint8_t     m;
            biLeds1 = biLeds2 = 0;
            switch (++ph) {
                case 1:
                    red = kLedSt.kLedRd;
                    grn = kLedSt.kLedGr;
                    for (m = 1; m != 0; red >>= 1, grn >>= 1, m <<= 1) {
                        biLeds1 |= m & (uint8_t)red;
                        biLeds2 |= m & (uint8_t)grn;
                    }
                    // S1
                    if (isNoAlertNow(sStat[1])) LEDON(GRN1);
                    else                       LEDOFF(GRN1);
                    if (isAlert(sStat[1]))      LEDON(RED1);
                    else                       LEDOFF(RED1);
                    div = 150;
                    break;
                case 2:
                    div = 1;
                    break;
                case 3:
                    red = kLedSt.kLedRd >> 1;
                    grn = kLedSt.kLedGr >> 1;
                    for (m = 1; m != 0; red >>= 1, grn >>= 1, m <<= 1) {
                        biLeds1 |= m & (uint8_t)red;
                        biLeds2 |= m & (uint8_t)grn;
                    }
                    // S2
                    if (isNoAlertNow(sStat[2])) LEDON(GRN1);
                    else                       LEDOFF(GRN1);
                    if (isAlert(sStat[2]))      LEDON(RED1);
                    else                       LEDOFF(RED1);
                    div = 150;
                    break;
                case 4:
                    div = 5;
                    ph  =  0;
                    break;
            }
        }
    }
    if (--div1s == 0) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            ++cnt1s;
            autoConfClk();
        }
        div1s = 100;
        // Ha 2 percen kereszt�l nincs kommunik�ci�, akkor reset
        if (++silentCt > 120 && isOff.bits.silent == 0) reset();
    }
    // Ha tul r�g�ta vagyunk ad�sban, akkor letiltjuk az ad�st.
    if (oeCt > 0 && --oeCt == 0) {
        rs485_setrec();
        PRNP0("OE TO!!!\n");
        errstat.bits.oeto = 1;
    }
    // Kapcsol�k, SPI/Debug port v�delme.
    if (isOff.bits.diag == 0){
        // Kapcsol�k
        sVal.bits.bit1 = GETJP(S1);
        sVal.bits.bit2 = GETJP(S2);
        sVal.bits.bit3 = GETJP(J1); // J0 kimarad !!!
        sVal.bits.bit4 = GETJP(J2);
        sVal.bits.bit5 = GETJP(J3);
        sVal.bits.bit6 = GETJP(J4);
        sVal.bits.bit7 = GETJP(J5);
        sVal.byte ^= config.s_rev;
        // A PDO �s SCK k�z�tt r�vidz�rnak kell lennie !!! (nincs polarit�s bit)
        STPJP(SCK);
        SETBIT(PDO);
        sVal.bits.bit0 = GETBIT(SCK);
        CLRBIT(PDO);
        sVal.bits.bit0 &= GETJP(SCK);
        STPLED(SCK);        // Riaszt�s k�zvetlen kijelz�se
        if (sVal.bits.bit0) SETBIT(SCK);
        else                CLRBIT(SCK);
        sStat[0] = newSwStat(sVal.bits.bit0, sStat[0]);
        sStat[1] = config.s_disabled & _BV(1) ? KDISA : newSwStat(sVal.bits.bit1, sStat[1]);
        sStat[2] = config.s_disabled & _BV(2) ? KDISA : newSwStat(sVal.bits.bit2, sStat[2]);
        sStat[3] = config.s_disabled & _BV(3) ? KDISA : newSwStat(sVal.bits.bit3, sStat[3]);
        sStat[4] = config.s_disabled & _BV(4) ? KDISA : newSwStat(sVal.bits.bit4, sStat[4]);
        sStat[5] = config.s_disabled & _BV(5) ? KDISA : newSwStat(sVal.bits.bit5, sStat[5]);
        sStat[6] = config.s_disabled & _BV(6) ? KDISA : newSwStat(sVal.bits.bit6, sStat[6]);
        sStat[7] = config.s_disabled & _BV(7) ? KDISA : newSwStat(sVal.bits.bit7, sStat[7]);
    }
}

/* A/D konverter, Timer0 ut�n �t be, ugyanazzal a frekvenci�val */
ISR(SIG_ADC)
{
    static uint8_t	_ct = 0;
    uint8_t v  = ADCH;		/* M�rt �rt�k */
    if (kIxPhase >= KNUM) ERROR("ADC");
    alive |= ALIVE_ADC;
    if (v == 0) v = 1;      // A 0 azt jelenti nincs m�rt �rt�k, A ki�rt�kel�sn�l egy vagy nulla az mindegy.
    // Elrakjuk a m�rt �rt�ket
    // Ha a LED-eknek nem k�ne vil�g�tani, akkor azonnal ki kell kapcsolni
    if (kLedPhase == 0) {    // LED1 kikapcs
        kVal1[kIxPhase] = v;
        if (0 == (kLedSt.kLed1 & kMask)) {
            if (kIxPhase < 8) SAL_POU |= (uint8_t) kMask;
            else              SAH_POU |= (uint8_t)(kMask >> 8);
        }
    }
    else {
        kVal2[kIxPhase] = v;
        if (0 == (kLedSt.kLed2 & kMask)) {
            if (kIxPhase < 8) SAL_POU &= ~((uint8_t) kMask);
            else              SAH_POU &= ~((uint8_t)(kMask >> 8));
        }
        // Egy �rz�ke�re megvan mindk�t m�r�s, �ll�tjuk a statuszt is
        kStat[kIxPhase] = newStat(kEval(kIxPhase), kStat[kIxPhase]);
        if (isAlert(kStat[kIxPhase])) {
            kLedSt.kLedRd |= kMask;
            if (config.rev & kMask) kLedSt.kLed2 |= kMask; else kLedSt.kLed1 |= kMask;
        }
        else {
            kLedSt.kLedRd &= ~kMask;
            if (config.rev & kMask) kLedSt.kLed2 &=~kMask; else kLedSt.kLed1 &=~kMask;
        }
        if (isNoAlertNow(kStat[kIxPhase])) {
            kLedSt.kLedGr |= kMask;
            if (config.rev & kMask) kLedSt.kLed1 |= kMask; else kLedSt.kLed2 |= kMask;
        }
        else {
            kLedSt.kLedGr &= ~kMask;
            if (config.rev & kMask) kLedSt.kLed1 &=~kMask; else kLedSt.kLed2 &=~kMask;
        }

    }
    if (((++_ct & 7) == 0)) {	/* Az anal�g m�r�s ut�n, hogy azt ne zavarja (100Hz-el) ricogtatjuk a k�tsz�n� LED-ek meghalyt�s�t */
        setBiLeds(_ct & 8);
    }
    else if ((_ct & 7) > ((_ct & 8) ? config.lightc2 : config.lightc1)) {
        offBiLeds();
    }
}

static inline void answerMsg()
{
    uint8_t i;
    uint16_t ui16;
    uint16_t mm;
    uint8_t  ui8;
    uint8_t  m;
    // Kontakt �rz�kel�k
    memcpy(aMsg2.kontakt, (void *)kStat, KNUM); // A st�tuszok kim�sol�sa a v�lasz blokkba
    ui16 = hhhh2i(qMsg2.clrKontSt);     // Melyekn�l t�rlend� a riaszt�s �llapota
    for (i = 0, mm = 1; i < KNUM; i++, mm <<= 1) {
        if ((ui16 & mm) && isAlert(kStat[i])) kStat[i] = KNULL;
    }
    // Kapcsol�k
    memcpy(aMsg2.swstat, (void *)sStat, SNUM);
    ui8 = hh2i(qMsg2.clrSwSt);
    for (i = 0,  m = 1; i < SNUM; i++,  m <<= 1) {
        if ((ui8  &  m) && isAlert(sStat[i])) sStat[i] = KNULL;
    }
    // Hiba st�tusz
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ui16 = errstat.all;
        if (qMsg2.clrErrSt == 'T') errstat.all = 0;
    }
    i2hhhh(ui16, aMsg2.errstat);
    // Szamlal� "uptime"
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ui16 = cnt1s;
    }
    i2hhhh(ui16, aMsg2.counter);
}

/// delay �rt�kek a _delay_loop_2 makr�hoz
#define DELAY_2_4MS	14747U
#define DELAY_2_6MS	22121U
#define DELAY_2_8MS	29494U
#define DELAY_2_10MS	36968U

/// Az RS485 ad�s el�tt v�rakozunk 4mSec-et hogy a master tuti elengedje a buszt
#define RS485DELAY	_delay_loop_2(DELAY_2_4MS)

/// Vett�nk egy "sztenderd" �zenetet az rs485 buszon
void qMsgReceived()
{
    DEBP0("Q:"); DEBDMP(&qMsg2, sizeof(qMsg2));
    answerMsg();
    DEBP0(" A:"); DEBDMP(&aMsg2, sizeof(aMsg2)); DEBC('\n');
    RS485DELAY;
    rs485_write((char *)&aMsg2, sizeof(struct aMsg2));
}

void reset(void)
{
    LEDON(BLUE);
    wdt_disable();
    wdt_enable(WDTO_15MS);
    rs232_flush();
    LEDOFF(BLUE);
    LEDON(RED1);
    cli();
    while (1) {
        int i;
        LEDINV(GRN1);
        LEDINV(RED1);
        for (i = 0; i < 11; i++) _delay_loop_2(0);   // 56 ~= 1s
    }
}

void setupByMsg(void)
{
    if (cMsg2.disabled[0] == '!' && cMsg2.disabled[1] == '!') {
        DEBP0("!!RESET\n");
        reset();
    }
    DEBP0("setBM\n");
    config.disabled   = hhhh2i(cMsg2.disabled);
    config.s_disabled = hh2i(cMsg2.dis_sw);
    config.s_rev      = hh2i(cMsg2.rev_sw) & 0xfe;   // Az 0. bit inverzi�ja nem megengedett
    config.light1     = h2i(cMsg2.light[0]);
    config.light2     = h2i(cMsg2.light[1]);
    wrConfig();
}

/// Vett�nk egy be�ll�t� �zenetet az RS485 buszon
void cMsgReceived()
{
    DEBP0("C:"); DEBDMP(&cMsg2, sizeof(cMsg2));
    setupByMsg();
    // DEBP0(" A:"); DEBDMP(&aMsg, sizeof(aMsg)); DEBC('\n');
    RS485DELAY;
    rs485_putchar('#', NULL);   // A "!!" lecser�lj�k "##"-re
    rs485_putchar('#', NULL);
    rs485_write(((char *)&cMsg2) +2, sizeof(struct cMsg2) -2);
}

enum wMessage { MSG_WAIT = 0, MSG_QUERY = 1,  MSG_COMMAND = 2, MSG_TERM = 3 };
void loop_rs485()
{
    silentCt = 0;
    char ch = rs485_get();
    DEBCHR(ch);
    uint8_t ix = 0;
    static int8_t  sw = MSG_WAIT;
    switch (sw) {
        case MSG_WAIT:
            switch (ch) {
                case '?':
                    sw = MSG_QUERY;
                    pMsg = (char *)&qMsg2;
                    goto loop_rs485_next;
                case '!':
                    sw = MSG_COMMAND;
                    pMsg = (char *)&cMsg2;
                    goto loop_rs485_next;
                case '@':
                    sw = MSG_COMMAND;
                    pMsg = (char *)&tMsg;
                    goto loop_rs485_next;
                default:
                    goto loop_rs485_drop;
            }
        case MSG_QUERY:
            ix = pMsg - (char *)&qMsg2;
            if (pMsg == NULL || ix >= sizeof(struct qMsg2)) {
                PERR();
                fatal();
            }
            switch (ix) {
                case  1:    // Marker 2. byte : '?'
                    if (ch != '?') goto loop_rs485_err;
                    goto loop_rs485_next;
                case  2:    // ID fels� hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id >>   4) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nek�nk sz�l
                    goto loop_rs485_next;
                case  3:    // ID als� hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id & 0x0f) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nek�nk sz�l
                    goto loop_rs485_next;
                case 4:    // Hiba st�tusz t�rl�se (T/F)
                    if (!isbool(ch)) goto loop_rs485_err;
                    goto loop_rs485_sto;
                case 5: case 6: case  7: case  8: // �rz�kel�k hiba �llapot t�rl�se bitvektor hexa
                case 9: case 10:                  // Kapcsol�k hiba �llapot t�rl�se bitvektor hexa
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    goto loop_rs485_sto;
                case  11:    // Marker '\r'
                    if (ch != '\r') goto loop_rs485_err;
                    goto loop_rs485_next;
                case 12:    // Marker '\n'
                    if (ch != '\n') goto loop_rs485_err;
                    qMsgReceived();
                    goto loop_rs485_drop;
                default:
                    PERR();
                    fatal();
            }
            break;
        case MSG_COMMAND:
            ix = pMsg - (char *)&cMsg2;
            if (pMsg == NULL || ix >= sizeof(struct cMsg2)) {
                PERR();
                fatal();
            }
            switch (ix) {
                case  1:    // Marker 2. byte : '!'
                    if (ch != '!') goto loop_rs485_err;
                    goto loop_rs485_next;
                case  2:    // ID fels� hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id >>   4) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nek�nk sz�l
                    goto loop_rs485_sto;
                case  3:    // ID als� hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id & 0x0f) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nek�nk sz�l
                    goto loop_rs485_sto;
                case  4:    // Ha reset akkor felki�lt�jel, �s ld. a k�v.
                    if (isxdigit(ch) || ch == '!') goto loop_rs485_sto;
                    goto loop_rs485_err;
                case  5:    // Ha reset akkor felki�lt�jel, egy�bk�nt ld. a k�v.
                    if(pMsg[-1] == '!') {
                        if (ch == '!') {
                            *pMsg = ch;
                            pMsg = cMsg2.endMark;   // v�ge k�vetkezik
                            return;
                        }
                        goto loop_rs485_err;
                    }
                    if (isxdigit(ch)) goto loop_rs485_sto;
                    goto loop_rs485_err;
                case  6: case  7:       // Kontakt enged�lyezve/tiltva (HEXA)
                case  8: case  9:       // Kapcsol�k enged�ly (HEXA)
                case 10: case 11:       // Kapcsol�k polarit�s (HEXA)
                case 12: case 13:       // Kontakt �rz�kel� p�rok f�nyereje (HEXA)
                case 14: case 15:       // Csati LED-ek f�nyereje (HEXA)
                    if (isxdigit(ch)) goto loop_rs485_sto;
                    goto loop_rs485_err;
                    if (ch >= '0' && ch <= '3') goto loop_rs485_sto;
                    goto loop_rs485_err;

                case 16:    // Marker '\r'
                    if (ch != '\r') goto loop_rs485_err;
                    goto loop_rs485_next;
                case 17:    // Marker '\n'
                    if (ch != '\n') goto loop_rs485_err;
                    cMsgReceived();
                    goto loop_rs485_drop;
                default:
                    PERR();
                    fatal();
            }
            break;
    case MSG_TERM:
        ix = pMsg - (char *)&tMsg;
        if (pMsg == NULL || ix >= sizeof(struct tMsg)) {
            PERR();
            fatal();
        }
        // ...........
        goto loop_rs485_err;
    }
    PERR();
    fatal();
loop_rs485_err:
    DEBP1("EBBM%d\n", ix);
loop_rs485_drop:
    DEBC('^');
    sw = MSG_WAIT;
    pMsg = NULL;
    return;
loop_rs485_sto:
    *pMsg = ch;
loop_rs485_next:
    pMsg++;
    return;

}

DEFBUFF(lin, 128)
extern int yyparse();
void loop_rs232()
{
    silentCt = 0;
    char ch = rs232_getchar(NULL);
    if (ch == '\n' || ch == '\r') {
        if (linlen > 0) {
            rs232_putchar('\n', NULL); // Echo
            linpush(0);
            if (errstat.bits.linbuffoverfl) {
                fputs_P(PSTR("OVF\n"), &rs232_str);
                errstat.bits.linbuffoverfl = 0;
            }
            else {
                yyparse();
            }
            linbas = linlen = 0;    // Sor puffer kinull�z�sa.
        }
    }
    else {
        linpush(ch);
        rs232_putchar(ch, NULL); // Echo
        if (errstat.bits.linbuffoverfl) {
            fputs_P(PSTR("OVF\n"), &rs232_str);
            errstat.bits.linbuffoverfl = 0;
            linbas = linlen = 0;    // Sor puffer kinull�z�sa.
        }
    }
}

void loop_t()
{
    static uint16_t last = 0;
    unsigned char st = OWI_StateMachine();
    if (st == OWI_STATE_CONVERTED_T) {
        las = cnt1s;
        tmp = temperature >= 255 ? 254 : temperature < 0 ? 0 : (uint8_t)temperature;
    }
    else if (last > cnt1s) {    // Ha esetleg tulcsordult a sz�ml�l�
        last = cnt1s;
    }
    else if (last + 10 < cnt1s) {
        tmp = 255;  // Invalid
    }
}

void loopf(void)
{
    if ((alive & ALIVE_ALL) == ALIVE_ALL) {
        wdt_reset();
        alive &= ~ALIVE_ALL;
    }
    if (rxnlen) loop_rs485();
    if (rxdlen) loop_rs232();
    loop_conf();
    loop_t();
}

void fatal()
{
    DEBP0("Fatal...\n");
    LEDOFF(GRN1); LEDOFF(GRN2);         // Z�ld off
    LEDON(RED1);  LEDOFF(RED2);
    LEDOFF(BLUE);
    rs232_flush();
    cli();
    while (1) {         // Fat�lis hiba, le�llunk, piros LED gyorsan villog
        int i;
        LEDINV(RED1);
        LEDINV(RED2);
        for (i = 0; i < 11; i++) _delay_loop_2(0);   // 56 ~= 1s
    }
}

void stop()
{
    DEBP0("Stop...\n");
    LEDOFF(RED1); LEDOFF(RED2);     // Piros off
    LEDON(GRN1);  LEDOFF(GRN2);
    LEDOFF(BLUE);
    rs232_flush();
    cli();
    while (1) {     // K�sz, le�llunk, z�ld LED gyorsan villog
        int i;
        LEDINV(GRN1);
        LEDINV(GRN2);
        for (i = 0; i < 11; i++) _delay_loop_2(0);   // 56 ~= 1s
    }
}

void main(void)
{
    errstat.all = 0;
    if (MCUSR & _BV(WDRF)) errstat.bits.wdt_occur = 1;
    if (MCUSR & _BV(BORF)) errstat.bits.bor_occur = 1;
    if (MCUSR & _BV(EXTRF))errstat.bits.exr_occur = 1;
    if (MCUSR & _BV(PORF)) errstat.bits.pon_occur = 1;
    isOff.all = 0;
    MCUSR = 0;
    memset((void *)kStat, 0, KNUM); // A 0 jelent�se: els� m�r�st eldobjuk, �s KNULL az eredm�ny st�tusz
    memset((void *)sStat, 0, SNUM); // A 0 jelent�se: els� m�r�st eldobjuk, �s KNULL az eredm�ny st�tusz
    prtinit();  // IO portok (LED, Jumper) inicializ�l�sa, Jumperek beolvas�sa
    rdConfig();         // Konfig az EEPROM-b�l
    autoConfInit();     // Konfig aut�matikus ki�r�s�t vez�rl� id�z�t� sz�ml�l� be�ll�t�sa
    rs232_init();       // RS232 (debug port) inicializ�l�sa
    t2init();           // T2 CTC inicializ�l�sa 100Hz
    sei();              // Van IT
    LEDON(RED2);
    // (C) �zenet -> RS232
    if (unConfigured()) { PRNS(version1); }
    else {               _PRNP1(version1m, config.id); }
    PRNS(version2); PRNS((prog_char *)&vtm);
    PRNS(version3);
    // Aktu�lis konfig -> RS232
    prnConf();
#if DEBUG
    prnJumpers();
#endif
    if (errstat.bits.wdt_occur == 0) {
        setup();    // Felt�teles setup a jumperekkel, vagy diagnosztika
    }
    LEDOFF(RED2);
    LEDON(GRN2);
    if (unConfigured())     ERROR("UNCNF");
    adinit();           // A/D konverter init
    t0init();           // Kontakt �rz�kel�ket kezel� 800Hz/1600Hz-es IT
    i2hh(config. id, aMsg2.sMaster); // Az el�k�sz�tett v�lasz �zenetbe be�rjuk a saj�t c�met, ami eddig �res volt.
    rs485_init();
    // Watchdogot is inicializ�ljuk
    alive = 0;
    wdt_enable(WDTO_1S);
    DEBP0("Loop...\n");
    while(1) loopf();   // A polloz� ciklusunk
}

