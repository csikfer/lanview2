#include <math.h>
#include "import.h"
#include "others.h"
#include "ping.h"
#include "import_parser.h"


int sImportParse(QString& text)
{
    fileNm = "[stream]";
    yyif = new QTextStream(&text);
    PDEB(INFO) << "Start parser ..." << endl;
    int r = yyparse();
    PDEB(INFO) << "End parser." << endl;
    pDelete(yyif);
    return r;
}

int fImportParse(const QString& fn)
{
    fileNm = fn;
    if (fileNm == _sMinus || fileNm == _sStdin) {
        yyif = new QTextStream(stdin, QIODevice::ReadOnly);
    }
    else {
        QFile in(fileNm);
        if (!srcOpen(in)) EXCEPTION(EFOPEN, -1, in.fileName());
        yyif = new QTextStream(&in);
    }
    PDEB(INFO) << "Start parser ..." << endl;
    int r = yyparse();
    PDEB(INFO) << "End parser." << endl;
    return r;
}

bool srcOpen(QFile& f)
{
    if (f.exists()) return f.open(QIODevice::ReadOnly);
    QString fn = f.fileName();
    QDir    d(lanView::getInstance()->homeDir);
    if (d.exists(fn)) {
        f.setFileName(d.filePath(fn));
        return f.open(QIODevice::ReadOnly);
    }
    EXCEPTION(ENOTFILE, -1, fn);
    return false;   // A fordító szerint ez kell
}

/* ---------------------------------------------------------------------------------------------------- */

QStack<c_yyFile> c_yyFile::stack;
void c_yyFile::inc(QString *__f)
{
    c_yyFile    o;
    o.newFile = new QFile(*__f);
    if (!srcOpen(*o.newFile)) yyerror(QObject::trUtf8("include: file open error."));
    o.oldStream = yyif;
    yyif = new QTextStream(o.newFile);
    o.oldFileName = fileNm;
    fileNm = *__f;
    delete __f;
    o.oldPos = lineNo;
    lineNo = 0;
    stack.push(o);
}

void c_yyFile::eoi()
{
    c_yyFile    o = stack.pop();
    delete yyif;
    delete o.newFile;
    yyif = o.oldStream;
    fileNm = o.oldFileName;
    lineNo = o.oldPos;
}


/* ---------------------------------------------------------------------------------------------------- */

QStringList     allNotifSwitchs;
qlonglong       globalPlaceId = NULL_ID; // ID of none
cArpServerDefs *pArpServerDefs = NULL;

qlonglong newAttachedNode(QSqlQuery &q, const QString& __n, const QString& __d)
{
    cNode   node;
    node.asmbAttached(__n, __d, gPlace());
    node.insert(q, true);
    return node.ports.first()->getId();
}

void newAttachedNodes(QSqlQuery &q, const QString& __np, const QString& __dp, int __from, int __to)
{
    QString name, descr;
    for (int i = __from; i <= __to; i++) {
        name  = nameAndNumber(__np, i);
        descr = nameAndNumber(__dp, i);
        newAttachedNode(q, name, descr);
    }
}

qlonglong newWorkstation(QSqlQuery& q, const QString& __n, const cMac& __mac, const QString& __d)
{
    cNode host;
    host.asmbWorkstation(q, __n, __mac, __d, gPlace());
    host.insert(q, true);
    return host.ports.first()->getId();
}

/* ************************************************************************************************ */
bool cArpServerDef::firstTime = true;
void cArpServerDef::updateArpTable(QSqlQuery& q) const
{
    DBGFN();
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
}

/* --------------------------------------------------------------------------------------------------- */

cLink::cLink(QSqlQuery& q, int __t1, QString *__n1, int __t2, QString *__n2)
    : phsLinkType1(i2LinkType(__t1))
    , phsLinkType2(i2LinkType(__t2))
{
    nodeId1 = __n1->isEmpty() ? NULL_ID : cPatch::getNodeIdByName(q, *__n1);
    nodeId2 = __n2->isEmpty() ? NULL_ID : cPatch::getNodeIdByName(q, *__n2);
    delete __n1;
    delete __n2;
    newNode = false;
}

