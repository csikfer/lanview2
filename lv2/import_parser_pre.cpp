/*!
 * @file import_parser_pre.cpp
 * @author Csiki Ferenc <csikfer@gmail.com>
 * @brief Nem valódi forrás állomány, az imput_parser.yy forrás inkludálja a definíciók elött.
 *        Csak kényelmi szemponból van külön fájlban, mivel az yy fájlt a qtcreator csak részben támogatja.
 */

#include <math.h>
#include "lanview.h"
#include "guidata.h"
#include "others.h"
#include "ping.h"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  PARSER
#include "import_parser.h"

static void insertCode(const QString& __txt);
static QSqlQuery      *piq = NULL;
static inline QSqlQuery& qq() { if (piq == NULL) EXCEPTION(EPROGFAIL); return *piq; }

class cHostServices : public tRecordList<cHostService> {
public:
    cHostServices(QSqlQuery& q, QString * ph, QString * pp, QString * ps, QString * pn) : tRecordList<cHostService>()
    {
        tRecordList<cNode> hl;
        hl.fetchByNamePattern(q, *ph, false); delete ph;
        tRecordList<cService> sl;
        sl.fetchByNamePattern(q, *ps, false); delete ps;
        for (tRecordList<cNode>::iterator i = hl.begin(); i < hl.end(); ++i) {
            cNode &h = **i;
            for (tRecordList<cService>::iterator j = sl.begin(); j < sl.end(); ++j) {
                cService &s = **j;
                cHostService *phs = new cHostService();
                phs->setId(_sNodeId,    h.getId());
                phs->setId(_sServiceId, s.getId());
                if (pn) phs->setName(_sHostServiceNote, *pn);
                if (pp) phs->setId(_sPortId, cNPort().getPortIdByName(q, *pp, h.getId()));
                *this << phs;
            }
        }
        pDelete(pn);
        pDelete(pp);
    }
    cHostServices(QSqlQuery& q, cHostService *&_p)  : tRecordList<cHostService>(_p)
    {
        _p = NULL;
        while (true) {
            cHostService *p = new cHostService;
            if (!p->next(q)) {
                delete p;
                return;
            }
            this->append(p);
        }
    }
    void setsPort(QSqlQuery& q, const QString& pn) {
        for (int i = 0; i < size(); ++i) {
            at(i)->set(_sPortId, cNPort().getPortIdByName(q, pn, at(i)->getId(_sNodeId)));
        }
    }
    void cat(cHostServices *pp) {
        cHostService *p;
        while (NULL != (p = pp->pop_front(false))) pp->append(p);
        delete pp;
    }
};

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
        if (__cont.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Üres minta."))
        (*this)[__type].set(__name, __cont);
    }
    /// Egy adott nevű template elhelyezése a konténerbe, és az adatbázisban.
    void save(const QString& __type, const QString& __name, const QString& __cont, const QString& __descr) {
        if (__cont.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Üres minta."))
        (*this)[__type].save(qq(), __name, __cont, __descr);
    }
    /// Egy adott nevű template törlése a konténerből, és az adatbázisból.
    void del(const QString& __type, const QString& __name) {
        (*this)[__type].del(qq(), __name);
    }
};

class  cArpServerDefs;
static cArpServerDefs *pArpServerDefs = NULL;
static void insertCode(const QString& __txt);
static QString  macbuff;
static QString lastLine;

static qlonglong       globalPlaceId = NULL_ID;

static int yyparse();
cError *importLastError = NULL;

