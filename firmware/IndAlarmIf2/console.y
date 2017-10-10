/*
*/
%{

#include "indalarmif2.h"

DECLBUFF(lin)

static inline int yyerror(const char * em) { PRNP1("yyerror(%s)\n", em); return 0; }
static inline int  yyget()          { return linpop(); }
static inline void yyunget(int _c)  { linunpop(_c); }
static int yylex(void);

static void printStat();
static void setLed(uint8_t led, uint8_t val);
static void invLed(uint8_t led);
static void prnKLeds();
static void prnCounters();

static inline void prnStErr()
{
    PRNP0("ERR:");
    if (errstat.bits.exr_occur)     PRNP0("RST:E ");
    if (errstat.bits.wdt_occur)     PRNP0("RST:W ");
    if (errstat.bits.bor_occur)     PRNP0("RST:B ");
    if (errstat.bits.pon_occur)     PRNP0("RST:P ");
    if (errstat.bits.txnbuffoverfl) PRNP0("SN:T ");
    if (errstat.bits.rxnbuffoverfl) PRNP0("SN:R ");
    if (errstat.bits.rxn_fre)       PRNP0("SN:F ");
    if (errstat.bits.txdbuffoverfl) PRNP0("SD:T ");
    if (errstat.bits.rxdbuffoverfl) PRNP0("SD:R ");
    if (errstat.bits.rxd_fre)       PRNP0("SD:F ");
    if (errstat.bits.linbuffoverfl) PRNP0("LIN ");
    if (errstat.bits.wordbuffoverfl)PRNP0("WRD ");
    if (errstat.bits.adc)           PRNP0("ADC ");
    if (errstat.bits.oeto)          PRNP0("OETO ");
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
    rs232_flush();
}

static void prnVals()
{
    uint8_t i;
    for (i = 0; i < KNUM; i++) prnVal(i);
}

static inline void prnSwVal(uint8_t i)
{
    if (sVal.byte & (1 << i)) PRNP2("S[%d]:%c/true\n", i, sStat[i]);
    else                      PRNP2("S[%d]:%c/falsee\n", i, sStat[i]);
}

static void prnSwVals()
{
    uint8_t i;
    for (i = 0; i < SNUM; i++) prnSwVal(i);
}


%}

%union {
    int             i;
//    char *          s;
}

%token      NAME_T INVALID_T ON_T OFF_T
%token      INV_T LED_T STAT_T ALL_T ERR_T VAL_T VALS_T ALIVE_T
%token      KLED_T KLIGHT_T BEEP_T CONF_T SWITCH_T SWITCHS_T REV_T
%token      SETUP_T WRITE_T ID_T MASTER_T DISABLE_T RESET_T SILENT_T
%token      LIGHT_T T0_T AD_T AUTO_T

%token <i>  INTEGER_T
// %token <s>  STRING_T NAME_T
%token <i>  BOOLEAN_T
// %type  <s>  string
%type  <i>  integer /*boolean*/

%%

command :   stats
        |   debug
        |   setled
        |   setup
/*        |   beep */
;
integer :   INTEGER_T                   { $$ = $1; }
;
/*boolean :   ON_T                        { $$ = 1; }
        |   OFF_T                       { $$ = 0; }
        |   integer                     { $$ = $1 ? 1 :0; }
;
string  :   STRING_T                    { $$ = $1; }
;*/
stats   :   '?'                         { printStat(); }
        |   STAT_T ALL_T                { printStat(); }
        |   STAT_T ERR_T                { prnStErr();  }
        |   STAT_T VAL_T integer        { prnVal($3 & 15); }
        |   STAT_T VALS_T               { prnVals();   }
        |   STAT_T SWITCH_T integer     { prnSwVal($3 & 7); }
        |   STAT_T SWITCHS_T            { prnSwVals(); }
        |   STAT_T KLED_T               { prnKLeds();  }
        |   STAT_T CONF_T               { prnConf();   }
;
setled  :   LED_T integer integer       { setLed($2, $3); }
        |   LED_T integer INV_T         { invLed($2); }
;
debug   :   alive
        |   ON_T ALL_T                  { isOff.all = 0; }
        |   OFF_T SILENT_T              { isOff.bits.silent = 1; }
        |   ON_T SILENT_T               { isOff.bits.silent = 0; }
        ;
