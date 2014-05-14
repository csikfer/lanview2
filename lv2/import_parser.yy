%{
#include <math.h>
#include <lanview.h>
#include "guidata.h"
#include "others.h"
#include "ping.h"
#include "import_parser.h"

class  cArpServerDefs;
static  cArpServerDefs *pArpServerDefs = NULL;
static void insertCode(const QString& __txt);
static QString  macbuff;
static QString lastLine;

static qlonglong       globalPlaceId = NULL_ID;

static int yyparse();
cError *importLastError = NULL;

int importParse()
{
    globalPlaceId = NULL_ID;
    int i = -1;
    try {
        i = yyparse();
    }
    CATCHS(importLastError);
    if (importLastError != NULL) {
        importLastError->mDataLine = importLineNo;
        importLastError->mDataName = importFileNm;
        importLastError->mDataMsg  = "lastLine : " + quotedString(lastLine) + "\n macbuff : " + quotedString(macbuff);
    }
    return i;
}

static int yyerror(QString em)
{
    EXCEPTION(EPARSE, -1, em);
    return -1;
}

static int yyerror(const char * em)
{
    return yyerror(QString(em));
}

class cArpServerDef {
public:
    enum eType { UNKNOWN, SNMP, DHCPD_LOCAL, DHCPD_SSH,  PROC_LOCAL, PROC_SSH  };
    /// A lekérdezés típusa
    enum eType      type;
    /// A szerver címe
    QHostAddress    server;
    /// Fájl paraméter
    QString         file;
    /// Community név
    QString         community;
    ///
    bool firstTime;
    cArpServerDef() : server(), file(), community() { type = UNKNOWN; firstTime = true;}
    cArpServerDef(const cArpServerDef& __o)
        : server(__o.server), file(__o.file), community(__o.community) { type = __o.type; firstTime = __o.firstTime; }
    cArpServerDef(enum eType __t, const QString& __s, const QString& __f = QString(), const QString& __c = QString())
        : server(__s), file(__f), community(__c) { type = __t; firstTime = true; }
    void updateArpTable(QSqlQuery &q);
};

class cArpServerDefs : public QVector<cArpServerDef> {
public:
    cArpServerDefs() : QVector<cArpServerDef>() { ; }
    void append(QSqlQuery &q, cArpServerDef __asd) {
        __asd.updateArpTable(q);
        *(cArpServerDefs *)this << __asd;
    }

    void updateArpTable(QSqlQuery &q) {
        iterator i, n = end();
        for (i = begin(); i != n; ++i) { i->updateArpTable(q); }
    }
};

enum eShare {
    ES_     = 0,
    ES_A,   ES_AA, ES_AB,
    ES_B,   ES_BA, ES_BB,
    ES_C,   ES_D
};

static QStringList     allNotifSwitchs;
static QSqlQuery      *piq = NULL;
void initImportParser()
{
    if (pArpServerDefs == NULL)
        pArpServerDefs = new cArpServerDefs();
    // notifswitch tömb = SET, minden on-ba, visszaolvasás listaként
    if (allNotifSwitchs.isEmpty())
        allNotifSwitchs = cUser().set(_sHostNotifSwitchs, QVariant(0xffff)).get(_sHostNotifSwitchs).toStringList();
    if (piq == NULL)
        piq = newQuery();
}

void downImportParser()
{
    pDelete(pArpServerDefs);
    pDelete(piq);
}

static inline QSqlQuery& qq() { if (piq == NULL) EXCEPTION(EPROGFAIL); return *piq; }

#define UNKNOWN_PLACE_ID 0LL

class c_yyFile {
public:
    c_yyFile(const c_yyFile& __o) : oldFileName(__o.oldFileName) {
        oldStream = __o.oldStream;
        newFile = __o.newFile;
        oldPos = __o.oldPos;
    }
    c_yyFile() : oldFileName() { oldStream = NULL; newFile = NULL; }
    static void inc(QString *__f);
    static void eoi();
    static int size() { return stack.size(); }
private:
    QTextStream*    oldStream;
    QFile *         newFile;
    QString         oldFileName;
    unsigned int    oldPos;
    static QStack<c_yyFile> stack;
};

/// A fizikai linkeket definiáló blokk adatait tartalmazó, ill. a blokkon
/// belüli definíciókat rögzítő metódusok osztálya.
/// Az objektumban épül fel a rögzítendő rekord.
class cLink {
private:
    enum eSide { LEFT = 1, RIGHT = 2};
    void side(eSide __e, QString * __n, QString *__p, int __s);
    void side(eSide __e, QString * __n, int __p, int __s);
    qlonglong& nodeId(eSide __e);
    qlonglong& portId(eSide __e);
public:
    /// Konstruktor
    /// @param __t1 A bal oldali elemen értelmezett link típusa (-1,0,1)
    /// @param __n1 Az alapértelmezett bal oldali elem neve, vagy egy üres string
    /// @param __t2 A jobb oldali elemen értelmezett link típusa (-1,0,1)
    /// @param __n2 Az alapértelmezett jobb oldali elem neve, vagy egy üres string
    cLink(int __t1, QString *__n1, int __t2, QString *__n2);
    /// A link baloldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port név. A pointert felszabadítja.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás
    void left(QString * __n, QString *__p, int __s = 0)   { side(LEFT, __n,__p, __s); }
    /// A link jobboldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port név. A pointert felszabadítja.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás
    void right(QString * __n, QString *__p, int __s = 0)  { side(RIGHT,__n,__p, __s); }
    /// A link baloldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port index, vagy NULL_ID, ha a kötelezően megadott node egyetlen portját jelöltük ki.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás)
    void left(QString * __n, int __p, int __s)  { side(LEFT, __n, __p, __s); }
    /// A link jobboldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port index, vagy NULL_ID, ha a kötelezően megadott node egyetlen portját jelöltük ki.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás)
    void right(QString * __n, int __p, int __s) { side(RIGHT, __n, __p, __s); }
    /// 'jobb' oldali link egy röptében felvett workstationra.
    /// @arg __n A munkaállomás neve
    /// @arg __mac A munkaáééomás MAC címe
    /// @arg __d node_descr értéke
    void workstation(QString * __n, cMac * __mac, QString * __d);
    void attached(QString * __n, QString * __d);
    void insert(QString * __d, QStringList * __srv);
    void insert(QString *__hn1, qlonglong __pi1, QString *__hn2, qlonglong __pi2, qlonglong __n);
    static const QString& i2LinkType(int __i);
    static const QString& chkShare(int __s);
    const QString&  phsLinkType1;
    qlonglong       nodeId1;
    qlonglong       portId1;
    const QString&  phsLinkType2;
    qlonglong       nodeId2;
    qlonglong       portId2;
    QString         share;
    bool            newNode;
};

typedef QList<QStringPair> QStringPairList;

typedef QList<qlonglong> intList;

class cTemplateMapMap : public QMap<QString, cTemplateMap> {
 public:
    /// Konstruktor
    cTemplateMapMap() : QMap<QString, cTemplateMap>() { ; }
    ///
    cTemplateMap& operator[](const QString __t) {
        if (contains(__t)) {
            return  (*(QMap<QString, cTemplateMap> *)this)[__t];
        }
        return *insert(__t, cTemplateMap(__t));
    }

    /// Egy megadott nevű template lekérése, ha nincs a konténerben, akkor beolvassa az adatbázisból.
    /// Az eredményt stringgel tér vissza
    const QString& _get(const QString& __type, const QString&  __name) {
        const QString& r = (*this)[__type].get(qq(), __name);
        return r;
    }
    /// Egy megadott nevű template lekérése, ha nincs a konténerben, akkor beolvassa az adatbázisból.
    /// Az eredményt a macro bufferbe tölti
    void get(const QString& __type, const QString&  __name) {
        insertCode(_get(__type, __name));
    }
    /// Egy adott nevű template elhelyezése a konténerbe, de az adatbázisban nem.
    void set(const QString& __type, const QString& __name, const QString& __cont) {
        (*this)[__type].set(__name, __cont);
    }
    /// Egy adott nevű template elhelyezése a konténerbe, és az adatbázisban.
    void save(const QString& __type, const QString& __name, const QString& __cont, const QString& __descr) {
        (*this)[__type].save(qq(), __name, __cont, __descr);
    }
    /// Egy adott nevű template törlése a konténerből, és az adatbázisból.
    void del(const QString& __type, const QString& __name) {
        (*this)[__type].del(qq(), __name);
    }
};


enum {
    EP_NIL = -1,
    EP_IP  = 0,
    EP_ICMP = 1,
    EP_TCP = 6,
    EP_UDP = 17
};

/* */

unsigned long yyflags = 0;

static cTemplateMapMap   templates;
static qlonglong    actVlanId = -1;
static QString      actVlanName;
static QString      actVlanNote;
static enum eSubNetType netType = NT_INVALID; // firstSubNet = ;
static cPatch *     pPatch = NULL;
static cImage *     pImage = NULL;
static cUser *      pUser = NULL;
static cGroup *     pGroup = NULL;
static cNode *      pNode  = NULL;
static cLink      * pLink = NULL;
static cService   * pService = NULL;
static cHostService*pHostService = NULL;
static cTableShape *pTableShape = NULL;
static qlonglong           alertServiceId = NULL_ID;
static QMap<QString, qlonglong>    ivars;
static QMap<QString, QString>      svars;
// QMap<QString, duble>        rvars;
static QString       sPortIx = "PI";   // Port index
static QString       sPortNm = "PN";   // Port név

QStack<c_yyFile> c_yyFile::stack;

/* ---------------------------------------------------------------------------------------------------- */

static inline qlonglong gPlace() { return globalPlaceId == NULL_ID ? UNKNOWN_PLACE_ID : globalPlaceId; }
static inline cPlace& rPlace(void)
{
    static cPlace *pPlace = NULL;
    if (pPlace == NULL) pPlace = new cPlace();
    return *pPlace;
}
#define place   rPlace()

static inline qlonglong& vint(const QString& n){
    if (!ivars.contains(n)) yyerror(QString("Integer variable %1 not found").arg(n));
    return ivars[n];
}

static inline QString& vstr(const QString& n){
    if (!svars.contains(n)) yyerror(QString("String variable %1 not found").arg(n));
    return svars[n];
}

static inline const QString& nextNetType() {
    enum eSubNetType r = netType;
    switch (r) {
    case NT_PRIMARY:    netType = NT_SECONDARY;      break;
    case NT_SECONDARY:  break;
    case NT_PRIVATE:
    case NT_PSEUDO:     netType = NT_INVALID;        break;
    default:            yyerror("Compiler error.");
    }
    return subNetType(r);
}

static inline cSnmpDevice& snmpdev() {
    if (pNode == NULL) EXCEPTION(EPROGFAIL);
    return *pNode->reconvert<cSnmpDevice>();
}

void c_yyFile::inc(QString *__f)
{
    c_yyFile    o;
    o.newFile = new QFile(*__f);
    if (!importSrcOpen(*o.newFile)) yyerror(QObject::trUtf8("include: file open error."));
    o.oldStream = importInputStream;
    importInputStream = new QTextStream(o.newFile);
    o.oldFileName = importFileNm;
    importFileNm = *__f;
    delete __f;
    o.oldPos = importLineNo;
    importLineNo = 0;
    stack.push(o);
}

void c_yyFile::eoi()
{
    c_yyFile    o = stack.pop();
    delete importInputStream;
    delete o.newFile;
    importInputStream = o.oldStream;
    importFileNm = o.oldFileName;
    importLineNo = o.oldPos;
}


/* ************************************************************************************************ */
void cArpServerDef::updateArpTable(QSqlQuery& q)
{
    DBGFN();
#ifdef MUST_SCAN
    cArpTable at;
    switch (type) {
    case UNKNOWN:       break;
    case SNMP: {
            cSnmp snmp(server.toString(), community, SNMP_VERSION_2c);
            at.getBySnmp(snmp);
            break;
        }
    case DHCPD_LOCAL:   if (firstTime) at.getByLocalDhcpdConf(file);                          break;
    case DHCPD_SSH:     if (firstTime) at.getBySshDhcpdConf(server.toString(), file);         break;
    case PROC_LOCAL:    at.getByLocalProcFile(file);                                          break;
    case PROC_SSH:      at.getBySshProcFile(server.toString(), file);                         break;
    default: EXCEPTION(EPROGFAIL);
    }
    firstTime = false;
    cArp::replaces(q, at);
    return;
/*#else
    #warning "SCAN functions is not supported."*/
#endif
    (void)q;
    EXCEPTION(ENOTSUPP);
}

/* --------------------------------------------------------------------------------------------------- */
static qlonglong newAttachedNode(const QString& __n, const QString& __d)
{
    cNode   node;
    node.asmbAttached(__n, __d, gPlace());
    node.insert(qq(), true);
    return node.ports.first()->getId();
}

static void newAttachedNodes(const QString& __np, const QString& __dp, int __from, int __to)
{
    QString name, descr;
    for (int i = __from; i <= __to; i++) {
        name  = nameAndNumber(__np, i);
        descr = nameAndNumber(__dp, i);
        newAttachedNode(name, descr);
    }
}

static qlonglong newWorkstation(const QString& __n, const cMac& __mac, const QString& __d)
{
    cNode host;
    host.asmbWorkstation(qq(), __n, __mac, __d, gPlace());
    host.insert(qq(), true);
    return host.ports.first()->getId();
}

/* --------------------------------------------------------------------------------------------------- */

cLink::cLink(int __t1, QString *__n1, int __t2, QString *__n2)
    : phsLinkType1(i2LinkType(__t1))
    , phsLinkType2(i2LinkType(__t2))
{
    nodeId1 = __n1->isEmpty() ? NULL_ID : cPatch::getNodeIdByName(qq(), *__n1);
    nodeId2 = __n2->isEmpty() ? NULL_ID : cPatch::getNodeIdByName(qq(), *__n2);
    delete __n1;
    delete __n2;
    newNode = false;
}

qlonglong& cLink::nodeId(eSide __e)
{
    if (__e == LEFT) return nodeId1;
    if (__e != RIGHT) EXCEPTION(EPROGFAIL);
    return nodeId2;
}

qlonglong& cLink::portId(eSide __e)
{
    if (__e == LEFT) return portId1;
    if (__e != RIGHT) EXCEPTION(EPROGFAIL);
    return portId2;
}

//qlonglong& cLink::portId(eSide __e);

void cLink::side(eSide __e, QString * __n, QString *__p, int __s)
{
    qlonglong   nid;    // Node id
    if (__n->size() > 0) {  // Ha megadtunk node-ot
        nid = cPatch::getNodeIdByName(qq(), *__n);
    }
    else {
        nid = nodeId(__e);
        if (nid == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
    }
    portId(__e) = cNPort::getPortIdByName(qq(), *__p, nid, false);
    if (portId(__e) == NULL_ID) yyerror(QObject::trUtf8("Invalid left port specification, #%1:%2").arg(nid). arg(*__p));
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::trUtf8("Multiple share defined"));
        share = chkShare(__s);
    }
    delete __n;
    delete __p;
    _DBGFNL() << _sSpace << portId(__e) << endl;
}

