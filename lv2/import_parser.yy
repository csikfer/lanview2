%{

#include <stdio.h>
#include <math.h>
#include "lanview.h"
#include "guidata.h"
#include "others.h"
#include "lv2link.h"
#include "lv2service.h"
#include "lv2glpi.h"

#undef  __MODUL_NAME__
#define __MODUL_NAME__  PARSER

#include "import_parser.h"
#include "scan.h"
#include "vardata.h"
#include "export.h"

#define  YYERROR_VERBOSE
#define  YYINITDEPTH    5000

/// A parser hiba függvénye. A hiba üzenettel dob egy kizárást.
static inline int yyerror(QString em)
{
    EXCEPTION(EPARSE, -1, em);
    return -1;
}
/// A parser hiba függvénye. A hiba üzenettel dob egy kizárást.
static inline int yyerror(const char * em)
{
    return yyerror(QString(em));
}

static void insertCode(const QString& __txt);
static QSqlQuery      *piq = nullptr;
static inline QSqlQuery& qq() { if (piq == nullptr) EXCEPTION(EPROGFAIL); return *piq; }

class cHostServices : public tRecordList<cHostService> {
public:
    cHostServices(QSqlQuery& q, QString * ph, QString * pp, QString * ps, QString * pn) : tRecordList<cHostService>()
    {
        tRecordList<cNode> hl;
        hl.fetchByNamePattern(q, *ph, false);
        QString n;
        n = *ph; pDelete(ph);
        if (hl.isEmpty()) yyerror(QObject::tr("A %1 mintára egyetlen hálózati elem neve sem illeszkedik.").arg(n));
        tRecordList<cService> sl;
        sl.fetchByNamePattern(q, *ps, false);
        n = *ps; pDelete(ps);
        if (sl.isEmpty()) yyerror(QObject::tr("A %1 mintára egyetlen szolgáltatás típus neve sem illeszkedik.").arg(n));
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
        if (isEmpty()) yyerror(QObject::tr("A megadott minta kollekcióra nincs semmilyen találat."));
        pDelete(pn);
        pDelete(pp);
    }

    cHostServices(QSqlQuery& q, cHostService *&_p) : tRecordList<cHostService>(_p)
    {
        _p = nullptr;
        while (true) {
            cHostService *p = new cHostService;
            if (!p->next(q)) {
                delete p;
                return;
            }
            this->append(p);
        }
    }
    void cat(cHostServices *pp) {
        cHostService *p;
        while (nullptr != (p = pp->pop_front(EX_IGNORE))) pp->append(p);
        delete pp;
    }
};

/// @class cTemplateMapMap
/// @brief cTemplateMap konténer: kulcs a template típus neve
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
        if (__cont.isEmpty()) {
            EXCEPTION(EDATA, -1, QObject::tr("Üres minta."));
        }
        (*this)[__type].set(__name, __cont);
    }
    /// Egy adott nevű template elhelyezése a konténerbe, és az adatbázisban.
    void save(const QString& __type, const QString& __name, const QString& __cont, const QString& __descr) {
        if (__cont.isEmpty()) {
            EXCEPTION(EDATA, -1, QObject::tr("Üres minta."));
        }
        (*this)[__type].save(qq(), __name, __cont, __descr);
    }
    /// Egy adott nevű template törlése a konténerből, és az adatbázisból.
    void del(const QString& __type, const QString& __name) {
        (*this)[__type].del(qq(), __name);
    }
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
    /// 'jobb' oldali link egy röptében felvett munkaállomásra.
    /// @arg __n A munkaállomás neve (Ha létezik akkor az azonos nevű rekordot módosítja (replace))
    /// @arg __mac A munkaállomás MAC címe
    /// @arg __note node_note értéke
    void workstation(QString * __n, cMac * __mac, QString * __d);
    /// 'jobb' oldali link egy röptében felvett passzív elemre.
    /// @arg __n A passzív elem neve (Ha létezik akkor az azonos nevű rekordot módosítja (replace))
    /// @arg __note node_note értéke
    void attached(QString * __n, QString * __note);
    /// Egy az objektumban rögzített link kiírása az adatbázisba
    /// @param __note A Linkhez tartozó megjegzés
    /// @param __srv A linkhez hozzárendelt szolgáltatás példány adatai, vagy NULL ha ilyen nincs.
    void replace(QString * __note, QStringList * __srv);
    /// Több link kírása az adatbázisba.
    /// @param __hn1 A jobb oldali hálózati elem neve.
    /// @param __pi1 A jobb oldali port indexe.
    /// @param __hn2 A bal oldali kálózati elem neve.
    /// @param __pi2 A bal oldali port indexe.
    /// @param __n A kiírandó lonkel száma, az egymást követő linkek esetén a port indexxek mindíg eggyel növekednek.
    void replace(QString *__hn1, qlonglong __pi1, QString *__hn2, qlonglong __pi2, qlonglong __n);
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

class  cArpServerDefs;
static cArpServerDefs *pArpServerDefs = nullptr;
/// Kód beinzertálása a macbuff-ba.
/// Makró jellegű parancsok a macbuff aktuális tartalma elé inzertálják a makrü kifejtés eredményét.
static void insertCode(const QString& __txt);
/// Forrás sor puffer
static QString  macbuff;
/// Az utolsó beolvasott forrássor
static QString lastLine;

/// Alapértelmezett place_id. NULL-ra inicializált.
static qlonglong        globalPlaceId = NULL_ID;
static qlonglong        globalSuperiorId = NULL_ID;
static bool             globalReplaceFlag;
static bool             isReplace = false;
static cTemplateMapMap  templates;
static qlonglong        actVlanId = -1;
static QString          actVlanName;
static QString          actVlanNote;
static int              netType = ENUM_INVALID;
static cPatch *         pPatch = nullptr;
static cImage *         pImage = nullptr;
static cPlace *         pPlace = nullptr;
static cUser *          pUser = nullptr;
static cGroup *         pGroup = nullptr;
static cLink      *     pLink = nullptr;
static cService   *     pService = nullptr;
static cHostService*    pHostService = nullptr;
static cHostService*    pHostService2 = nullptr;
static cServiceVarType* pServiceVarType = nullptr;
static cServiceVar*     pServiceVar = nullptr;
static cTableShape *    pTableShape = nullptr;
static qlonglong        alertServiceId = NULL_ID;
static QMap<QString, qlonglong>    ivars;
static tStringMap                  svars;
static QMap<QString, QVariantList> avars;
static QSqlQuery  *     pq2 = nullptr;
static bool             breakParse = false;
static tRecordList<cMenuItem>   menuItems;
static bool             bSkeep = false;
static QString          actEnumType;
static cEnumVal        *pActEnum = nullptr;
static cRecordAny      *pRecordAny;
/// A parser
static int yyparse();
/// A parser utolsó hibajelentése, NULL, ha nincs hiba
cError *pImportLastError = nullptr;
static void set_debug(bool f);


/// A parse() hívása a hiba elkapásával.
/// Hiba esetén a hiba objektumba menti a parser következő állpotváltozóit:
/// Feldolgozott forrásállomány neve, aktuális sor sorszáma, tartalma, és a sor puffer (macbuff) tartalma.
/// @param _st Az importParserStat változó új értéke IPS_RUN vagy IPS_THREAD.
/// @exception Ha az importParserStat értéke nem IPS_READY, vagy _st értéke IPS_READY, akkor kizárást dob EPROGFAIL hibakóddal.
int importParse(eImportParserStat _st)
{
    if (importParserStat != IPS_READY || _st == IPS_READY)
        EXCEPTION(EPROGFAIL, (int)importParserStat);
    importParserStat = _st;
    PDEB(VERBOSE) << VEDEB(importParserStat, ipsName) << endl;
    // static const QString tn = "YYParser";
    int i = -1;
    set_debug(false);        // DEBUG PARSER
    try {
        // sqlBegin(qq(), tn);
        i = yyparse();
    } CATCHS(pImportLastError)
    if (pImportLastError != nullptr) {
        // sqlRollback(qq(), tn);
        pImportLastError->mDataLine = importLineNo;
        pImportLastError->mDataName = importFileNm;
        pImportLastError->mDataMsg  = "lastLine : " + quotedString(lastLine) + "\n macbuff : " + quotedString(macbuff);
        PDEB(DERROR) << pImportLastError->msg() << endl;
    }
    else {
        // sqlCommit(qq(), tn);
    }
    importParserStat = IPS_READY;
    PDEB(VERBOSE) << VEDEB(importParserStat, ipsName) << endl;
    return i;
}

class cArpServerDef {
public:
    enum eType { UNKNOWN, SNMP, DHCPD_CONF_LOCAL, DHCPD_CONF_SSH,  PROC_LOCAL, PROC_SSH, DHCPD_LEASES_LOCAL, DHCPD_LEASES_SSH  };
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
/*  Deprecated
 *     cArpServerDef(const cArpServerDef& __o)
        : server(__o.server), file(__o.file), community(__o.community) { type = __o.type; firstTime = __o.firstTime; }
*/
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

typedef QList<qlonglong> intList;

class c_yyFile {
public:
    c_yyFile() : oldFileName() {
        oldStream = nullptr;
        newFile = nullptr;
        oldPos = -1;
    }
/* Deprecated
 *     c_yyFile(const c_yyFile& __o) : oldFileName(__o.oldFileName) {
        oldStream = __o.oldStream;
        newFile = __o.newFile;
        oldPos = __o.oldPos;
    }
*/
    static void inc(QString *__f);
    static void eoi();
    static int size() { return stack.size(); }
//  static void clear();
    static void dropp();
private:
    QTextStream*    oldStream;
    QFile *         newFile;
    QString         oldFileName;
    unsigned int    oldPos;
    static QStack<c_yyFile> stack;
};

QStack<c_yyFile> c_yyFile::stack;

void c_yyFile::inc(QString *__f)
{
    c_yyFile    o;
    o.newFile = new QFile(*__f);
    if (!importSrcOpen(*o.newFile)) yyerror(QObject::tr("include: file open error."));
    o.oldStream = pImportInputStream;
    pImportInputStream = new QTextStream(o.newFile);
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
    delete pImportInputStream;
    delete o.newFile;
    pImportInputStream = o.oldStream;
    importFileNm = o.oldFileName;
    importLineNo = o.oldPos;
}

void c_yyFile::dropp()
{
    if (stack.size() == 0) return;
    int     n = importLineNo;
    QString f = importFileNm;
    while (stack.size() > 0) {
        eoi();
    }
    importLineNo = n;
    importFileNm = f;
}

// static  qlonglong       id;

unsigned long yyflags = 0;

static cTableShapeField *pTableShapeField = nullptr;

void initImportParser()
{
    breakParse = false;
    bSkeep     = false;
    if (pArpServerDefs == nullptr)
        pArpServerDefs = new cArpServerDefs();
    // notifswitch tömb = SET, minden on-ba, visszaolvasás listaként
    pDelete(piq);
    piq = newQuery();

    yyflags = 0;
    globalPlaceId = NULL_ID;
    globalSuperiorId = NULL_ID;
    actVlanId = -1;
    netType = ENUM_INVALID;
    alertServiceId = NULL_ID;

    pDelete(pImportLastError);
    importLineNo = 0;
    if (c_yyFile::size() > 0) EXCEPTION(EPROGFAIL);
    isReplace = false;
    globalReplaceFlag = cSysParam::getBoolSysParam(*piq, "global_replace_flag", false);
}

void downImportParser()
{
    pDelete(pArpServerDefs);
    pDelete(piq);
    macbuff.clear();
    lastLine.clear();
    menuItems.clear();

    templates.clear();
    actVlanName.clear();
    actVlanNote.clear();;
    pDelete(pPatch);
    pDelete(pImage);
    pDelete(pPlace);
    pDelete(pUser);
    pDelete(pGroup);
    pDelete(pLink);
    pDelete(pService);
    pDelete(pHostService);
    pDelete(pHostService2);
    pDelete (pServiceVarType);
    pDelete (pServiceVar);
    pDelete(pTableShape);
    ivars.clear();
    svars.clear();
    avars.clear();
    pDelete(pq2);
    actEnumType.clear();
    pDelete(pActEnum);
    pDelete(pRecordAny);

    c_yyFile::dropp();
    breakParse = false;
    bSkeep     = false;
}

void breakImportParser()
{
    breakParse = true;
}

bool isBreakImportParser(bool __ex)
{
    bool r = breakParse;
    if (__ex) {
        breakParse = false;
        if (r) EXCEPTION(EBREAK);
    }
    return r;
}


QSqlQuery& qq2()
{
    if (pq2 == nullptr) pq2 = newQuery();
    return *pq2;
}


/* */

// QMap<QString, duble>        rvars;
static const QString       sPortIx = "PI";   // Port index
static const QString       sPortNm = "PN";   // Port név


/* ---------------------------------------------------------------------------------------------------- */

static inline qlonglong gPlace() { return globalPlaceId == NULL_ID ? UNKNOWN_PLACE_ID : globalPlaceId; }

static inline qlonglong& vint(const QString& n){
    if (!ivars.contains(n)) yyerror(QObject::tr("Integer variable %1 not found").arg(n));
    return ivars[n];
}

static inline QString& vstr(const QString& n){
    if (!svars.contains(n)) yyerror(QObject::tr("String variable %1 not found").arg(n));
    return svars[n];
}

static inline QVariantList& varray(const QString& n){
    if (!avars.contains(n)) yyerror(QObject::tr("String variable %1 not found").arg(n));
    return avars[n];
}

static inline const QString& nextNetType() {
    int r = netType;
    switch (r) {
    case NT_PRIMARY:    netType = NT_SECONDARY;      break;
    case NT_SECONDARY:  break;
    case NT_PRIVATE:
    case NT_PSEUDO:     netType = ENUM_INVALID;        break;
    default:            yyerror(QObject::tr("Compiler error."));
    }
    return subNetType(r);
}

static inline cNode& node() {
    if (pPatch == nullptr) EXCEPTION(EPROGFAIL);
    return *pPatch->reconvert<cNode>();
}

static inline cSnmpDevice& snmpdev() {
    if (pPatch == nullptr) EXCEPTION(EPROGFAIL);
    return *pPatch->reconvert<cSnmpDevice>();
}

/* ************************************************************************************************ */
void cArpServerDef::updateArpTable(QSqlQuery& q)
{
    DBGFN();
#ifdef SNMP_IS_EXISTS
    cArpTable at;
    switch (type) {
    case UNKNOWN:       break;
    case SNMP: {
            cSnmp snmp(server.toString(), community, SNMP_VERSION_2c);
            at.getBySnmp(snmp);
            break;
        }
    case DHCPD_CONF_LOCAL:  if (firstTime) at.getByLocalDhcpdConf(file);                          break;
    case DHCPD_CONF_SSH:    if (firstTime) at.getBySshDhcpdConf(server.toString(), file);         break;
    case DHCPD_LEASES_LOCAL:at.getByLocalDhcpdLeases(file);                                       break;
    case DHCPD_LEASES_SSH:  at.getBySshDhcpdLeases(server.toString());                            break;
    case PROC_LOCAL:        at.getByLocalProcFile(file);                                          break;
    case PROC_SSH:          at.getBySshProcFile(server.toString(), file);                         break;
    default: EXCEPTION(EPROGFAIL);
    }
    firstTime = false;
    cArp::replaces(q, at);
    return;
/*#else
    #warning "SNMP functions is not supported."*/
#endif
    (void)q;
    EXCEPTION(ENOTSUPP);
}

/* --------------------------------------------------------------------------------------------------- */
static void setLastPort(cNPort *p);
static void newAttachedNode(const QString& __n, const QString& __d)
{
    cNode   *pNode = new cNode;
    pNode->asmbAttached(__n, __d, gPlace());
    pNode->containerValid = CV_ALL_HOST;
    if (pPatch != nullptr) EXCEPTION(EPROGFAIL);
    pPatch = pNode;
    setLastPort(pNode->ports.first());
}

static qlonglong replaceAttachedNode(const QString& __n, const QString& __d)
{
    cNode   node;
    node.asmbAttached(__n, __d, gPlace());
    node.containerValid = CV_ALL_HOST;
    node.replace(qq(), EX_ERROR);
    return node.ports.first()->getId();
}

static void newWorkstation(const QString& __n, const tStringPair * __addr, const QString * __mac, const QString& __d)
{
    cNode *pHost = new cNode;
    pHost->asmbWorkstation(qq(), __n, __addr, __mac, __d, gPlace());
    pHost->containerValid = CV_ALL_HOST;
    if (pPatch != nullptr) EXCEPTION(EPROGFAIL);
    pPatch = pHost;
    setLastPort(pHost->ports.first());
}

static qlonglong replaceWorkstation(const QString& __n, const cMac& __mac, const QString& __d)
{
    cNode host;
    tStringPair *pAddr = new tStringPair(QString(), _sDynamic);
    QString *pMac = new QString(__mac.toString());
    host.asmbWorkstation(qq(), __n, pAddr, pMac, __d, gPlace());
    delete pAddr;
    host.containerValid = CV_ALL_HOST;
    host.replace(qq(), EX_ERROR);
    return host.ports.first()->getId();
}

/* --------------------------------------------------------------------------------------------------- */

cLink::cLink(int __t1, QString *__n1, int __t2, QString *__n2)
    : phsLinkType1(phsLinkType(__t1))
    , phsLinkType2(phsLinkType(__t2))
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
    qlonglong   nid = NULL_ID;    // Node id
    if (__n->size() > 0) {  // Ha megadtunk node-ot
        nid = cPatch::getNodeIdByName(qq(), *__n);
    }
    else {
        nid = nodeId(__e);  // A blokk fejlécében megadott node, ha van
        if (nid == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
    }
    portId(__e) = cNPort::getPortIdByName(qq(), *__p, nid, EX_IGNORE);
    if (portId(__e) == NULL_ID) yyerror(QObject::tr("Invalid left port specification, #%1:%2").arg(nid). arg(*__p));
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::tr("Multiple share defined"));
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
            portId(__e) = cNPort::getPortIdByIndex(qq(), __p, nid, EX_IGNORE);
        }
    }
    else {
        nid = nodeId(__e);
        if (nid == NULL_ID) EXCEPTION(EDATA, -1, QObject::tr("Nincs megadva node!"));
        portId(__e) = cNPort::getPortIdByIndex(qq(), __p, nid, EX_IGNORE);
    }
    if (portId(__e) == NULL_ID) {
        yyerror(QObject::tr("Invalid port index, #%1:#%2").arg(nid).arg(__p));
    }
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::tr("Multiple share defined"));
        share = chkShare(__s);;
    }
    delete __n;
    _DBGFNL() << QChar(' ') << portId(__e) << endl;
}

void cLink::workstation(QString * __n, cMac * __mac, QString * __d)
{
    portId2 = replaceWorkstation(*__n, *__mac, *__d);
    delete __n;
    delete __mac;
    delete __d;
    newNode = true;
    _DBGFNL() << QChar(' ') << portId2 << endl;
}

void cLink::attached(QString * __n, QString * __note)
{
    portId2 = replaceAttachedNode(*__n, *__note);;
    delete __n;
    delete __note;
    newNode = true;
    _DBGFNL() << QChar(' ') << portId2 << endl;
}

void cLink::replace(QString * __note, QStringList * __srv)
{
    cPhsLink    lnk;
    lnk[_sPortId1]      = portId1;
    lnk[_sPhsLinkType1] = phsLinkType1;
    lnk[_sPortId2]      = portId2;
    lnk[_sPhsLinkType2] = phsLinkType2;
    lnk[_sPortShared]   = share;
    lnk[_sPhsLinkNote]  = *__note;
    if (__srv != nullptr) {    // Volt Alert
        cHostService    hose;
        cService        se;
        cNode           ho;
        cNPort          po;
        // Szervíz: megadták, vagy default
        if (__srv->size() > 0 && __srv->at(0).size() > 0) { if (!se.fetchByName(__srv->at(0))) yyerror(QObject::tr("Invalis alert service name.")); }
        else                                              { if (!se.fetchById(alertServiceId)) yyerror(QObject::tr("Nothing default alert service.")); }
        po.setById(qq(), portId2);   // A jobb oldali port a mienk
        ho.setById(qq(), po.getId(_sNodeId));
        // Volt megjegyzés ?
        if (__srv->size() > 1 && __srv->at(1).size() > 0) hose.setName(_sHostServiceNote, __srv->at(1));
        hose.setId(_sServiceId,  se.getId());
        hose.setId(_sNodeId,     ho.getId());
        hose.setId(_sPortId,     po.getId());
        hose[_sDelegateHostState] = true;
        QString it = se.feature(_sIfType);
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
                if (!newNode) yyerror(QObject::tr("Invalid port type"));
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
        lnk.replace(qq());
        hose.insert(qq());
        delete __srv;
    }
    else {
        lnk.replace(qq());
    }
    share.clear();
    portId1 = portId2 = NULL_ID;
    delete __note;
    newNode = false;
}


const QString&  cLink::chkShare(int __s)
{
    static QStringList  shares;
    if (shares.size() == 0) {
        shares << _sNul;
        shares << QString(_sA) << QString(_sAA) << QString(_sAB);
        shares << QString(_sB) << QString(_sBA) << QString(_sBB);
        shares << QString(_sC) << QString(_sD);
    }
    if (__s >= shares.size() || __s < 0 ) yyerror(QObject::tr("Invalid share value."));
    return shares[__s];
}

