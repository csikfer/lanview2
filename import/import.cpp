#include "import.h"
#include "import_parser_yacc.h"
#include "lv2service.h"
#include "others.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

cArpTable *pArpTable = NULL;
cArpServerDefs * pArpServerDefs = NULL;
void setAppHelp()
{
    lanView::appHelp  = QObject::trUtf8("-u|--user-id <id>           Set user id\n");
    lanView::appHelp += QObject::trUtf8("-U|--user-name <name>       Set user name\n");
    lanView::appHelp += QObject::trUtf8("-i|--input-file <path>      Set input file path\n");
    lanView::appHelp += QObject::trUtf8("-I|--input-stdin            Set input file is stdin\n");
    lanView::appHelp += QObject::trUtf8("-D|--daemon-mode            Set daemon mode\n");
}

int main (int argc, char * argv[])
{
    QCoreApplication app(argc, argv);
    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = true;
    lanView::gui        = false;

    // Elmentjük az aktuális könyvtárt
    QString actDir = QDir::currentPath();
    lv2import   mo;
    if (mo.lastError != NULL) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    PDEB(VVERBOSE) << "Saved original current dir : " << actDir << "; actDir : " << QDir::currentPath() << endl;

    pArpTable = new cArpTable();
    pArpTable->getFromDb(qq());
    pArpServerDefs = new cArpServerDefs();
    // notifswitch tömb = SET, minden on-ba, visszaolvasás listaként
    allNotifSwitchs = cUser().set(_sHostNotifSwitchs, QVariant(0xffff)).get(_sHostNotifSwitchs).toStringList();

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    if (mo.daemonMode) {        // Daemon mód
        fileNm = "<TEXT>";      // Nem fájlt dolgozunk fel
        // A beragadt rekordok kikukázása
        mo.abortOldRecords();
        return app.exec();
    }
    else {
        // Ha nem daemon mód van, akkor visszaállítjuk az aktuális könyvtárt
        QDir::setCurrent(actDir);
        PDEB(VVERBOSE) << "Restore act dir : " << QDir::currentPath() << endl;
        try {
            if (mo.fileNm.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Nincs megadva forrás fájl!"));
            fileNm = mo.fileNm;
            QFile in(fileNm);
            if (fileNm == _sMinus || fileNm == _sStdin) {
                yyif = new QTextStream(stdin, QIODevice::ReadOnly);
            }
            else {
                if (!srcOpen(in)) EXCEPTION(EFOPEN, -1, in.fileName());
                yyif = new QTextStream(&in);
            }
            PDEB(VVERBOSE) << "BEGIN transaction ..." << endl;
            sqlBegin(qq());
            PDEB(INFO) << "Start parser ..." << endl;
            yyparse();
            PDEB(INFO) << "End parser." << endl;
        } catch(cError *e) {
            mo.lastError = e;
        } catch(...) {
            mo.lastError = NEWCERROR(EUNKNOWN);
        }
        if (mo.lastError) {
            sqlRollback(qq());
            PDEB(DERROR) << "**** ERROR ****\n" << mo.lastError->msg() << endl;
        }
        else {
            sqlEnd(qq());
            PDEB(DERROR) << "**** OK ****" << endl;
        }

        if (pArpTable)      delete pArpTable;
        if (pArpServerDefs) delete pArpServerDefs;
        cDebug::end();
        return mo.lastError == NULL ? 0 : mo.lastError->mErrorCode;
    }
}

void lv2import::abortOldRecords()
{
    QString sql =
            "UPDATE imports "
            "   SET exec_state = 'aborted',"
                "   ended = CURRENT_TIMESTAMP,"
                "   result_msg = 'Start imports server: old records aborted.'"
            " WHERE exec_state = 'wait' AND exec_state = 'execute'";
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
}

void lv2import::dbNotif(QString __s)
{
    QString     text;
    cImport     imp;
    try {
        PDEB(INFO) << QString(trUtf8("DB notification : %1.")).arg(__s) << endl;
        imp.setName(_sExecState, _sWait);
        imp.fetch(*pq, false, imp.mask(_sExecState), imp.iTab(_sDateOf), 1);
        if (imp.isEmpty_()) {
            DWAR() << trUtf8("No waitig imports record, dropp notification.") << endl;
            return;
        }
        sqlBegin(*pq);
        imp.setName(_sExecState, _sExecute);
        imp.set(_sStarted, QVariant(QDateTime::currentDateTime()));
        imp.setId(_sPid, QCoreApplication::applicationPid());
        imp.update(*pq, false, imp.mask(_sExecState, _sStarted, _sPid));
        sqlEnd(*pq);
        sqlBegin(*pq);
        text = imp.getName(_sImportText);
        yyif = new QTextStream(&text);
        yyparse();
    }
    catch(cError *e) {  lastError = e; }
    catch(...)       {  lastError = NEWCERROR(EUNKNOWN); }
    if (lastError == NULL) {
        imp.setName(_sExecState, _sOk);
        imp.setName(_sResultMsg, _sOk);
        imp.set(_sEnded, QVariant(QDateTime::currentDateTime()));
        imp.clear(_sAppLogId);
        imp.update(*pq, false, imp.mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId));
        sqlEnd(*pq);
    }
    else {
        sqlRollback(*pq);
        sqlBegin(*pq);
        qlonglong eid = sendError(lastError);
        imp.setName(_sExecState, _sFaile);
        imp.setName(_sResultMsg, lastError->msg());
        delete lastError;
        lastError = NULL;
        imp.set(_sEnded, QVariant(QDateTime::currentDateTime()));
        imp.setId(_sAppLogId, eid);
        imp.update(*pq, false, imp.mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId));
        sqlEnd(*pq);
    }
    QCoreApplication::exit(0);
}


