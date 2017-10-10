
#include "indalarmif1.h"
#include <util/atomic.h>


const char version1[]  PROGMEM =    "IndAlarmIf1 ???\n";
const char version1m[] PROGMEM = 	"IndAlarmIf1 Master %02d\n";
const char version1s[] PROGMEM =    "IndAlarmIf1 Slave %02d\n";
const char version2[]  PROGMEM = 	"V:1.3 ";
struct vtm vtm PROGMEM = { __DATE__, ' ', __TIME__, "\n" };
const char version3[]  PROGMEM = 	"(c)Indasoft bt.\n";

union jumpers jumpers;
union kLedSt kLedSt;
struct kLight kLight = { 2, 2 }; // Kontakt érzékelő LED-jei közepes fényerőn
uint8_t kVal1[KNUM] = { 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t kVal2[KNUM] = { 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t alive = 0;
uint16_t cnt1s = 0;
uint8_t  silentCt = 0;
uint8_t  oeCt = 0;

/// Query message receive buffer
struct qMsg     qMsg = { "??", "00", "00", 'F', "00", "\r\n"};
/// Answer message send buffer
struct aMsg     aMsg = { "..", "00", "00", "00000000", "0000", "0000", "\r\n" };
/// Answer messgae temporary (receive from TWI and transfer to RS485)
struct aMsg     aMsgTmp;
char * pQMsg = (char *)&qMsg;
char * pAMsg = NULL;
char * pAMsgTmp = NULL;
struct cMsg     cMsg = { "!!", "00", "00", "EEEEEEEE", "33", "DD", "\r\n" };
char * pCMsg = (char *)&cMsg;
char twiRecBuff[32];

union errstat   errstat;
uint8_t  kStat[KNUM] = { KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL };

struct {
    unsigned    receive:1;
    unsigned    query:1;
    unsigned    answer:1;
    unsigned    command:1;
}   s = { 0,0,0,0 };

void setupByMsg(void);

/// @def STPJP(JP)
/// Setup jumper port bit: Bit is input, and pull up resistor is on.
/// @param JP Jumper name
#define STPJP(JP)   JP ## _DDR &= ~_BV(JP ## _BIT); JP ## _POU |=  _BV(JP ## _BIT)
/// @def STPLED(L)
/// Setup LED port bit: Bit is output
/// @param L LED name
#define STPLED(L)   L ## _DDR |=  _BV(L ## _BIT)

static inline void getJumpers()
{
    jumpers.bits.jp3 = GETJP(J3);
    jumpers.bits.jp4 = GETJP(J4);
    jumpers.bits.sck = GETJP(SC);
    jumpers.bits.miso= GETJP(MI);
    jumpers.bits.mosi= GETJP(MO);
}

static inline void prtinit(void)
{
    // Alap init
    MCUCR = _BV(JTD);    // PUD=0 (felhúzó ellenállások) engedélyezve)
    MCUCR = _BV(JTD);    // JTAG tiltva (tiltható FUSE-ból is, de semmiképpen nem kell.)
    PRR   = _BV(PRTIM1) | _BV(PRSPI);   // TIM1 and SPI power down
    /* LED-ek */
    STPLED(L1);
    STPLED(L2);
    LEDOFF(L1);
    LEDOFF(L2);
    /* Jumperek */
    STPJP(J3);
    STPJP(J4);
    STPJP(SC);
    STPJP(MI);
    STPJP(MO);
    getJumpers();
    /* Érzékelő kapcsolók */
    STPJP(S1);
    STPJP(S2);
}

static inline void t0init()
{
    DEBP0("I:T0\n");
    /* Timer0: 400Hz IT */
    TCNT0 = 0;
    TCCR0A = _BV(WGM01);            // CTC mód
    TCCR0B = _BV(CS02) | _BV(CS00); // 14745600/1024 =  14.4kHz
    OCR0A  = 36;                    //    14400/  36 = 400 Hz
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
    DEBP0("I:A/D\n");
    /* AD konverter */
    memset(kVal1, 0, KNUM);
    memset(kVal2, 0, KNUM);
    ADMUX  = _BV(REFS0) | _BV(ADLAR);    // Referencia a tápfesz, eredmény balra igazítva (8bit)
    // AD konverter engedélyezve, it is, előosztó 64 (230.4 kHz)
    // A 230kH es órajel, nagyobb mint a javasolt max 200kHz, de csak 8 bites konverzió lessz)
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS1) | _BV(ADPS2);
//    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);  // Óra 115.2kHz
    DIDR0  = 0xff;  // Digital input disable
    /* Kontakt érzékelőket tápláló port kimenetként állítása, H kimeneti szint mindegyikre */
    KOA_DDR |= KOA_BITS;
    KOB_DDR |= KOB_BITS;
    KOA_POU |= KOA_BITS;
    KOB_POU |= KOB_BITS ;
}

/* T0 IT 400Hz; */
ISR(TIMER0_COMPA_vect)
{
    static int8_t cnt = 0;
    alive |= ALIVE_CTC0;
    cnt = (cnt + 1) & 7;    // 8 fázisunk van;
    // 1-es LED csökkentett fényerő esetén (kLight1 < 3) a LED kikapcsolása (A=B=H)
    if (kLight.l1 < 3) switch (cnt - kLight.l1){
        case 1:     SETK(B, 01);    break;
        case 2:     SETK(B, 23);    break;
        case 3:     SETK(B, 45);    break;
        case 4:     SETK(B, 67);    break;
    }
    // 2es LED csökkentett fényerő esetén (kLight2 < 3) a LED kikapcsolása
    if (kLight.l2 < 3) switch ((cnt - kLight.l2) & 7) {
        case 5:     SETK(A, 01);    break;
        case 6:     SETK(A, 23);    break;
        case 7:     SETK(A, 45);    break;
        case 0:     SETK(A, 67);    break;
    }
    SETADMUX((cnt % 4) *2); // A/D csatorna beállítás (érzékelő pár első elemére)
    switch (cnt) {
        // 1-es led bekapcsolása érzékelő páronként A=H, B=L
        case 0:     SETK(A, 01);    CLRK(B, 01);    break;
        case 1:     SETK(A, 23);    CLRK(B, 23);    break;
        case 2:     SETK(A, 45);    CLRK(B, 45);    break;
        case 3:     SETK(A, 67);    CLRK(B, 67);    break;
        // Kigyujtjuk a másik led-et A=L, B=H
        case 4:     CLRK(A, 01);    SETK(B, 01);    break;
        case 5:     CLRK(A, 23);    SETK(B, 23);    break;
        case 6:     CLRK(A, 45);    SETK(B, 45);    break;
        case 7:     CLRK(A, 67);    SETK(B, 67);    break;
    }
    kLight.phase = cnt > 3; // Az 1-es vagy 2-es LED-ek kigyújtása történt
    _delay_loop_1(20);  // kb 4usec várakozás
    ADCSRA  |= _BV(ADSC);   // Indítjuk az A/D könverziót
}


/// 100 Hz
ISR(TIMER2_COMPA_vect, ISR_NOBLOCK)
{
    static uint8_t div = 1;
    static uint8_t ph  = 0;
    static uint8_t div1s = 100;
    alive |= ALIVE_CTC2;
    if (--div == 0) {
        if (ph & 1) {   // "Elválasztó 0.01sec
            LEDINV(L1);
            LEDINV(L2);
            div = 1;
        }
        else {
            uint8_t i = ph/2;
            while(1) {      // Átugorjuk azokat a fázisokat, amik tiltottak
                switch (i) {
                  case 0: case 1: case 2: case 3: case 4:case 5:case 6:case 7:
                    if (config.disabled.k & _BV(i)) { ++i, ph +=2; continue; }
                    break;
                  case 8:
                    if (config.disabled.s1) { ++i, ph +=2; continue; }
                    break;
                  case 9:
                    if (config.disabled.s2) { ++i, ph +=2; continue; }
                }
                break;
            }
            switch (i) {
              case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                i = kStat[i];
                div = 60;
                break;
              case 8:
                i = (errstat.bits.s1 |= SEVAL(1)) ? KALERT : KOK;
                div = 100;
                break;
              case 9:
                i = (errstat.bits.s2 |= SEVAL(2)) ? KALERT : KOK;
                div = 100;
                break;
              case 10:
                div = 200;
                i = 0;
                break;
            }
            if (i == 0) {
                LEDOFF(L1);
                LEDOFF(L2);
            }
            else if (isAlert(i)) {
                LEDON(L1);
                if (i & KOLD) LEDON(L2);
                else          LEDOFF(L2);
            }
            else {
                LEDOFF(L1);
                if (i == KOK) LEDON(L2);
                else          LEDOFF(L2);
            }
        }
        if (++ph > 20) ph = 0;
    }
    if (--div1s == 0) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            ++cnt1s;
        }
        div1s = 100;
	// Ha 2 percen keresztül nincs kommunikáció, akkor reset
	if (++silentCt > 120) reset();
    }
    // Ha tul régóta vagyunk adásban, akkor letiltjuk az adást.
    if (oeCt > 0 && --oeCt == 0) OE_POU &=~_BV(OE_BIT);
}
/* A/D konverter */
ISR(SIG_ADC)
{
    uint8_t ch = GETADMUX();
    uint8_t v  = ADCH;
    if (ch > 7) ERROR("ADC");
    alive |= ALIVE_ADC;
    if (v == 0) v = 1;      // A 0 azt jelenti nincs mért érték, A kiértékelésnél egy vagy nulla az mindegy.
    // Elrakjuk a mért értéket
    if (kLight.phase == 0)  kVal1[ch] = v;
    else                    kVal2[ch] = v;
    if ((ch & 1) == 0) {        // Az érzékelő párnak csak az egyik csatornája lett lemérve
        SETADMUX(ch+1);         // A másikat is le kell mérni
        _delay_loop_1(21);      // kb 4usec várakozás
        ADCSRA  |= _BV(ADSC);   // Indítjuk az A/D könverziót
    }
    else {                  // Mindkettő lemérve
        // Ha a LED-eknek nem kéne világítani, akkor azonnal ki kell kapcsolni
        if (kLight.phase == 0) {    // LED1 kikapcs
            switch (ch) {
              case 1: if ((kLedSt.bits.k01 & KLED1) == 0) SETK(B, 01);    break;
              case 3: if ((kLedSt.bits.k23 & KLED1) == 0) SETK(B, 23);    break;
              case 5: if ((kLedSt.bits.k45 & KLED1) == 0) SETK(B, 45);    break;
              case 7: if ((kLedSt.bits.k67 & KLED1) == 0) SETK(B, 67);    break;
            }
        }
        else {  // LED2 kikapcs...
            // Egy érzékeő párra megvan mindkét mérés, állítjuk a statuszt is
            switch (ch) {
              case 1:
                if ((kLedSt.bits.k01 & KLED2) == 0) SETK(A, 01);
                kStat[0] = newStat(kEval(0), kStat[0]);
                kStat[1] = newStat(kEval(1), kStat[1]);
                if (isAlert(kStat[0]) || isAlert(kStat[1])) kLedSt.bits.k01 = KLED1 | KLED2;
                else kLedSt.bits.k01 = config.disabled.rev & 1 ? KLED1 : KLED2;
                break;
              case 3:
                if ((kLedSt.bits.k23 & KLED2) == 0) SETK(A, 23);
                kStat[2] = newStat(kEval(2), kStat[2]);
                kStat[3] = newStat(kEval(3), kStat[3]);
                if (isAlert(kStat[2]) || isAlert(kStat[3])) kLedSt.bits.k23 = KLED1 | KLED2;
                else kLedSt.bits.k23 = config.disabled.rev & 2 ? KLED1 : KLED2;
                break;
              case 5:
                if ((kLedSt.bits.k45 & KLED2) == 0) SETK(A, 45);
                kStat[4] = newStat(kEval(4), kStat[4]);
                kStat[5] = newStat(kEval(5), kStat[5]);
                if (isAlert(kStat[4]) || isAlert(kStat[5])) kLedSt.bits.k45 = KLED1 | KLED2;
                else kLedSt.bits.k45 = config.disabled.rev & 4 ? KLED1 : KLED2;
                break;
              case 7:
                if ((kLedSt.bits.k67 & KLED2) == 0) SETK(A, 67);
                kStat[6] = newStat(kEval(6), kStat[6]);
                kStat[7] = newStat(kEval(7), kStat[7]);
                if (isAlert(kStat[6]) || isAlert(kStat[7])) kLedSt.bits.k67 = KLED1 | KLED2;
                else kLedSt.bits.k67 = config.disabled.rev & 8 ? KLED1 : KLED2;
                break;
            }
        }
    }
}

