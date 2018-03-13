#include "report.h"
#include "srvdata.h"
#include "guidata.h"

const QString sHtmlLine   = "\n<hr>\n";
const QString sHtmlTabBeg = "<table border=\"1\" cellpadding=\"2\" cellspacing=\"2\">";
const QString sHtmlTabEnd = "</table>\n";
const QString sHtmlRowBeg = "<tr>";
const QString sHtmlRowEnd = "</tr>\n";
const QString sHtmlTh     = "<th> %1 </th>";
const QString sHtmlTd     = "<td> %1 </td>";
const QString sHtmlBold   = "<b>%1</b>";
const QString sHtmlVoid   = " - ";
const QString sHtmlBr     = "<br>";
const QString sHtmlBRed   = "<b><span style=\"color:red\"> %1 </span></b>";
const QString sHtmlBGreen = "<b><span style=\"color:green\"> %1 </span></b>";


QString toHtml(const QString& text, bool chgBreaks, bool esc)
{
    static const QChar   cr = QChar('\n');
    QString r = text;
    if (chgBreaks)  r = r.trimmed();
    if (esc)        r = r.toHtmlEscaped();
    if (chgBreaks)  r = r.replace(cr, sHtmlBr);
    return r;
}


QString htmlTableLine(const QStringList& fl, const QString& ft)
{
    QString r = tag("tr");
    foreach (QString f, fl) {
        r += tag(ft);
        r += f.toHtmlEscaped();
        r += tag("/" + ft);
    }
    r += tag("/tr");
    return r + "\n";
}

QString htmlTable(QStringList head, QList<QStringList> matrix)
{
    QString table;
    table += "\n<table border=\"1\"> ";
    if (!head.isEmpty()) table += htmlTableLine(head, "th");
    foreach (QStringList line, matrix) {
        table += htmlTableLine(line, "td");
    }
    table += "</table>\n";
    return table;
}

QString query2html(QSqlQuery q, cTableShape &_shape, const QString& _where, const QVariantList& _par, const QString& shrt)
{
    cRecordAny rec(_shape.getName(_sTableName));
    tRecordList<cRecordAny> list;
    QString ord = shrt;
    if (ord.isEmpty()) {        // Default: order by name, if exixst name
        QString nameName = rec.nameName(EX_IGNORE);
        if (!nameName.isEmpty())ord = QString(" ORDER BY %1 ASC").arg(nameName);
    }
    else if (ord == "!") {      // No order
        ord.clear();
    }
    else {
        ord.prepend(" ");
    }
    QString sql = QString("SELECT * FROM %1 WHERE ").arg(rec.tableName()) + _where + ord;
    if (execSql(q, sql, _par)) do {
        rec.set(q);
        list << rec;
    } while (q.next());
    if (list.isEmpty()) return QString();
    return list2html(q, list, _shape, false);
}

/* *********************************************************************************************** */

QString sReportPlace(QSqlQuery& q, qlonglong _pid, bool parents, bool zones, bool cat)
{
    QString r;
    cPlace  place;
    place.setById(q, _pid);
    r = place.getName();
    if (parents) {
        qlonglong pid = place.getId(_sParentId);
        cPlace ppl;
        while (pid != NULL_ID && pid != ROOT_PLACE_ID) {
            ppl.setById(q, pid);
            r.prepend(ppl.getName() + " / ");
            pid = ppl.getId(_sParentId);
        }
    }
    r.prepend(QObject::trUtf8("Hely : "));
    if (zones) {
        const static QString sql = "SELECT place_group_name FROM place_groups WHERE place_group_id > 1 AND is_place_in_zone(?, place_group_id)";
        if (execSql(q, sql, _pid)) {
            r += QObject::trUtf8(" Zónák : ");
            do {
                r += q.value(0).toString() + ", ";
            } while (q.next());
            r.chop(2);
        }
        else {
            r += QObject::trUtf8(" A hely nem része egyetlen zónának sem.");
        }
    }
    if (cat) {
        const static QString sql = "SELECT place_group_name FROM place_groups JOIN place_group_places USING(place_group_id) WHERE place_group_type = 'category' AND place_id = ?";
        if (execSql(q, sql, _pid)) {
            r += QObject::trUtf8(" Kategória : ");
            do {
                r += q.value(0).toString() + ", ";
            } while (q.next());
            r.chop(2);
        }
        else {
            r += QObject::trUtf8(" Nincs kategória.");
        }
    }
    return r;
}