void cLink::replace(QString *__hn1, qlonglong __pi1, QString *__hn2, qlonglong __pi2, qlonglong __n)
{
    cPhsLink    lnk;
    lnk[_sPhsLinkType1] = phsLinkType1;
    lnk[_sPhsLinkType2] = phsLinkType2;
    lnk[_sPortShared]   = _sNul;
    qlonglong nid1 = __hn1->size() > 0 ? cPatch::getNodeIdByName(qq(), *__hn1) : nodeId1;
    qlonglong nid2 = __hn2->size() > 0 ? cPatch::getNodeIdByName(qq(), *__hn2) : nodeId2;
    while (__n--) {
        lnk[_sPortId1] = cNPort::getPortIdByIndex(qq(), __pi1, nid1, EX_ERROR);
        lnk[_sPortId2] = cNPort::getPortIdByIndex(qq(), __pi2, nid2, EX_ERROR);
        lnk.replace(qq());
        lnk.clear(lnk.idIndex());   // töröljük az id-t mert a köv insert-nél baj lessz.
        __pi1++;                    // Léptetjük az indexeket
        __pi2++;
    }
    delete __hn1;
    delete __hn2;
}

/* --------------------------------------------------------------------------------------------------- */

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
static cNPort *hostAddPort(int ix, QString *pt, QString *pn, tStringPair *ip, QVariant *mac, QString *d)
{
    cNPort& p = node().asmbHostPort(qq(), ix, *pt, *pn, ip, mac, *d);
    pDelete(pt);
    pDelete(pn);
    pDelete(ip);
    pDelete(mac);
    pDelete(d);
    return &p;
}

static cNPort *portAddAddress(cNPort *_p, tStringPair *ip, QString *d)
{
    cInterface *p = _p->reconvert<cInterface>();
    p->addIpAddress(QHostAddress(ip->first), ip->second, *d);
    delete ip; delete d;
    return _p;
}

static cNPort *portAddAddress(QString *pn, tStringPair *ip, QString *d)
{
    cNPort *p = node().getPort(*pn);
    delete pn;
    return portAddAddress(p, ip, d);

}

static cNPort *portAddAddress(int ix, tStringPair *ip, QString *d)
{
    cNPort *p = node().getPort(ix);
    return portAddAddress(p, ip, d);
}

static void changeIfType(const QString& ifn)
{
    if (node().ports.size() != 1) {
        yyerror(QObject::tr("Port change ifType : Invalid context."));
    }
    cNPort *pp = node().ports.first();
    const cIfType& ift = cIfType::ifType(ifn);
    if (ift.getName(_sIfTypeObjType) != pp->ifType().getName(_sIfTypeObjType)) {
        yyerror(QObject::tr("Port change ifType : The type of object cannot change."));
    }
    pp->setId(_sIfTypeId, ift.getId());
}

/* --------------------------------------------------------------------------------------------------- */

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


static QString *pMenuApp = nullptr;
static cMenuItem& actMenuItem()
{
    if (menuItems.isEmpty()) EXCEPTION(EPROGFAIL);
    return *menuItems.last();
}

/// Menü objektum létrehozása. Almenüi lesznek
static void newMenuMenu(const QString& _n, const QString *_pd)
{
    if (pMenuApp == nullptr) EXCEPTION(EPROGFAIL);
    cMenuItem *p = new cMenuItem;
    p->setName(_n);
    if (_pd != nullptr) {
        p->setNote(*_pd);
        delete _pd;
    }
    p->setName(_sAppName, *pMenuApp);
    if (!menuItems.isEmpty()) {
        p->setId(_sUpperMenuItemId, actMenuItem().getId());
    }
    p->setId(cMenuItem::ixMenuItemType(), MT_MENU);
    // Improve, replace!
    p->setText(cMenuItem::LTX_MENU_TITLE, _n);
    p->setName(cMenuItem::LTX_TAB_TITLE, _n);
    menuItems << p;
}