static inline void answerMsg()
{
//    uint8_t m = h2i(qMsg.clrKontSt[0]) + 16 + h2i(qMsg.clrKontSt[1]);
    uint8_t i, m;
    uint16_t ui16;
    memcpy(aMsg.kontakt, kStat, KNUM);
    m = hh2i(qMsg.clrKontSt);
    for (i = 0; i< KNUM; i++) if (m & _BV(i) && isAlert(kStat[i])) kStat[i] = KNULL;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ui16 = errstat.all;
        if (qMsg.clrErrSt == 'T') errstat.all = 0;
    }
    i2hhhh(ui16, aMsg.errstat);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ui16 = cnt1s;
    }
    i2hhhh(ui16, aMsg.counter);
}

static void twiAnswer(void)
{
    if (s.receive == 0) {
        DEBP0("TWI<!"); DEBC('\n');
    }
    else if (twiRecBuff[0] == '?') {
        s.answer = 1;
        memcpy((void *)&qMsg, twiRecBuff, sizeof(qMsg));
        answerMsg();
    }
    else if (twiRecBuff[0] == '!') {
        s.command = 1;
        memcpy((void *)&cMsg, twiRecBuff, sizeof(cMsg));
        setupByMsg();
    }
    else {
        DEBP0("TWI<?"); DEBC('\n');
    }
}
#define MAXERRCNT 4
uint8_t eventFromTwi(uint8_t et)
{
    static uint8_t eCnt = 0;
    if (et & TWI_ERROR) {
        eCnt++;
        DEBP2("E(%x:%d)", et, eCnt);
        if (MAXERRCNT < eCnt) {
            eCnt = 0;
            return TWI_ERROR_RESULT;
        }
        return TWI_OK_RESULT;
    }
    eCnt = 0;
    DEBP1("e(%x)", et);
    switch (et) {
        case TWI_SLAVE_RECEIVE:
            s.receive = 1;
            s.answer = s.command = 0;
            return twiSlaveSetRecBuff(twiRecBuff, sizeof(twiRecBuff));
        case TWI_SLAVE_RECEIVED:
            twiAnswer();
            break;
        case TWI_SLAVE_SEND:
            if (s.receive && !(s.answer || s.command)) twiAnswer();
            if (s.answer) {
                s.receive = s.answer = s.command = 0;
                return twiSlaveSetSendBuff((char *)&aMsg, sizeof(struct aMsg));
            }
            if (s.command) {
                s.receive = s.answer = s.command = 0;
                twiRecBuff[0] = twiRecBuff[1] = '#';
                return twiSlaveSetSendBuff(twiRecBuff, sizeof(struct cMsg));
            }
            s.receive = s.answer = s.command = 0;
            DEBP0("TWI>?"); DEBC('\n');
            break;
        case TWI_SLAVE_SENDED:
            break;
        case TWI_MASTER_SEND:
            break;
        case TWI_MASTER_SENDED:
            break;
        case TWI_MASTER_RECEIVE:
            break;
        case TWI_MASTER_RECEIVED:
            if (twi.rBuff == (char *)&aMsgTmp) {     // válasz egy qMsg-re
                i2hh(config.id, aMsgTmp.sMaster);   // A Master ID-t a Slave nem küldi
                rs485_write((char *)&aMsgTmp, sizeof(aMsgTmp));
                DEBP0("TWI>RS485a:"); DEBDMP(&aMsgTmp, sizeof(aMsgTmp)); DEBC('\n');
            }
            else if (twi.rBuff == (char *)&cMsg) {   // Válasz egy cMsg-re
                i2hh(config.id, cMsg.tMaster);   // A Master ID-t a Slave nem küldi
                rs485_write((char *)&cMsg, sizeof(cMsg));
                DEBP0("TWI>RS485c:"); DEBDMP(&cMsg, sizeof(cMsg)); DEBC('\n');
            }
            else {
                DEBP0("TWI:PRGE"); DEBC('\n');
            }
            break;
    }
    return TWI_OK_RESULT;
}