void cLink::side(eSide __e, QString * __n, int __p, int __s)
{
    qlonglong   nid;
    if (__n->size() > 0) {  // Ha megadtunk node-ot
        nid = cPatch::getNodeIdByName(qq(), *__n);
        if (__p == NULL_IX) {   // Ha megadtuk a node-t akkor lehet null, de akkor csak egy portja lehet
            cNPort ep;
            ep.set(_sNodeId, nid);   // megkeressük a node egyetlen portját
            if (ep.completion(qq()) != 1) yyerror("Insufficient or invalid data.");  // Pont egynek kell lennie
            portId(__e) = ep.getId();
        }
        else {
            portId(__e) = cNPort::getPortIdByIndex(qq(), __p, nid, false);
        }
    }
    else {
        nid = nodeId(__e);
        if (nid == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
        portId(__e) = cNPort::getPortIdByIndex(qq(), __p, nid, false);
    }
    if (portId(__e) == NULL_ID) {
        yyerror(QObject::trUtf8("Invalid port index, #%1:#%2").arg(nid).arg(__p));
    }
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::trUtf8("Multiple share defined"));
        share = chkShare(__s);;
    }
    delete __n;
    _DBGFNL() << _sSpace << portId(__e) << endl;
}

void cLink::workstation(QString * __n, cMac * __mac, QString * __d)
{
    portId2 = newWorkstation(*__n, *__mac, *__d);
    delete __n;
    delete __mac;
    delete __d;
    newNode = true;
    _DBGFNL() << _sSpace << portId2 << endl;
}

void cLink::attached(QString * __n, QString * __d)
{
    portId2 = newAttachedNode(*__n, *__d);;
    delete __n;
    delete __d;
    newNode = true;
    _DBGFNL() << _sSpace << portId2 << endl;
}

void cLink::insert(QString * __d, QStringList * __srv)
{
    cPhsLink    lnk;
    lnk[_sPortId1]      = portId1;
    lnk[_sPhsLinkType1] = phsLinkType1;
    lnk[_sPortId2]      = portId2;
    lnk[_sPhsLinkType2] = phsLinkType2;
    lnk[_sPortShared]   = share;
    lnk[_sPhsLinkNote]  = *__d;
    if (__srv != NULL) {    // Volt Alert
        cHostService    hose;
        cService        se;
        cNode           ho;
        cNPort          po;
        // Szervíz: megadták, vagy default
        if (__srv->size() > 0 && __srv->at(0).size() > 0) { if (!se.fetchByName(__srv->at(0))) yyerror("Invalis alert service name."); }
        else                                              { if (!se.fetchById(alertServiceId)) yyerror("Nothing default alert service."); }
        po.setById(qq(), portId2);   // A jobb oldali port a mienk
        ho.setById(qq(), po.getId(_sNodeId));
        // Volt megjegyzés ?
        if (__srv->size() > 1 && __srv->at(1).size() > 0) hose.setName(_sHostServiceNote, __srv->at(1));
        hose.setId(_sServiceId,  se.getId());
        hose.setId(_sNodeId,     ho.getId());
        hose.setId(_sPortId,     po.getId());
        hose[_sDelegateHostState] = true;
        QString it = se.magicParam(_sIfType);
        if (it.isEmpty()) { //??
            DWAR() << "Invalid or empty service magic param : iftype" << endl;
        }
        else if (it == _sEthernet) {     // A megadott szolgáltatás az ethernethez tartozik
            if (po.getId(_sIfTypeId) != cIfType::ifTypeId(_sEthernet)) yyerror("Invalid port type");
        }
        else if (it == _sAttach) {
            cIfType att;
            att.setByName(qq(), _sAttach);
            if (po.getId(_sIfTypeId) != cIfType::ifTypeId(_sAttach)) {
                if (!newNode) yyerror("Invalid port type");
                po.set();
                po.setName(__sAttach);
                po.setId(_sNodeId, ho.getId());
                po.setId(_sIfTypeId, cIfType::ifTypeId(_sAttach));
                po.insert(qq());
                lnk[_sPortId2] = po.getId();
                hose.setId(_sPortId,     po.getId());
            }
        }
        else {
            DWAR() << "iftype = " << it << " not supported." << endl;
        }
        lnk.insert(qq());
        hose.insert(qq());
        delete __srv;
    }
    else {
        lnk.insert(qq());
    }
    share.clear();
    portId1 = portId2 = NULL_ID;
    delete __d;
    newNode = false;
}


const QString& cLink::i2LinkType(int __i)
{
    switch (__i) {
    case  1:    return _sFront;
    case -1:    return _sBack;
    case  0:    return _sTerm;
    default:    EXCEPTION(EDATA, __i);
    }
    return _sNul;   // Inaktív kód, csak hogy nem pampogjon a fordító
}

const QString&  cLink::chkShare(int __s)
{
    static QStringList  shares;
    if (shares.size() == 0) {
        shares << _sNul;
        shares << QString("A") << QString("AA") << QString("AB");
        shares << QString("B") << QString("BA") << QString("BB");
        shares << QString("C") << QString("D");
    }
    if (__s >= shares.size() || __s < 0 ) yyerror(QObject::trUtf8("Invalid share value."));
    return shares[__s];
}

void cLink::insert(QString *__hn1, qlonglong __pi1, QString *__hn2, qlonglong __pi2, qlonglong __n)
{
    cPhsLink    lnk;
    lnk[_sPhsLinkType1] = phsLinkType1;
    lnk[_sPhsLinkType2] = phsLinkType2;
    lnk[_sPortShared]   = _sNul;
    qlonglong nid1 = __hn1->size() > 0 ? cPatch::getNodeIdByName(qq(), *__hn1) : nodeId1;
    qlonglong nid2 = __hn2->size() > 0 ? cPatch::getNodeIdByName(qq(), *__hn2) : nodeId2;
    while (__n--) {
        lnk[_sPortId1] = cNPort::getPortIdByIndex(qq(), __pi1, nid1, true);
        lnk[_sPortId2] = cNPort::getPortIdByIndex(qq(), __pi2, nid2, true);
        lnk.insert(qq());
        lnk.clear(lnk.idIndex());   // töröljük az id-t mert a köv insert-nél baj lessz.
        __pi1++;                    // Léptetjük az indexeket
        __pi2++;
    }
    delete __hn1;
    delete __hn2;
}


//-----------------------------------------------------------------------------------------------------
/// A superior_host_service_id beállítása
/// @param phs A módosítandó objektum pointere.
/// @param phn A keresett host_services rekordban hivatkozott host neve, vagy üres.
/// @param phs A keresett host_services rekordban hivatkozott szervíz típus név
/// @param ppo A keresett host_services rekordban hivatkozott port neve, vagy NULL pointer. Ha phn üres, akkor NULL.
static void setSuperiorHostService(cHostService * phs, QString * phn, QString * psn, QString *ppo = NULL)
{
    cHostService shs;
    if (!phn->isEmpty()) {
        qlonglong   nid = cNode().getIdByName(qq(), *phn);
        shs.setId(_sNodeId, nid);
        if (ppo != NULL) {
            qlonglong pid = cNPort().getPortIdByName(qq(), *ppo, nid);
            shs.setId(_sPortId, pid);
            delete ppo;
        }
    }
    shs.setId(_sServiceId, cService().getIdByName(qq(), *psn));
    delete psn;
    delete phn;
    int n = shs.completion(qq());
    if (n == 0) yyerror("horst_services record not found.");
    if (n >  1) yyerror("horst_services record ambigouos.");
    (*phs)[_sSuperiorHostServiceId] = shs.getId();
}

static QString e1 = "Redefined port name or index.";
static QString e2 = "There is insufficient data.";

/// Egy új port létrehozása (a paraméterként megadott pointereket felszabadítja)
/// @param h A node objektum, amihez hozzáadjuk a portot
/// @param ix Port index, vagy NULL_IX, ha az index NULL lessz-
/// @param pt Pointer a port típus név stringre.
/// @param pn Port nevére mutató pointer
/// @param ip Pointer egy string pár, az első elem az IP cím, vagy az "ARP" string, ha az ip címet a MAC címéből kell meghatározni.
///           A második elem az ip cím típus neve. Ha a pointer NULL, akkor nincs IP cím.
/// @param mac Ha NULL nincs mac, ha a variant egy string, akkor az a MAC cím stringgé konvertélva, vagy az "ARP" string,
///           ha variant egy int, akkor az annak a Portnak az indexe, melynek ugyanez a MAC cíne.
/// @param d Port leírás/megjegyzés szövegre mutató pointer, üres string esetén az NULL lessz.
/// @return Az új port objektum pointere
static cNPort *hostAddPort(int ix, QString *pt, QString *pn, QStringPair *ip, QVariant *mac, QString *d)
{
    if (pNode == NULL) EXCEPTION(EPROGFAIL);
    cNPort& p = pNode->asmbHostPort(qq(), ix, *pt, *pn, ip, mac, *d);
    pDelete(pt);
    pDelete(pn);
    pDelete(ip);
    pDelete(mac);
    pDelete(d);
    return &p;
}

static cNPort *portAddAddress(cNPort *_p, QStringPair *ip, QString *d)
{
    cInterface *p = _p->reconvert<cInterface>();
    p->addIpAddress(QHostAddress(ip->first), ip->second, *d);
    delete ip; delete d;
    return _p;
}

static cNPort *portAddAddress(QString *pn, QStringPair *ip, QString *d)
{
    cNPort *p = pNode->getPort(*pn);
    delete pn;
    return portAddAddress(p, ip, d);

}

static cNPort *portAddAddress(int ix, QStringPair *ip, QString *d)
{
    cNPort *p = pNode->getPort(ix);
    return portAddAddress(p, ip, d);
}

static void setSuperior(QStringPair *pshs, QStringPairList * pshl)
{
    QSqlQuery q = getQuery();
    QSqlQuery q2 = getQuery();
    cHostService hs;
    int r  = hs.fetchByNames(q, pshs->first, pshs->second, false);
    delete pshs;
    if (r != 1) yyerror("Ismeretlen, vagy nem eggyértelmű host_services rekord megadása.");
    qlonglong id = hs.getId();
    QString sn;
    foreach (QStringPair sp, *pshl) {
        if (!sp.second.isEmpty()) sn = sp.second;
        if (sn.isEmpty()) yyerror("Nincs megadva szervice név.");
        r = hs.fetchByNames(q, sp.first, sn, false);
        if (r == 0) {   // Nincs, csinálunk egyet
            cNode n;
            if (!n.fetchByName(q2, sp.first)) yyerror("A megadott node nem létezik.");
            hs.clear();
            hs.setId(_sNodeId, n.getId());
            hs.setId(_sServiceId, cService::service(q2, sn).getId());
            hs.setId(_sSuperiorHostServiceId, id);
            hs.insert(q2);
        }
        else while (1) {    // Van egy, vagy több
            hs.setId(_sSuperiorHostServiceId, id);
            hs.update(q2, false, hs.mask(_sSuperiorHostServiceId));
            if (q.next()) {
                hs.set(q);
            }
            else break;
        }
    }
    delete pshl;
}


static cTableShape * newTableShape(QString *pTbl, QString * pMod, const QString *pDescr)
{
    cTableShape *p = new cTableShape();
    p->setName(*pMod);
    p->setName(_sTableName, pTbl->isEmpty() ? *pMod : *pTbl);
    p->setName(_sTableShapeNote, *pDescr);
    delete pTbl;
    delete pMod;
    delete pDescr;
    return p;
}


static QString *pMenuApp = NULL;
static tRecordList<cMenuItem>   menuItems;

static cMenuItem& actMenuItem()
{
    if (menuItems.isEmpty()) EXCEPTION(EPROGFAIL);
    return *menuItems.last();
}

static void newMenuMenu(const QString& _n, const QString& _t)
{
    _DBGFN() << VDEB(_n) << VDEB(_t) << endl;
    if (pMenuApp == NULL) EXCEPTION(EPROGFAIL);
    cMenuItem *p = new cMenuItem;
    p->setName(_n);
    p->setName(_sAppName, *pMenuApp);
    if (!menuItems.isEmpty()) {
        p->setId(_sUpperMenuItemId, actMenuItem().getId());
    }
    if (!_t.isEmpty()) p->setName(_sMenuItemTitle, _t);
    p->setName(_sProperties, ":sub:");
    PDEB(VVERBOSE) << "Insert : " << p->allToString() << endl;
    p->insert(qq());
    menuItems << p;
}

static void delMenuItem()
{
    cMenuItem *p = menuItems.pop_back();
    delete p;
}
/// Egy menu_items rekord létrehozása
/// @param _n Rekord név
/// @param _sn paraméter
/// @param _t Megjelenített név
/// @param typ Minta string a properties mezőhöz (paraméter _n
static void newMenuItem(const QString& _n, const QString& _sn, const QString& _t, const char * typ)
{
    if (pMenuApp == NULL) EXCEPTION(EPROGFAIL);
    cMenuItem *p = new cMenuItem;
    p->setName(_n);
    p->set(_sAppName, *pMenuApp);
    p->setId(_sUpperMenuItemId, actMenuItem().getId());
    if (!_t.isEmpty()) p->setName(_sMenuItemTitle, _t);
    p->setName(_sProperties, QString(typ).arg(_sn));
    p->insert(qq());
    menuItems << p;
}

static void setToolTip(const QString& _tt)
{
    actMenuItem().setName(_sToolTip, _tt);
    actMenuItem().update(qq(), false, actMenuItem().mask(_sToolTip));
}

