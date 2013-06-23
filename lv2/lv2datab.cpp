#include "lanview.h"
// #include "lv2data.h"
// #include "scan.h"
// #include "lv2service.h"
#include "others.h"


bool metaIsInteger(int id)
{
    switch (id) {
    case QMetaType::Char:
    case QMetaType::UChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Int:
    case QMetaType::UInt:       return true;

    default:                    return false;
    }
}

bool variantIsInteger(const QVariant & _v)
{
    return metaIsInteger(_v.userType());
}

bool metaIsString(int id)
{
    switch (id) {
    case QMetaType::QByteArray:
    case QMetaType::QChar:
    case QMetaType::QString:    return true;

    default:                    return false;
    }
}
bool variantIsString(const QVariant & _v)
{
    return metaIsString(_v.userType());
}

bool metaIsFloat(int id)
{
    switch (id) {
    case QMetaType::Double:
    case QMetaType::Float:      return true;
    default:                    return false;
    }
}

bool variantIsFloat(const QVariant & _v)
{
    return metaIsFloat(_v.userType());
}

bool variantIsNum(const QVariant & _v)
{
    return metaIsFloat(_v.userType() || metaIsInteger(_v.userType()));
}


QString langBool(bool b)
{
    return b ? QObject::trUtf8("igen") : QObject::trUtf8("nem");
}

bool str2bool(const QString& _b, bool __ex)
{
    QString b = _b.toLower();
    if (b == "t" || b == "true" || b == "y" || b == "yes" || b == "on" || b == "1") return true;
    if (b == "f" || b == "false"|| b == "n" || b == "no"  || b == "off"|| b == "0") return false;
    if (langBool(true)  == b) return true;
    if (langBool(false) == b) return false;
    if (__ex) EXCEPTION(EDBDATA, -1, b);
    return !b.isEmpty();
}

static qlonglong __schemaoid(QSqlQuery q, const QString& __s)
{
    DBGFN();
    QString sql = "SELECT oid FROM pg_namespace WHERE nspname = ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __s);
    if (!q.exec()) SQLQUERYERR(q);
    if (!q.first()) EXCEPTION(EDBDATA, 0, QString("Schema %1 OID not found.").arg(__s));
    return q.value(0).toInt();
}

qlonglong schemaoid(QSqlQuery q, const QString& __s)
{
    DBGFN();
    static qlonglong _publicSchemaOId = NULL_ID;
    if (__s.isEmpty() || __s == _sPublic) {
        if (_publicSchemaOId == NULL_ID) {
            _publicSchemaOId = __schemaoid(q, _sPublic);
        }
        return _publicSchemaOId;
    }
    return __schemaoid(q, __s);
}

qlonglong tableoid(QSqlQuery q, const QString& __t, qlonglong __sid, bool __ex)
{
    DBGFN();
    if (__sid == NULL_ID) __sid = schemaoid(q, _sPublic);
    QString sql = "SELECT oid FROM pg_class WHERE relname = ? AND relnamespace = ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __t);
    q.bindValue(1, __sid);
    if (!q.exec()) SQLQUERYERR(q);
    if (!q.first()) {
        if (__ex) EXCEPTION(EDBDATA, 0, QString("Table %1.%2 OID not found.").arg(__sid).arg(__t));
        return NULL_ID;
    }
    return q.value(0).toInt();
}

QStringPair tableoid2name(QSqlQuery q, qlonglong toid)
{
    QString sql =
            " SELECT pg_class.relname, pg_namespace.nspname"
            " FROM pg_class"
            " JOIN pg_namespace ON pg_namespace.oid = pg_class.relnamespace"
            " WHERE pg_class.oid = " + QString::number(toid);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (!q.first()) EXCEPTION(EDBDATA, 0, QString("Table OID %1 not found.").arg(toid));
    QStringPair r;
    r.first  = q.value(0).toString();
    r.second = q.value(1).toString();
    return r;
}


/* ******************************************************************************************************* */

cColStaticDescr::cColStaticDescr(int __t)
    : QString()
    , colType()
    , udtName()
    , colDefault()
    , enumVals()
    , fKeySchema()
    , fKeyTable()
    , fKeyField()
    , fKeyTables()
    , fnToName()
{
    isNullable  = false;
    pos =  ordPos = -1;
    eColType = __t;
    isUpdatable = false;
    fKeyType = FT_NONE;
    pFRec    = NULL;
}

cColStaticDescr::cColStaticDescr(const cColStaticDescr& __o)
    : QString(__o)
    , colType(__o.colType)
    , udtName(__o.udtName)
    , colDefault(__o.colDefault)
    , enumVals(__o.enumVals)
    , fKeySchema(__o.fKeySchema)
    , fKeyTable(__o.fKeyTable)
    , fKeyField(__o.fKeyField)
    , fKeyTables(__o.fKeyTables)
    , fnToName(__o.fnToName)
{
    isNullable  = __o.isNullable;
    ordPos      = __o.ordPos;
    pos         = __o.pos;
    eColType    = __o.eColType;
    isUpdatable = __o.isUpdatable;
    fKeyType    = __o.fKeyType;
    pFRec       = __o.pFRec;
}

cColStaticDescr& cColStaticDescr::operator=(const cColStaticDescr __o)
{
    *(QString *)this = __o;
    colType     = __o.colType;
    udtName     = __o.udtName;
    colDefault  = __o.colDefault;
    isNullable  = __o.isNullable;
    ordPos      = __o.ordPos;
    pos         = __o.pos;
    enumVals    = __o.enumVals;
    eColType    = __o.eColType;
    isUpdatable = __o.isUpdatable;
    fKeySchema  = __o.fKeySchema;
    fKeyTable   = __o.fKeyTable;
    fKeyField   = __o.fKeyField;
    fKeyTables  = __o.fKeyTables;
    fKeyType    = __o.fKeyType;
    fnToName    = __o.fnToName;
    pFRec       = __o.pFRec;
    return *this;
}

QString cColStaticDescr::toString() const
{
    QString r;
    r  = dQuoted((QString&)*this) + _sSpace + colType + "/" + udtName;
    if (enumVals.size())       r += _sCBraB + enumVals.join(_sComma) + _sCBraE;
    if (!isNullable)           r += " NOT NULL ";
    if (!colDefault.isEmpty()) r += " DEFAULT " + colDefault;
    r += (isUpdatable ? _sSpace : QString(" nem")) + " módosítható";
    return r;
}

QVariant cColStaticDescr::fromSql(const QVariant& _f) const
{
    if (_f.isNull()) return _f;
    return _f; // Tfh. ez igy jó
}

QVariant cColStaticDescr::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    return _f; // Tfh. ez igy jó
}
/**
A megadott értéket konvertálja a tárolási típussá, ami :
- eColType == FT_INTGER esetén qlonglong
- eColType == FT_REAL esetén double
- eColType == FT_TEXT esetén QString
Ha a paraméter típusa qlonglong, akkor a NULL_ID, ha int, akkor a NULL_IX is null objektumot jelent.
Ha a konverzió nem lehetséges, akkor a visszaadott érték egy üres objektum lessz, és a
statusban a ES_DEFECTIVE bit be lessz billentve (ha nincs hiba, akkor a bit nem törlődik).
Ha üres objektumot adunk meg, és isNullable értéke false, akkor szintén be lessz állítva a hiba bit.
@param _f A konvertálandó érték
@param str A status referenciája
@return A konvertált érték
@exception cError Ha eColType értéke nem a fenti három érték eggyike.
*/
QVariant cColStaticDescr::set(const QVariant& _f, int &str) const
{
    QVariant r =_f;
    bool ok = true;
    if ((eColType != FT_TEXT && variantIsString(_f) && _f.toString().isEmpty())
     || (QMetaType::LongLong == _f.userType() && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == _f.userType() && NULL_IX == _f.toInt())) {
        r.clear();
    }
    if (r.isNull()) {
        ok = isNullable || !colDefault.isEmpty();
    }
    else switch (eColType) {
    case FT_INTEGER:
        r = _f.toLongLong(&ok);
        break;
    case FT_REAL:
        r = _f.toDouble(&ok);
        break;
    case FT_TEXT:
        ok = _f.canConvert<QString>();
        r = _f.toString();
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (!ok) str |= cRecord::ES_DEFECTIVE;
    return r;
}

QString cColStaticDescr::toName(const QVariant& _f) const
{
    return QVariantToString(_f);
}

qlonglong cColStaticDescr::toId(const QVariant& _f) const
{
    if (_f.isNull()) return NULL_ID;
    bool ok;
    qlonglong r = _f.toLongLong(&ok);
    return ok ? r : NULL_ID;
}

QString cColStaticDescr::toView(QSqlQuery& q, const QVariant &_f) const
{
    static QString  rNul = QObject::trUtf8("[NULL]");
    if (_f.isNull()) return rNul;
    if (eColType == FT_INTEGER) {
        qlonglong id = toId(_f);
        if (id == NULL_ID) return rNul; //?!
        QString r = QString::number(id);
        QString h = "#";
        if (fnToName.isEmpty() == false) {
            QString sql = "SELECT " + fnToName + _sABraB + r + _sABraE;
            if (!q.exec(sql)) SQLPREPERR(q, sql);
            if (q.first()) return q.value(0).toString();
            return h + r;
        }
        if (fKeyTable.isEmpty() == false) {
            if (pFRec == NULL) {
                // Sajnos itt trükközni kell, mivel ezt máskor nem tehetjük meg, ill veszélyes, macerás, de itt meg konstans a pointer
                *const_cast<cAlternate **>(&pFRec) = new cAlternate(fKeyTable, fKeySchema);
            }
            QString n = pFRec->getNameById(q, id , false);
            if (n.isEmpty()) return h + r;
            return n;
        }
    }
    return toName(_f);
}

#define CDDUPDEF(T)     cColStaticDescr *T::dup() const { return new T(*this); }

CDDUPDEF(cColStaticDescr)

/// @def TYPEDETECT
/// A makró a colType mezőből azonosítható típusokra (tömb ezeknél a típusoknál nem támogatott)
#define TYPEDETECT(p,c)    if (0 == colType.compare(p, Qt::CaseInsensitive)) { eColType = c; return; }
/// @def TYPEDETUDT
/// A makró a udtName mezőből azonosítható típusokra (tömb ezeknél a típusoknál nem támogatott)
#define TYPEDETUDT(p,c)    if (0 == udtName.compare(p, Qt::CaseInsensitive)) { eColType = c; return; }

void cColStaticDescr::typeDetect()
{
    TYPEDETECT(_sByteA,    FT_BINARY)
    TYPEDETECT(_sPolygon,  FT_POLYGON)
    TYPEDETECT(_sBoolean,  FT_BOOLEAN)
    TYPEDETECT("macaddr",  FT_MAC)
    TYPEDETECT("inet",     FT_INET)
    TYPEDETECT("cidr",     FT_CIDR)
    TYPEDETUDT("time",     FT_TIME)
    TYPEDETUDT("date",     FT_DATE)
    TYPEDETUDT("timestamp",FT_DATE_TIME)
    TYPEDETUDT("interval", FT_INTERVAL)
    if      (enumVals.count() > 0)                         eColType = FT_ENUM;
    else if (udtName.contains("int",Qt::CaseInsensitive))  eColType = FT_INTEGER;
    else if (udtName.contains("real",Qt::CaseInsensitive)) eColType = FT_REAL;
    else if (udtName.contains("char",Qt::CaseInsensitive)) eColType = FT_TEXT;
    else {
        DERR() << "Unknown field type : " << colType << _sSlash << udtName << endl;
        eColType = FT_ANY;
    }
    if (colType == _sARRAY) eColType |= FT_ARRAY;
    return;
}

/* ------------------------------------------------------------------------------------------------------- */

QVariant  cColStaticDescrBool::fromSql(const QVariant& _f) const
{
    return str2bool(_f.toString(), true);
}
QVariant  cColStaticDescrBool::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    if (_f.userType() == QMetaType::Bool) return QVariant(QString(_f.toBool() ? "true":"false"));
    bool ok;
    qlonglong i = _f.toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA,-1,"Az adat nem értelmezhető.");
    return QVariant(QString(i ? "true":"false"));
}

