#include "report.h"

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
    r.prepend(QObject::tr("Hely : "));
    if (zones) {
        const static QString sql =
                "SELECT place_group_name FROM place_groups"
                " WHERE place_group_id > 1 AND place_group_type = 'zone'::placegrouptype AND is_place_in_zone(?, place_group_id)";
        if (execSql(q, sql, _pid)) {
            r += QObject::tr(" Zónák : ");
            do {
                r += q.value(0).toString() + ", ";
            } while (q.next());
            r.chop(2);
        }
        else {
            r += QObject::tr(" A hely nem része egyetlen zónának sem.");
        }
    }
    if (cat) {
        const static QString sql =
                "SELECT place_group_name FROM place_groups JOIN place_group_places USING(place_group_id)"
                " WHERE place_group_type = 'category'::placegrouptype AND place_id = ?";
        if (execSql(q, sql, _pid)) {
            r += QObject::tr(" Kategória : ");
            do {
                r += q.value(0).toString() + ", ";
            } while (q.next());
            r.chop(2);
        }
        else {
            r += QObject::tr(" Nincs kategória.");
        }
    }
    return r;
}

tStringPair htmlReportPlace(QSqlQuery& q, cRecord& o)
{
    qlonglong pid = o.getId();
    cTableShape shape;
    shape.setByName(q, _sPlaces);
    QString t = QObject::tr("%1 hely vagy helyiség riport.").arg(o.getName());
    QString html = htmlInfo(sReportPlace(q, pid));
    // Groups
    const static QString sql_pg = "SELECT place_groups.* FROM place_group_places JOIN place_groups USING (place_group_id) WHERE place_id = ?";
    if (execSql(q, sql_pg, pid)) {
        html += htmlInfo(QObject::tr("Csoport tagságok :"));
        cPlaceGroup pg;
        tRecordList<cPlaceGroup>    pgl;
        pg.set(q);
        do {
            pgl << pg;
        } while(pg.next(q));
        shape.setByName(_sPlaceGroups);
        html += list2html(q, pgl, shape, _sReport);
    }
    else {
        html += htmlInfo(QObject::tr("A hely ill. helyiség nem tagja egy csoportnak sem."));
    }
    // pachs + nodes
    tRecordList<cPatch> patchs;
    patchs.fetch(q, true, _sPlaceId, pid);
    tRecordList<cNode>  nodes;
    nodes.fetch(q, false, _sPlaceId, pid);
    if (patchs.isEmpty() && nodes.isEmpty()) {
        html += htmlInfo(QObject::tr("A helyiségben nincsenek eszközök."));
    }
    else {
        html += htmlInfo(QObject::tr("A helyiségben lévő eszközök."));
        if (!patchs.isEmpty()) {
            html += htmlInfo(QObject::tr("Patch penelek, vagy fali csatlakozók :"));
            shape.setByName(_sPatchs);
            html += list2html(q, patchs, shape, _sReport);
        }
        if (!nodes.isEmpty()) {
            html += htmlInfo(QObject::tr("Hálózati eszközök :"));
            shape.setByName(_sNodes);
            html += list2html(q, nodes, shape, _sReport);
        }
    }
    tRecordList<cUser>  users;
    users.fetch(q, false, _sPlaceId, pid);
    if (!users.isEmpty()) {
        html += htmlInfo(QObject::tr("Felhasználók :"));
        shape.setByName(_sUsers);
        html += list2html(q, users, shape, _sReport);
    }
    return tStringPair(t, html);
}


/* ------------------------------------------------------------------------------------------------ */

