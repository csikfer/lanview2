#include "indalarmif2.h"
#include "OWIIntFunctions.h"
#include "OWIStateMachine.h"

void diag_loop()
{

}

// V�rakoz�s megadott tized m�sodpercig
void diag_wait1s()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        div1s = 100;    // M�sodperc el�oszt�, a 100-ad m�sodpercre
        silentCt = 0;   // Marad�k m�sodpercre a silent sz�ml�l� (most nem kell m�sra)
    }
    while (1 > silentCt) diag_loop();
}
void diag_wait200ms()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        div1s = 20;     // M�sodperc el�oszt�, a 100-ad m�sodpercre
        silentCt = 0;   // Marad�k m�sodpercre a silent sz�ml�l� (most nem kell m�sra)
    }
    while (1 > silentCt) diag_loop();
}
void diag_sensor_pause(uint8_t led)
{
    rxdlen = 0;
    biLeds1 = biLeds2 = 0;
    PRNP0("Ha k�sz �ss�n le egy billenty�t...\n");
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
    PRNP0("Nyugt�zza egy billenty� le�t�ssel!\n");
    while (rxdlen == 0) {
        diag_wait200ms();
        LEDINV(RED1);
        LEDINV(RED2);
    }
    LEDOFF(RED1);
    LEDOFF(RED2);
    rxdlen = 0;
}

