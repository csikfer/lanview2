#include "report.h"
#include "srvdata.h"
#include "guidata.h"

QString toHtml(const QString& text, bool chgBreaks, bool esc)
{
    static const QString br = " <br> ";
    static const QChar   cr = QChar('\n');
    QString r = text;
    if (chgBreaks)  r = r.trimmed();
    if (esc)        r = r.toHtmlEscaped();
    if (chgBreaks)  r = r.replace(cr, br);
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
    if (list.size() == NULL) return QString();
    return list2html(q, list, _shape, shrt);
}


QString reportByMac(QSqlQuery& q, const QString& sMac)
{
    static const QString tbeg = "<table border=\"1\" cellpadding=\"2\" cellspacing=\"2\">";
    static const QString tend = "</table>\n";
    static const QString rbeg = "<tr>";
    static const QString rend = "</tr>\n";
    static const QString th   = "<th> %1 </th>";
    static const QString td   = "<td> %1 </td>";
    static const QString bold = "<b>%1</b>";
    static const QString nil  = " - ";
    static const QString sep  = ", ";
    static const QString line = "\n<hr>\n";
#define SEP_SIZE 2
    QString text;
    cMac mac(sMac);
    if (!mac) {
        text = QObject::trUtf8("A '%1' nem valós MAC!").arg(sMac);
        return text;
    }
    /* ** OUI ** */
    cOui oui;
    if (oui.fetchByMac(q, mac)) {
        text += QObject::trUtf8("OUI rekord : ") + oui.getName() + "<br>";
        text += oui.getNote();
    }
    else {
        text += QObject::trUtf8("Nincs OUI rekord.");
    }
    text += line;
    /* ** NODE ** */
    cNode node;
    if (node.fetchByMac(q, mac)) {
        QString tablename = node.getOriginalDescr(q)->tableName();
        text += QObject::trUtf8("Bejegyzett hálózati elem (%1) : ").arg(tablename);
        text += bold.arg(node.getName());
        /* -- PARAM -- */
        if (node.fetchParams(q)) {
            text += tbeg + rbeg;
            text += th.arg(QObject::trUtf8("Paraméter típus"));
            text += th.arg(QObject::trUtf8("Érték"));
            text += th.arg(QObject::trUtf8("Dim."));
            text += rend;
            QListIterator<cNodeParam *> li(node.params);
            while (li.hasNext()) {
                 cNodeParam * p = li.next();
                 text += rbeg;
                 text += td.arg(p->name());
                 text += td.arg(p->value().toString());
                 text += td.arg(p->dim());
                 text += rend;
            }
            text += tend;
        }
        /* -- PORTS -- */
        if (node.fetchPorts(q)) {
            node.sortPortsByIndex();
            text += tbeg + rbeg;
            text += th.arg(QObject::trUtf8("port"));
            text += th.arg(QObject::trUtf8("típus"));
            text += th.arg(QObject::trUtf8("MAC"));
            text += th.arg(QObject::trUtf8("ip cím(ek)"));
            text += th.arg(QObject::trUtf8("DNS név"));
            text += th.arg(QObject::trUtf8("Fizikai link"));
            text += th.arg(QObject::trUtf8("Logikai link"));
            text += th.arg(QObject::trUtf8("LLDP link"));
            text += rend;
            QListIterator<cNPort *> li(node.ports);
            while (li.hasNext()) {
                 cNPort * p = li.next();
                 text += rbeg;
                 text += td.arg(p->getName());
                 text += td.arg(cIfType::ifTypeName(p->getId(_sIfTypeId)));
                 if (p->descr() == cInterface::_descr_cInterface()) {  // Interface
                     QString ips, dns;
                     cInterface *i = p->reconvert<cInterface>();
                     QListIterator<cIpAddress *> ii(i->addresses);
                     while (ii.hasNext()) {
                          cIpAddress * ia = ii.next();
                          ips += bold.arg(ia->view(q, _sAddress)) + "/" + ia->getName(_sIpAddressType) + sep;
                          QHostInfo hi = QHostInfo::fromName(ia->address().toString());
                          dns += hi.hostName() + sep;
                     }
                     ips.chop(SEP_SIZE);
                     dns.chop(SEP_SIZE);
                     text += td.arg(i->mac().toString());
                     text += td.arg(ips);
                     text += td.arg(dns);
                 }
                 else {
                     text += td.arg(nil);
                     text += td.arg(nil);
                     text += td.arg(nil);
                 }
                 qlonglong id, lp;
                 id = p->getId();
                 lp = LinkGetLinked<cPhsLink>(q, id);
                 text += td.arg(lp == NULL_ID ? nil : cNPort::getFullNameById(q, lp));
                 lp = LinkGetLinked<cLogLink>(q, id);
                 text += td.arg(lp == NULL_ID ? nil : cNPort::getFullNameById(q, lp));
                 lp = LinkGetLinked<cLldpLink>(q, id);
                 text += td.arg(lp == NULL_ID ? nil : cNPort::getFullNameById(q, lp));


                 text += rend;
            }
            text += tend;
        }
        else {
            text += "<br><b>" + QObject::trUtf8("Nincs egyetlen portja sem az adatbázisban az eszköznek.") + "</b>";
        }
    }
    else {
        text += QObject::trUtf8("Nincs bejegyzett hálózati elem ezzel a MAC-el");
    }
    text += line;
    /* ** ARP ** */
    text += QObject::trUtf8("Találatok at arps táblában :");
    QVariantList par;
    par << mac.toString();
    QString tab = query2html(q, "arps", "hwaddress = ? ORDER BY last_time", par, true);
    if (tab.isEmpty()) {
        text += QObject::trUtf8("Nincs találat az arps táblában a megadott MAC-el");
    }
    else {
        text += tab;
    }
    text += line;
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
    text += line;
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