qlonglong& cLink::nodeId(eSide __e)
{
    switch (__e) {
    case LEFT:  return nodeId1;
    case RIGHT: return nodeId2;
    default:    EXCEPTION(EPROGFAIL);
    }
}
qlonglong& cLink::portId(eSide __e)
{
    switch (__e) {
    case LEFT:  return portId1;
    case RIGHT: return portId2;
    default:    EXCEPTION(EPROGFAIL);
    }
}

qlonglong& cLink::portId(eSide __e);

void cLink::side(eSide __e, QSqlQuery &q, QString * __n, QString *__p, int __s)
{
    qlonglong   nid;    // Node id
    if (__n->size() > 0) {  // Ha megadtunk node-ot
        nid = cPatch::getNodeIdByName(q, *__n);
    }
    else {
        nid = nodeId(__e);
        if (nid == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
    }
    portId(__e) = cNPort::getPortIdByName(q, *__p, nid, false);
    if (portId(__e) == NULL_ID) yyerror(QObject::trUtf8("Invalid left port specification, #%1:%2").arg(nid). arg(*__p));
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::trUtf8("Multiple share defined"));
        share = chkShare(__s);
    }
    delete __n;
    delete __p;
    _DBGFNL() << _sSpace << portId(__e) << endl;
}