tStringPair htmlReportPlace(QSqlQuery& q, cRecord& o)
{
    qlonglong pid = o.getId();
    cTableShape shape;
    shape.setByName(q, _sPlaces);
    QString t = QObject::trUtf8("%1 hely vagy helyiség riport.").arg(o.getName());
    QString html = htmlInfo(sReportPlace(q, pid));
    // Groups
    const static QString sql_pg = "SELECT place_groups.* FROM place_group_places JOIN place_groups USING (place_group_id) WHERE place_id = ?";
    if (execSql(q, sql_pg, pid)) {
        html += htmlInfo(QObject::trUtf8("Csoport tagságok :"));
        cPlaceGroup pg;
        tRecordList<cPlaceGroup>    pgl;
        pg.set(q);
        do {
            pgl << pg;
        } while(pg.next(q));
        shape.setByName(_sPlaceGroups);
        html += list2html(q, pgl, shape);
    }
    else {
        html += htmlInfo(QObject::trUtf8("A hely ill. helyiség nem tagja egy csoportnak sem."));
    }
    // pachs + nodes
    tRecordList<cPatch> patchs;
    patchs.fetch(q, true, _sPlaceId, pid);
    tRecordList<cNode>  nodes;
    nodes.fetch(q, false, _sPlaceId, pid);
    if (patchs.isEmpty() && nodes.isEmpty()) {
        html += htmlInfo(QObject::trUtf8("A helyiségben nincsenek eszközök."));
    }
    else {
        html += htmlInfo(QObject::trUtf8("A helyiségben lévő eszközök."));
        if (!patchs.isEmpty()) {
            html += htmlInfo(QObject::trUtf8("Patch penelek, vagy fali csatlakozók :"));
            shape.setByName(_sPatchs);
            html += list2html(q, patchs, shape);
        }
        if (!patchs.isEmpty()) {
            html += htmlInfo(QObject::trUtf8("Hálózati eszközök :"));
            shape.setByName(_sNodes);
            html += list2html(q, nodes, shape);
        }
    }
    tRecordList<cUser>  users;
    users.fetch(q, false, _sPlaceId, pid);
    if (!patchs.isEmpty()) {
        html += htmlInfo(QObject::trUtf8("Felhasználók :"));
        shape.setByName(_sUsers);
        html += list2html(q, users, shape);
    }
    return tStringPair(t, html);
}


/* ------------------------------------------------------------------------------------------------ */

QString titleNode(int t, const cRecord& n)
{
    QString title = n.getName() + " #" + QString::number(n.getId()) + " ";
    switch(t) {
    case NOT_PATCH:
        title += QObject::trUtf8("Patch port vagy fali csatlakozó");
        break;
    case NOT_NODE:
        title += QObject::trUtf8("Passzív vagy aktív hálózati eszköz");
        break;
    case NOT_SNMPDEVICE:
        title += QObject::trUtf8("Aktív SNMP eszköz");
        break;
    default:
        EXCEPTION(EDATA, 0, n.identifying());
        break;
    }
    return title;
}

QString titleNode(const cRecord& n)
{
    int t = NOT_INVALID;
    qlonglong toid = n.tableoid();
    if      (toid == cPatch::     _descr_cPatch()     .tableoid()) t = NOT_PATCH;
    else if (toid == cNode::      _descr_cNode()      .tableoid()) t = NOT_NODE;
    else if (toid == cSnmpDevice::_descr_cSnmpDevice().tableoid()) t = NOT_SNMPDEVICE;
    else    EXCEPTION(EDATA, 0, n.identifying());
    return titleNode(t ,n);
}

QString titleNode(QSqlQuery &q, const cRecord& n)
{
    int t = nodeObjectType(q, n);
    t &= ~(NOT_ANY | NOT_NEQ);
    return titleNode(t ,n);
}