lv2import::lv2import() : lanView(), fileNm(), in()
{
    daemonMode = false;
    if (lastError != NULL) {
        pq     = NULL;
        pPlace = NULL;
        return;
    }

    int i;
    qlonglong userId   = NULL_ID;
    QString   userName;
    if (0 < (i = findArg('u', "user-id", args)) && (i + 1) < args.count()) {
        // ???
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('U', "user-name", args)) && (i + 1) < args.count()) {
        userName = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('i', "input-file", args)) && (i + 1) < args.count()) {
        fileNm = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('I', "input-stdin", args))) {
        fileNm = '-';
        args.removeAt(i);
    }
    if (0 < (i = findArg('D', "daemon-mode", args))) {
        args.removeAt(i);;
        PDEB(INFO) << trUtf8("Set daemon mode.") << endl;
        daemonMode = true;
        subsDbNotif();
    }
    if (args.count() > 1) DWAR() << trUtf8("Invalid arguments : ") << args.join(_sSpace) << endl;
    pq = newQuery();
    pPlace = new cPlace();
    if (daemonMode) return;
    insertStart(*pq);
    if (!userName.isNull()) {
        userId = cUser().getIdByName(userName);
        if (userId == NULL_ID) EXCEPTION(EFOUND,-1,userName);
        if (!pq->exec(QString("SELECT set_user_id(%1)").arg(userId))) SQLQUERYERR(*pq);
    }
}

lv2import::~lv2import()
{
    DBGFN();
    if (pPlace != NULL) {
        PDEB(VVERBOSE) << "delete pPlace ..." << endl;
        delete pPlace;
    }
    if (pq     != NULL) {
        PDEB(VVERBOSE) << "delete pq ..." << endl;
        delete pq;
    }
    DBGFNL();
}

/* ---------------------------------------------------------------------------------------------------- */

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
QStringList allNotifSwitchs;

qlonglong globalPlaceId = NULL_ID; // ID of none

cNode newAttachedNode(const QString& __n, const QString& __d)
{
    static qlonglong attachTypeId = -1;
    if (attachTypeId < 0) attachTypeId = cIfType::ifTypeId(_sAttach);
    cNode   node;
    node.setName(__n);
    node[_sNodeDescr] = __d;
    node[_sPortName]  = _sAttach;
    node[_sIfTypeId]  = attachTypeId;
    node[_sPlaceId]   = gPlace();
    node.insert(qq(), true);
    return node;
}