static void setWhatsThis(const QString& _wt)
{
    actMenuItem().setName(_sWhatsThis, _wt);
    actMenuItem().update(qq(), false, actMenuItem().mask(_sWhatsThis));
}

static void insertCode(const QString& __txt)
{
    _DBGFN() << quotedString(__txt) << " + " << quotedString(macbuff) << endl;
    macbuff = __txt + macbuff;
}


static QChar yyget()
{
    QChar c;

    if (!macbuff.size()) {
        do {
            ++importLineNo;
            lastLine = macbuff  = importInputStream->readLine();
            PDEB(INFO) << "YYLine : \"" << lastLine << "\"" << endl;
            if (macbuff.isNull()) {
                if (c_yyFile::size() > 0) {
                    c_yyFile::eoi();
                    continue;
                }
                return c; // return NULL
            }
        } while (macbuff.isEmpty());
        macbuff += QChar('\n');
    }
    if (macbuff.size()) {
        c = *(macbuff.data());      // Get first character in macbuff
        macbuff.remove(0,1);        // Delete first character in macbuff
    }
    return c;
}

static inline void yyunget(const QChar& _c) {
    macbuff.insert(0,_c);
}

static int yylex(void);
static void forLoopMac(QString *_in, QVariantList *_lst);
static void forLoop(QString *_in, QVariantList *_lst);
static void forLoop(QString *m, qlonglong fr, qlonglong to, qlonglong st);
static QStringList *listLoop(QString *m, qlonglong fr, qlonglong to, qlonglong st);
static intList *listLoop(qlonglong fr, qlonglong to, qlonglong st);


static inline QVariant *sp2vp(QString * __s)
{
    QVariant *p = new QVariant(*__s);
    delete __s;
    return p;
}

static inline QVariant *v2p(const QVariant& __v)
{
    return new QVariant(__v);
}

#define RRDSIZE 8
#define RRD(T, n) \
    static inline const T& n(T * p) { \
        static T	rrd[RRDSIZE]; \
        static int i = 0; \
        i = (i + 1) % RRDSIZE; \
        rrd[i] = *p; \
        delete p; \
        return rrd[i]; \
    }

RRD(QString, sp2s)
RRD(QVariant, vp2v)
RRD(QVariantList, vlp2vl)

static const QVariantList& slp2vl(QStringList * slp)
{
    _DBGFN() << list2string(*slp) << endl;
    static QVariantList   vl;
    vl.clear();
    foreach (QString s, *slp) vl << QVariant(s);
    delete slp;
    _DBGFNL() << list2string(vl) << endl;
    return vl;
}

static const QStringList& slp2sl(QStringList * slp)
{
    static QStringList   sl;
    sl = *slp;
    delete slp;
    return sl;
}

static inline const cMac& mp2m(cMac * mp)
{
    static cMac      m;
    m = *mp;
    delete mp;
    return m;
}


static QString * sTime(qlonglong h, qlonglong m, qlonglong s = 0)
{
    if (h < 0 || h >  24) yyerror("Invalid hour value");
    if (m < 0 || m >= 60) yyerror("Invalid min value");
    if (s < 0 || s >= 60) yyerror("Invalid sec value");
    if (h == 24 && (m > 0 || s > 0)) yyerror("Invalid time value");
    QString *p = new QString();
    *p = QString("%1:%2:%3").arg(h).arg(m).arg(s);
    return p;
}

static enum eShare s2share(QString * __ps)
{
    QString s;
    if (__ps != NULL) {
        s = *__ps;
        delete __ps;
    }
    if (s == "")   return ES_;
    if (s == "A")  return ES_A;
    if (s == "AA") return ES_AA;
    if (s == "AB") return ES_AB;
    if (s == "B")  return ES_B;
    if (s == "BA") return ES_BA;
    if (s == "BB") return ES_BB;
    if (s == "C")  return ES_C;
    if (s == "D")  return ES_D;
    yyerror(QString(QObject::trUtf8("Helytelen megosztás típus : %1")).arg(s));
    return ES_;     // Hogy ne pofázzon a fordító
}

static void portShare(intList *pil, QStringList *psl)
{
    intList     il = *pil;
    QStringList sl = *psl;
    delete pil;
    delete psl;
    int siz = il.size();
    QString e = QObject::trUtf8("Helytelen megosztás megadás.");
    if (siz != sl.size())  yyerror(e);
    switch (siz) {
    case 2:
        if      (sl[0] == "A" && sl[1] == "B") pPatch->setShare((int)il[0], NULL_IX, (int)il[1]);
        else if (sl[0] == "B" && sl[1] == "A") pPatch->setShare((int)il[1], NULL_IX, (int)il[0]);
        else yyerror(e);
        break;
    case 3:
        if      (sl[0] == "A"  && sl[1] == "BA" && sl[2] == "BB") pPatch->setShare((int)il[0], NULL_IX, (int)il[1], (int)il[2]);
        else if (sl[0] == "AA" && sl[1] == "AB" && sl[2] == "B" ) pPatch->setShare((int)il[0], (int)il[1], (int)il[2]);
        else    yyerror(e);
        break;
    case 4:
        if      (sl[0] == "AA" && sl[1] == "AB" && sl[2] == "BA" && sl[3] == "BB") pPatch->setShare((int)il[0], (int)il[1], (int)il[2], (int)il[3]);
        else if (sl[0] == "AA" && sl[1] == "C"  && sl[2] == "D"  && sl[3] == "BB") pPatch->setShare((int)il[0], (int)il[1], (int)il[2], (int)il[3], true);
        else    yyerror(e);
        break;
    default:
        yyerror(e);
    }
}


static QString *toddef(QString * name, QString *  day, QString * fr, QString *  to, QString *descr)
{
    cTpow t;
    t.setName(*name);
    t[_sTpowNote] = *descr;
    t[_sDow]       = *day;
    t[_sBeginTime] = *fr;
    t[_sEndTime]   = *to;
    t.insert(qq());
    delete day; delete fr; delete to; delete descr;
    return name;
}

static void setLastPort(QString * np, qlonglong ix) {
    if (np != NULL && np->size() > 0) svars[sPortNm] = *np;
    else                              svars.remove(sPortNm);
    if (ix != NULL_IX)                ivars[sPortIx] = ix;
    else                              ivars.remove(sPortIx);
}

static void setLastPort(const QString& n, qlonglong ix) {
    if (n.size() > 0)   svars[sPortNm] = n;
    else                svars.remove(sPortNm);
    if (ix != NULL_IX)  ivars[sPortIx] = ix;
    else                ivars.remove(sPortIx);
}

static void setLastPort(cNPort *p)
{
    qlonglong ix = p->getId(_sPortIndex);
    if (ix == NULL_ID) ix = NULL_IX;
    setLastPort(p->getName(_sPortName), ix);
}

void newNode(QStringList * t, QString *name, QString *d)
{
    _DBGFN() << "@(" << *name << _sComma << *d << ")" << endl;
    pNode = new cNode();
    pNode->asmbNode(qq(), *name, NULL, NULL, NULL, *d, gPlace());
    pNode->set(_sNodeType, *t);
    pDelete(t);
    pDelete(name); pDelete(d);
}

/// Egy új host vagy snmp eszköz létrehozása (a paraméterként megadott pointereket felszabadítja)
/// @param name Az eszköz nevére mutató pointer, vagy a "LOOKUP" stringre mutató pointer
/// @param ip Pointer egy string pár, az első elem az IP cím, vagy az ARP ill. LOOKUP string, ha az ip címet a MAC címből ill.
///           a névből kell meghatározni. A második elem az ip cím típus neve.
/// @param mac Vagy a MAC stringgé konvertálva, vagy az ARP string, ha az IP címből kell meghatározni, vagy NULL, ha nincs MAC
/// @param d Port secriptorra/megjegyzés  mutató pointer, üres string esetln az NULL lessz.
/// @return Az új objektum pointere
static void newHost(QStringList * t, QString *name, QStringPair *ip, QString *mac, QString *d)
{
    if (t->contains(_sSnmp, Qt::CaseInsensitive)) pNode = new cSnmpDevice();
    else                                          pNode = new cNode();
    pNode->asmbNode(qq(), *name, NULL, ip, mac, *d, gPlace());
    pNode->set(_sNodeType, *t);
    pDelete(t);
    pDelete(name); pDelete(ip); pDelete(mac); pDelete(d);
    setLastPort(pNode->ports.first());
}

#define NEWOBJ(p, t) \
    if (p != NULL) yyerror(_STR(p) " is not null"); \
    p = new t;
//  if (p->stat == ES_DEFECTIVE) yyerror("Nincs elegendő adat, vagy kétértelműség.")

#define INSERTANDDEL(p) \
    if (p == NULL) yyerror(_STR(p) " is null"); \
    p->insert(qq()); \
    delete p; p = NULL; \
    setLastPort(NULL, NULL_IX)  // Törli

#define DELOBJ(p) \
    if (p == NULL) yyerror(_STR(p) " is null"); \
    delete p; p = NULL

#define INSREC(T, dn, n, d) \
    T  o; o.setName(*n)[dn] = *d; \
    o.insert(qq()); delete n; delete d;

%}

%union {
    void *          u;
    qlonglong       i;
    intList *      il;
    QList<qlonglong> *idl;
    bool            b;
    QString *       s;
    QStringList *  sl;
    cMac *        mac;
    double          r;
    QHostAddress * ip;
    netAddress *    n;
    QVariant *      v;
    QVariantList * vl;
    QPointF *     pnt;
    tPolygonF *   pol;
    QStringPair *  ss;
    QStringPairList *ssl;
    cSnmpDevice *  sh;
}

%token      MACRO_T FOR_T DO_T TO_T SET_T CLEAR_T BEGIN_T END_T ROLLBACK_T
%token      VLAN_T SUBNET_T PORTS_T PORT_T NAME_T SHARED_T SENSORS_T
%token      PLACE_T PATCH_T HUB_T SWITCH_T NODE_T HOST_T ADDRESS_T
%token      PARENT_T IMAGE_T FRAME_T TEL_T NOTE_T MESSAGE_T ALARM_T
%token      PARAM_T TEMPLATE_T COPY_T FROM_T NULL_T VIRTUAL_T
%token      INCLUDE_T PSEUDO_T OFFS_T IFTYPE_T WRITE_T RE_T
%token      ADD_T READ_T UPDATE_T ARPS_T ARP_T SERVER_T FILE_T BY_T
%token      SNMP_T SSH_T COMMUNITY_T DHCPD_T LOCAL_T PROC_T CONFIG_T
%token      ATTACHED_T LOOKUP_T WORKSTATION_T LINKS_T BACK_T FRONT_T
%token      TCP_T UDP_T ICMP_T IP_T NIL_T COMMAND_T SERVICE_T PRIME_T
%token      MAX_T CHECK_T ATTEMPTS_T NORMAL_T INTERVAL_T RETRY_T 
%token      FLAPPING_T CHANGE_T TRUE_T FALSE_T ON_T OFF_T YES_T NO_T
%token      DELEGATE_T STATE_T SUPERIOR_T TIME_T PERIODS_T LINE_T GROUP_T
%token      USER_T DAY_T OF_T PERIOD_T PROTOCOL_T ALERT_T INTEGER_T FLOAT_T
%token      DELETE_T ONLY_T PATTERN_T STRING_T SAVE_T TYPE_T STEP_T
%token      MASK_T LIST_T VLANS_T ID_T DYNAMIC_T FIXIP_T PRIVATE_T PING_T
%token      NOTIF_T ALL_T RIGHTS_T REMOVE_T SUB_T PROPERTIES_T MAC_T EXTERNAL_T
%token      LINK_T LLDP_T SCAN_T TABLE_T FIELD_T SHAPE_T TITLE_T REFINE_T
%token      LEFT_T DEFAULTS_T ENUM_T RIGHT_T VIEW_T INSERT_T EDIT_T
%token      INHERIT_T NAMES_T HIDE_T VALUE_T DEFAULT_T FILTER_T FILTERS_T
%token      ORD_T SEQUENCE_T MENU_T GUI_T OWN_T TOOL_T TIP_T WHATS_T THIS_T
%token      EXEC_T TAG_T ANY_T BOOLEAN_T CHAR_T IPADDRESS_T REAL_T URL_T

%token <i>  INTEGER_V
%token <r>  FLOAT_V
%token <s>  STRING_V NAME_V
%token <mac> MAC_V 
%token <ip> IPV4_V IPV6_V
%type  <i>  int int_ iexpr lnktype shar ipprotp ipprot offs ix_z vlan_t set_t
%type  <i>  vlan_id place_id iptype pix pix_z pix_ iptype_a step timep image_id tmod int0
%type  <i>  ptypen
%type  <il> list_i pixs // ints
%type  <b>  bool bool_on pattern
%type  <r>  /* real */ num fexpr
%type  <s>  str str_ str_z name_q time tod _toddef sexpr pnm mac_q ha nsw ips
%type  <sl> strs strs_z alert list_m nsws nsws_ node_h host_h
%type  <v>  value mac_qq
%type  <vl> vals
%type  <n>  subnet
%type  <ip> ip
%type  <pnt> point
%type  <pol> frame points
%type  <idl> vlan_ids
%type  <ss>  ip_qq ip_q ip_a hs
%type  <ssl> hss
%type  <mac> mac
%type  <sh>  snmph

%left  '^'
%left  '+' '-' '|'
%left  '*' '/' '&'
%left  NEG

%%

conf    :   commands            { ; }
        ;
commands:   command
        |   command commands
        ;
command : macro
        | trans
        | user
        | timeper
        | vlan
        | image
        | place
        | nodes
        | ptype
        | arp
        | link
        | srv
        | delete
        | eqs
        | scan
        | gui
        ;
macro   : INCLUDE_T str ';'                                 { c_yyFile::inc($2); }
        | MACRO_T            NAME_V str ';'               { templates.set (_sMacros,     sp2s($2), sp2s($3));     }
        | MACRO_T            NAME_V str SAVE_T str_z ';'  { templates.save(_sMacros,     sp2s($2), sp2s($3), sp2s($5)); }
        | TEMPLATE_T PATCH_T NAME_V str ';'               { templates.set (_sPatchs,     sp2s($3), sp2s($4));     }
        | TEMPLATE_T PATCH_T NAME_V str SAVE_T str_z ';'  { templates.save(_sPatchs,     sp2s($3), sp2s($4), sp2s($6)); }
        | TEMPLATE_T NODE_T  NAME_V str ';'               { templates.set (_sNodes,      sp2s($3), sp2s($4));     }
        | TEMPLATE_T NODE_T  NAME_V str SAVE_T str_z ';'  { templates.save(_sNodes,      sp2s($3), sp2s($4), sp2s($6)); }
        | for_m
        ;
