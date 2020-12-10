#ifndef LV2HTML_H
#define LV2HTML_H

#include "lanview.h"
#include "guidata.h"

EXT_ const QString sHtmlHead;
EXT_ const QString sHtmlTail;
EXT_ const QString sHtmlLine;
EXT_ const QString sHtmlTabBeg;
EXT_ const QString sHtmlTabEnd;
EXT_ const QString sHtmlRowBeg;
EXT_ const QString sHtmlRowEnd;
EXT_ const QString sHtmlTh;
EXT_ const QString sHtmlTd;
EXT_ const QString sHtmlBold;
EXT_ const QString sHtmlItalic;
EXT_ const QString sVoid;
EXT_ const QString sHtmlBr;
EXT_ const QString sHtmlBRed;
EXT_ const QString sHtmlBGreen;
EXT_ const QString sHtmlNbsp;
EXT_ const QString sVoid;
EXT_ const QString sSpColonSp;

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
/// @param indent Csak akkor érvényes, ha chgBreak értéke true, ebben az esetben a sor eleji space-ket kikényszeríti.
/// @return A konverált szöveg.
EXT_ QString toHtml(const QString& text, bool chgBreaks = false, bool esc = true, int indent = 0);

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
    return "<div style=\"color:red\"> <b> " + toHtml(text, chgBreaks, esc) + " </b> </div>";
}
/// HTML konverzió
/// @param text A konvertálandó szöveg
/// @param chgBreaks Ha igaz, akkor a sor töréseket kicseréli a "<br>" stringre,
///         elötte törli a többszörös soremeléseket, vagy szóközöket, tabulátorokat.
/// @param esc Ha igaz, akkor a szöveget konvertálja a QString::toHtmlEscaped() metódussal.
/// @return Széles karakterű, és zöld betü színű bekezdés, a konvertált szöveggel
inline QString htmlGrInf(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div style=\"color:green\"> <b> " + toHtml(text, chgBreaks, esc) + " </b> </div>";
}

