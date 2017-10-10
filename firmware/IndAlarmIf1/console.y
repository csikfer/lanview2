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
13 o o 14    ID 6.bit:  close=1, open=0
15 o o 16    ID 7.bit:  close=1, open=0 Slave esetén nullának kell lennie
*/
%{

#include "indalarmif1.h"

DECLBUFF(lin)

static inline int yyerror(const char * em) { PRNP1("yyerror(%s)\n", em); return 0; }
static inline int  yyget()          { return linpop(); }
static inline void yyunget(int _c)  { linunpop(_c); }
static int yylex(void);

static void printStat();
static void setLed(uint8_t led, uint8_t val);
static void invLed(uint8_t led);
static void setKLed(uint8_t k, uint8_t l);
static void setKLight(uint8_t l, uint8_t v);
static void prnKLeds();

static inline void prnStErr()
{
    PRNP0("ERR:");
    if (errstat.bits.exr_occur)     PRNP0("RST:E ");
    if (errstat.bits.wdt_occur)     PRNP0("RST:W ");
    if (errstat.bits.bor_occur)     PRNP0("RST:B ");
    if (errstat.bits.pon_occur)     PRNP0("RST:P ");
    if (errstat.bits.tx0buffoverfl) PRNP0("S0:T ");
    if (errstat.bits.rx0buffoverfl) PRNP0("S0:R ");
    if (errstat.bits.rx0_fre)       PRNP0("S0:F ");
    if (errstat.bits.tx1buffoverfl) PRNP0("S1:T ");
    if (errstat.bits.rx1buffoverfl) PRNP0("S1:R ");
    if (errstat.bits.rx1_fre)       PRNP0("S1:F ");
    if (errstat.bits.linbuffoverfl) PRNP0("LIN ");
    if (errstat.bits.wordbuffoverfl)PRNP0("WRD ");
    if (errstat.bits.twi)           PRNP0("TWI ");
    if (errstat.bits.adc)           PRNP0("ADC ");
    PRNC('\n');
}

/// Az A/D konverter által mért értékből csinál egy stringet 2 tizedest és a mértékegységet írja ki.
/// A visszaadott érték egy statikus buffer. Minden alkalommal felülíródik!
static inline char * adval(uint16_t v)
{
    static char b[8];
    v = (v * 125) / 64;    // v * 5V * 100 / 256 = U * 100  (Szorzó és osztó 4-el egyszerüsítve, ne legyen túlcsrdulás)
    snprintf_P(b, 8, PSTR("%d.%02dV"), v / 100, v % 100);
    return b;
}
static inline void prnVal(uint8_t i)
{
    PRNP2("V[%d]:%s/", i, adval(kVal1[i]));
    PRNP2("%s:%c\n", adval(kVal2[i]), kStat[i]);
}
static void prnVals();
static void prnTwi();

%}

%union {
    int             i;
//    char *          s;
}

%token      NAME_T INVALID_T
%token      ON_T OFF_T INV_T LED_T STAT_T ALL_T ERR_T VAL_T VALS_T ALIVE_T TWI_T
%token      KLED_T KLIGHT_T BEEP_T CONF_T
%token      SETUP_T WRITE_T ID_T MASTER_T SLAVE_T DISABLE_T RESET_T

%token <i>  INTEGER_T
// %token <s>  STRING_T NAME_T
%token <i>  BOOLEAN_T
// %type  <s>  string
%type  <i>  integer boolean

%%

command :   stats
        |   debug
        |   setled
        |   kont
        |   setup
/*        |   beep */
;
integer :   INTEGER_T                   { $$ = $1; }
;
boolean :   ON_T                        { $$ = 1; }
        |   OFF_T                       { $$ = 0; }
        |   integer                     { $$ = $1 ? 1 :0; }