static void saveMenuItem()
{
    cMenuItem *p = menuItems.last();
    switch (p->getId(cMenuItem::ixMenuItemType())) {
    case MT_MENU:
        if (!p->isNull(cMenuItem::ixMenuParam())) {
            EXCEPTION(EPROGFAIL);
        }
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    p->insert(qq());
    p->saveText(qq());
}

static void delMenuItem()
{
    cMenuItem *p = menuItems.pop_back();
    switch (p->getId(cMenuItem::ixMenuItemType())) {
    case MT_MENU:
        delete p;
        return;
    case MT_SHAPE:
        if (NULL_ID == cTableShape().getIdByName(qq(), p->getParam(), EX_IGNORE)) {
            yyerror(QObject::tr("SHAPE '%1' not found.").arg(p->getParam()));
        }
    default:
        break;
    }
    p->insert(qq());
    p->saveText(qq());
    delete p;
}

/// Egy menu_items objektum létrehozása
/// @param _n Rekord név
/// @param _d Megjegyzés
/// @param _t típus
static void newMenuItem(const QString& _n, const QString *_pd, int _t)
{
    if (pMenuApp == nullptr) EXCEPTION(EPROGFAIL);
    cMenuItem *p = new cMenuItem;
    p->setName(_n);
    if (_pd != nullptr) {
        p->setNote(*_pd);
        delete _pd;
    }
    p->set(_sAppName, *pMenuApp);
    p->setId(_sUpperMenuItemId, actMenuItem().getId());
    switch (_t) {
    case MT_SHAPE:
    case MT_OWN:
    case MT_EXEC:
        p->setId(cMenuItem::ixMenuItemType(), _t);
        break;
    case MT_MENU:
    default:
        EXCEPTION(EPROGFAIL, _t); break;
    }
    // Improve, replace!
    p->setName(cMenuItem::ixMenuParam(), _n);
    p->setText(cMenuItem::LTX_MENU_TITLE, _n);
    p->setName(cMenuItem::LTX_TAB_TITLE, _n);
    menuItems << p;
}

/* --------------------------------------------------------------------------------------------------- */
#if       SNMP_IS_EXISTS
static cOId * newOId(QString * ps)
{
    cOId *r = ps == nullptr ? new cOId() : new cOId(*ps);
    delete ps;
    return r;
}
static cOId * addOId(cOId * po, qlonglong i)
{
    *po << int(i);
    return po;
}
static void printOId(cOId * po)
{
    cExportQueue::push(po->toString());
    delete po;
}
static void snmpGet(cSnmpDevice * pn, cOId *po)
{
    cSnmp snmp;
    pn->open(qq(), snmp);
    int r = snmp.get(*po);
    QString msg;
    if (r) {
        msg = QObject::tr("Error #%1; %2").arg(r).arg(snmp.emsg);
    }
    else {
        snmp.first();
        msg = po->toString() + " = " + debVariantToString(snmp.value());
    }
    cExportQueue::push(msg);
    delete po;
    delete pn;
}
static void snmpNext(cSnmpDevice * pn, cOId *po)
{
    cSnmp snmp;
    pn->open(qq(), snmp);
    int r = snmp.getNext(*po);
    QString msg;
    if (r) {
        msg = QObject::tr("Error #%1; %2").arg(r).arg(snmp.emsg);
    }
    else {
        snmp.first();
        msg = po->toString() + " = " + debVariantToString(snmp.value());
    }
    cExportQueue::push(msg);
    delete po;
    delete pn;
}
static void snmpWalk(cSnmpDevice * pn, cOId *po)
{
    cSnmp snmp;
    pn->open(qq(), snmp);
    int r = snmp.getNext(*po);
    while (r == 0) {
        snmp.first();
        cOId o = snmp.name();
        if (!(*po < o)) break;  // END
        cExportQueue::push(o.toString() + " = " + debVariantToString(snmp.value()));
        r = snmp.getNext();
    }
    if (r) {
        cExportQueue::push(QObject::tr("Error #%1; %2").arg(r).arg(snmp.emsg));
    }
    delete po;
    delete pn;

}
#else  // SNMP_IS_EXISTS
static cOId * newOId(QString *)         { EXCEPTION(ENOTSUPP); }
static cOId * addOId(cOId *, qlonglong) { EXCEPTION(ENOTSUPP); }
static void printOId(cOId *)            { EXCEPTION(ENOTSUPP); }
static void snmpGet(cSnmpDevice *, cOId *) { EXCEPTION(ENOTSUPP); }
static void snmpNext(cSnmpDevice *, cOId *){ EXCEPTION(ENOTSUPP); }
static void snmpWalk(cSnmpDevice *, cOId *){ EXCEPTION(ENOTSUPP); }
#endif // SNMP_IS_EXISTS
/* --------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------- */

static void insertCode(const QString& __txt)
{
    _DBGFN() << quotedString(__txt) << " + " << quotedString(macbuff) << endl;
    macbuff = __txt + macbuff;
}

QString yygetline()
{
    isBreakImportParser(true);
    ++importLineNo;
    if (pImportInputStream != nullptr) return pImportInputStream->readLine();
    if (importParserStat != IPS_THREAD) EXCEPTION(EPROGFAIL);
    return cImportParseThread::instance().pop();
}

static QChar yyget()
{
    QChar c;

    if (!macbuff.size()) {
        do {
            lastLine = macbuff  = yygetline();
            PDEB(VVERBOSE) << "YYLine(" << importFileNm << "[" << importLineNo << "]) : \"" << lastLine << "\"" << endl;
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
static QString *strReplace(QString * s, QString * src, QString *trg);
static void forLoopMac(QString *_in, QVariantList *_lst, int step = 1);
static void forLoop(QString *_in, QVariantList *_lst, int step = 1);
static QStringList *listLoop(QString *m, qlonglong fr, qlonglong to, qlonglong st);
static intList *listLoop(qlonglong fr, qlonglong to, qlonglong st);
static QString getAllTokens(bool sort = true);


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


static QString * sTime(qlonglong h, qlonglong m, double s = 0)
{
    QString *p = new QString();
    if (h == 24 && m == 0 && int(s * 10000) == 0) {
        *p = "23:59:59.999";
    }
    else {
        if (h < 0 || h >= 24) yyerror(QObject::tr("Invalid hour value"));
        if (m < 0 || m >= 60) yyerror(QObject::tr("Invalid min value"));
        if (s < 0 || s >= 60) yyerror(QObject::tr("Invalid sec value"));
        static const QChar fillChar = QLatin1Char('0');
        *p = QString("%1:%2:%3")
                .arg(h, 2, 10, fillChar)
                .arg(m, 2, 10, fillChar)
                .arg(s, 6, 'f', 3, fillChar);
    }
    return p;
}

static enum ePortShare s2share(QString * __ps)
{
    QString s;
    if (__ps != nullptr) {
        s = *__ps;
        delete __ps;
    }
    if (s.isEmpty())return ES_;
    if (s == _sA)   return ES_A;
    if (s == _sAA)  return ES_AA;
    if (s == _sAB)  return ES_AB;
    if (s == _sB)   return ES_B;
    if (s == _sBA)  return ES_BA;
    if (s == _sBB)  return ES_BB;
    if (s == _sC)   return ES_C;
    if (s == _sD)   return ES_D;
    yyerror(QString(QObject::tr("Helytelen megosztás típus : %1")).arg(s));
    return ES_;     // Hogy ne pofázzon a fordító
}

static bool portChkShare(intList&  pl, QStringList *psl)
{
    QStringList sl = *psl;
    delete psl;
    int siz = pl.size();
    QString e = QObject::tr("Helytelenül magadott kábelmegosztás #");
    if (siz != sl.size())  yyerror(e + "1");
    switch (siz) {
    case 2:
        if      (sl[0] == _sA && sl[1] == _sB) { pl << -1 << -1;                          return false; }
        else if (sl[0] == _sB && sl[1] == _sA) { pl.swapItemsAt(0,1); pl << -1 << -1;     return false; }
        else    yyerror(e + "2");
        break;
    case 3:
        if      (sl[0] == _sA  && sl[1] == _sBA && sl[2] == _sBB) { pl.insert(1, -1);     return false; }
        else if (sl[0] == _sAA && sl[1] == _sAB && sl[2] == _sB ) { pl << -1;             return false; }
        else    yyerror(e + "3");
        break;
    case 4:
        if      (sl[0] == _sAA && sl[1] == _sAB && sl[2] == _sBA && sl[3] == _sBB)        return false;
        else if (sl[0] == _sAA && sl[1] == _sC  && sl[2] == _sD  && sl[3] == _sBB)        return true;
        else    yyerror(e + "4");
        break;
    default:
        yyerror(e + "5");
    }
    return false;   // WARNING
}

static void portSetShare(intList  *seql, QStringList *psl)
{
    intList pl = *seql;
    delete seql;
    bool f = portChkShare(pl, psl);
    pPatch->setShare(pl[0], pl[1], pl[2], pl[3], f);
}

static void portUpdateShare(intList  *seql, QStringList *psl)
{
    intList pl = *seql;
    delete seql;
    bool f = portChkShare(pl, psl);
    pPatch->updateShared(qq(), pl[0], pl[1], pl[2], pl[3], f);
}


static void portSetNC(intList *pi)
{
    foreach (qlonglong i, *pi) {
        pPatch->ports[i]->setName(_sSharedCable, _sNC);
    }
    delete pi;
}

static void portUpdateNc(intList  *seql, bool _f)
{
    intList pl = *seql;
    delete seql;
    foreach (qlonglong i, pl) {
        cPPort *p = pPatch->ports[i]->reconvert<cPPort>();
        p->setId(_sSharedCable, _f ? ES_NC : ES_);
        p->clear(_sSharedPortId);
        p->update(qq(), true, p->mask(_sSharedCable, _sSharedPortId));
    }
}

static QString *toddef(QString * name, QString *  day, QString * fr, QString *  to, QString *note)
{
    cTpow t;
    t.setName(*name);
    t[_sTpowNote] = *note;
    t[_sDow]       = *day;
    t[_sBeginTime] = *fr;
    t[_sEndTime]   = *to;
    delete day; delete fr; delete to; delete note;
    t.insert(qq());
    return name;
}

static void setLastPort(QString * np, qlonglong ix) {
    if (np != nullptr && np->size() > 0) svars[sPortNm] = *np;
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

inline static cNPort * setLastPort(qlonglong ix) {
    if (pPatch == nullptr) EXCEPTION(EPROGFAIL);
    cNPort * p = pPatch->ports.get(cNPort::ixPortIndex(), QVariant(ix));
    setLastPort(p);
    return p;
}

inline static cNPort * setLastPort(const QString& n) {
    if (pPatch == nullptr) EXCEPTION(EPROGFAIL);
    cNPort * p = pPatch->ports.get(_sPortName, QVariant(n));
    setLastPort(p);
    return p;
}

inline cNPort *lastPort() {
    QString n = svars[sPortNm];
    if (pPatch == nullptr) EXCEPTION(EPROGFAIL);
    cNPort * p = pPatch->ports.get(n);
    return p;
}

/// A port neve (port_name mező) értéke alapján a port
/// indexe a pPatch->ports konténerben
static int portName2SeqN(const QString& n)
{
    if (pPatch == nullptr) EXCEPTION(EPROGFAIL);
    int r = pPatch->ports.indexOf(_sPortName,  QVariant(n));
    if (r < 0) yyerror(QObject::tr("Nincs %1 nevű port.").arg(n));
    setLastPort(n, r);
    return r;
}

/// A port index (port_index mező) értéke alapján a port
/// indexe a pPatch->ports konténerben
static int portIndex2SeqN(qlonglong ix)
{
    if (pPatch == nullptr) EXCEPTION(EPROGFAIL);
    int r = pPatch->ports.indexOf(_sPortIndex,  QVariant(ix));
    if (r < 0) yyerror(QObject::tr("Nincs %1 indexű port.").arg(ix));
    setLastPort(pPatch->ports[r]);
    return r;
}

static void patchCopy(QStringList *pExl, QStringList *pInc, QString *pSrc)
{
    cPatch src;
    src.setByName(qq(), *pSrc);
    delete pSrc;
    QBitArray mask;
    bool ports = false, params = false;
    if (pExl == nullptr && pInc == nullptr) { // Üres listák, mindent másolunk;
        mask = ~pPatch->mask(_sNodeId, _sNodeName);
        ports = params = true;
    }
    else if (pInc == nullptr) {            // Kihagyandók listája van megadva
        mask = ~pPatch->mask(_sNodeId, _sNodeName);
        ports = params = true;
        foreach (QString fn, *pExl) {
            int i = pPatch->toIndex(fn, EX_IGNORE);
            if (i < 0) {
                if      (0 == fn.compare(_sPorts,  Qt::CaseInsensitive)) ports  = false;
                else if (0 == fn.compare(_sParams, Qt::CaseInsensitive)) params = false;
                else EXCEPTION(EPROGFAIL, 0, "Parser error");
            }
            else mask[i] = false;
        }
        delete pExl;
    }
    else if (pExl == nullptr) {            // A Másolandók listája van megadva
        mask.resize(pPatch->cols());
        ports = params = false;
        foreach (QString fn, *pExl) {
            int i = pPatch->toIndex(fn, EX_IGNORE);
            if (i < 0) {
                if      (0 == fn.compare(_sPorts,  Qt::CaseInsensitive)) ports  = true;
                else if (0 == fn.compare(_sParams, Qt::CaseInsensitive)) params = true;
                else EXCEPTION(EPROGFAIL, 0, "Parser error");
            }
            else mask[i] = true;
        }
        delete pInc;
    }
    else EXCEPTION(EPROGFAIL, 0, "Parser error");  // Mindkét lista nem adható meg
    pPatch->fieldsCopy(src, mask);
    if (ports) {
        src.fetchPorts(qq());
        pPatch->ports.append(src.ports);
        pPatch->ports.clearId();
        pPatch->ports.setsOwnerId();
    }
    if (params) {
        src.fetchParams(qq());
        pPatch->params.append(src.params);
        pPatch->params.clearId();
        pPatch->params.setsOwnerId();
    }
}

static void copyTableShape(QString *pSrc)
{
    cTableShape src;
    src.setByName(qq(), *pSrc);
    delete pSrc;
    pTableShape->fieldsCopy(src, ~pTableShape->mask(_sTableShapeId, _sTableShapeName));
    tTableShapeFields::iterator i, n = src.shapeFields.end();
    int ixId  = cTableShapeField().idIndex();
    int ixOwn = src.shapeFields.ixOwnerId;
    for (i = src.shapeFields.begin(); i != n; ++i) {
        cTableShapeField& srcField = **i;
        cTableShapeField *pField = srcField.dup()->reconvert<cTableShapeField>();
        pField->clear(ixId);
        pField->clear(ixOwn);
        pTableShape->shapeFields << pField;
    }
}

static tPolygonF *rectangle(QPointF *p1, QPointF *p2)
{
    tPolygonF *pol = new tPolygonF;
    *pol << *p1;
    *pol << QPointF(p1->x(), p2->y());
    *pol << *p2;
    *pol << QPointF(p2->x(), p1->y());
    delete p1;
    delete p2;
    return pol;
}

enum eReplace {
    REPLACE_DEF, REPLACE_ON, REPLACE_OFF, REPLACE_NULL
};

static void setReplace(int _rep)
{
    switch (_rep) {
    case REPLACE_DEF:   isReplace = globalReplaceFlag;  break;
    case REPLACE_OFF:   isReplace = false;              break;
    case REPLACE_ON:    isReplace = true;               break;
    case REPLACE_NULL:                                  break;
    default:    EXCEPTION(EPROGFAIL, _rep);
    }
}

#define NEWOBJ(p, t) \
    if (p != nullptr) EXCEPTION(EPROGFAIL, 0,"NEWOBJ(" _STR(p) ", " _STR(t) ") : pointer is not null"); \
    isReplace = false; \
    p = new t;
//  if (p->stat == ES_DEFECTIVE) yyerror("Nincs elegendő adat, vagy kétértelműség.")

#define INSERTANDDEL(p) \
    if (p == nullptr) EXCEPTION(EPROGFAIL, 0, "Parser Error: " _STR(p) " pointer is null"); \
    if (p == nullptr) EXCEPTION(EPROGFAIL, 0, "Parser Error: isReplace is true, in INSERTANDDEL(p) macro."); \
    p->insert(qq()); \
    pDelete(p); \
    setLastPort(nullptr, NULL_IX)  // Törli

#define DELOBJ(p) \
    if (p == nullptr) EXCEPTION(EPROGFAIL, 0, "Parser error : " _STR(p) " pointer is null"); \
    pDelete(p)

#define INSREC(T, dn, n, d) \
    T  o; o.setName(*n)[dn] = *d; \
    o.insert(qq()); delete n; delete d;

#define SETNOTE(pO, pN)  if(!pN->isEmpty()) pO->setNote(*pN); \
                         delete pN;

/// @param p    Az objektum pointer változó
/// @param t    At objektum típusa
/// @param r    Ha hamis, akkor új objektumot hozunk létre, ha igaz, akkor ha létezik, modosítjuk.
/// @param pnm  Az objektum nevére mutató pointer (a makró felszabadítja)
/// @param pno  Opcionális, megjegyzés mező értéke, vagy NULL
#define REPOBJ(p, t, r, pnm, pno) \
    setReplace(r); \
    if (p != nullptr) EXCEPTION(EPROGFAIL, 0,"Parser error: REPOBJ(" _STR(p) ", " _STR(t) ", " + *pnm + ") : pointer is not null"); \
    p = new t; \
    p->setName(*pnm); \
    if (pno != nullptr) { \
        SETNOTE(p, pno) \
    } \
    delete pnm;

/// Egy definiált/modosított objektum kiírása, és az objektum törlése
template <class R> void writeAndDel(R *& p)
{
    if (p == nullptr) yyerror(QObject::tr("Null pointer in %1.").arg(__PRETTY_FUNCTION__));
    QString emsg;
    if (isReplace) {
        emsg = QObject::tr("Replace object : ");
        p->replace(qq());
    }
    else {
        emsg = QObject::tr("Insert object : ");
        p->insert(qq());
    }
    emsg += p->identifying();
    cExportQueue::push(emsg);
    pDelete(p);
    setLastPort(nullptr, NULL_IX);  // Törli
}

/// Egy új node/host vagy snmp eszköz létrehozása (a paraméterként megadott pointereket felszabadítja)
/// @param name Az eszköz nevére mutató pointer, vagy a "LOOKUP" stringre mutató pointer.
/// @param ip Pointer egy string pár, az első elem az IP cím, vagy az ARP ill. LOOKUP string, ha az ip címet a MAC címből ill.
///           a névből kell meghatározni. A második elem az ip cím típus neve. Ha NULL nincs IP hivatkozás.
/// @param mac Vagy a MAC stringgé konvertálva, vagy az ARP string, ha az IP címből kell meghatározni, vagy NULL, ha nincs MAC
/// @param d Megjegyzés  mutató pointer, üres string esetén az NULL lessz.
/// @return Az új objektum pointere
static void newHost(qlonglong t, QString *name, tStringPair *ip, QString *mac, QString *d)
{
    cNode *pNode;
    if (t & ENUM2SET(NT_SNMP)) pNode = new cSnmpDevice();
    else                       pNode = new cNode();
    pPatch = pNode;
    if (ip == nullptr) {
        pNode->setName(*name);
        if (d != nullptr) pNode->setNote(*d);
        setLastPort(nullptr, NULL_IX);
    }
    else {
        pNode->asmbNode(qq(), *name, nullptr, ip, mac, *d, gPlace());
        setLastPort(pNode->ports.first());
    }
    pNode->setId(_sNodeType, t);
    pDelete(name); pDelete(ip); pDelete(mac); pDelete(d);
}

static void yySqlExec(const QString& _cmd, QVariantList *pvl = nullptr, QVariantList * _ret = nullptr)
{
    QSqlQuery q = qq();
    PDEB(INFO) << "SQL : " << _cmd << endl;
    if (pvl == nullptr) {
        if (!q.exec(_cmd)) SQLPREPERR(q, _cmd);
    }
    else {
        if (!q.prepare(_cmd)) SQLPREPERR(q, _cmd);
        int i = 0;
        foreach (QVariant v, *pvl) {
            q.bindValue(i, v);
            ++i;
        }
        delete pvl;
        if (!q.exec()) SQLQUERYERR(q);
    }
    if (_ret != nullptr) {
        _ret->clear();
        if (q.first()) do {
            int n = q.record().count();
            for (int i = 0; i < n; ++i) {
                *_ret << q.value(i);
            }
        } while (q.next());
    }
}

static void setNodePlace(QStringList nodes, qlonglong place_id)
{
    static const QString sql =
            "UPDATE patchs SET place_id = ? WHERE node_name LIKE ?";
    foreach (QString node, nodes) {
        execSql(qq(), sql, place_id, node);
    }
}

static void setNodePortsParam(const QString& __node, const QStringList& _ports, const QString& __ptype, const QString& __name, const QVariant& __val)
{
    const cParamType& pt = cParamType::paramType(__ptype);
    // node
    cPatch n;   // Node közös ős
    n.setByName(qq(), __node);
    n.ports.fetch(qq());        // a fetchPorts() nem jó, csak a patch portokat olvasná
    cPortParam pp;
    pp._toReadBack = RB_NO;
    pp.setName(__name);
    pp.setId(_sParamTypeId, pt.getId());
    pp.set(_sParamValue, __val);
    foreach (QString pn, _ports) {
        cNPort *p= n.ports.get(pn);
        pp.setId(_sPortId, p->getId());
        pp.replace(qq());
    }
}

static void delNodePortsParam(const QString& __node, const QStringList& _ports, const QString& __name)
{
    cPatch n;   // Node közös ős
    n.setByName(qq(), __node);
    n.ports.fetch(qq());        // a fetchPorts() nem jó, cak a patch portokat olvasná
    cPortParam pp;
    QBitArray  where = pp.mask(_sPortParamName, _sPortId);
    pp.setName(__name);
    foreach (QString pn, _ports) {
        pp.setId(_sPortId,      n.ports.get(pn)->getId());
        pp.remove(qq(), false, where, EX_ERROR);
    }
}

static void setNodeParam(const QStringList& __nodes, const QString& __ptype, const QString& __name, const QVariant& __val)
{
    const cParamType& pt = cParamType::paramType(__ptype);
    cNodeParam np;
    np._toReadBack = RB_NO;
    np.setName(__name);
    np.setId(_sParamTypeId, pt.getId());
    np.set(_sParamValue, __val);
    foreach (QString nn, __nodes) {
        qlonglong nid = cPatch().getIdByName(qq(), nn);
        np.setId(_sNodeId, nid);
        np.replace(qq());
    }
}

static void delNodesParam(const QStringList& __nodes, const QString& __name)
{
    cNodeParam np;
    np.setName(__name);
    QBitArray  where = np.mask(_sNodeParamName, _sNodeId);
    foreach (QString nn, __nodes) {
        qlonglong nid = cPatch().getIdByName(qq(), nn);
        np.setId(_sNodeId,      nid);
        np.remove(qq(), false, where, EX_ERROR);
    }
}

static void newEnum(const QString& _type, QString * _pval = nullptr, int _rep = REPLACE_NULL)
{
    if (pActEnum != nullptr) EXCEPTION(EPROGFAIL);
    setReplace(_rep);
    actEnumType = _type;
    const QChar cSep = QChar('.');
    if (actEnumType.contains(cSep)) { // boolean ?
        QStringList sl = actEnumType.split(cSep);
        if (sl.size() == 2) {
            QString sTableName = sl.first();
            QString sFieldName = sl.at(1);
            cRecordAny rec(sTableName);
            int t = rec.colDescr(sFieldName).eColType;
            if (t == FT_BOOLEAN) {
                if (_pval == nullptr) {
                    pActEnum = new cEnumVal(actEnumType);
                    return;
                }
                else {
                    if (0 != _sTrue.compare(*_pval, Qt::CaseInsensitive) && 0 != _sFalse.compare(*_pval, Qt::CaseInsensitive)) {
                        yyerror(QObject::tr("Invalid enum value : %1.%2").arg(actEnumType, *_pval));
                        delete _pval;
                        return;
                    }
                    pActEnum = new cEnumVal(actEnumType, _pval->toLower());
                    delete _pval;
                    return;
                }
            }
        }
        pDelete(_pval);
        yyerror(QObject::tr("Invalid boolean field name : %1").arg(actEnumType));
        return;
    }
    else {
        const cColEnumType& cType = *cColEnumType::fetchOrGet(qq(), actEnumType);
        if (_pval == nullptr) {
            pActEnum = new cEnumVal(actEnumType);
            return;
        }
        else {
            if (!cType.containsValue(*_pval)) {
                yyerror(QObject::tr("Invalid enum value : %1.%2").arg(actEnumType, *_pval));
                delete _pval;
                return;
            }
            pActEnum = new cEnumVal(actEnumType, *_pval);
            delete _pval;
            return;
        }
    }
}

static void saveEnum()
{
    if (pActEnum == nullptr) EXCEPTION(EPROGFAIL);
    if (isReplace) {
        pActEnum->replace(qq());
    }
    else {
        pActEnum->insert(qq());
    }
    pActEnum->saveText(qq());
    pDelete(pActEnum);
}

static inline cEnumVal& actEnum()
{
    if (pActEnum == nullptr) EXCEPTION(EPROGFAIL);
    return *pActEnum;
}

/* *************************************************** DEBUG *************************************************** */
// miért nincs debug !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define YYDEBUG 1
#define YYFPRINTF   yydebprintf
#define PFOUTMAX 512
static int yydebprintf(FILE *, const char * format, ... )
{
    char    cs[PFOUTMAX +1];
    va_list args;
    va_start(args, format);
    vsnprintf(cs, PFOUTMAX, format, args);
    va_end(args);
    QString s(cs);
    int r = s.size();
    if (s.endsWith(QChar('\n'))) {
        s.chop(1);
        _PDEB(VERBOSE) << "\t[YY]\t" << s << "\\" << endl;
    }
    else {
        _PDEB(VERBOSE) << "\t[YY]\t" << s << endl;
    }
    return r;
}
static QString intList2String(const intList& il) {
    QString r = "{";
    foreach (int i, il) { r += QString::number(i) + _sCommaSp; }
    r.chop(2);
    return r + "}";
}
inline QString tStringPair2String(const tStringPair& ss) { return QString("(%1, %2)").arg(quotedString(ss.first), quotedString(ss.second)); }

/* ************************************************* DEBUG END ************************************************* */
%}

%union {
    qlonglong           i;
    intList *           il;
    QVariantList *      ids;
    bool                b;
    QString *           s;
    QStringList *       sl;
    cMac *              mac;
    double              r;
    QHostAddress *      ip;
    netAddress *        n;
    QVariant *          v;
    QVariantList *      vl;
    QPointF *           pnt;
    tPolygonF *         pol;
    tStringPair *       ss;
    cSnmpDevice *       sh;
    cHostServices *     hss;
    tStringMap *        sm;
    cOId *              coid;
}

%token      TEST_T
%token      MACRO_T FOR_T DO_T TO_T SET_T CLEAR_T OR_T AND_T DEFINED_T
%token      VLAN_T SUBNET_T PORTS_T PORT_T NAME_T SHARED_T SENSORS_T
%token      PLACE_T PATCH_T SWITCH_T NODE_T HOST_T ADDRESS_T ICON_T
%token      PARENT_T IMAGE_T FRAME_T TEL_T NOTE_T MESSAGE_T LEASES_T
%token      PARAM_T TEMPLATE_T COPY_T FROM_T NULL_T TERM_T NEXT_T
%token      INCLUDE_T PSEUDO_T OFFS_T IFTYPE_T WRITE_T RE_T SYS_T
%token      ADD_T READ_T UPDATE_T ARPS_T ARP_T SERVER_T FILE_T BY_T
%token      SNMP_T SSH_T COMMUNITY_T DHCPD_T LOCAL_T PROC_T CONFIG_T
%token      ATTACHED_T LOOKUP_T WORKSTATION_T LINKS_T BACK_T FRONT_T
%token      IP_T COMMAND_T SERVICE_T PRIME_T PRINT_T EXPORT_T
%token      MAX_T CHECK_T ATTEMPTS_T NORMAL_T INTERVAL_T RETRY_T 
%token      FLAPPING_T CHANGE_T TRUE_T FALSE_T ON_T OFF_T YES_T NO_T
%token      DELEGATE_T STATE_T SUPERIOR_T TIME_T PERIODS_T LINE_T GROUP_T
%token      USER_T DAY_T OF_T PERIOD_T PROTOCOL_T ALERT_T INTEGER_T FLOAT_T
%token      DELETE_T ONLY_T STRING_T SAVE_T TYPE_T STEP_T EXCEPT_T
%token      MASK_T LIST_T VLANS_T ID_T DYNAMIC_T PRIVATE_T PING_T
%token      NOTIF_T ALL_T RIGHTS_T REMOVE_T SUB_T FEATURES_T MAC_T
%token      LINK_T LLDP_T SCAN_T TABLE_T FIELD_T SHAPE_T TITLE_T REFINE_T
%token      DEFAULTS_T ENUM_T RIGHT_T VIEW_T INSERT_T EDIT_T LANG_T
%token      INHERIT_T NAMES_T VALUE_T DEFAULT_T STYLE_T SHEET_T
%token      ORD_T SEQUENCE_T MENU_T GUI_T OWN_T TOOL_T TIP_T WHATS_T THIS_T
%token      EXEC_T TAG_T ENABLE_T SERIAL_T INVENTORY_T NUMBER_T
%token      DISABLE_T PREFIX_T RESET_T CACHE_T INVERSE_T RAREFACTION_T
%token      DATA_T IANA_T IFDEF_T IFNDEF_T NC_T QUERY_T PARSER_T IF_T
%token      REPLACE_T RANGE_T EXCLUDE_T PREP_T POST_T CASE_T RECTANGLE_T
%token      DELETED_T PARAMS_T DOMAIN_T VAR_T PLAUSIBILITY_T CRITICAL_T
%token      AUTO_T FLAG_T TREE_T NOTIFY_T WARNING_T PREFERED_T RAW_T  RRD_T
%token      REFRESH_T SQL_T CATEGORY_T ZONE_T HEARTBEAT_T GROUPS_T AS_T
%token      END_T ELSE_T TOKEN_T COLOR_T BACKGROUND_T FOREGROUND_T FONT_T ATTR_T FAMILY_T
%token      OS_T VERSION_T INDEX_T GET_T WALK_T OID_T GLPI_T SYNC_T LIKE_T
%token      OBJECTS_T PLATFORMS_T WAIT_T LOWER_T UPPER_T

%token <i>  INTEGER_V
%token <r>  FLOAT_V
%token <s>  STRING_V NAME_V
%token <mac> MAC_V 
%token <ip> IPV4_V IPV6_V
%type  <i>  int int_ iexpr lnktype shar offs ix_z vlan_t set_t srvtid lnx
%type  <i>  vlan_id place_id iptype pix pix_z image_id image_idz tmod int0 replace
%type  <i>  fhs hsid srvid grpid tmpid node_id port_id snet_id ift_id plg_id
%type  <i>  usr_id ftmod p_seq lnktypez fflags fflag tstypes tstype pgtype
%type  <i>  node_h node_ts srvvt_id reattr nodetype
%type  <il> list_i p_seqs p_seqsl // ints
%type  <b>  bool_ bool_on bool_off bool ifdef exclude replfl prefered inverse
%type  <r>  /* real */ num fexpr
%type  <s>  str str_ str_z str_zz name_q time tod _toddef sexpr pnm mac_q ha nsw ips rights
%type  <s>  imgty tsintyp usrfn usrgfn plfn ptcfn copy_from features osver
%type  <sl> strs strsz strs_z strs_zz alert list_m nsws nsws_
%type  <sl> usrfns usrgfns plfns ptcfns
%type  <v>  value mac_qq
%type  <vl> vals vtfilt
%type  <n>  subnet
%type  <ip> ip
%type  <pnt> point
%type  <pol> frame points rectangle
%type  <ids> vlan_ids grpids
%type  <ss>  ip_qq ip_q ip_a as feature map
%type  <mac> mac
%type  <sh>  snmph
%type  <hss> hss hsss
%type  <sm>  feature_s maps
%type  <coid> coid

/* Generate the parser description file.  */
%verbose
/* Enable run-time traces (yydebug).  */
%define parse.trace
%debug

/* Formatting semantic values.  */
%printer { PDEB(VERBOSE) << $$                           << "::qlonglong";       } <i>;
%printer { PDEB(VERBOSE) << intList2String(*$$)          << "::intList";         } <il>;
%printer { PDEB(VERBOSE) << debVariantToString(*$$);                             } <ids>;
%printer { PDEB(VERBOSE) << DBOOL($$)                    << "::bool";            } <b>;
%printer { PDEB(VERBOSE) << *$$                          << "::QString";         } <s>;
%printer { PDEB(VERBOSE) << debVariantToString(*$$);                             } <sl>;
%printer { PDEB(VERBOSE) << $$->toString()               << "::cMac";            } <mac>;
%printer { PDEB(VERBOSE) << $$                           << "::double";          } <r>;
%printer { PDEB(VERBOSE) << $$->toString()               << "::QHostAddress";    } <ip>;
%printer { PDEB(VERBOSE) << $$->toString()               << "::netAddress";      } <n>;
%printer { PDEB(VERBOSE) << debVariantToString(*$$)      << "::QVariant";        } <v>;
%printer { PDEB(VERBOSE) << debVariantToString(*$$)      << "::QVariantList";    } <vl>;
%printer { PDEB(VERBOSE) << QPointFTosString(*$$)        << "::QPointF";         } <pnt>;
%printer { PDEB(VERBOSE) << tPolygonFToString(*$$)       << "::tPolygonF";       } <pol>;
%printer { PDEB(VERBOSE) << tStringPair2String(*$$)      << "::tStringPair";     } <ss>;
%printer { PDEB(VERBOSE) << $$->identifying(false)       << "::cSnmpHost";       } <sh>;
%printer { PDEB(VERBOSE) << $$->size()             << "::cHostServices::size()"; } <hss>;
%printer { PDEB(VERBOSE) << "{" << cFeatures::join(*$$) << "}::tStringMap";      } <sm>;
%printer { PDEB(VERBOSE) << $$->toString()               << "::cOId";            } <coid>;

// A destruktorokat rendesen meg kellene csinálni !!!! A többit is!
%destructor { pDelete($$); } <sm>

%left  NOT_T
%left  OR_T
%left  AND_T
%left  '!'
%left  '^'
%left  '+' '-' '|'
%left  '*' '/' '&'
%left  NEG

%%

main    :
        | commands
        ;
commands: command
        | commands command
        ;
command : glpi
        | INCLUDE_T str ';'                               { c_yyFile::inc($2); }
        | macro
        | sql
        | user
        | timeper
        | vlan
        | image
        | place
        | iftype
        | nodes
        | params
        | arp
        | link
        | srv
        | tools
        | delete
        | eqs
        | scan
        | gui
        | modify
        | if
        | replaces
        | exports
        | snmp
        | ';'
        ;
glpi    : GLPI_T PLACE_T SYNC_T bool_ bool_ bool_off ';' {
                    cGlpiLocationsTreeItem *p = cGlpiLocationsTreeItem::syncing(bool2ts($4), bool2ts($5), $6);
                    if (p != nullptr) delete p;
                }
        ;
// Makró vagy makró jellegű definíciók
macro   : MACRO_T            NAME_V str ';'                 { templates.set (_sMacros, sp2s($2), sp2s($3));           }
        | MACRO_T            NAME_V str SAVE_T str_z ';'    { templates.save(_sMacros, sp2s($2), sp2s($3), sp2s($5)); }
        | TEMPLATE_T PATCH_T NAME_V str ';'                 { templates.set (_sPatchs, sp2s($3), sp2s($4));           }
        | TEMPLATE_T PATCH_T NAME_V str SAVE_T str_z ';'    { templates.save(_sPatchs, sp2s($3), sp2s($4), sp2s($6)); }
        | TEMPLATE_T NODE_T  NAME_V str ';'                 { templates.set (_sNodes,  sp2s($3), sp2s($4));           }
        | TEMPLATE_T NODE_T  NAME_V str SAVE_T str_z ';'    { templates.save(_sNodes,  sp2s($3), sp2s($4), sp2s($6)); }
        | for_m
        | EXEC_T str ';'                                    { insertCode(sp2s($2)); }
        ;
// Ciklusok
for_m   : FOR_T vals DO_T MACRO_T NAME_V ';'                { forLoopMac($5, $2); }
        | FOR_T vals DO_T str ';'                           { forLoop($4, $2); }
        | STEP_T int FOR_T vals DO_T MACRO_T NAME_V ';'     { forLoopMac($7, $4, $2); }
        | STEP_T int FOR_T vals DO_T str ';'                { forLoop($6, $4, $2); }
        ;
// Listák
list_m  : LIST_T iexpr TO_T iexpr MASK_T str                { $$ = listLoop($6, $2, $4, 1);  }
        | LIST_T iexpr TO_T iexpr STEP_T iexpr MASK_T str   { $$ = listLoop($8, $2, $4, $6); }
        | LIST_T str TABLE_T str_z LIKE_T str   { $$ = new QStringList;
                                                  QString nn = cRecordAny(*$2).nameName();
                                                  QString sql = QString("SELECT %1 FROM %2 WHERE %3 LIKE ?")
                                                    .arg(nn, sp2s($2), $4->isEmpty() ? nn : *$4);
                                                  delete $4;
                                                  sql = sql.arg(nn);
                                                  QSqlQuery q = qq();
                                                  execSql(q, sql, sp2s($6));
                                                  getListFromQuery(q, *$$, 0);
                                                }
        ;
// tranzakciókezelés
sql     : NOTIFY_T str ';'                      { yySqlExec("NOTIFY " + sp2s($2)); }
        | NOTIFY_T str ',' str ';'              { yySqlExec(QString("NOTIFY %1, '%2'").arg(sp2s($2), sp2s($4))); }
        | RESET_T str ';'                       { yySqlExec(QString("NOTIFY %1, 'reset'").arg(sp2s($2))); }
        | RESET_T CACHE_T DATA_T ';'            { lanView::resetCacheData(); }
        | SQL_T '(' str ')' ';'                 { yySqlExec(sp2s($3)); }
        | SQL_T '(' str ',' vals ')' ';'        { yySqlExec(sp2s($3), $5); }
        ;
eqs     : '#' NAME_V '=' iexpr ';'              { ivars[*$2] = $4; delete $2; }
        | '&' NAME_V '=' sexpr ';'              { svars[*$2] = *$4; delete $2; delete $4; }
        | '@' NAME_V '=' vals ';'               { avars[*$2] = *$4; delete $2; delete $4; }
        ;
str_    : STRING_V                              {
                                                    QString dummy = QString("STRING : $1 = ") + debPString($1);
                                                    $$ = $1;
                                                }
        | NAME_V                                { $$ = $1; }
        | NULL_T                                { $$ = new QString(); }
        | STRING_T '(' iexpr ')'                { $$ = new QString(QString::number($3)); }
        | MASK_T  '(' sexpr ',' iexpr ')'       { $$ = new QString(nameAndNumber(sp2s($3), (int)$5)); }
        | STRING_T MACRO_T '(' sexpr ')'        { $$ = new QString(templates._get(_sMacros, sp2s($4))); }
        | PATCH_T TEMPLATE_T '(' sexpr ')'      { $$ = new QString(templates._get(_sPatchs, sp2s($4))); }
        | NODE_T  TEMPLATE_T '(' sexpr ')'      { $$ = new QString(templates._get(_sNodes,  sp2s($4))); }
        | LOWER_T '(' sexpr ')'                 { $$ = $3; *$$ = $$->toLower(); }
        | UPPER_T '(' sexpr ')'                 { $$ = $3; *$$ = $$->toUpper(); }
        ;
str     : str_                      { $$ = $1; }
        | '&' '[' sexpr ']'         { $$ = $3; }
        | '&' NAME_V                { *($$ = $2) = vstr(*$2); }
        ;
sexpr   : str_                      { $$ = $1; }
        | '(' sexpr ')'             { $$ = $2; }
        | sexpr '+' sexpr           { *($$ = $1) += *$3; delete $3; }
        | iexpr '*' sexpr           { *($$ = $3)  =  $3->repeated($1); }
        | REPLACE_T str '/' str '/' str { $$ = strReplace($2, $4, $6); }
        ;
str_z   : str                       { $$ = $1; }
        |                           { $$ = new QString(); }
        ;
str_zz  : str_z                     { $$ = $1; }
        | '@'                       { $$ = new QString("@"); }
        ;
strs    : str                       { $$ = new QStringList(*$1); delete $1; }
        | list_m                    { $$ = $1; }
        | strs ',' str              { $$ = $1;   *$$ << *$3;     delete $3; }
        | strs ',' list_m           { $$ = $1;   *$$ << *$3;     delete $3; }
        ;
strsz   : strs                      { $$ = $1; }
        |                           { $$ = new QStringList; }
        ;
strs_z  : str_z                     { $$ = new QStringList(*$1); delete $1; }
        | list_m                    { $$ = $1; }
        | strs_z ',' str_z          { $$ = $1;   *$$ << *$3;     delete $3; }
        | strs_z ',' list_m         { $$ = $1;   *$$ << *$3;     delete $3; }
        ;
strs_zz : str_zz                    { $$ = new QStringList(*$1); delete $1; }
        | list_m                    { $$ = $1; }
        | strs_zz ',' str_zz        { $$ = $1;   *$$ << *$3;     delete $3; }
        | strs_zz ',' list_m        { $$ = $1;   *$$ << *$3;     delete $3; }
        ;

int_    : INTEGER_V                             { $$ = $1; }
        | ID_T NODE_T '(' node_id ')'           { $$ = $4; }
        | ID_T PORT_T '(' port_id ')'           { $$ = $4; }
        | ID_T HOST_T SERVICE_T '(' hsid ')'    { $$ = $5; }
        | ID_T SERVICE_T '(' srvid ')'          { $$ = $4; }
        | ID_T SERVICE_T TYPE_T '(' srvtid ')'  { $$ = $5; }
        | ID_T PLACE_T '(' place_id ')'         { $$ = $4; }
        | ID_T PLACE_T GROUP_T '(' plg_id ')'   { $$ = $5; }
        | ID_T TIME_T PERIOD_T '(' tmpid ')'    { $$ = $5; }
        | ID_T SUBNET_T '(' snet_id ')'         { $$ = $4; }
        | ID_T IFTYPE_T '(' ift_id ')'          { $$ = $4; }
        | ID_T VLAN_T '(' vlan_id ')'           { $$ = $4; }
        | ID_T USER_T '(' usr_id ')'            { $$ = $4; }
        | ID_T USER_T GROUP_T '(' grpid ')'     { $$ = $5; }
        | ID_T GROUP_T '(' grpid ')'            { $$ = $4; }
        | ID_T TABLE_T SHAPE_T '(' tmod ')'     { $$ = $5; }
        | ID_T TABLE_T SHAPE_T '(' ftmod ')'    { $$ = $5; }
        | ID_T SERVICE_T VAR_T TYPE_T '(' srvvt_id ')' { $$ = $6; }
        | '#' NAME_V                            { $$ = vint(*$2); delete $2; }
        | '#' '+' NAME_V                        { $$ = (vint(*$3) += 1); delete $3; }
        | '#' '-' NAME_V                        { $$ = (vint(*$3) -= 1); delete $3; }
        ;
int     : int_                                  { $$ = $1; }
        | '-' INTEGER_V                         { $$ =-$2; }
        | '#' '[' iexpr ']'                     { $$ = $3; }
        | '#' NULL_T                            { $$ = NULL_ID; }
        ;
// Név alapján a patchs (vagy leszármazottja) rekord ID-t adja vissza (node_id)
node_id : str                                   { $$ = cPatch().getIdByName(qq(), *$1); delete $1; }
        ;
// Port ID a node_id és port név, vagy port index alapján
port_id : node_id ':' str                       { $$ = cNPort().setPortByName(qq(), *$3, $1).getId(); delete $3; }
        | node_id ':' int                       { $$ = cNPort().setPortByIndex(qq(), $3, $1).getId(); }
        ;
// Név alapján a vlans rekord ID-t adja vissza
vlan_id : str                                   { $$ = cVLan().getIdByName(qq(), *$1); delete $1; }
// Az int-el azonos, de ellenörzi, hogy az érték egy vlan id-e
        | int                                   { cVLan().getNameById(qq(), $$ = $1, EX_ERROR); }
        ;
// Név alapján a places rekord ID-t adja vissza
place_id: str                                   { $$ = cPlace().getIdByName(qq(), *$1); delete $1; }
        ;
// Név alapján a services rekord ID-t adja vissza
srvid   : str                                   { $$ = cService().getIdByName(qq(),*$1); delete $1; }
        ;
srvtid  : str                                   { $$ = cServiceType().getIdByName(qq(),*$1); delete $1; }
        ;
// Név alapján a timeperiods rekord ID-t adja vissza
tmpid   : str                       { $$ = cTimePeriod().getIdByName(qq(), *$1); delete $1; }
        ;
snet_id : str                       { $$ = cSubNet().getIdByName(qq(), sp2s($1)); }
        | ip                        { cSubNet n; int i = n.getByAddress(qq(), *$1); delete $1;
                                      if (i < 0) yyerror("Not found.");
                                      if (i > 1) yyerror("Ambiguous.");
                                      $$ = n.getId(); }
        ;
ift_id  : str                       { $$ = cIfType::ifTypeId(*$1); delete $1; }
        ;
image_id: str                       { $$ = cImage().getIdByName(sp2s($1)); }
        ;
image_idz : image_id                { $$ = $1; }
        |                           { $$ = NULL_ID; }
        ;
plg_id  : str                       { $$ = cPlaceGroup().getIdByName(sp2s($1)); }
        ;
usr_id  : str                       { $$ = cGroup().getIdByName(sp2s($1)); }
        ;
// Név alapján a groups (felhasználói csoport) rekord ID-t adja vissza
grpid   : str                       { $$ = cGroup().getIdByName(qq(), *$1); delete $1; }
        ;
grpids  : grpid                     { *($$ = new QVariantList) << $1; }
        | grpids ',' grpid          { *($$ = $1) << $3; }
        ;
srvvt_id: str                       { $$ = cServiceVarType::srvartype(qq(), *$1)->getId(); delete $1; }
        ;
/* */
iexpr   : int_                      { $$ = $1; }
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
ix_z    :                           { $$ = NULL_ID; }
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
/*real    : FLOAT_V                 { $$ = $1; }
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
bool_   : ON_T                      { $$ = true; }
        | YES_T                     { $$ = true; }
        | TRUE_T                    { $$ = true; }
        | OFF_T                     { $$ = false; }
        | NO_T                      { $$ = false; }
        | FALSE_T                   { $$ = false; }
        ;
bool    : bool_                     { $$ = $1; }
        | NOT_T bool                { $$ = !$2; }
        | bool AND_T bool           { $$ = $1 && $3; }
        | bool OR_T  bool           { $$ = $1 || $3; }
        | sexpr '=' '=' sexpr       { $$ = 0 == $1->compare(*$4); delete $1; delete $4; }
        | sexpr '!' '=' sexpr       { $$ = 0 != $1->compare(*$4); delete $1; delete $4; }
        | iexpr '=' '=' iexpr       { $$ = $1 == $4; }
        | iexpr '!' '=' iexpr       { $$ = $1 != $4; }
        | DEFINED_T ifdef           { $$ = $2; }
        | sexpr '~' sexpr           { QRegularExpression r(sp2s($3)); $$ = r.match(sp2s($1)).hasMatch(); }
        ;
bool_on :                           { $$ = true; }
        | bool_                     { $$ = $1; }
        ;
bool_off:                           { $$ = false; }
        | bool_                     { $$ = $1; }
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
//      | NULL_T                    { $$ = new QVariant(); }
        | '@' str '[' int ']'       { $$ = new QVariant(varray(sp2s($2))[$4]); }
        ;
vals    : value                     { $$ = new QVariantList(); *$$ << *$1; delete $1; }
        | list_m                    { $$ = new QVariantList(slp2vl($1)); }
        | list_i                    { $$ = new QVariantList(); foreach (qlonglong i, *$1) *$$ << QVariant(i); delete $1; }
        | vals ',' value            { $$ = $1;                 *$$ << *$3; delete $3; }
        | vals ',' list_m           { $$ = $1;                 *$$ << slp2vl($3); }
        | vals ',' list_i           { $$ = $1; foreach (qlonglong i, *$3) *$$ << QVariant(i); delete $3; }
        | SQL_T '(' str ')'         { yySqlExec(sp2s($3),nullptr, $$ = new QVariantList); }
        | SQL_T '(' str ',' vals ')'{ yySqlExec(sp2s($3),  $5, $$ = new QVariantList); }
        | '@' str                   { $$ = new QVariantList(varray(sp2s($2))); }
        ;
num     : iexpr                     { $$ = $1; }
        | fexpr                     { $$ = $1; }
        ;
replace :                           { $$ = REPLACE_DEF; }
        | REPLACE_T                 { $$ = REPLACE_ON;  }
        | INSERT_T                  { $$ = REPLACE_OFF; }
        ;
replfl  : replace                   { switch ($1) {
                                        case REPLACE_ON:    $$ = true;              break;
                                        case REPLACE_OFF:   $$ = false;             break;
                                        case REPLACE_DEF:   $$ = globalReplaceFlag; break;
                                      }
                                    }
        ;
features: str ';'                   { $$ = $1; }
        | '{' feature_s '}'         { $$ = new QString(cFeatures::join(*$2)); }
        ;
feature_s: feature                  { ($$ = new tStringMap)->insert($1->first, $1->second); delete $1; }
        | feature_s feature         { ($$ = $1)            ->insert($2->first, $2->second); delete $2; }
        ;
feature : map
        | str '=' '{' maps '}'      { $$ = new tStringPair(sp2s($1), cFeatures::map2value(*$4)); /* auto.delete $4; */ }
        ;
maps    : map                       { ($$ = new tStringMap)->insert($1->first, $1->second); delete $1; }
        | maps map                  { ($$ = $1)            ->insert($2->first, $2->second); delete $2; }
        ;
map     : str '=' vals ';'          { $$ = new tStringPair(sp2s($1), cFeatures::list2value(*$3, true)); delete $3; }
        | str ';'                   { $$ = new tStringPair(sp2s($1), _sNul); }
        ;
// felhasználók, felhesználói csoportok definíciója.
user    : USER_T replace str str_z              { REPOBJ(pUser, cUser(), $2, $3, $4); pUser->setId(_sPlaceId, gPlace()); }
            user_e                              { writeAndDel(pUser); }
        | USER_T GROUP_T str str_z              { REPOBJ(pGroup, cGroup(), REPLACE_DEF, $3, $4); }
            ugrp_e                              { writeAndDel(pGroup); }
        | USER_T GROUP_T REPLACE_T str str_z    { REPOBJ(pGroup, cGroup(), REPLACE_ON, $4, $5); }
            ugrp_e                              { writeAndDel(pGroup); }
        | USER_T GROUP_T INSERT_T str str_z     { REPOBJ(pGroup, cGroup(), REPLACE_OFF, $4, $5); }
            ugrp_e                              { writeAndDel(pGroup); }
//  Felhasználói csoportagság kezelése
        | USER_T GROUP_T str ADD_T str ';'      { tGroupUser gu(qq(), *$3, *$5); if (NULL_ID == gu.test(qq())) gu.insert(qq()); delete $3; delete $5; }
        | USER_T GROUP_T str REMOVE_T str ';'   { tGroupUser gu(qq(), *$3, *$5); if (NULL_ID != gu.test(qq())) gu.remove(qq()); delete $3; delete $5; }
        ;
// Felhasználók tulajdonságai
user_e  : ';'
        | '{' user_ps '}'
        ;
user_ps :
        | user_ps user_p
        ;
user_p  : NOTE_T str ';'                        { SETNOTE(pUser, $2); }
        | DOMAIN_T USER_T strs ';'              { pUser->setStringList(_sDomainUsers, slp2sl($3));}
        | ADD_T DOMAIN_T USER_T strs ';'        { pUser->addStringList(_sDomainUsers, slp2sl($4));}
        | HOST_T NOTIF_T PERIOD_T tmpid ';'     { pUser->setId(_sHostNotifPeriod, $4); }
        | SERVICE_T NOTIF_T PERIOD_T tmpid ';'  { pUser->setId(_sServNotifPeriod, $4); }
        | HOST_T NOTIF_T SWITCH_T nsws ';'      { pUser->set(_sHostNotifSwitchs, QVariant(*$4)); delete $4; }
        | SERVICE_T NOTIF_T SWITCH_T nsws ';'   { pUser->set(_sServNotifSwitchs, QVariant(*$4)); delete $4; }
        | HOST_T NOTIF_T COMMAND_T str ';'      { pUser->setName(_sHostNotifCmd, *$4); delete $4; }
        | SERVICE_T NOTIF_T COMMAND_T str ';'   { pUser->setName(_sServNotifCmd, *$4); delete $4; }
        | TEL_T strs ';'                        { pUser->setStringList(_sTels, slp2sl($2)); }
        | ADD_T TEL_T strs ';'                  { pUser->addStringList(_sTels, slp2sl($3)); }
        | ADDRESS_T strs ';'                    { pUser->setStringList(_sAddresses, slp2sl($2)); }
        | ADD_T ADDRESS_T strs ';'              { pUser->addStringList(_sAddresses, slp2sl($3)); }
        | PLACE_T place_id ';'                  { pUser->setId(_sPlaceId, $2); }
        | DISABLE_T ';'                         { pUser->setBool(_sEnabled, false); }
        | ENABLE_T ';'                          { pUser->setBool(_sEnabled, true); }
        | CLEAR_T ';'                           { pUser->clear(~pUser->mask(_sUserId, _sUserName)); }
        | CLEAR_T usrfns ';'                    { pUser->clear(pUser->mask(slp2sl($2))); }
        | PLACE_T ';'                           { pUser->setId(_sPlaceId, globalPlaceId); }
        | COPY_T copy_from ';'                  { pUser->fieldsCopy(qq(), $2, ~pUser->mask(_sUserId, _sUserName)); pDelete($2); }
        | COPY_T usrfns copy_from ';'           { pUser->fieldsCopy(qq(), $3,  pUser->mask(slp2sl($2))); pDelete($3); }
        | COPY_T EXCEPT_T usrfns copy_from ';'  { pUser->fieldsCopy(qq(), $4, ~(pUser->mask(_sUserId, _sUserName) | pUser->mask(slp2sl($3)))); pDelete($4); }
        ;
copy_from:                                      { $$ = nullptr; }
        | FROM_T str                            { $$ = $2; }
        ;
nsws    : ALL_T                                 { $$ = new QStringList(cUser().descr()[_sHostNotifSwitchs].enumType().enumValues); }
        |                                       { $$ = new QStringList(); }
        | nsws_                                 { $$ = $1; }
        ;
nsws_   : nsw                                   { $$ = new QStringList(*$1); delete $1; }
        | nsws_ ',' nsw                         { *($$ = $1) << *$3; delete $3; }
        ; 
nsw     : str                                   { (void)notifSwitch(*$1); $$ = $1; }
        ;
usrfns  : usrfn                         { $$ = new QStringList(); *$$ << *$1; delete $1; }
        | usrfns ',' usrfn              { $$ = $1;                *$$ << *$3; delete $3; }
        ;
usrfn   : NOTE_T                        { $$ = new QString(_sUserNote); }
        | HOST_T NOTIF_T PERIOD_T       { $$ = new QString(_sHostNotifPeriod); }
        | SERVICE_T NOTIF_T PERIOD_T    { $$ = new QString(_sServNotifPeriod); }
        | HOST_T NOTIF_T SWITCH_T       { $$ = new QString(_sHostNotifSwitchs); }
        | SERVICE_T NOTIF_T SWITCH_T    { $$ = new QString(_sServNotifSwitchs); }
        | HOST_T NOTIF_T COMMAND_T      { $$ = new QString(_sHostNotifCmd); }
        | SERVICE_T NOTIF_T COMMAND_T   { $$ = new QString(_sServNotifCmd); }
        | TEL_T                         { $$ = new QString(_sTels); }
        | ADDRESS_T                     { $$ = new QString(_sAddresses); }
        | PLACE_T                       { $$ = new QString(_sPlaceId); }
        | str                           { $$ = $1; }
        ;
// Felhasználói csoportok tulajdonságai
ugrp_e  : ';'
        | '{' ugrp_ps '}'
        ;
ugrp_ps :
        | ugrp_ps ugrp_p
        ;
ugrp_p  : NOTE_T str ';'                        { SETNOTE(pGroup, $2); }
        | RIGHTS_T rights ';'                   { pGroup->setName(_sGroupRights, *$2); delete $2; }
        | PLACE_T GROUP_T plg_id ';'            { pGroup->setId(_sPlaceGroupId, $3); }
        | FEATURES_T features                   { pGroup->setName(_sFeatures); }
        | CLEAR_T ';'                           { pGroup->clear(~pGroup->mask(_sGroupId, _sGroupName)); }
        | CLEAR_T usrgfns ';'                   { pGroup->clear(pGroup->mask(slp2sl($2))); }
        | COPY_T FROM_T str ';'                 { pGroup->fieldsCopy(cGroup().setByName(qq(), sp2s($3)), ~pGroup->mask(_sGroupId, _sGroupName));}
        | COPY_T usrgfns FROM_T str ';'         { pGroup->fieldsCopy(cGroup().setByName(qq(), sp2s($4)),  pGroup->mask(slp2sl($2))); }
        | COPY_T EXCEPT_T usrgfns FROM_T str ';'{ pGroup->fieldsCopy(cGroup().setByName(qq(), sp2s($5)), ~(pGroup->mask(_sGroupId, _sGroupName) | pUser->mask(slp2sl($3)))); }
        ;
rights  : str                                   { $$ = $1;
                                                  if (cColStaticDescr::VC_INVALID == cGroup().descr()[_sGroupRights].check(*$$)) {
                                                      yyerror(QObject::tr("Ivalid rights value : %1").arg(*$1));
                                                      delete $1;
                                                  }
                                                }
        ;
usrgfn  : NOTE_T                                { $$ = new QString(_sGroupNote); }
        | RIGHTS_T                              { $$ = new QString(_sGroupRights); }
        | PLACE_T GROUP_T                       { $$ = new QString(_sPlaceGroupId); }
        | FEATURES_T                            { $$ = new QString(_sFeatures); }
        | str                                   { $$ = $1; }
        ;
usrgfns : usrgfn                                { $$ = new QStringList(); *$$ << *$1; delete $1; }
        | usrgfns ',' usrgfn                    { $$ = $1;                *$$ << *$3; delete $3; }
        ;
//Timeperiod rekord definíció (insert), napi időperiódusok hozzárendelése/definíciója
timeper : TIME_T PERIOD_T str str_z ';'         { INSREC(cTimePeriod, _sTimePeriodNote, $3, $4); }
        | tod ADD_T TIME_T PERIOD_T str ';'     { tTimePeriodTpow(qq(), *$5, *$1).insert(qq()); delete $1; delete $5; }
        | toddef
        ;
tod     : _toddef                   { $$ = $1; }
        | TIME_T OF_T DAY_T str     { $$ = $4; }
        ;
// Egy napon bellüli időpont megadása
time    : INTEGER_V ':' INTEGER_V               { $$ = sTime($1, $3); }
        | INTEGER_V ':' INTEGER_V ':' INTEGER_V { $$ = sTime($1, $3, double($5)); }
        | INTEGER_V ':' INTEGER_V ':' FLOAT_V   { $$ = sTime($1, $3, $5); }
        ;
// Inzertál egy time_of_day rekordot, a rekord névvel tár vissza
_toddef : TIME_T OF_T DAY_T str str FROM_T time TO_T time str_z	{ $$ = toddef($4, $5, $7, $9, $10); }
        ;
// Inzertál egy time_of_day rekordot, önálló parancsként
toddef  : _toddef ';'               { delete $1; }
        ;
// Rendszer, vagy port paraméterek
params  : ptype
        | syspar
        ;
// Paraméter típus definíciók
ptype   : PARAM_T replfl str str_z TYPE_T str str_z ';'{ cParamType::insertNew(qq(), sp2s($3), sp2s($4), paramTypeType(sp2s($6)), sp2s($7), $2); }
        ;
// Renddszerparaméterek definiálása
syspar  : SYS_T str PARAM_T str '=' value ';'   { cSysParam::setSysParam(qq(), sp2s($4), vp2v($6), sp2s($2)); }
        ;
// VLAN definíciók
vlan    : VLAN_T int str str_z      {
                                        actVlanId = cVLan::insertNew($2, actVlanName = *$3, actVlanNote = *$4, true);
                                        delete $3; delete $4;
                                        netType = NT_PRIMARY;
                                    }
            '{' subnets '}'         { actVlanId = -1; }
        | PRIVATE_T SUBNET_T        { actVlanId = -1; netType = NT_PRIVATE; }
          subnet                    { netType = ENUM_INVALID; }
        | PSEUDO_T  SUBNET_T        { actVlanId = -1; netType = NT_PSEUDO;  }
          subnet                    { netType = ENUM_INVALID; }
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
ip      : IPV4_V                    { $$ = $1; }
        | IPV6_V                    { $$ = $1; }
        | IP_T '(' sexpr ')'        { $$ = new QHostAddress(); if (!$$->setAddress(*$3)) yyerror("Invalid address"); }
        ;
ips     : ip                        { $$ = new QString($1->toString()); delete $1; }
        ;
// Image definíciók
image   : IMAGE_T replace str str_z { REPOBJ(pImage, cImage(), $2, $3, $4); }
            image_e                 { writeAndDel(pImage); }
        ;
image_e : '{' image_ps '}'
        ;
image_ps: image_p
        | image_ps image_p
        ;
image_p : TYPE_T imgty ';'          { pImage->setName(_sImageType, *$2); delete $2; }
        | SUB_T TYPE_T str ';'      { pImage->setName(_sImageSubType, *$3); delete $3; }
        | IMAGE_T FILE_T str ';'    { pImage->load(*$3); delete $3; }
        ;
imgty   : str                       { $$ = $1;
                                      if (cColStaticDescr::VC_INVALID == cImage().descr()[_sImageType].check(*$1)) {
                                           yyerror(QObject::tr("Ivalid notif swich value : %1").arg(*$1));
                                           delete $1;
                                      }
                                    }
        ;
// Helyiség definíciók
place   : PLACE_T replace str str_z             { REPOBJ(pPlace, cPlace, $2, $3, $4); pPlace->setId(_sParentId, globalPlaceId); }
          place_e                               { writeAndDel(pPlace); }
        | PLACE_T GROUP_T str str_z pgtype ';'  { if (globalReplaceFlag) cPlaceGroup::replaceNew(qq(), sp2s($3), sp2s($4), $5);
                                                  else                   cPlaceGroup::insertNew( qq(), sp2s($3), sp2s($4), $5); }
        | PLACE_T GROUP_T REPLACE_T str str_z pgtype ';'{cPlaceGroup::replaceNew(qq(), sp2s($4), sp2s($5), $6); }
        | PLACE_T GROUP_T INSERT_T str str_z pgtype ';'{ cPlaceGroup::insertNew(qq(), sp2s($4), sp2s($5), $6); }
        | PLACE_T GROUP_T str ADD_T str ';'     { cGroupPlace gp(qq(), *$3, *$5); if (NULL_ID == gp.test(qq())) gp.insert(qq()); delete $3; delete $5; }
        | PLACE_T GROUP_T str REMOVE_T str ';'  { cGroupPlace gp(qq(), *$3, *$5); if (NULL_ID != gp.test(qq())) gp.remove(qq()); delete $3; delete $5; }
        ;
place_e : ';'
        | '{' plac_ps '}'
        ;
plac_ps : place_p
        | plac_ps place_p
        ;
place_p : NOTE_T str ';'                        { pPlace->set(_sPlaceNote, *$2);     delete $2; }
        | PARENT_T place_id ';'                 { pPlace->set(_sParentId, $2); }
        | IMAGE_T image_id ';'                  { pPlace->set(_sImageId,  $2); }
        | FRAME_T frame ';'                     { pPlace->set(_sFrame, QVariant::fromValue(*$2)); delete $2; }
        | RECTANGLE_T rectangle ';'             { pPlace->set(_sFrame, QVariant::fromValue(*$2)); delete $2; }
        | TEL_T strs ';'                        { pPlace->set(_sTels, *$2);            delete $2; }
        | CLEAR_T ';'                           { pPlace->clear(~pPlace->mask(_sPlaceId, _sPlaceName)); }
        | CLEAR_T plfns ';'                     { pPlace->clear(pPlace->mask(slp2sl($2))); }
        | PARENT_T ';'                          { pPlace->setId(_sParentId, globalPlaceId); }
        | COPY_T FROM_T str ';'                 { pPlace->fieldsCopy(cPlace().setByName(qq(), sp2s($3)), ~pPlace->mask(_sPlaceId, _sPlaceName));}
        | COPY_T plfns FROM_T str ';'           { pPlace->fieldsCopy(cPlace().setByName(qq(), sp2s($4)),  pPlace->mask(slp2sl($2))); }
        | COPY_T EXCEPT_T plfns FROM_T str ';'  { pPlace->fieldsCopy(cPlace().setByName(qq(), sp2s($5)), ~(pPlace->mask(_sPlaceId, _sPlaceName) | pPlace->mask(slp2sl($3)))); }
        ;
frame   : '{' points  '}'           { $$ = $2; }
        ;
points  : point                     { $$ = new tPolygonF(); *$$ << *$1;  delete $1; }
        | points ',' point          { $$ = $1;              *$$ << *$3;  delete $3; }
        ;
point   : '[' num ',' num ']'       { $$ = new QPointF($2, $4); }
        ;
rectangle : '{' point ',' point '}' { $$ = rectangle($2, $4); }
        ;
plfns   : plfn                      { $$ = new QStringList(); *$$ << *$1; delete $1; }
        | plfns ',' plfn            { $$ = $1;                *$$ << *$3; delete $3; }
        ;
plfn    : NOTE_T                    { $$ = new QString(_sPlaceNote); }
        | PARENT_T                  { $$ = new QString(_sParentId); }
        | IMAGE_T                   { $$ = new QString(_sImageId); }
        | FRAME_T                   { $$ = new QString(_sFrame); }
        | TEL_T                     { $$ = new QString(_sTels); }
        ;
pgtype  :                           { $$ = PG_GROUP; }
        | CATEGORY_T                { $$ = PG_CATEGORY; }
        | ZONE_T                    { $$ = PG_ZONE; }
        ;
iftype  : IFTYPE_T replace str str_z IANA_T int str str prefered ';'
                                    { cIfType::insertNew(qq(), $2, sp2s($3), sp2s($4), (int)$6, sp2s($7), sp2s($8), $9); }
        | IFTYPE_T replace str str_z IANA_T int TO_T int ';'
                                    { cIfType::insertNew(qq(), $2, sp2s($3), sp2s($4), (int)$6, (int)$8); }
        ;
prefered:                           { $$ = false; }
        | PREFERED_T                { $$ = true; }
        ;
nodes   : patch
        | node
patch   : patch_h                               { if (pPatch->isNull(_sPlaceId)) pPatch->setId(_sPlaceId, gPlace());
                                                  pPatch->containerValid = CV_ALL_PATCH; }
          patch_e                               { writeAndDel(pPatch); }
        | ADD_T PATCH_T str PORT_T              { NEWOBJ(pPatch, cPatch); pPatch->setByName(sp2s($3)); }
          apport                                { DELOBJ(pPatch); }
        ;
        | SET_T PATCH_T str                     { NEWOBJ(pPatch, cPatch); pPatch->setByName(qq(), sp2s($3)); pPatch->fetchPorts(qq()); }
          spport                                { DELOBJ(pPatch); }

        ;
patch_h : PATCH_T replace str str_z                 { REPOBJ(pPatch, cPatch, $2, $3, $4); }
        | PATCH_T replace str str_z TEMPLATE_T str  { REPOBJ(pPatch, cPatch, $2, $3, $4); templates.get(_sPatchs, sp2s($6)); }
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
        | SET_T str '=' value ';'               { pPatch->set(sp2s($2), vp2v($4)); }
        | PARAM_T str '=' value ':' ':' str ';' { pPatch->setParam(sp2s($2), vp2v($4), sp2s($7)); }
        | INVENTORY_T NUMBER_T str ';'          { pPatch->setName(_sInventoryNumber, sp2s($3)); }
        | SERIAL_T NUMBER_T str ';'             { pPatch->setName(_sSerialNumber, sp2s($3)); }
        | ADD_T PORT_T int str str_z ';'        { setLastPort(pPatch->addPort(sp2s($4), sp2s($5), $3)); }
        | ADD_T PORT_T int str str_z TAG_T str ';'
                                                { cNPort *p = pPatch->addPort(sp2s($4), sp2s($5), $3); setLastPort(p); p->setName(_sPortTag, sp2s($7)); }
        | ADD_T PORTS_T offs FROM_T int TO_T int offs str ';'
                                                { setLastPort(pPatch->addPorts(sp2s($9), $8, $5, $7, $3)); }
        | PORT_T pix NOTE_T strs ';'            { setLastPort(pPatch->portSet($2, _sPortNote, slp2vl($4))); }
        | PORT_T pnm NOTE_T str ';'             { setLastPort(pPatch->portSet(sp2s($2), _sPortNote, sp2s($4))); }
        | PORT_T pix TAG_T strs ';'             { setLastPort(pPatch->portSet($2, _sPortTag, slp2vl($4))); }
        | PORT_T pnm TAG_T str ';'              { setLastPort(pPatch->portSet(sp2s($2), _sPortTag, sp2s($4))); }
        | PORT_T pix SET_T str '=' vals ';'     { setLastPort(pPatch->portSet($2, sp2s($4), vlp2vl($6))); }
        | PORT_T pnm SET_T str '=' value ';'    { setLastPort(pPatch->portSet(sp2s($2), sp2s($4), vp2v($6))); }
        | PORT_T pnm PARAM_T str '=' value ':' ':' str ';'  { setLastPort(pPatch->portSetParam(sp2s($2), sp2s($4), vp2v($6), sp2s($9))); }
        | PORT_T pix PARAM_T str '=' vals ':' ':' str ';'   { setLastPort(pPatch->portSetParam($2, sp2s($4), vlp2vl($6), sp2s($9))); }
        | PORTS_T p_seqs SHARED_T strs ';'      { portSetShare($2, $4); }
        | PORT_T  p_seqs NC_T ';'               { portSetNC($2); }
        | for_m
        | eqs
        | CLEAR_T ';'                           { pPatch->clear(~pPatch->mask(_sNodeId, _sNodeName));
                                                  pPatch->ports.clear();
                                                  pPatch->params.clear(); }
        | CLEAR_T clrpfns ';'
        | PLACE_T ';'                           { pPatch->setId(_sPlaceId, gPlace()); }
        | COPY_T FROM_T str ';'                 { patchCopy(nullptr, nullptr, $3); }
        | COPY_T ptcfns FROM_T str ';'          { patchCopy(nullptr, $2, $4); }
        | COPY_T EXCEPT_T usrfns FROM_T str ';' { patchCopy($3, nullptr, $5); }
        ;
// Opcionális offsett, alapértelmezetten 0
offs    : OFFS_T int                    { $$ = $2; }
        |                               { $$ = 0; }
        ;
// Port index megadásának a lehetőségei
pix     : INTEGER_V                     { $$ = $1; }
        | '#' NAME_V                    { $$ = vint(*$2); delete $2; }
        | '#' '+' NAME_V                { $$ = (vint(*$3) += 1); delete $3; }
        | '#' '[' iexpr ']'             { $$ = $3; }
        | '#' '@'                       { $$ = vint(sPortIx); }
        | '#' '+' '@'                   { $$ = (vint(sPortIx) += 1); }
        ;
// Port index megadásának a lehetőségei, opcionális, alapértelmetetten NULL_IX
pix_z   : INTEGER_V                     { $$ = $1; }
        | '#' NAME_V                    { $$ = vint(*$2); delete $2; }
        | '#' '+' NAME_V                { $$ = (vint(*$3) += 1); delete $3; }
        | '#' '[' iexpr ']'             { $$ = $3; }
        | '#' '@'                       { $$ = vint(sPortIx); }
        | '#' '+' '@'                   { $$ = (vint(sPortIx) += 1); }
        |                               { $$ = NULL_ID; }
        ;
// Port név magdásának a lehetőségie
pnm     : str                                   { $$ = $1; }
        | '&' '@'                               { $$ = new QString(vstr(sPortNm)); }
        ;
// A port nevéből port a indexe a pPatch->ports kontéberben
p_seq   : pnm                                   { $$ = portName2SeqN(sp2s($1));  }
// A port indexéből (port_index mező értéke) a port indexe a pPatch->ports kontéberben
        | pix                                   { $$ = portIndex2SeqN($1); }
        ;
p_seqsl : list_i                                {   $$ = new intList;
                                                    foreach (int ix, *$1) {
                                                        *$$ << portIndex2SeqN(ix);
                                                    }
                                                    delete $1;
                                                }
        |  list_m                               {   $$ = new intList;
                                                    foreach (QString n, *$1) {
                                                         *$$ << portName2SeqN(n);
                                                     }
                                                     delete $1;
                                                }
        ;
p_seqs  : p_seqs ',' p_seq                      { *($$ = $1)          <<  $3; }
        | p_seqs ',' p_seqsl                    { *($$ = $1)          += *$3; delete $3; }
        | p_seq                                 { *($$ = new intList) << $1; }
        | p_seqsl                               { $$ = $1; }
        ;
clrpfns : clrpfn
        | clrpfns ',' clrpfn
        ;
clrpfn  : PORTS_T                               { pPatch->ports.clear(); }
        | PARAMS_T                              { pPatch->params.clear(); }
        | NOTE_T                                { pPatch->clear(_sNodeNote); }
        | PLACE_T                               { pPatch->clear(_sPlaceId); }
        | FEATURES_T                            { pPatch->clear(_sFeatures); }
        | DELETED_T                             { pPatch->clear(_sDeleted); }
        ;
ptcfns  : ptcfn                                 { $$ = new QStringList; *$$ << *$1; delete $1; }
        | ptcfns ',' ptcfn                      { $$ = $1;              *$$ << *$3; delete $3; }
        ;
ptcfn   : PORTS_T                               { $$ = new QString(_sPorts); }
        | PARAMS_T                              { $$ = new QString(_sParams); }
        | NOTE_T                                { $$ = new QString(_sNodeNote); }
        | PLACE_T                               { $$ = new QString(_sPlaceId); }
        | FEATURES_T                            { $$ = new QString(_sFeatures); }
        | DELETED_T                             { $$ = new QString(_sDeleted); }
        ;
apport  : int str str_z ';'                     { pPatch->insertPort(qq(), $1, sp2s($2), sp2s($3)); }
        | int str str_z TAG_T str ';'           { pPatch->insertPort(qq(), $1, sp2s($2), sp2s($3), sp2s($5)); }
        ;
spport  : PORTS_T p_seqs SHARED_T strs ';'      { portUpdateShare($2, $4); }
        | PORTS_T p_seqs bool_on NC_T ';'       { portUpdateNc($2, $3); }
        ;
// nodes
node_h  : NODE_T replace node_ts                { $$ = $3; setReplace($2); }
        ;
node_ts :                                       { $$ = ENUM2SET(NT_NODE); }
        | '(' nodetype ')'                      { $$ = $2; }
        ;
nodetype: strs                                  { $$ = 0; foreach (QString s, *$1) { $$ |= ENUM2SET(nodeType(s)); } delete $1; }
        ;
node    : node_h str str_z                      { newHost($1, $2, nullptr, nullptr, $3); pPatch->containerValid = CV_ALL_NODE; }
                node_cf node_e                  { writeAndDel(pPatch); }
        | ATTACHED_T replace str str_z { setReplace($2); newAttachedNode(sp2s($3), sp2s($4)); }
                node_e                          { writeAndDel(pPatch); }
        | node_h name_q ip_q mac_q str_z        { newHost($1, $2, $3, $4, $5); pPatch->containerValid = CV_ALL_HOST; }
            node_cf node_e                      { writeAndDel(pPatch); }
        | WORKSTATION_T replace name_q ip_q mac_q str_z ';'
                                                { setReplace($2); newWorkstation(sp2s($3), $4, $5, sp2s($6)); delete $4; delete $5; }
                node_e                          { writeAndDel(pPatch); }
        | SNMP_T NODE_T str REFRESH_T ';'       { pPatch = cSnmpDevice::refresh(qq(), sp2s($3)); pDelete(pPatch); }
        ;
node_e  : '{' node_ps '}'
        | ';'
        ;
node_ps : node_p node_ps
        | 
        ;
node_cf :
        | TEMPLATE_T str    { templates.get(_sNodes, sp2s($2)); }   node_ps
        | READ_T            { node().setByName(qq()); node().fetchAllChilds(qq()); }
        ;
node_p  : TYPE_T '+' nodetype ';'               { node().setId(_sNodeType, node().getId(_sNodeType) | $3); }
        | NOTE_T str ';'                        { node().setName(_sNodeNote, sp2s($2)); }
        | PLACE_T place_id ';'                  { node().setId(_sPlaceId, $2); }
        | SET_T str '=' value ';'               { node().set(sp2s($2), vp2v($4)); }
        | PARAM_T str '=' value ':' ':' str ';' { node().setParam(sp2s($2), vp2v($4), sp2s($7)); }
        | INVENTORY_T NUMBER_T str ';'          { node().setName(_sInventoryNumber, sp2s($3)); }
        | SERIAL_T NUMBER_T str ';'             { node().setName(_sSerialNumber, sp2s($3)); }
        | OS_T str_z osver ';'                  { if (!$2->isNull()) node().setName(_sOsName, *$2);
                                                  if (!$3->isNull()) node().setName(_sOsVersion, *$3);
                                                  delete $2; delete $3;
                                                }
        | PORT_T pix { setLastPort($2); }       '{' np_ps '}'
        | PORT_T pnm { setLastPort(sp2s($2)); } '{' np_ps '}'
        | ADD_T PORTS_T str offs FROM_T int TO_T int offs str ';'
                                                { setLastPort(node().addPorts(sp2s($3), sp2s($10), $9, $6, $8, $4)); }
        | ADD_T PORT_T pix_z str str str_z      { setLastPort(node().addPort(sp2s($4), sp2s($5), sp2s($6), $3)); }
                np_pp
        | DELETE_T PORT_T str ';'               { cNPort *p = node().ports.pull(sp2s($3), EX_IGNORE); pDelete(p); }
        | DELETE_T PORT_T int ';'               { cNPort *p = node().ports.pull(_sPortIndex, QVariant($3), EX_IGNORE); pDelete(p); }
        | PORT_T pix TYPE_T ix_z str str str_z ';' { setLastPort(node().portModType($2, sp2s($5), sp2s($6), sp2s($7), $4)); }
        | PORT_T pix NAME_T str str_z ';'       { setLastPort(node().portModName($2, sp2s($4), sp2s($5))); }
        | PORT_T pix NOTE_T strs ';'            { setLastPort(node().portSet($2, _sPortNote, slp2vl($4))); }
        | PORT_T pnm NOTE_T str ';'             { setLastPort(node().portSet(sp2s($2), _sPortNote, sp2s($4))); }
        | PORT_T pix TAG_T strs ';'             { setLastPort(node().portSet($2, _sPortTag, slp2vl($4))); }
        | PORT_T pnm TAG_T str ';'              { setLastPort(node().portSet(sp2s($2), _sPortTag, sp2s($4))); }
        | PORT_T pnm SET_T str '=' value ';'    { setLastPort(node().portSet(sp2s($2), sp2s($4), vp2v($6))); }
        | PORT_T pix SET_T str '=' vals ';'     { setLastPort(node().portSet($2, sp2s($4), vlp2vl($6)));; }
        | PORT_T pnm PARAM_T str '=' value ':' ':' str ';'  { setLastPort(node().portSetParam(sp2s($2), sp2s($4), vp2v($6), sp2s($9))); }
        | PORT_T pix PARAM_T str '=' vals ':' ':' str ';'   { setLastPort(node().portSetParam($2, sp2s($4), vlp2vl($6), sp2s($9))); }
        /* host */
        | DELETE_T PORT_T mac ';'               { cNPort *p; while (nullptr != (p = node().ports.pull(_sHwAddress, QVariant::fromValue(*$3), EX_IGNORE))) { delete p; } delete $3; }
        | ADD_T PORT_T pix_z str str ip_qq mac_qq str_z { setLastPort(hostAddPort((int)$3, $4,$5,$6,$7,$8)); }
                np_pp
        | PORT_T pnm ADD_T ADDRESS_T ip_a str_z ';'         { setLastPort(portAddAddress($2, $5, $6)); }
        | PORT_T pix ADD_T ADDRESS_T ip_a str_z ';'         { setLastPort(portAddAddress((int)$2, $5, $6)); }
        | ADD_T SENSORS_T offs FROM_T int TO_T int offs str ip ';'  /* index offset ... név offset */
                                                            { setLastPort(node().addSensors(sp2s($9), $8, $5, $7, $3, *$10)); delete $10; }
        | PORT_T pnm VLAN_T vlan_id vlan_t set_t ';'        { setLastPort(node().portSetVlan(sp2s($2), $4, (enum eVlanType)$5, (enum eSetType)$6)); }
        | PORT_T pix VLAN_T vlan_id vlan_t set_t ';'        { setLastPort(node().portSetVlan(     $2,  $4, (enum eVlanType)$5, (enum eSetType)$6)); }
        | PORT_T pnm VLAN_T vlan_id ';'                     { setLastPort(node().portSetVlan(sp2s($2), $4, VT_HARD, ST_MANUAL)); }
        | PORT_T pix VLAN_T vlan_id ';'                     { setLastPort(node().portSetVlan(     $2,  $4, VT_HARD, ST_MANUAL)); }
        | PORT_T pnm VLANS_T vlan_ids ';'                   { setLastPort(node().portSetVlans(sp2s($2), *$4)); delete $4; }
        | PORT_T pix VLANS_T vlan_ids ';'                   { setLastPort(node().portSetVlans(     $2,  *$4)); delete $4; }
        /* snmpdev  */
        | PORTS_T BY_T SNMP_T str_z ';'             { snmpdev().setBySnmp(sp2s($4)); setLastPort(_sNul, NULL_IX); }
        | COMMUNITY_T str ';'                       { snmpdev().setName(_sCommunityRd, sp2s($2)); }
        | READ_T COMMUNITY_T str ';'                { snmpdev().setName(_sCommunityRd, sp2s($3)); }
        | WRITE_T COMMUNITY_T str ';'               { snmpdev().setName(_sCommunityWr, sp2s($3)); }
        | for_m
        | eqs
        ;
ip_q    : ips iptype                            { $$ = new tStringPair(sp2s($1),  addrType($2)); }
        | DYNAMIC_T                             { $$ = new tStringPair(QString(), _sDynamic); }
        | LOOKUP_T                              { $$ = new tStringPair(_sLOOKUP,  _sFixIp); }
        | ARP_T                                 { $$ = new tStringPair(_sARP,     _sFixIp); }
        ;
ip_qq   : ips iptype                            { $$ = new tStringPair(sp2s($1),  addrType($2)); }
        | DYNAMIC_T                             { $$ = new tStringPair(QString(), _sDynamic); }
        | NULL_T                                { $$ = nullptr; }
        | ARP_T                                 { $$ = new tStringPair(_sARP,     _sFixIp); }
        ;
ip_a    : ips iptype                            { $$ = new tStringPair(sp2s($1),  addrType($2)); }
        ;
mac     : MAC_V                                 { $$ = $1; }
        | MAC_T '(' sexpr ')'                   {
                                                    QString dummy = QString("MAC( $3  = ") + debPString($3) + ")";
                                                    $$ = new cMac(sp2s($3)); if (!$$->isValid()) yyerror("Invalid MAC."); }
        ;
mac_q   : mac                                   { $$ = new QString($1->toString()); delete $1; }
        | ARP_T                                 { $$ = new QString(_sARP); }
        | NULL_T                                { $$ = nullptr; }
        ;
mac_qq  : mac                                   { $$ = new QVariant($1->toString()); delete $1; }       // MAC mint literal
        | '<' '@'                               { $$ = new QVariant(vint(sPortIx)); }                   // MAC azonos az előző port MAC-jával
        | '<' int                               { $$ = new QVariant($2); }                              // Azonos egy megadott indexű port MAC-jával
        | ARP_T                                 { $$ = new QVariant(_sARP); }                           // Az IP címbőé kell kitalálni az "ARP"-al
        | NULL_T                                { $$ = nullptr; }                                          // A MAC NULL lessz
        ;
iptype  :                                       { $$ = AT_FIXIP;   }
        | '/' str                               { $$ = addrType(sp2s($2)); }
        ;
vlan_t  : str                                   { $$ = (qlonglong)vlanType(sp2s($1)); }
        ;
set_t   : str                                   { $$ = (qlonglong)setType(sp2s($1)); }
        |                                       { $$ = (qlonglong)ST_MANUAL; }
        ;
vlan_ids: vlan_id                               { *($$ = new QVariantList) << $1; }
        | vlan_ids ',' vlan_id                  { *($$ = $1) << $3; }
        ;
name_q  : str                                   { $$ = $1; }
        | LOOKUP_T                              { $$ = new QString(); }
        ;
osver   :                                       { $$ = new QString(); }
        | VERSION_T str                         { $$ = $2; }
        ;
np_pp   : ';'
        | '{' np_ps '}'
        ;
np_ps   : np_p np_ps
        |
        ;
np_p    : PARAM_T str '=' value ':' ':' str ';'  { node().portSetParam(svars[sPortNm], sp2s($2), vp2v($4), sp2s($7)); }
        | ADD_T ADDRESS_T ip_a str_z ';'         { portAddAddress(new QString(svars[sPortNm]), $3, $4); }
        | TAG_T str ';'                          { node().portSet(svars[sPortNm], _sPortTag, sp2s($2)); }
        | NAME_T str ';'                         { node().portSet(svars[sPortNm], _sPortName, *$2); svars[sPortNm] = sp2s($2); }
        | INDEX_T ix_z ';'                       { node().portSet(svars[sPortNm], _sPortIndex, $2); }
        | IFTYPE_T str ';'                       { changeIfType(sp2s($2)); }
        ;
arp     : ADD_T ARP_T SERVER_T ips BY_T SNMP_T COMMUNITY_T str ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::SNMP, sp2s($4), QString(), sp2s($8))); }
        | ADD_T ARP_T SERVER_T ips BY_T SSH_T PROC_T FILE_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::PROC_SSH, sp2s($4), sp2s($9))); }
        | ADD_T ARP_T SERVER_T ips BY_T SSH_T DHCPD_T CONFIG_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::DHCPD_CONF_SSH, sp2s($4), sp2s($9))); }
        | ADD_T ARP_T SERVER_T ips BY_T SSH_T DHCPD_T LEASES_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::DHCPD_LEASES_SSH, sp2s($4), sp2s($9))); }
        | ADD_T ARP_T LOCAL_T BY_T PROC_T FILE_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::PROC_LOCAL, QString(), sp2s($7))); }
        | ADD_T ARP_T LOCAL_T BY_T DHCPD_T CONFIG_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::DHCPD_CONF_LOCAL, QString(), sp2s($7))); }
        | ADD_T ARP_T LOCAL_T BY_T DHCPD_T LEASES_T str_z ';'
                { pArpServerDefs->append(qq(), cArpServerDef(cArpServerDef::DHCPD_LEASES_LOCAL, QString(), sp2s($7))); }
        | RE_T UPDATE_T ARPS_T ';'              { pArpServerDefs->updateArpTable(qq()); }
        | PING_T ha ';'                         { QProcess::execute(QString("ping"), QStringList() << "-c1" << *$2); delete $2; }
        ;
