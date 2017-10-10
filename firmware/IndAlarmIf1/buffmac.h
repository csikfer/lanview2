#ifndef	BUFFMAC_H
#define	BUFFMAC_H

#define DEFPUSH(pref, size)	\
void pref##push(uint8_t sc) { \
	if (pref##len == size) { \
		errstat.bits.pref##buffoverfl = 1; \
	} \
	else { \
		pref##bf[(pref##bas + pref##len) & (size -1) ] = sc; \
		pref##len += 1; \
	} \
}
#define DEFPOP(pref, size)	\
uint8_t pref##pop(void) { \
	uint8_t res; \
	if (!pref##len) res = 0; \
	else { \
		res = pref##bf[pref##bas]; \
		pref##bas += 1; \
		pref##bas &= (size -1); \
		pref##len -= 1; \
	} \
	return res; \
}
#define DEFUNPOP(pref, size) \
void pref##unpop(uint8_t sc) { \
	if (pref##len == size) { \
		errstat.bits.pref##buffoverfl = 1; \
	} \
	else { \
		pref##bas -= 1; \
		pref##bas &= (size -1); \
		pref##len += 1; \
		pref##bf[pref##bas] = sc; \
	} \
}

// Körpuffer kezelő függvény definíció:
// pref	prefix
// size	A kezelt puffer mérete (csak 2 hatványa lehet)
#define	DEFBUFF(pref, size)	\
	static uint8_t	pref##bf[size], pref##bas = 0; \
	volatile uint8_t	pref##len = 0; \
	DEFPUSH(pref,size) \
	DEFPOP(pref,size) \
	DEFUNPOP(pref,size)
// Kör puffer kezelő deklaráció
#define DECLBUFF(pref) \
	extern volatile uint8_t	pref##len; \
	extern void pref##push(uint8_t sc); \
	extern uint8_t pref##pop(void); \
	extern void pref##unpop(uint8_t sc);
	
#endif /* BUFFMAC_H*/