;
/*string  :   STRING_T                    { $$ = $1; }
;*/
stats   :   '?'                         { printStat(); }
        |   STAT_T ALL_T                { printStat(); }
        |   STAT_T ERR_T                { prnStErr();  }
        |   STAT_T VAL_T integer        { prnVal($3 & 7); }
        |   STAT_T VALS_T               { prnVals();   }
        |   STAT_T KLED_T               { prnKLeds();  }
        |   STAT_T TWI_T                { prnTwi();    }
        |   STAT_T CONF_T               { prnConf();   }
;
setled  :   LED_T integer boolean       { setLed($2, $3); }
        |   LED_T integer INV_T         { invLed($2); }
;
kont    :   KLED_T integer integer      { setKLed($2, $3); }
        |   KLIGHT_T integer integer    { setKLight($2,$3); }
;
debug   :   alive
;
alive   :   ALIVE_T                     { PRNP1("A:%x\n", alive); alive = 0; }    //DEBUG!
;
setup   :   SETUP_T WRITE_T             { wrConfig(); }
        |   SETUP_T MASTER_T integer    { config.type = MASTER; config.id = $3; }
        |   SETUP_T SLAVE_T integer     { config.type = SLAVE;  config.id = $3; }
        |   SETUP_T DISABLE_T integer boolean boolean
                                        { config.disabled.k  = $3;
                                          config.disabled.s1 = $4;
                                          config.disabled.s2 = $5;
                                        }
        |   SETUP_T KLIGHT_T integer integer
                                        { config.light.k1 = $3;
                                          config.light.k2 = $4;
                                        }
        |   RESET_T                     { reset(); }
/*beep    :   BEEP_T integer              { beep($2); }
;*/

%%

DEFBUFF(word, 16)
static inline void wordclr() { wordlen = wordbas = 0; }

#define _STR(t)  #t
#define TOK(t)  { _STR(t), t##_T }

static int yylex(void)
{
    static char cToken[] PROGMEM = "?";
    static struct token {
        prog_char       name[16];
        prog_int16_t    value;
    } sToken[] PROGMEM = {
        TOK(ON),    TOK(OFF),   TOK(INV),   TOK(LED),
        TOK(STAT),  TOK(ALL),   TOK(ERR),   TOK(ALIVE),
        TOK(KLED),  TOK(KLIGHT),TOK(BEEP),  TOK(TWI),
        TOK(CONF),  TOK(SETUP), TOK(WRITE), TOK(ID),
        TOK(MASTER),TOK(SLAVE), TOK(DISABLE),TOK(RESET),
        { "", 0 }
    };
    const struct token *p;

    yylval.i = 0;
    int     c;
    while (isspace(c = yyget()));
    if (!c) return 0;
    // VALUE INT
    if (isdigit(c)) {
        wordclr();
        char cf = c;    // Meg kelleni fog
        wordpush((char)c);
        while (isdigit(c = yyget())
           || ('x' == tolower(c) && cf == '0' && wordlen == 1)
           || (strchr_P(PSTR("abcdef"), tolower(c)) && strncasecmp_P((char *)wordbf, PSTR("0x"), 2) == 0)) {
            wordpush(c);
        }
        if (c) yyunget(c);
        wordpush(0);
        if (cf == '0') {
            char cn = wordlen > 1 ? wordbf[1] : 0;
            if (cn == 'x' || cn == 'X') {   // HEXA
                yylval.i = (int)strtol((char *)wordbf, NULL, 16);
            }
            else {                          // OCTAL
                yylval.i = (int)strtol((char *)wordbf, NULL, 8);
            }
        }
        else {
            yylval.i = (int)strtol((char *)wordbf, NULL, 10);;
        }
        wordclr();
        return INTEGER_T;
    }
    // Egybetus tokenek
    if (strchr_P(cToken, c)) {
        return c;
    }
    wordclr();
    while (isalnum(c) || c == '_') {
        wordpush(c);
        c = yyget();
    }
    if (wordlen) {
        if (c != 0) yyunget(c);
        wordpush(0);
//        DEBP1("T/%s/", (char *)wordbf);
        for (p = sToken; 1; p++) {
            int r = pgm_read_word_near(&(p->value));
            if (r == 0) break;
//            DEBP1("C/%S/", p->name);
            if (strcasecmp_P((char *)wordbf, p->name) == 0) {
//                 DEBP1("M%d\n", r);
                 return r;
            }
        }
//        DEBP0("UM\n");
/*        for (p = sToken; 0 != pgm_read_word_near(&(p->name)); p++) {
            if (strcmp_P((char *)wordbf, (prog_char *)pgm_read_word_near(&(p->name)))) {
                return pgm_read_word_near(&(p->value));
            }
        }*/
        //  yylval.s = strdup((char *)wordbf);
        return NAME_T;
    }
    return INVALID_T;
}

