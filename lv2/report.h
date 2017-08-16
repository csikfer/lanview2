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

static inline QString htmlWarning(const QString& text) {
    return "<div><b>" + text + "</b></div>";
}
static inline QString htmlInfo(const QString& text) {
    return "<div>" + text + "</div>";
}
static inline QString htmlError(const QString& text) {
    return "<div style=\"color:red\"><b>" + text + "</b></div>";
}
static inline QString htmlItalic(const QString& text) {
    return "<i>" + text + "</i>";
}
static inline QString htmlBold(const QString& text) {
    return "<b>" + text + "</b>";
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
    for (i = 0; i < n; ++i) {
        const cTableShapeField& fs = *shape.shapeFields.at(i);
        if (shrt ? !fs.getBool(_sFieldFlags, FF_HTML) : fs.getBool(_sFieldFlags, FF_TABLE_HIDE)) continue;
        head << fs.getName(_sTableTitle);
        QString fn = fs.getName(_sTableShapeFieldName);
        for (j = 0; j < m; ++j) {
            data[j] << list.at(j)->view(q, fn);
        }
    }
    return htmlTable(head, data);
}

EXT_ QString reportByMac(QSqlQuery& q, const QString& aMac);

EXT_ QString linksHtmlTable(QSqlQuery& q, tRecordList<cPhsLink>& list, bool _swap = false);

EXT_ bool linkColisionTest(QSqlQuery& q, bool& exists, const cPhsLink& _pl, QString& msg);

#endif // REPORT_H
