#ifndef MISC_H
#define MISC_H

#include <ctype.h>

static inline uint8_t h2i(char c)
{
    return (uint8_t)(isdigit(c) ? c -'0' : toupper(c) - 'A' + 10);
}

static inline int8_t isbool(char c)
{
    return c == 'T' || c == 'F';
}

static inline char i2h(uint8_t _i)
{
    uint8_t i = _i & 0x0f;
    return i < 10 ? '0' + i : 'A'+ i - 10;
}

static inline void i2hh(uint8_t i, char * ph)
{
    ph[0] = i2h(i >> 4);
    ph[1] = i2h(i);
}

static inline void i2hhhh(uint16_t i, char * ph)
{
    ph[0] = i2h(i >> 12);
    ph[1] = i2h(i >>  8);
    ph[2] = i2h(i >>  4);
    ph[3] = i2h(i);
}

static inline uint8_t hh2i(char * ph)
{
    return h2i(ph[0]) * 16 + h2i(ph[1]);
}

static inline void prnc(FILE *str, char c)
{
    switch (c) {
        case 0:     fputs_P(PSTR("\\0"), str);    break;
        case '\n':  fputs_P(PSTR("\\n"), str);    break;
        case '\r':  fputs_P(PSTR("\\r"), str);    break;
        case '\t':  fputs_P(PSTR("\\t"), str);    break;
        case '\\':  fputs_P(PSTR("\\\\"), str);   break;
        case '"':   fputs_P(PSTR("\\\""), str);   break;
        default:
            if (isprint(c)) putc(c, str);
            else fprintf_P(str, PSTR("\\x%02x"), c);
    }
}

extern void dump(FILE *str, void *b, uint8_t s);
#endif