for_m   : FOR_T vals  DO_T MACRO_T NAME_V ';'               { forLoopMac($5, $2); }
        | FOR_T vals  DO_T str ';'                          { forLoop($4, $2); }
        | FOR_T iexpr TO_T iexpr step MASK_T str ';'        { forLoop($7, $2, $4, $5); }
        ;
step    :                                                   { $$ = 1; }
        | STEP_T iexpr                                      { $$ = $2; }
        ;
list_m  : LIST_T iexpr TO_T iexpr MASK_T str                { $$ = listLoop($6, $2, $4, 1);  }
        | LIST_T iexpr TO_T iexpr STEP_T iexpr MASK_T str   { $$ = listLoop($8, $2, $4, $6); }
        ;
trans   : BEGIN_T ';'   { if (!qq().exec("BEGIN TRANSACTION"))    yyerror("Error BEGIN sql command"); PDEB(INFO) << "BEGIN TRANSACTION" << endl; }
        | END_T ';'     { if (!qq().exec("END TRANSACTION"))      yyerror("Error END sql command");   PDEB(INFO) << "END TRANSACTION" << endl; }
        | ROLLBACK_T ';'{ if (!qq().exec("ROLLBACK TRANSACTION")) yyerror("Error END sql command");   PDEB(INFO) << "ROLLBACK TRANSACTION" << endl; }
        ;
eqs     : '#' NAME_V '=' iexpr ';'  { ivars[*$2] = $4; delete $2; }
        | '&' NAME_V '=' sexpr ';'  { svars[*$2] = *$4; delete $2; delete $4; }
        ;
str_    : STRING_V                  { $$ = $1; }
        | NAME_V                    { $$ = $1; }
        | NULL_T                    { $$ = new QString(); }
        | STRING_T '(' iexpr ')'    { $$ = new QString(QString::number($3)); }
        | MASK_T  '(' sexpr ',' iexpr ')' { $$ = new QString(nameAndNumber(sp2s($3), (int)$5)); }
        | MACRO_T '(' sexpr ')'     { $$ = new QString(templates._get(_sMacros, sp2s($3))); }
        | TEMPLATE_T PATCH_T '(' sexpr ')'    { $$ = new QString(templates._get(_sPatchs,      sp2s($4))); }
        | TEMPLATE_T NODE_T  '(' sexpr ')'    { $$ = new QString(templates._get(_sNodes,       sp2s($4))); }
        ;
str     : str_                      { $$ = $1; }
        | '&' '[' sexpr ']'         { $$ = $3; }
        | '&' NAME_V                { *($$ = $2) = vstr(*$2); }
        ;
sexpr   : str_                      { $$ = $1; }
        | '(' sexpr ')'             { $$ = $2; }
        | sexpr '+' sexpr           { *$$ += *$3; delete $3; }
        | iexpr '*' sexpr           { *$$ = $3->repeated($1); }
        ;
str_z   : str                       { $$ = $1; }
        |                           { $$ = new QString(); }
        ;
strs    : str                       { $$ = new QStringList(*$1); delete $1; }
        | list_m                    { $$ = $1; }
        | strs ',' str              { $$ = $1;   *$$ << *$3;     delete $3; }
        | strs ',' list_m           { $$ = $1;   *$$ << *$3;     delete $3; }
        ;
strs_z  : str_z                     { $$ = new QStringList(*$1); delete $1; }
        | strs_z ',' str_z          { $$ = $1;   *$$ << *$3;     delete $3; }
        ;

int_    : INTEGER_V                     { $$ = $1; }
        | ID_T NODE_T '(' str ')'       { $$ = cPatch().getIdByName(qq(), sp2s($4)); }
        | ID_T PLACE_T '(' place_id ')' { $$ = $4;  }
        | ID_T TIME_T PERIOD_T '(' str ')'   { $$ = cTimePeriod().getIdByName(qq(), sp2s($5)); }
        | ID_T SUBNET_T '(' str ')'     { $$ = cSubNet().getIdByName(qq(), sp2s($4)); }
        | ID_T IFTYPE_T '(' str ')'     { $$ = cIfType().getIdByName(qq(), sp2s($4)); }
        | ID_T VLAN_T '(' vlan_id ')'   { $$ =  $4; }
        | '#' NAME_V                    { $$ = vint(*$2); delete $2; }
        | '#' '+' NAME_V                { $$ = (vint(*$3) += 1); delete $3; }
        ;
int     : int_                      { $$ = $1; }
        | '-' INTEGER_V             { $$ = -$2; }
        | '#' '[' iexpr ']'         { $$ = $3; }
        ;
vlan_id : str                       { $$ = cVLan().getIdByName(qq(), *$1); delete $1; }
        | int                       { $$ = $1; }
        ;
place_id: str                       { $$ = cPlace().getIdByName(qq(), *$1); delete $1; }
        ;
iexpr   : int_                       { $$ = $1; }
        | '-' iexpr  %prec NEG      { $$ = -$2; }
        | iexpr '+' iexpr           { $$ = $1 + $3; }
        | iexpr '-' iexpr           { $$ = $1 - $3; }
        | iexpr '|' iexpr           { $$ = $1 | $3; }
        | iexpr '*' iexpr           { $$ = $1 * $3; }
        | iexpr '/' iexpr           { $$ = $1 / $3; }
        | iexpr '&' iexpr           { $$ = $1 & $3; }
        | '(' iexpr ')'             { $$ = $2; }
        | INTEGER_T '(' fexpr ')'   { $$ = $3; }
        ;
ix_z    :                           { $$ = NULL_IX; }
        | int                       { $$ = $1; }
        ;
/*ints    : int                       { $$ = new intList; *$$ << $1; }
        | list_i                    { $$ = $1; }
        | ints ',' int              { *$$ << $3; }
        | ints ',' list_i           { *$$ << *$3; delete $3; }
        ;*/
list_i  : LIST_T iexpr TO_T iexpr                { $$ = listLoop($2, $4, 1);  }
        | LIST_T iexpr TO_T iexpr STEP_T iexpr   { $$ = listLoop($2, $4, $6); }
        ;
/*real    : FLOAT_V                   { $$ = $1; }
        | '[' fexpr ']'             { $$ = $2; }
        ;*/
fexpr   : FLOAT_V                   { $$ = $1; }
        | '-' fexpr  %prec NEG      { $$ = -$2; }
        | fexpr '+' fexpr           { $$ = $1 + $3; }
        | fexpr '-' fexpr           { $$ = $1 - $3; }
        | fexpr '*' fexpr           { $$ = $1 * $3; }
        | fexpr '/' fexpr           { $$ = $1 / $3; }
        | iexpr '^' iexpr           { $$ = pow((double)$1, (double)$3); }
        | '(' fexpr ')'             { $$ = $2; }
        | FLOAT_T '(' iexpr ')'     { $$ = $3; }
        ;
value   : iexpr                     { $$ = new QVariant($1); }
        | fexpr                     { $$ = new QVariant($1); }
        | str                       { $$ = sp2vp($1);}
        | ip                        { $$ = new QVariant(); $$->setValue(*$1); delete $1; }
        | bool                      { $$ = new QVariant($1); }
        | mac                       { $$ = new QVariant(); $$->setValue(*$1); delete $1; }
        | subnet                    { $$ = new QVariant(); $$->setValue(*$1); delete $1; }
        | frame                     { $$ = new QVariant(); $$->setValue(*$1); delete $1; }
        | point                     { $$ = new QVariant(); $$->setValue(*$1); delete $1; }
        ;
vals    : value                     { $$ = new QVariantList(); *$$ << *$1; delete $1; }
        | list_m                    { $$ = new QVariantList(slp2vl($1)); }
        | list_i                    { $$ = new QVariantList(); foreach (qlonglong i, *$1) *$$ << QVariant(i); delete $1; }
        | vals ',' value            { $$ = $1;                 *$$ << *$3; delete $3; }
        | vals ',' list_m           { $$ = $1;                 *$$ << slp2vl($3); }
        | vals ',' list_i           { $$ = $1; foreach (qlonglong i, *$3) *$$ << QVariant(i); delete $3; }
        ;
num     : iexpr                     { $$ = $1; }
        | fexpr                     { $$ = $1; }
        ;
bool    : ON_T      { $$ = true; }  | YES_T     { $$ = true; }  | TRUE_T    { $$ = true; }
        | OFF_T     { $$ = false; } | NO_T      { $$ = false; } | FALSE_T   { $$ = false; }
        ;
bool_on :                           { $$ = true; }
        | bool                      { $$ = $1; }
        ;
user    : USER_T str str_z          { NEWOBJ(pUser, cUser()); pUser->setName(*$2).setName(_sUserNote, *$3); delete $2; delete $3; }
            user_e                  { INSERTANDDEL(pUser); }
        | USER_T GROUP_T str str_z  { NEWOBJ(pGroup, cGroup()); pGroup->setName(*$3).setName(_sGroupNote, *$4); delete $3; delete $4; }
            ugrp_e                  { INSERTANDDEL(pGroup); }
        | USER_T GROUP_T str ADD_T str ';'      { cGroupUser gu(qq(), *$3, *$5); if (!gu.test(qq())) gu.insert(qq()); delete $3; delete $5; }
        | USER_T GROUP_T str REMOVE_T str ';'   { cGroupUser gu(qq(), *$3, *$5); if (gu.test(qq())) gu.remove(qq()); delete $3; delete $5; }
        ;
user_e  : ';'
        | '{' user_ps '}'
        ;
user_ps :
        | user_ps user_p
        ;
user_p  : HOST_T NOTIF_T PERIOD_T timep ';'     { pUser->setId(_sHostNotifPeriod, $4); }
        | SERVICE_T NOTIF_T PERIOD_T timep ';'  { pUser->setId(_sServNotifPeriod, $4); }
        | HOST_T NOTIF_T SWITCH_T nsws ';'      { pUser->set(_sHostNotifSwitchs, QVariant(*$4)); delete $4; }
        | SERVICE_T NOTIF_T SWITCH_T nsws ';'   { pUser->set(_sServNotifSwitchs, QVariant(*$4)); delete $4; }
        | HOST_T NOTIF_T COMMAND_T str ';'      { pUser->setName(_sHostNotifCmd, *$4); delete $4; }
        | SERVICE_T NOTIF_T COMMAND_T str ';'   { pUser->setName(_sServNotifCmd, *$4); delete $4; }
        | TEL_T strs ';'                        { pUser->set(_sTel, QVariant(*$2)); delete $2; }
        | ADDRESS_T strs ';'                    { pUser->set(_sAddresses, QVariant(*$2)); delete $2; }
        | PLACE_T place_id ';'                  { pUser->setId(_sPlaceId, $2); }
        ;
timep   : str                                   { $$ = cTimePeriod().fetchByName(*$1); delete $1; }
        ;
nsws    : ALL_T                                 { $$ = new QStringList(allNotifSwitchs); }
        |                                       { $$ = new QStringList(); }
        | nsws_                                 { $$ = $1; }
        ;
nsws_   : nsw                                   { $$ = new QStringList(*$1); delete $1; }
        | nsws_ ',' nsw                         { *($$ = $1) << *$3; delete $3; }
        ; 
nsw     : str                                   { $$ = $1; if (!allNotifSwitchs.contains(*$1)) yyerror("Ivalis notif swich value."); delete $1; }
        ;
ugrp_e  : ';'
        | '{' ugrp_ps '}'
        ;
ugrp_ps :
        | ugrp_ps ugrp_p
        ;
ugrp_p  : RIGHTS_T str ';'                      { pGroup->setName(_sGroupRights, *$2); delete $2; }
        | PLACE_T place_id ';'                  { pGroup->setId(_sPlaceId, $2); }
        ; 
timeper : TIME_T PERIOD_T str str_z ';'         { INSREC(cTimePeriod, _sTimePeriodNote, $3, $4); }
        | tod ADD_T TIME_T PERIOD_T str ';'     { tTimePeriodTpow(qq(), *$5, *$1).insert(qq()); delete $1; delete $5; }
        | toddef
        ;
tod     : _toddef                   { $$ = $1; }
        | TIME_T OF_T DAY_T str     { $$ = $4; }
        ;
time    : INTEGER_V ':' INTEGER_V               { $$ = sTime($1, $3); }
        | INTEGER_V ':' INTEGER_V ':' INTEGER_V { $$ = sTime($1, $3, $5); }
        ;
_toddef : TIME_T OF_T DAY_T str str FROM_T time TO_T time str_z	{ $$ = toddef($4, $5, $7, $9, $10); }
        ;
toddef  : _toddef ';'               { delete $1; }
        ;
        /*
ptype   : PARAM_T TYPE_T str str ptypen str_z ';'{ cParamType::insertNew(*$3, *$4, $5, *$6); delete $3; delete $4; delete $6; }
        ;*/
ptypen  : ANY_T                     { $$ = PT_ANY; }
        | BOOLEAN_T                 { $$ = PT_BOOLEAN; }
        | INTEGER_T                 { $$ = PT_INTEGER; }
        | REAL_T                    { $$ = PT_REAL; }
        | CHAR_T                    { $$ = PT_CHAR; }
        | STRING_T                  { $$ = PT_STRING; }
        | INTERVAL_T                { $$ = PT_INTERVAL; }
        | IPADDRESS_T               { $$ = PT_IPADDRESS; }
        | URL_T                     { $$ = PT_URL; }
        ;