inline QString htmlItalicInf(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div> <i> " + toHtml(text, chgBreaks, esc) + " </i> </div>";
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

inline QString toGreen(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<span style=\"color:green\">" + toHtml(text, chgBreaks, esc) + "</span>";
}

inline QString toRed(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<span style=\"color:red\">" + toHtml(text, chgBreaks, esc) + "</span>";
}

// HTML dekoráció


// dekoráció enumerációk alapján

enum eEnumDecorationMask {
    EDM_FONT_ATTR           =  1,
    EDM_FONT_FAMILY         =  2,
    EDM_FONT_COLOR          =  4,
    EDM_BACKGROUND_COLOR    =  8,
    EDM_ALL                 = 15
};

EXT_ QString htmlEnumDecoration(const QString text, const cEnumVal& eval, int m = EDM_ALL, bool chgBreaks = false, bool esc = true);

// HTML táblázat

EXT_ QString htmlTableLine(const QStringList& fl, const QString& ft = QString(), bool esc = true);
EXT_ QString htmlTable(const QStringList& head, const QList<QStringList>& matrix, bool esc = true, int padding_pix = 0);

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

/// Egy rekord sethez csak a hfejléc konvertálása a tábla megjelenítés leíró alapján
/// @param head A táblázat fejléce.
/// @param q Az adatbázis műveletekhez használható query objektum.
/// @param shape A megjelenítést leíró objektum. Ha nincsenek beolvasva a mező leírók, akor azokat beolvassa.
/// @param mergeKey Egy másodlagos kulcs, ami alapján modosítható a leíró feature értéke, lásd: cFeatures::selfMerge(const QString& key) metódust.
///  Ha mergeKey nem üres string, akkor mindenképpen újraolvassa a metódus, a leíróban a mező leírókat.
inline void list2head(QStringList& head, QSqlQuery& q, cTableShape& shape, const QString& mergeKey = QString())
{
    bool merge = !mergeKey.isEmpty();
    if (merge) shape.features().selfMerge(mergeKey);
    if (merge || shape.shapeFields.isEmpty()) shape.fetchFields(q);
    int col;
    int cols = shape.shapeFields.size();
    bool reportView = shape.feature(_sReport).compare(_sTable);
    for (col = 0; col < cols; ++col) {   // COLUMNS
        const cTableShapeField& fs = *shape.shapeFields.at(col);
        if (reportView ? !fs.getBool(_sFieldFlags, FF_HTML) : fs.getBool(_sFieldFlags, FF_TABLE_HIDE)) {
            continue;
        }
        head << fs.getText(cTableShapeField::LTX_TABLE_TITLE, fs.getName());
    }
}

/// Egy rekord set szövegekké konvertálása a tábla megjelenítés leíró alapján, a fejléc nélkül
/// @param data A listából konvertált táblázat. A QList típusú konténer a tábla sorait tartalmazza.
/// @param q Az adatbázis műveletekhez használható query objektum.
/// @param list A rekord ill. objektum lista
/// @param shape A megjelenítést leíró objektum. Feltételezi, hogy a mezőleírók már be vannak olvasva
template <class R>
void list2texts(QList<QStringList>& data, QSqlQuery& q, const tRecordList<R>& list, cTableShape& shape)
{
    int col, row;
    int cols = shape.shapeFields.size();
    int rows = list.size();
    for (row = 0; row < rows; ++row) {
        data << QStringList();
    }
    bool reportView = shape.feature(_sReport).compare(_sTable);
    for (col = 0; col < cols; ++col) {   // COLUMNS
        const cTableShapeField& fs = *shape.shapeFields.at(col);
        if (reportView ? !fs.getBool(_sFieldFlags, FF_HTML) : fs.getBool(_sFieldFlags, FF_TABLE_HIDE)) {
            continue;
        }
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
}

/// Egy rekord set szövegekké konvertálása a tábla megjelenítés leíró alapján
/// @param head A táblázat fejléce.
/// @param data A listából konvertált táblázat. A QList típusú konténer a tábla sorait tartalmazza.
/// @param q Az adatbázis műveletekhez használható query objektum.
/// @param list A rekord ill. objektum lista
/// @param shape A megjelenítést leíró objektum. Ha nincsenek beolvasva a mező leírók, akor azokat beolvassa.
/// @param mergeKey Egy másodlagos kulcs, ami alapján modosítható a leíró feature értéke, lásd: cFeatures::selfMerge(const QString& key) metódust.
///  Ha mergeKey nem üres string, akkor mindenképpen újraolvassa a metódus, a leíróban a mező leírókat.
template <class R>
void list2texts(QStringList& head, QList<QStringList>& data, QSqlQuery& q, const tRecordList<R>& list, cTableShape& shape, const QString& mergeKey = QString())
{
    list2head(head, q, shape, mergeKey);
    list2texts(data, q, list, shape);
}

/// Egy rekord set HTML szöveggé konvertálása a tábla megjelenítés leíró alapján
/// @param q Az adatbázis műveletekhez használható query objektum.
/// @param list A rekord ill. objektum lista
/// @param shape A megjelenítést leíró objektum. Ha nincsenek beolvasva a mező leírók, akor azokat beolvassa.
/// @param mergeKey Egy másodlagos kulcs, ami alapján modosítható a leíró feature értéke, lásd: cFeatures::selfMerge(const QString& key) metódust.
///  Ha mergeKey nem üres string, akkor mindenképpen újraolvassa a metódus, a leíróban a mező leírókat.
template <class R>
QString list2html(QSqlQuery& q, const tRecordList<R>& list, cTableShape& shape, const QString& mergeKey = QString())
{
    QStringList head;
    QList<QStringList> data;
    list2texts<R>(head, data, q, list, shape, mergeKey);
    return htmlTable(head, data);
}

/// Rekordok beolvasása és HTML formátumú táblázat készítése.
/// @param q Query objektum az adatbázis eléréshez
/// @param _shape A megjelenítést leíró objektum
/// @param _where A szűrési feltétel a WHRE striggel együtt, vagy üres string. Opcionális, alapértelmezetten nincs szűrési feltétel.
/// @param _par A szűrési feltétel paraméterei. Opcionális, alapértelmezetten nincs paraméter.
/// @param shrt A rendezés SQL string, ha üres, akkor a _shape leíró szerinti a endezés, ha '@' akkor a név mezőre vagy ha az nincs az id-re van rendezés, ha értéke '!', akkor nincs rendezés,
///     illetve, ha az előzőek közül egyiksem, akkor a rendezés módját tartalmazza az "ORDER BY" szöveg együtt. Opcionális, alapértelmezetten a shape leíró szerinti rendezés.
/// @param mergeKey Opcionális kulcs, ami alapján kiegészíti a feature változókat a shape laíróban (lásd a cFeatures osztály merge() metódustát). Opcionális, alapértelmezett érték a "report" string.
EXT_ QString query2html(QSqlQuery q, cTableShape& _shape, const QString& _where = QString(), const QVariantList &_par = QVariantList(), const QString& shrt = QString(), const QString& mergeKey = _sReport);
/// Rekordok beolvasása és HTML formátumú táblázat késítése.
/// @param q Query objektum az adatbázis eléréshez
/// @param _shapeName A megjelenítést leíró objektum (cTableShape / table_shapes) neve
/// @param _where A szűrési feltétel a WHRE striggel együtt
/// @param _par A szűrési feltétel paraméterei
/// @param shrt A rendezés SQL string, ha üres, akkor a _shape leíró szerinti a endezés, ha '@' akkor a név mezőre vagy ha az nincs az id-re van rendezés, ha értéke '!', akkor nincs rendezés,
/// illetve, ha az előzőek közül egyiksem, akkor a rendezés módját tartalmazza az "ORDER BY" szöveg együtt.
inline QString query2html(QSqlQuery q, const QString& _shapeName, const QString& _where = QString(), const QVariantList &_par = QVariantList(), const QString& shrt = QString())
{
    cTableShape shape;
    shape.setByName(q, _shapeName);
    return query2html(q, shape, _where, _par, shrt);
}
/*
EXT_ QString query2html(QSqlQuery q, cTableShape &ownerShape, cTableShape &childShape, const QString& _where = QString(), const QVariantList& _par = QVariantList(), const QString& ownerShrt = QString(),
                        const QString& childShrt = QString(), const QString &mergeKey = _sReport);
*/

#endif // LV2HTML_H