/// delay értékek a _delay_loop_2 makróhoz
#define DELAY_2_4MS	14747U
#define DELAY_2_6MS	22121U
#define DELAY_2_8MS	29494U
#define DELAY_2_10MS	36968U

/// Az RS485 adás elött várakozunk 6mSec-et hogy a master tuti elengedje a buszt
/// Ez a konstans a _delay_loop_2 makróhoz
#define RS485DELAY	DELAY_2_4MS

/// Vettünk egy "sztenderd" üzenetet az rs485 buszon
void qMsgReceived()
{
    if (qMsg.tSlave[0] == '0' && qMsg.tSlave[1] == '0') {   // Ha ezt tőlünk kérdezik
        DEBP0("Q:"); DEBDMP(&qMsg, sizeof(qMsg));
        answerMsg();
        DEBP0(" A:"); DEBDMP(&aMsg, sizeof(aMsg)); DEBC('\n');
	_delay_loop_2(RS485DELAY);
        rs485_write((char *)&aMsg, sizeof(struct aMsg));
    }
    else {                                                  // Ezt csak továbbítani kell
        i2hh(config.id, qMsg.tMaster);  // A Master ID-t nem írtuk a bufferbe
        twi.sla = hh2i(qMsg.tSlave) << 1;
        DEBP1(">%d:", twi.sla); DEBDMP(&qMsg, sizeof(qMsg)); DEBC('\n');
        twiMasterRead(twi.sla,                                  // Slave address
                      (char *)&qMsg,    sizeof(struct qMsg),    // Adó buffer
                      (char *)&aMsgTmp, sizeof(struct aMsg),    // Vevő buffer
                      eventFromTwi);                            // Event function
    }
}