/**
A megadott értéket konvertálja a tárolási típussá, ami bool.
Ha a paraméter típusa qlonglong, akkor a NULL_ID, ha int, akkor a NULL_IX is null objektumot jelent.
Ha a konverzió nem lehetséges, akkor a visszaadott érték egy üres objektum lessz, és a
statusban a ES_DEFECTIVE bit be lessz billentve (ha nincs hiba, akkor a bit nem törlődik).
Ha üres objektumot adunk meg, és isNullable értéke false, akkor szintén be lessz állítva a hiba bit.
@param _f A konvertálandó érték
@param str A status referenciája
@return A konvertált érték
*/
QVariant  cColStaticDescrBool::set(const QVariant& _f, int & str) const
{
    bool ok = true;
    QVariant r =_f;
    if ((QMetaType::LongLong == _f.userType() && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == _f.userType() && NULL_IX == _f.toInt())) r.clear();
    if (r.isNull()) {
        ok = isNullable || !colDefault.isEmpty();
    }
    else if (variantIsString(_f)) {
        r = str2bool(_f.toString(), false);
    }
    else if (variantIsInteger(_f)) {
        r = _f.toLongLong(&ok) != 0;
    }
    else {
        ok = _f.canConvert<bool>();
        r = _f.toBool();
    }
    if (!ok) str |= cRecord::ES_DEFECTIVE;
    return r;
}
QString   cColStaticDescrBool::toName(const QVariant& _f) const
{
    return _f.toBool() ? _sTrue : _sFalse;
}
qlonglong cColStaticDescrBool::toId(const QVariant& _f) const
{
    return _f.toBool() ? 1 : 0;
}

QString cColStaticDescrBool::toView(QSqlQuery&, const QVariant& _f) const
{
    return _f.toBool() ? enumVals[1] : enumVals[0];
}
void cColStaticDescrBool::init()
{
    enumVals << langBool(false);
    enumVals << langBool(true);
}

CDDUPDEF(cColStaticDescrBool)
/* ....................................................................................................... */

QVariant  cColStaticDescrArray::fromSql(const QVariant& _f) const
{
    if (_f.isNull()) return _f;
    // A tömböket stringen keresztül bontjuk ki
    QString s = _f.toString();
    // Ez a hiba üzenethez kelhet
    QString em = QString("ARRAY %1 = %2").arg(colName()).arg(s);
    // A tömb { ... } zárójelek közt kell legyen
    if (s.at(0) != QChar('{') || !s.endsWith(QChar('}'))) EXCEPTION(EDBDATA, 1, em);
    s = s.mid(1, s.size() -2);  // Lekapjuk a kapcsos zárójelet
    // Az elemek közötti szeparátor a vessző
    QStringList sl = s.split(QChar(','),QString::KeepEmptyParts);
    int t = eColType & ~FT_ARRAY;
    if (t == FT_INTEGER) {         // egész tömb
         QVariantList vl;
         if (s.size() > 0) foreach (const QString& si, sl) {
             bool ok;
             qlonglong i = si.toLongLong(&ok);
             if (!ok) EXCEPTION(EDBDATA, 2, "Invalid number : " + em);
             vl << QVariant(i);
         }
         return QVariant(vl);
    }
    if (t == FT_REAL) {
        QVariantList vl;
        if (s.size() > 0) foreach (const QString& si, sl) {
            bool ok;
            double i = si.toDouble(&ok);
            if (!ok) EXCEPTION(EDBDATA, 3, "Invalid number : " + em);
            vl << QVariant(i);
        }
        return QVariant(vl);
    }
    // A többiről feltételezzük, hogy string, De kell valamit kezdeni az idézőjelekkel
    const   QChar   m('"');
    for (int i = 0; i < sl.size(); ++i) {
        s = sl[i];
        if (s[0] == m) {
            while (! s.endsWith(m)) {   // Ha szétdaraboltunk egy stringet, újra összerakjuk
                if (sl.size() <= (i +1)) EXCEPTION(EDBDATA, 8, "Invalid string array : " + em);
                sl[i] += QChar(',') + (s = sl[i +1]);
                sl.removeAt(i +1);
            }
            s = sl[i] = sl[i].mid(1, sl[i].size() -2);  // Lekapjuk az idézőjelet
        }
        if (s.isEmpty()) sl.removeAt(i--);  // Az üres stringek eltávolítása
    }
    if (sl.isEmpty() && isNullable) return QVariant();
    return QVariant(sl);
}

QVariant  cColStaticDescrArray::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    bool    empty = true;
    QString s;
    switch (eColType) {
    case FT_INTEGER_ARRAY: {    // egész tömb
        QVariantList vl = _f.toList();
        s = _sCBraB;
        bool ok;
        foreach (const QVariant& vi, vl) {
            s += QString::number(vi.toLongLong(&ok)) + _sComma;
            if (!ok) EXCEPTION(EDATA,-1,"Invalid number");
            empty = false;
        }
        if (empty == false) s.chop(1);
        s += _sCBraE;
        return QVariant(s);
    }
    case FT_REAL_ARRAY:  {
        QVariantList vl = _f.toList();
        s = _sCBraB;
        bool ok;
        foreach (const QVariant& vi, vl) {
            s += QString::number(vi.toDouble(&ok)) + _sComma;
            if (!ok) EXCEPTION(EDATA,-1,"Invalid number");
            empty = false;
        }
        if (empty == false) s.chop(1);
        s += _sCBraE;
        return QVariant(s);
    }
    case FT_TEXT_ARRAY: {
        QStringList sl = _f.toStringList();
        s = _sCBraB;
        foreach (const QString& si, sl) {
            s += dQuoted(si) + _sComma;
            empty = false;
        }
        if (empty == false) s.chop(1);
        s += _sCBraE;
        return QVariant(s);
    }
    default:
        EXCEPTION(EPROGFAIL);
    }
    return QVariant();  // Csak hogy ne legeyen warning...
}
/**
A megadott értéket konvertálja a tárolási típussá, ami :\n
- eColType == FT_INTGER_ARRAY esetén QVariantList\n
- eColType == FT_REAL_ARRAY esetén QVariantList\n
- eColType == FT_TEXT_ARRAY esetén QStringList\n
Ha a paraméter típusa qlonglong, akkor a NULL_ID, ha int, akkor a NULL_IX is null objektumot jelent.
Ha a konverzió nem lehetséges, akkor a visszaadott érték egy üres objektum lessz, és a
statusban a ES_DEFECTIVE bit be lessz billentve (ha nincs hiba, akkor a bit nem törlődik).
Ha üres objektumot adunk meg, és isNullable értéke false, akkor szintén be lessz állítva a hiba bit.
Ha a paraméter és a tárolási típus is QVariantList, akkor a lista elemeket nem ellenőrzi.
@param _f A konvertálandó érték
@param str A status referenciája
@return A konvertált érték
@exception cError Ha eColType értéke nem a fenti három érték eggyike.
*/
QVariant  cColStaticDescrArray::set(const QVariant& _f, int &str) const
{
    QVariant r =_f;
    if ((QMetaType::LongLong == _f.userType() && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == _f.userType() && NULL_IX == _f.toInt())) r.clear();
    if (r.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return r;
    }
    int t = _f.userType();
    QVariantList vl;
    QStringList sl;
    bool ok = true;
    if (QMetaType::QVariantList == t)  switch (eColType) {
    case FT_INTEGER_ARRAY:
    case FT_REAL_ARRAY:
        return _f;
    case FT_TEXT_ARRAY:
        foreach (QVariant v, _f.toList()) {
            sl << QVariantToString(v, &ok);
            if (!ok) str |= cRecord::ES_DEFECTIVE;
        }
        return QVariant(sl);
    default:
        EXCEPTION(EPROGFAIL);
    }
    if (QMetaType::QStringList == t)  switch (eColType) {
    case FT_TEXT_ARRAY:
        return _f;
    case FT_INTEGER_ARRAY:
        foreach (QString s, _f.toStringList()) {
            vl << QVariant(s.toLongLong(&ok));
            if (!ok) str |= cRecord::ES_DEFECTIVE;
        }
        return QVariant(vl);
    case FT_REAL_ARRAY:
        foreach (QString s, _f.toStringList()) {
            vl << QVariant(s.toDouble(&ok));
            if (!ok) str |= cRecord::ES_DEFECTIVE;
        }
        return QVariant(vl);
    default:
        EXCEPTION(EPROGFAIL);
    }
    switch (eColType) {
    case FT_TEXT_ARRAY:
        sl << QVariantToString(_f, &ok);
        if (!ok) str |= cRecord::ES_DEFECTIVE;
        return QVariant(sl);
    case FT_INTEGER_ARRAY:
        if (_f.canConvert<qlonglong>()) vl << _f.toLongLong();
        else str |= cRecord::ES_DEFECTIVE;
        return QVariant(vl);
    case FT_REAL_ARRAY:
        if (_f.canConvert<double>()) vl << _f.toDouble();
        else str |= cRecord::ES_DEFECTIVE;
        return QVariant(vl);
    default:
        EXCEPTION(EPROGFAIL);
    }
    return _f; // Csak a warning miatt
}
QString   cColStaticDescrArray::toName(const QVariant& _f) const
{
    return QVariantToString(_f);
}
qlonglong cColStaticDescrArray::toId(const QVariant& _f) const
{
    return _f.toList().size();
}

QString cColStaticDescrArray::toView(QSqlQuery &q, const QVariant &_f) const
{
    (void)q;
    return _f.toStringList().join(QChar(','));
}
CDDUPDEF(cColStaticDescrArray)
/* ....................................................................................................... */

QVariant  cColStaticDescrEnum::fromSql(const QVariant& _f) const
{
    return _f;
}
QVariant  cColStaticDescrEnum::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    return _f;
}
/**
A megadott értéket konvertálja a tárolási típussá, ami QString
Ha a paraméter típusa qlonglong, akkor a NULL_ID, ha int, akkor a NULL_IX is null objektumot jelent.
Ha a konverzió nem lehetséges, akkor a visszaadott érték egy üres objektum lessz, és a
statusban a ES_DEFECTIVE bit be lessz billentve (ha nincs hiba, akkor a bit nem törlődik).
Ha üres objektumot adunk meg, és isNullable értéke false, akkor szintén be lessz állítva a hiba bit.
Ha a megadott objektum típusa egész szám, akkor a konvertált érték az azonos indexű enumVal string lessz,
Ha nincs elyen indexű enumVal érték, akkor be lessz állítva a hiba bit, és a visszaadott érteék
egy üres objektum.
@param _f A konvertálandó érték
@param str A status referenciája
@return A konvertált érték
@exception cError Ha az enumVals adattag üres, és szükséges a konverzióhóz.
*/
QVariant  cColStaticDescrEnum::set(const QVariant& _f, int & str) const
{
    QVariant r =_f;
    qlonglong i;
    bool ok = true;
    if ((QMetaType::LongLong == _f.userType() && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == _f.userType() && NULL_IX == _f.toInt())
     || r.isNull()) {
        ok = isNullable || !colDefault.isEmpty();
        r.clear();
    }
    else if (variantIsInteger(r)) {
        if (enumVals.isEmpty()) EXCEPTION(EPROGFAIL);
        i = r.toLongLong();
        if (isContIx(enumVals, i)) {
            r = enumVals[i];
        }
        else {
            ok = false;
        }
    }
    else if (!r.convert(QMetaType::QString) || !enumVals.contains(r.toString())) {
        ok = false;
    }
    if (!ok) {
        DWAR() << "Invalid enum value : " << debVariantToString(_f) << endl;
        str |= cRecord::ES_DEFECTIVE;
        r.clear();
    }
    return r;
 }

QString   cColStaticDescrEnum::toName(const QVariant& _f) const
{
    return _f.toString();
}
/// Az enumerációs értéket, amely mindíg stringként van letárolva, egész számértékké konvertálja.
/// Enumeráció esetén a numerikus érték az adatbázisban az enum típusban megadott listabeli sorszáma (0,1 ...)
/// Az API-ban lévő sring - enumeráció kovertáló függvényeknél ügyelni kell, hogy a C-ben definiált
/// enumerációs értékek megfeleljenek az adatbázisban a megfelelő enumerációs érték sorrendjének.
/// Az enumerációk konzisztens kezelése a bool checkEnum(const cColStaticDescr& descr, tE2S e2s, tS2E s2e);
/// függvénnyel vagy a CHKENUM makróval ellenőrizhető le.
qlonglong cColStaticDescrEnum::toId(const QVariant& _f) const
{
    QString s = _f.toString();
    if (s.isEmpty()) return NULL_ID;
    if (enumVals.isEmpty()) EXCEPTION(EPROGFAIL);
    qlonglong i = enumVals.indexOf(s);
    if (i < 0) EXCEPTION(EDATA, -1, s);
    return i;
}

CDDUPDEF(cColStaticDescrEnum)
/* ....................................................................................................... */