tStringPair htmlReportNode(QSqlQuery& q, cRecord& _node, const QString& _sTitle, qlonglong flags)
{
    QString text;
    QString title;
    cPatch *po   = nodeToOrigin(q, &_node);
    cPatch& node = *(po == NULL ? dynamic_cast<cPatch *>(&_node) : dynamic_cast<cPatch *>(po));
    if (po != NULL) {
        if (flags &  CV_PORTS)       node.fetchPorts(q);
        if (flags &  CV_NODE_PARAMS) node.fetchParams(q);
    }
    QString tablename = node.tableName();
    bool isPatch = tablename == _sPatchs;
    title = _sTitle;
    if (title.isNull()) {      // Default title
        title = titleNode(node);
    }
    else {
        title = _sTitle.arg(node.getName(), tablename);
    }
    if (!isPatch) {
        text += htmlInfo(QObject::trUtf8("Típus jelzők : %1").arg(node.getName(_sNodeType)));
        text += htmlInfo(QObject::trUtf8("Állapota : %1").arg(node.getName(_sNodeStat)));
    }
    QString s = _node.getNote();
    if (!s.isEmpty()) text += htmlInfo(QObject::trUtf8("Megjegyzés : %1").arg(s));
    qlonglong pid = _node.getId(_sPlaceId);
    if (pid <= 1) text += htmlInfo(QObject::trUtf8("Az eszköz helye ismeretlen"));
    else text += htmlInfo(sReportPlace(q, pid));
    /* -- PARAM -- */
    if (flags & CV_NODE_PARAMS) {
        if ((node.containerValid & CV_NODE_PARAMS) == 0) node.fetchParams(q);
        text += htmlInfo(QObject::trUtf8("Eszköz paraméterek :"));
        text += sHtmlTabBeg + sHtmlRowBeg;
        text += sHtmlTh.arg(QObject::trUtf8("Paraméter típus"));
        text += sHtmlTh.arg(QObject::trUtf8("Érték"));
        text += sHtmlTh.arg(QObject::trUtf8("Dim."));
        text += sHtmlRowEnd;
        QListIterator<cNodeParam *> li(node.params);
        while (li.hasNext()) {
             cNodeParam * p = li.next();
             text += sHtmlRowBeg;
             text += sHtmlTd.arg(p->name());
             text += sHtmlTd.arg(p->value().toString());
             text += sHtmlTd.arg(p->dim());
             text += sHtmlRowEnd;
        }
        text += sHtmlTabEnd;
    }
    /* -- PORTS -- */
    if (flags & CV_PORTS) {
        if ((node.containerValid & CV_PORTS) == 0) node.fetchPorts(q);
        text += htmlInfo(QObject::trUtf8("Portok :"));
        node.sortPortsByIndex();
        // Table header
        text += sHtmlTabBeg + sHtmlRowBeg;
        text += sHtmlTh.arg(QObject::trUtf8("#"));
        text += sHtmlTh.arg(QObject::trUtf8("Port"));
        text += sHtmlTh.arg(QObject::trUtf8("Cimke"));
        if (!isPatch) text += sHtmlTh.arg(QObject::trUtf8("Típus"));
        text += sHtmlTh.arg(isPatch ? QObject::trUtf8("Shared") : QObject::trUtf8("MAC"));
        text += sHtmlTh.arg(isPatch ? QObject::trUtf8("S.p.")   : QObject::trUtf8("IP cím(ek)"));
        if (!isPatch) text += sHtmlTh.arg(QObject::trUtf8("DNS név"));
        text += sHtmlTh.arg(QObject::trUtf8("Fizikai link"));
        if (!isPatch) text += sHtmlTh.arg(QObject::trUtf8("Logikai link"));
        if (!isPatch) text += sHtmlTh.arg(QObject::trUtf8("LLDP link"));
        if (!isPatch) text += sHtmlTh.arg(QObject::trUtf8("MAC in mactab"));
        text += sHtmlRowEnd;
        // Table data
        QListIterator<cNPort *> li(node.ports);
        while (li.hasNext()) {
            cNPort * p = li.next();
            cInterface *pif = NULL;
            text += sHtmlRowBeg;
            text += sHtmlTd.arg(p->getName(_sPortIndex));
            text += sHtmlTd.arg(p->getName());
            text += sHtmlTd.arg(p->getName(_sPortTag));
            if (!isPatch) text += sHtmlTd.arg(cIfType::ifTypeName(p->getId(_sIfTypeId)));
            // Columns: port, típus, MAC|Shared, IP|S.p., DNS|-
            if (p->descr() == cInterface::_descr_cInterface()) {  // Interface
                QString ips, dns;
                pif = p->reconvert<cInterface>();
                QListIterator<cIpAddress *> ii(pif->addresses);
                while (ii.hasNext()) {
                    cIpAddress * ia = ii.next();
                    ips += sHtmlBold.arg(ia->view(q, _sAddress)) + "/" + ia->getName(_sIpAddressType) + _sCommaSp;
                    QHostInfo hi = QHostInfo::fromName(ia->address().toString());
                    dns += hi.hostName() + _sCommaSp;
                }
                ips.chop(_sCommaSp.size());
                dns.chop(_sCommaSp.size());
                text += sHtmlTd.arg(pif->mac().toString());
                text += sHtmlTd.arg(ips);
                text += sHtmlTd.arg(dns);
            }
            else if (isPatch) {                                 // PPort
                cPPort *pp = p->reconvert<cPPort>();
                text += sHtmlTd.arg(pp->getName(_sSharedCable));
                qlonglong spid = pp->getId(_sSharedPortId);
                if (spid == NULL_ID) text += sHtmlTd.arg(sHtmlVoid);
                else {
                    int ix = node.ports.indexOf(spid);
                    if (ix < 0) text += sHtmlTd.arg(htmlError("!?"));
                    else        text += sHtmlTd.arg(node.ports.at(ix)->getName());
                }
            }
            else {                                              // NPort
                 text += sHtmlTd.arg(sHtmlVoid);
                 text += sHtmlTd.arg(sHtmlVoid);
                 text += sHtmlTd.arg(sHtmlVoid);
            }
            qlonglong pid = p->getId();
            /// Columns: PhsLink, LogLink|-, LLDP|-, MACTab|-
            if (isPatch) {
                cPhsLink pl;
                pl.setId(_sPortId1, pid);
                int n = pl.completion(q);
                QStringList sl;
                if (n > 0) {
                    do {
                        QString sh = pl.getName(_sPortShared);
                        QString sp = cNPort::getFullNameById(q, pl.getId(_sPortId2));
                        sl << (sh.isEmpty() ? sp : (sh + ":" + sp));
                    } while (pl.next(q));
                }
                text += sHtmlTd.arg(sl.isEmpty() ? sHtmlVoid : sl.join(sHtmlBr));
            }
            else {
                qlonglong plp = LinkGetLinked<cPhsLink>(q, pid);    // -> phisical link
                qlonglong llp = LinkGetLinked<cLogLink>(q, pid);    // -> logical link
                qlonglong ldp = LinkGetLinked<cLldpLink>(q, pid);   // -> LLDP
                qlonglong mtp = NULL_ID;                            // -> port ID: MAC in MacTab
                if (pif != NULL) {  // Interface ?
                    cMacTab mt;
                    mt.setMac(_sHwAddress, pif->getMac(_sHwAddress));
                    if (mt.completion(q)) {
                        mtp = mt.getId(_sPortId);
                    }
                }
                // phisical
                text += sHtmlTd.arg(plp == NULL_ID ? sHtmlVoid : cNPort::getFullNameById(q, plp));
                // logical
                if (llp == NULL_ID) text += sHtmlTd.arg(sHtmlVoid);
                else {
                    QString n = cNPort::getFullNameById(q, llp);
                    if (mtp != NULL_ID) {
                        if (mtp != llp) n = sHtmlBRed.arg(n);
                        else            n = sHtmlBGreen.arg(n);
                    }
                    text += sHtmlTd.arg(n);
                }
                // lldp
                if (ldp == NULL_ID) text += sHtmlTd.arg(sHtmlVoid);
                else {
                    QString n = cNPort::getFullNameById(q, ldp);
                    if (llp != NULL_ID) {
                        if (llp != ldp) n = sHtmlBRed.arg(n);
                        else            n = sHtmlBGreen.arg(n);
                    }
                    text += sHtmlTd.arg(n);
                }
                // mac tab
                if (mtp != NULL_ID) {
                    QString n = cNPort::getFullNameById(q, mtp);
                    if (llp != NULL_ID) {
                        if (mtp != llp) n = sHtmlBRed.arg(n);
                        else            n = sHtmlBGreen.arg(n);
                    }
                    text += sHtmlTd.arg(n);
                }
            }
            text += sHtmlRowEnd;
        }
        text += sHtmlTabEnd;
    }
    else if (flags) {
        text += sHtmlBr + sHtmlBr + QObject::trUtf8("Nincs egyetlen portja sem az adatbázisban az eszköznek.") + "</b>";
    }
    pDelete(po);
    return tStringPair(title, text);
}

