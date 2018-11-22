#ifndef EXPORT_H
#define EXPORT_H
#include "lv2data.h"
#include "srvdata.h"
#include "guidata.h"

#define EXPORT_INDENT_SIZE  4

class LV2SHARED_EXPORT cExport : public QObject {
public:
    cExport(QObject *par = nullptr);
protected:
    QString sNoAnyObj;
    int     actIndent;
    QString divert;
    QStringList exportedNames;

    /// Egy tábla kiexportálása
    /// @param q
    /// @param Egy objektum példány, amelyik típusról kell egy teljes export, ha az objektum üres. Nem üres objekzum esetén az
    ///        objektum tartalma a szűrési feltétel.
    /// @param _ord_ix A mező indexek, amire rendezni kell az objetum példányokat
    /// @param __ex Ha értéke EX_NOOP (ez az alapértelmezés), akkor kizárást dob, ha egy objektum/rekord sincs az adatbázisban.
    /// @return A kiexportált szöveg.
    template <class R> QString sympleExport(R& o, const tIntVector& __ord_ixs, eEx __ex = EX_NOOP, const QBitArray& where = QBitArray(1, false))
    {
        QSqlQuery q = getQuery();
        QSqlQuery q2 = getQuery();
        QString r;
        if (o.fetch(q, false, where, __ord_ixs)) {
            do {
                r += _export(q2, o);
            } while (o.next(q));
        }
        else {
            if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
        }
        return r;
    }
    /// Egy tábla kiexportálása
    /// @param q
    /// @param Egy objektum példány, amelyik típusról kell egy teljes export. (tartalma érdektelen).
    /// @param _ord_ix A mező index, amire rendezni kell az objetum példányokat, ha negatív, akkor nincs rendezés
    /// @param __ex Ha értéke EX_NOOP (ez az alapértelmezés), akkor kizárást dob, ha egy objektum/rekord sincs az adatbázisban.
    /// @return A kiexportált szöveg.
    template <class R> QString sympleExport(R& o, int __ord_ix, eEx __ex = EX_NOOP, const QBitArray& where = QBitArray(1, false))
    {
        tIntVector ord;
        if (__ord_ix >= 0) ord << __ord_ix;
        return sympleExport(o, ord, __ex, where);
    }

    template <class R> QString exportWhere(R& o, const tIntVector& __ord_ixs, const QString& where, eEx __ex = EX_NOOP, const QBitArray& fm = QBitArray())
    {
        QSqlQuery q = getQuery();
        QSqlQuery q2 = getQuery();
        QString r;
        if (o.fetchQuery(q, false, fm, __ord_ixs, 0, 0, _sNul, where)) {
            o.set(q);
            do {
                r += _export(q2, o);
            } while (o.next(q));
        }
        else {
            if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
        }
        return r;
    }
    QString line(const QString& s) { return QString(actIndent * EXPORT_INDENT_SIZE, QChar(' ')) + s + '\n'; }
    static QString escaped(const QString& s);
    static QString head(const QString& kw, const QString& s, const QString &n);
    static QString head(const QString& kw, const cRecord &o) { return head(kw, o.getName(), o.getNote()); }
    static QString str(const cRecordFieldRef& fr, bool sp = true);
    static QString str_z(const cRecordFieldRef &fr, bool sp = true);
    static QString value(QSqlQuery& q, const cRecordFieldRef &fr, bool sp = true);
    static QString langComment();
    QString lineText(const QString &kw, const cRecord& o, int _tix);
    /// Title texts
    /// @param kw Keyword(s)
    /// @param o Bbject
    /// @param _fr First text index
    /// @param _to Last text index
    QString lineTitles(const QString &kw, const cRecord& o, int _fr, int _to);
    QString features(cRecord& o);
    QString paramLine(QSqlQuery &q, const QString& kw, const cRecordFieldRef& fr, const QVariant &_def = QVariant());
    QString flag(const QString& kw, const cRecordFieldRef& fr, bool inverse = false);
    QString lineBeginBlock(const QString& s) { QString r = line(s + " {"); actIndent++; return r; }
    QString lineEndBlock();
    QString lineEndBlock(const QString& s, const QString& b);
public:
    QString paramType(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cParamType& o);
    QString sysParams(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cSysParam& o);
    QString ifType(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cIfType& o);
    QString tableShapes(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cTableShape& o);
    QString menuItems(eEx __ex = EX_NOOP);
    QString menuItems(const QString& _app, qlonglong upperMenuItemId = NULL_ID, eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cMenuItem &o);
    QString enumVals(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cEnumVal &o);
    QString services(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cService& o);
    QString queryParser(eEx __ex = EX_NOOP);
};

#endif // EXPORT_H
