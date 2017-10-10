#ifndef TWI_HEADER
#define TWI_HEADER

#include <util/twi.h>

#define TWDEBUG    1
#if     TWDEBUG
#define TWDEBC(c)       DEBC(c)
#define TWDEBS(s)       DEBS(s)
#define TWDEBP0(s)      DEBP0(s)
#define TWDEBP1(f,p)    DEBP1(f,p)
#define TWDEBP2(f,p1,p2) DEBP2(f,p1,p2)
#define TWDEBOUTC(c)    if (isalnum(c)) DEBP1("[%c]", c); else DEBP1("[\\%x]", c)
#else
#define TWDEBC(c)
#define TWDEBS(s)
#define TWDEBP0(s)
#define TWDEBP1(f, p)
#define TWDEBP2(f,p1,p2)
#define TWDEBOUTC(c)
#endif

/// @def TWI_CLK TWI (I2C) busz órajel frekvencia 100kHz
#define TWI_CLK 100000

#define TWCR_CMD_MASK           0x0F
#define TWI_OK_RESULT           0x00
#define TWI_ERROR_RESULT        0x01

// TWI Event types :
#define TWI_SLAVE_RECEIVE    1
#define TWI_SLAVE_RECEIVED   2
#define TWI_SLAVE_SEND       3
#define TWI_SLAVE_SENDED     4
#define TWI_MASTER_RECEIVE   5
#define TWI_MASTER_RECEIVED  6
#define TWI_MASTER_SEND      7
#define TWI_MASTER_SENDED    8
#define TWI_ERROR            0x80
// TWI Software status
#define TWI_IDLE         0
#define TWI_BUSY         1
#define TWI_MASTER_TX    2
#define TWI_MASTER_RX    3
#define TWI_SLAVE_TX     4
#define TWI_SLAVE_RX     5
#define TWI_WAIT         0x40
// #define TWI_ERROR     0x80

typedef uint8_t (*eventPtr)(uint8_t et);

struct twi {
    uint8_t     st;         ///< Sofware status
    uint8_t     sla;        ///< Slave address
    eventPtr    pEventFn;   ///< Event function pointer or NULL
    char *      pB;         ///< Pointer on actual buffer to actual character
    char *      sBuff;      ///< Send buffer pointer
    uint8_t     sSize;      ///< Send buffer size
    char *      rBuff;      ///< Receive buffer pointer
    uint8_t     rSize;     ///< Receive buffer size
};

extern volatile struct twi twi;

#define TWI_SEND_START TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT) | _BV(TWSTA) | _BV(TWIE)
#define TWI_SLAVE_RST  TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT) | _BV(TWEA);
#define TWI_CLEAR_BUFF twi.pB = twi.sBuff = twi.rBuff = NULL; twi.sSize = twi.rSize = 0


/// TWI inicializálás
/// @param sla Slave address(even) or zero
static inline void twi_init(uint8_t sla, eventPtr pFn)
{
    TWDEBP2("twi_init(%x, %p)\n", sla, pFn);
    twi.st = TWI_IDLE;
    TWI_CLEAR_BUFF;
    sla &= ~1;    // Igy aztán tuti őáros
    twi.pEventFn = pFn;
    TWSR = 0; /* előosztas = 1 */
    TWBR = 66; //((F_CPU / TWI_CLK) - 16) / 2 +1;
    if ((sla) != 0 && pFn != NULL) {  // SLAVE
        TWAR = twi.sla = sla;
        TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWIE) | _BV(TWEA);
    }
    else {                          // MASTER
        TWCR = _BV(TWEN);
    }
}
static inline uint8_t twiMasterWrite(uint8_t sla, char *data, uint8_t size, eventPtr pFn)
{
    if ((twi.st & ~TWI_ERROR) != TWI_IDLE) {
        TWDEBP1("twMW:S:%02x\n", twi.st);
        return twi.st;
    }
    if (TWCR & _BV(TWINT)) {
        TWDEBP1("twMW:C:%02x\n", TWCR);
        return twi.st |= TWI_ERROR;
    }
    twi.sla     = ~TW_READ & sla;
    twi.sBuff   = data;
    twi.sSize   = size;
    twi.rBuff   = NULL;
    twi.rSize   = 0;
    twi.pB      = twi.sBuff;
    twi.st      = TWI_MASTER_TX;
    twi.pEventFn = pFn;
    TWI_SEND_START;
    return TWI_OK_RESULT;
}

static inline uint8_t twiMasterRead(uint8_t sla, char *sData, uint8_t sSize, char *rBuff, uint8_t rSize, eventPtr pFn)
{
    if ((twi.st & ~TWI_ERROR) != TWI_IDLE) {
        TWDEBP1("twMR:S:%02x\n", twi.st);
        return twi.st;
    }
    if (TWCR & _BV(TWINT)) {
        TWDEBP1("twMR:C:%02x\n", TWCR);
        return twi.st |= TWI_ERROR;
    }
    twi.sBuff   = sData;
    twi.sSize   = sSize;
    twi.rBuff   = rBuff;
    twi.rSize   = rSize;
    twi.pEventFn = pFn;
    if (twi.sBuff != NULL && twi.sSize != 0) {  // Van küldés is.
        twi.sla   = ~TW_READ & sla;
        twi.st    = TWI_MASTER_TX;
        twi.pB    = twi.sBuff;
    }
    else {                                      // Csak fogadás
        twi.sla   =  TW_READ | sla;
        twi.st    = TWI_MASTER_RX;
        twi.sBuff = NULL;
        twi.sSize = 0;
        twi.pB    = twi.rBuff;
    }
    TWI_SEND_START;
    TWDEBP0("twMR:OK\n");
    return TWI_OK_RESULT;
}

static inline uint8_t twiSlaveSetRecBuff(char * buff, uint8_t size)
{
    if (twi.st != (TWI_SLAVE_RX | TWI_WAIT)) {
        TWDEBP1("twSRB:%02x", twi.st);
        return TWI_ERROR;
    }
    twi.pB = twi.rBuff = buff;
    twi.rSize = size;
    TWDEBP0("twSRB:OK\n");
    return TWI_OK_RESULT;
}

static inline uint8_t twiSlaveSetSendBuff(char * buff, uint8_t size)
{
    if (twi.st != (TWI_SLAVE_TX | TWI_WAIT)) {
        TWDEBP1("twSSB:%02x\n", twi.st);
        return TWI_ERROR;
    }
    twi.pB = twi.sBuff = buff;
    twi.sSize = size;
    TWDEBP0("twSSB:OK\n");
    return TWI_OK_RESULT;
}

// Undefine private macros
#ifndef TWI_C_FILE
#undef TWI_SEND_START
#undef TWI_CLEAR_BUFF
#undef TWI_SLAVE_RST
#endif

#endif // TWI_HEADER