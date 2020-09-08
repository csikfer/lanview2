#ifndef REPORT_H
#define REPORT_H
#include "lanview.h"
#include "lv2data.h"
#include "srvdata.h"
#include "lv2link.h"
#include "lv2user.h"
#include "lv2html.h"

EXT_ QString sReportPlace(QSqlQuery& q, qlonglong _pid, bool parents = true, bool zones = true, bool cat = true);
EXT_ tStringPair htmlReportPlace(QSqlQuery& q, cRecord& o);
inline tStringPair htmlReportPlace(QSqlQuery& q, qlonglong pid)
{
    cPlace o;
    o.setById(q, pid);
    return htmlReportPlace(q, o);
}

// Node report flags
#define NODE_REPORT_SERVICES    0x00010000;

/// Node report title
/// @param t Type: NOT_PATCH, NOT_NODE, or NOT_SNMPDEVICE.
/// @param n node object
EXT_ QString titleNode(QSqlQuery& q, const cRecord& n);
/// Node report title
/// @param n node object, original type!
EXT_ QString titleNode(const cRecord& n);
/// Node report title by real type
/// @param n node object
EXT_ QString titleNode(int t, const cRecord& n);
/// HTML riport egy node-ról
/// @param q Query objektum az adatbázis eléréshez
/// @param node Az objektum amiről a riportot kell készíteni
/// @param _sTitle Egy opcionális címsor.
/// @param flags
EXT_ tStringPair htmlReportNode(QSqlQuery& q, cRecord& node, const QString& _sTitle = QString(), qlonglong flags = -1);
/// Egy címsort és egy szöveg törzset tartalmazó string párból fűz össze egy stringet.
/// A címsor bold lesz.
inline QString htmlPair2Text(const tStringPair& sp) {
    return htmlWarning(sp.first) + sp.second;
}

/// HTML riport egy MAC alapján
/// @param q Query objektum az adatbázis eléréshez
/// @param sMac A keresett MAC
EXT_ QString htmlReportByMac(QSqlQuery& q, const QString& sMac);
/// HTML riport egy IP alapján
/// @param q Query objektum az adatbázis eléréshez
/// @param aAddr A keresett IP
EXT_ QString htmlReportByIp(QSqlQuery& q, const QString& sAddr);

inline QString logLink2str(QSqlQuery& q, cLogLink& lnk) {
    return QObject::tr("#%1 : %2  <==> %3\n")
            .arg(lnk.getId())
            .arg(cNPort::getFullNameById(q, lnk.getId(_sPortId1)), cNPort::getFullNameById(q, lnk.getId(_sPortId2)));
}

inline QString lldpLink2str(QSqlQuery& q, cLldpLink& lnk) {
    return QObject::tr("#%1 : %2  <==> %3  (%4 > %5\n")
            .arg(lnk.getId())
            .arg(cNPort::getFullNameById(q, lnk.getId(_sPortId1)), cNPort::getFullNameById(q, lnk.getId(_sPortId2)))
            .arg(lnk.getName(_sFirstTime), lnk.getName(_sLastTime));
}

inline QString mactab2str(QSqlQuery& q, cMacTab& mt) {
    return QObject::tr("%1  ==> %2  (%3 > %4\n")
            .arg(cNPort::getFullNameById(q, mt.getId(_sPortId)), mt.getName(_sHwAddress))
            .arg(mt.getName(_sFirstTime), mt.getName(_sLastTime));
}

EXT_ QString linksHtmlTable(QSqlQuery& q, tRecordList<cPhsLink>& list, bool _swap = false, const QStringList _verticalHeader = QStringList());

EXT_ bool linkColisionTest(QSqlQuery& q, bool& exists, const cPhsLink& _pl, QString& msg);