QString htmlReportByMac(QSqlQuery& q, const QString& sMac)
{
    QString text;
    cMac mac(sMac);
    if (!mac) {
        text = QObject::trUtf8("A '%1' nem valós MAC!").arg(sMac);
        return text;
    }
    /* ** OUI ** */
    cOui oui;
    if (oui.fetchByMac(q, mac)) {
        text += QObject::trUtf8("OUI rekord : ") + oui.getName() + sHtmlBr;
        text += oui.getNote();
    }
    else {
        text += QObject::trUtf8("Nincs OUI rekord.");
    }
    text += sHtmlLine;
    /* ** NODE ** */
    cNode node;
    int n = node.fetchByMac(q, mac, EX_ERROR);
    switch (n) {
    case 0:
        text += QObject::trUtf8("Nincs bejegyzett hálózati elem ezzel a MAC-kel");
        text += sHtmlLine;
        break;
    case 1:
        text += htmlPair2Text(htmlReportNode(q, node));
        text += sHtmlLine;
        break;
    default:    // ??????
        text += htmlWarning("Ezzel az MAC címmel több hálózati elem is be van jegyezve.");
        do {
            QSqlQuery qq = getQuery();
            text += htmlPair2Text(htmlReportNode(qq, node));
            text += sHtmlLine;
        } while(node.next(q));
    }
    /* ** ARP ** */
    QVariantList par;
    par << mac.toString();
    QString tab = query2html(q, _sArps, "hwaddress = ? ORDER BY last_time", par);
    if (tab.isEmpty()) {
        text += QObject::trUtf8("Nincs találat az arps táblában a megadott MAC-kel");
    }
    else {
        text += QObject::trUtf8("Találatok a MAC - IP (arps) táblában :");
        text += tab;
    }
    text += sHtmlLine;
    /* ** MAC TAB ** */
    cMacTab mt;
    mt.setMac(_sHwAddress, mac);
    if (mt.completion(q)) {
        text += QObject::trUtf8("Találat a switch cím táblában : <b>%1</b> (%2 - %3; %4)").arg(
                    cNPort::getFullNameById(q, mt.getId(_sPortId)), // Switch port full name
                    mt.view(q, _sFirstTime),
                    mt.view(q, _sLastTime),
                    mt.view(q, _sMacTabState));
    }
    else {
        text += QObject::trUtf8("Nincs találat a switch címtáblákban a megadott MAC-el");
    }
    text += sHtmlLine;
    // LOG
    text += QObject::trUtf8("Napló bejegyzések:");
    text += sHtmlLine;
    // ARP LOG
    par << par.first(); // MAC 2*
    tab = query2html(q, "arp_logs", "hwaddress_new = ? OR hwaddress_old = ? ORDER BY date_of", par);
    if (!tab.isEmpty()) {
        text += QObject::trUtf8("MAC - IP változások (arp_logs) :");
        text += tab + sHtmlLine;
    }
    // MACTAB LOG
    par.pop_back(); // MAC 1*
    tab = query2html(q, "mactab_logs", "hwaddress = ? ORDER BY date_of", par);
    if (!tab.isEmpty()) {
        text += QObject::trUtf8("A MAC mozgása a címtáblákban (mactab_logs) :");
        text += tab + sHtmlLine;
    }
    return text;
}