void newAttachedNodes(const QString& __np, const QString& __dp, int __from, int __to)
{
    QString name, descr;
    for (int i = __from; i <= __to; i++) {
        name  = nameAndNumber(__np, i);
        descr = nameAndNumber(__dp, i);
        newAttachedNode(name, descr);
    }
}

/// @param __n Node neve
/// @param __mac MAC cím
/// @param __d node_descr mező értéke.
cHost newWorkstation(const QString& __n, const cMac& __mac, const QString& __d)
{
    static qlonglong wstTypeId = -1;
    if (wstTypeId < 0) wstTypeId = cIfType::ifTypeId(_sEthernet);
    cHost   node;
    node.setName(__n);
    node[_sNodeDescr] = __d;
    node[_sPortName]  = _sEthernet;
    node[_sIfTypeId]  = wstTypeId;
    node[_sPlaceId]   = gPlace();
    node[_sHwAddress] = __mac.toString();
    node[_sIpAddressType]= _sDynamic;   // Dinamikus IP címet feltéteéezünk

    QHostAddress ha;
    QList<QHostAddress> al;
    al = (*pArpTable)[__mac];                     // MAC alapján kell címet keresni
    if (al.size() == 1) ha = al[0];
    if (ha.isNull()) {
        QHostInfo hi = QHostInfo::fromName(__n);         // Név alapján lekérdezzük az IP címet
        al = hi.addresses();
        if (al.size() == 1) ha = al[0];
    }
    if (!ha.isNull()) {     // Ha van ip, akkor beállítjuk
        node.set(__sAddress, ha.toString());
    }
    node.insert(qq(), true);
    return node;
}

/* ************************************************************************************************ */

void cArpServerDef::updateArpTable(cArpTable& __at) const
{
    DBGFN();
    switch (type) {
    case UNKNOWN:       break;
    case SNMP: {
            cSnmp snmp(server.toString(), community, SNMP_VERSION_2c);
            __at.getBySnmp(snmp);
            break;
        }
    case DHCPD_LOCAL:   __at.getByLocalDhcpdConf(file);                                         break;
    case DHCPD_SSH:     __at.getBySshDhcpdConf(server.toString(), file);                        break;
    case PROC_LOCAL:    __at.getByLocalProcFile(file);                                          break;
    case PROC_SSH:      __at.getBySshProcFile(server.toString(), file);                         break;
    default: EXCEPTION(EPROGFAIL);
    }
}

void cArpServerDef::reUpdateArpTable(cArpTable& __at) const
{
    DBGFN();
    switch (type) {
    case UNKNOWN:       break;
    case SNMP: {
            cSnmp snmp(server.toString(), community, SNMP_VERSION_2c);
            __at.getBySnmp(snmp);
            break;
        }
    case DHCPD_LOCAL:   break;
    case DHCPD_SSH:     break;
    case PROC_LOCAL:    __at.getByLocalProcFile(file);                                          break;
    case PROC_SSH:      __at.getBySshProcFile(server.toString(), file);                         break;
    default: EXCEPTION(EPROGFAIL);
    }
}

/* --------------------------------------------------------------------------------------------------- */


cLink::cLink(int __t1, QString *__n1, int __t2, QString *__n2)
    : phsLinkType1(i2LinkType(__t1))
    , phsLinkType2(i2LinkType(__t2))
{
    cPatch  n;
    nodeId1 = __n1->isNull() ? NULL_ID : n.getIdByName(qq(), *__n1);
    nodeId2 = __n2->isNull() ? NULL_ID : n.getIdByName(qq(), *__n2);
    delete __n1;
    delete __n2;
    newNode = false;
}

