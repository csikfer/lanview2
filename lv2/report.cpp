#include "report.h"
#include "guidata.h"

const QString sHtmlLine   = "\n<hr>\n";
const QString sHtmlTabBeg = "<table border=\"1\" cellpadding=\"2\" cellspacing=\"2\">";
const QString sHtmlTabEnd = "</table>\n";
const QString sHtmlRowBeg = "<tr>";
const QString sHtmlRowEnd = "</tr>\n";
const QString sHtmlTh     = "<th> %1 </th>";
const QString sHtmlTd     = "<td> %1 </td>";
const QString sHtmlBold   = "<b>%1</b>";
const QString sHtmlItalic = "<i>%1</i>";
const QString sHtmlVoid   = " - ";
const QString sHtmlBr     = "<br>\n";
const QString sHtmlBRed   = "<b><span style=\"color:red\"> %1 </span></b>";
const QString sHtmlBGreen = "<b><span style=\"color:green\"> %1 </span></b>";
const QString sHtmlNbsp   = " &nbsp; ";


QString toHtml(const QString& text, bool chgBreaks, bool esc)
{
    static const QChar   cr = QChar('\n');
    QString r = text;
    if (chgBreaks)  r = r.trimmed();
    if (esc)        r = r.toHtmlEscaped();
    if (chgBreaks)  r = r.replace(cr, sHtmlBr);
    return r;
}


QString htmlTableLine(const QStringList& fl, const QString& ft, bool esc)
{
    QString r = tag("tr");
    foreach (QString f, fl) {
        r += tag(ft.isEmpty() ? "td" : ft);
        r += esc ? f.toHtmlEscaped() : f;
        r += tag("/" + ft);
    }
    r += tag("/tr");
    return r + "\n";
}