QString htmlReportByIp(QSqlQuery& q, const QString& sAddr)
{
    QString text;
    QHostAddress a(sAddr);
    if (a.isNull()) {
        text = QObject::trUtf8("A '%1' nem valós IP cím!").arg(sAddr);
        return text;
    }
    cNode node;
    int n = node.fetchByIp(q, a, EX_IGNORE);
    switch (n) {
    case 1:
        text += htmlPair2Text(htmlReportNode(q, node));
        text += sHtmlLine;
        break;
    case 0:
        text += QObject::trUtf8("Nincs bejegyzett hálózati elem ezzel a IP címmel");
        text += sHtmlLine;
        break;
    default:
        text += htmlWarning("Ezzel az IP címmel több hálózati elem is be van jegyezve.");
        do {
            QSqlQuery qq = getQuery();
            text += htmlPair2Text(htmlReportNode(qq, node));
            text += sHtmlLine;
        } while(node.next(q));
    }
    /* ** ARP ** */
    cMac mac = cArp::ip2mac(q, a, EX_IGNORE);
    if (mac.isEmpty()) {
        text += QObject::trUtf8("Nincs találat az arps táblában a megadott IP címmel");
    }
    else {
        text += QObject::trUtf8("Találat a MAC - IP (arps) táblában táblában : %1").arg(mac.toString());
    }
    text += sHtmlLine;
    // LOG
    text += QObject::trUtf8("Napló bejegyzések:");
    text += sHtmlLine;
    // ARP LOG
    QVariantList par;
    par << a.toString();
    QString tab = query2html(q, "arp_logs", "ipaddress = ? ORDER BY date_of", par);
    if (!tab.isEmpty()) {
        text += QObject::trUtf8("IP - MAC változások (arp_logs) :");
        text += tab + sHtmlLine;
    }
    return text;
}

