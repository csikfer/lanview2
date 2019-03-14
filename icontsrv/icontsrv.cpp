#include "icontsrv.h"
#include "time.h"
#include <sys/ioctl.h>
#include "lv2link.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   99
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString _sIndAlarmIf1ma("indalarmif1ma");
const QString _sIndAlarmIf1sl("indalarmif1sl");
const QString _sIndAlarmIf2("indalarmif2");
const QString _sTcpRs("tcp.rs");
const QString _sAttached("attached");

const QString& setAppHelp()
{
    return _sNul;
}

int main (int argc, char * argv[])
{
    QCoreApplication app(argc, argv);

    SETAPP();
    lanView::gui        = false;
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2icontsrv   *pmo = new lv2icontsrv;

    if (pmo->lastError) {  // Ha hiba volt, vagy vége
        int r = pmo->lastError->mErrorCode;
        delete pmo;
        return r; // a mo destruktora majd kiírja a hibaüzenetet.
    }

    int r = app.exec();
    PDEB(INFO) << QObject::trUtf8("Az esemény hurok kilépett.") << endl;
    r = pmo->lastError == nullptr ? r : pmo->lastError->mErrorCode;
    delete pmo;
    exit(r);
}

lv2icontsrv::lv2icontsrv() : lanView()
{
    if (lastError == nullptr) {
        try {
            allPortEnabled     = false;
            int i;
            if (0 < (i = findArg(QChar('E'), "enabled-all-ports", args))) {
                args.removeAt(i);
                allPortEnabled = true;
            }
            setSelfStateF = true;
            insertStart(*pQuery);
            subsDbNotif();
            setup(TS_TRUE);
        } CATCHS(lastError)
    }
}

lv2icontsrv::~lv2icontsrv()
{
    DBGFN();
    down();
    pDelete(pQuery);
    DBGFNL();
}

void lv2icontsrv::staticInit(QSqlQuery *pq)
{
    cIndAlarmIf::pIndAlarmIf1ma = cService::service(*pq, _sIndAlarmIf1ma);
    cIndAlarmIf::pIndAlarmIf1sl = cService::service(*pq, _sIndAlarmIf1sl);
    cIndAlarmIf::pIndAlarmIf2   = cService::service(*pq, _sIndAlarmIf2);
}

void lv2icontsrv::setup(eTristate _tr)
{
    staticInit(pQuery);
    tSetup<cIndaContact>(_tr);
}

// -----------------------------------------------------------------------------------------

cIndaContact::cIndaContact(QSqlQuery& q, const QString& sn) : cInspector(q, sn)
{
    ;
}

cIndaContact::~cIndaContact()
{
    ;
}

cInspector * cIndaContact::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *par)
{
    cInspector *p = new cGateway(q, hsid, hoid, par);
    return p;
}

// -----------------------------------------------------------------------------------------

const QString    cGateway::sSerialDefault = "19200 7E1 No";

cGateway::cGateway(QSqlQuery& q, qlonglong hsid, qlonglong hoid, cInspector * par)
    : cInspector(q, hsid, hoid, par)
{
    pSock       = nullptr;
    pSerio      = nullptr;
    baudRate    = QSerialPort::Baud19200;
    dataBits    = QSerialPort::Data8;
    stopBits    = QSerialPort::OneStop;
    flowControl = QSerialPort::NoFlowControl;
    parity      = QSerialPort::NoParity;
    type        = EP_INVALID;
}

cGateway::~cGateway()
{
    ;
}

void cGateway::setSubs(QSqlQuery &q, const QString &)
{
    _DBGFN() << name() << endl;
    if (pSubordinates == nullptr) EXCEPTION(EPROGFAIL);
    QSqlQuery q2 = getQuery();
    QSqlQuery q3 = getQuery();
    static const QString sql =
            "SELECT hs.host_service_id, h.tableoid"
            " FROM host_services AS hs JOIN nodes AS h USING(node_id) JOIN services AS s USING(service_id)"
            " WHERE hs.superior_host_service_id = %1"
              " AND NOT s.deleted  AND NOT hs.deleted"
              " AND NOT s.disabled AND NOT hs.disabled";
    if (!q.exec(sql.arg(hostServiceId()))) SQLPREPERR(q, sql);
    if (q.first()) do {
        qlonglong       hsid = q.value(0).toLongLong();  // host_service_id      A szervíz rekord amit be kell olvasni
        qlonglong       hoid = q.value(1).toLongLong();  // node tableoid        A node typusa
        cIndAlarmIf *p = new cIndAlarmIf(q2, hsid, hoid, this);
        p->postInit(q2);
        *pSubordinates << p;
        if (p->type == cIndAlarmIf::IAIF1M) {        // Az iaif1 master-slave -eket mellérendeltként kezeljük
            static const QString sql2 =
                    "SELECT hs.host_service_id, h.tableoid"
                    " FROM host_services AS hs JOIN nodes AS h USING(node_id) JOIN services AS s USING(service_id)"
                    " WHERE hs.superior_host_service_id = %1 AND hs.service_id = %2"
                      " AND NOT s.deleted  AND NOT hs.deleted"
                      " AND NOT s.disabled AND NOT hs.disabled";
            QString s = sql2.arg(p->hostServiceId()).arg(cIndAlarmIf::pIndAlarmIf1sl->getId());
            if (!q2.exec(s)) SQLPREPERR(q, s);
            if (q2.first()) do {
                qlonglong       hsid2 = q2.value(0).toLongLong();  // host_service_id      A szervíz rekord amit be kell olvasni
                qlonglong       hoid2 = q2.value(1).toLongLong();  // node tableoid        A node typusa
                cIndAlarmIf *ps = new cIndAlarmIf(q3, hsid2, hoid2, p);
                ps->postInit(q3);
                *pSubordinates << ps;
            } while (q2.next());
        }
    } while (q.next());

    if (0 == protoServiceName().compare(_sLocal, Qt::CaseInsensitive)) {
        type = EP_LOCAL;
        getSerialParams();
    }
    else if (0 == protoServiceName().compare(_sTcpRs, Qt::CaseInsensitive)) {
        type = EP_TCP_RS;
        getSocketParams();
    }
    else {
        EXCEPTION(ENOTSUPP, protoServiceId(), QObject::trUtf8("A megadott proto_service nem támogatptt"))
    }

}

