#define     TWI_C_FILE
#include    "indalarmif1.h"


volatile struct twi twi = { TWI_IDLE, 0, NULL, NULL, NULL, 0, NULL, 0 };

static inline uint8_t twiEvent(uint8_t et)
{
    uint8_t r = TWI_OK_RESULT;

    if (twi.pEventFn != NULL) {
        twi.st |=  TWI_WAIT;
        r = (*twi.pEventFn)(et);
        twi.st &= ~TWI_WAIT;
        if (TWI_OK_RESULT != r) twi.st |= TWI_ERROR;
    }
    return r;
}

static inline uint8_t twiError(uint8_t et)
{
    uint8_t r = TWI_ERROR_RESULT;

    twi.st |= TWI_ERROR;
    if (twi.pEventFn != NULL) {
        twi.st |=  TWI_WAIT;
        r = (*twi.pEventFn)(et | TWI_ERROR);
        twi.st &= ~TWI_WAIT;
    }
    return r;
}

static inline void twiSendByte(uint8_t data)
{
    // save data to the TWDR
    TWDR = data;
    // begin send
    TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT) | _BV(TWEA);
}

static inline uint8_t  twiSndIx()
{
    uint16_t    ix;
    if (twi.sBuff == NULL
     || twi.sBuff > twi.pB
     || (ix = twi.pB - twi.sBuff) > twi.sSize) return twi.sSize;
    return (uint8_t)ix;

}
static inline uint8_t  twiRecIx()
{
    uint16_t    ix;
    if (twi.rBuff == NULL
        || twi.rBuff > twi.pB
        || (ix = twi.pB - twi.rBuff) > twi.rSize) return twi.rSize;
    return (uint8_t)ix;
}
static inline void     twiReStartReceive()
{
    twi.sla |= TW_READ;
    twi.pB   = twi.rBuff;
    twi.st   = TWI_MASTER_RX;
    TWI_SEND_START;
    TWDEBC('<');
}
static inline void     twiReStartSend()
{
    twi.sla &= ~TW_READ;
    twi.pB   = twi.sBuff;
    twi.st   = TWI_MASTER_TX;
    TWI_SEND_START;
    TWDEBC('+');
}
static inline void     twiStop()
{
    TWCR = (TWCR & TWCR_CMD_MASK & ~_BV(TWIE)) | _BV(TWINT) | _BV(TWSTO);
    TWI_CLEAR_BUFF;
    twi.pEventFn = NULL;
    twi.st &= TWI_ERROR;    // Set IDLE, de hiba bit marad
    TWDEBC('}');
}
static inline void     twiRelease()
{
    TWCR = (TWCR & TWCR_CMD_MASK & ~_BV(TWIE)) | _BV(TWINT);
    TWI_CLEAR_BUFF;
    twi.pEventFn = NULL;
    twi.st &= TWI_ERROR;
    TWDEBC('$');
}

static inline uint8_t twiRecRem()
{
    return twi.rSize - twiRecIx();
}
static inline uint8_t twiRecByte()
{
    char c = TWDR;
    if (twiRecRem()) {
        *twi.pB++ = c;
#if TWDEBUG && DEBUG
        if (isalnum(c)) DEBP1("(%c)", c); else DEBP1("(\\%x)", c);
#endif
        return 1;
    }
    else {
#if TWDEBUG && DEBUG
        if (isalnum(c)) DEBP1("\\%c\\", c); else DEBP1("/\\%x/", c);
#endif
        return 0;
    }
}
static inline uint8_t twiSndRem()
{
    return twi.sSize - twiSndIx();
}