QString linksHtmlTable(QSqlQuery& q, tRecordList<cPhsLink>& list, bool _swap)
{
    QString table;
    table += "\n<table border=\"1\"> ";
    QStringList head;
    head << QObject::trUtf8("Cél port")
         << QObject::trUtf8("Megosztás")
         << QObject::trUtf8("Típus")
         << QObject::trUtf8("Megosztás")
         << QObject::trUtf8("Megjegyzés")
         << QObject::trUtf8("Létrehozva")
         << QObject::trUtf8("Felhasználó")
         << QObject::trUtf8("Modosítva")
         << QObject::trUtf8("Felhasználó");
    table += htmlTableLine(head, "th");
    int i, n = list.size();
    for (i = 0; i < n; ++i) {
        cPhsLink& link = *list[i];
        if (_swap) link.swap();
        cNPort *p = cNPort::getPortObjById(q, link.getId(__sPortId2));
        ePhsLinkType t = (ePhsLinkType)link.getId(_sPhsLinkType2);
        QStringList col;
        col << p->getFullName(q)
            << (p->tableoid() == cNPort::tableoid_pports() ?  p->getName(_sSharedCable) : _sNul)
            << phsLinkType(t)
            << (t == LT_FRONT ? link.getName(_sPortShared) : _sNul)
            << link.getNote()
            << link.getName(_sCreateTime)
            << link.view(q, _sCreateUserId)
            << link.getName(_sModifyTime)
            << link.view(q, _sModifyUserId);
        table += htmlTableLine(col, "td");
        delete p;
    }
    table += "</table>\n";
    return table;
}

/// \brief linkColisionTest
/// \return A link statusz (IS_OK vagy IS_COLLISION)
/// @param q
/// @param exists Ha a link már létezik, akkor értéke true lesz, egyébként false.
/// @param _pl A link rekord
/// @param msg  Üzenet string referencia. HTML stringként ide kerülnek az ötköző linkek.
/// @return Ha van ütközés, akkor true-val tér vissza.
bool linkColisionTest(QSqlQuery& q, bool& exists, const cPhsLink& _pl, QString& msg)
{
    DBGFN();
    msg.clear();
    cPhsLink link;  // Munka objektum
    tRecordList<cPhsLink> list; // Ütközők listája
    bool r;
    exists = false;
    link.collisions(q, list, _pl.getId(_sPortId1), (ePhsLinkType)_pl.getId(_sPhsLinkType1), (ePortShare)_pl.getId(_sPortShared));
    link.collisions(q, list, _pl.getId(_sPortId2), (ePhsLinkType)_pl.getId(_sPhsLinkType2), (ePortShare)_pl.getId(_sPortShared));
    if (list.size()) {
        list.removeDuplicates();    // Beolvasott rekordok, van ID,
        for (int i = 0; i < list.size(); ++i) { // Ha esetleg már létezik a rekord, az is a listában lesz, meg kell keresni
            if (_pl.compare(*list.at(i), true)) {   // nem ID szerinti összehasonlítás, felcserélt iránnyal is
                exists = true;
                delete list.takeAt(i);
            }
        }
    }
    if (exists) {
        msg += htmlInfo(QObject::trUtf8("A megadott link már létezik."));
    }
    if (list.size() > 0) {
        msg += htmlInfo(QObject::trUtf8("A megadott link a következő link(ek)el ütközik:"));
        msg += linksHtmlTable(q, list);
        r = true;
        if (exists) {
            msg = QObject::trUtf8("Létező link nem ütközhet más linkekkel. Adatbázis hiba!\n") + msg;
            EXCEPTION(EDATA, 0, msg);
        }
    }
    else {
        r = false;
    }
    _DBGFNL() << r << endl;
    return r;
}

/* ************************************************************************************************ */

tStringPair htmlReport(QSqlQuery& q, cRecord& o, const QString& _name)
{
    QString name = _name.isEmpty() ? o.tableName() : _name;
    if (name == _sPatchs || name == _sNodes || name == _sSnmpDevices) {
        return htmlReportNode(q, o);
    }
    if (name == _sPlaces) {
        return htmlReportPlace(q, o);
    }
    EXCEPTION(EDATA, 0, QObject::trUtf8("Invalid report type : name = %1; Object : %2").arg(_name, o.identifying()));
    return tStringPair();
}
