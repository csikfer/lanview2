#include "indalarmif2.h"
#include "OWIIntFunctions.h"
#include "OWIStateMachine.h"

void diag_loop()
{

}

// Várakozás megadott tized másodpercig
void diag_wait1s()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        div1s = 100;    // Másodperc elõosztó, a 100-ad másodpercre
        silentCt = 0;   // Maradék másodpercre a silent számláló (most nem kell másra)
    }
    while (1 > silentCt) diag_loop();
}
void diag_wait200ms()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        div1s = 20;     // Másodperc elõosztó, a 100-ad másodpercre
        silentCt = 0;   // Maradék másodpercre a silent számláló (most nem kell másra)
    }
    while (1 > silentCt) diag_loop();
}
void diag_sensor_pause(uint8_t led)
{
    rxdlen = 0;
    biLeds1 = biLeds2 = 0;
    PRNP0("Ha kész üssön le egy billentyût...\n");
    while (rxdlen == 0) {
        biLeds1 ^= led;
        diag_wait200ms();
    }
    biLeds1 = 0;
    biLeds2 = led;
    diag_wait1s();
    biLeds2 = 0;
    rxdlen = 0;
}

void diag_error_pause()
{
    LEDOFF(RED1);
    LEDON(RED2);
    rxdlen = 0;
    PRNP0("Nyugtázza egy billentyû leütéssel!\n");
    while (rxdlen == 0) {
        diag_wait200ms();
        LEDINV(RED1);
        LEDINV(RED2);
    }
    LEDOFF(RED1);
    LEDOFF(RED2);
    rxdlen = 0;
}