void cLink::left(QString * __n, QString *__p, int  __s)
{
    cNPort  p; // Bár a cPPort-nak nem őse, de csak az ID kell, így elég ha csak az adatbázisban az.
    qlonglong   nid;
    p.setName(*__p);
    if (__n->size() > 0) {  // Ha megadtun node-ot
        nid = cPatch().getIdByName(qq(), *__n);
    }
    else {
        if (nodeId1 == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
        nid = nodeId1;
    }
    p[_sNodeId] = nid;
    if (1 != p.completion()) yyerror(QString(QObject::trUtf8("Invalid left port specification, node_id = %1, port_name = %2"))
                                     .arg(nid). arg(*__p));
    portId1 = p.getId();
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::trUtf8("Multiple share defined"));
        share = chkShare(__s);
    }
    delete __n;
    delete __p;
    _DBGFNL() << _sSpace << portId1 << endl;
}

void cLink::right(QString * __n, QString *__p, int  __s)
{
    cNPort  p; // Bár a cPPort-nak nem őse, de csak az ID kell, így elég ha csak az adatbázisban az.
    qlonglong   nid;
    p.setName(*__p);
    if (__n->size() > 0) {  // Ha megadtun node-ot
        nid = cPatch().getIdByName(qq(), *__n);
    }
    else {
        if (nodeId2 == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
        nid = nodeId2;
    }
    p[_sNodeId] = nid;
    if (1 != p.completion()) yyerror(QString(QObject::trUtf8("Invalid right port specification, node_id = %1, port_name = %2"))
                                     .arg(nid). arg(*__p));
    portId2 = p.getId();
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::trUtf8("Multiple share defined"));
        share = chkShare(__s);
    }
    delete __n;
    delete __p;
    _DBGFNL() << _sSpace << portId2 << endl;
}

void cLink::left(QString * __n, int __p, int  __s)
{
    cNPort  p;      // Az adatbázisban ez a bazis tábla
    qlonglong   nid;
    p.set(_sPortIndex, QVariant(__p));
    if (__n->size() > 0) {  // Ha megadtunk node-ot
        nid = cPatch().getIdByName(qq(), *__n); // Az adatbázis szerinti bázist használjuk, nam bázis osztály !!!
        if (__p == NULL_IX) {
            cNPort n;
            n.set(_sNodeId, nid);   // megkeressük a node egyetlen portját
            if (n.completion(qq()) != 1) yyerror("Insufficient or invalid data.");  // Pont egynek kell lennie
            p.clear(_sPortIndex);       // Az index mégsem ismert
            p.setId(n.getId(_sPortId)); // ID azonosítja
        }
    }
    else {
        if (nodeId1 == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
        nid = nodeId1;
    }
    p[_sNodeId] = nid;
    if (0 == p.completion()) {
        yyerror(QString(QObject::trUtf8("Invalid left port specification, port_index = %1, node_id = %2"))
                .arg(__p).arg(nid));
    }
    if (1  < p.completion()) {
        yyerror(QString(QObject::trUtf8("Redundant left port specification"))
                .arg(__p).arg(nid));
    }
    portId1 = p.getId();
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::trUtf8("Multiple share defined"));
        share = chkShare(__s);;
    }
    delete __n;
    _DBGFNL() << _sSpace << portId1 << endl;
}

void cLink::right(QString * __n, int __p, int  __s)
{
    cNPort  p;
    qlonglong   nid;
    p.set(_sPortIndex, QVariant(__p));
    if (__n->size() > 0) {  // Ha megadtun node-ot
        nid = cPatch().getIdByName(qq(), *__n);  // Az adatbázis szerinti bázist használjuk, nam bázis osztály !!!
        if (__p == NULL_IX) {
            cNPort n;
            n.set(_sNodeId, nid);   // megkeressük a node egyetlen portját
            if (n.completion(qq()) != 1) yyerror("Insufficient or invalid data.");  // Pont egynek kell lennie
            p.clear(_sPortIndex);       // Az index mégsem ismert
            p.setId(n.getId(_sPortId)); // ID azonosítja
        }
    }
    else {
        if (nodeId2 == NULL_ID) EXCEPTION(EDATA, -1, "Nincs megadva node!");
        nid = nodeId2;
    }
    p[_sNodeId] = nid;
    if (0 == p.completion()) {
        yyerror(QString(QObject::trUtf8("Invalid right port specification, port_index = %1, node_id = %2"))
                .arg(__p).arg(nid));
    }
    if (1  < p.completion()) {
        yyerror(QString(QObject::trUtf8("Redundant right port specification"))
                .arg(__p).arg(nid));
    }
    portId2 = p.getId();
    if (__s != 0) {
        if (share.size() > 0) yyerror(QObject::trUtf8("Multiple share defined"));
        share = chkShare(__s);
    }
    delete __n;
    _DBGFNL() << _sSpace << portId2 << endl;
}