syspar  : SYS_T PARAM_T str str '=' value   { setSysParam
        ;
vlan    : VLAN_T int str str_z      {
                                        actVlanId = cVLan::insertNew($2, actVlanName = *$3, actVlanNote = *$4, true);
                                        delete $3; delete $4;
                                        netType = NT_PRIMARY;
                                    }
            '{' subnets '}'         { actVlanId = -1; }
        | PRIVATE_T SUBNET_T        { actVlanId = -1; netType = NT_PRIVATE; }  subnet { netType = NT_INVALID; }
        | PSEUDO_T  SUBNET_T        { actVlanId = -1; netType = NT_PSEUDO;  }  subnet { netType = NT_INVALID; }
        ;
subnets : subnet                    { actVlanName.clear(); actVlanNote.clear(); }
        | subnets subnet
        ;
subnet  : ip '/' INTEGER_V ';'
                {
                    if (netType != NT_PRIMARY) yyerror("no name");
                    cSubNet::insertNew(actVlanName, actVlanNote, netAddress(*$1, (int)$3), actVlanId, _sPrimary);
                    delete $1;
                }
        | ip '/' IPV4_V ';'
                {
                    if (netType != NT_PRIMARY) yyerror("no name"); if ($1->protocol() != QAbstractSocket::IPv4Protocol) yyerror("Invalid net address");
                    cSubNet::insertNew(actVlanName, actVlanNote, netAddress(*$1, toIPv4Mask(*$3)), actVlanId, _sPrimary);
                    delete $1; delete $3;
                }
        | ip '/' int str str_z ';'
                {
                    cSubNet::insertNew(*$4, *$5, netAddress(*$1, (int)$3), actVlanId, nextNetType());
                    delete $1;
                }
        | ip '/' IPV4_V str str_z ';'
                {
                    if ($1->protocol() != QAbstractSocket::IPv4Protocol) yyerror("Invalid net address");
                    cSubNet::insertNew(*$4, *$5, netAddress(*$1, toIPv4Mask(*$3)), actVlanId, nextNetType());
                    delete $1; delete $3;
                }
        ;
ip      : IPV4_V                    { $$ = $1; PDEB(VVERBOSE) << "ip(IPV4):" << $$->toString() << endl; }
        | IPV6_V                    { $$ = $1; PDEB(VVERBOSE) << "ip(IPV6):" << $$->toString() << endl; }
        | IP_T '(' sexpr ')'        { $$ = new QHostAddress(); if (!$$->setAddress(*$3)) yyerror("Invalid address"); }
        ;
ips     : ip                        { $$ = new QString($1->toString()); delete $1; }
        ;
image   : IMAGE_T str str_z         { NEWOBJ(pImage, cImage()); pImage->setName(*$2).setName(_sImageNote, *$3); delete $2; delete $3; }
            image_e                 { INSERTANDDEL(pImage); }
        ;
image_e : '{' image_ps '}'
        ;
image_ps: image_p
        | image_ps image_p
        ;
image_p : TYPE_T str ';'            { pImage->setName(_sImageType, *$2); delete $2; }
        | SUB_T TYPE_T str ';'      { pImage->setName(_sImageSubType, *$3); delete $3; }
        | IMAGE_T FILE_T str ';'    { pImage->load(*$3); delete $3; }
        ;
image_id: str                       { $$ = cImage().getIdByName(sp2s($1)); }
        ;
place   : PLACE_T str str_z         { place.clear().setName(*$2).set(_sPlaceNote, *$3); delete $2; delete $3; }
          place_e                   { place.insert(qq()); }
        | SET_T PLACE_T place_id ';'{ globalPlaceId = $3; }
        | CLEAR_T PLACE_T ';'       { globalPlaceId = NULL_ID; }
        | PLACE_T GROUP_T str str_z ';' { cPlaceGroup::insertNew(sp2s($3), sp2s($4)); }
        | PLACE_T GROUP_T str ADD_T str ';'     { cGroupPlace gp(qq(), *$3, *$5); if (!gp.test(qq())) gp.insert(qq()); delete $3; delete $5; }
        | PLACE_T GROUP_T str REMOVE_T str ';'  { cGroupPlace gp(qq(), *$3, *$5); if (gp.test(qq())) gp.remove(qq()); delete $3; delete $5; }
        ;
place_e : ';'
        | '{' plac_ps '}'
        ;
plac_ps : place_p
        | plac_ps place_p
        ;
place_p : NOTE_T str ';'           { place.set(_sPlaceNote, *$2);     delete $2; }
        | PARENT_T place_id ';'     { place.set(_sParentId, $2); }
        | IMAGE_T image_id ';'      { place.set(_sImageId,  $2); }
        | FRAME_T frame ';'         { place.set(_sFrame, QVariant::fromValue(*$2)); delete $2; }
        | TEL_T strs ';'            { place.set(_sTel, *$2);            delete $2; }
        | ALARM_T MESSAGE_T str ';' { place.set(_sPlaceAlarmMsg, *$3);  delete $3; }
        ;
frame   : '{' points  '}'           { $$ = $2; }
        ;
points  : point                     { $$ = new tPolygonF(); *$$ << *$1;  delete $1; }
        | points ',' point          { $$ = $1;              *$$ << *$3;  delete $3; }
        ;
point   : '[' num ',' num ']'       { $$ = new QPointF($2, $4); }
        ;
nodes   : patch
        | node
        ;
patch   : patch_h { pPatch->setId(_sPlaceId, gPlace()); } patch_e { INSERTANDDEL(pPatch); }
        ;
patch_h : PATCH_T str str_z                     { NEWOBJ(pPatch, cPatch(*$2, *$3)); delete $2; delete $3; }
        | PATCH_T str str_z COPY_T FROM_T str   { NEWOBJ(pPatch, cPatch(*$2, *$3)); templates.get(_sPatchs, sp2s($6)); delete $2; delete $3; }
                patch_ps
        ;
patch_e : '{' patch_ps '}'
        | ';'
        ;
patch_ps: patch_p patch_ps
        |
        ;
patch_p : NOTE_T str ';'                        { pPatch->setName(_sNodeNote, sp2s($2)); }
        | PLACE_T place_id ';'                  { pPatch->set(_sPlaceId, $2); }
        | ADD_T PORT_T int str str_z ';'        { setLastPort(pPatch->addPort(sp2s($4), sp2s($5), $3)); }
        | ADD_T PORTS_T offs FROM_T int TO_T int offs str ';'
                                                { setLastPort(pPatch->addPorts(sp2s($9), $8, $5, $7, $3)); }
        | PORT_T pix NOTE_T strs ';'            { setLastPort(pPatch->portSet($2, _sPortNote, slp2vl($4))); }
        | PORT_T pnm NOTE_T str ';'             { setLastPort(pPatch->portSet(sp2s($2), _sPortNote, sp2s($4))); }
        | PORT_T pix TAG_T strs ';'             { setLastPort(pPatch->portSet($2, _sPortTag, slp2vl($4))); }
        | PORT_T pnm TAG_T str ';'              { setLastPort(pPatch->portSet(sp2s($2), _sPortTag, sp2s($4))); }
        | PORT_T pix SET_T str '=' vals ';'     { setLastPort(pPatch->portSet($2, sp2s($4), vlp2vl($6))); }
        | PORT_T str SET_T str '=' value ';'    { setLastPort(pPatch->portSet(sp2s($2), sp2s($4), vp2v($6))); }
        | PORTS_T pixs SHARED_T strs ';'        { int _pix = $2->last();
                                                  setLastPort(pPatch->ports.get(_sPortIndex, QVariant(_pix))->getName(_sPortName), _pix);
                                                  portShare($2, $4);
                                                }
        | for_m
        | eqs
        ;
offs    : OFFS_T int                    { $$ = $2; }
        |                               { $$ = 0; }
        ;
pix     : INTEGER_V                     { $$ = $1; }
        | '#' NAME_V                    { $$ = vint(*$2); delete $2; }
        | '#' '+' NAME_V                { $$ = (vint(*$3) += 1); delete $3; }
        | '#' '[' iexpr ']'             { $$ = $3; }
        | '#' '@'                       { $$ = vint(sPortIx); }
        | '#' '+' '@'                   { $$ = (vint(sPortIx) += 1); }
        ;
pix_z   : INTEGER_V                     { $$ = $1; }
        | '#' NAME_V                    { $$ = vint(*$2); delete $2; }
        | '#' '+' NAME_V                { $$ = (vint(*$3) += 1); delete $3; }
        | '#' '[' iexpr ']'             { $$ = $3; }
        | '#' '@'                       { $$ = vint(sPortIx); }
        | '#' '+' '@'                   { $$ = (vint(sPortIx) += 1); }
        |                               { $$ = NULL_IX; }
        ;
pnm     : str                                   { $$ = $1; }
        | '&' '@'                               { $$ = new QString(vstr(sPortNm)); }
        ;
pixs    : pix_                                  { $$ = new intList; *$$ << $1; }
        | pixs ',' pix_                         { *$$ << $3; }
        ;
pix_    : int                                   { $$ = vint(sPortIx) = $1; }
        | '#' '+' '@'                           { $$ = (vint(sPortIx) += 1); }
        | str                                   { $$ = vint(sPortIx) = pPatch->ports.get(_sPortName, *$1)->getId(_sPortIndex); delete $1; }
        ;
        node_h  : NODE_T                        { *($$ = new QStringList) << _sNode; }
        | NODE_T SWITCH_T                       { *($$ = new QStringList) << _sNode << _sSwitch; }
        | NODE_T VIRTUAL_T                      { *($$ = new QStringList) << _sNode << _sVirtual; }
        | NODE_T VIRTUAL_T SWITCH_T             { *($$ = new QStringList) << _sNode << _sVirtual << _sSwitch; }
        | HUB_T                                 { *($$ = new QStringList) << _sHub; }
        | HUB_T VIRTUAL_T                       { *($$ = new QStringList) << _sHub << _sVirtual; }
        ;
host_h  : HOST_T                                { *($$ = new QStringList) << _sHost; }
        | HOST_T VIRTUAL_T                      { *($$ = new QStringList) << _sHost <<_sVirtual; }
        | HOST_T SWITCH_T                       { *($$ = new QStringList) << _sHost << _sSwitch; }
        | HOST_T VIRTUAL_T SWITCH_T             { *($$ = new QStringList) << _sHost << _sVirtual << _sSwitch; }
        | HOST_T SNMP_T                         { *($$ = new QStringList) << _sHost << _sSnmp; }
        | HOST_T VIRTUAL_T SNMP_T               { *($$ = new QStringList) << _sHost << _sVirtual << _sSnmp; }
        | HOST_T SNMP_T SWITCH_T                { *($$ = new QStringList) << _sHost << _sSnmp << _sSwitch; }
        | HOST_T VIRTUAL_T SNMP_T SWITCH_T      { *($$ = new QStringList) << _sHost << _sVirtual << _sSnmp << _sSwitch; }
        ;
node    : node_h str str_z                      { newNode($1, $2, $3); }
                node_cf node_e                  { INSERTANDDEL(pNode); }
        | ATTACHED_T str str_z ';'              { newAttachedNode(*$2, *$3); delete $2; delete $3; }
        | ATTACHED_T str str_z FROM_T int TO_T int ';'
                                                { newAttachedNodes(*$2, *$3, $5, $7); delete $2; delete $3; }
        | host_h name_q ip_q mac_q str_z        { newHost($1, $2, $3, $4, $5); }
            node_cf node_e                      { INSERTANDDEL(pNode); }
        | WORKSTATION_T str mac str_z ';'       { newWorkstation(*$2,*$3,*$4); delete $2; delete $3; delete $4; }
        ;
node_e  : '{' node_ps '}'
        | ';'
        ;
node_ps : node_p node_ps
        | 
        ; 
node_cf :
        | COPY_T FROM_T str                     { templates.get(_sNodes, sp2s($3)); }
                node_ps
        ;
node_p  : NOTE_T str ';'                       { pNode->setName(_sNodeNote, sp2s($2)); }
        | PLACE_T place_id ';'                  { pNode->setId(_sPlaceId, $2); }
        | ALARM_T MESSAGE_T str ';'             { pNode->setName(_sNodeAlarmMsg, sp2s($3)); }
        | SET_T str '=' value ';'               { pNode->set(sp2s($2), vp2v($4)); }
        | ADD_T PORTS_T str offs FROM_T int TO_T int offs str ';'
                                                { setLastPort(pNode->addPorts(sp2s($3), sp2s($10), $9, $6, $8, $4)); }
        | ADD_T PORT_T pix_z str str str_z ';'  { setLastPort(pNode->addPort(sp2s($4), sp2s($5), sp2s($6), $3)); }
        | PORT_T pix TYPE_T ix_z str str str_z ';' { setLastPort(pNode->portModType($2, sp2s($5), sp2s($6), sp2s($7), $4)); }
        | PORT_T pix NAME_T str str_z ';'       { setLastPort(pNode->portModName($2, sp2s($4), sp2s($5))); }
        | PORT_T pix NOTE_T strs ';'            { setLastPort(pNode->portSet($2, _sPortNote, slp2vl($4))); }
        | PORT_T pnm NOTE_T str ';'             { setLastPort(pNode->portSet(sp2s($2), _sPortNote, sp2s($4))); }
        | PORT_T pix TAG_T strs ';'             { setLastPort(pNode->portSet($2, _sPortTag, slp2vl($4))); }
        | PORT_T pnm TAG_T str ';'              { setLastPort(pNode->portSet(sp2s($2), _sPortTag, sp2s($4))); }
        | PORT_T pnm SET_T str '=' value ';'    { setLastPort(pNode->portSet(sp2s($2), sp2s($4), vp2v($6))); }
        | PORT_T pix SET_T str '=' vals ';'     { setLastPort(pNode->portSet($2, sp2s($4), vlp2vl($6)));; }
        | PORT_T pnm PARAM_T str '=' str ';'    { setLastPort(pNode->portSetParam(sp2s($2), sp2s($4), sp2s($6))); }
        | PORT_T pix PARAM_T str '=' strs ';'   { setLastPort(pNode->portSetParam($2, sp2s($4), slp2sl($6))); }
        /* host */
        | ALARM_T PLACE_T GROUP_T str ';'       { pNode->setId(_sAlarmPlaceGroupId, cPlaceGroup().getIdByName(qq(), sp2s($4))); }
        | ADD_T PORT_T pix_z str str ip_qq mac_qq str_z ';' { setLastPort(hostAddPort((int)$3, $4,$5,$6,$7,$8)); }
        | PORT_T pnm ADD_T ADDRESS_T ip_a str_z ';'         { setLastPort(portAddAddress($2, $5, $6)); }
        | PORT_T pix ADD_T ADDRESS_T ip_a str_z ';'         { setLastPort(portAddAddress((int)$2, $5, $6)); }
        | ADD_T SENSORS_T offs FROM_T int TO_T int offs str ip ';'  /* index offset ... név offset */
                                                            { setLastPort(pNode->addSensors(sp2s($9), $8, $5, $7, $3, *$10)); delete $10; }
        | PORT_T pnm VLAN_T vlan_id vlan_t set_t ';'        { setLastPort(pNode->portSetVlan(sp2s($2), $4, (enum eVlanType)$5, (enum eSetType)$6)); }
        | PORT_T pix VLAN_T vlan_id vlan_t set_t ';'        { setLastPort(pNode->portSetVlan(     $2,  $4, (enum eVlanType)$5, (enum eSetType)$6)); }
        | PORT_T pnm VLAN_T vlan_id ';'                     { setLastPort(pNode->portSetVlan(sp2s($2), $4, VT_HARD, ST_MANUAL)); }
        | PORT_T pix VLAN_T vlan_id ';'                     { setLastPort(pNode->portSetVlan(     $2,  $4, VT_HARD, ST_MANUAL)); }
        | PORT_T pnm VLANS_T vlan_ids ';'                   { setLastPort(pNode->portSetVlans(sp2s($2), *$4)); delete $4; }
        | PORT_T pix VLANS_T vlan_ids ';'                   { setLastPort(pNode->portSetVlans(     $2,  *$4)); delete $4; }
        /* snmpdev  */
        | PORTS_T BY_T SNMP_T str_z ';'             { snmpdev().setBySnmp(sp2s($4)); setLastPort(_sNul, NULL_IX); }
        | COMMUNITY_T str ';'                       { snmpdev().setName(_sCommunityRd, sp2s($2)); }
        | READ_T COMMUNITY_T str ';'                { snmpdev().setName(_sCommunityRd, sp2s($3)); }
        | WRITE_T COMMUNITY_T str ';'               { snmpdev().setName(_sCommunityWr, sp2s($3)); }
        | for_m
        | eqs
        ;
ip_q    : ips iptype                            { $$ = new QStringPair(sp2s($1),  addrType($2)); }
        | DYNAMIC_T                             { $$ = new QStringPair(QString(), _sDynamic); }
        | LOOKUP_T                              { $$ = new QStringPair(_sLOOKUP,  _sFixIp); }
        | ARP_T                                 { $$ = new QStringPair(_sARP,     _sFixIp); }
        ;
ip_qq   : ips iptype                            { $$ = new QStringPair(sp2s($1),  addrType($2)); }
        | DYNAMIC_T                             { $$ = new QStringPair(QString(), _sDynamic); }
        | NULL_T                                { $$ = NULL; }
        | ARP_T                                 { $$ = new QStringPair(_sARP,     _sFixIp); }
        ;
ip_a    : ips iptype_a                          { $$ = new QStringPair(sp2s($1),  addrType($2)); }
        ;
mac     : MAC_V                                 { $$ = $1; }
        | MAC_T '(' sexpr ')'                   { $$ = new cMac(sp2s($3)); if (!$$->isValid()) yyerror("Invalid MAC."); }
        ;
mac_q   : mac                                   { $$ = new QString($1->toString()); delete $1; }
        | ARP_T                                 { $$ = new QString(_sARP); }
        | NULL_T                                { $$ = NULL; }
        ;
mac_qq  : mac                                   { $$ = new QVariant($1->toString()); delete $1; }       // MAC mint literal
        | '<' '@'                               { $$ = new QVariant(vint(sPortIx)); }                   // MAC azonos az előző port MAC-jával
        | '<' int                               { $$ = new QVariant($2); }                              // Azonos egy megadott indexű port MAC-jával
        | ARP_T                                 { $$ = new QVariant(_sARP); }                           // Az IP címbőé kell kitalálni az "ARP"-al
        | NULL_T                                { $$ = NULL; }                                          // A MAC NULL lessz
        ;
iptype  :                                       { $$ = AT_FIXIP;   }
        | '/' FIXIP_T                           { $$ = AT_FIXIP;   }
        | '/' PRIVATE_T                         { $$ = AT_PRIVATE; }
        | '/' DYNAMIC_T                         { $$ = AT_DYNAMIC; }
        | '/' PSEUDO_T                          { $$ = AT_PSEUDO;  }
        | '/' EXTERNAL_T                        { $$ = AT_EXTERNAL;}
        ;
iptype_a:                                       { $$ = AT_FIXIP; }
        | '/' FIXIP_T                           { $$ = AT_FIXIP; }
        | '/' PRIVATE_T                         { $$ = AT_PRIVATE; }
        | '/' PSEUDO_T                          { $$ = AT_PSEUDO; }
        ;
vlan_t  : str                                   { $$ = (qlonglong)vlanType(sp2s($1)); }
        ;
set_t   : str                                   { $$ = (qlonglong)setType(sp2s($1)); }
        |                                       { $$ = (qlonglong)ST_MANUAL; }
        ;
vlan_ids: vlan_id                               { *($$ = new QList<qlonglong>()) << $1; }
        | vlan_ids ',' vlan_id                  { *($$ = $1) << $3; }
        ;
name_q  : str                                   { $$ = $1; }
        | LOOKUP_T                              { $$ = new QString(); }
        ;
arp     : ADD_T ARP_T SERVER_T ips BY_T SNMP_T COMMUNITY_T str ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::SNMP, sp2s($4), QString(), sp2s($8))); }
        | ADD_T ARP_T SERVER_T ips BY_T SSH_T PROC_T FILE_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::PROC_SSH, sp2s($4), sp2s($9))); }
        | ADD_T ARP_T SERVER_T ips BY_T SSH_T DHCPD_T CONFIG_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::DHCPD_SSH, sp2s($4), sp2s($9))); }
        | ADD_T ARP_T LOCAL_T BY_T PROC_T FILE_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::PROC_LOCAL, QString(), sp2s($7))); }
        | ADD_T ARP_T LOCAL_T BY_T DHCPD_T CONFIG_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::DHCPD_LOCAL, QString(), sp2s($7))); }
        | RE_T UPDATE_T ARPS_T ';'              { pArpServerDefs->updateArpTable(qq()); }
        | PING_T ha ';'                         { QProcess::execute(QString("ping -c1 %1").arg(*$2)); delete $2; }
                                              //{ Pinger().ping(*$2, 1); delete $2; }
        ;