// Diagnosztika. T2 (100Hz) és RS232 inicializálva. watchdog még nincs.
void diags()
{
    uint8_t i;
    struct {
        unsigned a:1;
        unsigned b:1;
        unsigned c:1;
    } f;

    PRNP0("Diagnosztika...\n");
    isOff.bits.diag = 1;    // Diagnosztikai állapot
    isOff.bits.ad = 1;      // Egyenlõre nincs A/D
    isOff.bits.silent = 1;  // Nem kell reset, ha nincs kommunikáció

    PRNP0("OWI_Init()\n");
    OWI_Init();
    rxdini();
    while (1) {
        unsigned char st = OWI_StateMachine();
        if (st == OWI_STATE_CONVERTED_T) {
            PRNP2("T = %d.%d\n", temperature / 2, (temperature & 1) ?  5 : 0);
        }
        if (rxdlen > 0) {
            char c = rs232_getchar(NULL);
            if (c == 'e') break;
        }
        _delay_loop_2(14747U);  // ~4ms
    }

    // Felvillantjuk sorba az érzékelõk LED-jeit:
    PRNP0("Érzékelõ csatlakozó LED-jei:\n");
    for (uint8_t m = 1; m; m <<= 1) {
        DEBC('P');
        biLeds1 = m;
        biLeds2 = 0;
        diag_wait200ms();
        DEBC('Z');
        biLeds1 = 0;
        biLeds2 = m;
        diag_wait200ms();
        DEBC('S');
        biLeds1 = biLeds2 = m;
        diag_wait200ms();
    }
    PRNP0("\nA három egyedi LED:\n");
    biLeds1 = biLeds2 = 0;
    LEDON(RED1);
    diag_wait1s();
    LEDOFF(RED1);
    LEDON(GRN1);
    diag_wait1s();
    LEDON(RED1);
    diag_wait1s();
    LEDOFF(GRN1);
    LEDOFF(RED1);

    LEDON(RED2);
    diag_wait1s();
    LEDOFF(RED2);
    LEDON(GRN2);
    diag_wait1s();
    LEDON(RED2);
    diag_wait1s();
    LEDOFF(GRN2);
    LEDOFF(RED2);

    LEDON(BLUE);
    diag_wait1s();
    LEDOFF(BLUE);

    PRNP0("\nRS485:\n");
    OE_DDR |= _BV(OE_BIT);  // OE Kimenet
    IE_DDR |= _BV(IE_BIT);  // -IE kimenet
    OE_POU &=~_BV(OE_BIT);  //  OE = L, tiltva
    IE_POU |= _BV(IE_BIT);  // -IE = H, tiltva
    DDRD   |= _BV(2);       // RX Ideiglenesen kimenet
    PRNP0("Adás LED sötét. Vételi LED villog. 3mp:\n");
    for (i = 0; i < 15; ++i) {
        PIND  |= _BV(2);    // Togle pin
        diag_wait200ms();
    }
    DDRD   &=~_BV(2);       // RX bemenet
    DDRD   |= _BV(3);       // TX kimenet
    OE_POU |= _BV(OE_BIT);  //  OE = H, engedélyezve
    IE_POU &=~_BV(IE_BIT);  // -IE = L, engedélyezve
    PORTD  |= _BV(3);       // onnal kezdünk
    f.a = 1;                // a ciklusban is kéne errõl tudni
    PRNP0("Adás LED ég. Vételi LED villog. 3mp:\n");
    for (i = 0; i < 15; ++i) {
        PIND |= _BV(3);    // Togle pin
        f.a = f.a ? 0 : 1;
        diag_wait200ms();
        f.b = (PIND & _BV(3)) ? 1 : 0;
        // PRNP2("PIN := %d, = %d\n", f.a, f.b);
        if (f.a != f.b) {
            PRNP0("Nem a kiírt szintet olvastam vissza !\n");
            diag_error_pause();
        }
    }
    rs485_init();
    PRNP0("Adat küldés / saját adat fogadása:\n");
    OE_POU |= _BV(OE_BIT);  //  OE = H, engedélyezve
    IE_POU &=~_BV(IE_BIT);  // -IE = L, engedélyezve
    {
#define PSIZ 60
        static const char pat[PSIZ] PROGMEM = "0123456789ABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvxyz";
        PGM_P p = pat;
        // Kiírjuk a mintát
        UCSR1B &= ~_BV(UDRIE1);     // Disable interrupt
        for (i = 0; i < PSIZ; ++i, ++p) {
            txnpush(pgm_read_byte(p));
        }
        UCSR1B |=  _BV(UDRIE1);     // Enable interrupt

        // Várunk
        diag_wait1s();   // Ez kb. 2 kbyte-hoz elég idõ

        // Megpróbáljuk visszaolvasni
        p = pat;
        for (i = 0; i < PSIZ; ++i, ++p) {
            char    c, cp;
            if (0 == rxnlen) {
                PRNP2("Kevesebb adatot olvastam vissza, mint amit kiírtam : %d != %d\n", PSIZ, i);
                diag_error_pause();
                break;
            }
            UCSR1B &= ~_BV(RXCIE1);     // Disable interrupt
            c = rxnpop();
            UCSR1B |=  _BV(RXCIE1);     // Enable interrupt
            cp = pgm_read_byte(p);
            if (c != cp) {
                PRNP3("A kiírt és visszaolvasott adat nem eggyezik [%d]:'%c' helyett '%c'.\n", i, cp, c);
                diag_error_pause();
                break;
            }
        }
        if (0 != rxnlen) {
            PRNP0("Több adatot olvastam vissza, mint amit kiírtam.\n");
            diag_error_pause();
        }
    }
    // S1
    PRNP0("A REED tesztje...\n");
    if (GETJP(S1)) {    // Zárt
        PRNP0("Az S1 REED zárt, vagy zárlat!\n");
        diag_error_pause();
    }
    else {
        PRNP0("Közelítsen egy mágnest az S1 REED csõhöz, vagy üssön egy billentyût, ha ezt kihagyja!\n");
        rxdlen = 0;
        while (1) {
            if (rxdlen != 0) {
                PRNP0("Ez a teszt kihagyva.\n");
                rxdlen = 0;
                break;
            }
            if (GETJP(S1)) {
                PRNP0("S1 O.K.\n");
                break;
            }
        }
    }
    // Érzékelõk
    PRNP0("Érzékelõk tesztelése, nincsenek szenzorok csatlakozva:\n");
    adinit();           // A/D konverter init
    t0init();           // Kontakt érzékelõket kezelõ 800Hz/1600Hz-es IT
    diag_sensor_pause(0xff);
    diag_wait1s();      // várunk, hogy legyen mérési eredmény
    //Ha kVal1[i] < 0.1V és kVal2[i] > 4.9V, akkor OK.
#define U0V1    (01*256/50)
#define U4V9    (49*256/50)
#define TOmV(x) ((((uint16_t)(x)) * 250) / 128)
    for (i = 0; i < KNUM; i++) {
        f.c = 0;
        if (kVal1[i] >= U0V1) {
            PRNP3("A %d érzékelõ port hibás, túl magas feszültség: %d0mV / %d0mV\n", i, TOmV(kVal1[i]), TOmV(kVal2[i]));
            diag_error_pause();
            f.c = 1;
        }
        if (kVal2[i] <= U4V9) {
            PRNP3("A %d érzékelõ port hibás, túl alacsony feszültség: %d0mV / %d0mV\n", i, TOmV(kVal1[i]), TOmV(kVal2[i]));
            diag_error_pause();
            f.c = 1;
        }
        if (f.c == 0) {
            PRNP3("A %d érzékelõ feszültség rendben: %d0mV / %d0mV\n", i, TOmV(kVal1[i]), TOmV(kVal2[i]));
            rs232_flush();
        }
    }

    for (i = 0; i < KNUM; i += 4) {
        PRNP2("Érzékelõk tesztelése, rövidzárak az %d-%d szenzorok helyén.:\n", i+1, i+4);
        uint8_t j = _BV(i/2) | _BV((i/2)+1);
        diag_sensor_pause(j);
        for (j = i; j < (i  +4); j++) {
            f.c = 0;
            if (kVal2[j] >= AD1V) {
                PRNP3("A %d érzékelõ port hibás, túl magas feszültség: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal1[j] <= AD4V) {
                PRNP3("A %d érzékelõ port hibás, túl alacsony feszültség: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (f.c == 0) {
                PRNP3("A %d érzékelõ feszültség rendben : %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                rs232_flush();
            }
        }
    }

    for (i = 0; i < KNUM; i += 4) {
        PRNP2("Érzékelõk tesztelése, antiparalel LED-ek az %d-%d szenzorok helyén.:\n", i+1, i+4);
        uint8_t j = _BV(i/2) | _BV((i/2)+1);
        diag_sensor_pause(j);
        for (j = i; j < (i  +4); j++) {
            f.c = 0;
            if (kVal2[j] >= AD3V) {
                PRNP3("A %d érzékelõ port(1) hibás, túl magas feszültség: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal2[j] <= AD2V) {
                PRNP3("A %d érzékelõ port(1) hibás, túl alacsony feszültség: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal1[j] <= AD2V) {
                PRNP3("A %d érzékelõ port(2) hibás, túl alacsony feszültség: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal1[j] >= AD3V) {
                PRNP3("A %d érzékelõ port(2) hibás, túl magas feszültség: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (f.c == 0) {
                PRNP3("A %d érzékelõ feszültség rendben : %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                rs232_flush();
            }
        }
    }

    PRNP0("Diagnosztika vége\n");
    rs232_flush();
    reset();
}