QVariant  cColStaticDescrSet::fromSql(const QVariant& _f) const
{
    QString s = _f.toString();
    // Ez a hiba üzenethez kelhet
    QString em = QString("ARRAY %1 = %2").arg(colName()).arg(s);
    // A tömb { ... } zárójelek közt kell legyen
    if (s.at(0) != QChar('{') || !s.endsWith(QChar('}'))) EXCEPTION(EDBDATA, 1, em);
    s = s.mid(1, s.size() -2);  // Lekapjuk a kapcsos zárójelet
    // Az elemek közötti szeparátor a vessző, elvileg nincsenek idézőjelek
    QStringList sl = s.split(QChar(','),QString::KeepEmptyParts);
    if (sl.isEmpty() && isNullable) return QVariant();
    return QVariant(sl);
}
QVariant  cColStaticDescrSet::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    QString s;
    QStringList sl = _f.toStringList();
    s = _sCBraB + sl.join(_sComma) + _sCBraE;   // Feltételezzük, hogy nem kell idézőjelbe rakni
    //PDEB(VVERBOSE) << "_toSql() string list -> string : " << s << endl;
    return QVariant(s);
}
/**
A megadott értéket konvertálja a tárolási típussá, ami QStringList
Ha a paraméter típusa qlonglong, akkor a NULL_ID, ha int, akkor a NULL_IX is null objektumot jelent.
Ha a konverzió nem lehetséges, akkor a visszaadott érték egy üres objektum lessz, és a
statusban a ES_DEFECTIVE bit be lessz billentve (ha nincs hiba, akkor a bit nem törlődik).
Ha üres objektumot adunk meg, és isNullable értéke false, akkor szintén be lessz állítva a hiba bit.
Ha a megadott objektum típusa egész szám, akkor a konvertált bináris értékben minden 1-es bittel az azonos
indexű enumVal string tagja lessz a konvertélt tömbnek, az extra bitek figyelmen kívül lesznek hagyva (nincs hiba bit billentés).
Ha olyan elemeket adunk meg, amik nincsenek az enumVals -ban, akkor azok az elemek nem kerülnek bele a konvertált listába,
és ekkor be lessz állítva a hiba bit a státuszban. A cél értékben a duplikált elemek törölve lesznek.
@param _f A konvertálandó érték
@param str A status referenciája
@return A konvertált stringlista vagy null objektum
@exception cError Ha az enumVals adattag üres, és szükséges a konverzióhóz/ellenörzéshez
*/
QVariant  cColStaticDescrSet::set(const QVariant& _f, int & str) const
{
    _DBGFN() << debVariantToString(_f) << endl;
    int t = _f.userType();
    if ((QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())
     || _f.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    QStringList sl;
    bool ok = true;
    if (metaIsInteger(t)) {
        if (enumVals.isEmpty()) EXCEPTION(EPROGFAIL);
        qlonglong i,m, v = _f.toLongLong();
        for (m = 1, i = 0; m <= v && i < enumVals.size() && 0 < m; m <<= 1, i++) {
            if (m & v) sl << enumVals[i];
        }
        return QVariant(sl);
    }
    else if (metaIsString(t)) {
        sl = _f.toString().split(_sComma);
    }
    else if (QMetaType::QStringList == t || QMetaType::QVariantList == t) {
        sl = _f.toStringList();
    }
    sl.removeDuplicates();
    foreach (QString s, sl) {
        if (!enumVals.contains(s)) {
            ok = false;
            sl.removeOne(s);
        }
    }
    if (!ok) str |= cRecord::ES_DEFECTIVE;
    return QVariant(sl);
 }

QString   cColStaticDescrSet::toName(const QVariant& _f) const
{
    return _f.toStringList().join(_sComma);
}
/// A SET (ENUM ARRAY) esetén string listaként letárolt értéket egész számértékké konvertálja.
/// A numerikus értékben a megadott sorszámú bit reprezentál egy enumerációs értéket.
/// Az API-ban lévő sring - enumeráció kovertáló függvényeknél ügyelni kell, hogy a C-ben definiált
/// enumerációs értékek megfeleljenek az adatbázisban a megfelelő enumerációs érték sorrendjének.
/// Az enumerációk konzisztens kezelése a bool checkEnum(const cColStaticDescr& descr, tE2S e2s, tS2E s2e);
/// függvénnyel vagy a CHKENUM makróval ellenőrizhető le.
qlonglong cColStaticDescrSet::toId(const QVariant& _f) const
{
    if (_f.isNull()) return 0;
    if (enumVals.isEmpty()) EXCEPTION(EPROGFAIL);
    qlonglong r = 0;
    QStringList vl = _f.toStringList();
    foreach (const QString& v, vl) {
        int i = enumVals.indexOf(v);
        if (i >= 0) r |= enum2set(i);
        else DWAR() << "Invalid enum " << udtName << " value : " << v << endl;
    }
    return r;
}

CDDUPDEF(cColStaticDescrSet)
/* ....................................................................................................... */

QVariant  cColStaticDescrPolygon::fromSql(const QVariant& _f) const
{
    if (_f.isNull()) return _f;
    QString s = _f.toString();
    QString em = QString("Polygon %1 = %2").arg(colName()).arg(s);
    // Az egésznek zárójelben kell lennie
    if (s.at(0) != QChar('(') || !s.endsWith(QChar(')'))) EXCEPTION(EDBDATA, 1, em);
    s = s.mid(1, s.size() -2);
    // PDEB(VVERBOSE) << "Point list : \"" << s << "\"" << endl;
    QPolygonF    pol;
    if (s.isEmpty() && isNullable) return QVariant();
    // A pontok, koordináta párok vesszővel vannak elválasztva
    QStringList sl = s.split(QChar(','),QString::KeepEmptyParts);
    bool ok, x = true;  // elöször x
    QPointF  pt;
    foreach (QString sp, sl) {
        if (x) {
            // Az x elött lessz egy zárójel.
            if (sp.at(0) != QChar('(')) EXCEPTION(EDBDATA, 2, em);
            sp = sp.mid(1);     // a zárójelet lecsípjük
            pt.setX(sp.toDouble(&ok));
            if (!ok) EXCEPTION(EDBDATA, 4, em);
        }
        else { //y
            // Az y után lessz egy zárójel.
            if (!sp.endsWith(QChar(')'))) EXCEPTION(EDBDATA, 4, em);
            sp.chop(1);     // a zárójelet lecsípjük
            pt.setY(sp.toDouble(&ok));
            if (!ok) EXCEPTION(EDBDATA, 5, em);
            pol << pt;
        }
        x = !x;
    }
    if (!x) EXCEPTION(EDBDATA, 6, em);
    return QVariant::fromValue(pol);
}

QVariant  cColStaticDescrPolygon::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    bool    empty = true;
        QPolygonF    pol = _f.value<QPolygonF>();    // = _f;
        if (pol.isEmpty()) return QVariant();
        QString     s = _sABraB;
        foreach(const QPointF& pt, pol) {
            s += _sABraB + QString::number(pt.x()) + _sComma + QString::number(pt.y()) + _sABraE + _sComma;
            empty = false;
        }
        if (empty == false) s.chop(1);
        s += _sABraE;
        //PDEB(VVERBOSE) << "_toSql() polygon -> string : " << s << endl;
        return QVariant(s);
    return _f; // Tfh. ez igy jó
}
QVariant  cColStaticDescrPolygon::set(const QVariant& _f, int& str) const
{
    int t = _f.userType();
    if ((QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())
     || _f.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    switch (t) {
    case QMetaType::QPolygon: {
        QPolygonF   pol;
        foreach (QPoint p, _f.value<QPolygon>()) {
            pol << QPointF(p);
        }
        return QVariant::fromValue(pol);
    }
    case QMetaType::QPolygonF:
        return _f;
    case QMetaType::QPoint:{
        QPolygonF   pol;
        pol << QPointF(_f.value<QPoint>());
        return QVariant::fromValue(pol);
    }
    case QMetaType::QPointF:{
        QPolygonF   pol;
        pol << _f.value<QPointF>();
        return QVariant::fromValue(pol);
    }
    }
    str |= cRecord::ES_DEFECTIVE;
    return QVariant();

}
QString   cColStaticDescrPolygon::toName(const QVariant& _f) const
{
    return QVariantToString(_f);
}
qlonglong cColStaticDescrPolygon::toId(const QVariant& _f) const
{
    return _f.value<QPolygonF>().size();
}

CDDUPDEF(cColStaticDescrPolygon)
/* ....................................................................................................... */

QVariant  cColStaticDescrAddr::fromSql(const QVariant& _f) const
{
    int es;
    return set(_f, es);
}
QVariant  cColStaticDescrAddr::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    QVariant r = toName(_f);
    if (r.toString().isEmpty()) EXCEPTION(EDATA,-1,"Invalid data type.");
    return r;
}
QVariant  cColStaticDescrAddr::set(const QVariant& _f, int &str) const
{
    int mtid = _f.userType();
    if ((QMetaType::LongLong == mtid && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == mtid && NULL_IX == _f.toInt())
     || _f.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    bool ok = true;
    QVariant r;
    netAddress a;
    cMac mac;
    switch (eColType) {
    case FT_MAC:
        if (mtid == _UMTID_cMac) {
            mac = _f.value<cMac>();
        }
        else if (metaIsInteger(mtid))   mac.set(_f.toLongLong());
        else if (metaIsString(mtid))    mac.set(_f.toString());
        if (mac.isValid()) r = QVariant::fromValue(mac);
        else               ok = false;
        break;
    case FT_INET:
    case FT_CIDR:
        if (mtid == _UMTID_netAddress) {
            a = _f.value<netAddress>();
        }
        else if (mtid == _UMTID_QHostAddress) {
            a = _f.value<QHostAddress>();
        }
        else if (metaIsString(mtid)) {
            QString as = _f.toString();
            if (as.count() > 0) {
                // néha hozzábigyeszt valamit az IPV6 címhez
                int i = as.indexOf(QRegExp("[^\\d\\.:A-Fa-f/]"));
                // Ha a végén van szemét, azt levágjuk
                if (i > 0) as = as.mid(0, i);
                a.set(as);
            }
        }
        if (a.isValid()) r = QVariant::fromValue(a);
        else             ok = false;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    if (!ok) str |= cRecord::ES_DEFECTIVE;
    return r;
}

QString   cColStaticDescrAddr::toName(const QVariant& _f) const
{
    switch (eColType) {
    case FT_MAC:
        if (_f.userType() == _UMTID_cMac) return _f.value<cMac>().toString();
        break;
    case FT_INET:
    case FT_CIDR:
        if (_f.userType() == _UMTID_netAddress) return _f.value<netAddress>().toString();
        break;
    default:        EXCEPTION(EPROGFAIL);
    }
    if (!_f.isNull()) DERR() << "Invalid data type : " << _f.typeName() << endl;
    return QString();
}

qlonglong cColStaticDescrAddr::toId(const QVariant& _f) const
{
    switch (eColType) {
    case FT_MAC:
        if (_f.userType() == _UMTID_cMac) return _f.value<cMac>().toLongLong();
        break;
    case FT_INET:
    case FT_CIDR:
        return 0;
        break;
    default:        EXCEPTION(EPROGFAIL);
    }
    if (!_f.isNull()) DERR() << "Invalid data type : " << _f.typeName() << endl;
    return NULL_ID;
}

CDDUPDEF(cColStaticDescrAddr)
/* ....................................................................................................... */

/// Tárolási adattípus QTime
QVariant  cColStaticDescrTime::fromSql(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticDescrTime::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    return _f;
}
QVariant  cColStaticDescrTime::set(const QVariant& _f, int& str) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    int t = _f.userType();
    if ((QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())
     || _f.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    QTime   dt;
    if (_f.canConvert<QTime>()) dt = _f.toTime();
    else if (variantIsNum(_f)) {   // millicesc to QTime
        qlonglong h = variantIsFloat(_f) ? (qlonglong)_f.toDouble() : _f.toLongLong();
        int ms = h % 1000; h /= 1000;
        int s  = h %   60; h /=   60;
        int m  = h %   60; h /=   60;
        if (h < 24) {
            dt = QTime((int)h, m, s, ms);
        }
    }
    else if (variantIsString(_f)) {
        dt = QTime::fromString(_f.toString());
    }
    if (dt.isValid()) return QVariant(dt);
    DERR() << QObject::trUtf8("Az adat nem értelmezhető  idő ként : ") << endl;
    str |= cRecord::ES_DEFECTIVE;
    return QVariant();
}
QString   cColStaticDescrTime::toName(const QVariant& _f) const
{
    _DBGFN() << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f.toTime().toString("hh:mm:ss.zzz");
}
/// Ezred másodperc értéket ad vissza
qlonglong cColStaticDescrTime::toId(const QVariant& _f) const
{
    _DBGFN() << _f.typeName() << _sCommaSp << _f.toString() << endl;
    qlonglong r;
    QTime tm = _f.toTime();
    if (tm.isNull()) return NULL_ID;
    r = tm.hour();
    r = tm.minute() + r * 60;
    r = tm.second() + r * 60;
    r = tm.msec()   + r * 1000;
    return r;
}

CDDUPDEF(cColStaticDescrTime)
/* ....................................................................................................... */

/// Tárolási adattípus QDate
QVariant  cColStaticDescrDate::fromSql(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticDescrDate::toSql(const QVariant& _f) const
{
    // _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    return _f;
}
QVariant  cColStaticDescrDate::set(const QVariant& _f, int& str) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    int t = _f.userType();
    if (_f.isNull()
     || (QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    QDate   dt;
    if (_f.canConvert<QDate>()) dt = _f.toDate();
    else if (variantIsString(_f)) {
        dt = QDate::fromString(_f.toString());
    }
    if (dt.isValid()) return QVariant(dt);
    DERR() << QObject::trUtf8("Az adat nem értelmezhető  idő ként : ") << endl;
    str |= cRecord::ES_DEFECTIVE;
    return QVariant();
}
QString   cColStaticDescrDate::toName(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f.toString();
}
qlonglong cColStaticDescrDate::toId(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isValid()) return 0;
    return NULL_ID;
}
CDDUPDEF(cColStaticDescrDate)
/* ....................................................................................................... */

static const QString _tmstmpFrm = "yyyy-MM-dd HH:mm:ss";
/// Timezone kezelés nincs, a tört másodperceket eldobja
QVariant  cColStaticDescrDateTime::fromSql(const QVariant& _f) const
{
//    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticDescrDateTime::toSql(const QVariant& _f) const
{
//    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    return _f;
}
QVariant  cColStaticDescrDateTime::set(const QVariant& _f, int& str) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    int t = _f.userType();
    if (_f.isNull()
     || (QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())
     || (QMetaType::QDateTime== t && _f.toDateTime().isNull())) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    QDateTime   dt;
    bool ok = true;
    if (_f.canConvert<QDateTime>()) dt = _f.toDateTime();
    else if (variantIsInteger(_f))  dt = QDateTime::fromTime_t(_f.toUInt(&ok));
    else if (variantIsString(_f))   dt = QDateTime::fromString(_f.toString());
    else ok = false;
    if (ok && dt.isNull()) {
        DERR() << QObject::trUtf8("Az adat nem értelmezhető dátum és idő ként.") << endl;
        str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    _DBGFNL() << " Return :" << dt.toString() << endl;
    return QVariant(dt);
}
QString   cColStaticDescrDateTime::toName(const QVariant& _f) const
{
 //   _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) return QString();
    return _f.toDateTime().toString(_tmstmpFrm);
}
qlonglong cColStaticDescrDateTime::toId(const QVariant& _f) const
{
//    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) return NULL_ID;
    return _f.toDateTime().toTime_t();
}

CDDUPDEF(cColStaticDescrDateTime)
/* ....................................................................................................... */

/// Az intervallum qlonglong-ban tárolódik, és mSec-ben értendő.
/// Konvertál egy intervallum stringet mSec-re.
/// Negatív tartomány nincs értelmezve, és az óra:perc:másodperc formán túl csak a napok megadása támogatott.
/// @param s A konvertálandó string
/// @param pOk egy bool pointer, ha nem NULL, és az s sem üres, akkor ha sikerült a konverzió a mutatott változót true-ba írja.
///            ha pOk nem NULL. és az nem üres, és a konverzió nem sikerült, akkor mutatott változót false-ba írja.
/// @return A konvertált érték, vagy NULL_ID ha üres stringet adtunk meg, vagy nem sikerült a kovverzió.
qlonglong parseTimeInterval(const QString& s, bool *pOk)
{
    if (s.isEmpty()) return NULL_ID;
    QStringList sl = s.split(QChar(' '));
    qlonglong   r = 0;
    bool ok = false;
    if (sl.size() == 3 ) {
        if (sl.first().startsWith("DAY")) r = (24 * 3600 * 1000) * sl[1].toLongLong(&ok);
    }
    else if (sl.size() == 1) {
        ok = true;
    }
    if (ok) {
        sl = sl.last().split(QChar(':'));
        if (sl.size() != 3) ok = false;
        else {
            bool ok2, ok3;
            r += sl[0].toLongLong(&ok) * 3600 * 1000;
            r += sl[1].toLongLong(&ok2) * 60 * 1000;
            r += (qlonglong)(sl[2].toDouble(&ok3) * 1000 + 0.5);
            ok = ok && ok2 && ok3;
        }
    }
    if (pOk != NULL) *pOk = ok;
    return ok ? r : NULL_ID;
}

/// Az intervallum qlonglong-ban tárolódik, és mSec-ben értendő.
/// Negatív tartomány nincs értelmezve, és az óra:perc:másodperc formán túl csak a napok megadása támogatott.
QVariant  cColStaticDescrInterval::fromSql(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    bool ok = true;
    QString s = _f.toString();
    qlonglong r = parseTimeInterval(s, &ok);
    if (!ok) EXCEPTION(EPARSE, -1, s);
    return QVariant(r);
}
QVariant  cColStaticDescrInterval::toSql(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    QString is;
    qlonglong i = _f.toLongLong();
    qlonglong j = i % 1000;
    i /= 1000;
    if (j) is = _sPoint + QString::number(j);
    is = QString::number(i % 60) + is;
    i /= 60;
    is = QString::number(i % 60) + _sColon + is;
    i /= 60;
    is = QString::number(i % 24) + _sColon + is;
    i /= 24;
    if (i) {
        is = (i == 1 ? "DAY " : "DAYS ") + QString::number(i) + _sSpace + is;
    }
    return QVariant(is);

}
/// Egy időintervallum értéket konvertál, a tárolási tíousra (mSec)
QVariant  cColStaticDescrInterval::set(const QVariant& _f, int& str) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    int t = _f.userType();
    if (_f.isNull()
     || (QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())
     || (metaIsString(t)          && _f.toString().isEmpty())) {
        if (!isNullable && colDefault.isEmpty()) str |= cRecord::ES_DEFECTIVE;
        return QVariant();
    }
    bool ok = false;
    qlonglong r = NULL_ID;
    if      (variantIsFloat(_f))   r = (qlonglong)(_f.toDouble(&ok) + 0.5);
    else if (variantIsString(_f))  r = parseTimeInterval(_f.toString(), &ok);
    else if (variantIsInteger(_f)) r = _f.toLongLong(&ok);
    if (!ok) str |= cRecord::ES_DEFECTIVE;
    return r == NULL_ID || !ok ? QVariant() : QVariant(r);
}
QString   cColStaticDescrInterval::toName(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isValid()) return toSql(_f).toString();
    return _sNul;
}
qlonglong cColStaticDescrInterval::toId(const QVariant& _f) const
{
    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f.toLongLong();
}

CDDUPDEF(cColStaticDescrInterval)
/* ******************************************************************************************************* */

cColStaticDescrList::cColStaticDescrList()
    : QList<cColStaticDescr *>()
{
    ;
}

cColStaticDescrList::~cColStaticDescrList()
{
    ConstIterator i = constBegin();
    ConstIterator n = constEnd();
    for (; i != n; ++i) delete *i;
}

cColStaticDescrList& cColStaticDescrList::operator<<(cColStaticDescr * __o)
{
    *(list *)this << __o;
    if (size() != __o->pos) EXCEPTION(EDBDATA,0,QString("pos = %1, and list síze = %2, is not equal.").arg(__o->pos).arg(size()));
    return *this;
}

int cColStaticDescrList::toIndex(const QString& __n, bool __ex) const
{
    if (__n.isEmpty()) {
        static QString msg = "Empty field name";
        if (__ex) EXCEPTION(EDATA,-1, msg);
        DERR() << msg << endl;
        return NULL_IX;
    }
    const_iterator  i;
    for (i = constBegin(); i != constEnd(); i++) if (**i == __n) return (*i)->pos -1;
    if (__ex) EXCEPTION(ENOFIELD, -1, __n);
    return NULL_IX;
}


cColStaticDescr& cColStaticDescrList::operator[](const QString& __n)
{
    int i = toIndex(__n);
    if (i < 0 || i > size() -1) EXCEPTION(EPROGFAIL, i, __n);
    return (*this)[i];
}

const cColStaticDescr& cColStaticDescrList::operator[](const QString& __n) const
{
    int i = toIndex(__n);
    if (i < 0 || i > size() -1) EXCEPTION(EPROGFAIL, i, __n);
    return (*this)[i];
}

cColStaticDescr& cColStaticDescrList::operator[](int i)
{
    if (i < 0 || i >= size()) EXCEPTION(ENOINDEX, i);
    return *(*(list *)this)[i];
}

const cColStaticDescr& cColStaticDescrList::operator[](int i) const
{
    if (i < 0 || i >= size()) EXCEPTION(ENOINDEX, i);
    return *(*(const list *)this)[i];
}

/* ******************************************************************************************************* */

QMap<qlonglong, cRecStaticDescr *>   cRecStaticDescr::_recDescrMap;
QMutex                               cRecStaticDescr::_mapMutex;

cRecStaticDescr::cRecStaticDescr(const QString &__t, const QString &__s)
    : _schemaName()
    , _tableName()
    , _viewName()
    , _tableRecord()
    , _columnDescrs()
    , _primaryKeyMask()
    , _uniqueMasks()
    , _autoIncrement()
    , _parents()
{
    _tableType      = UNKNOWN_TABLE;
    _columnsNum     = 0;
    _tableOId       = _schemaOId = NULL_ID;
    _idIndex        = _nameIndex = _descrIndex = _deletedIndex = NULL_IX;
    _isUpdatable    = false;
    _set(__t,__s);
    addMap();
    // Csak most tudjuk rendesen előszedni, esetleg megkreálni az id->name konverziós függvényeket
    int i, n = cols();
    QSqlQuery *pq = NULL;
    for (i = 0; i < n; ++i) {
        const cColStaticDescr& cd = colDescr(i);
        if (cd.fKeyType != cColStaticDescr::FT_NONE && cd.fnToName.isEmpty()) {
            if (pq == NULL) pq = newQuery();
            const_cast<cColStaticDescr *>(&cd)->fnToName = checkId2Name(*pq, cd.fKeyTable, cd.fKeySchema);
        }
    }
    pDelete(pq);
}

cRecStaticDescr::~cRecStaticDescr()
{
    ;
}

bool cRecStaticDescr::addMap()
{
    _DBGFN() << _tableName << endl;
    _mapMutex.lock();
    if (_recDescrMap.contains(_tableOId)) {
        _mapMutex.unlock();
        DWAR() << "Remap static descriptor : " << toString() << endl;
        return false;
    }
    _recDescrMap[_tableOId] = this;
    _mapMutex.unlock();
    _DBGFNL() << _sOk << endl;
    return true;
}

int cRecStaticDescr::_setReCallCnt = 0;

void cRecStaticDescr::_set(const QString& __t, const QString& __s)
{
    _DBGFN() << " @(" << __t << _sCommaSp << __s << _sABraE << endl << dec; // Valahol valaki hex-re állítja :(
    if (_setReCallCnt) DWAR() << "******** RECURSION #" << _setReCallCnt << " ********" << endl;
    ++_setReCallCnt;
    // Set table and schema name
    _schemaName     = __s.isEmpty() ? _sPublic : __s;
    _tableName      = __t;

    QString baseName = __t.endsWith('s') ? __t.mid(0, __t.size() -1) : _sNul;

    QSqlQuery   *pq  = newQuery();
    QSqlQuery   *pq2 = newQuery();
    QString sql;
    // *********************** set scheam and Table  OID
    _schemaOId = ::schemaoid(*pq, __s);
    _tableOId  = ::tableoid(*pq, __t, _schemaOId);
    PDEB(INFO) << "Table " << fullTableName() << " tableoid : " << _tableOId << endl;

    // *********************** get Parents table(s) **********************
    sql = "SELECT inhparent FROM pg_inherits WHERE inhrelid = ? ORDER BY inhseqno";
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    pq->bindValue(0, _tableOId);
    if (!pq->exec()) SQLQUERYERR(*pq);
    const cRecStaticDescr *pp;
    if (pq->first()) do {
        long poid = pq->value(0).toInt();
        pp = get(poid);
        _parents.append(pp);
    } while(pq->next());

    // *********************** get table record
    sql = "SELECT * FROM information_schema.tables WHERE table_schema = ? AND table_name = ?";
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    pq->bindValue(0, _schemaName);
    pq->bindValue(1, _tableName);
    if (!pq->exec()) SQLQUERYERR(*pq);
    if (!pq->first()) EXCEPTION(EDBDATA, 0, QString("Table %1.%2 not found.").arg(_schemaName, _tableName));
    _tableRecord = pq->record();
    if (pq->next()) EXCEPTION(EDBDATA, 0, QString("Table : %1,%2.").arg(_schemaName, _tableName));

    QString n = _tableRecord.value("table_type").toString();
    if      (n == "BASE TABLE")
        _tableType = BASE_TABLE;
    else if (n == "VIEW") {
        _tableType = VIEW_TABLE;
        _viewName = _tableName;
        // Ha egy link tábláról van szó, akkor annak itt a view tábláját kell megadni, és ebben az estenben
        // a tábla név az a view név kiegészítve a "_table" uótaggal.
        QString tn = _viewName + "_table";
        qlonglong toid = ::tableoid(*pq, tn, _schemaOId, false);
        if (NULL_ID != toid) {
            PDEB(INFO) << "Table " << _viewName << _sSlash << _tableName << " table type : LINK_TABLE." << endl;
            _tableName = tn;
            _tableOId  = toid;
            _tableType = LINK_TABLE;
        }
    }
    else EXCEPTION(EDBDATA, 0, QString("Invalid table type %1.%2 : %3").arg(_schemaName, _tableName, n));
    PDEB(INFO) << fullTableName() << " table is " << n << endl;

    // ************************ get columns records
    sql = "SELECT * FROM information_schema.columns WHERE table_schema = ? AND table_name = ? ORDER BY ordinal_position";
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    pq->bindValue(0, _schemaName);
    pq->bindValue(1, _tableName);
    if (!pq->exec()) SQLQUERYERR(*pq);
    if (!pq->first()) EXCEPTION(EDBDATA, 0, QString("Table columns %1,%2 not found.").arg(_schemaName).arg(_tableName));
    _columnsNum = pq->size();
    if (_columnsNum < 1) EXCEPTION(EPROGFAIL, _columnsNum, "Invalid size.");
    _autoIncrement.resize(_columnsNum);
    _primaryKeyMask.resize(_columnsNum);
    PDEB(VVERBOSE) << VDEB(_columnsNum) << endl;
    int i = 0;
    do {
        ++i;    // Vigyázz, fizikus index! (1,2,...)
        cColStaticDescr columnDescr;
        columnDescr.colName() = pq->record().value("column_name").toString();
        columnDescr.pos       = i;
        columnDescr.ordPos    = pq->record().value("ordinal_position").toInt();
        columnDescr.colDefault= pq->record().value("column_default").toString();
        columnDescr.colType   = pq->record().value("data_type").toString();
        columnDescr.udtName   = pq->record().value("udt_name").toString();
        columnDescr.isNullable= pq->record().value("is_nullable").toString() == QString("YES");
        columnDescr.isUpdatable=pq->record().value("is_updatable").toString() == QString("YES");
        _isUpdatable = _isUpdatable || columnDescr.isUpdatable;
        PDEB(INFO) << fullTableName() << " field #" << i << _sSlash << columnDescr.ordPos << " : " << columnDescr.toString() << endl;
        //Ez egy auto increment mező ?
        _autoIncrement.setBit(i -1, columnDescr.colDefault.indexOf("nextval('") == 0);
        // Megnézzük enum-e
        if (columnDescr.colType ==  _sUSER_DEFINED || columnDescr.colType ==  _sARRAY) {
            if (columnDescr.colType ==  _sARRAY && columnDescr.udtName.startsWith(_sUnderscore)) {
                // Az ARRAY-nál a típus névhez, hozzá szokott bigyeszteni egy '_'-karakter (nem mindíg)
                columnDescr.udtName = columnDescr.udtName.mid(1);   // Ha a sját típust keressük, az '_'-t törölni kell.
            }
            sql = QString(
                        "SELECT pg_enum.enumlabel FROM  pg_catalog.pg_enum JOIN pg_catalog.pg_type ON pg_type.oid = pg_enum.enumtypid "
                        "WHERE pg_type.typname = '%1' ORDER BY pg_enum.oid").arg(columnDescr.udtName);
            if (!pq2->exec(sql)) SQLPREPERR(*pq2, sql);
            if (pq2->first()) do {
                columnDescr.enumVals << pq2->value(0).toString();
            } while(pq2->next());
            PDEB(VVERBOSE) << columnDescr.colName()
                           << (columnDescr.colType ==  _sARRAY ? " is set" : " is enum : ")
                           << columnDescr.udtName << _sCBraB << columnDescr.enumVals.join(_sComma) << _sCBraE << endl;
        }
        if (columnDescr.colName() == _sDeleted) {
            if (columnDescr.colType == "boolean") {
                if (_deletedIndex >= 0) EXCEPTION(EPROGFAIL);
                _deletedIndex = i -1;
                PDEB(VVERBOSE) << "deleted field is found, index : " << _deletedIndex << endl;
            }
            else {
                PDEB(VVERBOSE) << "deleted field is found, but type is not boolean, index : " << (i -1) << " type : " << columnDescr.colType << endl;
            }
        }
        // Mező típus
        columnDescr.typeDetect();
        // A név és a descr : Kötött nevű szöveges mezők
        // Vagy a sorszámuk adott a végződéssel
        if (columnDescr.eColType == cColStaticDescr::FT_TEXT) {
            #define NAME_INDEX  1
            #define DESCR_INDEX 2
            static QString nameSufix  = "_name";
            static QString descrSufix = "_descr";
            if (!baseName.isEmpty()) {
                if (_nameIndex  < 0 && 0 == columnDescr.colName().compare(baseName + nameSufix))  _nameIndex  = i -1;
                if (_descrIndex < 0 && 0 == columnDescr.colName().compare(baseName + descrSufix)) _descrIndex = i -1;
            }
            if (_nameIndex   < 0 && i == (NAME_INDEX  +1) && columnDescr.colName().endsWith(nameSufix))  _nameIndex  = NAME_INDEX;
            if (_descrIndex  < 0 && i == (DESCR_INDEX +1) && columnDescr.colName().endsWith(descrSufix)) _descrIndex = DESCR_INDEX;
        }
        // A távoli kulcs mezők detektálása:
        if (columnDescr.eColType == cColStaticDescr::FT_INTEGER) {  // Feltépelezzük, hogy távoli kulcs csak egész szám (ID) lehet
            sql = QString(
                        "SELECT f.table_schema, f.table_name, f.column_name, t.unusual_fkeys_type"
                        "    FROM information_schema.referential_constraints r"
                        "    JOIN information_schema.key_column_usage        l"
                        "        USING(constraint_name, constraint_schema)"
                        "    JOIN information_schema.key_column_usage        f"
                        "        ON r.unique_constraint_name = f.constraint_name AND r.unique_constraint_schema = f.constraint_schema"
                        "    LEFT JOIN fkey_types t"
                        "            ON l.table_schema = t.table_schema AND l.table_name = t.table_name AND l.column_name = t.column_name"
                        "    WHERE l.table_schema = '%1' AND l.table_name = '%2' AND l.column_name = '%3'"
                        ).arg(_schemaName, _tableName, columnDescr.colName());
            if (!pq2->exec(sql)) SQLPREPERR(*pq2, sql);
            if (pq2->first()) { // Ha ő egy távoli kulcs, hova:
                columnDescr.fKeySchema= pq2->value(0).toString();
                columnDescr.fKeyTable = pq2->value(1).toString();
                columnDescr.fKeyField = pq2->value(2).toString();
                QString t = pq2->value(3).toString();
                if (t.isEmpty()) {   // Nincs megadva típus a távoli kulcsra (nincs fley_types rekord)
                    if (_tableName == columnDescr.fKeyTable && _schemaName == columnDescr.fKeySchema)
                        columnDescr.fKeyType = cColStaticDescr::FT_SELF;
                    else
                        columnDescr.fKeyType = cColStaticDescr::FT_PROPERTY;
                    // FT_OWNER típus esetén kötelező definiáni a típust egy fkey_types vagy unusual_fkeys rekorddal !
                }
                else if (!t.compare("property", Qt::CaseInsensitive)) {
                    columnDescr.fKeyType = cColStaticDescr::FT_PROPERTY;
                }
                else if (!t.compare("owner",    Qt::CaseInsensitive)) {
                    columnDescr.fKeyType = cColStaticDescr::FT_OWNER;
                    if (_tableType != BASE_TABLE) EXCEPTION(EDBDATA, _tableType, "Table type conflict.");
                    _tableType = CHILD_TABLE;
                }
                else if (!t.compare("self",     Qt::CaseInsensitive)) {
                    if (_tableName != columnDescr.fKeyTable || _schemaName != columnDescr.fKeySchema) EXCEPTION(EDBDATA);
                    columnDescr.fKeyType = cColStaticDescr::FT_SELF;
                }
                else EXCEPTION(EDATA, -1, t);
                columnDescr.fKeyTables << columnDescr.fKeyTable;
                // columnDescr.fnToName = pq2->value(4).toString();
            }
            // Az ismert, de az adatbázisban kerülő uton (öröklés miatt) kezelt távoli kulcsok
            else  {
                sql = QString(
                            "SELECT f_table_schema, f_table_name, f_column_name, unusual_fkeys_type, f_inherited_tables"
                            "    FROM unusual_fkeys"
                            "    WHERE table_schema = '%1' AND table_name = '%2' AND column_name = '%3'"
                            ).arg(_schemaName, _tableName, columnDescr.colName());
                if (!pq2->exec(sql)) SQLPREPERR(*pq2, sql);
                if (pq2->first()) { // Ha ő egy nem szokványos távoli kulcs
                    columnDescr.fKeySchema = pq2->value(0).toString();
                    columnDescr.fKeyTable  = pq2->value(1).toString();
                    columnDescr.fKeyField  = pq2->value(2).toString();
                    columnDescr.fKeyTables = stringArrayFromSql(pq2->value(4));
                    QString t = pq2->value(3).toString();
                    if      (!t.compare("property", Qt::CaseInsensitive)) {
                        columnDescr.fKeyType = cColStaticDescr::FT_PROPERTY;
                    }
                    else if (!t.compare("owner",    Qt::CaseInsensitive)) {
                        if (_tableType != BASE_TABLE) EXCEPTION(EDBDATA, _tableType, "Table type conflict.");
                        _tableType = CHILD_TABLE;
                        columnDescr.fKeyType = cColStaticDescr::FT_OWNER;
                    }
                    else if (!t.compare("self",     Qt::CaseInsensitive)) {
                        columnDescr.fKeyType = cColStaticDescr::FT_SELF;
                    }
                    else EXCEPTION(EDATA, -1, t);
                }
            }
            if (columnDescr.fKeyType != cColStaticDescr::FT_NONE) {
                // Az id -> name konverziós függvény : (csak ha már megvan a cél tábla descriptora)
                columnDescr.fnToName = checkId2Name(*pq2, columnDescr.fKeySchema, columnDescr.fnToName, false);
            }
        }
        // A távoli kulcs mezők detektálása VÉGE
        // A konveziós függvények miatt a megfelelő típuso mező leíró objektumot kell a konténerbe rakni.
        switch (columnDescr.eColType) {
        case cColStaticDescr::FT_DATE:          _columnDescrs << new cColStaticDescrDate(columnDescr);      break;
        case cColStaticDescr::FT_DATE_TIME:     _columnDescrs << new cColStaticDescrDateTime(columnDescr);  break;
        case cColStaticDescr::FT_TIME:          _columnDescrs << new cColStaticDescrTime(columnDescr);      break;
        case cColStaticDescr::FT_INTERVAL:      _columnDescrs << new cColStaticDescrInterval(columnDescr);  break;

        case cColStaticDescr::FT_BOOLEAN:       _columnDescrs << new cColStaticDescrBool(columnDescr);      break;
        case cColStaticDescr::FT_POLYGON:       _columnDescrs << new cColStaticDescrPolygon(columnDescr);   break;

        case cColStaticDescr::FT_MAC:
        case cColStaticDescr::FT_INET:
        case cColStaticDescr::FT_CIDR:          _columnDescrs << new cColStaticDescrAddr(columnDescr);      break;

        case cColStaticDescr::FT_ENUM:          _columnDescrs << new cColStaticDescrEnum(columnDescr);      break;
        case cColStaticDescr::FT_SET:           _columnDescrs << new cColStaticDescrSet(columnDescr);       break;

        default:
            if (columnDescr.eColType & cColStaticDescr::FT_ARRAY) _columnDescrs << new cColStaticDescrArray(columnDescr);
            else                                                  _columnDescrs << columnDescr;
            break;
        }
        cColStaticDescr *pp = ((cColStaticDescrList::list)_columnDescrs)[i -1];
        PDEB(VERBOSE) << "Field " << pp->colName() << " type is " << typeid(*pp).name() << endl;
     } while(pq->next());
    if (_columnsNum != i) EXCEPTION(EPROGFAIL, -1, "Nem egyértelmű mező szám");

    // ************************ get key_column_usage records
    /* sql = "SELECT * FROM information_schema.key_column_usage WHERE table_schema = ? AND table_name = ?"; */
    sql = "SELECT * FROM information_schema.key_column_usage "
                   "JOIN information_schema.table_constraints "
                     "USING(constraint_name) "
              "WHERE information_schema.key_column_usage.table_schema = ? "
                "AND information_schema.key_column_usage.table_name = ?";
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    pq->bindValue(0, _schemaName);
    pq->bindValue(1, _tableName);
    if (!pq->exec()) SQLQUERYERR(*pq);
    // PDEB(VVERBOSE) << "Read keys in " << _fullTableName() << " table..." << endl;
    if (pq->first()) {
        QMap<QString, int>  map;
        do {
            QString constraintName = pq->record().value("constraint_name").toString();
            QString columnName     = pq->record().value("column_name").toString();
            QString constraintType = pq->record().value("constraint_type").toString();
            i = toIndex(columnName, false);
            if (i < 0) EXCEPTION(EDBDATA,0, QString("Invalid column name : %1").arg(fullColumnName(columnName)));
            if     (constraintType == "PRIMARY KEY") {
                //PDEB(VVERBOSE) << "Set _primaryKeyMask bit, index = " << i << endl;
                _primaryKeyMask.setBit(i);
                if (columnName.endsWith(QString("_id"))) _idIndex = i;
            }
            else if(constraintType == "UNIQUE") {
                QMap<QString, int>::iterator    it = map.find(constraintName);
                int j;
                if (it == map.end()) {
                    j = _uniqueMasks.size();
                    //PDEB(VVERBOSE) << "Insert #" << j << " bit vector to _uniqueMasks ..." << endl;
                    map.insert(constraintName, j);
                    _uniqueMasks << QBitArray(_columnsNum);
                }
                else j = it.value();
                //PDEB(VVERBOSE) << "Set _uniqueMasks[" << j << "] bit #" << i << " to true..." << endl;
                _uniqueMasks[j].setBit(i);
                // if (_nameIndex < 0 && columnName.endsWith(QString("_name"))) _nameIndex = i;
            }
        } while(pq->next());
    }
    // Ha találtunk ID-t, akkor az csak egyedüli egyedi kulcs lehet!
    if (_primaryKeyMask.count(true) != 1) _idIndex = NULL_IX;
    //PDEB(VERBOSE) << VDEB(_idIndex) << " ; " << VDEB(_nameIndex) << endl;
    //
    if (_tableType == BASE_TABLE && _nameIndex < 0              // Típus még nem derült ki, és nincs neve
     && colDescr(1).fKeyType == cColStaticDescr::FT_PROPERTY    // A második mező egy távoli kulcs
     && colDescr(2).fKeyType == cColStaticDescr::FT_PROPERTY)   // és a harmadik mező is.
        _tableType = SWITCH_TABLE;
    delete pq;
    delete pq2;
    // ************************ init O.K
    _setReCallCnt--;
    DBGFNL();
}

int cRecStaticDescr::toIndex(const QString& __n, bool __ex) const
{
    // _DBGFN() << " @(" << __n << "): " << fullTableName() << endl;
    // Esetleges schema vagy tábla név leválasztása
    QStringList m = __n.split(QChar('.'));
    // csak három elem lehet: schema.table.field
    if (m.size() > 3) {
        if (__ex) EXCEPTION(EDATA, -1, "Invalid field name " + __n);
        DERR() << "Invalid index : -1 / To many field in name"  << endl;
        return NULL_IX;
    }
    // Ha megvan adva schema név
    if (m.size() == 3) {
        QString sn = _schemaName;
        if (sn == unDQuoted(m.first())) {
            m.pop_front();  // Schema név ok, eldobjuk
        }
        else{
            if (__ex) EXCEPTION(EDATA, -1, "Invalid frield name (other schema name) " + __n);
            DERR() << "Invalid index : -1 / mismatch shema : my schema is " << schemaName() << endl;
            return NULL_IX; // Másik schemarol van szó
        }
    }
    // Ha megvan adva tábla név
    if (m.size() == 2) {
        if (_tableName == unDQuoted(m.first())
         || (_viewName.size() > 0 &&_viewName  == unDQuoted(m.first()))) {
            m.pop_front();  // Tábla név ok, eldobjuk
        }
        else {
            if (__ex) EXCEPTION(EDATA, -1, "Invalid frield name (other table name) " + __n);
            DERR() << "Invalid index : -1 / mismatch table : my table is " << tableName() << endl;
            return NULL_IX; // Másik tábláról van szó
        }
    }
    // maradt a mező név, keressük a listűban
    QString name = unDQuoted(m.first());
    return _columnDescrs.toIndex(name, __ex);
}

const cRecStaticDescr *cRecStaticDescr::get(qlonglong _oid, bool find_only)
{
    cRecStaticDescr *p = NULL;
    QMap<qlonglong, cRecStaticDescr *>::iterator    i;
    _mapMutex.lock();
    if ((i = _recDescrMap.find(_oid)) != _recDescrMap.end()) {
        p = *i;
        _mapMutex.unlock();
        PDEB(VERBOSE) << "Találat. " << endl;
        return *i;
    }
    QSqlQuery *pq = newQuery();
    QStringPair tsn = tableoid2name(*pq, _oid);
    delete pq;
    _mapMutex.unlock();
    PDEB(VVERBOSE) << "Nincs találat." << endl;
    // Ha nem találjuk, létre kell hozni ?
    if (find_only) return NULL;
    if (_setReCallCnt >= 10) EXCEPTION(EPROGFAIL);
    PDEB(VERBOSE) << "Új objektum allokálása, és inicializálása." << endl;
    p = new cRecStaticDescr(tsn.first, tsn.second);
    return p;
}

const cRecStaticDescr *cRecStaticDescr::get(const QString& _t, const QString& _s, bool find_only)
{
    _DBGFN() << _t << _sCommaSp << _s << endl;
    QSqlQuery *pq = newQuery();
    qlonglong tableOId = ::tableoid(*pq, _t, schemaoid(*pq, _s), !find_only);
    delete pq;
    return tableOId == NULL_ID ? NULL : get(tableOId, find_only);
}

QString cRecStaticDescr::checkId2Name(QSqlQuery& q, const QString& _tn, const QString& _sn, bool __ex)
{
    const cRecStaticDescr *pDescr = get(_tn, _sn, !__ex);
    if (pDescr == NULL) {
        if (__ex) EXCEPTION(EDATA);
        return QString();
    }
    const QString& sIdName     = pDescr->idName();
    const QString& sSchemaName = pDescr->schemaName();
    // Az id -> name konverziós függvény :
    QString sFnToName = sIdName + "2name"; // Az alapértelmezett név
    // Csak a nevet ellenőrizzük, ilyen neve ne legyen més függvénynek !!
    QString sql = QString("SELECT COUNT(*) FROM  information_schema.routines WHERE routine_schema = '%1' AND routine_name = '%2'")
            .arg(sSchemaName).arg(sFnToName);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (!q.first() || q.value(0).toInt() == 1) return sFnToName;    // Definiálva van a keresett függvény, OK
    const QString& sNameName   = pDescr->nameName();
    const QString& sTableName  = pDescr->tableName();
    DWAR() << "Function : " << sFnToName << " not found, create ..." << endl;
    sql = QString(
         "CREATE OR REPLACE FUNCTION %1(integer) RETURNS varchar(32) AS $$"
        " DECLARE"
            " name varchar(32);"
        " BEGIN"
            " SELECT %4 INTO name FROM %2 WHERE %3 = $1;"
            " IF NOT FOUND THEN"
                " PERFORM error('IdNotFound', $1, '%3', '%1', '%2');"
            " END IF;"
            " RETURN name;"
        " END"
        " $$ LANGUAGE plpgsql"
            ).arg(sFnToName).arg(sTableName).arg(sIdName).arg(sNameName);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    PDEB(INFO) << "Created procedure : " << quotedString(sql) << endl;
    return sFnToName;
}

bool cRecStaticDescr::operator<(const cRecStaticDescr& __o) const
{
    if (*this == __o) return false;
    QVector<const cRecStaticDescr *>::ConstIterator i, n = __o._parents.constEnd();
    for (i = __o._parents.constBegin(); i != n; ++i) {
        if (*this == **i) return true;
        if (*this  < **i) return true;
    }
    return false;
}

qlonglong cRecStaticDescr::getIdByName(QSqlQuery& __q, const QString& __n, bool ex) const
{
    QString sql = QString("SELECT %1 FROM %2 WHERE %3 = '%4'")
                  .arg(dQuoted(idName()), fullTableNameQ(), dQuoted(nameName()), __n);
    // PDEB(VERBOSE) << __PRETTY_FUNCTION__ << " SQL : " << sql << endl;
    if (!__q.exec(sql)) {
        if (ex == false && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
        else                                                                    SQLQUERYERR(__q)
        return NULL_ID;
    }
    if (!__q.first()) {
        if (ex) EXCEPTION(EFOUND,0,__n);
        return NULL_ID;
    }
    QVariant id = __q.value(0);
    if (id.isNull()) {
        if (ex) EXCEPTION(EDATA, 1);
        return NULL_ID;
    }
    bool    ok;
    qlonglong i =  id.toLongLong(&ok);
    if (!ok) {
        if (ex) EXCEPTION(EDATA, 2, id.toString());
        return NULL_ID;
    }
    if (__q.next()) {
        if (ex) EXCEPTION(AMBIGUOUS, 2, id.toString());
        return NULL_ID;
    }
    return i;
}

QString cRecStaticDescr::getNameById(QSqlQuery& __q, qlonglong __id, bool ex) const
{
    QString sql = QString("SELECT %1 FROM %2 WHERE %3 = %4")
            .arg(dQuoted(nameName()), fullTableNameQ(), dQuoted(idName())). arg(__id);
    if (!__q.exec(sql)) {
        if (ex == false && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
        else                                                                    SQLQUERYERR(__q)
        return QString();
    }
    if (!__q.first()) {
        if (ex) EXCEPTION(EFOUND, __id);
        return QString();
    }
    QVariant name = __q.value(0);
    if (name.isNull()) {
        if (ex) EXCEPTION(EDATA, 1);
        return QString();
    }
    if (__q.next()) {
        if (ex) EXCEPTION(AMBIGUOUS, 2, name.toString());
        return QString();
    }
    return name.toString();
}

QString cRecStaticDescr::toString() const
{
    QString s = "CREATE TABLE " + fullTableNameQ() + _sSpace + _sABraB + '\n';
    cColStaticDescrList::ConstIterator i, n = columnDescrs().constEnd();
    for (i = columnDescrs().constBegin(); i != n; ++i) {
        const cColStaticDescr& fd = **i;
        s += _sSpace + fd.toString();
        if (_primaryKeyMask.count(true) == 1 && _primaryKeyMask[fd.pos -1]) {
            s += _sSpace + "PRIMARY KEY";
        }
        else {
            foreach (const QBitArray& um,  _uniqueMasks) {
                if (um.count(true) == 1 && um[fd.pos -1]) {
                    s += _sSpace + "UNIQUE";
                    break;
                }
            }
        }
        s += _sComma;
        if (fd.fKeyType != cColStaticDescr::FT_NONE) {
            s += " -- Foreign key to " + fd.fKeySchema + _sPoint + fd.fKeyTable + _sPoint + fd.fKeyField + _sSlash;
            switch (fd.fKeyType) {
            case cColStaticDescr::FT_PROPERTY:  s += "Property";    break;
            case cColStaticDescr::FT_OWNER:     s += "Owner";       break;
            case cColStaticDescr::FT_SELF:      s += "Self";        break;
            default:                            s += "?";           break;
            }
        }
        s += '\n';
    }
    if (_primaryKeyMask.count(true) > 1) {
        s += _sSpace + "PRIMARY KEY" + _sSpace + _sABraB;
        for (int i = 0; i < _columnsNum; i++) if (_primaryKeyMask[i]) {
            s += _sSpace + dQuoted(columnDescrs()[i].colName()) + _sComma;
        }
        s += _sABraE + _sSColon;
    }
    for (int i = 0; i < _uniqueMasks.size(); i++) if (_uniqueMasks[i].count(true) > 1) {
        s += _sSpace + "UNIQUE" + _sSpace + _sABraB;
        for (int j = 0; j < _uniqueMasks[i].size(); j++) if (_uniqueMasks[i][j]) {
            s += _sSpace + dQuoted(columnDescrs()[j].colName()) + _sComma;
        }
        s += _sABraE + _sSColon;
    }
    s += _sSpace   + "_idIndex = "    + QString::number(_idIndex);
    s += _sCommaSp + "_nameIndex = "  + QString::number(_nameIndex);
    s += _sCommaSp + "_descrIndex = " + QString::number(_descrIndex);
    // s.chop(1);  // utolsó felesleges vesszőt lecsípjük
    s += _sABraE;
    if (_parents.size()) {
        s += _sSpace + "INHERITS(";
        QVector<const cRecStaticDescr *>::ConstIterator ii, nn = parent().constEnd();
        for (ii = parent().constBegin(); ii != nn; ++ii) {
            s += _sSpace + (*ii)->fullTableNameQ() + _sComma;
        }
        s.chop(1);
        s += _sABraE;
    }
    s += _sSColon +  " -- Table type : ";
    switch (_tableType) {
    case BASE_TABLE:    s += "Base";    break;
    case VIEW_TABLE:    s += "View";    break;
    case SWITCH_TABLE:  s += "Switch";  break;
    case LINK_TABLE:    s += "Link";    break;
    case CHILD_TABLE:   s += "Child";   break;
    default:            s += "?";       break;
    }
    return s + _sSpace;
}

int cRecStaticDescr::ixToOwner(bool __ex) const
{
    int fix;
    for (fix = 0; fix < _columnsNum; ++fix) {          // Az ownerre mutató mezőt keressük
        const cColStaticDescr& cd = columnDescrs()[fix];
        cColStaticDescr::eFKeyType t = cd.fKeyType;
        if (t == cColStaticDescr::FT_OWNER) break;
    }
    if (fix >= _columnsNum) {
        if (__ex) EXCEPTION(EDATA);
        return NULL_IX;
    }
    return fix;
}

/* ******************************************************************************************************* */
const QVariant  cRecord::_vNul;

cRecord::cRecord() : QObject(), _fields(), _likeMask()
{
   // _DBGFN() << _sSpace << VDEBPTR(this) << endl;
    _deletedBehavior = NO_EFFECT ;
    _stat = ES_NULL;
}

cRecord::cRecord(const cRecord&) : QObject(), _fields(), _likeMask()
{
    EXCEPTION(ENOTSUPP);
    //__o.descr();  // Inaktív kód, csak hogy ne ugasson a fordító.
}

cRecord::~cRecord()
{
    // _DBGFN() << _sSpace << VDEBPTR(this) n0t0kaptus
}

cRecord& cRecord::_clear()
{
    _fields.clear();
    _stat = ES_NULL;
    _likeMask.clear();
    return *this;
}

cRecord& cRecord::_clear(int __ix)
{
    if (isNull()) return *this;
    if (_fields.size() <= __ix) EXCEPTION(EPROGFAIL, __ix);
    _fields[__ix].clear();
    if  (isEmpty()) _stat  = ES_NULL;
    else             _stat |= ES_MODIFY;
    return *this;
}

bool cRecord::isEmpty()
{
    if (_fields.size() == 0) {
        _stat = ES_NULL;
        return true;
    }
    int i, n = _fields.size();
    // if (n != cols()) EXCEPTION(EPROGFAIL, n); // Ez egy konstruktorban tilos (virtuális)
    for (i = 0; i < n; i++) {
        if (! _fields[i].isNull()) {
            if (_stat == ES_EMPTY || _stat == ES_NULL) _stat = ES_MODIFY;
            return false;
        }
    }
    _stat = ES_EMPTY;
    return true;
}

void cRecord::clearToEnd()
{
    // _DBGFN() << " *** EMPTY ***" << endl;
}

void cRecord::toEnd()
{
    // _DBGFN() << " *** EMPTY ***" << endl;
}

bool cRecord::toEnd(int i)
{
    i = i;
    // _DBGFN() << " @(" << i << ") *** EMPTY ***" << endl;
    return false;
}

cRecord& cRecord::_set(const cRecStaticDescr& __d)
{
    _fields.clear();
    int i, n = __d.cols();
    for (i = 0; i < n; i++) _fields << QVariant();
    _stat = ES_EMPTY;
    return *this;
}

cRecord& cRecord::set()
{
    clear();
    return _set(descr());
}

cRecord& cRecord::clear()
{
    _clear();
    clearToEnd();
    cleared();
    return *this;
}

cRecord& cRecord::clear(int __ix, bool __ex)
{
    if (isIndex(__ix) || __ex) {
        _clear(__ix);
        toEnd(__ix);
        fieldModified(__ix);
    }
    return *this;
}


cRecord& cRecord::_set(const QSqlRecord& __r, const cRecStaticDescr& __d, int* _fromp, int __size)
{
//    _DBGFN() << " @(," << (_fromp == NULL ? QString("NULL") : QString("*(%1)").arg(*_fromp)) << "," << __size << ")" << endl;
    int i;                          // Forrás index (__r)
    int n = __r.count();            // Mezők száma a forrás rekordban
    if (__size > 0) {               // Ha csak egy szelete kell a forrásnak
        n = __size;
    }
    int first = 0;                  // Első elem a forrásból
    if (_fromp != NULL) {          // Ha csak egy szelet kell a forrásból
        first = *_fromp;
        if (__size > 0) {           // A következő szelet első eleme
            *_fromp = n += first;
        }
        else {
            *_fromp = n;
        }
        // PDEB(VVERBOSE) << "*_fromp = " << *_fromp << " Selet : " << first << " - " << n << endl;
    }
    if (n > __r.count()) {          // A forrás végénél nem kéne tovább menni
        DWAR() << "Rekord túlolvasási kisérlet, rekord méret : " << __r.count() << " ciklus határ : " << n << endl;
        n = __r.count();
    }
    int ix;                 // Cél index    (_record)r
    int m = __d.cols();     // Mezők száma a cél rekordban
    const int invalid = -1;
    QVector<int>    ixv(m, invalid); // Kereszt index: cél index -> forrás index
    for (i = first; i < n; i++) {       // csinálunk egy kereszt index táblát
        int ix = __d.toIndex(__r.fieldName(i), false);
        if (ix >= 0) ixv[ix] = i;
    }
    _set(__d);  // isEmpty() == true
    if (ixv.count(invalid) == ixv.size()) {
        DWAR() << "Nincs egy keresett mező sem a forrás rekordban." << endl;
        return *this;   // Nincs egy mező sem, üres rekordal térünk vissza
    }
    _stat = ES_EXIST;
    const cColStaticDescrList&  fl = __d.columnDescrs();
    int cnt = 0;
    for (ix = 0; ix < m; ix++) {            // Végigrohanunk a cél rekord mezőin
        const cColStaticDescr& fd = fl[ix]; // A mező leíró
        i = ixv[ix];                        // forrás idex, a keresz táblából
        QVariant f;
        if (i < 0) {
            f.clear();
        }
        else {
            cnt++;
            f = fd.fromSql(__r.value(i));
        }
        if (!f.isNull()) {
            _set(ix, f);
        }
        else {
            if (!fd.isNullable) _stat |= ES_INCOMPLETE;
        }
   }
    if (cnt == m) {
        if (_stat & ES_INCOMPLETE) _stat  = ES_DEFECTIVE;
        else                       _stat |= ES_COMPLETE;
    }
    if (isIdent()) _stat |= ES_IDENTIF;
    else           _stat |= ES_UNIDENT;
    return *this;
}


bool cRecord::_copy(const cRecord &__o, const cRecStaticDescr &d)
{
    bool m = false;
    int i;
    if (__o.isEmpty_()) return false;
    if (isNull()) _set(d);
    int n = __o.cols();    // Mezők száma a forrás rekordban
    for (i = 0; i < n; i++) {
        QString fn = __o.columnName(i);     // Mező név
        int ii = d.toIndex(fn, false);      // A cél ugyanojan nevű mezőjének az indexe
        if (!d.isIndex((ii))) continue;     // A cél rekordban nincs ilyen mező
        if (__o.isNull(i))    continue;     // vagy null a forrás mező
        _set(ii, __o.get(i));
        m = true;
    }
    return m;
}

bool cRecord::isIdent()
{
    if (isIdent(primaryKey())) return true;
    foreach (const QBitArray& m, uniques()) if (isIdent(m)) return true;
    return false;
}

bool cRecord::isIdent(const QBitArray& __m)
{
    for (int i = 0; i < __m.size(); ++i) if (__m[i] && isNull(i)) return false;
    return true;
}

cRecord& cRecord::set(const QSqlRecord& __r, int* _fromp, int __size)
{
    _set(__r, descr(), _fromp, __size);
    toEnd();
    modified();
    return *this;
}

cRecord& cRecord::set(int __i, const QVariant& __v)
{
    // _DBGFN() << " @(" << __i << _sCommaSp << __v.toString() << _sABraE << endl;
    if (_fields.isEmpty()) set();
    _set(__i, descr()[__i].set(__v, _stat));
    _stat |= ES_MODIFY;
    toEnd(__i);
    fieldModified(__i);
    // DBGFNL();
    return *this;
}

const QVariant& cRecord::_get(int __i) const
{
    if (_fields.size() <= __i || 0 > __i) return _vNul;
    return _fields[__i];
}

const QVariant& cRecord::get(int __i) const
{
    if (isNull()) return _vNul;
    return _fields[chkIndex(__i)];
}

QString cRecord::view(QSqlQuery& q, int __i) const
{
    if (isIndex(__i) == false) return QObject::trUtf8("[nincs]");
    return descr()[__i].toView(q, get(__i));
}

bool cRecord::insert(QSqlQuery& __q, bool _ex)
{
    // _DBGFN() << "@(," << DBOOL(_ex) << ") table : " << fullTableName() << endl;
    const cRecStaticDescr& recDescr = descr();
    __q.finish();
    if (!recDescr.isUpdatable()) {
        if (_ex) EXCEPTION(EDATA, -1 , QObject::trUtf8("Az adat nem módosítható."));
        return false;
    }
    QString sql, qms;
    if (descr()._deletedIndex != -1 && _deletedBehavior & REPLACED_BY_NAME) {
        cRecord *po = newObj();
        po->setName(getName());
        po->set(descr()._deletedIndex, QVariant(true));
        if (po->completion()) {     // Van egy azonos nevű deleted = true rekordunk.
            delete po;
            QBitArray   sets(cols(), true); // ???!!
            sets.clearBit(nameIndex());
            return update(__q, true, sets, mask(nameIndex()),_ex);
        }
        delete po;
    }
    sql  = "INSERT INTO " + fullTableNameQ() + " (";
    for (int i = 0; i < recDescr.cols(); ++i) {
        if (!get(i).isNull()) {
            qms += QString("?,");
            sql += dQuoted(columnName(i)) + QChar(',');
        }
    }
    if (qms.size() < 2) EXCEPTION(EDATA, 0, recDescr.fullTableName());
    qms.chop(1);    // Végén van egy felesleges vessző
    sql.chop(1);    // ennek is
    sql += ") VALUES ( " + qms + ")";   // a stóc kérdőjel
    sql += " RETURNING *";
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql)
    int i = 0;  // Nem null mezők indexe
    // PDEB(VVERBOSE) << "Insert cmd :" << sql << endl;
    for (int ix = 0; ix < recDescr.cols(); ++ix) {           // ix: mező index a rekordban
        const cColStaticDescr& f = recDescr.colDescr(ix);
        if (!isNull(ix)) {
            QVariant v = f.toSql(get(ix));
            // PDEB(VVERBOSE) << "bind #" << i << " value : " << v.toString() << endl;
            QSql::ParamType t = QSql::In;
            if (f.eColType == f.FT_BINARY) {
                t |= QSql::Binary;
            }
            __q.bindValue(i, v, t);
            ++i;
        }
    }
    bool r;
    if (!(r = __q.exec())) {
        // Pontosítani kéne, sajnos nincs hibakód!!
        if (_ex == false && __q.lastError().type() == QSqlError::StatementError) {
            SQLQUERYERRDEB(__q);
        }
        else {
            SQLQUERYERR(__q);
        }
    }
    else {
        if (__q.first()) {
            set(__q);
            _stat =  ES_EXIST | ES_COMPLETE | ES_IDENTIF;
            PDEB(VERBOSE) << "Insert RETURNING :" << toString() << endl;
        }
    }
    // DBGFNL();
    return r;
}

bool cRecord::query(QSqlQuery& __q, const QString& sql, const tIntVector& __arg, bool __ex)
{
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    for (int i = 0; i < __arg.size(); ++i) {
        const cColStaticDescr f = descr().colDescr(__arg[i]);
        const QVariant& v = f.toSql(get(__arg[i]));
        // PDEB(VVERBOSE) << "bind #" << j << " value : " <<  v.toString() << endl;
        QSql::ParamType t = QSql::In;
        if (f.eColType == f.FT_BINARY) {
            t |= QSql::Binary;
        }
        __q.bindValue(i, v, t);
    }
    bool r = __q.exec();
    if (__ex && !r) SQLQUERYERR(__q);
    return r;
}

bool cRecord::query(QSqlQuery& __q, const QString& sql, const QBitArray& _fm, bool __ex)
{
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    int i,j;
    for (i = j = 0; _fm.size() > i; i++) if (_fm.at(i) && false == get(i).isNull()) {
        const cColStaticDescr& f = descr().colDescr(i);
        const QVariant& v = f.toSql(get(i));
        // PDEB(VVERBOSE) << "bind #" << j << " value : " <<  v.toString() << endl;
        QSql::ParamType t = QSql::In;
        if (f.colType == "bytea") {
            t |= QSql::Binary;
        }
        __q.bindValue(j++, v, t);
    }
    bool r = __q.exec();
    if (__ex && !r) SQLQUERYERR(__q);
    return r;
}

QString cRecord::whereString(QBitArray& _fm)
{
    QString where;
    if (_fm.size() == 0) _fm = descr().primaryKey();
    if (_fm.count(true)) {
        for (int i = 0; _fm.size() > i; i++) if (_fm.at(i)) {
            if (!where.isEmpty()) {
                where += " AND ";
            }
            where += columnNameQ(i);
            where += get(i).isNull() ? " is NULL" : _isLike(i) ? " LIKE ?" : " = ?";
        }
        if (descr()._deletedIndex != -1 && _deletedBehavior & FILTERED && _fm.size() <= descr()._deletedIndex && _fm[descr()._deletedIndex] == FALSE) {
            where += " AND deleted = FALSE";
        }
        where = " WHERE " + where;
    }
    else if (descr()._deletedIndex != -1 && _deletedBehavior & FILTERED) {
        where = " WHERE deleted = FALSE";
    }
    return where;
}

void cRecord::fetchQuery(QSqlQuery& __q, bool __only, const QBitArray& _fm, const tIntVector& __ord, int __lim, int __off, QString __s, QString __w)
{
    // _DBGFN() << " @(" << _fm << _sCommaSp << __ord << _sCommaSp << __lim << _sCommaSp << __off <<  _sCommaSp << __s << _sABraE << endl;
    // PDEB(VVERBOSE) << "Record value : " << toString() << endl;
    const cRecStaticDescr& recDescr = descr();
    QBitArray fm = _fm;
    QString     sql;    // sql parancs
    sql  = "SELECT " + (__s.isEmpty() ? "*" : __s) + " FROM ";
    if (__only) sql += "ONLY ";
    sql += descr().fullViewName();
    if (__w.isEmpty()) {
        sql += whereString(fm);
    }
    else {
        sql += " WHERE " + __w;
    }
    if (__ord.size() > 0) {
        sql += " ORDER BY ";
        foreach (int i, __ord) {
            sql += recDescr.columnNameQ(qAbs(i));
            sql += _sSpace + (i < 0 ? "DESC" : "ASC") + _sComma;
        }
        sql.chop(1);
    }
    if (__lim > 0) sql += " LIMIT "  + QString::number(__lim);
    if (__off > 0) sql += " OFFSET " + QString::number(__off);
    query(__q, sql, fm, true);
}

bool cRecord::fetch(QSqlQuery& __q, bool __only, const QBitArray& _fm, const tIntVector& __ord, int __lim, int __off)
{
    // _DBGFN() << " @(" << _fm << _sCommaSp << __ord << _sCommaSp << __lim << _sCommaSp << __off << _sABraE << endl;
    fetchQuery(__q,__only, _fm, __ord, __lim, __off);
    if (__q.first()) {
        set(__q.record());
        _stat |= ES_EXIST;
        return true;
    }
    set();
    return false;
}

QBitArray cRecord::getSetMap()
{
    int i, n = cols();
    QBitArray m(n);
    for (i = 0; n > i; i++) m[i] = !get(i).isNull();
    return m;
}

int cRecord::completion(QSqlQuery& __q)
{
    QBitArray m = getSetMap();
    if (m.count(true) == 0) return -1;
    fetch(__q, false, m);
    return __q.size();
}

bool cRecord::first(QSqlQuery& __q)
{
    if (__q.first()) {
        set(__q.record());
        return true;
    }
    set();
    return false;

}
bool cRecord::next(QSqlQuery& __q)
{
    if (__q.next()) {
        set(__q.record());
        return true;
    }
    set();
    return false;

}

qlonglong cRecord::fetchTableOId(QSqlQuery& __q, bool __ex)
{
    QBitArray m = getSetMap();              // Mit ismerünk ?
    if (m.count(true) == 0) {
        if (__ex) EXCEPTION(EDATA);
        return NULL_ID; // Ha semmit, az gáz
    }
    // Ha lehet szűkítjük a feltételben résztvevő mezőket
    if ((m & primaryKey()) == primaryKey()) m = primaryKey();   // Ha ismert az elsődleges kulcs
    else foreach (QBitArray u, uniques()) {
        if ((m & u) == u) {                                     // Ismert egy unique kulcs
            m = u;
            break;
        }
    }

    fetchQuery(__q, false, m, tIntVector(),0,0,QString("tableoid"));
    if (__q.first()) {
        if (__q.size() == 1) {
            bool ok;
            qlonglong r = __q.value(0).toLongLong(&ok);
            if (!ok) {
                if (__ex) EXCEPTION(EDATA);
                return NULL_ID;
            }
            return r;
        }
        else {
            if (__ex) EXCEPTION(AMBIGUOUS);
            return NULL_ID;
        }
    }
    if (__ex) EXCEPTION(EFOUND);
    return NULL_ID;
}

bool cRecord::update(QSqlQuery& __q, bool __only, const QBitArray& __set, const QBitArray& __where, bool __ex)
{
//    _DBGFN() << "@("
//             << this->toString() << _sCommaSp << DBOOL(__only) << _sCommaSp << list2string(__set) <<  _sCommaSp << list2string(__where) << _sCommaSp << DBOOL(__ex)
//             << ")" << endl;
    __q.finish();
    if (!isUpdatable()) {
        if (__ex) EXCEPTION(EDATA, -1 , QObject::trUtf8("Az adat nem módosítható."));
        return false;
    }
    QBitArray bset  = __set;
    QBitArray where = __where;
    if (where.size() == 0) where = primaryKey();
    if (bset.count(true) == 0) {
        bset = QBitArray(cols(), true) & ~where;      // Változott, nem csak a beállított értékeket updateli alapértelmezetten
    }
    if (bset.count(true) == 0) {
        if (__ex) EXCEPTION(EDATA);
        return false;
    }
    QString sql = QString(__only ? "UPDATE ONLY %1 SET" : "UPDATE %1 SET").arg(fullTableNameQ());
    int i,j;
    for (i = 0; i < bset.size(); i++) {
        if (bset[i]) {
            sql += _sSpace + columnNameQ(i);
            if (get(i).isNull()) sql += " = DEFAULT,";
            else                 sql += _sCondW + _sSpace + _sComma;
        }
    }
    sql.chop(1);
    QString w;
    for (i = 0; i < where.size(); i++) {
        if (where[i]) {
            if (w.size() > 0) w += " AND";
            w += _sSpace + columnNameQ(i);
            if (get(i).isNull()) w += _sIsNull;
            else                 w += _sCondW;
            w += _sSpace;
        }
    }
    w.chop(1);
    if (w.size()) sql += " WHERE" + w + " RETURNING *";
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql)
    for (j = i = 0; i < bset.size(); i++) {
        if (bset[i] && false == get(i).isNull()) {
            const cColStaticDescr& f = descr().colDescr(i);
            QSql::ParamType t = QSql::In;
            if (f.colType == "bytea") {
                t |= QSql::Binary;
            }
            __q.bindValue(j++, f.toSql(get(i)), t);
        }
    }
    for (i = 0; i < where.size(); i++)
        if (where[i] && false == get(i).isNull())
            __q.bindValue(j++, descr().colDescr(i).toSql(get(i)));
    bool r;

    if (!(r = __q.exec())) {
        // Pontosítani kéne, sajnos nincs hibakód!!
        if (__q.lastError().type() == QSqlError::StatementError && !__ex) SQLQUERYERRDEB(__q)
        else                                                              SQLQUERYERR(__q)
    }
    else {
        if (__q.first()) {
            set(__q);
            _stat =  ES_EXIST | ES_COMPLETE | ES_IDENTIF;
            PDEB(VERBOSE) << "Update RETURNING :" << toString() << endl;
        }
        else {
            DWAR() << "Nothing modify any record." << endl;
            if (__ex) EXCEPTION(EDATA);
            r = false;
        }
    }
    // DBGFNL();
    return r;

}

bool cRecord::remove(QSqlQuery& __q, bool __only, const QBitArray& _fm, bool __ex)
{
    QBitArray fm = _fm;
    if (!isUpdatable()) {
        if (__ex) EXCEPTION(EDATA, -1 , QObject::trUtf8("Az adat nem módosítható."));
        return false;
    }
    if (descr()._deletedIndex != -1 && _deletedBehavior & DELETE_LOGICAL) {
        set(descr()._deletedIndex, QVariant(true));
        return update(__q, __only, mask(descr()._deletedIndex), fm, __ex);
    }
    QString sql = "DELETE FROM ";
    if (__only) sql += "ONLY ";
    sql += tableName() + whereString(fm);
    return query(__q, sql, fm, __ex) && __q.numRowsAffected() > 0;
}

bool cRecord::checkData()
{
    int i;
    const cRecStaticDescr& recDescr = descr();
    const cColStaticDescrList& colcDescrs = recDescr.columnDescrs();
    if (_fields.count() == 0) {
        _stat = ES_EMPTY;
        return false;
    }
    _stat = ES_EMPTY;
    bool    allNull = true;
    for (i = 0; isIndex(i); ++i) {
        if (!get(i).isNull()) {
            allNull = false;
        }
        else if (colcDescrs[i].isNullable == false && colcDescrs[i].colDefault.isEmpty()) {
            _stat = ES_DEFECTIVE;
        }
    }
    if (allNull == true) {
        _stat = ES_EMPTY;
        return false;
    }
    if (isIdent()) _stat |= ES_IDENTIF;
    return true;
}

cRecordFieldRef cRecord::operator[](int __i)
{
    return cRecordFieldRef(*this, __i);
}
cRecordFieldConstRef cRecord::operator[](int __i) const
{
    return cRecordFieldConstRef(*this, __i);
}

QString cRecord::toString() const
{
    QString s = _sCBraB;
    for (int i = 0; isIndex(i); i++) {
        s += _sSpace + columnNameQ(i);
        if (isNull(i)) s += " IS NULL, ";
        else           s +=  " = " + getName(i) + QString("::") + QString(get(i).typeName()) + _sSColon + _sSpace;
    }
    s.chop(2);
    s + _sCBraE;
    return s;
}

// NEM KEZELI AZ ESETLEGES DELETED MEZŐT !!!!
int cRecord::delByName(QSqlQuery& q, const QString& __n, bool __pat, bool __only)
{
    QString sql = "DELETE FROM ";
    if (__only) sql += "ONLY ";
    sql += tableName() + " WHERE " + dQuoted(nameName()) + (__pat ? " LIKE " : " = ") + quoted(__n);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    int n = q.numRowsAffected();
    PDEB(VVERBOSE) << "delByName SQL : " << sql << " removed " << n << " records." << endl;
    return  n;
}

/* ******************************************************************************************************* */
// Dummy osztály dummy példány
no_init_ _no_init_;
/* ******************************************************************************************************* */
cAlternate::cAlternate() : cRecord()
{
    pStaticDescr = NULL;
    _stat = ES_FACELESS;
}

cAlternate::cAlternate(const QString& _tn, const QString& _sn) : cRecord()
{
    pStaticDescr = cRecStaticDescr::get(_tn, _sn);
    _set(*pStaticDescr);
}

cAlternate::cAlternate(const cRecStaticDescr *_p) : cRecord()
{
    pStaticDescr = _p;
    _set(*pStaticDescr);
}

cAlternate::cAlternate(const cAlternate &__o)  : cRecord()
{
    *this = *(cRecord *)&__o;
}

cAlternate::cAlternate(const cRecord& __o) : cRecord()
{
    *this = __o;
}

cAlternate::~cAlternate()
{
    ;
}

cAlternate& cAlternate::setType(const QString& _tn, const QString& _sn)
{
    pStaticDescr = cRecStaticDescr::get(_tn, _sn);
    _set(*pStaticDescr);
    return *this;
}

cAlternate& cAlternate::setType(const cRecord& __o)
{
    pStaticDescr = &__o.descr();
    _set(*pStaticDescr);
    return *this;
}

cAlternate& cAlternate::setType(const cRecStaticDescr *_pd)
{
    pStaticDescr = _pd;
    _set(*pStaticDescr);
    return *this;
}

const cRecStaticDescr& cAlternate::descr() const
{
    if (pStaticDescr == NULL) EXCEPTION(EPROGFAIL,-1,QObject::trUtf8("Nincs beállítva az adat típus."));
    return *pStaticDescr;
}

cRecord *cAlternate::newObj() const
{
    cAlternate *p = new cAlternate();
    if (!isFaceless()) p->setType(*this);
    return p;
}
cRecord *cAlternate::dup() const
{
    return new cAlternate(*this);
}

cAlternate& cAlternate::operator=(const cRecord& __o)
{
    if (isFaceless()) {
        clearType();
    }
    else {
        setType(__o);
        _cp(__o);
    }
    return *this;
}

cAlternate& cAlternate::clearType()
{
    _clear();
    pStaticDescr = NULL;
    _stat = ES_FACELESS;
    return *this;
}
