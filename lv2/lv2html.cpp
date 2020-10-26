#include <QtCore>
#include "lv2html.h"

const QString sHtmlHead   = "<!DOCTYPE html> <html> <head> <meta charset=\"UTF-8\"> </head> <body>\n";
const QString sHtmlTail   = "</body> </html>\n";
const QString sHtmlLine   = "\n<hr>\n";
const QString sHtmlTabBeg = "<table border=\"1\" cellpadding=\"2\" cellspacing=\"2\">";
const QString sHtmlTabEnd = "</table>\n";
const QString sHtmlRowBeg = "<tr>";
const QString sHtmlRowEnd = "</tr>\n";
const QString sHtmlTh     = "<th> %1 </th>";
const QString sHtmlTd     = "<td> %1 </td>";
const QString sHtmlBold   = "<b>%1</b>";
const QString sHtmlItalic = "<i>%1</i>";
const QString sHtmlBr     = "<br>\n";
const QString sHtmlBRed   = "<b><span style=\"color:red\"> %1 </span></b>";
const QString sHtmlBGreen = "<b><span style=\"color:green\"> %1 </span></b>";
const QString sHtmlNbsp   = " &nbsp; ";
const QString sVoid   = " - ";
const QString sSpColonSp  = " : ";


QString toHtml(const QString& text, bool chgBreaks, bool esc, int indent)
{
    static const QChar   cr = QChar('\n');
    static const QString sp = "&#160;";
    QString r = text;
    if (chgBreaks)  r = r.trimmed();
    if (esc)        r = r.toHtmlEscaped();
    if (chgBreaks)  {
        if (indent) {
            QString rr;
            int i, n = r.size();
            for (i = 0; i < n; i++) {
                QChar c = r[i];
                if (c == cr) {
                    rr += sHtmlBr;
                    while (true) {
                        ++i;
                        if (i >= n) {
                            EXCEPTION(EPROGFAIL);
                        }
                        c = r[i];
                        switch (c.toLatin1()) {
                        case 32:    // Space
                            rr += sp;
                            continue;
                        case 11:    // TAB
                            for (int j = indent; j > 0; --j) rr += sp;
                            continue;
                        }
                        break;
                    }
                    rr += c;
                }
                else {
                    rr += c;
                }
            }
            r = rr;
        }
        else {
            r = r.replace(cr, sHtmlBr);
        }
    }
    return r;
}

/* ****************************************************************************************************************** */

QString htmlEnumDecoration(const QString text, const cEnumVal& eval, int m, bool chgBreaks, bool esc)
{
    QString r = toHtml(text, chgBreaks, esc);
    qlonglong fa = eval.getId(cEnumVal::ixFontAttr());
    if (m & EDM_FONT_ATTR && fa != NULL_ID) {
        if (fa & ENUM2SET(FA_BOOLD))     r = toHtmlBold(         r, false, false);
        if (fa & ENUM2SET(FA_ITALIC))    r = toHtmlItalic(       r, false, false);
        if (fa & ENUM2SET(FA_STRIKEOUT)) r = toHtmlStrikethrough(r, false, false);
        if (fa & ENUM2SET(FA_UNDERLINE)) r = toHtmlUnderline(    r, false, false);
    }
    QString s, style;
    if (m & EDM_FONT_COLOR) {
        s = eval.getName(cEnumVal::ixFgColor());
        if (!s.isEmpty()) {
            style  = QString(" \"color:%1\" ").arg(s);
        }
    }
    if (m & EDM_BACKGROUND_COLOR) {
        s = eval.getName(cEnumVal::ixBgColor());
        if (!s.isEmpty()) {
            style += QString(" \"background-color:%1\" ").arg(s);
        }
    }
    if (m & EDM_FONT_FAMILY) {
        s = eval.getName(cEnumVal::ixFontFamily());
        if (!s.isEmpty()) {
            style += QString(" \"font-family:%1\" ").arg(s);
        }
    }
    if (!style.isEmpty()) {
        r = "<span style=" + style + ">" + r + "</span>";
    }
    return r;
}

/* ****************************************************************************************************************** */

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

