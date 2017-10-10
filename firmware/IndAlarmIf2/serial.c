#include	"indalarmif2.h"
/*
    USART0:  RS232 Debug
    USART1:  RS485 bus "Net"
    USART2:  1-Wire bus
    USART3:  kikapcsolva
*/

// Méri az időt, mennyi ideig adunk a RS485-ön, 10mS-es számláló.
volatile uint8_t  oeCt = 0;

#define NET_BAUDRATE   19200
#define DEB_BAUDRATE  115200L

// #define BRN0        ((F_CPU / 16 / BAUDRATE0) - 1)
// #define BRN1        ((F_CPU / 16 / BAUDRATE1) - 1)
#define NET_BRN       47
#define DEB_BRN        7

void rs485_init()
{
    DEBP0("I:RS485\n");
    // RS485 busz (USART1) init
    OE_DDR |= _BV(OE_BIT);  // OE Kimenet
    IE_DDR |= _BV(IE_BIT);  // IE kimenet
    rs485_setrec();         // Transceiver: csak vétel
    txnini(); rxnini();     // Bufferek kiürítése
    UBRR1L = (uint8_t)NET_BRN;
    UBRR1H = (uint8_t)((NET_BRN >> 8) & 0x0f);
    UCSR1A = 0;
    UCSR1B = _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1) | _BV(TXCIE1);  // Vétel, adás engedélyezve (adás ugyse megy ki)
    //UCSR0C = _BV(UCSZ00) | _BV(UCSZ01) | _BV(USBS0) | _BV(UPM00); // 8 adat, 2 stop bit, even p.
    UCSR1C = _BV(UCSZ11) | _BV(UPM11); // 7 adat, 1 stop bit, even p.
}

void rs232_init()
{
    stdout = stderr = &rs232_str;
    stdin  = &rs232_str;
    // Debug (USART0) init
    txdini(); rxdini();
    UBRR0L = (uint8_t)DEB_BRN;
    UBRR0H = (uint8_t)((DEB_BRN >> 8) & 0x0f);
    UCSR0A = 0;
    UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);  // Vétel, adás engedélyezve
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);             // 8 adat, 1 stop bit, no parity
}

// NET
DEFBUFF(txn, 64)
DEFBUFF(rxn, 64)
// DEBUG
DEFBUFF(txd, 256)
DEFBUFF(rxd, 64)

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
    UCSR1B &= ~_BV(UDRIE1);     // Disable interrupt
    rs485_setsnd();
    txnpush((uint8_t)c);
    UCSR1B |=  _BV(UDRIE1);     // Enable interrupt
    return 0;
}

int rs485_write(char * b, uint8_t s)
{
    rs485_setsnd();
    UCSR1B &= ~_BV(UDRIE1);     // Disable interrupt
    for (;s > 0; --s, ++b) {
        txnpush((uint8_t)*b);
    }
    UCSR1B |=  _BV(UDRIE1);     // Enable interrupt
    return s;
}

int rs485_get()
{
    int c;
    UCSR1B &= ~_BV(RXCIE1);     // Disable interrupt
    c = rxnpop();
    UCSR1B |=  _BV(RXCIE1);     // Enable interrupt
    return c;
}
int rs485_getchar(FILE * dummy)
{
    int c;
    while (0 == rxnlen) ;
    UCSR1B &= ~_BV(RXCIE1);     // Disable interrupt
    c = rxnpop();
    UCSR1B |=  _BV(RXCIE1);     // Enable interrupt
    return c;
}

int rs232_putchar(char c, FILE * dummy)
{
    UCSR0B &= ~_BV(UDRIE0);     // Disable interrupt
    if (c == '\n') txdpush((uint8_t)'\r');
    txdpush((uint8_t)c);
    UCSR0B |=  _BV(UDRIE0);     // Enable interrupt
    return 0;
}


int rs232_getchar(FILE * dumy)
{
    int c;
    while (0 == rxdlen) ;
    UCSR0B &= ~_BV(RXCIE0);     // Disable interrupt
    c = rxdpop();
    UCSR0B |=  _BV(RXCIE0);     // Enable interrupt

    return c;
}

// USART1 (RS485/NET) interrupts
ISR(USART1_UDRE_vect)   // USART1 Data register Empty
{
    alive |= ALIVE_NET;
    if (txnlen) {
        UDR1 = txnpop();
    }
    else {
        UCSR1B &= ~_BV(UDRIE1); // A továbbiakban letiltjuk ezt az IT-t, mert nincs mit adni
    }
}
ISR(USART1_RX_vect) // USART1, Rx Complete
{
    alive |= ALIVE_NET;
    char c = UDR1;
    rxnpush(c);
}

ISR(USART1_TX_vect)  // USART1, Tx Complete
{
    alive |= ALIVE_NET;
    if (txnlen == 0 && (UCSR1B & _BV(UDRIE1)) == 0) {   // Nincs mit adni, már az adás IT is letiltva
        rs485_setrec();
    }
}

// USART0 (RS232/Debug) interrupt
ISR(USART0_UDRE_vect)   // USART0 Data register Empty
{
    alive |= ALIVE_NET;
    if (txdlen) {
        UDR0 = txdpop();
    }
    else {
        UCSR0B &= ~_BV(UDRIE0);
    }
}

ISR(USART0_RX_vect) // USART1, Rx Complete
{
    alive |= ALIVE_DEB;
    char c = UDR0;
    rxdpush(c);
}

