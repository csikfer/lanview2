#ifndef EXPORT_H
#define EXPORT_H
#include "lv2data.h"
#include "guidata.h"

#define EXPORT_INDENT_SIZE  4

class LV2SHARED_EXPORT cExport : public QObject {
public:
    cExport(QObject *par = NULL);
protected:
    QString sNoAnyObj;
    int     actIndent;
    QString divert;
    QStringList exportedNames;

    template <class R> QString sympleExport(QSqlQuery& q, R& o, int __ord_ix, eEx __ex = EX_NOOP)
    {
        QString r;
        divert.clear();
        tIntVector ord;
        if (__ord_ix >= 0) ord << __ord_ix;
        if (o.fetch(q, false, QBitArray(1, true), ord)) {
            do {
                r += _export(q, o);
            } while (o.next(q));
        }
        else {
            if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
        }
        r += divert;
        divert.clear();
        return r;
    }

    QString line(const QString& s) { return QString(actIndent * EXPORT_INDENT_SIZE, QChar(' ')) + s + '\n'; }
    static QString str(QSqlQuery& q, const cRecordFieldRef& fr, bool sp = true);
    static QString str_z(QSqlQuery& q, const cRecordFieldRef &fr, bool sp = true);
    static QString value(QSqlQuery& q, const cRecordFieldRef &fr, bool sp = true);
    QString paramLine(QSqlQuery &q, const QString& kw, const cRecordFieldRef& fr);
    QString lineBeginBlock(const QString& s) { QString r = line(s + " {"); actIndent++; return r; }
    QString lineEndBlock() { actIndent--; return line("}"); }
public:
    QString paramType(QSqlQuery& q);
    QString _export(QSqlQuery &q, cParamType& o);
    QString sysParams(QSqlQuery& q);
    QString _export(QSqlQuery &q, cSysParam& o);
    QString tableShapes(QSqlQuery& q);
    QString _export(QSqlQuery &q, cTableShape& o);
    QString _export(QSqlQuery &q, cTableShapeField& o);
};

#endif // EXPORT_H
