#ifndef IT_SER_H
#define IT_SER_H
#include "buffmac.h"

/*
    USART0:  RS485 bus
    USART1:  Debug
*/

DECLBUFF(tx0)
DECLBUFF(rx0)
DECLBUFF(tx1)
DECLBUFF(rx1)

extern FILE dummy_str;
extern FILE rs485_str;
extern FILE rs232_str;

extern int dummy_putchar(char c, FILE *);
extern int dummy_getchar(FILE *);
extern void rs485_init();
extern int rs485_putchar(char c, FILE *);
extern int rs485_write(char * b, uint8_t s);
extern int rs485_get();
extern int rs485_getchar(FILE *);
extern void rs232_init();
extern int rs232_putchar(char c, FILE *);
extern int rs232_getchar(FILE *);

#endif	/* IT_SER_H */
