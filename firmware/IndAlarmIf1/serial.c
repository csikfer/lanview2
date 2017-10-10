#include	"indalarmif1.h"

#define BAUDRATE0   19200
#define BAUDRATE1  115200L

// #define BRN0        ((F_CPU / 16 / BAUDRATE0) - 1)
// #define BRN1        ((F_CPU / 16 / BAUDRATE1) - 1)
#define BRN0       47
#define BRN1        7

void rs485_init()
{
    DEBP0("I:RS485\n");
    // RS485 busz (USART0) init
    OE_DDR |= _BV(OE_BIT);  // OE Kimenet
    OE_POU &=~_BV(OE_BIT);  // OE értéke Low, RS485 tr. kimenete tiltva
    tx0len = rx0len = 0;    // Bufferek kiürítése
    UBRR0L = (uint8_t)BRN0;
    UBRR0H = (uint8_t)((BRN0 >> 8) & 0x0f);
    UCSR0A = 0;
    UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0) | _BV(TXCIE0);  // Vétel, adás engedélyezve (adás ugyse megy ki)
    //UCSR0C = _BV(UCSZ00) | _BV(UCSZ01) | _BV(USBS0) | _BV(UPM00); // 8 adat, 2 stop bit, even p.
    UCSR0C = _BV(UCSZ01) | _BV(UPM01); // 7 adat, 1 stop bit, even p.
}

void rs232_init()
{
    stdout = stderr = &rs232_str;
    stdin  = &dummy_str;
    // Debug (USART1) init
    tx1len = rx1len = 0;
    UBRR1L = (uint8_t)BRN1;
    UBRR1H = (uint8_t)((BRN1 >> 8) & 0x0f);
    UCSR1A = 0;
    UCSR1B = _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1);  // Vétel, adás engedélyezve
    UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);             // 8 adat, 1 stop bit, no parity
}

DEFBUFF(tx0, 64)
DEFBUFF(rx0, 64)
DEFBUFF(tx1, 256)
DEFBUFF(rx1, 64)

FILE dummy_str = FDEV_SETUP_STREAM(dummy_putchar, dummy_getchar, _FDEV_SETUP_RW);
FILE rs485_str = FDEV_SETUP_STREAM(rs485_putchar, rs485_getchar, _FDEV_SETUP_RW);
FILE rs232_str = FDEV_SETUP_STREAM(rs232_putchar, rs232_getchar, _FDEV_SETUP_RW);

int dummy_putchar(char c, FILE * dummy)
{
    return 0;
}

int dummy_getchar(FILE * dummy)
{
    return -1;
}

int rs485_putchar(char c, FILE * dumy)
{
    UCSR0B &= ~_BV(UDRIE0);     // Disable interrupt
    OE_POU |=  _BV(OE_BIT);     // OE értéke High, RS485 tr. kimenete engedélyezve
    tx0push((uint8_t)c);
    UCSR0B |=  _BV(UDRIE0);     // Enable interrupt
    oeCt = OE_DELAY;		// Max 0.1s-ig lehetünk adásban
    return 0;
}

int rs485_write(char * b, uint8_t s)
{
    OE_POU |=  _BV(OE_BIT);     // OE értéke High, RS485 tr. kimenete engedélyezve
    UCSR0B &= ~_BV(UDRIE0);     // Disable interrupt
    for (;s > 0; --s, ++b) {
        tx0push((uint8_t)*b);
    }
    UCSR0B |=  _BV(UDRIE0);     // Enable interrupt
    oeCt = OE_DELAY;		// Max 0.1s-ig lehetünk adásban
    return s;
}

int rs485_get()
{
    int c;
    UCSR0B &= ~_BV(RXCIE0);     // Disable interrupt
    c = rx0pop();
    UCSR0B |=  _BV(RXCIE0);     // Enable interrupt
    return c;
}
int rs485_getchar(FILE * dummy)
{
    int c;
    while (0 == rx0len) ;
    UCSR0B &= ~_BV(RXCIE0);     // Disable interrupt
    c = rx0pop();
    UCSR0B |=  _BV(RXCIE0);     // Enable interrupt
    return c;
}

int rs232_putchar(char c, FILE * dummy)
{
    UCSR1B &= ~_BV(UDRIE1);     // Disable interrupt
    if (c == '\n') tx1push((uint8_t)'\r');
    tx1push((uint8_t)c);
    UCSR1B |=  _BV(UDRIE1);     // Enable interrupt
    return 0;
}


int rs232_getchar(FILE * dumy)
{
    int c;
    while (0 == rx1len) ;
    UCSR1B &= ~_BV(RXCIE1);     // Disable interrupt
    c = rx1pop();
    UCSR1B |=  _BV(RXCIE1);     // Enable interrupt

    return c;
}

// USART0 interrupts
ISR(USART0_UDRE_vect)   // USART0 Data register Empty
{
    alive |= ALIVE_SER0;
    if (tx0len) {
        UDR0 = tx0pop();
    }
    else {
        UCSR0B &= ~_BV(UDRIE0); // A továbbiakban letiltjuk ezt az IT-t, mert nincs mit adni
    }
}
ISR(USART0_RX_vect) // USART0, Rx Complete
{
    alive |= ALIVE_SER0;
    char c = UDR0;
    rx0push(c);
}

ISR(USART0_TX_vect)  // USART0, Tx Complete
{
    alive |= ALIVE_SER0;
    if (tx0len == 0 && (UCSR0B & _BV(UDRIE0)) == 0) {   // Nincs mit adni, már az adás IT is letiltva
        OE_POU &=~_BV(OE_BIT);  // OE értéke Low, RS485 tr. kimenete tiltva
	oeCt = 0;
    }
}

// USART1 interrupt
ISR(USART1_UDRE_vect)   // USART0 Data register Empty
{
    alive |= ALIVE_SER1;
    if (tx1len) {
        UDR1 = tx1pop();
    }
    else {
        UCSR1B &= ~_BV(UDRIE1);
    }
}

ISR(USART1_RX_vect) // USART1, Rx Complete
{
    alive |= ALIVE_SER1;
    char c = UDR1;
    rx1push(c);
}