ha      : str                                   { $$ = $1; }
        | ip                                    { $$ = new QString($1->toString()); delete $1; }
        ;
link    : LINKS_T lnktype str_z TO_T lnktype str_z  { NEWOBJ(pLink, cLink($2, $3, $5, $6)); }
         '{' links '}'                              { DELOBJ(pLink); }
        ;
lnktype : BACK_T                                    { $$ = LT_BACK; }
        | FRONT_T                                   { $$ = LT_FRONT; }
        | TERM_T                                    { $$ = LT_TERM; }
        ;
lnktypez: BACK_T                                    { $$ = LT_BACK; }
        | FRONT_T                                   { $$ = LT_FRONT; }
        | TERM_T                                    { $$ = LT_TERM; }
        |                                           { $$ = LT_INVALID; }
        ;
links   : lnk
        | links lnk
        ;
lnk     : lport '&' rport alert str_z ';'           { pLink->replace($5, $4); }
        | int '*' str_z ':' int '&' str_z ':' int ';'{pLink->replace($3, $5, $7, $9, $1); }
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
alert   :                                           { $$ = nullptr; }
        /* ALERT ([<service_name> [, <host_service_descr>[, <host_service_alarm_msg>[, <node_alarm_msg>]]]]) */
        | ALERT_T '(' strs_z ')'                    { $$ = $3; }
        ;