QString htmlTable(QStringList head, QList<QStringList> matrix, bool esc)
{
    QString table;
    table += "\n<table border=\"1\"> ";
    if (!head.isEmpty()) table += htmlTableLine(head, "th", esc);
    foreach (QStringList line, matrix) {
        table += htmlTableLine(line, "td", esc);
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
        const static QString sql =
                "SELECT place_group_name FROM place_groups"
                " WHERE place_group_id > 1 AND place_group_type = 'zone'::placegrouptype AND is_place_in_zone(?, place_group_id)";
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
        const static QString sql =
                "SELECT place_group_name FROM place_groups JOIN place_group_places USING(place_group_id)"
                " WHERE place_group_type = 'category'::placegrouptype AND place_id = ?";
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
        if (!nodes.isEmpty()) {
            html += htmlInfo(QObject::trUtf8("Hálózati eszközök :"));
            shape.setByName(_sNodes);
            html += list2html(q, nodes, shape);
        }
    }
    tRecordList<cUser>  users;
    users.fetch(q, false, _sPlaceId, pid);
    if (!users.isEmpty()) {
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

// Hiányos, kéne használni a tableShape rekordot
tStringPair htmlReportNode(QSqlQuery& q, cRecord& _node, const QString& _sTitle, qlonglong flags)
{
    QString text;
    QString title;
    cPatch *po   = nodeToOrigin(q, &_node);
    cPatch& node = *(po == nullptr ? dynamic_cast<cPatch *>(&_node) : dynamic_cast<cPatch *>(po));
    if (po != nullptr) {
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
    if (!_node.isNull(_sSerialNumber))    text += htmlInfo(QObject::trUtf8("Gyári szám : ")   + _node.getName(_sSerialNumber));
    if (!_node.isNull(_sInventoryNumber)) text += htmlInfo(QObject::trUtf8("Leltári szám : ") + _node.getName(_sInventoryNumber));
    if (!_node.isNull(_sModelName))       text += htmlInfo(QObject::trUtf8("Model név : ")    + _node.getName(_sModelName));
    if (!_node.isNull(_sModelNumber))     text += htmlInfo(QObject::trUtf8("Model szám : ")   + _node.getName(_sModelNumber));
    /* -- PARAM -- */
    if (flags & CV_NODE_PARAMS) {
        if ((node.containerValid & CV_NODE_PARAMS) == 0) node.fetchParams(q);
        if (node.params.isEmpty()) {
            text += htmlInfo(QObject::trUtf8("Eszköz paraméterek nincsenek."));
        }
        else {
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
        if (isPatch) {
            text += sHtmlTh.arg(QObject::trUtf8("Előlapi link(ek)"));
            text += sHtmlTh.arg(QObject::trUtf8("Hátlapi link"));
        }
        else {
            text += sHtmlTh.arg(QObject::trUtf8("Fizikai link"));
            text += sHtmlTh.arg(QObject::trUtf8("Logikai link"));
            text += sHtmlTh.arg(QObject::trUtf8("LLDP link"));
            text += sHtmlTh.arg(QObject::trUtf8("MAC in mactab"));
        }
        text += sHtmlRowEnd;
        // Table data
        QListIterator<cNPort *> li(node.ports);
        while (li.hasNext()) {
            cNPort * p = li.next();
            cInterface *pInterface = nullptr;
            cPPort *pPatchPort = nullptr;
            text += sHtmlRowBeg;
            text += sHtmlTd.arg(p->getName(_sPortIndex));   // #
            text += sHtmlTd.arg(p->getName());              // Port (name)
            text += sHtmlTd.arg(p->getName(_sPortTag));     // Cimke (tag)
            if (!isPatch) text += sHtmlTd.arg(cIfType::ifTypeName(p->getId(_sIfTypeId)));   // Típus (type)
            // Columns: port, típus, MAC|Shared, IP|S.p., DNS|-
            if (p->descr() == cInterface::_descr_cInterface()) {  // Interface
                QString ips, dns;
                pInterface = p->reconvert<cInterface>();
                QListIterator<cIpAddress *> ii(pInterface->addresses);
                while (ii.hasNext()) {
                    cIpAddress * ia = ii.next();
                    ips += sHtmlBold.arg(ia->view(q, _sAddress)) + "/" + ia->getName(_sIpAddressType) + _sCommaSp;
                    QHostInfo hi = QHostInfo::fromName(ia->address().toString());
                    dns += hi.hostName() + _sCommaSp;
                }
                ips.chop(_sCommaSp.size());
                dns.chop(_sCommaSp.size());
                text += sHtmlTd.arg(pInterface->mac().toString());
                text += sHtmlTd.arg(ips);
                text += sHtmlTd.arg(dns);
            }
            else if (isPatch) {                                 // PPort
                pPatchPort = p->reconvert<cPPort>();
                text += sHtmlTd.arg(pPatchPort->getName(_sSharedCable));    // Shared
                qlonglong spid = pPatchPort->getId(_sSharedPortId);
                if (spid == NULL_ID) text += sHtmlTd.arg(sHtmlVoid);// S.p.
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
                // Front link(s)
                pl.setId(_sPortId1, pid);
                pl.setId(_sPhsLinkType1, LT_FRONT);
                int n = pl.completion(q, pl.iTab(_sPortShared));
                QStringList sl;
                if (n > 0) {
                    tRecordList<cPhsLink>   list;
                    list.set(q);
                    while (1) {
                        cPhsLink *ppl = list.pop_front(EX_IGNORE);
                        if (ppl == nullptr) break;
                        QString sh = ppl->getName(_sPortShared);
                        QString sp = cNPort::getFullNameById(q, ppl->getId(_sPortId2));
                        sl << (sh.isEmpty() ? sp : (sh + ":" + sp));
                    }
                }
                text += sHtmlTd.arg(sl.isEmpty() ? sHtmlVoid : sl.join(sHtmlBr));
                // Back link
                pl.clear();
                pl.setId(_sPortId1, pid);
                pl.setId(_sPhsLinkType1, LT_BACK);
                n = pl.completion(q);
                sl.clear();
                QSqlQuery q2 = getQuery();
                if (n > 0) {
                    do {
                        qlonglong pid2 = pl.getId(_sPortId2);
                        QString sh;
                        cPPort pp;
                        if (pp.fetchById(q2, pid2)) {
                            sh = pp.getName(_sSharedCable);
                            if (!sh.isEmpty()) sh += ":";
                        }
                        QString sp = cNPort::getFullNameById(q2, pid2);
                        sl << (sh + sp);
                    } while (pl.next(q));
                    if (n == 1) text += sHtmlTd.arg(sl.join(sHtmlBr));
                    else        text += sHtmlTd.arg(htmlError(sl.join(QChar('\n')), true));
                }
                else {
                    QString cel = sHtmlVoid;
                    qlonglong spid = pPatchPort->getId(_sSharedPortId);
                    if (spid != NULL_ID) {
                        static const QString sql =
                                "SELECT pp2.shared_cable || ':' || port_id2full_name(pp2.port_id)"
                                " FROM phs_links JOIN pports AS pp2 ON (pp2.shared_port_id = port_id2) "
                                " WHERE port_id1 = ? AND pp2.shared_cable = ?";
                        if (execSql(q, sql, spid, pPatchPort->getName(_sSharedCable))) {
                            cel =  sHtmlItalic.arg(q.value(0).toString());
                        }
                    }
                    text += sHtmlTd.arg(cel);
                }
            }
            else {
                qlonglong plp = LinkGetLinked<cPhsLink>(q, pid);    // -> phisical link
                qlonglong llp = LinkGetLinked<cLogLink>(q, pid);    // -> logical link
                qlonglong ldp = LinkGetLinked<cLldpLink>(q, pid);   // -> LLDP
                qlonglong mtp = NULL_ID;                            // -> port ID: MAC in MacTab
                if (pInterface != nullptr) {  // Interface ?
                    cMacTab mt;
                    mt.setMac(_sHwAddress, pInterface->getMac(_sHwAddress));
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

QString linksHtmlTable(QSqlQuery& q, tRecordList<cPhsLink>& list, bool _swap, const QStringList _verticalHeader)
{
    QString table;
    table += "\n<table border=\"1\"> ";
    QStringList head;
    if (!_verticalHeader.isEmpty()) head << "N";
    head << QObject::trUtf8("ID")
         << QObject::trUtf8("Port")
         << QObject::trUtf8("Típus")

         << QObject::trUtf8("Cél port")
         << QObject::trUtf8("Típus")

         << QObject::trUtf8("Létrehozva")
         << QObject::trUtf8("Felhasználó")
         << QObject::trUtf8("Modosítva")
         << QObject::trUtf8("Felhasználó")

         << QObject::trUtf8("Megjegyzés");
    table += htmlTableLine(head, "th");
    int i, n = list.size();
    for (i = 0; i < n; ++i) {
        cPhsLink& link = *list[i];
        if (_swap) link.swap();
        cNPort *p1 = cNPort::getPortObjById(q, link.getId(__sPortId1));
        cNPort *p2 = cNPort::getPortObjById(q, link.getId(__sPortId2));
        ePhsLinkType t1 = ePhsLinkType(link.getId(_sPhsLinkType1));
        ePhsLinkType t2 = ePhsLinkType(link.getId(_sPhsLinkType2));
        ePortShare   sh = ePortShare(link.getId(_sPortShared));
        ePortShare   psh;
        qlonglong    sp;
        QString type1, type2;
        switch (t1) {
        case LT_TERM:
            type1 = _sTerm;
            break;
        case LT_FRONT:
            type1 = _sFront;
            if (sh != ES_) {
                type1 += "/" + portShare(sh);
                if (t2 == LT_FRONT) type1 = sHtmlBRed.arg(type1);
            }
            break;
        case LT_BACK:
            type1 = _sBack;
            psh = ePortShare(p1->getId(_sSharedCable));
            if (psh != ES_) {
                type1 += "/" + portShare(psh);
                sp = p1->getId(_sSharedPortId);
                if (sp != NULL_ID) {
                    type1 += "/" + cNPort().getNameById(q, sp);
                }
            }
            break;
        default:
            type1 = sHtmlBRed.arg("??");
            break;
        }
        switch (t2) {
        case LT_TERM:
            type2 = _sTerm;
            break;
        case LT_FRONT:
            type2 = _sFront;
            if (sh != ES_) {
                type2 += "/" + portShare(sh);
                if (t1 == LT_FRONT) type2 = sHtmlBRed.arg(type1);
            }
            break;
        case LT_BACK:
            type2 = _sBack;
            psh = ePortShare(p2->getId(_sSharedCable));
            if (psh != ES_) {
                type2 += "/" + portShare(psh);
                sp = p2->getId(_sSharedPortId);
                if (sp != NULL_ID) {
                    type2 += "/" + cNPort().getNameById(q, sp);
                }
            }
            break;
        default:
            type1 = sHtmlBRed.arg("??");
            break;
        }

        QStringList col;
        if (!_verticalHeader.isEmpty()) col << _verticalHeader[i];
        col << link.view(q, link.idIndex())
            << toHtml(p1->getFullName(q))
            << type1
            << toHtml(p2->getFullName(q))
            << type2
            << toHtml(link.getName(_sCreateTime))
            << link.view(q, _sCreateUserId)
            << toHtml(link.getName(_sModifyTime))
            << link.view(q, _sModifyUserId)
            << toHtml(link.getNote());
        table += htmlTableLine(col, "td", false);
        delete p2;
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

QString linkChainReport(QSqlQuery& q, qlonglong _pid, ePhsLinkType _type, ePortShare _sh, QMap<ePortShare, qlonglong>& endMap)
{
    QString msg;
    endMap.clear();
    if (_type == LT_TERM || _pid == NULL_ID) {
        endMap[_sh] = _pid;
        return msg;
    }
    cPhsLink link;
    tRecordList<cPhsLink> list;
    QList<ePortShare>   shares;
    QList<int>          indexes;
    QMap<int, QString>  branchMap;
    qlonglong pid = _pid;
    ePhsLinkType type;
    ePortShare  sh;
    int         index;
    bool        isBranch = false;

    QString ssh = portShare(_sh);
    link.setId(_sPortId2, _pid);
    link.setId(_sPhsLinkType2, _type);
    list    << link;   // Starting point is not part of the list to be written.
    indexes << 0;
    shares  << _sh;
    branchMap[0] = ssh; // [list/index] <= share

    while (!branchMap.isEmpty()) {
        QMap<int, QString>  bm = branchMap;
        branchMap.clear();
        QList<int> bi = bm.keys();
        std::sort(bi.begin(), bi.end());
        PDEB(INFO) << "**** branchMap : " << numListToString(bi) << endl;
        foreach (index, bi) {
            ssh = bm[index];
            PDEB(INFO) << QString("*** %1").arg(ssh) << endl;
            QStringList sshl = ssh.split(QChar(','));
            foreach (ssh, sshl) {
                PDEB(INFO) << QString("** #%1 / '%2'").arg(index).arg(ssh) << endl;
                link.copy(*list.at(index));
                sh = ePortShare(portShare(ssh));
                type = ePhsLinkType(link.getId(_sPhsLinkType2));
                while (true) {
                    PDEB(INFO) << QString("* (%1) %2/%3 -> %4/%5  #%6 -> #%7")
                                      .arg(portShare(sh))
                                      .arg(link.view(q, _sPortId1), link.getName(_sPhsLinkType1))
                                      .arg(link.view(q, _sPortId2), link.getName(_sPhsLinkType2))
                                      .arg(link.getName(_sPortId1), link.getName(_sPortId2))
                                   << endl;
                    pid  = link.getId(_sPortId2);
                    if (!link.nextLink(q, pid, type, sh)) break;
                    ++index;
                    PDEB(INFO) << QString(" NEXT#%9 (%1) %2/%3 -> %4/%5  #%6 -> #%7  '%8'")
                                      .arg(link.getName(_sPortShared))
                                      .arg(link.view(q, _sPortId1), link.getName(_sPhsLinkType1))
                                      .arg(link.view(q, _sPortId2), link.getName(_sPhsLinkType2))
                                      .arg(link.getName(_sPortId1), link.getName(_sPortId2))
                                      .arg(link.getNote()).arg(index)
                                   << endl;
                    QString bs = link.getNote();                // branch
                    sh = ePortShare(link.getId(_sPortShared));  // result share
                    cPhsLink *po = new cPhsLink;
                    po->setById(q, link.getId()); // Get original record
                    list    << po;
                    shares  << sh;
                    indexes << index;
                    type = ePhsLinkType(link.getId(_sPhsLinkType2));
                    if (!bs.isEmpty()) {
                        PDEB(INFO) << "Branch ..." << endl;
                        branchMap[list.size() -2] = bs; // Branching to the previous item
                        isBranch = true;
                    }
                    if (type == LT_TERM) {
                        PDEB(INFO) << "Term ..." << endl;
                        endMap[sh] = link.getId(_sPortId2);
                        break;
                    }
                }
            }
        }
    }
    if (list.size() > 1) {
        delete list.pop_front();    // Drop first row
        indexes.pop_front();
        shares.pop_front();
        int terminalCount = endMap.size();
        switch(terminalCount) {
        case 0:
            if (isBranch) msg = htmlInfo(QObject::trUtf8("A linkek lánca elágazik, mind csonka, nem ér el a végpontig :"));
            else          msg = htmlInfo(QObject::trUtf8("A linkek lánca, csonka, nem ér el a végpontig :"));
            break;
        case 1:
            if (isBranch) msg = htmlInfo(QObject::trUtf8("A linkek lánca elágazik, egy végpont van:"));
            else          msg = htmlInfo(QObject::trUtf8("A linkek lánca a végpontig:"));
            break;
        default:
            if (!isBranch) EXCEPTION(EPROGFAIL); // :-O
            msg = htmlInfo(QObject::trUtf8("A linkek lánca elágazik, több végpont van:"));
            break;
        }
        QStringList verticalHeader;
        if (indexes.size() != shares.size() || indexes.size() != list.size()) EXCEPTION(EPROGFAIL);
        for (int i = 0; i < indexes.size(); ++i) {
            sh = shares[i];
            QString vh = "#" + QString::number(indexes[i]);
            if (sh != ES_) vh += "/" + portShare(sh);
            verticalHeader << vh;
        }
        msg += linksHtmlTable(q, list, false, verticalHeader);
    }
    if (pid != NULL_ID) _sh = ePortShare(portShare(ssh));
    return msg;
}

QString linkEndEndLogReport(QSqlQuery& q, qlonglong _pid1, qlonglong _pid2, bool saved, const QString& msgPref)
{
    QString msg;
    if (_pid1 == NULL_ID || _pid2 == NULL_ID) {
        if (saved) EXCEPTION(EPROGFAIL);
        return msg;   // Is't valid port ID.
    }
    PDEB(INFO) << "linkEndEndLogReport(," << cNPort::getFullNameById(q, _pid1) << _sCommaSp << cNPort::getFullNameById(q, _pid2) << ")" << endl;
    // Check logical link
    qlonglong epid;
    QPair<qlonglong, qlonglong> pids;
    pids.first  = _pid1;
    pids.second = _pid2;
    cLogLink  llnk;
    int n = 2;
    QSqlQuery q2 = getQuery();
    bool critical = false;
    while (n) {  // 2*
        llnk.setId(_sPortId2, pids.first);
        switch (llnk.completion(q)) {
        case 0:
            if (saved && n == 2) {    //
                msg = QObject::trUtf8("Inkonzisztens logikai link tábla. Hiányzó link : %1 <==> %2\n").arg(cNPort::getFullNameById(q2, _pid1), cNPort::getFullNameById(q2, _pid2));
                critical = true;
            }
            break;
        case 1:
            epid = llnk.getId(_sPortId1);
            if (epid != pids.second) {
                if (saved) {
                    msg = QObject::trUtf8("Inkonzisztens logikai link tábla. Hiányzó link : %1 <==> %2, és ütközés :\n").arg(cNPort::getFullNameById(q2, _pid1), cNPort::getFullNameById(q2, _pid2));
                    critical = true;
                }
                else {
                    PDEB(INFO) << "Log. coll. : " << cNPort::getFullNameById(q, epid) << endl;
                    msg += htmlError(msgPref + QObject::trUtf8("A (törlendő) logikai linkek : ").arg(cNPort::getFullNameById(q, epid)));
                }
                msg += logLink2str(q2, llnk);
            }
            break;
        default:
            msg = QObject::trUtf8("Inkonzisztens logikai link tábla, egy porthoz több link is tartozik!\n");
            do {
                msg += logLink2str(q2, llnk);
            } while (llnk.next(q));
            critical = true;
            break;
        }
        if (--n) {
            pids.first  = _pid2;
            pids.second = _pid1;
        }
    }
    if (critical) {
        APPMEMO(q, msg, RS_CRITICAL);
        msg = htmlError(msg, true);
    }
    else {
        msg = toHtml(msg, true);
    }

    cLldpLink ldnl;
    ldnl.setId(_sPortId2, _pid2);
    bool eq = false;
    if (ldnl.completion(q)) {
        epid = ldnl.getId(_sPortId1);
        if (epid != _pid1) {
            msg += htmlError(msgPref + QObject::trUtf8("A végpont ütköző linkje az LLDP alapján : %1").arg(cNPort::getFullNameById(q, epid)));
        }
        else {
            eq = true;
            msg += htmlGrInf(msgPref + QObject::trUtf8("Megeggyező LLDP link."));
        }
    }
    if (!eq) {
        ldnl.clear();
        ldnl.setId(_sPortId1, _pid1);
        if (ldnl.completion(q)) {
            epid = ldnl.getId(_sPortId2);
            if (epid != _pid2) {
                PDEB(INFO) << "LLDP coll. : " << cNPort::getFullNameById(q, epid) << endl;
                msg += htmlError(msgPref + QObject::trUtf8("Az ütköző link az LLDP alapján : %1").arg(cNPort::getFullNameById(q, epid)));
            }
        }
    }
    return msg;
}

QString linkEndEndMACReport(QSqlQuery& q, qlonglong _pid1, qlonglong _pid2, const QString& msgPref)
{
    QString msg;
    if (_pid1 == NULL_ID || _pid2 == NULL_ID) return msg;   // Is't valid port ID.
    cInterface pif;
    if (pif.fetchById(q, _pid1) && pif.getMac(_sHwAddress).isValid()) {
        cMacTab mt;
        mt.setMac(_sHwAddress, pif.getMac(_sHwAddress));
        if (mt.completion(q)) {
            qlonglong mtp = mt.getId(_sPortId);
            if (_pid2 == mtp) {
                msg = htmlGrInf(msgPref + QObject::trUtf8("A Link megerősítve a MAC címtáblák alapján."));
            }
            else {
                msg = htmlError(msgPref + QObject::trUtf8("A Link ütközik a MAC címtáblákkal, talált port %1 .").arg(cNPort::getFullNameById(q, mtp)));
            }
        }
//      else {
//          msg = htmlInfo(msgPref + QObject::trUtf8("A Link nincs megerősítve a MAC címtáblák alapján."));
//      }
    }
    return msg;
}

/* ************************************************************************************************ */

tStringPair htmlReport(QSqlQuery& q, cRecord& o, const cTableShape& shape)
{
    static QString s = " : ";
    QString t = QObject::trUtf8("%1 riport.").arg(shape.getText(cTableShape::LTX_DIALOG_TITLE));
    QString html;
    foreach (cTableShapeField * pf, (QList<cTableShapeField *>)shape.shapeFields) {
        static const int ixFieldFlag = pf->toIndex(_sFieldFlags);
        if (pf->getBool(ixFieldFlag, FF_HTML)) {
            bool huge = pf->getBool(ixFieldFlag, FF_HUGE);
            QString t = pf->getText(cTableShapeField::LTX_DIALOG_TITLE) + s;
            QString v = pf->view(q, o);
            if (huge) {
                html += htmlWarning(t);   // bold
                html += htmlInfo(v, true);
            }
            else {
                html += htmlInfo(toHtmlBold(t) + toHtml(v), false, false);
            }
        }
    }
    return tStringPair(t, html);

}

tStringPair htmlReport(QSqlQuery& q, cRecord& o, const QString& _name, const cTableShape * pShape)
{
    QString name = _name.isEmpty() ? o.tableName() : _name;
    // Custom
    if (name == _sPatchs || name == _sNodes || name == _sSnmpDevices) {
        return htmlReportNode(q, o);
    }
    if (name == _sPlaces) {
        return htmlReportPlace(q, o);
    }
    // General
    cTableShape shape;
    if (pShape == nullptr) {
        shape.setByName(q, name);
        shape.fetchFields(q);
        shape.fetchText(q);
        pShape = &shape;
    }
    return htmlReport(q, o, *pShape);
}
