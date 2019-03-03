#ifndef REPORT_H
#define REPORT_H
#include "lanview.h"
#include "lv2data.h"
#include "srvdata.h"
#include "guidata.h"
#include "lv2link.h"
#include "lv2user.h"

EXT_ const QString sHtmlLine;
EXT_ const QString sHtmlTabBeg;
EXT_ const QString sHtmlTabEnd;
EXT_ const QString sHtmlRowBeg;
EXT_ const QString sHtmlRowEnd;
EXT_ const QString sHtmlTh;
EXT_ const QString sHtmlTd;
EXT_ const QString sHtmlBold;
EXT_ const QString sHtmlItalic;
EXT_ const QString sHtmlBr;
EXT_ const QString sHtmlBRed;
EXT_ const QString sHtmlBGreen;
EXT_ const QString sHtmlNbsp;


inline QString tag(const QString& t) {
    QString r = "<" + t;
    return r +">";
}

inline QString tag(const QString& t, const QString& p) {
    QString r = "<" + t;
    r += " " + p;
    return r +">";
}

/// Feltételes szöveg HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return A konverált szöveg.
EXT_ QString toHtml(const QString& text, bool chgBreaks = false, bool esc = true);

/// HTML konverzió, indentáltparagrafus
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Széles karakterű bekezdés, a konvertált szöveggel
inline QString htmlIndent(int _pix, const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div style=\"text-indent: " + QString::number(_pix) + "px\">" + toHtml(text, chgBreaks, esc) + "</div>";
}


/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Széles karakterű bekezdés, a konvertált szöveggel
inline QString htmlWarning(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div><b>" + toHtml(text, chgBreaks, esc) + "</b></div>";
}
#define htmlBold htmlWarning
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Normál bekezdés, a konvertált szöveggel
inline QString htmlInfo(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div>" + toHtml(text, chgBreaks, esc) + "</div>";
}
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Széles karakterű, és piros betű színű bekezdés, a konvertált szöveggel
inline QString htmlError(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div style=\"color:red\"><b>" + toHtml(text, chgBreaks, esc) + "</b></div>";
}
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Széles karakterű, és zöld betü színű bekezdés, a konvertált szöveggel
inline QString htmlGrInf(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div style=\"color:green\"><b>" + toHtml(text, chgBreaks, esc) + "</b></div>";
}
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Italic (nem bekezdés!) konvertált szöveggel
inline QString toHtmlItalic(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<i>" + toHtml(text, chgBreaks, esc) + "</i>";
}
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Széles karakter (nem bekezdés!) konvertált szöveggel
inline QString toHtmlBold(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<b>" + toHtml(text, chgBreaks, esc) + "</b>";
}
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Aláhúzott karakter (nem bekezdés!) konvertált szöveggel
inline QString toHtmlUnderline(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<u>" + toHtml(text, chgBreaks, esc) + "</u>";
}
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Áthúzott karakter (nem bekezdés!) konvertált szöveggel
inline QString toHtmlStrikethrough(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<s>" + toHtml(text, chgBreaks, esc) + "</s>";
}

EXT_ QString htmlTableLine(const QStringList& fl, const QString& ft = QString(), bool esc = true);
EXT_ QString htmlTable(QStringList head, QList<QStringList> matrix, bool esc = true);

/// Egy rekord set HTML szöveggé konvertálása a tábla megjelenítés leíró alapján
/// @param q Az adatbázis műveletekhez használható query objektum.
/// @param list A rekord ill. objektum lista
/// @param shape A megjelenítést leíró objektum.
/// @param shrt Az oszlopok láthatóságának a forráas: true esetén (ez az alapértelmezés) akkor jelenik meg az oszlop,
/// ha az oszlop leíróban a FF_HTML flag igaz, false esetén a feltétel ugyan az mint a grafikus megjelenítésnél:
/// az oszlop nem jelenik meg, ha a FF_TABLE_HIDE igaz.
template <class R>
QString list2html(QSqlQuery& q, const tRecordList<R>& list, cTableShape& shape, bool shrt = true)
{
    if (shape.shapeFields.isEmpty()) shape.fetchFields(q);
    QStringList head;
    QList<QStringList> data;
    int col, row;
    int cols = shape.shapeFields.size();
    int rows = list.size();
    for (row = 0; row < rows; ++row) {
        data << QStringList();
    }
    for (col = 0; col < cols; ++col) {   // COLUMNS
        const cTableShapeField& fs = *shape.shapeFields.at(col);
        if (shrt ? !fs.getBool(_sFieldFlags, FF_HTML) : fs.getBool(_sFieldFlags, FF_TABLE_HIDE)) {
            continue;
        }
        head << fs.getText(cTableShapeField::LTX_TABLE_TITLE, fs.getName());
        QString fn = fs.getName(_sTableFieldName);
        int fix = NULL_IX;
        if (rows) {
            fix = list.first()->toIndex(fn, EX_IGNORE);
        }
        for (row = 0; row < rows; ++row) {   // ROWS
            R *p = list.at(row);
            data[row] << fs.view(q, *p, fix);
        }
    }
    return htmlTable(head, data);
}

