#include "report.h"
#include "lv2link.h"
#include "srvdata.h"

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
    QList<QHostAddress> al = cArp::mac2ips(q, mac);
    if (al.isEmpty()) {
        text += QObject::trUtf8("Nincs találat az arps táblában a megadott MAC-el");
    }
    else {
        QString ips;
        foreach (QHostAddress a, al) {
            ips += a.toString() + ", ";
        }
        ips.chop(2);
        text += QObject::trUtf8("Az arps táblában a megadott MAC-hez talált IP címek : <b>%1</b>").arg(ips);
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