ha      : str                                   { $$ = $1; }
        | ip                                    { $$ = new QString($1->toString()); delete $1; }
        ;
link    : LINKS_T lnktype str_z TO_T lnktype str_z  { NEWOBJ(pLink, cLink($2, $3, $5, $6)); }
         '{' links '}'                              { DELOBJ(pLink); }
        ;
lnktype : BACK_T                                    { $$ = -1; }
        | FRONT_T                                   { $$ =  1; }
        | END_T                                     { $$ =  0; }
        ;
links   : lnk
        | links lnk
        ;
lnk     : lport '&' rport alert str_z ';'           { pLink->insert($5, $4); }
        | int '*' str_z ':' int '&' str_z ':' int ';'   { pLink->insert($3, $5, $7, $9, $1); }
        | for_m
        ;
lport   : str_z ':' str shar                        { pLink->left($1, $3, $4); }
        | str_z ':' int shar                        { pLink->left($1, $3, $4); }
        | str shar                                  { pLink->left($1, NULL_IX, $2); }
        ;
rport   : str_z ':' str shar                        { pLink->right($1, $3, $4); }
        | str_z ':' int shar                        { pLink->right($1, $3, $4); }
        | str shar                                  { pLink->right($1, NULL_IX, $2); }
        | WORKSTATION_T '(' str mac str_z ')'       { pLink->workstation($3,$4, $5); }
        | ATTACHED_T '(' str str_z ')'              { pLink->attached($3, $4); }
        ;
alert   :                                           { $$ = NULL; }
        /* ALERT [(<service_name> [, <host_service_descr>[, <host_service_alarm_msg>[, <node_alarm:msg>]]])] */
//        | ALERT_T '('  ')'                          { $$ = new QStringList(); }
        | ALERT_T '(' strs_z ')'                    { $$ = $3; }
        ;
shar    :                                           { $$ = ES_; }
        | '/' NAME_V                                { $$ = s2share($2);  }
        ;
srv     : service
        | hostsrv
        ;
service : SERVICE_T str str_z    { NEWOBJ(pService, cService());
                                      pService->setName(*$2);
                                      pService->set(_sServiceNote, *$3);
                                      delete $2; delete $3; }
          srvend                    { pService->insert(qq()); DELOBJ(pService); }
        | SET_T ALERT_T SERVICE_T str ';'    { alertServiceId = cService().getIdByName(qq(), *$4); delete $4; }
        ;
srvend  : '{' srv_ps '}'
        | ';'
        ;
srv_ps  : srv_p
        | srv_ps srv_p
        ;
srv_p   : ipprotp int ';'                       { (*pService)[_sProtocolId] = $1; (*pService)[_sPort] = $2; }
        | ipprot ix_z ';'                       { (*pService)[_sProtocolId] = $1; if ($2 >= 0) (*pService)[_sPort] = $2; }
        | SUPERIOR_T SERVICE_T MASK_T str ';'   { (*pService)[_sSuperiorServiceMask] = *$4; delete $4; }
        | COMMAND_T str  ';'                    { (*pService)[_sCheckCmd]   = *$2; delete $2; }
        | PROPERTIES_T str ';'                  { (*pService)[_sProperties] = *$2; delete $2; }
        | MAX_T CHECK_T ATTEMPTS_T int ';'      { (*pService)[_sMaxCheckAttempts]    = $4; }
        | NORMAL_T CHECK_T INTERVAL_T int ';'   { (*pService)[_sNormalCheckInterval] = $4; }
        | RETRY_T CHECK_T INTERVAL_T int ';'    { (*pService)[_sRetryCheckInterval]  = $4; }
        | FLAPPING_T INTERVAL_T str ';'         { (*pService)[_sFlappingInterval]    = *$3; delete $3; }
        | FLAPPING_T MAX_T CHANGE_T int ';'     { (*pService)[_sFlappingMaxChange]   = $4; }
        | SET_T str '=' value ';'               { (*pService)[*$2] = *$4; delete $2; delete $4; }
        | ALARM_T MESSAGE_T str ';'             { (*pService)[_sServiceAlarmMsg] = *$3; delete $3; }
        ;
ipprotp : TCP_T                     { $$ = EP_TCP; }
        | UDP_T                     { $$ = EP_UDP; }
        ;
ipprot  : ICMP_T                    { $$ = EP_ICMP; }
        | IP_T                      { $$ = EP_IP; }
        | NIL_T                     { $$ = EP_NIL; }
        | PROTOCOL_T str            { $$ = cIpProtocol().getIdByName(qq(), *$2, true); delete $2; }
        ;
hostsrv : HOST_T SERVICE_T str ':' str str_z    { NEWOBJ(pHostService, cHostService());
                                                    (*pHostService)[_sNodeId]    = cNode().cRecord::getIdByName(qq(),*$3, true);  delete $3;
                                                    (*pHostService)[_sServiceId] = cService().getIdByName(qq(),*$5);              delete $5;
                                                    (*pHostService)[_sHostServiceNote] =  *$6;                                    delete $6;
                                                }
          hsrvend                               { pHostService->insert(qq(), true); DELOBJ(pHostService); }
        | HOST_T SERVICE_T str ':' str ':' str str_z
                                                { NEWOBJ(pHostService, cHostService());
                                                    (*pHostService)[_sNodeId]    = cNode().cRecord::getIdByName(qq(),*$3, true);  delete $3;
                                                    (*pHostService)[_sServiceId] = cService().getIdByName(qq(),*$5);              delete $5;
                                                    (*pHostService)[_sHostServiceNote] =  *$8;                                    delete $8;
                                                    (*pHostService)[_sPortId] = cNPort().getPortIdByName(qq(), *$7, pHostService->get(_sNodeId).toLongLong(), true); delete $7;
                                                }
          hsrvend                               { pHostService->insert(qq(), true); DELOBJ(pHostService); }
        | SET_T SUPERIOR_T hs TO_T hss ';'      { setSuperior($3, $5); }
        ;
hsrvend : '{' hsrv_ps '}'
        | ';'
        ;
hsrv_ps : hsrv_p
        | hsrv_ps hsrv_p
        ;
hsrv_p  : PRIME_T SERVICE_T str ';'             { (*pHostService)[_sPrimeServiceId] = cService().getIdByName(qq(),*$3); delete $3; }
        | PROTOCOL_T SERVICE_T str ';'          { (*pHostService)[_sProtoServiceId] = cService().getIdByName(qq(),*$3); delete $3; }
        | PORT_T str ';'                        { (*pHostService)[_sPortId] = cNPort().getPortIdByName(qq(), *$2, pHostService->get(_sNodeId).toLongLong(), true); delete $2; }
        | DELEGATE_T HOST_T STATE_T bool_on ';' { (*pHostService)[_sDelegateHostState]  = $4; }
        | COMMAND_T str ';'                     { (*pHostService)[_sCheckCmd] = *$2; delete $2; }
        | PROPERTIES_T str ';'                  { (*pHostService)[_sProperties] = *$2; delete $2; }
        | SUPERIOR_T SERVICE_T str ';'                  { setSuperiorHostService(pHostService, new QString(), $3); }
        | SUPERIOR_T SERVICE_T str ':' str ';'          { setSuperiorHostService(pHostService, $3, $5); }
        | SUPERIOR_T SERVICE_T str ':' str ':' str ';'  { setSuperiorHostService(pHostService, $3, $7, $5); }
        | MAX_T CHECK_T ATTEMPTS_T int ';'      { (*pHostService)[_sMaxCheckAttempts]    = $4; }
        | NORMAL_T CHECK_T INTERVAL_T int ';'   { (*pHostService)[_sNormalCheckInterval] = $4; }
        | RETRY_T CHECK_T INTERVAL_T int ';'    { (*pHostService)[_sRetryCheckInterval]  = $4; }
        | TIME_T PERIODS_T str ';'              { (*pHostService)[_sTimePeriodId]  = cTimePeriod().getIdByName(qq(), *$3); delete $3; }
        | OFF_T LINE_T GROUP_T str ';'          { (*pHostService)[_sOffLineGroupId] = cGroup().getIdByName(qq(), *$4); delete $4; }
        | ON_T LINE_T GROUP_T str ';'           { (*pHostService)[_sOnLineGroupId] = cGroup().getIdByName(qq(), *$4); delete $4; }
        | SET_T str '=' value ';'               { (*pHostService)[*$2] = *$4; delete $2; delete $4; }
        | ALARM_T MESSAGE_T str ';'             { (*pService)[_sHostServiceAlarmMsg] = *$3; delete $3; }
        ;