void cLink::side(eSide __e, QSqlQuery &q, QString * __n, int __p, int __s)
{
    qlonglong   nid;
    if (__n->size() > 0) {  // Ha megadtunk node-ot
        nid = cPatch::getNodeIdByName(q, *__n);
        if (__p == NULL_IX) {   // Ha megadtuk a node-t akkor lehet null, de akkor csak egy portja lehet
            cNPort ep;
            ep.set(_sNodeId, nid);   // megkeressük a node egyetlen portját
            if (ep.completion(qq()) != 1) yyerror("Insufficient or invalid data.");  // Pont egynek kell lennie
            portId(__e) = ep.getId();
        }
        else {
            portId(__e) = cNPort::getPortIdByIndex(q, __p, nid, false);
        }
    }
    else {
        nid = nodeId(__e);
        if (nid == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
        portId(__e) = cNPort::getPortIdByIndex(q, __p, nid, false);
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

void cLink::workstation(QSqlQuery &q, QString * __n, cMac * __mac, QString * __d)
{
    portId2 = newWorkstation(q, *__n, *__mac, *__d);
    delete __n;
    delete __mac;
    delete __d;
    newNode = true;
    _DBGFNL() << _sSpace << portId2 << endl;
}

void cLink::attached(QSqlQuery &q, QString * __n, QString * __d)
{
    portId2 = newAttachedNode(q, *__n, *__d);;
    delete __n;
    delete __d;
    newNode = true;
    _DBGFNL() << _sSpace << portId2 << endl;
}

void cLink::insert(QSqlQuery &q, QString * __d, QStringList * __srv)
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
                po.insert(q);
                lnk[_sPortId2] = po.getId();
                hose.setId(_sPortId,     po.getId());
            }
        }
        else {
            DWAR() << "iftype = " << it << " not supported." << endl;
        }
        lnk.insert(q);
        hose.insert(q);
        delete __srv;
    }
    else {
        lnk.insert(q);
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

void cLink::insert(QSqlQuery &q, QString *__hn1, qlonglong __pi1, QString *__hn2, qlonglong __pi2, qlonglong __n)
{
    cPhsLink    lnk;
    lnk[_sPhsLinkType1] = phsLinkType1;
    lnk[_sPhsLinkType2] = phsLinkType2;
    lnk[_sPortShared]   = _sNul;
    qlonglong nid1 = __hn1->size() > 0 ? cPatch::getNodeIdByName(q, *__hn1) : nodeId1;
    qlonglong nid2 = __hn2->size() > 0 ? cPatch::getNodeIdByName(q, *__hn2) : nodeId2;
    while (__n--) {
        lnk[_sPortId1] = cNPort::getPortIdByIndex(q, __pi1, nid1, true);
        lnk[_sPortId2] = cNPort::getPortIdByIndex(q, __pi2, nid2, true);
        lnk.insert(q);
        lnk.clear(lnk.idIndex());   // töröljük az id-t mert a köv insert-nél baj lessz.
        __pi1++;                    // Léptetjük az indexeket
        __pi2++;
    }
    delete __hn1;
    delete __hn2;
}


//-----------------------------------------------------------------------------------------------------
qlonglong placeId(QSqlQuery &q, const QString *__np)
{
    qlonglong id = cPlace().getIdByName(q, *__np, true);
    if (id == NULL_ID) yyerror(QObject::trUtf8("Place %1 not found.").arg(*__np));
    delete __np;
    return id;
}

/// A superior_host_service_id beállítása
/// @param phs A módosítandó objektum pointere.
/// @param phn A keresett host_services rekordban hivatkozott host neve, vagy üres.
/// @param phs A keresett host_services rekordban hivatkozott szervíz típus név
/// @param ppo A keresett host_services rekordban hivatkozott port neve, vagy NULL pointer. Ha phn üres, akkor NULL.
void setSuperiorHostService(cHostService * phs, QString * phn, QString * psn, QString *ppo)
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
cNPort *hostAddPort(QSqlQuery& q, cNode& h, int ix, QString *pt, QString *pn, QStringPair *ip, QVariant *mac, QString *d)
{
    cNPort& p = h.asmbHostPort(q, ix, *pt, *pn, ip, mac, *d);
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

cNPort *portAddAddress(QString *pn, QStringPair *ip, QString *d)
{
    cNPort *p = host().getPort(*pn);
    delete pn;
    return portAddAddress(p, ip, d);

}

cNPort *portAddAddress(int ix, QStringPair *ip, QString *d)
{
    cNPort *p = host().getPort(ix);
    return portAddAddress(p, ip, d);
}

static QString yylookup(const QHostAddress& ha)
{
    QHostInfo hi = QHostInfo::fromName(ha.toString());
    if (hi.error() != QHostInfo::NoError) yyerror("A host név nem állapítható meg a cím alapján");
    return hi.hostName();
}

static QHostAddress yylookup(const QString& hn)
{
    QHostInfo hi = QHostInfo::fromName(hn);
    if (hi.error() != QHostInfo::NoError) yyerror("A host cím nem állapítható meg a név alapján");
    QList<QHostAddress> al = hi.addresses();
    if (al.count() != 1) yyerror("A hoszt cím a név alapján nem egyértelmű.");
    return al[0];
}

/// Egy új host vagy snmp eszköz létrehozása (a paraméterként megadott pointereket felszabadítja)
/// @param name Az eszköz nevére mutató pointer, vagy a "LOOKUP" stringre mutató pointer
/// @param ip Pointer egy string pár, az első elem az IP cím, vagy az ARP ill. LOOKUP string, ha az ip címet a MAC címből iéé. a névből kell meghatározni.
///           A második elem az ip cím típus neve.
/// @param mac Vagy a MAC stringgé konvertálva, vagy az ARP string, ha az IP címből kell meghatározni.
/// @param d Port secriptorra/megjegyzés  mutató pointer, üres string esetln az NULL lessz.
/// @return Az új objektum pointere
template <class T> cNode *newHost(QString *name, QStringPair *ip, QString *mac, QString *d)
{
    _DBGFN() << "@(" << *name << _sComma << ip->first << "&" << ip->second << _sComma  << *mac << _sComma << *d << ")" << endl;
    cHost *p = new T(*name, *d);
    delete d;
    p->setName(_sIpAddressType, ip->second);
    QString ips  = ip->first;
    QHostAddress    ha;
    cMac            ma;

    if (*name == _sLOOKUP) {    // Ki kell deríteni a nevet
        if (ips == _sARP) {         // sőt az ip-t is
            if (!ma.set(*mac)) yyerror("Legalább egy helyes MAC-et meg kell adni.");
            ha = mac2addr(ma);
        }
        else {
            if (!ha.setAddress(ips)) yyerror("Nem értelmezhatő IP cím.");
            if (*mac == _sARP) ma = addr2mac(ha);
            else if (!ma.set(*mac)) yyerror("Nem értelmezhatő MAC cím.");
        }
        p->setName(yylookup(ha));
    }
    else if (ips == _sLOOKUP) { // A név OK, de a névből kell az IP
        ha = yylookup(*name);
        if (*mac == _sARP) ma = addr2mac(ha);
        else if (!ma.set(*mac)) yyerror("Nem értelmezhatő MAC cím.");
    }
    else if (ips == _sARP) {    // Az IP a MAC-ból derítebdő ki
        if (!ma.set(*mac)) yyerror("Az IP csak helyes MAC-ból deríthető ki.");
        ha = mac2addr(ma);
    }
    else if (*mac == _sARP) {   // Név és ip renddben kéne legyenek, MAC kidertendő
        if (!ha.setAddress(ips)) yyerror("Nem értelmezhatő IP cím.");
        ma = addr2mac(ha);
    }
    else {
        if (!ha.setAddress(ips)) yyerror("Nem értelmezhatő IP cím.");
        if (!ma.set(*mac)) yyerror("Nem értelmezhatő MAC cím.");
    }
    p->set(_sAddress, QVariant::fromValue(ha));
    p->set(_sHwAddress, QVariant::fromValue(ma));

    delete name; delete ip; delete mac;
    p->setId(_sPlaceId, gPlace());
    return p;
}
template cNode *newHost<cHost>(QString *name, QStringPair *ip, QString *mac, QString *d);
template cNode *newHost<cSnmpDevice>(QString *name, QStringPair *ip, QString *mac, QString *d);

void setLastPort(QString * np, qlonglong ix) {
    if (np != NULL && np->size() > 0) svars[sPortNm] = *np;
    else                              svars.remove(sPortNm);
    if (ix != NULL_IX)                ivars[sPortIx] = ix;
    else                              ivars.remove(sPortIx);
}

void setLastPort(const QString& n, qlonglong ix) {
    if (n.size() > 0)   svars[sPortNm] = n;
    else                svars.remove(sPortNm);
    if (ix != NULL_IX)  ivars[sPortIx] = ix;
    else                ivars.remove(sPortIx);
}

void setLastPort(cRecord *p)
{
    qlonglong ix = p->getId(_sPortIndex);
    if (ix == NULL_ID) ix = NULL_IX;
    setLastPort(p->getName(_sPortName), ix);
}


void setSuperior(QStringPair *pshs, QStringPairList * pshl)
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


cTableShape * newTableShape(QString *pTbl, QString * pMod, const QString *pDescr)
{
    cTableShape *p = new cTableShape();
    p->setName(*pMod);
    p->setName(_sTableName, pTbl->isEmpty() ? *pMod : *pTbl);
    p->setName(_sTableShapeDescr, *pDescr);
    delete pTbl;
    delete pMod;
    delete pDescr;
    return p;
}


QString *pMenuApp = NULL;
tRecordList<cMenuItem>   menuItems;

cMenuItem& actMenuItem()
{
    if (menuItems.isEmpty()) EXCEPTION(EPROGFAIL);
    return *menuItems.last();
}

void newMenuMenu(const QString& _n, const QString& _t)
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

void delMenuItem()
{
    cMenuItem *p = menuItems.pop_back();
    delete p;
}
/// Egy menu_items rekord létrehozása
/// @param _n Rekord név
/// @param _sn paraméter
/// @param _t Megjelenített név
/// @param typ Minta string a properties mezőhöz (paraméter _n
void newMenuItem(const QString& _n, const QString& _sn, const QString& _t, const char * typ)
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

void setToolTip(const QString& _tt)
{
    actMenuItem().setName(_sToolTip, _tt);
    actMenuItem().update(qq(), false, actMenuItem().mask(_sToolTip));
}

void setWhatsThis(const QString& _wt)
{
    actMenuItem().setName(_sWhatsThis, _wt);
    actMenuItem().update(qq(), false, actMenuItem().mask(_sWhatsThis));
}
