#ifndef REPORT_H
#define REPORT_H
#include "lanview.h"
#include "lv2data.h"
#include "guidata.h"
#include "lv2link.h"

static inline QString tag(const QString& t) {
    QString r = "<" + t;
    return r +">";
}

static inline QString tag(const QString& t, const QString& p) {
    QString r = "<" + t;
    r += " " + p;
    return r +">";
}

EXT_ QString toHtml(const QString& text, bool chgBreaks = false, bool esc = true);

static inline QString htmlWarning(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div><b>" + toHtml(text, chgBreaks, esc) + "</b></div>";
}
static inline QString htmlInfo(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div>" + toHtml(text, chgBreaks, esc) + "</div>";
}
static inline QString htmlError(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<div style=\"color:red\"><b>" + toHtml(text, chgBreaks, esc) + "</b></div>";
}
static inline QString htmlItalic(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<i>" + toHtml(text, chgBreaks, esc) + "</i>";
}
static inline QString htmlBold(const QString& text, bool chgBreaks = false, bool esc = true) {
    return "<b>" + toHtml(text, chgBreaks, esc) + "</b>";
}

EXT_ QString htmlTableLine(const QStringList& fl, const QString& ft = "td");
EXT_ QString htmlTable(QStringList head, QList<QStringList> matrix);

template <class R>
QString list2html(QSqlQuery& q, const tRecordList<R>& list, cTableShape& shape, bool shrt = true)
{
    if (shape.shapeFields.isEmpty()) shape.fetchFields(q);
    QStringList head;
    QList<QStringList> data;
    int i, j;
    int n = shape.shapeFields.size();
    int m = list.size();
    for (i = 0; i < n; ++i) {   // COLUMNS
        const cTableShapeField& fs = *shape.shapeFields.at(i);
        if (shrt ? !fs.getBool(_sFieldFlags, FF_HTML) : fs.getBool(_sFieldFlags, FF_TABLE_HIDE)) continue;
        head << fs.getText(cTableShapeField::LTX_TABLE_TITLE, fs.getName());
        QString fn = fs.getName(_sTableShapeFieldName);
        for (j = 0; j < m; ++j) {   // ROWS
            if (i == 0) data << QStringList();
            data[j] << list.at(j)->view(q, fn);
        }
    }
    return htmlTable(head, data);
}

EXT_ QString reportByMac(QSqlQuery& q, const QString& aMac);

EXT_ QString linksHtmlTable(QSqlQuery& q, tRecordList<cPhsLink>& list, bool _swap = false);

EXT_ bool linkColisionTest(QSqlQuery& q, bool& exists, const cPhsLink& _pl, QString& msg);

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
    cExportQueue::push(htmlItalic(text, chgBreaks, esc));
}
static inline void expBold(const QString& text, bool chgBreaks = false, bool esc = true) {
    cExportQueue::push(htmlBold(text, chgBreaks, esc));
}

#endif // REPORT_H