shar    :                                           { $$ = ES_; }
        | '/' NAME_V                                { $$ = s2share($2);  }
        ;
srv     : service
        | hostsrv
        | qparse
        ;
service : SERVICE_T str str_z                   { REPOBJ(pService, cService(), REPLACE_DEF, $2, $3); }
          srvend                                { writeAndDel(pService); }
        | SERVICE_T REPLACE_T str str_z         { REPOBJ(pService, cService(), REPLACE_ON, $3, $4); }
          srvend                                { writeAndDel(pService); }
        | SERVICE_T INSERT_T str str_z          { REPOBJ(pService, cService(), REPLACE_OFF, $3, $4); }
          srvend                                { writeAndDel(pService); }
        | SERVICE_T TYPE_T replfl str str_z ';'        { cServiceType::insertNew(qq(), sp2s($4), sp2s($5), $3); }
        | MESSAGE_T str str SERVICE_T TYPE_T srvtid nsws ';'    { cAlarmMsg::replaces(qq(), $6, slp2sl($7), sp2s($2), sp2s($3)); }
//      | SERVICE_T TYPE_T srvtid MESSAGE_T  '{'    { id = $3; }    srvmsgs '}'
        | SERVICE_T VAR_T hsid replace str str_z TYPE_T srvvt_id { REPOBJ(pServiceVar, cServiceVar(), $4, $5, $6);
                                                                          pServiceVar->setId(_sServiceVarTypeId, $8);
                                                                          pServiceVar->setId(_sHostServiceId, $3); }
          '{' svars '}'                             { writeAndDel(pServiceVar); }
        | SERVICE_T VAR_T hsid replace str str_z TYPE_T srvvt_id { REPOBJ(pServiceVar, cServiceVar(), $4, $5, $6);
                                                                          pServiceVar->setId(_sServiceVarTypeId, $8);
                                                                          pServiceVar->setId(_sHostServiceId, $3); }
            ';'                                     { writeAndDel(pServiceVar); }
        | SERVICE_T VAR_T TYPE_T replace str str_z  { REPOBJ(pServiceVarType, cServiceVarType(), $4, $5, $6); }
            '{' varts '}'                           { writeAndDel(pServiceVarType); }
        ;