void reset(void)
{
    wdt_disable();
    wdt_enable(WDTO_15MS);
    cli();
    for (;;);
}

void setupByMsg(void)
{
    int8_t i;
    if (cMsg.kontakt[0] == '!' && cMsg.kontakt[1] == '!') {
        reset();
    }
    DEBP0("setBM\n");
    config.disabled.k   = 0;
    config.disabled.rev = 0;
    for (i = 0; i < 8; i++) {
        if (cMsg.kontakt[i] == KDISA) config.disabled.k   |= 1 << i;
        if (cMsg.kontakt[i] == KREV)  config.disabled.rev |= 1 << (i/2);
    }
    config.light.k1 = cMsg.light[0] - '0';
    config.light.k2 = cMsg.light[1] - '0';
    config.disabled.s1 = cMsg.swdet[0] == 'D';
    config.disabled.s2 = cMsg.swdet[1] == 'D';
    wrConfig();
}

/// Vettünk egy beállító üzenetet az RS485 buszon
void cMsgReceived()
{
    i2hh(config.id, cMsg.tMaster);      // A Master ID-t nem írtuk a bufferbe
    if (cMsg.tSlave[0] == '0' && cMsg.tSlave[1] == '0') {   // Ha ezt tőlünk kérdezik
        DEBP0("C:"); DEBDMP(&cMsg, sizeof(cMsg));
        setupByMsg();
        // DEBP0(" A:"); DEBDMP(&aMsg, sizeof(aMsg)); DEBC('\n');
	_delay_loop_2(RS485DELAY);
        rs485_putchar('#', NULL);   // A "!!" lecseréljük "##"-re
        rs485_putchar('#', NULL);
        rs485_write(((char *)&cMsg) +2, sizeof(struct cMsg) -2);
    }
    else {                                                  // Ezt csak továbbítani kell
        cMsg.beginMark[0] = cMsg.beginMark[1] = '!'; // megváltozik a válasznál
        twi.sla = hh2i(cMsg.tSlave) << 1;
        DEBP1(">%d:", twi.sla); DEBDMP(&cMsg, sizeof(cMsg)); DEBC('\n');
        if (cMsg.kontakt[0] == '!') {           // Ha reset, akkor nem várunk választ
            twiMasterWrite(twi.sla,                               // Slave address
                        (char *)&cMsg, sizeof(struct cMsg),       // Adó buffer
                        eventFromTwi);                            // Event function

        }
        else {
            twiMasterRead(twi.sla,                                // Slave address
                        (char *)&cMsg, sizeof(struct cMsg),       // Adó buffer
                        (char *)&cMsg, sizeof(struct cMsg),       // Vevő buffer
                        eventFromTwi);                            // Event function
        }
    }
}