void cGateway::preInit()
{
    ;
}

void cGateway::threadPreInit()
{
    ;
}

int cGateway::run(QSqlQuery &q, QString &runMsg)
{
    (void)runMsg;
    (void)q;
    return RS_STAT_SETTED | RS_ON;  // A lekérdezések beállítják a statust (többször is)
}

int cGateway::open(QSqlQuery& q, QString& msg, int to)
{
    DBGFN();
    (void)q;
    bool r;
    switch (type) {
    case EP_LOCAL:  /* OPEN Serial Port */
        r = openSerial(msg);
        break;
    case EP_TCP_RS:
        r = openSocket(to);
        if (!r) {
            msg = trUtf8("Open socket[%1:%2] time out.").arg(sockAddr.toString()).arg(sockPort);
        }
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    _DBGFNL() << VDEB(r) << endl;
    return r ? RS_ON : RS_UNREACHABLE;
}

void cGateway::close()
{
    DBGFN();
    switch (type) {
    case EP_LOCAL:
        /* Serial Port */
        if (pSerio != nullptr) {
            pSerio->close();
        }
        else {
            DERR() << "pSerio is NULL" << endl;
        }
        break;
    case EP_TCP_RS:
        /* TCP SOCKET */
        if (pSock != nullptr) {
            pSock->close();
        }
        else {
            DERR() << "pSock is NULL" << endl;
        }
        break;
    default: EXCEPTION(EPROGFAIL);
    }
    DBGFNL();
}

void cGateway::getSerialParams()
{
    DBGFN();
    if (pPort == nullptr)
        EXCEPTION(EDATA, -1, QObject::trUtf8("Nincs megadva lokális soros interfész."));
    if (pPort->ifType().getName() != _sRS485 && pPort->ifType().getName() != _sRS232)
        EXCEPTION(EDATA, -1, QObject::trUtf8("A lekérdezéshez csak RS485-típusú port adható meg."));
    QString s = pPort->getName();
    const static QString devdir = "/dev/";
    if (s.indexOf("/dev/") != 0) s = devdir + s;
    pSerio = new QSerialPort(s, useParent());
    s = feature(_sSerial);                  // A serial port beállításai
    if (s.isEmpty()) s = sSerialDefault;    // Az alapértelmezés
    QStringList sl = s.split(QChar(' '));
    QString e = QString(trUtf8("Nem megfelelő soros port paraméter string: %1")).arg(s);    // hibaüzenet, ha nem lenne ok.
    if (sl.size() < 2 || sl[1].size() != 3) EXCEPTION(EDATA, -1, e);
    bool ok;
    baudRate = sl[0].toLong(&ok);
    if (!ok) EXCEPTION(EDATA, -1, e);
    // -----------------------
    qint32  validBaudRates[] = { // Megengede baud értékek
        QSerialPort::Baud1200,  QSerialPort::Baud2400,  QSerialPort::Baud4800,  QSerialPort::Baud9600,
        QSerialPort::Baud19200, QSerialPort::Baud38400, QSerialPort::Baud57600, QSerialPort::Baud115200 };
    for (uint i = 0; ; ++i) {
        if (i >= sizeof(validBaudRates)) EXCEPTION(EDATA, baudRate, e);
        if (baudRate == validBaudRates[i]) break;
    }
    // -----------------------
    char c = sl[1][0].toLatin1();
    switch (c) {             // Adat bitek száma
    case '7':   dataBits = QSerialPort::Data7;  break;
    case '8':   dataBits = QSerialPort::Data8;  break;
    default:    EXCEPTION(EDATA, -1, e);
    }
    // -----------------------
    c = sl[1][2].toLatin1();
    switch (c) {             // Stop bit
    case '1':   stopBits = QSerialPort::OneStop; break;
    case '2':   stopBits = QSerialPort::TwoStop; break;
    default:    EXCEPTION(EDATA, -1, e);
    }
    // -----------------------
    if (sl.size() > 2) switch(toupper(sl[2].at(0).toLatin1())) {
    case 'N':   flowControl = QSerialPort::NoFlowControl;    break;
    case 'H':   flowControl = QSerialPort::HardwareControl;  break;
    case 'X':
    case 'S':   flowControl = QSerialPort::SoftwareControl;  break;
    default:    EXCEPTION(EDATA, -1, e);
    }
    // -----------------------
    c = toupper(sl[1][1].toLatin1());
    switch (c) {             // Paritás bit
    case 'N':   parity = QSerialPort::NoParity;   break;
    case 'O':   parity = QSerialPort::OddParity;  break;
    case 'E':   parity = QSerialPort::EvenParity; break;
    case 'M':   parity = QSerialPort::MarkParity; break;
    case 'S':   parity = QSerialPort::SpaceParity;break;
    default:    EXCEPTION(EDATA, -1, e);
    }
    // DEBUG :
    PDEB(VVERBOSE) << "Serial device " << pSerio->portName() << endl;
    PDEB(VVERBOSE) << "\tBaud rate : " << baudRate << ", " << dataBits << " data, "
                   << parity << " parity, " << stopBits << " stop bit, Flow control "
                   << flowControl << endl;
}


bool cGateway::openSerial(QString& msg)
{
    DBGFN();
    if (pSerio == nullptr) EXCEPTION(EPROGFAIL, -1, trUtf8("Nincs serial objektumunk."));
    if (!pSerio->open(QIODevice::ReadWrite)) {
        DERR() << "Serial port " << pSerio->portName() << " (first) open error : " << pSerio->errorString() << endl;
        if (!pSerio->open(QIODevice::ReadWrite)) {   // Elsőre nem sikerül, ha nem volt lezárva rendesen
            msg = trUtf8("Serial port %1; Error string : %2").arg(pSerio->portName(),pSerio->errorString());
            DERR() << msg << endl;
            return false;
        }
    }
    pSerio->setBaudRate(baudRate);
    pSerio->setDataBits(dataBits);
    pSerio->setStopBits(stopBits);
    pSerio->setFlowControl(flowControl);
    pSerio->setParity(parity);
    // DEBUG :
    PDEB(VVERBOSE) << "Serial device " << pSerio->portName() << " opened." << endl;
    return true;
}

void cGateway::getSocketParams()
{
    bool ok;
    sockPort = feature(_sTcp).toInt(&ok);
    if (!ok)  sockPort = protoService().getId(_sPort);
    if (sockPort < 1 || sockPort > 65535) EXCEPTION(EDATA, sockPort, feature(_sTcp));
    QSqlQuery qq = getQuery();
    if (checkThread(&host())) {
        host().fetchPorts(qq);  // Az IP cím kitalálásához kelleni fognak a portok
        sockAddr = host().getIpAddress();
    }
    else {
        cNode h(host());    // Pampog, ha másik szálé az objektum, másolatot készítünk
        h.fetchPorts(qq);
        sockAddr = h.getIpAddress();
    }
    if (sockAddr.isNull()) EXCEPTION(EDATA, -1, QObject::trUtf8("Invalid host address, or address not found."));
    QObject *p = useParent();
    QString m = trUtf8("%1(%2) : current %3").arg(p2string(p), p2string(p->thread()), p2string(QThread::currentThread()));
    PDEB(VVERBOSE) << m << endl;
    pSock = new QTcpSocket(p);
    PDEB(VVERBOSE) << "Socket : " << sockAddr.toString() << ":" << sockPort << endl;
}

bool cGateway::openSocket(int to)
{
    if (pSock == nullptr) EXCEPTION(EPROGFAIL, -1, trUtf8("Nincs socket objektumunk."));
    pSock->connectToHost(sockAddr, sockPort);
    return pSock->waitForConnected(to);
}

QIODevice *cGateway::getIoDev()
{
    switch (type) {
    case EP_LOCAL:  return pSerio;
    case EP_TCP_RS: return pSock;
    default:        EXCEPTION(EPROGFAIL);
    }
    return nullptr;    // !warning
}

bool cGateway::waitForWritten(int msec)
{
    bool r = false;
    switch (type) {
    case EP_LOCAL: {
        QElapsedTimer t;
        t.start();
        quint64 w = msec;
        do {
            r = pSerio->waitForBytesWritten(msec);
            PDEB(VVERBOSE) << "serio->waitForBytesWritten result : " << DBOOL(r) << endl;
            if (r) break;
            w = msec - t.elapsed();
        } while (w > 0);
        break;
    }
    case EP_TCP_RS:
        r = pSock->waitForBytesWritten(msec);
        PDEB(VVERBOSE) << "sock->waitForBytesWritten result : " << DBOOL(r) << endl;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    return r;
}

bool cGateway::waitForRead(int msec)
{
    bool r = false;
    switch (type) {
    case EP_LOCAL: {
        QElapsedTimer t;
        t.start();
        qint64 w = msec;
        do {
            r = pSerio->waitForReadyRead(w);
            PDEB(VVERBOSE) << "serio->waitForReadyRead result : " << DBOOL(r) << endl;
            if (r) break;
            w = msec -  t.elapsed();
        } while (w > 0);
        break;
    }
    case EP_TCP_RS:
        r = pSock->waitForReadyRead(msec);
        PDEB(VVERBOSE) << "sock->waitForReadyRead result : " << DBOOL(r) << endl;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    return r;
}

/// Egy command blokk küldése és a válasz vétele
bool cGateway::com_s(void * io, size_t size, cInspector *pInsp)
{
    DBGFN();
    QIODevice * pIO = getIoDev();
    if (pIO == nullptr) {
        DERR() << "IO Device is NULL." << endl;
        return false;
    }
    unsigned long to = pInsp->stopTimeOut;
    PDEB(INFO) << QObject::trUtf8("Küldés : ") << QByteArray(static_cast<char *>(io), int(size)) << endl;
    qint64  wr = pIO->write(static_cast<char *>(io), qint64(size));
    PDEB(VVERBOSE) << "Write result : " << wr << endl;
    if (wr < 0) {
        DERR() << "Write error." << endl;
        return false;
    }
    bool wt = waitForWritten(pInsp->startTimeOut);
    if (!wt) {
        DERR() << "Wait for writen error." << endl;
        return false;
    }
    size_t   n = 0;
    QElapsedTimer   t;
    t.start();
    while ((to = pInsp->stopTimeOut - t.elapsed()) > 0) {
        bool r = waitForRead(to);
        if (!r) break;
        char c;
        while (pIO->getChar(&c)) {
            // A válasz ua., első két karakter: '#'
            if ((n < 2 && c == '#') || (n >= 2 && c == ((char *)io)[n])) {
                ++n;
                if (n == size) return true;
            }
            else {
                PDEB(VVERBOSE) << "Drop n = " << n << " c :" <<  c << endl;
                n = 0;
            }
        }
    }
    _DBGFNL() << " failed" << endl;
    return false;  // nem sikerült
}

static inline bool ishex(char c) { return isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f'); }
/// Egy query blokk küldése, és válasz fogadása
int cGateway::com_q(struct qMsg * out, struct aMsg * in, QString &msg, cInspector *pInsp)
{
    DBGFN();
    QIODevice * pIO = getIoDev();
    if (pIO == nullptr) {
        msg = trUtf8("IO Device is NULL.");
        DERR() << msg << endl;
        return RS_UNREACHABLE;
    }
    int to = pInsp->stopTimeOut;
    static const char kstat[] = "ONDAISCUEaiscue";
    PDEB(INFO) << QObject::trUtf8("Küldés : ") << QByteArray((char *)out, sizeof(struct qMsg)) << endl;
    qint64  wr = pIO->write((char *)out, sizeof(struct qMsg));
    PDEB(VVERBOSE) << "Write result : " << wr << endl;
    if (wr < 0) {
        msg = trUtf8("Write error.");
        DERR() << msg << endl;
        return RS_UNREACHABLE;
    }
    bool wt = waitForWritten(pInsp->startTimeOut);
    if (!wt) {
        msg = trUtf8("Quit for writen error.");
        DERR() << msg << endl;
        return RS_UNREACHABLE;
    }
    int   n = 0;
    QElapsedTimer   t;
    t.start();
    QString data, dropped;
    while ((to = pInsp->stopTimeOut - t.elapsed()) > 0) {
        bool r = waitForRead(to);
        if (!r) break;
        char c;
        while (pIO->getChar(&c)) {
            switch (n) {
            case 0: case 1:
                if (c == '.') {
                    in->beginMark[n] = c;
                    break;
                }
                goto char_drop;
            case 2: case 3:
                if (c == out->tMaster[n -2]) {
                    in->sMaster[n -2] = c;
                    break;
                }
                goto char_drop;
            case 4: case 5:
                if (c == out->tSlave[n -4]) {
                    in->sSlave[n -4] = c;
                    break;
                }
                goto char_drop;
            case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 13:
                if (strchr(kstat, c) != nullptr) {
                    in->kontakt[n -6] = c;
                    break;
                }
                goto char_drop;
            case 14: case 15: case 16: case 17: // errstat
            case 18: case 19: case 20: case 21: // counter
                if (ishex(c)) {
                    in->errstat[n -14] = c; // errstat + counter
                    break;
                }
                goto char_drop;
            case 22:
                if (c == '\r') break;
                goto char_drop;
            case 23:
                if (c == '\n') {
                    /* PDEB(INFO) */  cDebug::cout() << "Received: " << QByteArray((char *)in, sizeof(struct aMsg)) << endl;
                    return RS_ON;
                }
                goto char_drop;
            }
            ++n;
            data += c;
            continue;
            char_drop:
            PDEB(VVERBOSE) << "Drop n = " << n << " str :" << QByteArray((char *)in, n) << c << endl;
            n = 0;
            dropped += data + c;
            data.clear();
        }
    }
    return com_err(msg, data, dropped);
}

/// Egy query blokk küldése, és válasz fogadása
int cGateway::com_q(struct qMsg2 * out, struct aMsg2 * in, QString& msg, cInspector *pInsp)
{
    DBGFN();
    QIODevice * pIO = getIoDev();
    if (pIO == nullptr) {
        msg = trUtf8("IO Device is NULL.");
        DERR() << msg << endl;
        return RS_UNREACHABLE;
    }
    int to = pInsp->stopTimeOut;
    static const char kstat[] = "ONDAISCUEaiscue";
    PDEB(INFO) << QObject::trUtf8("Küldés : ") << QByteArray((char *)out, sizeof(struct qMsg2)) << endl;
    qint64  wr = pIO->write((char *)out, sizeof(struct qMsg2));
    PDEB(VVERBOSE) << "Write result : " << wr << endl;
    if (wr < 0) {
        msg = trUtf8("Write error.");
        DERR() << msg << endl;
        return RS_UNREACHABLE;
    }
    bool wt = waitForWritten(pInsp->startTimeOut);
    if (!wt) {
        msg = trUtf8("Quit for writen error.");
        DERR() << msg << endl;
        return RS_UNREACHABLE;
    }
    int   n = 0;
    QElapsedTimer   t;
    t.start();
    QString data, dropped;
    while ((to = pInsp->stopTimeOut - t.elapsed()) > 0) {
        bool r = waitForRead(to);
        if (!r) break;
        char c;
        while (pIO->getChar(&c)) {
            switch (n) {
            case 0: case 1:
                if (c == '.') {
                    in->beginMark[n] = c;
                    break;
                }
                goto char_drop;
            case 2: case 3:
                if (c == out->tMaster[n -2]) {
                    in->sMaster[n -2] = c;
                    break;
                }
                goto char_drop;
            case  4: case  5: case  6: case  7: case  8: case  9: case 10: case 11:
            case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19:
                if (strchr(kstat, c) != nullptr) {
                    in->kontakt[n -4] = c;
                    break;
                }
                goto char_drop;
            case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27:
                if (strchr(kstat, c) != nullptr) {
                    in->swstat[n -20] = c;
                    break;
                }
                goto char_drop;
            case 28: case 29: case 30: case 31: // errstat
            case 32: case 33: case 34: case 35: // counter
                if (ishex(c)) {
                    in->errstat[n -28] = c; // errstat + counter
                    break;
                }
                goto char_drop;
            case 36:
                if (c == '\r') {
                    in->endMark[0] = c;
                    break;
                }
                goto char_drop;
            case 37:
                if (c == '\n') {
                    in->endMark[1] = c;
                    /* PDEB(INFO) */  cDebug::cout() << "Received: " << QByteArray((char *)in, sizeof(struct aMsg2)) << endl;
                    return RS_ON;
                }
                goto char_drop;
            }
            ++n;
            data += c;
            continue;
            char_drop:
            PDEB(VVERBOSE) << "Drop n = " << n << " str :" << QByteArray((char *)in, n) << c << endl;
            n = 0;
            dropped += data + c;
            data.clear();
        }
    }
    return com_err(msg, data, dropped);
}

int cGateway::com_err(QString &msg, const QString& data, const QString&  dropped)
{
    int st;
    if (data.isEmpty() && dropped.isEmpty()) {
        msg = trUtf8("Időtullépés, nem jött semmilyen adat az interfésztől. ");
        st = RS_UNREACHABLE;
    }
    else {
        msg = trUtf8("Idő tullépés, ill. adat hiba : eldobva : %1, Feldolgozva : %2. ")
                .arg(dQuoted(data), dQuoted(dropped));
        st = RS_CRITICAL;
    }
    _DBGFNL() << msg << endl;
    return  st;  // nem sikerült
}

// -----------------------------------------------------------------------------------------

/// Az interfész által küldött státusz karakter alapján a port/szervíz státusz, ás üzenet megállapítása.
/// @par ks Az indakontakt interfész által vissaadott port állapot (ascii karakter)
/// @par sst A visszaadott szervíz státusz string.
/// @par nos A visszaadott szervíz státusz üzenet, ha van.
/// @par ost A visszaadott port státusz string.
/// @par ast A visszaadott port admin státusz string.
static void fromKStat(int ks, QString& sst, QString& nos, QString& ost, QString& ast)
{
    switch (ks) {
    case KOK:
        sst = _sOn;
        nos.clear();
        ast = ost = _sUp;
        break;
    case KNULL:
        sst = _sUnknown;
        nos.clear();
        ast = ost = sst;
        break;
    case KDISA:
        sst = _sWarning;
        nos = QObject::trUtf8("Az érzékelő le van tiltva.");
        ost = _sUnknown;
        ast = _sDown;
        break;
    case KALERT | KOLD:
    case KALERT:
        sst = _sDown;
        nos = QObject::trUtf8("Az érzékelő jelzett.");
        ost = _sDown;
        ast = _sUp;
        break;
    case KINV | KOLD:
    case KINV:
        sst = _sDown;
        nos = QObject::trUtf8("Az érzékelő jelzett, és fordítva van bekötve.");
        ost = _sDown;
        ast = _sInvert;
        break;
    case KSHORT | KOLD:
    case KSHORT:
        sst = _sCritical;
        nos = QObject::trUtf8("Érzékelő rövidzárat jelez. Szabotázs?");
        ost = _sShort;
        ast = _sUp;
        break;
    case KCUT | KOLD:
    case KCUT:
        sst = _sCritical;
        nos = QObject::trUtf8("Az érzékelő szakadást jelez. Szabotázs?");
        ost = _sBroken;
        ast = _sUp;
        break;
    case KANY | KOLD:
    case KANY:
        sst = _sUnreachable;
        nos = QObject::trUtf8("Érzékelő mérési hiba.");
        ost = _sError;
        ast = _sUp;
        break;
    case KERR | KOLD:
    case KERR:
        sst = _sUnreachable;
        nos = QObject::trUtf8("Érzékelő hiba.");
        ost = _sError;
        ast = _sUnknown;
        break;
    default:
        sst = _sUnknown;
        nos = QString(QObject::trUtf8("Az érzékelő állapota ismeretlen 0x%1.")).arg(ks,0,16);
        ast = ost = sst;
        break;
    }
}

static inline char n2c(int i)
{
    if (i < 10) return '0' + i;
    return 'A' + i - 10;
}

static void inline id2cc(int id, char * p)
{
    p[0] = n2c((id >> 4) & 0x0f);
    p[1] = n2c(id & 0x0f);
}


// -----------------------------------------------------------------------------------------

const cService *cIndAlarmIf::pIndAlarmIf1ma;  ///< IndAlarmIf1m (IndAlarmIf V1 master)
const cService *cIndAlarmIf::pIndAlarmIf1sl;  ///< IndAlarmIf1s (IndAlarmIf V1 slave)
const cService *cIndAlarmIf::pIndAlarmIf2;    ///< IndAlarmIf2 (IndAlarmIf V2)

cIndAlarmIf::cIndAlarmIf(QSqlQuery& q, qlonglong shid, qlonglong hoid, cInspector * par)
    : cInspector(q, shid, hoid, par)
{
    _DBGFN() << VDEB(shid) << _sCommaSp << VDEB(hoid) << ", parent : " << par->name() << endl;
    firstTime = true;
    masterId = slaveId = 0;
    sensors1Count = sensors2Count = 0;
    qlonglong seid = serviceId();
    if (seid == pIndAlarmIf1ma->getId()) {      // Interface V1 master
        type = IAIF1M;
        if (par->service()->getName() != _sRS485)
            EXCEPTION(EDATA, par->service()->getId(), QObject::trUtf8("Nem megfelelő szülő szolgáltatás típus."));
    }
    else if (seid == pIndAlarmIf1sl->getId()) { // Interface V1 slave
        type = IAIF1S;
        if (par->service()->getId() != pIndAlarmIf1ma->getId())
            EXCEPTION(EDATA, par->service()->getId(), QObject::trUtf8("Nem megfelelő szülő szolgáltatás típus."));
    }
    else if (seid == pIndAlarmIf2->getId()) {   // Interface V2
        type = IAIF2;
        if (par->service()->getName() != _sRS485)
            EXCEPTION(EDATA, par->service()->getId(), QObject::trUtf8("Nem megfelelő szülő szolgáltatás típus."));
    }
    // A szolgáltatás portja, ha nincs kitöltve, akkor pótoljuk
    if (pPort == nullptr) {
        QSqlQuery qq = getQuery();
        qlonglong ifTypeId;
        if (type != IAIF1S) {   // RS485
            ifTypeId = cIfType::ifTypeId(_sRS485);
        }
        else {                  // IIC
            ifTypeId = cIfType::ifTypeId(_sIIC);
        }
        if (host().ports.size() == 0) pNode->fetchPorts(qq);
        // Az első RS485 vagy IIC port indexe
        int i = pNode->ports.indexOf(_sIfTypeId, ifTypeId);
        if (i < 0) EXCEPTION(EFOUND, -1, name());
        // Van RS485 vagy IIC portunk, de nam lehet több!
        if (0 <= pNode->ports.indexOf(_sIfTypeId, cIfType::ifTypeId(_sRS485), i +1)) EXCEPTION(AMBIGUOUS, i, name());
        pPort = pNode->ports[i]->dup()->reconvert<cNPort>();    // Van port
        hostService.setId(_sPortId, pPort->getId());            // Kiírjuk az adatbázisba, ne keljen keresni
        hostService.update(qq, false, hostService.mask(_sPortId));
    }
    _DBGFNL() << name() << endl;
}

void cIndAlarmIf::postInit(QSqlQuery &q, const QString &)
{
    _DBGFN() << " / " << name() << endl;
    if (pSubordinates != nullptr) EXCEPTION(EDATA);
    inspectorType = IT_TIMING_TIMED;
    interval = variantToId(get(_sNormalCheckInterval), EX_IGNORE, -1);
    retryInt = variantToId(get(_sRetryCheckInterval),  EX_IGNORE, interval);
    if (interval <= 0) EXCEPTION(EDATA, interval, QObject::trUtf8("Időzített lekérdezés, időzítés nélkül."));
    pSubordinates = new QList<cInspector *>;
    cMac    mac(interface().getMac(_sHwAddress));   // és kell még az rs485 buszon az interfész azonosító byte
    if (!mac) EXCEPTION(EDATA);                 // ami a (nem valós) MAC-ba van rejtve
    masterId = (mac.toULongLong() >> 8) & 0xff; // annak alulról a második byte-ja
    slaveId  = (mac.toULongLong()) & 0xff;      // annak legalsó byte-ja
    if (masterId == 0) EXCEPTION(EDATA, masterId, QObject::trUtf8("Az RS485 Bus ID nem lehet nulla !"));
    switch (type) {
    case IAIF1M:
        PDEB(VVERBOSE) << "IaIf V1 master..." << endl;
        if (slaveId != 0) EXCEPTION(EDATA, slaveId, QObject::trUtf8("Master interfész, de az IIC Bus ID nem nulla !"));
        sensors1Count = IAIF1PORTS;
        sensors2Count = 0;
        break;
    case IAIF1S:
        PDEB(VVERBOSE) << "IaIf V1 slave..." << endl;
        if (slaveId == 0) EXCEPTION(EDATA, slaveId, QObject::trUtf8("Az IIC Bus ID nem lehet nulla !"));
        sensors1Count = IAIF1PORTS;
        sensors2Count = 0;
        break;
    case IAIF2:
        PDEB(VVERBOSE) << "IaIf V2..." << endl;
        if (slaveId != 0) EXCEPTION(EDATA, slaveId, QObject::trUtf8("Nincs IIC Bus, de a Bus ID nem nulla !"));
        sensors1Count = IAIF2PORTS;
        sensors2Count = IAIF2SWS;
        break;
    default:        EXCEPTION(EPROGFAIL, type, trUtf8("Invalid interface type."));
    }
    int allSensorCount = sensors1Count + sensors2Count;
    for (int i = 0; i < allSensorCount; ++i) (*pSubordinates) << nullptr;
    if (node().ports.size() == 0) node().fetchPorts(q);
    const qlonglong ptid = cIfType::ifType(_sSensor).getId();
    tRecordList<cNPort>::iterator i, n = node().ports.end();
    PDEB(VERBOSE) << "Find sensors..." << endl;
    for (i = node().ports.begin(); i != n; ++i) {
        cNPort *pp = *i;
        if (pp->getId(_sIfTypeId) != ptid) continue; /// Csak a sensor típusú portokkal foglalkozunk
        qlonglong lpid = cLogLink().getLinked(q, pp->getId()); // Mivel van linkelve?
        PDEB(INFO) << "port " << pp->getName() << ", ix = " << pp->getId(_sPortIndex)
                   << (lpid != NULL_ID ? QString(" -> #%1").arg(lpid) : _sNul)
                   << endl;
        if (lpid != NULL_ID) {  // Van link
            cNPort ap;
            ap.fetchById(q, lpid);  // A linkelt port
            int ix = pp->getId(_sPortIndex); // A port idex lessz a pSubordinates konténerben is az index.
            // De ez 1-től számozódik!!
            if (ix <= 0 || ix > allSensorCount) EXCEPTION(EDATA, ix, QObject::trUtf8("Nem megfelelő szenzor port index."));
            if ((*pSubordinates)[ix -1] != nullptr) EXCEPTION(EPROGFAIL, ix);
            cAttached& at = *(cAttached *)((*pSubordinates)[ix -1] = new cAttached(this));
            at.service(q, _sAttached);
            at.pNode = getObjByIdT<cNode>(q, ap.getId(_sNodeId));
            cHostService& hs = at.hostService;
            if (hs.fetchByIds(q, at.nodeId(), at.serviceId(), EX_IGNORE)) {
                if (hs.getId(_sSuperiorHostServiceId) != hostServiceId()) {  // máshova mutat a superior, a link szerint viszont a mienk !!!
                    hs.setId(_sSuperiorHostServiceId, hostServiceId());
                }
                if (hs.getId(_sPortId) != lpid) {                           // nem stimmel a port
                    hs.setId(_sPortId, lpid);
                }
                if (hs.isModify_()) {                                       // volt javítás, update
                    hs.update(q, true);
                }
                PDEB(INFO) << "Attached : " << at.name() << endl;
            }
            else {  // Nincs host_services rekordunk
                hs.clear();
                hs.setId(_sNodeId, at.nodeId());
                hs.setId(_sServiceId, at.serviceId());
                hs.setId(_sSuperiorHostServiceId, hostServiceId());
                hs.setId(_sPortId, lpid);
                hs.setName(_sHostServiceNote, trUtf8("Autómatikusan generálva az icontsrv által."));
                hs.setName(_sNoalarmFlag, _sOn);    // Nem kérünk riasztást az autómatikusan generált rekordhoz.
                hs.insert(q);
                PDEB(INFO) << "New Attached : " << at.name() << endl;
            }
            at.postInit(q);
        }
        else {
            PDEB(INFO) << "Not linked, drop." << endl;
        }
    }
    DBGFNL();
}

int cIndAlarmIf::run(QSqlQuery& q, QString &runMsg)
{
    DBGFN();
    cGateway *pGate;
    {
        cInspector *p = pParent;;
        if (p == nullptr) EXCEPTION(EPROGFAIL);
        if (type == IAIF1S) {               // Itt a Master interface a parent, de nekünk a gateway kell, ami az ő partentje.
            p = p->pParent;
            if (p == nullptr) EXCEPTION(EPROGFAIL);
        }
        if (typeid(*p) != typeid(cGateway)) EXCEPTION(EPROGFAIL);   // Egy cGateway objektum kell legyen.
        pGate = dynamic_cast<cGateway *>(p);
    }
    PDEB(VVERBOSE) << "Gateway : " << pGate->name() << " Open..." << endl;
    int r = pGate->open(q, runMsg, int(startTimeOut));
    if (r == RS_ON) {
        r = query(*pGate, q, runMsg);
        pGate->close();
        pGate->hostService.setState(q, _sOn, runMsg, lastRun.elapsed());
    }
    else {
        pGate->hostService.setState(q, notifSwitch(r), runMsg, lastRun.elapsed());
    }
    DBGFNL();
    return r;
}

int cIndAlarmIf::query(cGateway& g, QSqlQuery& q, QString& msg)
{
    static bool first = true;
    // IndAlarmIf1
    static struct qMsg  qm;
    struct        aMsg  am;
    // IndAlarmIf2
    static struct qMsg2 qm2;
    struct        aMsg2 am2;

    if (first) {       // Inicializáljuk a kérdés blokkokat, ha most járunk itt előszőr
        // IAIF1x
        strncpy(qm.beginMark,  "??",   sizeof(qm.beginMark));
        qm.clrErrSt = 'T';
        strncpy(qm.clrKontSt,  "FF",   sizeof(qm.clrKontSt));
        strncpy(qm.endMark,    "\r\n", sizeof(qm.endMark));
        // IAIF2
        strncpy(qm2.beginMark, "??",   sizeof(qm2.beginMark));
        qm2.clrErrSt = 'T';
        strncpy(qm2.clrKontSt, "FFFF", sizeof(qm2.clrKontSt));
        strncpy(qm2.clrSwSt,   "FF",   sizeof(qm2.clrSwSt));
        strncpy(qm2.endMark,   "\r\n", sizeof(qm2.endMark));
        first = false;
    }
    int rep = RS_UNREACHABLE;
    switch (type) {
    case IAIF1S:
    case IAIF1M:
        id2cc(masterId, qm.tMaster);
        id2cc(slaveId,  qm.tSlave);
        rep = g.com_q(&qm, &am, msg, this);
        break;
    case IAIF2:
        id2cc(masterId, qm2.tMaster);
        rep = g.com_q(&qm2, &am2, msg, this);
        break;
    default:        EXCEPTION(EPROGFAIL);
    }
    if (rep != RS_ON) return rep;
//    bool conf = false;
    int n = sensors1Count + sensors2Count;
    for (int i = 0; i < n; ++i) {
        int pi = node().ports.indexOf(_sPortIndex, QVariant(i +1));    // Keressük a portot, a ciklusváltozó a port_index mező értékével azonos,
        if (pi < 0) continue;                                       // Ha nincs portunk, akkor jöhet a következő
        cNPort  *pPrt = node().ports.at(pi);
        // Előkotorjuk a portra vonatkozó statusz karaktert.
        int ks = (type == IAIF2) ? (i < sensors1Count ? am2.kontakt[i]                  // IAIF2 alap szenzorok
                                                      : am2.swstat[i - sensors1Count])  // IAIF2 TTL szenzorok
                                 : (am.kontakt[i]);                                     // IAIF1 szenzorok
        QString sst, ost, ast, nos;
        fromKStat(ks, sst, nos, ost, ast);
        if (pPrt->getName(_sPortAStat) != ast) pPrt->setName(_sPortAStat, ast);
        if (pPrt->getName(_sPortOStat) != ost) pPrt->setName(_sPortOStat, ost);
        if (pPrt->isModify_()) pPrt->update(*pq, false, pPrt->mask(_sPortAStat, _sPortOStat));

        cAttached   *pa = (cAttached *)pSubordinates->at(i);
        if (pa == nullptr) continue;
        pa->hostService.setState(q, sst, nos, lastRun.elapsed(), g.parentId());
    }
//    if (conf) sets(qs);
    return RS_ON;
}

 /*
enum eNotifSwitch  cIndAlarmIf::sets(cGateway &g, QSqlQuery &q)
{
    static cMsg    cm;
    static cMsg2   cm2;
    bool allPortEnabled = ((lv2icontsrv *)lanView::getInstance())->allPortEnabled;
//    if (cm.beginMark[0] != '!') {   // Inicializáljuk a parancs blokkot
        memset(cm.beginMark, '!', sizeof(cm.beginMark));
        memset(cm.kontakt, allPortEnabled ? (int)KENA : (int)KDISA, sizeof(cm.kontakt));
        memset(cm.light,   '3',   sizeof(cm.light));
        memset(cm.swdet,   KDISA, sizeof(cm.swdet));
        strncpy(cm.endMark,"\r\n",sizeof(cm.endMark));

        memset(cm2.beginMark, '!', sizeof(cm2.beginMark));
        i2hhhh(allPortEnabled ? 0 : 0xffff, cm2.disabled);
        i2hh(  allPortEnabled ? 0 : 0xff,   cm2.dis_sw);
        i2hh(  0,                           cm2.rev_sw);
        memset(cm2.light,   '7',   sizeof(cm2.light));
        strncpy(cm2.endMark,"\r\n",2);
//    }
    int n, dis, sr, sd;
    switch (type) {
    case IAIF1M: case IAIF1S:
        id2cc(masterId, cm.tMaster);
        id2cc(slaveId,  cm.tSlave);
        n = sizeof(cm.kontakt);
        break;
    case IAIF2:
        id2cc(masterId, cm2.tMaster);
        n = IAIF2PORTS +IAIF2SWS;   //Vannak sima iput bemenetek is
        dis = 0;        // Érzékelők, letiltva
        sr  = 0;        // input bitek reverz
        sd  = 0;        // input bitek tiltva
        break;
    default:        EXCEPTION(EPROGFAIL);
    }

    for (QMap<int, sensor>::iterator i = sensors.begin(); i != sensors.end(); ++i) {
        int j = i.key() -1; // fizikus indexből matekos
        if (0 > j || j >= n) EXCEPTION(ENOINDEX, j);
        sensor& sen = i.value();
        QString prop = sen.serv.get(_sProperties).toString();
        if (allPortEnabled) {
            if (type != IAIF2) {
                if      (prop.contains("#reversed")) cm.kontakt[j] = KREV;
                else                                 cm.kontakt[j] = KENA;
            }
            else {
                if (j >= IAIF2PORTS) {
                    int k = j - IAIF2PORTS;
                    if  (prop.contains("#reversed")) sr |= 1 << k;
                }
            }
        }
        else {
            if (type == IAIF2) {
                if (j >= IAIF2PORTS) {
                    int k = j - IAIF2PORTS;
                    if      (prop.contains("#disabled")) sr |= 1 << k;
                    else if (prop.contains("#reversed")) sd |= 1 << k;
                }
                else {
                    if  (prop.contains("#disabled")) dis |= 1 << j;
                }
            }
            else {
                if      (prop.contains("#disabled")) cm.kontakt[j] = KDISA;
                else if (prop.contains("#reversed")) cm.kontakt[j] = KREV;
                else                                 cm.kontakt[j] = KENA;
            }
        }
    }
    if (type == IAIF2) {
        i2hhhh((uint16_t)dis, cm2.disabled);
        i2hh(  (uint8_t) sd,  cm2.dis_sw);
        i2hh(  (uint8_t) sr,  cm2.rev_sw);
    }
    QSqlQuery   *pq = newQuery();
    while (!(type == IAIF2 ? qs.com_s(&cm2) : qs.com_s(&cm))) {
        hostService.setState(*pq, _sUnreachable, "No answer in setings block");
        if (hostService.getName(_sHardState) == _sUnreachable) {
            delete pq;
            return false;
        }
    }
    delete pq;
    return true;
}
*/
// -----------------------------------------------------------------------------------------


void cAttached::postInit(QSqlQuery &q, const QString &)
{
    cInspector::postInit(q);
    setting = 0;
    state   = 0;
}
