#ifndef ICONTSRV_H
#define ICONTSRV_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#include "firmware/IndAlarmIf1/indalarmif1_gl.h"
#include "firmware/IndAlarmIf2/indalarmif2_gl.h"
#include "QtSerialPort/QtSerialPort"
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define APPNAME "icontsrv"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

// A használlt szolgálltatás nevek
extern const QString _sTcpRs;
extern const QString _sIndAlarmIf1ma;
extern const QString _sIndAlarmIf1sl;
extern const QString _sIndAlarmIf2;

/// @class Egy védett eszköz pszeudo szolgáltatást reprezentáló objektum
/// A szolgáltatás a védet nod passzív virtuális szolgáltatása
/// A pPort adattag viszont eltérően az őstől a szenzor portot tartalmazza
/// nem pedig a virtuális szolgáltatáshoz rendelt attach típusú portot.
class cAttached : public cInspector {
public:
    /// Konstruktor.
    cAttached(cInspector * par) : cInspector(par) { ; }
    virtual void postInit(QSqlQuery &q, const QString &qs = _sNul);
    char            setting;    ///< Senzor port settings (indalarmif1 reported)
    char            state;      ///< Senzor port status (indalarmif1 reported)
};

class cGateway;

/// @class slaveIf
/// Az objektum egy IndAlarmIf1 slave egységet, és a hozzá kapcsolt sensors objektumokat reprezentálja
/// Az IndAlarmIf2 -nél nincsen slave objektum. A master egységet reprezentáló objektum ebből származik.
/// Az ős objektum a host-ot, és a virtuális szolgáltatást reprezentálja
class cIndAlarmIf : public cInspector {
public:
    /// A par nem feltétlenül a cIspector fában a szülőre mutat,
    /// Az inerface V1 slave-ek esetén par a master-re mutat, a fában viszont ezek mellérendeltek.
    cIndAlarmIf(QSqlQuery& q, qlonglong shid, qlonglong hoid, cInspector * par);
    /// Az ős metódus hívása után feltölti a pSubordinates-t (superior=custom) a linkek alapján,
    /// A linkelt eszközöknél ha nem létezik az attached szolgáltatás, vagy más a superior,
    /// akkor korrigálja az adatbázist is.
    virtual void postInit(QSqlQuery &q, const QString &qs = _sNul);
    /// A lekérdezést végző virtuális metódus.
    /// @param q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual int run(QSqlQuery& q, QString& runMsg);
    int query(cGateway &g, QSqlQuery &q, QString &msg);
    enum eNotifSwitch sets(cGateway &g,  QSqlQuery &q);
    /// A szolgáltatás típusa (lekérdezendő eszköz típus)
    enum eIAIfType {
        IAIF1M, ///< IndAlarmIf Version 1 Master
        IAIF1S, ///< IndAlarmIf Version 1 Slave
        IAIF2   ///< IndAlarmIf Version 2
    } type;
    bool    firstTime;
    int     masterId, slaveId;
    /// A LED-es szenzorok száma
    int     sensors1Count;
    /// A TTL szenzorok száma
    int     sensors2Count;
    // Szolgálltatás objektumok (lv2icontsrv konstruktora tölti fel)
    static const cService *pIndAlarmIf1ma;  ///< IndAlarmIf1m (IndAlarmIf V1 master)
    static const cService *pIndAlarmIf1sl;  ///< IndAlarmIf1s (IndAlarmIf V1 slave)
    static const cService *pIndAlarmIf2;    ///< IndAlarmIf2 (IndAlarmIf V2)
};

class cGateway : public cInspector{
public:
    /// Konstruktor
    cGateway(QSqlQuery& q, qlonglong hsid, qlonglong hoid, cInspector * par);
    ~cGateway();
    /// Leklrdező objektumok (cIndAlarmIf) beolvasása / pSubordinates feltöltése
    virtual void setSubs(QSqlQuery& q, const QString& qs = _sNul);
    virtual void threadPreInit();
    virtual int run(QSqlQuery &q, QString &runMsg);
    int open(QSqlQuery& q, QString &msg);
    void close();
    bool reOpen(QSqlQuery& q);
    QIODevice *getIoDev();
    bool waitForRead(int msec);
    bool waitForWritten(int msec);
    bool com_s(void * io, size_t size);
    /// Egy query blokk küldése, és válasz fogadása
    int com_q(struct qMsg  * out, struct aMsg  * in, QString &msg);
    /// Egy query blokk küldése, és válasz fogadása
    int com_q(struct qMsg2 * out, struct aMsg2 * in, QString &msg);

    int com_err(QString &msg, const QString &data, const QString &dropped);

    /// A gateway típusa (a prime_service_id alapján)
    enum ePrime {
        EP_INVALID = -1,
        EP_LOCAL  = 0,      ///< Lokális soros port
        EP_TCP_RS = 1       ///< TCP IP - soros port konverter
    } type;
private:
    void getSerialParams();
    bool openSerial(QString &msg);
    void getSocketParams();
    bool openSocket();
    /// A serial port objektum pointere, ha a kommunikáció közvetlenül egy serial porton keresztül történik.
    QSerialPort  *pSerio;
    // Serial params:
    static const QString    sSerialDefault;
    qint32                  baudRate;
    QSerialPort::DataBits   dataBits;
    QSerialPort::StopBits   stopBits;
    QSerialPort::FlowControl flowControl;
    QSerialPort::Parity     parity;
    /// A TCP socket pointere, ha egy TCP socketen kersztül történik a kommunikáció.
    QTcpSocket *pSock;
    // Socket params
    QHostAddress            sockAddr;   ///< TCP address, ha a típus EP_TCP_RS, egyébként nem használt
    int                     sockPort;   ///< TCP port, ha a típus EP_TCP_RS, egyébként nem használt
};

/// @class cIndaCont
/// Saját root szolgáltatás lekérdező objektum
class cIndaContact : public cInspector {
public:
    cIndaContact(QSqlQuery& q, const QString& sn);
    ~cIndaContact();
    /// Egy gateway objektum létrehozása
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *par);
};

class lv2icontsrv : public lanView {
    Q_OBJECT
public:
    lv2icontsrv();
    ~lv2icontsrv();
    /// Indulás
    virtual void setup(eTristate _tr = TS_NULL);
    bool       allPortEnabled;
    static void staticInit(QSqlQuery *pq);
};

// Az AVR-GCC -ben használt típusok (firmware)
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;

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


#endif // ICONTSRV_H