enum wMessage { MSG_WAIT = 0, MSG_QUERY = 1,  MSG_COMMAND = 2 };
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
                    pQMsg = (char *)&qMsg;
                    goto loop_rs485_next;
                case '!':
                    sw = MSG_COMMAND;
                    pCMsg = (char *)&cMsg;
                    goto loop_rs485_cmd_next;
                default:
                    goto loop_rs485_drop;
            }
        case MSG_QUERY:
            ix = pQMsg - (char *)&qMsg;
            switch (ix) {
                case  1:    // Marker 2. byte : '?'
                    if (ch != '?') goto loop_rs485_err;
                    goto loop_rs485_next;
                case  2:    // ID felső hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id >>   4) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nekünk szól
                    goto loop_rs485_next;
                case  3:    // ID alsó hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id & 0x0f) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nekünk szól
                    goto loop_rs485_next;
                case  4:    // Slave ID felső hexa karakter
                case  5:    // Slave ID alsó hexa karakter
                case  7:    // Érzékelők hiba állapot törlése bitvektor felső hexa
                case  8:    // Érzékelők hiba állapot törlése bitvektor alsó hexa
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    goto loop_rs485_sto;
                case 6:    // Hiba státusz törlése (T/F)
                    if (!isbool(ch)) goto loop_rs485_err;
                    goto loop_rs485_sto;
                case  9:    // Marker '\r'
                    if (ch != '\r') goto loop_rs485_err;
                    goto loop_rs485_next;
                case 10:    // Marker '\n'
                    if (ch != '\n') goto loop_rs485_err;
                    qMsgReceived();
                    goto loop_rs485_drop;
                default:
                    goto loop_rs485_err;
            }
            loop_rs485_sto:
            *pQMsg = ch;
            loop_rs485_next:
            pQMsg++;
            return;
        case MSG_COMMAND:
            ix = pCMsg - (char *)&cMsg;
            switch (ix) {
                case  1:    // Marker 2. byte : '!'
                    if (ch != '!') goto loop_rs485_err;
                    goto loop_rs485_cmd_next;
                case  2:    // ID felső hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id >>   4) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nekünk szól
                    goto loop_rs485_cmd_next;
                case  3:    // ID alsó hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    if ((config.id & 0x0f) != h2i(ch)) goto loop_rs485_drop;  // Ez nem nekünk szól
                    goto loop_rs485_cmd_next;
                case  4:    // Slave ID felső hexa karakter
                case  5:    // Slave ID alsó hexa karakter
                    if (!isxdigit(ch)) goto loop_rs485_err;
                    goto loop_rs485_cmd_sto;
                case 6:    // Kontakt engedélyezve/tiltva/fordítva bekötve
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                    if (ch != 'E' && ch != 'D' && ch != 'R' && ch != '!') goto loop_rs485_err;
                    goto loop_rs485_cmd_sto;
                case 14:    // Kontakt érzékelő párok fényereje
                case 15:
                    if (ch >= '0' && ch <= '3') goto loop_rs485_cmd_sto;
                    goto loop_rs485_err;

                case 16:
                case 17:
                    if (ch != 'E' && ch != 'D') goto loop_rs485_err;
                    goto loop_rs485_cmd_sto;
                case 18:    // Marker '\r'
                    if (ch != '\r') goto loop_rs485_err;
                    goto loop_rs485_cmd_next;
                case 19:    // Marker '\n'
                    if (ch != '\n') goto loop_rs485_err;
                    cMsgReceived();
                    goto loop_rs485_drop;
                default:
                    goto loop_rs485_err;
            }
            loop_rs485_cmd_sto:
            *pCMsg = ch;
            loop_rs485_cmd_next:
            pCMsg++;
            return;
    }
