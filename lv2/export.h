#ifndef EXPORT_H
#define EXPORT_H
#include "lv2data.h"
#include "srvdata.h"
#include "guidata.h"
#include "vardata.h"
#include "lv2tooldata.h"
#include "lv2link.h"


template <typename IX> qlonglong getColDefaultInterval(const QString& tableName, IX ix)
{
    const cRecStaticDescr *pRecDescr = cRecStaticDescr::get(tableName);
    bool ok;
    const cColStaticDescr& colDescr = (*pRecDescr)[ix];
    QString sDef = colDescr.colDefault;
    qlonglong r = parseTimeInterval(sDef, &ok);
    return ok ? 0 : r;
}

template <typename IX> qlonglong getColDefaultInteger(const QString& tableName, IX ix)
{
    const cRecStaticDescr *pRecDescr = cRecStaticDescr::get(tableName);
    bool ok;
    const cColStaticDescr& colDescr = (*pRecDescr)[ix];
    QString sDef = colDescr.colDefault;
    qlonglong r = sDef.toLongLong(&ok);
    return ok ? 0 : r;
}

#define EXPORT_INDENT_SIZE  4

enum eExportPotential {
    EXPORT_NOT    = 0,
    EXPORT_TABLE  = 1,
    EXPORT_OBJECT = 2,
    EXPORT_INH    = 4,
    EXPORT_ANY    = EXPORT_TABLE | EXPORT_OBJECT,
    EXPORT_ANY_INH= EXPORT_ANY | EXPORT_INH,
    EXPORT_OBJ_INH= EXPORT_OBJECT | EXPORT_INH
};

#define X_EXPORTABLE_OBJECTS \
    X(ParamType,    EXPORT_ANY) \
    X(SysParam,     EXPORT_ANY) \
    X(ServiceType,  EXPORT_ANY) \
    X(Service,      EXPORT_ANY) \
    X(QueryParser,  EXPORT_TABLE) \
    X(IfType,       EXPORT_ANY) \
    X(TableShape,   EXPORT_ANY) \
    X(MenuItem,     EXPORT_TABLE) \
    X(EnumVal,      EXPORT_ANY) \
    X(ServiceVarType,EXPORT_ANY) \
    X(ServiceVar,   EXPORT_ANY) \
    X(Patch,        EXPORT_ANY) \
    X(Node,         EXPORT_ANY_INH) \
    X(SnmpDevice,   EXPORT_ANY_INH) \
    X(Tool,         EXPORT_ANY) \
    X(PhsLink,      EXPORT_OBJECT)

enum eExportableObjects {
#define X(e, f)       EO_##e,
    X_EXPORTABLE_OBJECTS
#undef X
};

class LV2SHARED_EXPORT cExport : public QObject {
public:
    cExport(QObject *par = nullptr);
    static QString escaped(const QString& s);
    static QString escaped(const QStringList& sl);
    static bool isShape(const QString& _name) { return _name.endsWith("_shape"); }
    static QString chopedShape(const QString& _name) { return _name.chopped(6); }
    bool setShape(const QString& _name) { return shape = isShape(_name); }
protected:
    QString sNoAnyObj;
    int     actIndent;
    QString divert;
    QStringList exportedNames;
    bool    shape;
    static QStringList _exportableObjects;
    static QList<int>  _exportPotentials;

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
    static QString head(const QString& kw, const QString& s, const QString &n);
    static QString head(const QString& kw, const cRecord &o) { return head(kw, o.getName(), o.getNote()); }
    static QString name(const cRecord& o, bool sp = true) { return str(o[o.nameIndex()], sp); }
    static QString note(const cRecord& o, bool sp = true) { return str_z(o[o.noteIndex()], sp, true); }
    static QString str(const cRecordFieldConstRef& fr, bool sp = true);
    static QString str_z(const cRecordFieldConstRef &fr, bool sp = true, bool skipEmpty = false);
    static QString int_z(const cRecordFieldConstRef &fr, bool sp = true);
    static QString value(QSqlQuery& q, const cRecordFieldRef &fr, bool sp = true);
    static QString langComment();
    QString lineText(const QString &kw, const cRecord& o, int _tix);
    /// Title texts
    /// @param kw Keyword(s)
    /// @param o Bbject
    /// @param _fr First text index
    /// @param _to Last text index
    QString lineTexts(const QString &kw, const cRecord& o, int _fr, int _to);
    QString features(const cRecord &o);
    QString paramLine(QSqlQuery &q, const QString& kw, const cRecordFieldRef& fr, const QVariant &_def = QVariant());
    QString flag(const QString& kw, const cRecordFieldRef& fr, bool inverse = false);
    QString lineBeginBlock(const QString& s) { QString r = line(s + " {"); actIndent++; return r; }
    QString lineEndBlock();
    QString lineEndBlock(const QString& s, const QString& b);
    QString block(const QString& head, const QString& bb);
    QString hostService(QSqlQuery& q, qlonglong _id);
public:
    static const QStringList& exportableObjects();
    static QStringList exportable(int msk);
    static int isExportable(int ix)              { return isContIx(_exportPotentials, ix) ? _exportPotentials[ix] : EXPORT_NOT; }
    static int isExportable(const QString& name) { return isExportable(exportableObjects().indexOf(name));  }
    static int isExportable(const cRecord& o)    { return isExportable(o.tableName()); }
    QString exportTable(const QString& name, eEx __ex = EX_NOOP);
    QString exportTable(int ix, eEx __ex = EX_NOOP);
    static QString exportObject(QSqlQuery &q, cRecord& o, eEx __ex = EX_ERROR);
    QString ParamTypes(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cParamType& o);
    QString SysParams(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cSysParam& o);
    QString IfTypes(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cIfType& o);
    QString TableShapes(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cTableShape& o);
    QString MenuItems(eEx __ex = EX_NOOP);
    QString MenuItems(const QString& _app, qlonglong upperMenuItemId = NULL_ID, eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cMenuItem &o);
    QString EnumVals(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cEnumVal &o);
    QString ServiceTypes(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cServiceType &o);
    QString Services(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cService& o);
    QString ServiceVars(eEx __ex = EX_NOOP);
    /// A változó aktuális értékeit, és az állapotot nem írja ki.
    QString _export(QSqlQuery &q, cServiceVar &o);
    QString ServiceVarTypes(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery &q, cServiceVarType& o);
    QString QueryParsers(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery&, cQueryParser&);
    QString Patchs(eEx __ex = EX_NOOP, bool only = true);
    QString _export(QSqlQuery& q, cPatch& o, bool only = true);
    QString Nodes(eEx __ex = EX_NOOP, bool only = false);
    QString _export(QSqlQuery& q, cNode& o, bool only = true);
    QString SnmpDevices(eEx __ex, bool only = false);
    QString _export(QSqlQuery& q, cSnmpDevice& o);
    QString Tools(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery& q, cTool& o);
    QString PhsLinks(eEx __ex = EX_NOOP);
    QString _export(QSqlQuery& q, cPhsLink& o);
private:
    template <class O, class P> QString oParam(tOwnRecords<P, O>& list);
    QString oAddress(tOwnRecords<cIpAddress, cInterface>& as, int first = 0);
    QString nodeCommon(QSqlQuery& q, cNode &o);
    QString fieldSetLine(QSqlQuery& q, cRecord &o, const QString fn) {
        static const QString sFSet = "SET \"%1\" =";
        return paramLine(q, sFSet.arg(fn), o[fn]);
    }
};

#endif // EXPORT_H