srvend  : '{' srv_ps '}'
        | ';'
        ;
srv_ps  : srv_p
        | srv_ps srv_p
        ;
srv_p   : SUPERIOR_T SERVICE_T MASK_T str ';'   { (*pService)[_sSuperiorServiceMask] = *$4; delete $4; }
        | COMMAND_T str  ';'                    { (*pService)[_sCheckCmd]   = *$2; delete $2; }
        | FEATURES_T features                   { (*pService)[_sFeatures] = *$2; delete $2; }
        | MAX_T CHECK_T ATTEMPTS_T int ';'      { (*pService)[_sMaxCheckAttempts]    = $4; }
        | NORMAL_T CHECK_T INTERVAL_T str ';'   { (*pService)[_sNormalCheckInterval] = *$4; delete $4; }
        | NORMAL_T CHECK_T INTERVAL_T int ';'   { (*pService)[_sNormalCheckInterval] = $4 * 1000; }
        | RETRY_T CHECK_T INTERVAL_T str ';'    { (*pService)[_sRetryCheckInterval]  = *$4; delete $4; }
        | RETRY_T CHECK_T INTERVAL_T int ';'    { (*pService)[_sRetryCheckInterval]  = $4 * 1000; }
        | FLAPPING_T INTERVAL_T str ';'         { (*pService)[_sFlappingInterval]    = *$3; delete $3; }
        | FLAPPING_T INTERVAL_T int ';'         { (*pService)[_sFlappingInterval]    = $3 * 1000; }
        | FLAPPING_T MAX_T CHANGE_T int ';'     { (*pService)[_sFlappingMaxChange]   = $4; }
        | SET_T str '=' value ';'               { (*pService)[*$2] = *$4; delete $2; delete $4; }
        | TYPE_T str ';'                        { (*pService)[_sServiceTypeId] = cServiceType().getIdByName(qq(), sp2s($2)); }
        | bool_on DISABLE_T ';'                 { (*pService)[_sDisabled] = $1; }
        | PORT_T int ';'                        { (*pService)[_sPort] = $2; }
        | TIME_T PERIODS_T tmpid ';'            { pService->set(_sTimePeriodId, $3); }
        | OFF_T LINE_T GROUPS_T grpids ';'      { pService->set(_sOffLineGroupIds, *$4); delete $4; }
        | ON_T LINE_T GROUPS_T grpids ';'       { pService->set(_sOnLineGroupIds, *$4); delete $4; }
        | HEARTBEAT_T TIME_T str ';'            { (*pService)[_sHeartbeatTime] = *$3; delete $3; }
        | HEARTBEAT_T TIME_T int ';'            { (*pService)[_sHeartbeatTime] = $3 * 1000; }
        ;
// srvmsgs : srvmsg
//        | srvmsgs srvmsg
//        ;
// srvmsg  : nsws_ ':' str str ';'                 { cAlarmMsg::replaces(qq(), id, slp2sl($1), sp2s($3), sp2s($4)); }
//         ;
hostsrv : HOST_T SERVICE_T str '.' str str_z    { NEWOBJ(pHostService, cHostService(qq(), sp2s($3), _sNul, sp2s($5), sp2s($6)));
                                                  pHostService->set(_sSuperiorHostServiceId, globalSuperiorId); }
          hsrvend                               { pHostService->insert(qq()); DELOBJ(pHostService); }
        | HOST_T SERVICE_T str ':' str '.' str str_z
                                                { NEWOBJ(pHostService, cHostService(qq(), sp2s($3), sp2s($5), sp2s($7), sp2s($8)));
                                                  pHostService->set(_sSuperiorHostServiceId, globalSuperiorId); }
          hsrvend                               { pHostService->insert(qq()); DELOBJ(pHostService); }
        ;
hsrvend : '{' hsrv_ps '}'
        | ';'
        ;
hsrv_ps : hsrv_p
        | hsrv_ps hsrv_p
        ;
hsrv_p  : PRIME_T SERVICE_T srvid ';'           { pHostService->set(_sPrimeServiceId, $3); }
        | PROTOCOL_T SERVICE_T srvid ';'        { pHostService->set(_sProtoServiceId, $3); }
        | PORT_T str ';'                        { pHostService->setPort(qq(), *$2); delete $2; }
        | DELEGATE_T HOST_T STATE_T bool_on ';' { pHostService->set(_sDelegateHostState, $4); }
        | COMMAND_T str ';'                     { pHostService->set(_sCheckCmd, *$2); delete $2; }
        | FEATURES_T features                   { pHostService->set(_sFeatures, *$2); delete $2; }
        | SUPERIOR_T SERVICE_T hsid ';'         { pHostService->set(_sSuperiorHostServiceId,  $3); }
        | MAX_T CHECK_T ATTEMPTS_T int ';'      { pHostService->set(_sMaxCheckAttempts, $4); }
        | NORMAL_T CHECK_T INTERVAL_T str ';'   { pHostService->set(_sNormalCheckInterval, sp2s($4)); }
        | NORMAL_T CHECK_T INTERVAL_T int ';'   { pHostService->set(_sNormalCheckInterval, $4 * 1000); }
        | RETRY_T CHECK_T INTERVAL_T str ';'    { pHostService->set(_sRetryCheckInterval, sp2s($4)); }
        | RETRY_T CHECK_T INTERVAL_T int ';'    { pHostService->set(_sRetryCheckInterval, $4 * 1000); }
        | FLAPPING_T INTERVAL_T str ';'         { pHostService->set(_sFlappingInterval, sp2s($3)); }
        | FLAPPING_T INTERVAL_T int ';'         { pHostService->set(_sFlappingInterval, $3 * 1000); }
        | FLAPPING_T MAX_T CHANGE_T int ';'     { pHostService->set(_sFlappingMaxChange,  $4); }
        | TIME_T PERIODS_T tmpid ';'            { pHostService->set(_sTimePeriodId, $3); }
        | OFF_T LINE_T GROUPS_T grpids ';'      { pHostService->set(_sOffLineGroupIds, *$4); delete $4; }
        | ON_T LINE_T GROUPS_T grpids ';'       { pHostService->set(_sOnLineGroupIds, *$4); delete $4; }
        | SET_T str '=' value ';'               { pHostService->set(*$2, *$4); delete $2; delete $4; }
        | bool_on DISABLE_T ';'                 { pHostService->set(_sDisabled,  $1); }
        | HEARTBEAT_T TIME_T str ';'            { (*pHostService)[_sHeartbeatTime] = *$3; delete $3; }
        | HEARTBEAT_T TIME_T int ';'            { (*pHostService)[_sHeartbeatTime] = $3 * 1000; }
        ;
// host_services rekordok előkotrása, a kifelyezés értéke a kapott rekord szám, az első rekord a megallokált pHostService2-be
// pHostService2-nek NULL-nak kell lennie. Több rekord a qq()-val kérhető be, ha el nem rontjuk a tartalmát.
fhs
// Csak az eszköz és a szervíz típus neve(minta) van megadva:  <host>.<service>
        : str '.' str                           { NEWOBJ(pHostService2, cHostService());
                                                  $$ = pHostService2->fetchByNames(qq(), sp2s($1), sp2s($3), EX_IGNORE); }
// Csak az eszköz és a szervíz típus neve van megadva:  <host>:<port>.<service>
// Ha a port nevet elhagyjuk, akkor ott NULL-t vár
        | str ':' str '.' str                 { NEWOBJ(pHostService2, cHostService());
                                                  $$ = pHostService2->fetchByNames(qq(), sp2s($1), sp2s($5), sp2s($3), EX_IGNORE); }
// Az összes azonosító megadva:  <host>:<port>.<service>(<protokol srv.>:<prime.srv.>)
// Ha a port nevet elhagyjuk, akkor ott NULL-t vár, a két utlsó paramétert elhagyva a 'nil' szolgáltatás típust jelenti
        | str ':' str '.' str '(' str_z ':' str_z ')'
                                                { NEWOBJ(pHostService2, cHostService());
                                                  $$ = pHostService2->fetchByNames(qq(), sp2s($1), sp2s($5), sp2s($3), sp2s($7), sp2s($9), EX_IGNORE); }
// A port kivételével az összes azonosító megadva:  <host>.<service>(<protokol srv.>:<prime.srv.>)
// Ha a port nevet elhagyjuk, akkor ott NULL-t vár, a két utlsó paramétert elhagyva a 'nil' szolgáltatás típust jelenti
        | str '.' str '(' str_z ':' str_z ')'
                                                { NEWOBJ(pHostService2, cHostService());
                                                  $$ = pHostService2->fetchByNames(qq(), sp2s($1), sp2s($3), _sNul, sp2s($5), sp2s($7), EX_IGNORE); }
        | int                                   { NEWOBJ(pHostService2, cHostService()); $$ = pHostService2->fetchById($1) ? 1 : 0; }
        ;
// A megadott host_services rekord ID-vel tér vissza, ha nem azonosítható be a rekord, akkor hívja yyyerror()-t.
hsid    : fhs                                   { if ($1 == 0) yyerror("Not found");
                                                  if ($1 != 1) yyerror("Ambiguous");
                                                  $$ = pHostService2->getId();  DELOBJ(pHostService2); }
        | NULL_T                                { $$ = NULL_ID; }
        ;
hss     : fhs                                   { $$ = new cHostServices(qq(), pHostService2); } // Ez nem biztos, hogy így jó!!!
        ;
hsss    : hss                                   { $$ = $1; }
        | hsss ',' hss                          { ($$ = $1)->cat($3); }
        ;
// Nincs letesztelve !!!!
varts   : vart
        | varts vart
        ;
vart    : TYPE_T str ';'                        { pServiceVarType->setId(_sParamTypeId,    cParamType().getIdByName(qq(),     *$2 ));
                                                  pServiceVarType->setId(_sRawParamTypeId, cParamType().getIdByName(qq(), sp2s($2))); }
        | TYPE_T str str ';'                    { pServiceVarType->setId(_sParamTypeId,    cParamType().getIdByName(qq(),     *$2 ));
                                                  pServiceVarType->setId(_sRawParamTypeId, cParamType().getIdByName(qq(), sp2s($2)));
                                                  pServiceVarType->setId(_sServiceVarType, serviceVarType(sp2s($3)));  }
        | TYPE_T str ',' str ';'                { pServiceVarType->setId(_sParamTypeId,    cParamType().getIdByName(qq(), sp2s($2)));
                                                  pServiceVarType->setId(_sRawParamTypeId, cParamType().getIdByName(qq(), sp2s($4))); }
        | TYPE_T str ',' str str ';'            { pServiceVarType->setId(_sParamTypeId,    cParamType().getIdByName(qq(), sp2s($2)));
                                                  pServiceVarType->setId(_sRawParamTypeId, cParamType().getIdByName(qq(), sp2s($4)));
                                                  pServiceVarType->setId(_sServiceVarType, serviceVarType(sp2s($5)));  }
        | PLAUSIBILITY_T vtfilt ';'             { pServiceVarType->setId(_sPlausibilityType, filterType($2->at(0).toString()));
                                                  pServiceVarType->set(_sPlausibilityInverse, $2->at(1));
                                                  pServiceVarType->set(_sPlausibilityParam1,  $2->at(2));
                                                  pServiceVarType->set(_sPlausibilityParam2,  $2->at(3));
                                                  delete $2; }
        | WARNING_T  vtfilt ';'                 { pServiceVarType->setId(_sWarningType, filterType($2->at(0).toString()));
                                                  pServiceVarType->set(_sWarningInverse, $2->at(1));
                                                  pServiceVarType->set(_sWarningParam1,  $2->at(2));
                                                  pServiceVarType->set(_sWarningParam2,  $2->at(3));
                                                  delete $2; }
        | CRITICAL_T  vtfilt ';'                { pServiceVarType->setId(_sCriticalType, filterType($2->at(0).toString()));
                                                  pServiceVarType->set(_sCriticalInverse, $2->at(1));
                                                  pServiceVarType->set(_sCriticalParam1,  $2->at(2));
                                                  pServiceVarType->set(_sCriticalParam2,  $2->at(3));
                                                  delete $2; }
        | FEATURES_T features                   { pServiceVarType->setName(_sFeatures, sp2s($2)); }
        | bool_on RAW_T TO_T RRD_T ';'          { pServiceVarType->setBool(_sRawToRrd, $1); }
        ;
vtfilt  : inverse str                           { *($$ = new QVariantList) << QVariant(sp2s($2)) << QVariant($1) << QVariant()         << QVariant();  }
        | inverse str value                     { *($$ = new QVariantList) << QVariant(sp2s($2)) << QVariant($1) << QVariant(vp2v($3)) << QVariant();}
        | inverse str value ',' value           { *($$ = new QVariantList) << QVariant(sp2s($2)) << QVariant($1) << QVariant(vp2v($3)) << QVariant(vp2v($5));}
        ;
inverse :                                       { $$ = false; }
        | INVERSE_T                             { $$ = true; }
        ;
svars   :
        | svar svars
        ;