loop_rs485_err:
    DEBP1("EBBM%d\n", ix);
loop_rs485_drop:
    DEBC('^');
    sw = MSG_WAIT;
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
            linbas = linlen = 0;    // Sor puffer kinullázása.
        }
    }
    else {
        linpush(ch);
        rs232_putchar(ch, NULL); // Echo
        if (errstat.bits.linbuffoverfl) {
            fputs_P(PSTR("OVF\n"), &rs232_str);
            errstat.bits.linbuffoverfl = 0;
            linbas = linlen = 0;    // Sor puffer kinullázása.
        }
    }
}

void loopf(void)
{
    if ((alive & ALIVE_ALL) == ALIVE_ALL) {
        wdt_reset();
        alive &= ~ALIVE_ALL;
    }
    if (rx0len) loop_rs485();
    if (rx1len) loop_rs232();
}

void fatal()
{
    DEBP0("Fatal...\n");
    LEDOFF(L2);         // Zöld off
    LEDON(L1);
    while (tx1len) ;
    cli();
    while (1) {         // Fatális hiba, leállunk, piros LED gyorsan villog
        int i;
        LEDINV(L1);
        for (i = 0; i < 11; i++) _delay_loop_2(0);   // 56 ~= 1s
    }
}

void stop()
{
    DEBP0("Stop...\n");
    LEDOFF(L1);     // Piros off
    LEDON(L2);
    while (tx1len) ;
    cli();
    while (1) {     // Kész, leállunk, zöld LED gyorsan villog
        int i;
        LEDINV(L2);
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
    MCUSR = 0;
    prtinit();  // IO portok (LED, Jumper) inicializálása, Jumperek beolvasása
    rdConfig();         // Konfig az EEPROM-ból
    rs232_init();       // RS232 (debug port) inicializálása
    t2init();           // T2 CTC inicializálása 100Hz
    sei();
    switch (config.type) {
      case UNKNOWN:     PRNS(version1);                  break;
      case MASTER:      _PRNP1(version1m, config.id);    break;
      case SLAVE:       _PRNP1(version1s, config.id);    break;
    };
    PRNS(version2); PRNS((prog_char *)&vtm);
    PRNS(version3);
    prnConf();
#if DEBUG
    prnJumpers();
#endif
    if (jumpers.bits.miso)  setup();    // Master/Slave, és ID beállítása
    if (jumpers.bits.sck)   setup2();   // Érzékelők letiltása/engedélyezése
    if (unConfigured())     ERROR("UNCNF");
    kLight.l1 = config.light.k1;
    kLight.l2 = config.light.k2;
    kLedSt.bits.k01 = kLedSt.bits.k23 = kLedSt.bits.k45 = kLedSt.bits.k67 = KLED2;
    memset(kStat, 0, KNUM); // A 0 jelentése: első mérést eldobjuk, és KNULL az eredmény
    adinit();           // A/D konverter init
    t0init();           // Kontakt érzékelőket kezelő 400Hz-es IT
    switch (config.type) {
        case MASTER:
            i2hh(config. id, aMsg.sMaster);
            rs485_init();
            twi_init(0, NULL);
            break;
        case SLAVE:
            i2hh(config. id, aMsg.sSlave);
            twi_init(config.id << 1, eventFromTwi);
            break;
        case UNKNOWN:
            ERROR("PRG");
    }
    // Watchdog
    alive = 0;
    wdt_enable(WDTO_1S);
    DEBP0("Loop...\n");
    while(1) loopf();
}