QString htmlTable(const QStringList head, const QList<QStringList> matrix, bool esc, int padding_pix)
{
    QString table;
    if (padding_pix == 0) table += "\n<table border=\"1\" > ";
    else table = QString("\n<table border=\"1\" cellpadding = \"%1\" > ").arg(padding_pix);
    if (!head.isEmpty()) table += htmlTableLine(head, "th", esc);
    foreach (QStringList line, matrix) {
        table += htmlTableLine(line, "td", esc);
    }
    table += "</table>\n";
    return table;
}

static bool queryTable(tRecordList<cRecordAny>& list, QSqlQuery q, cTableShape &_shape, const QString& _where, const QVariantList& _par, const QString& shrt)
{
    cRecordAny o(_shape.getName(_sTableName));
    QString ord = shrt;
    if (ord.isEmpty()) {
        foreach (cTableShapeField *pf, _shape.shapeFields.list()) {
            eOrderType ot = eOrderType(pf->getId(_sOrdInitType));
            switch (ot) {
            case OT_ASC:    ord += pf->getName(_sTableFieldName) + " ASC, ";    break;
            case OT_DESC:   ord += pf->getName(_sTableFieldName) + " DESC, ";   break;
            default:        break;
            }
        }
        if (!ord.isEmpty()) {
            ord.chop(2);    // drop last ", "
            ord.prepend(" ORDER BY ");
        }
    }
    else if (ord == "!") {      // No order
        ord.clear();
    }
    else if (ord == "@") {      // Default: order by name or id, if exixst
        int ix = NULL_IX;
        if (((ix = o.nameIndex(EX_IGNORE)) != NULL_IX) || ((ix = o.idIndex(EX_IGNORE)))) {
            ord = QString(" ORDER BY %1 ASC").arg(o.columnNameQ(ix));
        }
    }
    else {
        ord.prepend(" ");
    }
    QString sql = QString("SELECT * FROM %1 WHERE ").arg(o.tableName()) + " " + _where + " " + ord;
    if (execSql(q, sql, _par)) {
        o.set(q);
        do {
            o.set(q);
            list << o;
        } while (q.next());
    }
    return !list.isEmpty();
}

QString query2html(QSqlQuery q, cTableShape &_shape, const QString& _where, const QVariantList& _par, const QString& shrt, const QString& mergeKey)
{
    tRecordList<cRecordAny> list;
    if (queryTable(list, q, _shape, _where, _par, shrt)) return list2html(q, list, _shape, mergeKey);
    return QString();
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! const QString& childShrt, const QString& mergeKey
/*
QString query2html(QSqlQuery q, cTableShape &ownerShape, cTableShape &childShape, const QString& _where, const QVariantList& _par, const QString& ownerShrt, const QString& childShrt, const QString& mergeKey)
{
    tRecordList<cRecordAny> list;
    if (!queryTable(list, q, ownerShape, _where, _par, ownerShrt)) return QString();
    int listSize = list.size();
    if (listSize == 0) return QString();
    QStringList head;
    QList<QStringList> data;
    list2texts(head, data, q, list, ownerShape);
    QStringList orow, crow;
    int cols = head.size();
    for (int i = 0; i < cols; ++i) orow << _sNul;
    childShape.features().merge(ownerShape.features(), childShape.getName());
    list2head(head, q, childShape);
    cols = head.size() - cols;
    for (int i = 0; i < cols; ++i) crow << _sNul;
    cRecordAny o(childShape.getName(_sTableName));
    QString childWhere = list.first()->idName() + " = ? ";
    QVariantList    childPar;   childPar << QVariant();
    for (int ix = 0, row = 0; ix < listSize; ++ix, ++row) {
        tRecordList<cRecordAny> childList;
        childPar.first() = list.at(ix)->getId();
        bool child = queryTable(childList, q, childShape, childWhere, childPar, ownerShrt);
        if (child) {
            QList<QStringList> childData;
            data[row] << childData.takeFirst();
            foreach (QStringList crow, childData) {
                data.insert(row, orow + crow);
                ++row;
            }
        }
        else {
            data[row] << crow;
        }
    }
    return htmlTable(head, data);
}
*/
