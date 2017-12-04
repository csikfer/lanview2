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
    table += htmlTableLine(head, "th");
    foreach (QStringList line, matrix) {
        table += htmlTableLine(line, "td");
    }
    table += "</table>\n";
    return table;
}

QString query2html(QSqlQuery q, cTableShape &_shape, const QString& _where, const QVariantList& _par, bool shrt)
{
    cRecordAny rec(_shape.getName(_sTableName));
    tRecordList<cRecordAny> list;
    QString sql = QString("SELECT * FROM %1 WHERE ").arg(rec.tableName()) + _where;
    if (execSql(q, sql, _par)) do {
        rec.set(q);
        list << rec;
    } while (q.next());
    if (list.isEmpty()) return QString();
    return list2html(q, list, _shape, shrt);
}

// Át kéne írni, hogy a table_shapes rekordok alapján írja ki a riportot, ne fixen.
QString htmlReportNode(QSqlQuery& q, cPatch& node, const QString& _sTitle, bool ports)
{
    QString text;
    QString tablename = node.getOriginalDescr(q)->tableName();
    bool isPatch = tablename == _sPatchs;
    QString sTitle = _sTitle;
    if (sTitle.isNull()) {
        sTitle = isPatch ?
                    QObject::trUtf8("Bejegyzett patch panel, vagy csatlakozó : %1"):
                    QObject::trUtf8("Bejegyzett hálózati elem (%2) : %1");
    }
    text += htmlWarning(sTitle.arg(node.getName(), tablename)); // bold
    QString s = node.getNote();
    if (!s.isEmpty()) text += htmlInfo(QObject::trUtf8("Megjegyzés : %1").arg(s));
    qlonglong pid = node.getId(_sPlaceId);
    if (pid <= 1) text += htmlInfo(QObject::trUtf8("Az eszköz helye ismeretlen"));
    else text += htmlInfo(QObject::trUtf8("Helye : %1").arg(cPlace().getNameById(q, pid)));
    /* -- PARAM -- */
    if (ports && node.fetchParams(q)) {
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
    if (ports && node.fetchPorts(q)) {
        text += htmlInfo(QObject::trUtf8("Portok :"));
        node.sortPortsByIndex();
        text += sHtmlTabBeg + sHtmlRowBeg;
        text += sHtmlTh.arg(QObject::trUtf8("port"));
        text += sHtmlTh.arg(QObject::trUtf8("típus"));
        text += sHtmlTh.arg(QObject::trUtf8("MAC"));
        text += sHtmlTh.arg(QObject::trUtf8("ip cím(ek)"));
        text += sHtmlTh.arg(QObject::trUtf8("DNS név"));
        text += sHtmlTh.arg(QObject::trUtf8("Fizikai link"));
        text += sHtmlTh.arg(QObject::trUtf8("Logikai link"));
        text += sHtmlTh.arg(QObject::trUtf8("LLDP link"));
        text += sHtmlRowEnd;
        QListIterator<cNPort *> li(node.ports);
        while (li.hasNext()) {
             cNPort * p = li.next();
             text += sHtmlRowBeg;
             text += sHtmlTd.arg(p->getName());
             text += sHtmlTd.arg(cIfType::ifTypeName(p->getId(_sIfTypeId)));
             if (p->descr() == cInterface::_descr_cInterface()) {  // Interface
                 QString ips, dns;
                 cInterface *i = p->reconvert<cInterface>();
                 QListIterator<cIpAddress *> ii(i->addresses);
                 while (ii.hasNext()) {
                      cIpAddress * ia = ii.next();
                      ips += sHtmlBold.arg(ia->view(q, _sAddress)) + "/" + ia->getName(_sIpAddressType) + _sCommaSp;
                      QHostInfo hi = QHostInfo::fromName(ia->address().toString());
                      dns += hi.hostName() + _sCommaSp;
                 }
                 ips.chop(_sCommaSp.size());
                 dns.chop(_sCommaSp.size());
                 text += sHtmlTd.arg(i->mac().toString());
                 text += sHtmlTd.arg(ips);
                 text += sHtmlTd.arg(dns);
            }
            else {
                 text += sHtmlTd.arg(sHtmlVoid);
                 text += sHtmlTd.arg(sHtmlVoid);
                 text += sHtmlTd.arg(sHtmlVoid);
            }
            qlonglong id;
            id = p->getId();
            if (isPatch) {
                cPhsLink pl;
                pl.setId(_sPortId1, id);
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
                text += sHtmlTd.arg(sHtmlVoid) + sHtmlTd.arg(sHtmlVoid); // Nincs, nem lehet LLDP vagy Logikai link
            }
            else {
                qlonglong lp;
                lp = LinkGetLinked<cPhsLink>(q, id);
                text += sHtmlTd.arg(lp == NULL_ID ? sHtmlVoid : cNPort::getFullNameById(q, lp));
                lp = LinkGetLinked<cLogLink>(q, id);
                text += sHtmlTd.arg(lp == NULL_ID ? sHtmlVoid : cNPort::getFullNameById(q, lp));
                lp = LinkGetLinked<cLldpLink>(q, id);
                text += sHtmlTd.arg(lp == NULL_ID ? sHtmlVoid : cNPort::getFullNameById(q, lp));
            }

            text += sHtmlRowEnd;
        }
        text += sHtmlTabEnd;
    }
    else if (ports) {
        text += sHtmlBr + sHtmlBr + QObject::trUtf8("Nincs egyetlen portja sem az adatbázisban az eszköznek.") + "</b>";
    }
    return text;
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
    if (node.fetchByMac(q, mac)) {
        text += htmlReportNode(q, node);
    }
    else {
        text += QObject::trUtf8("Nincs bejegyzett hálózati elem ezzel a MAC-kel");
    }
    text += sHtmlLine;
    /* ** ARP ** */
    QVariantList par;
    par << mac.toString();
    QString tab = query2html(q, _sArps, "hwaddress = ? ORDER BY last_time", par, true);
    if (tab.isEmpty()) {
        text += QObject::trUtf8("Nincs találat az arps táblában a megadott MAC-kel");
    }
    else {
        text += QObject::trUtf8("Találatok a MAC - IP (arps) táblában :");
        text += tab;
    }
    text += sHtmlLine;
    /* ** MAC TAB** */
    cMacTab mt;
    mt.setMac(_sHwAddress, mac);
    if (mt.completion(q)) {
        text += QObject::trUtf8("Találat a switch cím táblában : <b>%1</b> (%2 - %3; %4)").arg(
                    cNPort::getFullNameById(q, mt.getId(_sPortId)),
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
    tab = query2html(q, "arp_logs", "hwaddress_new = ? OR hwaddress_old = ? ORDER BY date_of", par, true);
    if (!tab.isEmpty()) {
        text += QObject::trUtf8("MAC - IP változások (arp_logs) :");
        text += tab + sHtmlLine;
    }
    // MACTAB LOG
    par.pop_back(); // MAC 1*
    tab = query2html(q, "mactab_logs", "hwaddress = ? ORDER BY date_of", par, true);
    if (!tab.isEmpty()) {
        text += QObject::trUtf8("A MAC mozgása a címtáblákban (mactab_logs) :");
        text += tab + sHtmlLine;
    }
    return text;
}

QString htmlReportByIp(QSqlQuery& q, const QString& addr)
{
    QString text;
    QHostAddress a(addr);
    if (a.isNull()) {
        text = QObject::trUtf8("A '%1' nem valós IP cím!").arg(addr);
        return text;
    }
    cNode node;
    if (node.fetchByIp(q, a, EX_IGNORE)) {
        text += htmlReportNode(q, node);
    }
    else {
        text += QObject::trUtf8("Nincs bejegyzett hálózati elem ezzel a IP címmel");
    }
    text += sHtmlLine;
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
    QString tab = query2html(q, "arp_logs", "ipaddress = ? ORDER BY date_of", par, true);
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