hs      : str ':' str                           { $$ = new QStringPair(sp2s($1), sp2s($3)); }
        | str ':' '@'                           { $$ = new QStringPair(sp2s($1), _sNul); }
        ;
hss     : hs                                    { *($$ = new QStringPairList) << *$1; delete $1; }
        | hss ',' hs                            { *($$ = $1) << *$3; delete $3; }
        ;
delete  : DELETE_T PLACE_T pattern strs ';'     { foreach (QString s, *$4) { cPlace(). delByName(qq(), s, $3); }       delete $4; }
        | DELETE_T PATCH_T pattern strs ';'     { foreach (QString s, *$4) { cPatch(). delByName(qq(), s, $3, true); } delete $4; }
        | DELETE_T NODE_T pattern strs ';'      { foreach (QString s, *$4) { cNode().  delByName(qq(), s, $3, false); }delete $4; }
        | DELETE_T ONLY_T NODE_T pattern strs ';'{foreach (QString s, *$5) { cNode().  delByName(qq(), s, $4, true); } delete $5; }
        | DELETE_T VLAN_T pattern strs ';'      { foreach (QString s, *$4) { cVLan().  delByName(qq(), s, $3); }       delete $4; }
        | DELETE_T SUBNET_T pattern strs ';'    { foreach (QString s, *$4) { cSubNet().delByName(qq(), s, $3); }       delete $4; }
        | DELETE_T HOST_T str SERVICE_T pattern strs ';'
                                                { foreach (QString s, *$6) { cHostService().delByNames(qq(), sp2s($3), s, $5); } delete $6; }
        | DELETE_T MACRO_T strs ';'             { foreach (QString s, *$3) { templates.del(_sMacros, s); } delete $3; }
        | DELETE_T TEMPLATE_T PATCH_T strs ';'  { foreach (QString s, *$4) { templates.del(_sPatchs, s); } delete $4; }
        | DELETE_T TEMPLATE_T NODE_T strs ';'   { foreach (QString s, *$4) { templates.del(_sNodes,  s); } delete $4; }
        | DELETE_T LINK_T strs ';'              { foreach (QString s, *$3) { cPhsLink().unlink(qq(), s, "%", true); } delete $3; }
        | DELETE_T LINK_T str pattern strs ';'  { foreach (QString s, *$5) { cPhsLink().unlink(qq(), sp2s($3), s, $4); } delete $5; }
        | DELETE_T LINK_T str int ix_z ';'      { cPhsLink().unlink(qq(), sp2s($3), $4, $5); }
        | DELETE_T TABLE_T SHAPE_T pattern strs ';'
                                                { foreach (QString s, *$5) { cTableShape().delByName(qq(), s, $4, false); } delete $5; }
        | DELETE_T ENUM_T TITLE_T  strs ';'     { foreach (QString s, *$4) { cEnumVal().delByTypeName(qq(), s, false); } delete $4; }
        | DELETE_T ENUM_T TITLE_T  PATTERN_T strs ';'
                                                { foreach (QString s, *$5) { cEnumVal().delByTypeName(qq(), s, true); } delete $5; }
        | DELETE_T ENUM_T TITLE_T  str strs ';' { foreach (QString s, *$5) { cEnumVal().delByNames(qq(), sp2s($4), s); } delete $5; }
        | DELETE_T GUI_T pattern strs MENU_T ';'{ foreach (QString s, *$4) { cMenuItem().delByAppName(qq(), s, $3); } delete $4; }
        ;
pattern :                                       { $$ = false; }
        | PATTERN_T                             { $$ = true;  }
        ;
scan    : SCAN_T LLDP_T snmph ';'               { scanByLldp(qq(), *$3); delete $3; }
        ;
snmph   : str                                   { if (!($$ = new cSnmpDevice())->fetchByName(qq(), sp2s($1))) yyerror("ismeretlen SNMP eszköz név"); }
        ;
gui     : tblmod
        | ENUM_T str str TITLE_T str  ';'       { cEnumVal().setName(_sEnumValName, sp2s($3)).setName(_sEnumTypeName, sp2s($2)).setName(_sEnumValNote, sp2s($5)).insert(qq()); }
        | appmenu
        ;
tblmod  : TABLE_T str_z SHAPE_T str str_z '{'   { pTableShape = newTableShape($2, $4, $5); }
            tmodps
          '}'                                   { pTableShape->insert(qq()); }
        ;
tmodps  : tmodp
        | tmodps tmodp
        ;
tmodp   : SET_T DEFAULTS_T ';'                  { pTableShape->setDefaults(qq()); }
        | SET_T str '=' value ';'               { pTableShape->set(sp2s($2), vp2v($4)); }
        | TABLE_T TYPE_T strs ';'               { pTableShape->set(_sTableShapeType, slp2sl($3)); }
        | TABLE_T TITLE_T str ';'               { pTableShape->set(_sTableShapeTitle, sp2s($3)); }
        | TABLE_T READ_T ONLY_T bool_on ';'     { pTableShape->set(_sIsReadOnly, $4); }
        | TABLE_T PROPERTIES_T str ';'          { pTableShape->set(_sProperties, sp2s($3)); }
        | LEFT_T SHAPE_T tmod ';'               { pTableShape->setId(_sLeftShapeId, $3); }
        | RIGHT_T SHAPE_T tmod ';'              { pTableShape->setId(_sRightShapeId, $3); }
        | REFINE_T str ';'                      { pTableShape->setName(_sRefine, sp2s($2)); }
        | TABLE_T INHERIT_T TYPE_T str ';'      { pTableShape->setName(_sTableInheritType, sp2s($4)); }
        | INHERIT_T TABLE_T NAMES_T strs ';'    { pTableShape->set(_sInheritTableNames, slp2vl($4)); }
        | TABLE_T VIEW_T RIGHTS_T str ';'       { pTableShape->setName(_sViewRights, sp2s($4)); }
        | TABLE_T EDIT_T RIGHTS_T str ';'       { pTableShape->setName(_sEditRights, sp2s($4)); }
        | TABLE_T DELETE_T RIGHTS_T str ';'     { pTableShape->setName(_sRemoveRights, sp2s($4)); }
        | TABLE_T INSERT_T RIGHTS_T str ';'     { pTableShape->setName(_sInsertRights, sp2s($4)); }
        | SET_T str '.' str '=' value ';'       { pTableShape->fset(sp2s($2), sp2s($4), vp2v($6)); }
        | SET_T '(' strs ')' '.' str '=' value ';'{ pTableShape->fsets(slp2sl($3), sp2s($6), vp2v($8)); }
        | FIELD_T str TITLE_T str ';'           { pTableShape->fset(sp2s($2),_sTableShapeFieldTitle, sp2s($4)); }
        | FIELD_T str NOTE_T str ';'           { pTableShape->fset(sp2s($2),_sTableShapeFieldNote, sp2s($4)); }
        | FIELD_T str TITLE_T str str ';'       { pTableShape->fset(    *$2,    _sTableShapeFieldTitle, sp2s($4)   );
                                                  pTableShape->fset(sp2s($2),_sTableShapeFieldNote, sp2s($5)); }
        | FIELD_T strs VIEW_T RIGHTS_T str ';'  { pTableShape->fsets(slp2sl($2), _sViewRights, sp2s($5)); }
        | FIELD_T strs EDIT_T RIGHTS_T str ';'  { pTableShape->fsets(slp2sl($2), _sEditRights, sp2s($5)); }
        | FIELD_T strs PROPERTIES_T str ';'     { pTableShape->fsets(slp2sl($2), _sProperties, sp2s($4)); }
        | FIELD_T strs HIDE_T bool_on ';'       { pTableShape->fsets(slp2sl($2), _sIsHide, $4); }
        | FIELD_T strs DEFAULT_T VALUE_T str ';'{ pTableShape->fsets(slp2sl($2), _sDefaultValue, sp2s($5)); }
        | FIELD_T strs READ_T ONLY_T bool_on ';'{ pTableShape->fsets(slp2sl($2), _sIsReadOnly, $5); }
        | FIELD_T strs ADD_T FILTER_T str str_z ';' { pTableShape->addFilter(slp2sl($2), sp2s($5), sp2s($5)); }
        | FIELD_T strs ADD_T FILTERS_T strs ';'     { pTableShape->addFilter(slp2sl($2), *$5); delete $5; }
        | FIELD_T SEQUENCE_T int0 strs ';'      { pTableShape->setFieldSeq(slp2sl($4), $3); }
        | FIELD_T strs ORD_T int0 strs ';'      { pTableShape->setOrders(*$2, *$5, $4); delete $2; delete $5; }
        | FIELD_T '*'  ORD_T int0 strs ';'      { pTableShape->setAllOrders(*$5, $4); delete $5; }
        | FIELD_T ORD_T SEQUENCE_T int0 strs ';'{ pTableShape->setOrdSeq(slp2sl($5), $4); }
        ;
int0    : int                                   { $$ = $1; }
        |                                       { $$ = 0; }
        ;
tmod    : str                                   { $$ = cTableShape().getIdByName(sp2s($1)); }
        ;
appmenu : GUI_T str                             { pMenuApp = $2;}
            '{' menus '}'                       { pDelete(pMenuApp); }
        ;
menus   : menu
        | menu menus
        ;
menu    : str MENU_T str_z                      { newMenuMenu(sp2s($1), sp2s($3)); }
            miops  '{' mitems '}'               { delMenuItem(); }
        ;
mitems  : mitem mitems
        |
        ;
mitem   : str SHAPE_T str str_z                 { newMenuItem(sp2s($1), sp2s($3), sp2s($4), ":shape=%1:"); }
            miops ';'                           { delMenuItem(); }
        | str EXEC_T str str_z                  { newMenuItem(sp2s($1), sp2s($3), sp2s($4), ":exec=%1:"); }
            miops ';'                           { delMenuItem(); }
        | str OWN_T str str_z                   { newMenuItem(sp2s($1), sp2s($3), sp2s($4), ":own=%1:"); }
            miops ';'                           { delMenuItem(); }
        | str MENU_T str_z                      { newMenuMenu(sp2s($1), sp2s($3)); }
            miops  '{' mitems '}'               { delMenuItem(); }
        ;
miops   : miop miops
        |
        ;
miop    : TOOL_T TIP_T str ';'                  { setToolTip(sp2s($3)); }
        | WHATS_T THIS_T str ';'                { setWhatsThis(sp2s($3)); }
        ;
%%

static inline bool isXDigit(QChar __c) {
    int cc;
    if (__c.isDigit())  return true;
    if (!__c.isLetter()) return false;
    cc = __c.toLatin1();
    return (cc >= 'A' && cc <= 'F') || (cc >= 'a' && cc <= 'f');
}
static inline int digit2num(QChar c) { return c.toLatin1() - '0'; }
static inline int xdigit2num(QChar c) { return c.isDigit() ? digit2num(c) : (c.toUpper().toLatin1() - 'A' + 10); }
static inline bool isOctal(QChar __c) {
    if (!__c.isDigit()) return false;
    int c = __c.toLatin1();
    return c < '8';
}

static QString *yygetstr()
{
    QString *ps = new QString;
    QChar c;
    while (QChar('\"') != (c = yyget())) {
        if (c == QChar('\\')) switch ((c = yyget()).toLatin1()) {
            case '\\':              break;
            case 'e':  c = QChar::fromLatin1('\x1b');  break;
            case 'n':  c = QChar::fromLatin1('\n');    break;
            case 't':  c = QChar::fromLatin1('\t');    break;
            case 'r':  c = QChar::fromLatin1('\r');    break;
            case 'x':  {
                int n = 0;
                for (char i = 0; i < 2; i++) {
                    if (!isXDigit(c = yyget())) { yyunget(c); break; }
                    n *= 16;
                    n = xdigit2num(c);
                }
                c = QChar(n);
                break;
            }
            case '0':   case '1':   case '2':   case '3':
            case '4':   case '5':   case '6':   case '7': {
                int n = 0;
                for (char i = 0; i < 3; i++) {
                    if (!isOctal(c = yyget())) { yyunget(c); break; }
                    n *= 8;
                    n = digit2num(c);
                }
                c = QChar(n);
                break;
            }
        }
        else if (c.isNull()) {
            delete ps;
            yyerror("EOF in text literal.");
            break;
        }
        *ps += c;
    }
    return ps;
}

static QString *yygetstr2(const QString& mn)
{
    static const QString ee = "EOF in str litaeral";
    QString *ps = new QString;
    QChar c;
    while (!((c = yyget()).isNull())) {
        if (c != QChar('$')) {  // Lehet a str vége?
            *ps += c;           // nem
            continue;
        }
        int i;
        for (i = 0; i < mn.size(); i++) {
            if (mn[i] != (c = yyget())) {
                if (c.isNull()) break;
                *ps += QChar('$');
                if (i) *ps += mn.mid(0, i);
                i = -1;  // Nincs találat, ez tuti nem egyenlő mn.size() -vel.
            }
        }
        if (c.isNull()) break;
        if (i == mn.size()) {       // A literal stimmel, de a végén is kell egy '$'
            if (QChar('$') == (c = yyget())) {
                return ps;
            }
            *ps += QChar('$');
            *ps += mn;
            *ps += c;
        }
    }
    PDEB(VVERBOSE) << ee << _sSpace << *ps;
    delete ps;
    yyerror(ee);    // az yyerror() olyan mintha visszatérne, pedig dehogy.
    return NULL;
}

/*
static QString toString(const QStringList& sl) {
    QString r = "[";
    foreach(QString s, sl) { r += " \"" + s + "\","; }
    r.chop(1);
    return r + "]";
}
*/