ISR(TWI_vect)
{
    alive |= ALIVE_TWI;
    uint8_t twst = TW_STATUS;
    switch (TW_STATUS) {
    /***** Master *****/
      case TW_START:                // start condition transmitted
      case TW_REP_START:            // repeated start condition transmitted
        // Elküldjük a Slave címét
        twiSendByte(twi.sla);
        TWDEBP1("[@%d]", twi.sla);
        break;
    /***** Master Transmitter *****/
      case TW_MT_SLA_ACK:           // SLA+W transmitted, ACK received
      case TW_MT_DATA_ACK:          // data transmitted, ACK received
        // Ha van még mit küldeni
        if (twiSndRem()) {
            char c = *(twi.pB)++;
            twiSendByte(c);
            TWDEBOUTC(c);
        }
        else {
            uint8_t r = twiEvent(TWI_MASTER_SENDED);
            // Ha a küldés kész, de nincs fogadás, vagy abort
            if   (twi.rBuff == NULL || r != TWI_OK_RESULT) twiStop();
            // A küldés megvolt, indítjuk az adat fogadást...
            else                                    twiReStartReceive();
        }
        break;
      case TW_MT_SLA_NACK:          // SLA+W transmitted, NACK received
      case TW_MT_DATA_NACK:         // data transmitted, NACK received
        TWDEBC('!');
        if (TWI_OK_RESULT == twiError(TWI_ERROR | TWI_MASTER_SEND)) twiReStartSend();
        else                                                        twiStop();
        break;
      case TW_MT_ARB_LOST:          // arbitration lost in SLA+W or data
//    case TW_MR_ARB_LOST:          // arbitration lost in SLA+R or NACK
        TWDEBC('#');
        if (twi.st == TWI_MASTER_TX) {
            if (TWI_OK_RESULT == twiError(TWI_ERROR | TWI_MASTER_SEND)) {
                twiReStartSend();
                break;
            }
        }
        else {
            if (TWI_OK_RESULT == twiError(TWI_ERROR | TWI_MASTER_RECEIVE)) {
                twiReStartReceive();
                break;
            }
        }
        twiRelease();
        break;

    /***** Master Receiver *****/
      case TW_MR_SLA_ACK:           // SLA+R transmitted, ACK received
        if (twiRecRem() > 1) {
            TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT) | _BV(TWEA);
        }
        else {
            TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT);
            TWDEBC('>');
        }
        break;
      case TW_MR_DATA_ACK:          // data received, ACK returned
        if (twiRecByte()) {
            if (twiRecRem() > 1) {
                TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT) | _BV(TWEA);
            }
            else {
                TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT);
                TWDEBC('>');
            }
        }
        else {
            twi.st = TWI_ERROR;
            if ((twi.sSize > 0 || twi.rSize > 0)
             && TWI_OK_RESULT == twiEvent(TWI_MASTER_RECEIVED | TWI_ERROR)) {
                if (twi.sSize > 0) twiReStartSend();
                else               twiReStartReceive();
            }
            else twiStop();
        }
        break;
      case TW_MR_DATA_NACK:         // data received, NACK returned
        if (twiRecByte()) {
            twiEvent(TWI_MASTER_RECEIVED);
        }
        else {
            twiEvent(TWI_MASTER_RECEIVED | TWI_ERROR);
            twi.st |= TWI_ERROR;
        }
        twiStop();
        break;
      case TW_MR_SLA_NACK:          // SLA+R transmitted, NACK received
        if (TWI_OK_RESULT == twiError(TWI_ERROR | TWI_MASTER_RECEIVE)) twiReStartReceive();
        else                                                           twiStop();
        break;
    /***** Slave Receiver *****/
      case TW_SR_SLA_ACK:           // SLA+W received, ACK returned
        twi.st = TWI_SLAVE_RX;
        if (TWI_OK_RESULT == twiEvent(TWI_SLAVE_RECEIVE)) {
            TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT) | _BV(TWEA);
        }
        else {
            TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT);
            twi.st = TWI_ERROR;
        }
        break;
      case TW_SR_ARB_LOST_SLA_ACK:  // arbitration lost in SLA+RW, SLA+W received, ACK returned
      case TW_SR_GCALL_ACK:         // general call received, ACK returned
      case TW_SR_ARB_LOST_GCALL_ACK:// arbitration lost in SLA+RW, general call received, ACK returned
      case TW_SR_GCALL_DATA_ACK:    // general call data received, ACK returned
      case TW_SR_GCALL_DATA_NACK:   // general call data received, NACK returned
      case TW_SR_DATA_NACK:         // data received, NACK returned
        TWI_SLAVE_RST;
        TWDEBP1("\\%02x\\]", twst);
        twi.st = TWI_ERROR;
        break;
      case TW_SR_DATA_ACK:          // data received, ACK returned
        twiRecByte();
        if (twiRecRem()) {
            TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT) | _BV(TWEA);
        }
        else {
            TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT);
            twi.st = TWI_IDLE;
            twiEvent(TWI_SLAVE_RECEIVED);
            twi.rBuff = NULL; twi.rSize = 0;
        }
        break;
      case TW_SR_STOP:              // stop or repeated start condition received while selected
        TWI_SLAVE_RST
        if ((twi.st & ~TWI_ERROR) == TWI_IDLE) {
           TWDEBP0("[\\T]");
        }
        else {
            twi.st = TWI_ERROR;
        }
        /***** Slave Transmitter *****/
      case TW_ST_SLA_ACK:           // SLA+R received, ACK returned
        twi.st = TWI_SLAVE_TX;
        if (TWI_OK_RESULT == twiEvent(TWI_SLAVE_SEND) && twi.sBuff != NULL && twiSndRem()) {
            char c = *(twi.pB)++;
            twiSendByte(c);
            TWDEBC('@'); TWDEBOUTC(c);
        }
        else {
            twiSendByte(0);
            twi.st = TWI_ERROR;
            TWDEBP0("@[\\0]");
        }
        break;
      case TW_ST_DATA_ACK:          // data transmitted, ACK received
        if (twi.st == TWI_SLAVE_TX && twiSndRem()) {
            char c = *(twi.pB)++;
            twiSendByte(c);
            TWDEBOUTC(c);
        }
        else {
            twiSendByte(0);
            TWDEBP0("(\\0)");
            twi.st = TWI_SLAVE_TX | TWI_ERROR;
        }
        break;
      case TW_ST_ARB_LOST_SLA_ACK:   // arbitration lost in SLA+RW, SLA+R received, ACK returned
        TWCR = (TWCR & TWCR_CMD_MASK) | _BV(TWINT);
        TWDEBC('-');
        break;
      case TW_ST_DATA_NACK:         // data transmitted, NACK received
      case TW_ST_LAST_DATA:         // last data byte transmitted, ACK received
        TWI_SLAVE_RST;
        TWDEBP1(">%02x", twst);
        twi.st = TWI_IDLE;
        twiEvent(TWI_SLAVE_SENDED);
        twi.sBuff = NULL; twi.sSize = 0;
        break;
    /***** Misc *****/
      case TW_NO_INFO:              // no state information available
        if (config.type == MASTER) twiStop();
        else                       TWI_SLAVE_RST;
        TWDEBC('?');
        break;
      case TW_BUS_ERROR:            // illegal start or stop condition
          if (config.type == SLAVE) TWI_SLAVE_RST;
          TWDEBC('~');
        break;
    }
}