alive   :   ALIVE_T                     { PRNP1("A:%x\n", alive); alive = 0; }    //DEBUG!
;
setup   :   SETUP_T WRITE_T             { wrConfig(); }
        |   SETUP_T MASTER_T integer    { config.id = $3; }
        |   SETUP_T DISABLE_T integer integer
                                        { config.disabled   = $3;
                                          config.s_disabled = $4;
                                        }
        |   SETUP_T REV_T integer integer
                                        { config.rev   = $3;
                                          config.s_rev = $4;
                                        }
        |   SETUP_T KLIGHT_T integer integer
                                        { config.light1 = $3;
                                          config.light2 = $4;
                                        }
        |   SETUP_T LIGHT_T integer integer
                                        { config.lightc1 = $3;
                                          config.lightc2 = $4;
                                        }
        |   SETUP_T AD_T integer        { config.adSpeed = $3; }
        |   SETUP_T T0_T integer        { config.t0Speed = $3; }
        |   SETUP_T AUTO_T integer      { config.autoSetRev = $3; autoConfInit(); }
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
        TOK(KLED),  TOK(KLIGHT),TOK(BEEP),  TOK(SWITCH),
        TOK(CONF),  TOK(SETUP), TOK(WRITE), TOK(ID),
        TOK(MASTER),TOK(DISABLE),TOK(RESET),TOK(SWITCHS),
        TOK(REV),   TOK(VAL),   TOK(VALS),  TOK(SILENT),
        TOK(LIGHT), TOK(T0),    TOK(AD),    TOK(AUTO),
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

static void prnCounters()
{
    PRNP1("cnfWrCt = %ld;", cnfWrCt);
    PRNP1(" confIsMod = %04x;", confIsMod);
    PRNP1(" cnt1s = %d; ", cnt1s);
    PRNC('\n');
}

static void printStat()
{
    prnStErr();
    prnVals();
    rs232_flush();
    prnSwVals();
    prnKLeds();
    prnCounters();
}

static void setLed(uint8_t led, uint8_t val)
{
    switch (led) {
        case 1:
            switch(val) {
            case 0: LEDOFF(RED1); LEDOFF(GRN1); DEBP0("1-\n"); break;
            case 1: LEDON(RED1);  LEDOFF(GRN1); DEBP0("1R\n"); break;
            case 2: LEDOFF(RED1); LEDON(GRN1);  DEBP0("1G\n"); break;
            case 3: LEDON(RED1);  LEDON(GRN1);  DEBP0("1Y\n"); break;
            default:    PRNP1("E:Val #%d\n", val);
            }
            break;
        case 2:
            switch(val) {
            case 0: LEDOFF(RED2); LEDOFF(GRN2); DEBP0("2-\n"); break;
            case 1: LEDON(RED2);  LEDOFF(GRN2); DEBP0("2R\n"); break;
            case 2: LEDOFF(RED2); LEDON(GRN2);  DEBP0("2G\n"); break;
            case 3: LEDON(RED2);  LEDON(GRN2);  DEBP0("2Y\n"); break;
            default:    PRNP1("E:Val #%d\n", val);
            }
            break;
        case 3:
            switch(val) {
            case 0: LEDOFF(BLUE);   DEBP0("3-\n");    break;
            case 1: LEDON(BLUE);    DEBP0("3B\n");    break;
            default:    PRNP1("E:Val #%d\n", val);
            }
        break;
        default:    PRNP1("E:LED #%d\n", led);              break;
    }
}

static void invLed(uint8_t led)
{
    switch (led) {
        case 1:     LEDINV(RED1); LEDINV(GRN1);             break;
        case 2:     LEDINV(RED2); LEDINV(GRN2);             break;
        case 3:     LEDINV(BLUE);                           break;
        default:    PRNP1("E:LED #%d\n", led);              break;
    }
}


static void prnKLeds()
{
    uint8_t i;
    uint16_t m = 1;
    PRNP0("K:");
    for (i = 0; i < KNUM; i++) {
        if (i > 0) {
            PRNC('|');
        }
        PRNC(kLedSt.kLedRd & m ? 'P': '-');
        PRNC(kLedSt.kLedGr & m ? 'Z': '-');
        m <<= 1;
    }
    PRNP2(":%d/%d\n", config.light1 +1, config.light2 +1);
    PRNP0("  ");
    m = 1;
    for (i = 0; i < KNUM; i++) {
        if (i > 0) {
            PRNC('|');
        }
        PRNC(kLedSt.kLed1 & m ? '1': '-');
        PRNC(kLedSt.kLed2 & m ? '2': '-');
        m <<= 1;
    }
    PRNC('\n');
}