svar    : FEATURES_T features                   { pServiceVar->setName(_sFeatures, sp2s($2)); }
        | VAR_T value ';'                       { pServiceVar->setName(_sServiceVarValue, pServiceVar->valToString(qq(), *$2));
                                                  pServiceVar->setName(_sRawValue,    pServiceVar->rawValToString(qq(),*$2)); delete $2; }
        | VAR_T value ',' value ';'             { pServiceVar->setName(_sServiceVarValue, pServiceVar->valToString(qq(),*$2)); delete $2;
                                                  pServiceVar->setName(_sRawValue,    pServiceVar->rawValToString(qq(), *$4)); delete $4; }
        | DELEGATE_T SERVICE_T STATE_T bool ';' { pServiceVar->setBool(_sDelegateServiceState, $4); }
        | DELEGATE_T PORT_T    STATE_T bool ';' { pServiceVar->setBool(_sDelegatePortState, $4); }
        | RAREFACTION_T int ';'                 { pServiceVar->setBool(_sRarefaction, $2); }
        ;
/*******/
tools   : tool
        | obj_tool
        ;
tool    : TOOL_T replfl str str_z '{'           { REPOBJ(pRecordAny, cRecordAny(_sTools), $2, $3, $4); }
          tool_ps '}'                           { writeAndDel(pRecordAny); }
        ;
tool_ps :
        | tool_ps tool_p
        ;
tool_p  : NOTE_T str ';'                        { pRecordAny->setNote(sp2s($2)); }
        | TYPE_T str ';'                        { pRecordAny->setName(_sToolType, sp2s($2)); }
        | COMMAND_T str ';'                     { pRecordAny->setName(_sToolStmt, sp2s($2)); }
        | FEATURES_T features                   { pRecordAny->set(_sFeatures, *$2); delete $2; }
        | OBJECTS_T strs ';'                    { pRecordAny->set(_sObjectNames, slp2sl($2)); }
        | PLATFORMS_T strs ';'                  { pRecordAny->set(_sPlatforms, slp2sl($2)); }
        | bool_on WAIT_T ';'                    { pRecordAny->set(_sWait, $1); }
        | ICON_T str ';'                        { pRecordAny->setName(_sIcon, sp2s($2)); }
        | TITLE_T str ';'                       { pRecordAny->setText(0, sp2s($2)); }
        | TOOL_T TIP_T str ';'                  { pRecordAny->setText(1, sp2s($3)); }
        ;
obj_tool: TOOL_T replfl str NODE_T str str_z    { cNode o; o.setByName(qq(), sp2s($5));
                                                  cRecordAny t(_sTools); t.setByName(qq(), sp2s($3));
                                                  pRecordAny = new cRecordAny(_sToolObjects);
                                                  pRecordAny->setId(_sToolId, t.getId());
                                                  pRecordAny->setName(_sObjectName, o.getOriginalDescr(qq())->tableName());
                                                  pRecordAny->setId(_sObjectId, o.getId());
                                                  SETNOTE(pRecordAny, $6);
                                                  setReplace($2);
                                                }
          otool_e                               { writeAndDel(pRecordAny); }
        ;
otool_e : ';'
        | '{' otool_ps '}'
        ;
otool_ps :
        | otool_ps otool_p
        ;
otool_p : NOTE_T str ';'                        { pRecordAny->setNote(sp2s($2)); }
        | FEATURES_T features                   { pRecordAny->set(_sFeatures, *$2); delete $2; }
        ;
/*******/
delete  : DELETE_T PLACE_T strs ';'             { foreach (QString s, *$3) { cPlace(). delByName(qq(), s, true); }       delete $3; }
        | DELETE_T PLACE_T GROUP_T strs ';'     { foreach (QString s, *$4) { cPlaceGroup(). delByName(qq(), s, true); }  delete $4; }
        | DELETE_T USER_T strs ';'              { foreach (QString s, *$3) { cUser().  delByName(qq(), s, true); }       delete $3; }
        | DELETE_T USER_T GROUP_T strs ';'      { foreach (QString s, *$4) { cGroup(). delByName(qq(), s, true); }       delete $4; }
        | DELETE_T GROUP_T strs ';'             { foreach (QString s, *$3) { cGroup(). delByName(qq(), s, true); }       delete $3; }
        | DELETE_T PATCH_T strs ';'             { foreach (QString s, *$3) { cPatch(). delByName(qq(), s, true, true); } delete $3; }
        | DELETE_T NODE_T  strs ';'             { foreach (QString s, *$3) { cNode().  delByName(qq(), s, true, false); }delete $3; }
        | DELETE_T PORT_T str ':' strs ';'      { foreach (QString s, *$5) { cNPort::delPortByName(qq(), *$3, s, true); }delete $3; delete $5; }
        | DELETE_T VLAN_T  strs ';'             { foreach (QString s, *$3) { cVLan().  delByName(qq(), s, true); }       delete $3; }
        | DELETE_T SUBNET_T strs ';'            { foreach (QString s, *$3) { cSubNet().delByName(qq(), s, true); }       delete $3; }
        | DELETE_T SERVICE_T strs ';'           { foreach (QString s, *$3) { cService().delByName(qq(),s, true); }       delete $3; }
        | DELETE_T SERVICE_T TYPE_T strs ';'    { foreach (QString s, *$4) { cServiceType().delByName(qq(),s, true); }   delete $4; }
        | DELETE_T HOST_T  SERVICE_T hsss ';'   { $4->remove(qq()); delete $4; }
        | DELETE_T HOST_T strs SERVICE_T strs ';'
                                                { cHostService hs;
                                                  foreach (QString h, *$3) { foreach (QString s, *$5) { hs.delByNames(qq(), h, s, true, true); }}
                                                  delete $3; delete $5;
                                                }
        | DELETE_T MACRO_T strs ';'             { foreach (QString s, *$3) { templates.del(_sMacros, s); } delete $3; }
        | DELETE_T TEMPLATE_T PATCH_T strs ';'  { foreach (QString s, *$4) { templates.del(_sPatchs, s); } delete $4; }
        | DELETE_T TEMPLATE_T NODE_T strs ';'   { foreach (QString s, *$4) { templates.del(_sNodes,  s); } delete $4; }
        | DELETE_T LINK_T lnktypez strs ';'     { foreach (QString s, *$4) { cPhsLink().unlink(qq(), s, "%", (ePhsLinkType)$3); } delete $4; }
        | DELETE_T LINK_T lnktypez str strs ';' { foreach (QString s, *$5) { cPhsLink().unlink(qq(), sp2s($4), s, (ePhsLinkType)$3); } delete $5; }
        | DELETE_T LINK_T lnktypez str int ';'  { cPhsLink().unlink(qq(), sp2s($4), (ePhsLinkType)$3, $5); }
        | DELETE_T LINK_T lnktypez str int TO_T int ';'  { cPhsLink().unlink(qq(), sp2s($4), (ePhsLinkType)$3, $5, $7); }
        | DELETE_T TABLE_T SHAPE_T strs ';'     { foreach (QString s, *$4) { cTableShape().delByName(qq(), s, true, false); } delete $4; }
        | DELETE_T ENUM_T strs ';'              { foreach (QString s, *$3) { cEnumVal().delByTypeName(qq(), s, true); } delete $3; }
        | DELETE_T ENUM_T str strs ';'          { foreach (QString s, *$4) { cEnumVal().delByNames(qq(), sp2s($3), s); } delete $4; }
        | DELETE_T GUI_T strs MENU_T ';'        { foreach (QString s, *$3) { cMenuItem().delByAppName(qq(), s, true); } delete $3; }
        | DELETE_T QUERY_T PARSER_T strs ';'    { foreach (QString s, *$4) { cQueryParser().delByServiceName(qq(), s, true); } delete $4; }
        | DELETE_T NODE_T str PORTS_T strs PARAM_T str ';'
                                                { delNodePortsParam(sp2s($3), slp2sl($5), sp2s($7)); }
        | DELETE_T NODE_T strs PARAM_T str ';'  { delNodesParam(slp2sl($3), sp2s($5)); }
        ;
scan    : SCAN_T LLDP_T snmph bool_off ';'      { scanByLldp(qq(), *$3, true, $4); delete $3; }
        | LLDP_T snmph bool_off  ';'            { lldpInfo(qq(), *$2, true, $3); delete $2; }
        | SCAN_T SNMP_T snmph SET_T ';'         { if ($3->setBySnmp()) $3->rewrite(qq());  delete $3; }
        ;
snmph   : str                                   { if (!($$ = new cSnmpDevice())->fetchByName(qq(), sp2s($1))) yyerror("ismeretlen SNMP eszköz név"); }
        ;
gui     : tblmod
        | enum
        | appmenu
        ;
tblmod_h: TABLE_T str SHAPE_T replace str str_z '{'     { pTableShape = newTableShape($2,               $5, $6); setReplace($4); }
        | TABLE_T SHAPE_T replace str str_z '{'         { pTableShape = newTableShape(new QString(*$4), $4, $5); setReplace($3); }
        | TABLE_T SHAPE_T replace str str_z  COPY_T FROM_T str '{'
                                                        { REPOBJ(pTableShape, cTableShape, $3, $4, $5); copyTableShape($8); }
        ;
tblmod  : tblmod_h tmodps '}'                           { writeAndDel(pTableShape); }
        ;
tmodps  : tmodp
        | tmodps tmodp
        ;
tmodp   : SET_T DEFAULTS_T ';'              { pTableShape->setDefaults(qq()); }
        | SET_T NO_T TREE_T DEFAULTS_T ';'  { pTableShape->setDefaults(qq(), true); }
        | SET_T str '=' value ';'           { pTableShape->set(sp2s($2), vp2v($4)); }
        | TYPE_T tstypes ';'                { pTableShape->setId( _sTableShapeType, $2); }
        | TYPE_T ON_T tstypes ';'           { pTableShape->setOn( _sTableShapeType, $3); }
        | TYPE_T OFF_T tstypes ';'          { pTableShape->setOff(_sTableShapeType, $3); }
        | STYLE_T SHEET_T str ';'           { pTableShape->setName(_sStyleSheet, sp2s($3)); }
        // title, dialog title, member title (group), not member title (group)
        | TITLE_T strs_zz  ';'              { pTableShape->setTitle(slp2sl($2)); }
        | READ_T ONLY_T bool_on ';'         { pTableShape->enum2setBool(_sTableShapeType, TS_READ_ONLY, $3); }
        | FEATURES_T features               { pTableShape->set(_sFeatures, sp2s($2)); }
        | AUTO_T REFRESH_T str ';'          { pTableShape->setName(_sAutoRefresh, sp2s($3)); }
        | AUTO_T REFRESH_T int ';'          { pTableShape->setId(  _sAutoRefresh, 1000*$3 ); }  // sec->mSec !!
        | RIGHT_T SHAPE_T strs ';'          { pTableShape->addRightShape(*$3); delete $3; }
        | REFINE_T str ';'                  { pTableShape->setName(_sRefine, sp2s($2)); }
        | INHERIT_T TYPE_T tsintyp ';'      { pTableShape->setName(_sTableInheritType, sp2s($3)); }
        | INHERIT_T TABLE_T NAMES_T strs ';'{ pTableShape->set(_sInheritTableNames, slp2vl($4)); }
        | VIEW_T RIGHTS_T rights ';'        { pTableShape->setName(_sViewRights, sp2s($3)); }
        | EDIT_T RIGHTS_T rights ';'        { pTableShape->setName(_sEditRights, sp2s($3)); }
        | DELETE_T RIGHTS_T rights ';'      { pTableShape->setName(_sRemoveRights, sp2s($3)); }
        | INSERT_T RIGHTS_T rights ';'      { pTableShape->setName(_sInsertRights, sp2s($3)); }
        | SET_T str '.' str '=' value ';'   { pTableShape->fset(sp2s($2), sp2s($4), vp2v($6)); }
        | SET_T '(' strs ')' '.' str '=' value ';'{ pTableShape->fsets(slp2sl($3), sp2s($6), vp2v($8)); }
        | FIELD_T str TITLE_T strs_zz ';'       { pTableShape->shapeFields.get(sp2s($2))->setTitle(slp2sl($4)); }
        | FIELD_T str NOTE_T str ';'            { pTableShape->fset(sp2s($2),_sTableShapeFieldNote, sp2s($4)); }
        | FIELD_T strs VIEW_T RIGHTS_T rights ';'{pTableShape->fsets(slp2sl($2), _sViewRights, sp2s($5)); }
        | FIELD_T strs EDIT_T RIGHTS_T rights ';'{pTableShape->fsets(slp2sl($2), _sEditRights, sp2s($5)); }
        | FIELD_T strs FEATURES_T features      { pTableShape->fsets(slp2sl($2), _sFeatures, sp2s($4)); }
        | FIELD_T strs FLAG_T fflags ';'        { pTableShape->fsets(slp2sl($2), _sFieldFlags, $4); }
        | FIELD_T strs FLAG_T bool_ fflags ';'  { foreach (QString fn, *$2) {
                                                      cTableShapeField *pTS = pTableShape->shapeFields.get(fn);
                                                      qlonglong f = pTS->getBigInt(_sFieldFlags);
                                                      if ($4) f |=  $5;
                                                      else    f &= ~$5;
                                                      pTS->setId(_sFieldFlags, f);
                                                  }
                                                  delete $2;
                                                }
        | FIELD_T strs READ_T ONLY_T bool_on ';'{ foreach (QString fn, *$2) {
                                                    pTableShape->shapeFields.get(fn)->enum2setBool(_sFieldFlags, FF_READ_ONLY, $5);
                                                  }
                                                  delete $2;
                                                }
        | FIELD_T str DEFAULT_T VALUE_T str ';' { pTableShape->fset(sp2s($2), _sDefaultValue, sp2s($5)); }
        | FIELD_T str TOOL_T TIP_T str ';'      { pTableShape->shapeFields.get(sp2s($2))->setText(cTableShapeField::LTX_TOOL_TIP,   sp2s($5)); }
        | FIELD_T str WHATS_T THIS_T str ';'    { pTableShape->shapeFields.get(sp2s($2))->setText(cTableShapeField::LTX_WHATS_THIS, sp2s($5)); }
        | FIELD_T SEQUENCE_T int0 strs ';'      { pTableShape->setFieldSeq(slp2sl($4), $3); }
        | FIELD_T strs ORD_T strs ';'           { pTableShape->setOrders(*$2, *$4); delete $2; delete $4; }
        | FIELD_T '*'  ORD_T strs ';'           { pTableShape->setAllOrders(*$4); delete $4; }
        | FIELD_T ORD_T SEQUENCE_T int0 strs ';'{ pTableShape->setOrdSeq(slp2sl($5), $4); }
        | ADD_T FIELD_T as str_z ';'            { pTableShape->addField($3->first, $3->second, sp2s($4)); delete $3; }
        | ADD_T FIELD_T as str_z '{'            { pTableShapeField = pTableShape->addField($3->first, $3->second, sp2s($4)); delete $3; }
            fmodps '}'                          { pTableShapeField = nullptr; }
        | FIELD_T str '{'                       { pTableShapeField = pTableShape->shapeFields.get(sp2s($2)); }
            fmodps '}'                          { pTableShapeField = nullptr; }
        ;
as      : str AS_T str                          { $$ = new tStringPair(sp2s($3), sp2s($1)); }
        | str                                   { $$ = new tStringPair(*$1, *$1); delete $1; }
        ;
tstypes : tstype                                { $$ = $1; }
        | tstypes ',' tstype                    { $$ = $1 | $3; }
        ;
tstype  : str                                   { $$ = ENUM2SET(tableShapeType(sp2s($1))); }
        ;
tsintyp : str                           { $$ = $1; cTableShape().descr()[_sTableInheritType].check(*$1, cColStaticDescr::VC_OK); }
        ;
int0    : int                           { $$ = $1; }
        |                               { $$ = 0; }
        ;
tmod    : str                           { $$ = cTableShape().getIdByName(sp2s($1)); }
        ;
ftmod   : str '.' str                   { $$ = cTableShapeField::getIdByNames(qq(), sp2s($1), sp2s($3)); }
        ;
fflags  : fflag                         { $$ = $1; }
        | fflags ',' fflag              { $$ = $1 | $3; }
        ;
fflag   : str                           { $$ = ENUM2SET(fieldFlag(sp2s($1))); }
        ;
fmodps  : fmodp
        | fmodps fmodp
        ;
fmodp   : SET_T str '=' value ';'       { pTableShapeField->set(sp2s($2), vp2v($4)); }
        | TITLE_T strs_zz ';'           { pTableShapeField->setTitle(slp2sl($2)); }
        | NOTE_T str ';'                { pTableShapeField->setName(_sTableShapeFieldNote, sp2s($2)); }
        | VIEW_T RIGHTS_T rights ';'    { pTableShapeField->setName(_sViewRights, sp2s($3)); }
        | EDIT_T RIGHTS_T rights ';'    { pTableShapeField->setName(_sEditRights, sp2s($3)); }
        | FEATURES_T features           { pTableShapeField->setName(_sFeatures, sp2s($2)); }
        | FLAG_T fflags ';'             { pTableShapeField->setId(_sFieldFlags, $2); }
        | FLAG_T ON_T fflags ';'        { pTableShapeField->setOn(_sFieldFlags, $3); }
        | FLAG_T OFF_T fflags ';'       { pTableShapeField->setOff(_sFieldFlags, $3); }
        | DEFAULT_T VALUE_T str ';'     { pTableShapeField->setName(_sDefaultValue, sp2s($3)); }
        | TOOL_T TIP_T str ';'          { pTableShapeField->setText(cTableShapeField::LTX_TOOL_TIP, sp2s($3)); }
        | WHATS_T THIS_T str ';'        { pTableShapeField->setText(cTableShapeField::LTX_WHATS_THIS, sp2s($3)); }
        ;
enum    : ENUM_T replace  str str_z '{' { newEnum(sp2s($3), nullptr, $2); SETNOTE(pActEnum, $4); }
            enumps                      { saveEnum(); }
            enumvals
          '}'
        ;
enumps  :
        | enumps enump
        ;
enump   : BACKGROUND_T COLOR_T str ';'  { actEnum().setName(_sBgColor, sp2s($3)); }
        | FOREGROUND_T COLOR_T str ';'  { actEnum().setName(_sFgColor, sp2s($3)); }
        | FONT_T FAMILY_T str ';'       { actEnum().setName(_sFontFamily, sp2s($3)); }
        | FONT_T ATTR_T strs ';'        { actEnum().addStringList(_sFontAttr, slp2sl($3)); }
        | VIEW_T strs_zz ';'            { actEnum().setView(slp2sl($2)); }
        | TOOL_T TIP_T str ';'          { actEnum().setText(cEnumVal::LTX_TOOL_TIP, sp2s($3)); }
        | ICON_T str ';'                { actEnum().setName(_sIcon, sp2s($2)); }
        ;
enumvals: enumval
        | enumval enumvals
        ;
enumval : str str_z '{'                 { newEnum(actEnumType, $1); SETNOTE(pActEnum, $2); }
            enumps '}'                  { saveEnum(); }
        ;
appmenu : GUI_T str                     { pMenuApp = $2;     if (!menuItems.isEmpty()) EXCEPTION(EPROGFAIL, menuItems.size(), menuItems.toString()); }
            '{' menus '}'               { pDelete(pMenuApp); if (!menuItems.isEmpty()) EXCEPTION(EPROGFAIL, menuItems.size(), menuItems.toString()); }
        ;
menus   : menu
        | menu menus
        ;
menu    : MENU_T str str_z              { newMenuMenu(sp2s($2), $3); }
            '{' miops                   { saveMenuItem(); }
                menus '}'               { delMenuItem(); }
        | SHAPE_T str str_z             { newMenuItem(sp2s($2), $3, MT_SHAPE); }
            '{' miops '}'               { delMenuItem(); }
        | EXEC_T str str_z              { newMenuItem(sp2s($2), $3, MT_EXEC); }
            '{' miops '}'               { delMenuItem(); }
        | OWN_T str str_z               { newMenuItem(sp2s($2), $3, MT_OWN); }
            '{' miops '}'               { delMenuItem(); }
        ;
miops   : miop miops
        |
        ;
miop    : PARAM_T str ';'               { actMenuItem().setName(cMenuItem::ixMenuParam(), sp2s($2)); }
        | FEATURES_T features           { actMenuItem().setName(cMenuItem::ixFeatures(), sp2s($2)); }
        | TITLE_T strs_zz ';'           { actMenuItem().setTitle(slp2sl($2)); }
        | TOOL_T TIP_T str ';'          { actMenuItem().setText(cMenuItem::LTX_TOOL_TIP, sp2s($3)); }
        | WHATS_T THIS_T str ';'        { actMenuItem().setText(cMenuItem::LTX_WHATS_THIS, sp2s($3)); }
        | RIGHTS_T rights ';'           { actMenuItem().setName(_sMenuRights, sp2s($2)); }
        ;