/********/

static void printStat()
{
    prnStErr();
    prnVals();
    prnKLeds();
}

static void prnVals()
{
    uint8_t i;
    for (i = 0; i < 8; i++) prnVal(i);
}
static void setLed(uint8_t led, uint8_t val)
{
    switch (led) {
        case 1:     if (val) LEDON(L1); else LEDOFF(L1);    break;
        case 2:     if (val) LEDON(L2); else LEDOFF(L2);    break;
        default:    PRNP1("E:LED #%d\n", led);              break;
    }
}

static void invLed(uint8_t led)
{
    switch (led) {
        case 1:     LEDINV(L1);                             break;
        case 2:     LEDINV(L2);                             break;
        default:    PRNP1("E:LED #%d\n", led);              break;
    }
}

static void setKLed(uint8_t k, uint8_t l)
{
    if (k > 7 || l > 3) PRNP0("E:KLED\n");
    switch (k) {    // érzékelő sorszáma
        case 0:
        case 1:     kLedSt.bits.k01 = l;    break;
        case 2:
        case 3:     kLedSt.bits.k23 = l;    break;
        case 4:
        case 5:     kLedSt.bits.k45 = l;    break;
        case 6:
        case 7:     kLedSt.bits.k67 = l;    break;
    }
}

static void setKLight(uint8_t l, uint8_t v)
{
    if (l < 1 || l > 2 || v > 3) PRNP0("E:KLIGHT\n");
    if (l == 1) kLight.l1 = v;
    else        kLight.l2 = v;
}

static void prnKLeds()
{
    PRNP0("K:");
    PRNC(kLedSt.bits.k01 & KLED1 ? 'P': '-');
    PRNC(kLedSt.bits.k01 & KLED2 ? 'Z': '-');
    PRNC('|');
    PRNC(kLedSt.bits.k23 & KLED1 ? 'P': '-');
    PRNC(kLedSt.bits.k23 & KLED2 ? 'Z': '-');
    PRNC('|');
    PRNC(kLedSt.bits.k45 & KLED1 ? 'P': '-');
    PRNC(kLedSt.bits.k45 & KLED2 ? 'Z': '-');
    PRNC('|');
    PRNC(kLedSt.bits.k67 & KLED1 ? 'P': '-');
    PRNC(kLedSt.bits.k67 & KLED2 ? 'Z': '-');
    PRNP2(":%d/%d\n", kLight.l1 +1, kLight.l2 +1);
}

static void prnTwi()
{
    PRNP0("TWI:");
    switch (twi.st & ~(TWI_WAIT | TWI_ERROR)) {
        case TWI_IDLE:          PRNP0("ID");    break;
        case TWI_BUSY:          PRNP0("BY");    break;
        case TWI_MASTER_TX:     PRNP0("MT");    break;
        case TWI_MASTER_RX:     PRNP0("MR");    break;
        case TWI_SLAVE_TX:      PRNP0("ST");    break;
        case TWI_SLAVE_RX:      PRNP0("SR");    break;
        default:                PRNP1("%02x", twi.st & ~(TWI_WAIT | TWI_ERROR));    break;
    }
    if (twi.st & TWI_WAIT)  PRNP0("/W");
    if (twi.st & TWI_ERROR) PRNP0("/E");
    PRNP2(":C=%02x:S=%02x", TWCR, TWSR);
    PRNP2(":B=%02x:A=%02x", TWBR, TWAR);
    PRNP1(":M=%02x\n", TWAMR);
}