/// Egy rekord ill. objektum HTML szöveggé konvertálása a tábla megjelenítés leíró alapján
/// @param q Az adatbázis műveletekhez használható query objektum.
/// @param o A rekord ill. objektum.
/// @param shape A megjelenítést leíró objektum.
/// @param shrt A mező láthatóságának a forráas: true esetén (ez az alapértelmezés) akkor jelenik meg a mező,
/// ha az mező (oszlop) leíróban a FF_HTML flag igaz, false esetén a feltétel ugyan az mint a grafikus megjelenítésnél:
/// az mező nem jelenik meg, ha a FF_DIALOG_HIDE igaz.
template <class R>
QString rec2html(QSqlQuery& q, const R& o, cTableShape& shape, bool shrt = true)
{
    if (shape.shapeFields.isEmpty()) shape.fetchFields(q);
    QList<QStringList> data;
    int col;
    int cols = shape.shapeFields.size();
    for (col = 0; col < cols; ++col) {
        const cTableShapeField& fs = *shape.shapeFields.at(col);
        if (shrt ? !fs.getBool(_sFieldFlags, FF_HTML) : fs.getBool(_sFieldFlags, FF_TABLE_HIDE)) {
            continue;
        }
        QString fn = fs.getName(_sTableShapeFieldName);
        QStringList row;
        row << fs.getText(cTableShapeField::LTX_DIALOG_TITLE, fs.getName());
        row << fs.view(q, o);
        data << row;
    }
    return htmlTable(QStringList(), data);
}

/// Rekordok beolvasása és HTML formátumú táblázat késítése.
/// @param q Query objektum az adatbázis eléréshez
/// @param _shape A megjelenítést leíró objektum
/// @param _where A szűrési feltétel
/// @param _par A szűrési feltétel paraméterei
/// @param shrt A rendezés SQL string, ha üres, akkor a név mezőre van rendezés, ha értéke '!', akkor nincs rendezés.
EXT_ QString query2html(QSqlQuery q, cTableShape& _shape, const QString& _where, const QVariantList &_par = QVariantList(), const QString& shrt = QString());
/// Rekordok beolvasása és HTML formátumú táblázat késítése.
/// @param q Query objektum az adatbázis eléréshez
/// @param _shapeName A megjelenítést leíró objektum (cTableShape / table_shapes) neve
/// @param _where A szűrési feltétel
/// @param _par A szűrési feltétel paraméterei
/// @param shrt A rendezés SQL string, ha üres, akkor a név mezőre van rendezés, ha értéke '!', akkor nincs rendezés.
inline QString query2html(QSqlQuery q, const QString& _shapeName, const QString& _where, const QVariantList &_par = QVariantList(), const QString& shrt = QString())
{
    cTableShape shape;
    shape.setByName(q, _shapeName);
    return query2html(q, shape, _where, _par, shrt);
}

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

static inline QString logLink2str(QSqlQuery& q, cLogLink& lnk) {
    return QObject::trUtf8("#%1 : %2  <==> %3\n")
            .arg(lnk.getId())
            .arg(cNPort::getFullNameById(q, lnk.getId(_sPortId1)), cNPort::getFullNameById(q, lnk.getId(_sPortId2)));
}

static inline QString lldpLink2str(QSqlQuery& q, cLldpLink& lnk) {
    return QObject::trUtf8("#%1 : %2  <==> %3  (%4 > %5\n")
            .arg(lnk.getId())
            .arg(cNPort::getFullNameById(q, lnk.getId(_sPortId1)), cNPort::getFullNameById(q, lnk.getId(_sPortId2)))
            .arg(lnk.getName(_sFirstTime), lnk.getName(_sLastTime));
}

static inline QString mactab2str(QSqlQuery& q, cMacTab& mt) {
    return QObject::trUtf8("%1  ==> %2  (%3 > %4\n")
            .arg(cNPort::getFullNameById(q, mt.getId(_sPortId)), mt.getName(_sHwAddress))
            .arg(mt.getName(_sFirstTime), mt.getName(_sLastTime));
}

EXT_ QString linksHtmlTable(QSqlQuery& q, tRecordList<cPhsLink>& list, bool _swap = false, const QStringList _verticalHeader = QStringList());

EXT_ bool linkColisionTest(QSqlQuery& q, bool& exists, const cPhsLink& _pl, QString& msg);

EXT_ QString linkChainReport(QSqlQuery& q, qlonglong _pid, ePhsLinkType _type, ePortShare _sh, QMap<ePortShare, qlonglong>& endMap);
EXT_ QString linkEndEndLogReport(QSqlQuery& q, qlonglong _pid1, qlonglong _pid2, bool saved = false, const QString& msgPref = QString());
EXT_ QString linkEndEndMACReport(QSqlQuery& q, qlonglong _pid1, qlonglong _pid2, const QString& msgPref = QString());

static inline void expWarning(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(htmlWarning(text, chgBreaks, esc));
}
static inline void expInfo(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(htmlInfo(text, chgBreaks, esc));
}
static inline void expError(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(htmlError(text, chgBreaks, esc));
}
static inline void expItalic(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(toHtmlItalic(text, chgBreaks, esc));
}
static inline void expBold(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(toHtmlBold(text, chgBreaks, esc));
}
static inline void expGreen(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push("<div><b><span style=\"color:green\">" + toHtml(text, chgBreaks, esc) + "</span></b></div>");
}

static inline void expText(int _r, const QString& text, bool chgBreaks = false, bool esc = true) {
    int r = _r & RS_STAT_MASK;
    if      (r  < RS_WARNING) expInfo(text, chgBreaks, esc);
    else if (r == RS_WARNING) expWarning(text, chgBreaks, esc);
    else                      expError(text, chgBreaks, esc);
}

static inline void expHtmlLine() {
    cExportQueue::push(sHtmlLine);
}

EXT_ tStringPair htmlReport(QSqlQuery& q, cRecord& o, const cTableShape& shape);
EXT_ tStringPair htmlReport(QSqlQuery& q, cRecord& o, const QString& _name = _sNul, const cTableShape *pShape = nullptr);

#endif // REPORT_H