// Névvel azonosítható rekord egy mezőjének a modosítása az adatbázisban:
// SET <tábla név>[<modosítandó mező neve>].<rekordot azonosító név> = <új érték>;
modify  : SET_T str '[' strs ']' '.' str '=' value ';'
                { cRecordAny::updateFieldByNames(qq(), cRecStaticDescr::get(sp2s($2)), slp2sl($4), sp2s($7), vp2v($9)); }
        | SET_T str '.' str '[' strs ']' '.' str '=' value ';'
                { cRecordAny::updateFieldByNames(qq(), cRecStaticDescr::get(sp2s($4), sp2s($2)), slp2sl($6), sp2s($9), vp2v($11)); }
        | SET_T str '[' strs ']' '.' str '+' '=' value ';'
                { cRecordAny::addValueArrayFieldByNames(qq(), cRecStaticDescr::get(sp2s($2)), slp2sl($4), sp2s($7), vp2v($10)); }
        | SET_T str '.' str '[' strs ']' '.' str '+' '=' value ';'
                { cRecordAny::addValueArrayFieldByNames(qq(), cRecStaticDescr::get(sp2s($4), sp2s($2)), slp2sl($6), sp2s($9), vp2v($12)); }
        | SET_T IFTYPE_T str NAME_T PREFIX_T str ';'
                 { cIfType().setByName(qq(), sp2s($3)).setName(_sIfNamePrefix, sp2s($6)).update(qq(), false);
                   lanView::resetCacheData(); }
        | SET_T ALERT_T SERVICE_T srvid ';'     { alertServiceId = $4; }
        | SET_T SUPERIOR_T hsid TO_T hsss ';'   { $5->sets(_sSuperiorHostServiceId, $3); delete $5; }
        | SET_T NODE_T str SERIAL_T NUMBER_T str ';'    { cNode::setSeralNumber(qq(), sp2s($3), sp2s($6)); }
        | SET_T NODE_T str INVENTORY_T NUMBER_T str ';' { cNode::setInventoryNumber(qq(), sp2s($3), sp2s($6)); }
        | SET_T NODE_T str PORTS_T strs PARAM_T str str '=' value ';'
                                                { setNodePortsParam(sp2s($3), slp2sl($5), sp2s($7), sp2s($8), vp2v($10)); }
        | SET_T NODE_T strs PARAM_T str str '=' value ';'
                                                { setNodeParam(slp2sl($3), sp2s($5), sp2s($6), vp2v($8)); }
        | SET_T PLACE_T place_id NODE_T strs ';'{ setNodePlace(slp2sl($5), $3); }
        | SET_T PLACE_T place_id ';'            { globalPlaceId = $3; }
        | CLEAR_T PLACE_T ';'                   { globalPlaceId = NULL_ID; }
        | SET_T SUPERIOR_T hsid ';'             { globalSuperiorId = $3; }
        | CLEAR_T SUPERIOR_T ';'                { globalSuperiorId = NULL_ID; }
        // Felhasználó letiltása
        | DISABLE_T USER_T str ';'              { cUser().setByName(qq(), sp2s($3)).setBool(_sDisabled, true).update(qq(), true); }
        | ENABLE_T  USER_T str ';'              { cUser().setByName(qq(), sp2s($3)).setBool(_sDisabled, false).update(qq(), true); }
        // Összes csoporttag letiltása/engedélyezése
        | DISABLE_T USER_T str GROUP_T ';'      { tGroupUser().disableMemberByGroup(qq(), sp2s($3), true); }
        | ENABLE_T  USER_T str GROUP_T ';'      { tGroupUser().disableMemberByGroup(qq(), sp2s($3), false); }
        | DISABLE_T SERVICE_T str ';'           { cService().setByName(qq(), sp2s($3)).setBool(_sDisabled, true).update(qq(), false); }
        | ENABLE_T  SERVICE_T str ';'           { cService().setByName(qq(), sp2s($3)).setBool(_sDisabled, false).update(qq(), false); }
        | DISABLE_T HOST_T SERVICE_T hsid ';'   { cHostService().setById(qq(), $4).setBool(_sDisabled, true).update(qq(), false); }
        | ENABLE_T  HOST_T SERVICE_T hsid ';'   { cHostService().setById(qq(), $4).setBool(_sDisabled, false).update(qq(), false); }
        | SET_T REPLACE_T ';'                   { globalReplaceFlag = true; }
        | SET_T INSERT_T ';'                    { globalReplaceFlag = false; }
        | SET_T HOST_T SERVICE_T hsid VAR_T strs '=' vals ';'    { ivars[_sState] = cInspectorVar::setValues(qq(), $4, slp2sl($6), vlp2vl($8)); }
        | SET_T HOST_T SERVICE_T hsid STATE_T int str_z ';'      { pHostService = new cHostService();
                                                                   pHostService->setById(qq(), $4);
                                                                   pHostService->setState(qq(), notifSwitch($6), sp2s($7));
                                                                   pDelete(pHostService);
                                                                 }
        ;
if      : IFDEF_T  ifdef                { bSkeep = !$2; }
        | IFNDEF_T ifdef                { bSkeep =  $2; }
        | IF_T '(' bool ')'             { bSkeep = !$3; }
        ;
ifdef   : PLACE_T str                   { $$ = NULL_ID != cPlace().     getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | PLACE_T GROUP_T str           { $$ = NULL_ID != cPlaceGroup().getIdByName(qq(), sp2s($3), EX_IGNORE); }
        | USER_T str                    { $$ = NULL_ID != cUser().      getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | USER_T GROUP_T str            { $$ = NULL_ID != cGroup().     getIdByName(qq(), sp2s($3), EX_IGNORE); }
        | GROUP_T str                   { $$ = NULL_ID != cGroup().     getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | PATCH_T str                   { $$ = NULL_ID != cPatch().     getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | NODE_T  str                   { $$ = NULL_ID != cNode().      getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | PORT_T str ':' str            { $$ = NULL_ID != cNPort(). getPortIdByName(qq(), sp2s($4), sp2s($2), EX_IGNORE); }
        | VLAN_T  str                   { $$ = NULL_ID != cVLan().      getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | SUBNET_T str                  { $$ = NULL_ID != cSubNet().    getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | TABLE_T SHAPE_T str           { $$ = NULL_ID != cTableShape().getIdByName(qq(), sp2s($3), EX_IGNORE); }
        | SERVICE_T str                 { $$ = NULL_ID != cService().   getIdByName(qq(), sp2s($2), EX_IGNORE); }
        | HOST_T  SERVICE_T fhs         { $$ = $3 != 0; pDelete(pHostService2); }
        | MACRO_T str ';'               { $$ = !templates[_sMacros].get(qq(), sp2s($2), EX_IGNORE).isEmpty(); }
        | TEMPLATE_T PATCH_T str ';'    { $$ = !templates[_sPatchs].get(qq(), sp2s($3), EX_IGNORE).isEmpty(); }
        | TEMPLATE_T NODE_T str ';'     { $$ = !templates[_sNodes]. get(qq(), sp2s($3), EX_IGNORE).isEmpty(); }
        ;
qparse  : QUERY_T PARSER_T srvid '{'    { ivars[_sServiceId] = $3; ivars[_sItemSequenceNumber] = 10; }
            qparis '}'
        ;
qparis  : qpari
        | qparis qpari
        ;
qpari   : TEMPLATE_T reattr str str str_z ';'   { cQueryParser::_insert(qq(), ivars[_sServiceId], _sParse, $2, sp2s($3), sp2s($4), sp2s($5), ivars[_sItemSequenceNumber]);
                                      ivars[_sItemSequenceNumber] += 10; }
        | PREP_T str str_z ';'      { cQueryParser::_insert(qq(), ivars[_sServiceId], _sPrep, false, _sNul, sp2s($2), sp2s($3), NULL_ID); }
        | POST_T str str_z ';'      { cQueryParser::_insert(qq(), ivars[_sServiceId], _sPost, false, _sNul, sp2s($2), sp2s($3), NULL_ID); }
        ;
reattr  : ATTR_T '(' strsz ')'      { $$ = 0; foreach (QString s, *$3) { $$ |= ENUM2SET(regexpAttr(s)); } delete $3; }
        |                           { $$ = ENUM2SET(RA_EXACTMATCH); }
        ;
replaces: iprange
        | reparp
        ;
iprange : REPLACE_T DYNAMIC_T ADDRESS_T RANGE_T exclude ip TO_T ip hsid ';'
            { cDynAddrRange::replace(qq(), *$6, *$8, $9, $5); delete $6; delete $8; }
        ;
exclude : EXCLUDE_T                 { $$ = true;  }
        |                           { $$ = false; }
        ;
reparp  : REPLACE_T ARP_T ip mac str hsid str_z ';'
            { cArp arp;
              arp.setIp(_sIpAddress, *$3).setMac(_sHwAddress, *$4).setName(_sSetType, sp2s($5)).setId(_sHostServiceId, $6).setName(_sArpNote, sp2s($7));
              arp.replace(qq());
              delete $3; delete $4; }
        ;
exports : print
        | lang
        ;
print   : PRINT_T str ';'               { cExportQueue::push(*$2); delete $2; }
        | EXPORT_T SERVICE_T ';'        { cExportQueue::push(cExport().Services()); }
        | PRINT_T ALL_T TOKEN_T ';'     { cExportQueue::push(getAllTokens(true)); }
        ;
lang    : LANG_T replfl str str str image_idz lnx ';'  { if ($2) cLanguage::repLanguage(qq(), sp2s($3), sp2s($4), sp2s($5), $6, $7);
                                                         else    cLanguage::newLanguage(qq(), sp2s($3), sp2s($4), sp2s($5), $6, $7);
                                                       }
        ;
lnx     : NEXT_T str                    { $$ = cLanguage().getIdByName(qq(), sp2s($2)); }
        |                               { $$ = NULL_ID; }
        ;
snmp    : SNMP_T snmph GET_T  coid ';'  { snmpGet($2, $4); }
        | SNMP_T snmph NEXT_T coid ';'  { snmpNext($2, $4); }
        | SNMP_T snmph WALK_T coid ';'  { snmpWalk($2, $4); }
        | OID_T PRINT_T coid ';'        { printOId($3); }
        ;
coid    : str                           { $$ = newOId($1); }
        | int                           { $$ = addOId(newOId(nullptr), $1); }
        | coid '.' int                  { $$ = addOId($1, $3); }
        ;
%%

static void set_debug(bool f)
{
    yydebug = f ? 1 : 0;
}

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

/// LEX: Idézőjeles string beolvasása
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

/// LEX:  $$ jeles string beolvasása
/// @param mn A $ jelek közti név ami a string végét jelzi
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
                *ps += QChar('$');  // A $ jel
                *ps += c;           // A már nem ok. karakter
                if (i) *ps += mn.mid(0, i); // Meg amit eddig "felismertünk"
                i = -1;  // Nincs találat, ez tuti nem egyenlő mn.size() -vel.
                break;
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
    // (VVERBOSE) << ee << QChar(' ') << *ps;
    delete ps;
    yyerror(ee);    // az yyerror() olyan mintha visszatérne, pedig dehogy.
    return nullptr;
}

/// LEX: Cím típusú adat beolvasása
static int isAddress(const QString& __s)
{
    //_DBGFN() << " @(" << __s << ") macbuff = " << macbuff << endl;
    if (__s.indexOf("0x", 0, Qt::CaseInsensitive) ==  0) return 0;
    if (__s.contains(QRegularExpression("[g-zG-Z_]"))) return 0;
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
        // PDEB(VVERBOSE) << "INTEGER : " << as << endl;
        return INTEGER_V;
    }
    // MAC ?
    yylval.mac = new cMac(as);
    if (yylval.mac->isValid()) {
        // PDEB(VVERBOSE) << "MAC : " << as << endl;
        return MAC_V;
    }
    delete yylval.mac;
    // IP ADDRESS ?
    int n = as.count(QChar(':'));
    if (n > 2 || (n > 1 && as.contains("::"))) {
        yylval.ip = new QHostAddress();
        if (yylval.ip->setAddress(as)) {
            // PDEB(VVERBOSE) << "IPV6 : " << as << endl;
            return IPV6_V;
        }
        delete yylval.ip;
    }
    // Egyik sem
    if (__s.size() < as.size()) macbuff.insert(0, as.right(as.size() - __s.size()));    // yyunget
    // _DBGFNL() << " __s = " << __s << ", macbuff = " << macbuff << endl;
    return 0;
}

// Egy karakteres tokenek
static const char yyTokenChars[] = "=+-*(),;|&<>^{}[]:.#@~";
#define TOK(t)  { #t, t##_T },
// Tokenek
static const struct token {
    const char *name;
    int         value;
} yyTokenList[] = {
    TOK(TEST)
    TOK(MACRO) TOK(FOR) TOK(DO) TOK(TO) TOK(SET) TOK(CLEAR) TOK(OR) TOK(AND) TOK(NOT) TOK(DEFINED)
    TOK(VLAN) TOK(SUBNET) TOK(PORTS) TOK(PORT) TOK(NAME) TOK(SHARED) TOK(SENSORS)
    TOK(PLACE) TOK(PATCH) TOK(SWITCH) TOK(NODE) TOK(HOST) TOK(ADDRESS) TOK(ICON)
    TOK(PARENT) TOK(IMAGE) TOK(FRAME) TOK(TEL) TOK(NOTE) TOK(MESSAGE) TOK(LEASES)
    TOK(PARAM) TOK(TEMPLATE) TOK(COPY) TOK(FROM) TOK(NULL) TOK(TERM) TOK(NEXT)
    TOK(INCLUDE) TOK(PSEUDO) TOK(OFFS) TOK(IFTYPE) TOK(WRITE) TOK(RE) TOK(SYS)
    TOK(ADD) TOK(READ) TOK(UPDATE) TOK(ARPS) TOK(ARP) TOK(SERVER) TOK(FILE) TOK(BY)
    TOK(SNMP) TOK(SSH) TOK(COMMUNITY) TOK(DHCPD) TOK(LOCAL) TOK(PROC) TOK(CONFIG)
    TOK(ATTACHED) TOK(LOOKUP) TOK(WORKSTATION) TOK(LINKS) TOK(BACK) TOK(FRONT)
    TOK(IP) TOK(COMMAND) TOK(SERVICE) TOK(PRIME) TOK(PRINT) TOK(EXPORT)
    TOK(MAX) TOK(CHECK) TOK(ATTEMPTS) TOK(NORMAL) TOK(INTERVAL) TOK(RETRY)
    TOK(FLAPPING) TOK(CHANGE) { "TRUE", TRUE_T },{ "FALSE", FALSE_T }, TOK(ON) TOK(OFF) TOK(YES) TOK(NO)
    TOK(DELEGATE) TOK(STATE) TOK(SUPERIOR) TOK(TIME) TOK(PERIODS) TOK(LINE) TOK(GROUP)
    TOK(USER) TOK(DAY) TOK(OF) TOK(PERIOD) TOK(PROTOCOL) TOK(ALERT) TOK(INTEGER) TOK(FLOAT)
    TOK(DELETE) TOK(ONLY) TOK(STRING) TOK(SAVE) TOK(TYPE) TOK(STEP) TOK(EXCEPT)
    TOK(MASK) TOK(LIST) TOK(VLANS) TOK(ID) TOK(DYNAMIC) TOK(PRIVATE) TOK(PING)
    TOK(NOTIF) TOK(ALL) TOK(RIGHTS) TOK(REMOVE) TOK(SUB) TOK(FEATURES) TOK(MAC)
    TOK(LINK) TOK(LLDP) TOK(SCAN) TOK(TABLE) TOK(FIELD) TOK(SHAPE) TOK(TITLE) TOK(REFINE)
    TOK(DEFAULTS) TOK(ENUM) TOK(RIGHT) TOK(VIEW) TOK(INSERT) TOK(EDIT) TOK(LANG)
    TOK(INHERIT) TOK(NAMES) TOK(VALUE) TOK(DEFAULT) TOK(STYLE) TOK(SHEET)
    TOK(ORD) TOK(SEQUENCE) TOK(MENU) TOK(GUI) TOK(OWN) TOK(TOOL) TOK(TIP) TOK(WHATS) TOK(THIS)
    TOK(EXEC) TOK(TAG) TOK(ENABLE) TOK(SERIAL) TOK(INVENTORY) TOK(NUMBER)
    TOK(DISABLE) TOK(PREFIX) TOK(RESET) TOK(CACHE) TOK(INVERSE) TOK(RAREFACTION)
    TOK(DATA) TOK(IANA) TOK(IFDEF) TOK(IFNDEF) TOK(NC) TOK(QUERY) TOK(PARSER) TOK(IF)
    TOK(REPLACE) TOK(RANGE) TOK(EXCLUDE) TOK(PREP) TOK(POST) TOK(CASE) TOK(RECTANGLE)
    TOK(DELETED) TOK(PARAMS) TOK(DOMAIN) TOK(VAR) TOK(PLAUSIBILITY) TOK(CRITICAL)
    TOK(AUTO) TOK(FLAG) TOK(TREE) TOK(NOTIFY) TOK(WARNING) TOK(PREFERED) TOK(RAW)  TOK(RRD)
    TOK(REFRESH) TOK(SQL) TOK(CATEGORY) TOK(ZONE) TOK(HEARTBEAT) TOK(GROUPS) TOK(AS)
    TOK(TOKEN) TOK(COLOR) TOK(BACKGROUND) TOK(FOREGROUND) TOK(FONT) TOK(ATTR) TOK(FAMILY)
    TOK(OS) TOK(VERSION) TOK(INDEX) TOK(GET) TOK(WALK) TOK(OID) TOK(GLPI) TOK(SYNC) TOK(LIKE)
    TOK(OBJECTS) TOK(PLATFORMS) TOK(WAIT) TOK(LOWER) TOK(UPPER)
    { "WST",    WORKSTATION_T }, // rövidítések
    { "ATC",    ATTACHED_T },
    { "INT",    INTEGER_T },
    { "MSG",    MESSAGE_T },
    { "CMD",    COMMAND_T },
    { "PROTO",  PROTOCOL_T },
    { "SEQ",    SEQUENCE_T },
    { "DEL",    DELETE_T },
    { "CAT",    CATEGORY_T },
    { "FG",     FOREGROUND_T },
    { "BG",     BACKGROUND_T },
    { "OBJ",    OBJECTS_T },
    TOK(END) TOK(ELSE)
    { nullptr, 0 }
};

static int _yylex(void)
{
    // DBGFN();
recall:
    yylval.i = 0;
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
            if (c2 == QChar('*')) { // comment '/*'
                find_comment_end:
                while (!(c = yyget()).isNull() && c != QChar('*'));
                if (c.isNull() || (c = yyget()).isNull()) yyerror("EOF in commment.");
                if (c == QChar('/')) continue;  // comment végét megtaláltuk
                goto find_comment_end;          // keressük tövább
            }
            if (!c2.isNull()) yyunget(c2);  // A feleslegesen beolvasott karaktert vissza
            // PDEB(VVERBOSE) << "C TOKEN : /" << endl;
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
            // PDEB(VVERBOSE) << "ylex : $$ STRING : " << *(yylval.s) << endl;
            return STRING_V;
        }
        if (!c.isNull() && c.isSpace()) c = yyget();
        QStringList    parms;
        // PDEB(VVERBOSE) << "yylex: exec macro: " << mn << endl;
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
            // PDEB(VVERBOSE) << "Macro params : " << toString(parms);
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
        // PDEB(VVERBOSE) << "Macro Body : \"" << mm << "\"" << endl;
        insertCode(mm);
        goto recall;
    }
    // VALUE Numeric, address
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
            // PDEB(VVERBOSE) << "ylex : FLOAT : " << yylval.r << endl;
            return FLOAT_V;
        case 3:  // IP
            yylval.ip = new QHostAddress();
            if (!yylval.ip->setAddress(sn)) {
                yyerror("Invalid IP number.");
            }
            // PDEB(VVERBOSE) << "ylex : IPV4 : " << yylval.ip->toString() << endl;
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
        // PDEB(VVERBOSE) << "ylex : INTEGER : " << yylval.i << endl;
        return INTEGER_V;
    }
    // VALUE STRING
    if (c == QChar('\"')) {
        yylval.s = yygetstr();
        // PDEB(VVERBOSE) << "ylex : STRING : " << *(yylval.s) << endl;
        return STRING_V;
    }
    // Egybetus tokenek
    if (strchr(yyTokenChars, c.toLatin1())) {
        int r;
        if (c == QChar(':') && 0 != (r = isAddress(":"))) return r;
        int ct = c.toLatin1();
        // PDEB(VVERBOSE) << "ylex : char token : '" << c << "' (" <<  ct << QChar(')') << endl;
        return ct;
    }
    QString *sp = new QString();
    while (c.isLetterOrNumber() || c == QChar('_')) {
        *sp += c;
        c = yyget();
    }
    int r;
    if (!c.isNull()) yyunget(c);
    if (sp->isEmpty()) yyerror("Invalid character");
    for (const struct token *p = yyTokenList; p->name; p++) {
        if (p->name == *sp) {
            // PDEB(VVERBOSE) << "ylex TOKEN : " << *sp << endl;
            delete sp;
            return p->value;
        }
    }
    if (0 != (r = isAddress(*sp))) return r;
    yylval.s = sp;
    // PDEB(VVERBOSE) << "ylex : NAME : " << *sp << endl;
    return NAME_V;
}

static int yylex(void)
{
    while (true) {
        int r = _yylex();
        switch(r) {
        case 0:
            return r;
        case END_T:
            bSkeep = false;
            continue;
        case ELSE_T:
            bSkeep = !bSkeep;
            continue;
        default:
            break;
        }
        if (bSkeep) continue;
        return r;
    }
}

/* */

static QString getAllTokens(bool sort)
{
    QStringList r;
    for (const struct token *p = yyTokenList; p->name; p++) {
        r << QString(p->name);
    }
    if (sort) r.sort();
    QChar sep = ' ';
    return r.join(sep);
}

static QString * strReplace(QString * s, QString * src, QString *trg)
{
    QString r = *s;
    delete s;
    QRegularExpression re(*src);
    if (!re.isValid()) yyerror(QObject::tr("Invalid regexp : %1").arg(*src));
    delete src;
    r.replace(re, *trg);
    delete trg;
    return new QString(r);
}

static void forLoopMac(QString *_in, QVariantList *_lst, int step)
{
    QString s;
    int n = _lst->size();
    for (int i = 0; (i + step -1) < n; i += step) {
        s += QChar('$') + *_in + QChar('(');
        s += quotedVariantList(_lst->mid(i, step));
        s += QChar(')');
    }
    // PDEB(VVERBOSE) << "forLoopMac inserted : " << dQuoted(s) << endl;
    insertCode(s);
    delete _in; delete _lst;
}

static void forLoop(QString *_in, QVariantList *_lst, int step)
{
    QString *pmm = new QString("__");
    templates.set (_sMacros, *pmm, *_in);
    delete _in;
    forLoopMac(pmm, _lst, step);
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