static int isAddress(const QString& __s)
{
    //_DBGFN() << " @(" << __s << ") macbuff = " << macbuff << endl;
    if (__s.indexOf("0x", 0, Qt::CaseInsensitive) ==  0) return 0;
    if (__s.contains(QRegExp("[g-zG-Z_]"))) return 0;
    QChar c;
    QString as = __s;
    while (1) {
        c = yyget();
        if (c.isNull()) break;
        if (!isXDigit(c) && c != QChar(':')) break;
        as += c;
    }
    yyunget(c);
    // Ha csak egy sima szám
    bool ok;
    yylval.i = as.toLongLong(&ok);
    if (ok) {
        PDEB(VVERBOSE) << "INTEGER : " << as << endl;
        return INTEGER_V;
    }
    // MAC ?
    yylval.mac = new cMac(as);
    if (yylval.mac->isValid()) {
        PDEB(VVERBOSE) << "MAC : " << as << endl;
        return MAC_V;
    }
    delete yylval.mac;
    // IP ADDRESS ?
    int n = as.count(QChar(':'));
    if (n > 2 || (n > 1 && as.contains("::"))) {
        yylval.ip = new QHostAddress();
        if (yylval.ip->setAddress(as)) {
            PDEB(VVERBOSE) << "IPV6 : " << as << endl;
            return IPV6_V;
        }
        delete yylval.ip;
    }
    // Egyik sem
    if (__s.size() < as.size()) macbuff.insert(0, as.right(as.size() - __s.size()));    // yyunget
    // _DBGFNL() << " __s = " << __s << ", macbuff = " << macbuff << endl;
    return 0;
}

#define TOK(t)  { #t, t##_T },
static int yylex(void)
{
    static const char cToken[] = "=+-*(),;|&<>^{}[]:.#@";
    static const struct token {
        const char *name;
        int         value;
    } sToken[] = {
        TOK(MACRO) TOK(FOR) TOK(DO) TOK(TO) TOK(SET) TOK(CLEAR) TOK(BEGIN) TOK(END) TOK(ROLLBACK)
        TOK(VLAN) TOK(SUBNET) TOK(PORTS) TOK(PORT) TOK(NAME) TOK(SHARED) TOK(SENSORS)
        TOK(PLACE) TOK(PATCH) TOK(HUB) TOK(SWITCH) TOK(NODE) TOK(HOST) TOK(ADDRESS)
        TOK(PARENT) TOK(IMAGE) TOK(FRAME) TOK(TEL) TOK(NOTE) TOK(MESSAGE) TOK(ALARM)
        TOK(PARAM) TOK(TEMPLATE) TOK(COPY) TOK(FROM) TOK(NULL) TOK(VIRTUAL)
        TOK(INCLUDE) TOK(PSEUDO) TOK(OFFS) TOK(IFTYPE) TOK(WRITE) TOK(RE)
        TOK(ADD) TOK(READ) TOK(UPDATE) TOK(ARPS) TOK(ARP) TOK(SERVER) TOK(FILE) TOK(BY)
        TOK(SNMP) TOK(SSH) TOK(COMMUNITY) TOK(DHCPD) TOK(LOCAL) TOK(PROC) TOK(CONFIG)
        TOK(ATTACHED) TOK(LOOKUP) TOK(WORKSTATION) TOK(LINKS) TOK(BACK) TOK(FRONT)
        TOK(TCP) TOK(UDP) TOK(ICMP) TOK(IP) TOK(NIL) TOK(COMMAND) TOK(SERVICE) TOK(PRIME)
        TOK(MAX) TOK(CHECK) TOK(ATTEMPTS) TOK(NORMAL) TOK(INTERVAL) TOK(RETRY)
        TOK(FLAPPING) TOK(CHANGE) { "TRUE", TRUE_T },{ "FALSE", FALSE_T }, TOK(ON) TOK(OFF) TOK(YES) TOK(NO)
        TOK(DELEGATE) TOK(STATE) TOK(SUPERIOR) TOK(TIME) TOK(PERIODS) TOK(LINE) TOK(GROUP)
        TOK(USER) TOK(DAY) TOK(OF) TOK(PERIOD) TOK(PROTOCOL) TOK(ALERT) TOK(INTEGER) TOK(FLOAT)
        TOK(DELETE) TOK(ONLY) TOK(PATTERN) TOK(STRING) TOK(SAVE) TOK(TYPE) TOK(STEP)
        TOK(MASK) TOK(LIST) TOK(VLANS) TOK(ID) TOK(DYNAMIC) TOK(FIXIP) TOK(PRIVATE) TOK(PING)
        TOK(NOTIF) TOK(ALL) TOK(RIGHTS) TOK(REMOVE) TOK(SUB) TOK(PROPERTIES) TOK(MAC) TOK(EXTERNAL)
        TOK(LINK) TOK(LLDP) TOK(SCAN) TOK(TABLE) TOK(FIELD) TOK(SHAPE) TOK(TITLE) TOK(REFINE)
        TOK(LEFT) TOK(DEFAULTS) TOK(ENUM) TOK(RIGHT) TOK(VIEW) TOK(INSERT) TOK(EDIT)
        TOK(INHERIT) TOK(NAMES) TOK(HIDE) TOK(VALUE) TOK(DEFAULT) TOK(FILTER) TOK(FILTERS)
        TOK(ORD) TOK(SEQUENCE) TOK(MENU) TOK(GUI) TOK(OWN) TOK(TOOL) TOK(TIP) TOK(WHATS) TOK(THIS)
        TOK(EXEC) TOK(TAG) TOK(ANY) TOK(BOOLEAN) TOK(CHAR) TOK(IPADDRESS) TOK(REAL) TOK(URL)
        { "WST", WORKSTATION_T }, // rövidítések
        { "ATT", ATTACHED_T },
        { "INT", INTEGER_T },
        { "MSG", MESSAGE_T },
        { "CMD", COMMAND_T },
        { "PROP", PROPERTIES_T },
        { "PROTO", PROTOCOL_T },
        { "SEQ", SEQUENCE_T },
        { NULL, 0 }
    };
    // DBGFN();
recall:
    yylval.u = NULL;
    QChar     c;
    // Elvalaszto karakterek és kommentek atlepese
    // Fajl vege eseten vege
    while((c = yyget()).isNull() || c.isSpace() || c == QChar('\n') || c == QChar('/')) {
        if (c.isNull()) return 0;   // EOF, vége
        if (c == QChar('/')) {      // comment jel első karaktere?
            QChar c2 = yyget();
            if (c2 == QChar('/')) { // commwnt '//'
                while (!(c = yyget()).isNull() && c != QChar('\n'));
                if (c.isNull()) return 0;
                continue;
            }
            if (c2 == QChar('*')) { // commwnt '/*'
                find_comment_end:
                while (!(c = yyget()).isNull() && c != QChar('*'));
                if (c.isNull() || (c = yyget()).isNull()) yyerror("EOF in commment.");
                if (c == QChar('/')) continue;  // comment végét megtaláltuk
                goto find_comment_end;          // keressük tövább
            }
            if (!c2.isNull()) yyunget(c2);  // A feleslegesen beolvasott karaktert vissza
            PDEB(VVERBOSE) << "C TOKEN : /" << endl;
            return c.toLatin1();             // nem komment, hanem a '/' token
        }
    }
    // MACRO: $NAME
    if (c == QChar('$')) {
        QString mn = "";    // A makró neve
        while ((c = yyget()).isLetterOrNumber() || c == QChar('_')) mn += c;
        if (mn.size() == 0 && c != QChar('$')) // Ez nem makró, és nem is string, token
            return '$';
        if (c == QChar('$')) {  // Ez nem is makró, hanem string literál : $$ bla.bla $$, vagy $aa$ bla.bla $aa$
            yylval.s = yygetstr2(mn);
            PDEB(VVERBOSE) << "ylex : $$ STRING : " << *(yylval.s) << endl;
            return STRING_V;
        }
        if (!c.isNull() && c.isSpace()) c = yyget();
        QStringList    parms;
        //PDEB(VVERBOSE) << "yylex: exec macro: " << mn << endl;
        if (c == QChar('(')) { // Makró paraméterek
            static const char me[] = "EOF in macro parameter list.";
            //static const char mi[] = "Invalid char in macro parameter list.";
            QString parm;
            do {
                c = yyget();
                // QStdErr << "Macro parms, yyget() = " << c << endl;
                if (c.isNull()) yyerror(me);
                if (c.isSpace()) {
                    if (parm.isEmpty()) continue;   // Vezető space eldobása
                    while (!c.isNull() && c.isSpace()) c = yyget();
                    if (c.isNull()) yyerror(me);
                    if (c != QChar(',') && c != QChar(')')) yyerror("Macro parameter contain space chars.");
                }
                if (c == QChar(',') || c == QChar(')')) {
                    parms << parm;
                    parm = "";
                    if (c == QChar(')')) break;
                    continue;   // if c == ','
                }
                if (c == QChar('"')) {
                    if (parm != "") yyerror(QString("Macro parameter contain quote chars, and is not first character. Prefix : %1").arg(parm));
                    QString *p = yygetstr();
                    parms << *p;
                    delete p;
                    while ((c = yyget()).isSpace() && !c.isNull());
                    if (c == QChar(')')) break;
                    if (c == QChar(',')) continue;
                    if (c.isNull())      yyerror(me);
                    yyerror("Incoplet string in macro parameter");
                }
                parm += c;
            } while (true);
            //PDEB(VVERBOSE) << "Macro params : " << toString(parms);
        }
        else {
            yyunget(c);
        }
        QString mm;                         // Kifejtett makró
        const QString& mt(templates._get(_sMacros, mn));     // Makró törzs
        for (int i = 0; i < mt.size(); i++) {
            QChar c = mt[i];
            if (c == QChar('%') && mt[i +1].isDigit()) { // Macro parameter
                int n = mt[++i].toLatin1() - '1';
                if (parms.size() > n) {
                    mm += parms[n];
                }
            }
            else mm += c;
        }
        //PDEB(VVERBOSE) << "Macro Body : \"" << mm << "\"" << endl;
        insertCode(mm);
        goto recall;
    }
    // VALUE INT
    if (c.isDigit()) {
        QChar cf = c;    // Meg kelleni fog
        QString sn = cf;
        while (((c = yyget()).isDigit())
           || (QChar('x') == c.toLower() && cf == QChar('0') && sn.size() == 1)
           || (isXDigit(c)     && sn.indexOf("0x", 0, Qt::CaseInsensitive) == 0)
           || (c == QChar('.') && sn.indexOf("0x", 0, Qt::CaseInsensitive) != 0)) {
            sn += c;
        }
        // PDEB(VVERBOSE) << "INT U : " << c << endl;
        yyunget(c);
        bool ok;
        switch (sn.count(QChar('.'))) {
        case 1:  // Float
            yylval.r = sn.toDouble(&ok);
            if (!ok) {   // Ez nem float
                yyerror("Invalid float number.");
            }
            PDEB(VVERBOSE) << "ylex : FLOAT : " << yylval.r << endl;
            return FLOAT_V;
        case 3:  // IP
            yylval.ip = new QHostAddress();
            if (!yylval.ip->setAddress(sn)) {
                yyerror("Invalid IP number.");
            }
            PDEB(VVERBOSE) << "ylex : IPV4 : " << yylval.ip->toString() << endl;
            return IPV4_V;
            break;
        case 0: // Int
            break;
        default:
            yyerror("Invalid number, too many point.");
        }
        int r;
        if (0 != (r = isAddress(sn))) return r;
        if (cf == QChar('0')) {
            char cn = sn.size() > 1 ? sn[1].toLatin1() : 0;
            if (cn == 'x' || cn == 'X') {   // HEXA
                yylval.i = sn.toLongLong(&ok, 16);
            }
            else {                          // OCTAL
                yylval.i = sn.toLongLong(&ok, 8);
            }
        }
        else {
            yylval.i = sn.toLongLong(&ok, 10);
        }
        if (!ok) yyerror("Pprogram error");
        PDEB(VVERBOSE) << "ylex : INTEGER : " << yylval.i << endl;
        return INTEGER_V;
    }
    // VALUE STRING tipusu
    if (c == QChar('\"')) {
        yylval.s = yygetstr();
        PDEB(VVERBOSE) << "ylex : STRING : " << *(yylval.s) << endl;
        return STRING_V;
    }
    // Egybetus tokenek
    if (strchr(cToken, c.toLatin1())) {
        int r;
        if (c == QChar(':') && 0 != (r = isAddress(":"))) return r;
        int ct = c.toLatin1();
        PDEB(VVERBOSE) << "ylex : char token : '" << c << "' (" <<  ct << _sABraE << endl;
        return ct;
    }
    QString *sp = new QString();
    while (c.isLetterOrNumber() || c == QChar('_')) {
        *sp += c;
        c = yyget();
    }
    int r;
    if (!c.isNull()) yyunget(c);
    if (0 != (r = isAddress(*sp))) return r;
    if (sp->isEmpty()) yyerror("Invalid character");
    for (const struct token *p = sToken; p->name; p++) {
        if (p->name == *sp) {
            PDEB(VVERBOSE) << "ylex TOKEN : " << *sp << endl;
            delete sp;
            return p->value;
        }
    }
    yylval.s = sp;
    PDEB(VVERBOSE) << "ylex : NAME : " << *sp << endl;
    return NAME_V;
}

/* */
static void forLoopMac(QString *_in, QVariantList *_lst)
{
    QString s;
    foreach (QVariant v, *_lst) {
        s += _sDollar + *_in + _sABraB + v.toString() + _sABraE + _sSpace;
    }
    PDEB(VVERBOSE) << "forLoopMac inserted : \"" << s << _sDQuote << endl;
    insertCode(s);
    delete _in; delete _lst;
}

static void forLoop(QString *_in, QVariantList *_lst)
{
    QString *pmm = new QString("__");
    templates.set (_sMacros, *pmm, *_in);
    delete _in;
    forLoopMac(pmm, _lst);
}

static void forLoop(QString *m, qlonglong fr, qlonglong to, qlonglong st)
{
    QString s;
    for (qlonglong i = fr; i <= to; i += st) {
        s += nameAndNumber(*m, i) + _sSpace;
    }
    insertCode(s);
    delete m;
}

static QStringList *listLoop(QString *m, qlonglong fr, qlonglong to, qlonglong st)
{
    QStringList *psl = new QStringList();
    for (qlonglong i = fr; i <= to; i += st) {
        *psl << nameAndNumber(*m, i);
    }
    delete m;
    return psl;
}

static intList *listLoop(qlonglong fr, qlonglong to, qlonglong st)
{
    intList *ilp = new intList();
    for (qlonglong i = fr; i <= to; i += st) {
        *ilp << i;
    }
    return ilp;
}