// Diagnosztika. T2 (100Hz) �s RS232 inicializ�lva. watchdog m�g nincs.
void diags()
{
    uint8_t i;
    struct {
        unsigned a:1;
        unsigned b:1;
        unsigned c:1;
    } f;

    PRNP0("Diagnosztika...\n");
    isOff.bits.diag = 1;    // Diagnosztikai �llapot
    isOff.bits.ad = 1;      // Egyenl�re nincs A/D
    isOff.bits.silent = 1;  // Nem kell reset, ha nincs kommunik�ci�

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

    // Felvillantjuk sorba az �rz�kel�k LED-jeit:
    PRNP0("�rz�kel� csatlakoz� LED-jei:\n");
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
    PRNP0("\nA h�rom egyedi LED:\n");
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
    PRNP0("Ad�s LED s�t�t. V�teli LED villog. 3mp:\n");
    for (i = 0; i < 15; ++i) {
        PIND  |= _BV(2);    // Togle pin
        diag_wait200ms();
    }
    DDRD   &=~_BV(2);       // RX bemenet
    DDRD   |= _BV(3);       // TX kimenet
    OE_POU |= _BV(OE_BIT);  //  OE = H, enged�lyezve
    IE_POU &=~_BV(IE_BIT);  // -IE = L, enged�lyezve
    PORTD  |= _BV(3);       // onnal kezd�nk
    f.a = 1;                // a ciklusban is k�ne err�l tudni
    PRNP0("Ad�s LED �g. V�teli LED villog. 3mp:\n");
    for (i = 0; i < 15; ++i) {
        PIND |= _BV(3);    // Togle pin
        f.a = f.a ? 0 : 1;
        diag_wait200ms();
        f.b = (PIND & _BV(3)) ? 1 : 0;
        // PRNP2("PIN := %d, = %d\n", f.a, f.b);
        if (f.a != f.b) {
            PRNP0("Nem a ki�rt szintet olvastam vissza !\n");
            diag_error_pause();
        }
    }
    rs485_init();
    PRNP0("Adat k�ld�s / saj�t adat fogad�sa:\n");
    OE_POU |= _BV(OE_BIT);  //  OE = H, enged�lyezve
    IE_POU &=~_BV(IE_BIT);  // -IE = L, enged�lyezve
    {
#define PSIZ 60
        static const char pat[PSIZ] PROGMEM = "0123456789ABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvxyz";
        PGM_P p = pat;
        // Ki�rjuk a mint�t
        UCSR1B &= ~_BV(UDRIE1);     // Disable interrupt
        for (i = 0; i < PSIZ; ++i, ++p) {
            txnpush(pgm_read_byte(p));
        }
        UCSR1B |=  _BV(UDRIE1);     // Enable interrupt

        // V�runk
        diag_wait1s();   // Ez kb. 2 kbyte-hoz el�g id�

        // Megpr�b�ljuk visszaolvasni
        p = pat;
        for (i = 0; i < PSIZ; ++i, ++p) {
            char    c, cp;
            if (0 == rxnlen) {
                PRNP2("Kevesebb adatot olvastam vissza, mint amit ki�rtam : %d != %d\n", PSIZ, i);
                diag_error_pause();
                break;
            }
            UCSR1B &= ~_BV(RXCIE1);     // Disable interrupt
            c = rxnpop();
            UCSR1B |=  _BV(RXCIE1);     // Enable interrupt
            cp = pgm_read_byte(p);
            if (c != cp) {
                PRNP3("A ki�rt �s visszaolvasott adat nem eggyezik [%d]:'%c' helyett '%c'.\n", i, cp, c);
                diag_error_pause();
                break;
            }
        }
        if (0 != rxnlen) {
            PRNP0("T�bb adatot olvastam vissza, mint amit ki�rtam.\n");
            diag_error_pause();
        }
    }
    // S1
    PRNP0("A REED tesztje...\n");
    if (GETJP(S1)) {    // Z�rt
        PRNP0("Az S1 REED z�rt, vagy z�rlat!\n");
        diag_error_pause();
    }
    else {
        PRNP0("K�zel�tsen egy m�gnest az S1 REED cs�h�z, vagy �ss�n egy billenty�t, ha ezt kihagyja!\n");
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
    // �rz�kel�k
    PRNP0("�rz�kel�k tesztel�se, nincsenek szenzorok csatlakozva:\n");
    adinit();           // A/D konverter init
    t0init();           // Kontakt �rz�kel�ket kezel� 800Hz/1600Hz-es IT
    diag_sensor_pause(0xff);
    diag_wait1s();      // v�runk, hogy legyen m�r�si eredm�ny
    //Ha kVal1[i] < 0.1V �s kVal2[i] > 4.9V, akkor OK.
#define U0V1    (01*256/50)
#define U4V9    (49*256/50)
#define TOmV(x) ((((uint16_t)(x)) * 250) / 128)
    for (i = 0; i < KNUM; i++) {
        f.c = 0;
        if (kVal1[i] >= U0V1) {
            PRNP3("A %d �rz�kel� port hib�s, t�l magas fesz�lts�g: %d0mV / %d0mV\n", i, TOmV(kVal1[i]), TOmV(kVal2[i]));
            diag_error_pause();
            f.c = 1;
        }
        if (kVal2[i] <= U4V9) {
            PRNP3("A %d �rz�kel� port hib�s, t�l alacsony fesz�lts�g: %d0mV / %d0mV\n", i, TOmV(kVal1[i]), TOmV(kVal2[i]));
            diag_error_pause();
            f.c = 1;
        }
        if (f.c == 0) {
            PRNP3("A %d �rz�kel� fesz�lts�g rendben: %d0mV / %d0mV\n", i, TOmV(kVal1[i]), TOmV(kVal2[i]));
            rs232_flush();
        }
    }

    for (i = 0; i < KNUM; i += 4) {
        PRNP2("�rz�kel�k tesztel�se, r�vidz�rak az %d-%d szenzorok hely�n.:\n", i+1, i+4);
        uint8_t j = _BV(i/2) | _BV((i/2)+1);
        diag_sensor_pause(j);
        for (j = i; j < (i  +4); j++) {
            f.c = 0;
            if (kVal2[j] >= AD1V) {
                PRNP3("A %d �rz�kel� port hib�s, t�l magas fesz�lts�g: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal1[j] <= AD4V) {
                PRNP3("A %d �rz�kel� port hib�s, t�l alacsony fesz�lts�g: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (f.c == 0) {
                PRNP3("A %d �rz�kel� fesz�lts�g rendben : %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                rs232_flush();
            }
        }
    }

    for (i = 0; i < KNUM; i += 4) {
        PRNP2("�rz�kel�k tesztel�se, antiparalel LED-ek az %d-%d szenzorok hely�n.:\n", i+1, i+4);
        uint8_t j = _BV(i/2) | _BV((i/2)+1);
        diag_sensor_pause(j);
        for (j = i; j < (i  +4); j++) {
            f.c = 0;
            if (kVal2[j] >= AD3V) {
                PRNP3("A %d �rz�kel� port(1) hib�s, t�l magas fesz�lts�g: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal2[j] <= AD2V) {
                PRNP3("A %d �rz�kel� port(1) hib�s, t�l alacsony fesz�lts�g: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal1[j] <= AD2V) {
                PRNP3("A %d �rz�kel� port(2) hib�s, t�l alacsony fesz�lts�g: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (kVal1[j] >= AD3V) {
                PRNP3("A %d �rz�kel� port(2) hib�s, t�l magas fesz�lts�g: %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                diag_error_pause();
                f.c = 1;
            }
            if (f.c == 0) {
                PRNP3("A %d �rz�kel� fesz�lts�g rendben : %d0mV / %d0mV\n", j, TOmV(kVal1[j]), TOmV(kVal2[j]));
                rs232_flush();
            }
        }
    }

    PRNP0("Diagnosztika v�ge\n");
    rs232_flush();
    reset();
}