/// Végigmegy a linkek láncán, és készít egy táblázatot a talált rekordokból.
/// Ha talál végpontokat, akkor azok ID-jét az endMap-ba helyezi, ahol a kulcs az eredő megosztás.
/// @param q Az adatbázis műveletek query pbjektuma.
/// @param _pid A készülő, vagy létező link rekord egyik csomóponti elemének ID-je
/// @param _type A _pid ID-jű elem típusa.
/// @param _sh A készülő, vagy létező link megosztás típusa
/// @param endMap talált végpontok és az eredő megosztások
/// @param _exid Ha az ellenörzendő link létezik, akkor annak az ID-je, vagy NULL_ID ha még nem létezik
EXT_ QString linkChainReport(QSqlQuery& q, qlonglong _pid, ePhsLinkType _type, ePortShare _sh, QMap<ePortShare, qlonglong>& endMap);
EXT_ QString linkEndEndLogReport(QSqlQuery& q, qlonglong _pid1, qlonglong _pid2, bool saved = false, const QString& msgPref = QString());
EXT_ QString linkEndEndMACReport(QSqlQuery& q, qlonglong _pid1, qlonglong _pid2, const QString& msgPref = QString());

inline void expWarning(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(htmlWarning(text, chgBreaks, esc));
}
inline void expInfo(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(htmlInfo(text, chgBreaks, esc));
}
inline void expError(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(htmlError(text, chgBreaks, esc));
}
inline void expItalic(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(toHtmlItalic(text, chgBreaks, esc));
}
inline void expBold(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(toHtmlBold(text, chgBreaks, esc));
}
inline void expGreen(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push("<div><b><span style=\"color:green\">" + toHtml(text, chgBreaks, esc) + "</span></b></div>");
}

inline void expText(int _r, const QString& text, bool chgBreaks = false, bool esc = true) {
    int r = _r & RS_STAT_MASK;
    if      (r  < RS_WARNING) expInfo(text, chgBreaks, esc);
    else if (r == RS_WARNING) expWarning(text, chgBreaks, esc);
    else                      expError(text, chgBreaks, esc);
}

inline void expHtmlLine() {
    cExportQueue::push(sHtmlLine);
}

/// HTML formátumú riport egy beolvasott adat rekordról.
/// @param q Az esetleges további adatbázis műveletekhez használható query objektum.
/// @param o Az adat objektum, amiről a riport készül.
/// @param shape Az adatmezők kiírását leíró objeltum referenciája.
/// @return Egy tStringPair objektum, melynek first eleme a címet, a second eleme pedig a HTML formátumú szöveget tartalmazza.
EXT_ tStringPair htmlReport(QSqlQuery& q, cRecord& o, const cTableShape& shape);
/// HTML formátumú riport egy beolvasott adat rekordról. Egyedi riport formátumok kezelésével.
/// @param q Az esetleges további adatbázis műveletekhez használható query objektum.
/// @param o Az adat objektum, amiről a riport készül.
/// @param _name Opcionális, az adat típus neve, vagy a leíró (cTableShape) objektum neve.
/// A következő nevek megadása esetén egy egyedi riprt készül:
/// - "patchs", "nodes", snmp_devices"  esetén a riportot a htmlReportNode(q, o) függvény fogja elkészíteni
/// - "places"  esetén a riportot a htmlReportPlace(q, o) függvény fogja elkészíteni
/// - minden más esetben, ha pShape nem null, akkor a report a megadott nevű leíró alapján készül el,
/// - ha nincs megadva leíró név, akkor a leíró neve azonos az adat tábla nevével.
/// @param pShape Opcionális adatmezők kiírását leíró objeltum referenciája.
/// @param pShape Opcionális, az adatmezők kiírását leíró obkeltum referenciája pointere, vagy nullptr.
/// @return Egy tStringPair objektum, melynek first eleme a címet, a second eleme pedig a HTML formátumú szöveget tartalmazza.
EXT_ tStringPair htmlReport(QSqlQuery& q, cRecord& o, const QString& _name = _sNul, const cTableShape *pShape = nullptr);

#endif // REPORT_H