int importParse()
{
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

#define UNKNOWN_PLACE_ID 0LL

typedef QList<QStringPair> QStringPairList;

typedef QList<qlonglong> intList;

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
    static void clear() { stack.clear(); }
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

static  qlonglong       id;

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
static cHostServices*pHostServices = NULL;
static cHostService*pHostService2 = NULL;
static cTableShape *pTableShape = NULL;
static qlonglong           alertServiceId = NULL_ID;
static QMap<QString, qlonglong>    ivars;
static QMap<QString, QString>      svars;
static QSqlQuery  * pq2 = NULL;
QStack<c_yyFile> c_yyFile::stack;

static cTableShapeField *pTableShapeField;

void initImportParser()
{
    if (pArpServerDefs == NULL)
        pArpServerDefs = new cArpServerDefs();
    // notifswitch tömb = SET, minden on-ba, visszaolvasás listaként
    if (piq == NULL) {
        piq = newQuery();
    }

    yyflags = 0;
    globalPlaceId = NULL_ID;
    actVlanId = -1;
    netType = NT_INVALID; // firstSubNet = ;
    alertServiceId = NULL_ID;

    pDelete(importLastError);
    importLineNo = 0;
    c_yyFile::clear();
}

void downImportParser()
{
    pDelete(pArpServerDefs);
    pDelete(piq);
    macbuff.clear();
    lastLine.clear();

    templates.clear();
    actVlanName.clear();
    actVlanNote.clear();;
    pDelete(pPatch);
    pDelete(pImage);
    pDelete(pUser);
    pDelete(pGroup);
    pDelete(pNode);
    pDelete(pLink);
    pDelete(pService);
    pDelete(pHostServices);
    pDelete(pHostService2);
    pDelete(pTableShape);
    ivars.clear();
    svars.clear();
    pDelete(pq2);
}

QSqlQuery& qq2()
{
    if (pq2 == NULL) pq2 = newQuery();
    return *pq2;
}


enum {
    EP_NIL = -1,
    EP_IP  = 0,
    EP_ICMP = 1,
    EP_TCP = 6,
    EP_UDP = 17
};

/* */

// QMap<QString, duble>        rvars;
static const QString       sPortIx = "PI";   // Port index
static const QString       sPortNm = "PN";   // Port név


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
    if (!ivars.contains(n)) yyerror(QObject::trUtf8("Integer variable %1 not found").arg(n));
    return ivars[n];
}

static inline QString& vstr(const QString& n){
    if (!svars.contains(n)) yyerror(QObject::trUtf8("String variable %1 not found").arg(n));
    return svars[n];
}

static inline const QString& nextNetType() {
    enum eSubNetType r = netType;
    switch (r) {
    case NT_PRIMARY:    netType = NT_SECONDARY;      break;
    case NT_SECONDARY:  break;
    case NT_PRIVATE:
    case NT_PSEUDO:     netType = NT_INVALID;        break;
    default:            yyerror(QObject::trUtf8("Compiler error."));
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

/// @param __e Side
/// @param __n Node name
/// @param __p Port name
/// @param __s Share type
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
    _DBGFNL() << QChar(' ') << portId(__e) << endl;
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
        if (nid == NULL_ID) EXCEPTION(EDATA, -1, QObject::trUtf8("Nincs megadva node!"));
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
    _DBGFNL() << QChar(' ') << portId(__e) << endl;
}

void cLink::workstation(QString * __n, cMac * __mac, QString * __d)
{
    portId2 = newWorkstation(*__n, *__mac, *__d);
    delete __n;
    delete __mac;
    delete __d;
    newNode = true;
    _DBGFNL() << QChar(' ') << portId2 << endl;
}

void cLink::attached(QString * __n, QString * __d)
{
    portId2 = newAttachedNode(*__n, *__d);;
    delete __n;
    delete __d;
    newNode = true;
    _DBGFNL() << QChar(' ') << portId2 << endl;
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
        if (__srv->size() > 0 && __srv->at(0).size() > 0) { if (!se.fetchByName(__srv->at(0))) yyerror(QObject::trUtf8("Invalis alert service name.")); }
        else                                              { if (!se.fetchById(alertServiceId)) yyerror(QObject::trUtf8("Nothing default alert service.")); }
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
                if (!newNode) yyerror(QObject::trUtf8("Invalid port type"));
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

/// A sor maradékát eldobja.
/// megnézi hány space típuso karakterrel kezdődő sor, és eldobja az összes következő sort,
/// amelyik nem kezdődik több space típuső karakterrel, vagy üres.
static void yyskip(bool b)
{
    macbuff.clear();
    if (!b) return;
    QRegExp ns("\\S");      // Az első nem space karakter kereséséhez
    QRegExp as("^\\s*$");   // Csak space karakter
    int n = lastLine.indexOf(ns);
    if (n < 0) yyerror(QObject::trUtf8("Indent error ?"));
    do {
        ++importLineNo;
        lastLine = importInputStream->readLine();
        PDEB(INFO) << "yyskip : \"" << lastLine << "\"" << endl;
        if (lastLine.isNull()) break;               // fájl vége
        if (lastLine.indexOf(as) == 0) continue;    // Üres sor
    } while (n < lastLine.indexOf(ns));
    macbuff = lastLine;
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
    if (h < 0 || h >  24) yyerror(QObject::trUtf8("Invalid hour value"));
    if (m < 0 || m >= 60) yyerror(QObject::trUtf8("Invalid min value"));
    if (s < 0 || s >= 60) yyerror(QObject::trUtf8("Invalid sec value"));
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

static void newNode(QStringList * t, QString *name, QString *d)
{
    _DBGFN() << "@(" << *name << QChar(',') << *d << ")" << endl;
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