QString titleNode(int t, const cRecord& n)
{
    QString title = n.getName() + " #" + QString::number(n.getId()) + " ";
    switch(t) {
    case NOT_PATCH:
        title += QObject::tr("Patch port vagy fali csatlakozó");
        break;
    case NOT_NODE:
        title += QObject::tr("Passzív vagy aktív hálózati eszköz");
        break;
    case NOT_SNMPDEVICE:
        title += QObject::tr("Aktív SNMP eszköz");
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
    QString s, text;
    QString title;
    cPatch *po   = nodeToOrigin(q, &_node);
    cPatch& node = *(po == nullptr ? dynamic_cast<cPatch *>(&_node) : dynamic_cast<cPatch *>(po));
    QString tablename = node.tableName();
    bool isPatch = tablename == _sPatchs;
    title = _sTitle;
    if (title.isNull()) {      // Default title
        title = titleNode(node);
    }
    else {
        title = _sTitle.arg(node.getName(), tablename);
    }
/*  Nagyon lassú...
    QHostInfo hi = QHostInfo::fromName(node.getName());
    if (!hi.addresses().isEmpty()) {
        s.clear();
        foreach (QHostAddress a, hi.addresses()) {
            s += a.toString() + _sCommaSp;
        }
        s.chop(_sCommaSp.size());
        text += htmlInfo(QObject::tr("IP cím(ek) a (DNS) név alapján : %1 .").arg(s));
    }
*/
    if (!isPatch) {
        text += htmlInfo(QObject::tr("Típus jelzők : %1").arg(node.getName(_sNodeType)));
        text += htmlInfo(QObject::tr("Állapota : %1").arg(node.getName(_sNodeStat)));
    }
    s = _node.getNote();
    if (!s.isEmpty()) text += htmlInfo(QObject::tr("Megjegyzés : %1").arg(s));
    qlonglong pid = _node.getId(_sPlaceId);
    if (pid <= 1) text += htmlInfo(QObject::tr("Az eszköz helye ismeretlen"));
    else text += htmlInfo(sReportPlace(q, pid));
    if (!_node.isNull(_sSerialNumber))    text += htmlInfo(QObject::tr("Gyári szám : ")   + _node.getName(_sSerialNumber));
    if (!_node.isNull(_sInventoryNumber)) text += htmlInfo(QObject::tr("Leltári szám : ") + _node.getName(_sInventoryNumber));
    if (!_node.isNull(_sModelName))       text += htmlInfo(QObject::tr("Model név : ")    + _node.getName(_sModelName));
    if (!_node.isNull(_sModelNumber))     text += htmlInfo(QObject::tr("Model szám : ")   + _node.getName(_sModelNumber));
    /* -- PARAM -- */
    if (flags & CV_NODE_PARAMS) {
        if ((node.containerValid & CV_NODE_PARAMS) == 0) node.fetchParams(q);
        if (node.params.isEmpty()) {
            text += htmlInfo(QObject::tr("Eszköz paraméterek nincsenek."));
        }
        else {
            text += htmlInfo(QObject::tr("Eszköz paraméterek :"));
            text += sHtmlTabBeg + sHtmlRowBeg;
            text += sHtmlTh.arg(QObject::tr("Név"));
            text += sHtmlTh.arg(QObject::tr("Típus"));
            text += sHtmlTh.arg(QObject::tr("Érték"));
            text += sHtmlTh.arg(QObject::tr("Dim."));
            text += sHtmlRowEnd;
            QListIterator<cNodeParam *> li(node.params);
            while (li.hasNext()) {
                 cNodeParam * p = li.next();
                 text += sHtmlRowBeg;
                 text += sHtmlTd.arg(p->getName());
                 text += sHtmlTd.arg(p->typeName());
                 text += sHtmlTd.arg(p->value().toString());
                 text += sHtmlTd.arg(p->dim());
                 text += sHtmlRowEnd;
            }
            text += sHtmlTabEnd;
        }
    }
    /* -- PORTS -- */
    if (flags & CV_PORTS) {
        if ((node.containerValid & CV_PORTS) == 0) node.fetchPorts(q, flags);
        if (node.ports.isEmpty()) {
            text += sHtmlBr + sHtmlBr + QObject::tr("Nincs egyetlen portja sem az adatbázisban az eszköznek.") + "</b>";
        }
        else {
            text += htmlInfo(QObject::tr("Portok :"));
            node.sortPortsByIndex();
            // Table header
            text += sHtmlTabBeg + sHtmlRowBeg;
            text += sHtmlTh.arg(QObject::tr("#"));
            text += sHtmlTh.arg(QObject::tr("Port"));
            text += sHtmlTh.arg(QObject::tr("Cimke"));
            if (!isPatch) {
                text += sHtmlTh.arg(QObject::tr("Típus"));
                text += sHtmlTh.arg(QObject::tr("Trk."));
            }
            text += sHtmlTh.arg(isPatch ? QObject::tr("Shared") : QObject::tr("MAC"));
            text += sHtmlTh.arg(isPatch ? QObject::tr("S.p.")   : QObject::tr("IP cím(ek)"));
            if (!isPatch) text += sHtmlTh.arg(QObject::tr("DNS név"));
            if (isPatch) {
                text += sHtmlTh.arg(QObject::tr("Előlapi link(ek)"));
                text += sHtmlTh.arg(QObject::tr("Hátlapi link"));
            }
            else {
                text += sHtmlTh.arg(QObject::tr("Fizikai link"));
                text += sHtmlTh.arg(QObject::tr("Logikai link"));
                text += sHtmlTh.arg(QObject::tr("LLDP link"));
                text += sHtmlTh.arg(QObject::tr("MAC in mactab"));
            }
            text += sHtmlRowEnd;
            // Table data
            QListIterator<cNPort *> li(node.ports);
            while (li.hasNext()) {
                cNPort * p = li.next();
                cInterface *pInterface = nullptr;
                if (!isPatch && p->descr() == cInterface::_descr_cInterface()) {  // Interface
                    pInterface = p->reconvert<cInterface>();
                }
                cPPort *pPatchPort = nullptr;
                text += sHtmlRowBeg;
                text += sHtmlTd.arg(p->getName(_sPortIndex));   // #
                text += sHtmlTd.arg(p->getName());              // Port (name)
                text += sHtmlTd.arg(p->getName(_sPortTag));     // Cimke (tag)
                if (!isPatch) {
                    text += sHtmlTd.arg(cIfType::ifTypeName(p->getId(_sIfTypeId)));   // Típus (type)
                    if (pInterface != nullptr && !pInterface->isNull(_sPortStapleId)) {
                        qlonglong psid = pInterface->getId(_sPortStapleId);
                        text += sHtmlTd.arg(node.ports.get(psid)->getName());
                    }
                    else {
                        text += sHtmlTd.arg(sVoid);
                    }
                }
                // Columns: port, típus, MAC|Shared, IP|S.p., DNS|-
                if (pInterface != nullptr) {  // Interface
                    QString ips, dns;
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
                    if (spid == NULL_ID) text += sHtmlTd.arg(sVoid);// S.p.
                    else {
                        int ix = node.ports.indexOf(spid);
                        if (ix < 0) text += sHtmlTd.arg(htmlError("!?"));
                        else        text += sHtmlTd.arg(node.ports.at(ix)->getName());
                    }
                }
                else {                                              // NPort
                     text += sHtmlTd.arg(sVoid);
                     text += sHtmlTd.arg(sVoid);
                     text += sHtmlTd.arg(sVoid);
                }
                qlonglong pid = p->getId();
                // Columns: PhsLink, LogLink|-, LLDP|-, MACTab|-
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
                    text += sHtmlTd.arg(sl.isEmpty() ? sVoid : sl.join(sHtmlBr));
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
                        QString cel = sVoid;
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
                    cPhsLink pl;
                    qlonglong plp = LinkGetLinked(q, pl, pid);          // -> phisical link
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
                    // phisical link
                    if (plp == NULL_ID) {
                        text += sHtmlTd.arg(sVoid);
                    }
                    else {
                        QString s = cNPort::getFullNameById(q, plp);
                        ePortShare sh = ePortShare(pl.getId(_sPortShared));
                        if (sh != ES_) {
                            s += "/" + portShare(sh);
                        }
                        ePhsLinkType plt = ePhsLinkType(pl.getId(_sPhsLinkType2));
                        if (plt != LT_TERM) {
                            s += "/" + phsLinkType(plt);
                        }
                        if (sh == ES_) {
                            const cEnumVal& e = cEnumVal::enumVal("phslinktype", plt);
                            s = htmlEnumDecoration(s, e);
                        }
                        else {
                            const cEnumVal& e = cEnumVal::enumVal("portshare", sh);
                            s = htmlEnumDecoration(s, e);
                        }
                        text += sHtmlTd.arg(s);
                    }
                    // logical link
                    if (llp == NULL_ID) text += sHtmlTd.arg(sVoid);
                    else {
                        QString n = cNPort::getFullNameById(q, llp);
                        if (mtp != NULL_ID) {
                            if (mtp != llp) n = sHtmlBRed.arg(n);
                            else            n = sHtmlBGreen.arg(n);
                        }
                        text += sHtmlTd.arg(n);
                    }
                    // lldp
                    if (ldp == NULL_ID) text += sHtmlTd.arg(sVoid);
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
    }
    // Port - VLANs
    if (flags & CV_PORT_VLANS && !isPatch) {
        static const QString sql =
                "SELECT vlan_id, vlan_name"
                " FROM vlan_list_by_host"
                " WHERE node_id = ?"
                " ORDER BY vlan_id ASC";
        QString vlans;
        QStringList head;
        QStringList emptyLine;
        QList<QStringList> matrix;
        QList<int>  vlanIds;
        head      << toHtml(QObject::tr("Port \\ VLAN ID"));
        emptyLine << _sNul;
        if (execSql(q, sql, node.getId())) {
            do {
                vlans += QString("%2(%1), ").arg(q.value(0).toInt()).arg(q.value(1).toString());
                head    << q.value(0).toString();
                vlanIds << q.value(0).toInt();
                emptyLine << _sNul;
            } while (q.next());
            vlans.chop(2);
            text += htmlInfo(QObject::tr("VLAN-ok : ") + vlans);

            if ((node.containerValid & CV_PORTS) == 0) node.fetchPorts(q, flags);
            foreach (cNPort *p, node.ports.list()) {
                if (cInterface::_descr_cInterface() == p->descr()) {
                    cInterface *pi = p->reconvert<cInterface>();
                    if ((pi->containerValid & CV_PORT_VLANS) == 0) pi->fetchVlans(q);
                    if (pi->vlans.isEmpty()) {
                        PDEB(VERBOSE) << "VLAN table : " << p->getName() << " no any VLAN, drop." << endl;
                        continue;
                    }
                    matrix << emptyLine;
                    matrix.last()[0] = toHtml(p->getName());
                    foreach (cPortVlan *pv, pi->vlans.list()) {
                        int vid = int(pv->getId(_sVlanId));
                        int col = vlanIds.indexOf(vid);
                        if (col < 0) {
                            PDEB(DERROR) << "VLAN table : " << p->getName() << " not found " << vid <<", drop." << endl;
                            continue;
                        }
                        int e = int(pv->getId(_sVlanType));
                        const cEnumVal& ev = cEnumVal::enumVal("vlantype", e);
                        QString s = ev.getText(cEnumVal::LTX_VIEW_SHORT, vlanType(e));
                        matrix.last()[col + 1] = htmlEnumDecoration(s, ev);
                    }
                }
                else {
                    PDEB(VERBOSE) << "VLAN table : " << p->getName() << " is not interface, drop." << endl;
                }
            }
            text += htmlTable(head, matrix, false);
        }
        else {
            text += sHtmlBr + sHtmlBr + QObject::tr("Nincs egyetlen VLAN-sem hozzárendelve az eszközhöz.") + "</b>";
        }
    }
    pDelete(po);
    return tStringPair(title, text);
}

QString htmlReportByMac(QSqlQuery& q, const QString& sMac)
{
    QString text;
    cMac mac(sMac);
    if (!mac) {
        text = QObject::tr("A '%1' nem valós MAC!").arg(sMac);
        return text;
    }
    /* ** OUI ** */
    cOui oui;
    if (oui.fetchByMac(q, mac)) {
        text += QObject::tr("OUI rekord : ") + oui.getName() + sHtmlBr;
        text += oui.getNote();
    }
    else {
        text += QObject::tr("Nincs OUI rekord.");
    }
    text += sHtmlLine;
    /* ** NODE ** */
    cNode node;
    int n = node.fetchByMac(q, mac, EX_ERROR);
    switch (n) {
    case 0:
        text += QObject::tr("Nincs bejegyzett hálózati elem ezzel a MAC-kel");
        text += sHtmlLine;
        break;
    case 1:
        text += htmlPair2Text(htmlReportNode(q, node));
        text += sHtmlLine;
        break;
    default:    // ??????
        text += htmlError("Ezzel az MAC címmel több hálózati elem is be van jegyezve.");
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
        text += QObject::tr("Nincs találat az arps táblában a megadott MAC-kel");
    }
    else {
        text += QObject::tr("Találatok a MAC - IP (arps) táblában :");
        text += tab;
    }
    text += sHtmlLine;
    /* ** MAC TAB ** */
    cMacTab mt;
    mt.setMac(_sHwAddress, mac);
    if (mt.completion(q)) {
        text += QObject::tr("Találat a switch cím táblában : <b>%1</b> (%2 - %3; %4)").arg(
                    cNPort::getFullNameById(q, mt.getId(_sPortId)), // Switch port full name
                    mt.view(q, _sFirstTime),
                    mt.view(q, _sLastTime),
                    mt.view(q, _sMacTabState));
    }
    else {
        text += QObject::tr("Nincs találat a switch címtáblákban a megadott MAC-el");
    }
    text += sHtmlLine;
    // LOG
    text += QObject::tr("Napló bejegyzések:");
    text += sHtmlLine;
    // ARP LOG
    par << par.first(); // MAC 2*
    tab = query2html(q, "arp_logs", "hwaddress_new = ? OR hwaddress_old = ? ORDER BY date_of", par);
    if (!tab.isEmpty()) {
        text += QObject::tr("MAC - IP változások (arp_logs) :");
        text += tab + sHtmlLine;
    }
    // MACTAB LOG
    par.pop_back(); // MAC 1*
    tab = query2html(q, "mactab_logs", "hwaddress = ? ORDER BY date_of", par);
    if (!tab.isEmpty()) {
        text += QObject::tr("A MAC mozgása a címtáblákban (mactab_logs) :");
        text += tab + sHtmlLine;
    }
    return text;
}

QString htmlReportByIp(QSqlQuery& q, const QString& sAddr)
{
    QString text;
    QHostAddress a(sAddr);
    if (a.isNull()) {
        text = QObject::tr("A '%1' nem valós IP cím!").arg(sAddr);
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
        text += QObject::tr("Nincs bejegyzett hálózati elem ezzel a IP címmel");
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
        text += QObject::tr("Nincs találat az arps táblában a megadott IP címmel");
    }
    else {
        text += QObject::tr("Találat a MAC - IP (arps) táblában táblában : %1").arg(mac.toString());
    }
    text += sHtmlLine;
    // LOG
    text += QObject::tr("Napló bejegyzések:");
    text += sHtmlLine;
    // ARP LOG
    QVariantList par;
    par << a.toString();
    QString tab = query2html(q, "arp_logs", "ipaddress = ? ORDER BY date_of", par);
    if (!tab.isEmpty()) {
        text += QObject::tr("IP - MAC változások (arp_logs) :");
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
    head << QObject::tr("ID")
         << QObject::tr("Port")
         << QObject::tr("Típus")

         << QObject::tr("Cél port")
         << QObject::tr("Típus")

         << QObject::tr("Létrehozva")
         << QObject::tr("Felhasználó")

         << QObject::tr("Megjegyzés");
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
    link.collisions(q, list, _pl.getId(_sPortId1), ePhsLinkType(_pl.getId(_sPhsLinkType1)), ePortShare(_pl.getId(_sPortShared)));
    link.collisions(q, list, _pl.getId(_sPortId2), ePhsLinkType(_pl.getId(_sPhsLinkType2)), ePortShare(_pl.getId(_sPortShared)));
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
        msg += htmlInfo(QObject::tr("A megadott link már létezik."));
    }
    if (list.size() > 0) {
        msg += htmlInfo(QObject::tr("A megadott link a következő link(ek)el ütközik:"));
        msg += linksHtmlTable(q, list);
        r = true;
        if (exists) {
            msg = QObject::tr("Létező link nem ütközhet más linkekkel. Adatbázis hiba!\n") + msg;
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
    if (_type == LT_TERM || _pid == NULL_ID) {      // Endpoint or null
        if (_pid != NULL_ID) endMap[_sh] = _pid;    // End point
        return msg; // Null message (empty list)
    }
    cPhsLink link;  // Working record
    tRecordList<cPhsLink> list;
    QList<ePortShare>   shares;
    QPair<int, ePortShare>  branch;
    QList<QPair<int, ePortShare> >  branchLst;
    qlonglong pid = _pid;
    ePhsLinkType type;
    ePortShare  sh;
    int         index = 0;
    bool        isBranch = false;

    cPPort pport;
    pport.setById(q, _pid);
    sh = shareResultant(_sh, (ePortShare)pport.getId(_sSharedCable));
    link.setId(_sPortId1, _pid);
    link.setId(_sPhsLinkType1, _type);
    list    << link;   // Starting point is not part of the list to be written.
    shares  << sh;
    branch.first = 0;
    branch.second = sh;
    branchLst << branch;

    while (!branchLst.isEmpty()) {      // There is still a branch
        branch = branchLst.takeFirst();
        index  = branch.first;
        sh     = branch.second;
        PDEB(INFO) << QString("*** #%1 '%2'").arg(index).arg(portShare(sh)) << endl;
        while (true) {
            link.copy(*list.at(index));
            link.swap();
            type = ePhsLinkType(link.getId(_sPhsLinkType2));
            PDEB(INFO) << QString("* (%1) %2/%3 -> %4/%5  #%6 -> #%7")
                              .arg(portShare(sh))
                              .arg(link.view(q, _sPortId1), link.getName(_sPhsLinkType1))
                              .arg(link.view(q, _sPortId2), link.getName(_sPhsLinkType2))
                              .arg(link.getName(_sPortId1), link.getName(_sPortId2))
                           << endl;
            pid  = link.getId(_sPortId2);
            if (!link.nextLink(q, pid, type, sh)) break;    // Get next link: There is no such
            // Link data is not original record !! Modifyed port_shared and phs_link_note field.
            PDEB(INFO) << QString(" NEXT#%9 (%1) %2/%3 -> %4/%5  #%6 -> #%7  '%8'")
                              .arg(link.getName(_sPortShared))
                              .arg(link.view(q, _sPortId1), link.getName(_sPhsLinkType1))
                              .arg(link.view(q, _sPortId2), link.getName(_sPhsLinkType2))
                              .arg(link.getName(_sPortId1), link.getName(_sPortId2))
                              .arg(link.getNote()).arg(index)
                           << endl;
            QString bs = link.getNote();    // branch, share types list (see: cPhsLink::nextLink() and next_patch() plpgsql func.)
            sh = ePortShare(link.getId(_sPortShared));          // result share
            type = ePhsLinkType(link.getId(_sPhsLinkType2));    // next node link type
            cPhsLink *po = new cPhsLink;
            po->setId(link.getId());                    // Two view record !!
            po->set(_sPortId2, link.get(_sPortId1));    // Determine orientation (and swap)
            po->completion(q);              // Get original record
            list   << po;                   // Push record to list
            shares << sh;                   // Push share to list
            if (!bs.isEmpty()) {
                isBranch = true;
                foreach (QString ssh, bs.split(QChar(','))) {
                    branch.first  = index;
                    branch.second = ePortShare(portShare(ssh));;
                    branchLst << branch;
                }
            }
            if (type == LT_TERM) {
                PDEB(INFO) << "Term ..." << endl;
                endMap[sh] = link.getId(_sPortId2);
                break;
            }
            index = list.size() -1;
        }
    }
    if (list.size() > 1) {
        delete list.pop_front();    // Drop first row
        shares.pop_front();
        int terminalCount = endMap.size();
        switch(terminalCount) {
        case 0:
            if (isBranch) msg = htmlInfo(QObject::tr("A linkek lánca elágazik, mind csonka, nem ér el a végpontig :"));
            else          msg = htmlInfo(QObject::tr("A linkek lánca, csonka, nem ér el a végpontig :"));
            break;
        case 1:
            if (isBranch) msg = htmlInfo(QObject::tr("A linkek lánca elágazik, egy végpont van:"));
            else          msg = htmlInfo(QObject::tr("A linkek lánca a végpontig:"));
            break;
        default:
            if (!isBranch) EXCEPTION(EPROGFAIL); // :-O
            msg = htmlInfo(QObject::tr("A linkek lánca elágazik, több végpont van:"));
            break;
        }
        QStringList verticalHeader;
        if (shares.size() != list.size()) EXCEPTION(EPROGFAIL);
        for (int i = 0; i < shares.size(); ++i) {
            sh = shares[i];
            QString vh = "#" + QString::number(i);
            if (sh != ES_) vh += "/" + portShare(sh);
            verticalHeader << vh;
        }
        msg += linksHtmlTable(q, list, false, verticalHeader);
    }
    else {
        msg = htmlInfo(QObject::tr("Nincs további link, vége a láncnak."));
    }
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
                msg = QObject::tr("Inkonzisztens logikai link tábla. Hiányzó link : %1 <==> %2\n").arg(cNPort::getFullNameById(q2, _pid1), cNPort::getFullNameById(q2, _pid2));
                critical = true;
            }
            break;
        case 1:
            epid = llnk.getId(_sPortId1);
            if (epid != pids.second) {
                if (saved) {
                    msg = QObject::tr("Inkonzisztens logikai link tábla. Hiányzó link : %1 <==> %2, és ütközés :\n").arg(cNPort::getFullNameById(q2, _pid1), cNPort::getFullNameById(q2, _pid2));
                    critical = true;
                }
                else {
                    PDEB(INFO) << "Log. coll. : " << cNPort::getFullNameById(q, epid) << endl;
                    msg += htmlError(msgPref + QObject::tr("A (törlendő) logikai linkek : ").arg(cNPort::getFullNameById(q, epid)));
                }
                msg += logLink2str(q2, llnk);
            }
            break;
        default:
            msg = QObject::tr("Inkonzisztens logikai link tábla, egy porthoz több link is tartozik!\n");
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
            msg += htmlError(msgPref + QObject::tr("A végpont ütköző linkje az LLDP alapján : %1").arg(cNPort::getFullNameById(q, epid)));
        }
        else {
            eq = true;
            msg += htmlGrInf(msgPref + QObject::tr("Megeggyező LLDP link."));
        }
    }
    if (!eq) {
        ldnl.clear();
        ldnl.setId(_sPortId1, _pid1);
        if (ldnl.completion(q)) {
            epid = ldnl.getId(_sPortId2);
            if (epid != _pid2) {
                PDEB(INFO) << "LLDP coll. : " << cNPort::getFullNameById(q, epid) << endl;
                msg += htmlError(msgPref + QObject::tr("Az ütköző link az LLDP alapján : %1").arg(cNPort::getFullNameById(q, epid)));
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
                msg = htmlGrInf(msgPref + QObject::tr("A Link megerősítve a MAC címtáblák alapján."));
            }
            else {
                msg = htmlError(msgPref + QObject::tr("A Link ütközik a MAC címtáblákkal, talált port %1 .").arg(cNPort::getFullNameById(q, mtp)));
            }
        }
//      else {
//          msg = htmlInfo(msgPref + QObject::tr("A Link nincs megerősítve a MAC címtáblák alapján."));
//      }
    }
    return msg;
}

/* ************************************************************************************************ */

tStringPair htmlReport(QSqlQuery& q, cRecord& o, const cTableShape& shape)
{
    static QString s = " : ";
    QString t = QObject::tr("%1 riport.").arg(shape.getText(cTableShape::LTX_DIALOG_TITLE));
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

