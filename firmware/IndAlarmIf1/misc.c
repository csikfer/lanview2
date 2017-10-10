#include    "indalarmif1.h"

void dump(FILE *str, void *b, uint8_t s)
{
    char * p = b;
    uint8_t i;
    putc('"', str);
    for (i = 0; i < s; i++, p++) prnc(str, *p);
    putc('"', str);
}

