#ifndef IT_SER_H
#define IT_SER_H
#include "buffmac.h"
#include	"indalarmif2.h"

DECLBUFF(txn)
DECLBUFF(rxn)
DECLBUFF(txd)
DECLBUFF(rxd)

extern FILE dummy_str;
extern FILE rs485_str;
extern FILE rs232_str;

#define OE_DELAY	10
extern volatile uint8_t  oeCt;

extern int dummy_putchar(char c, FILE *);
extern int dummy_getchar(FILE *);
extern void rs485_init();
extern int rs485_putchar(char c, FILE *);
extern int rs485_write(char * b, uint8_t s);
extern int rs485_get();
extern int rs485_getchar(FILE *);
static inline void rs485_setrec() {
    OE_POU &=~_BV(OE_BIT);  // OE értéke Low, RS485 tr. kimenete tiltva
    IE_POU &=~_BV(IE_BIT);
    oeCt = 0;
}
static inline void rs485_setsnd() {
    OE_POU |= _BV(OE_BIT);
    IE_POU |= _BV(IE_BIT);
    oeCt = OE_DELAY;		// Max 0.1s-ig lehetünk adásban

}
static inline void rs485_setbid() {
    OE_POU |= _BV(OE_BIT);
    IE_POU &=~_BV(IE_BIT);
    oeCt = OE_DELAY;		// Max 0.1s-ig lehetünk adásban
}

extern void rs232_init();
extern int rs232_putchar(char c, FILE *);
extern int rs232_getchar(FILE *);
static inline void rs232_flush() {
    while (1) {
        if (txdlen == 0) break;
        if ((UCSR0B & _BV(UDRIE0)) == 0) break;
    }
}

#endif	/* IT_SER_H */