void cLink::workstation(QString * __n, cMac * __mac, QString * __d)
{
    // Host objektum kitöltése, és inzert
    cHost   h = newWorkstation(*__n, *__mac, *__d);
    delete __n;
    delete __mac;
    delete __d;
    portId2 = h.get(_sPortId).toLongLong(); // Az egy ethernet port van linkelve (ha mégse, majd javítjuk)
    newNode = true;
    _DBGFNL() << _sSpace << portId2 << endl;
}

void cLink::attached(QString * __n, QString * __d)
{
    cNode   h = newAttachedNode(*__n, *__d);
    delete __n;
    delete __d;
    portId2 = h.getId(_sPortId);
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
    lnk[_sPhsLinkDescr] = *__d;
    if (__srv != NULL) {    // Volt Alert
        cHostService    hose;
        cService        se;
        cNode           ho;
        cNPort          po;
        if (__srv->size() > 0 && __srv->at(0).size() > 0) { if (!se.fetchByName(__srv->at(0))) yyerror("Invalis alert service name."); }
        else                                              { if (!se.fetchById(alertServiceId)) yyerror("Nothing default alert service."); }
        po.setById(qq(), portId2);   // A jobb oldali port a mienk
        ho.setById(qq(), po.getId(_sNodeId));
        if (__srv->size() > 1 && __srv->at(1).size() > 0) hose.setName(_sHostServiceDescr, __srv->at(1));
        hose.setId(_sServiceId,  se.getId());
        hose.setId(_sNodeId,     ho.getId());
        hose.setId(_sPortId,     po.getId());
        hose[_sDelegateHostState] = true;
        QString it = se.magicParam(_sIfType);
        if (it.isEmpty()) {
            DWAR() << "Invalid or empty service magic param : iftype" << endl;
        }
        else if (it == _sEthernet) {     // A megasott szolgáltatás az ethernethez tartozik
            cIfType eth;
            eth.setByName(qq(), _sEthernet);
            if (po.getId(_sIfTypeId) != eth.getId()) yyerror("Invalid port type");
        }
        else if (it == _sAttach) {
            cIfType att;
            att.setByName(qq(), _sAttach);
            if (po.getId(_sIfTypeId) != att.getId()) {
                if (!newNode) yyerror("Invalid port type");
                po.set();
                po.setName(__sAttach);
                po.setId(_sNodeId, ho.getId());
                po.setId(_sIfTypeId, att.getId());
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
    cNPort      np;
    lnk[_sPhsLinkType1] = phsLinkType1;
    lnk[_sPhsLinkType2] = phsLinkType2;
    lnk[_sPortShared]   = _sNul;
    qlonglong nid1 = __hn1->size() > 0 ? cPatch().getIdByName(qq(), *__hn1) : nodeId1;
    qlonglong nid2 = __hn2->size() > 0 ? cPatch().getIdByName(qq(), *__hn2) : nodeId2;
    while (__n--) {
        lnk[_sPortId1] = np.getPortIdByIndex(qq(), __pi1, nid1, true);
        lnk[_sPortId2] = np.getPortIdByIndex(qq(), __pi2, nid2, true);
        lnk.insert(qq());
        lnk.clear(lnk.idIndex());
        __pi1++;
        __pi2++;
    }
    delete __hn1;
    delete __hn2;
}


//-----------------------------------------------------------------------------------------------------
qlonglong placeId(const QString *__np)
{
    qlonglong id = cPlace().getIdByName(qq(), *__np, true);
    if (id == NULL_ID) yyerror(QString(QObject::trUtf8("Place %1 not found.")).arg(*__np));
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
/// @param ix Port index, vagy NULL_IX, ha az index NULL lessz-
/// @param pt Pointer az port típus név stringre.
/// @param pn Port nevére mutató pointer
/// @param ip Pointer egy string pár, az első elem az IP cím, vagy az ARP string, ha az ip címet a MAC címéből kell meghatározni.
///           A második elem az ip cím típus neve. Ha a pointer NULL, akkor nincs IP cím (ez befolyásolja a port objektum típusát!)
///           Ha first, vagyis az IP cím egy öres string, akkor nincs IP, de az az objektum változat kell, amiben van (lehet) ip cím.
/// @param mac Ha NULL nincs mac, ha a variant egy string, akkor az a MAC cím stringgé konvertélva, vagy az "ARP" string, ha variant egy int,
///           akkor az abbak a Portnak az indexe, melynek ugyanez a MAC cíne.
/// @param d Port secriptorra/megjegyzés  mutató pointer, üres string esetln az NULL lessz.
/// @return Az új port objektum pointere
cNPort *hostAddPort(int ix, QString *pt, QString *pn, QStringPair *ip, QVariant *mac, QString *d)
{
    cHost& h = host();
    QHostAddress a;
    cMac m;
    if (h.getPort(ix, false) != NULL || h.getPort(*pn, false) != NULL) yyerror(e1);
    cNPort *p = cNPort::newPort(cIfType::ifType(*pt), ip == NULL ? 0 : 1);
    h.ports << p;
    p->setName(_sPortName, *pn);
    if (ix != NULL_IX) p->setId(_sPortIndex, ix);
    if (d->size()) p->setName(_sPortDescr, *d);
    if (ip != NULL) {
        QString type = ip->second;
        QString sip  = ip->first;
        if (sip == _sARP) {
            if (mac != NULL && mac->type() == QVariant::String) m.set(mac->toString());
            if (!m) yyerror(e2);                    // Nincs megadva MAC / nem OK
            QList<QHostAddress> al = (*pArpTable)[m];
            if (al.size() != 1) yyerror(e2);        // Nem talált címet, vagy több is van
            a = al.first();
        }
        else if (!sip.isEmpty()) {
            a.setAddress(sip);
        }
        if (!a.isNull()) p->set(_sAddress, QVariant::fromValue(a));
        p->setName(_sIpAddressType, type);
        delete ip;
    }
    if (mac != NULL) {
        switch (mac->type()) {
        case QVariant::String:
            if (mac->toString() == _sARP) {
                if (!a.isNull()) yyerror(e2);
                m = (*pArpTable)[a];
                if (!m) yyerror(e2);
            }
            else {
                m.set(mac->toString());
            }
            break;
        case QVariant::LongLong:
            m = h.getPort(mac->toInt())->get(_sHwAddress).value<cMac>();
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
        if (!m) yyerror("Invalid MAC");
        p->set(_sHwAddress, QVariant::fromValue(m));
        delete mac;
    }
    delete pt; delete pn; delete d;
    return p;
}

void    hostAddAddress(QStringPair *ip, QString *d)
{
    cHost& h = host();
    h.addIpAddress(QHostAddress(ip->first), ip->second, *d);
    delete ip; delete d;
}

static cNPort *portAddAddress(cNPort *_p, QStringPair *ip, QString *d)
{
    cIfaceAddr *p = _p->reconvert<cIfaceAddr>();
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

static QHostAddress mac2addr(const cMac& mac)
{
    QList<QHostAddress> la = (*pArpTable)[mac];
    if (la.count() != 1) yyerror("A IP cím nem állapítható meg a MAC címből.");
    return la[0];
}

static cMac addr2mac(const QHostAddress& ha)
{
    if (pArpTable->find(ha) == pArpTable->end()) yyerror("A MAC cím nem állapítható meg a IP címből.");
    return (*pArpTable)[ha];
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
