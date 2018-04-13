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
    return metaIsFloat(_v.userType()) || metaIsInteger(_v.userType());
}


QString langBool(bool b)
{
    return b ? QObject::trUtf8("igen") : QObject::trUtf8("nem");
}

bool str2bool(const QString& _b, enum eEx __ex)
{
    QString b = _b.toLower();
    if (b == "t" || b == "true" || b == "y" || b == "yes" || b == "on" || b == "1") return true;
    if (b == "f" || b == "false"|| b == "n" || b == "no"  || b == "off"|| b == "0") return false;
    if (langBool(true)  == b) return true;
    if (langBool(false) == b) return false;
    if (__ex) EXCEPTION(EDBDATA, -1, b);
    return !b.isEmpty();
}

eTristate str2tristate(const QString& _b, enum eEx __ex)
{
    QString b = _b.toLower();
    if (_b.isEmpty()) return TS_NULL;
    if (b == "t" || b == "true" || b == "y" || b == "yes" || b == "on" || b == "1") return TS_TRUE;
    if (b == "f" || b == "false"|| b == "n" || b == "no"  || b == "off"|| b == "0") return TS_FALSE;
    if (langBool(true)  == b) return TS_TRUE;
    if (langBool(false) == b) return TS_FALSE;
    if (__ex) EXCEPTION(EDBDATA, -1, b);
    return TS_NULL;
}


bool strIsBool(const QString& _b)
{
    static QStringList  boolValues;
    if (boolValues.isEmpty()) {
        boolValues << "t" << "true"  << "y" << "yes" << "on"  << "1"
                   << "f" << "false" << "n" << "no"  << "off" << "0"
                   << langBool(true) <<  langBool(false);
    }
    return boolValues.contains(_b, Qt::CaseInsensitive);
}

static qlonglong __schemaoid(QSqlQuery q, const QString& __s)
{
    // DBGFN();
    QString sql = "SELECT oid FROM pg_namespace WHERE nspname = ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __s);
    if (!q.exec()) SQLQUERYERR(q);
    if (!q.first()) EXCEPTION(EDBDATA, 0, QObject::trUtf8("Schema %1 OID not found.").arg(__s));
    return q.value(0).toInt();
}

qlonglong schemaoid(QSqlQuery q, const QString& __s)
{
    // DBGFN();
    static qlonglong _publicSchemaOId = NULL_ID;
    if (__s.isEmpty() || __s == _sPublic) {
        if (_publicSchemaOId == NULL_ID) {
            _publicSchemaOId = __schemaoid(q, _sPublic);
        }
        return _publicSchemaOId;
    }
    return __schemaoid(q, __s);
}

qlonglong tableoid(QSqlQuery q, const QString& __t, qlonglong __sid, eEx __ex)
{
    // DBGFN();
    if (__sid == NULL_ID) __sid = schemaoid(q, _sPublic);
    QString sql = "SELECT oid FROM pg_class WHERE relname = ? AND relnamespace = ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __t);
    q.bindValue(1, __sid);
    if (!q.exec()) SQLQUERYERR(q);
    if (!q.first()) {
        if (__ex) EXCEPTION(EDBDATA, 0, QObject::trUtf8("Table [#%1].%2 OID not found.").arg(__sid).arg(__t));
        return NULL_ID;
    }
    return q.value(0).toInt();
}

tStringPair tableoid2name(QSqlQuery q, qlonglong toid)
{
    QString sql =
            " SELECT pg_class.relname, pg_namespace.nspname"
            " FROM pg_class"
            " JOIN pg_namespace ON pg_namespace.oid = pg_class.relnamespace"
            " WHERE pg_class.oid = " + QString::number(toid);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (!q.first()) EXCEPTION(EDBDATA, 0, QObject::trUtf8("Table OID %1 not found.").arg(toid));
    tStringPair r;
    r.first  = q.value(0).toString();
    r.second = q.value(1).toString();
    return r;
}

/* ******************************************************************************************************* */

QSet<cColEnumType>  cColEnumType::colEnumTypes;

cColEnumType::cColEnumType(qlonglong id, const QString &name, const QStringList &values)
    : QString(name)
    , typeId(id)
    , enumValues(values)
{
    if (find(name) != NULL) EXCEPTION(EPROGFAIL, id, name);
    colEnumTypes.insert(*this);
}

cColEnumType::cColEnumType(const cColEnumType& _o)
    : QString(_o)
    , typeId(_o.typeId)
    , enumValues(_o.enumValues)
{
    ;
}

QString cColEnumType::toString() const
{
    return "Enum " + dQuoted((QString)*this) + " #" + QString::number(typeId)
              + " {" + enumValues.join(", ") + "}";
}

const cColEnumType *cColEnumType::fetchOrGet(QSqlQuery& q, const QString& name, eEx __ex)
{

    const cColEnumType *r = find(name);
    if (r != NULL) return r;        // Ha a típust már beolvastuk
    QString sql = QString(
                "SELECT pg_enum.enumlabel, pg_enum.enumtypid FROM  pg_catalog.pg_enum JOIN pg_catalog.pg_type ON pg_type.oid = pg_enum.enumtypid "
                "WHERE pg_type.typname = '%1' ORDER BY pg_enum.enumsortorder ASC").arg(name);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (q.first()) {
        qlonglong id = q.value(1).toLongLong();
        QStringList vals;
        do {
            vals << q.value(0).toString();
        } while(q.next());
        return new cColEnumType(id, name, vals);
    }
    if (__ex) EXCEPTION(EDATA, -1, QObject::trUtf8("Ismeretlrn enumerációs típus név: %1").arg(name));
    return NULL;
}

bool cColEnumType::check(const QStringList& v) const
{
    foreach (QString s, v) {
        if (!check(s)) return false;
    }
    return true;
}

const QString& cColEnumType::enum2str(qlonglong e, eEx __ex) const
{
    if (isContIx(enumValues, e)) {
        return enumValues[e];
    }
    if (__ex) EXCEPTION(EDATA, e, QObject::trUtf8("Nem megengedett enumerációs érték : %1.#%2").arg(*this).arg(e));
    return _sNul;
}

QStringList cColEnumType::set2lst(qlonglong b, eEx __ex) const
{
    if (__ex >= EX_ERROR && !checkSet(b)) {
        EXCEPTION(EDATA, b, QObject::trUtf8("Nem megengedett enumerációs érték : [%1]").arg(*this, QString::number(b, 2)));
    }
    QStringList r;
    for (int i = 0; i < enumValues.size(); ++i) {
        if (b & (1LL << i)) r << enumValues[i];
    }
    return r;
}

qlonglong cColEnumType::str2enum(const QString& _s, eEx __ex) const
{
    QString s = _s;
    if (s.endsWith("::" + *this)) { // '<value>'::<type>
        s.chop(this->size() + 2);
        s = unQuoted(s);
    }
    for (int i = 0; i < enumValues.size(); ++i) {
        if (0 == enumValues[i].compare(s, Qt::CaseInsensitive)) return i;
    }
    if (0 == this->compare(_sBoolean)) {    // type is boolean ?
        switch (str2tristate(s, EX_IGNORE)) {
        case TS_TRUE:   return 1;
        case TS_FALSE:  return 0;
        default:        break;
        }
    }
    if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, QObject::trUtf8("Nem megengedett enumerációs érték : %1.%2").arg(*this).arg(_s));
    return ENUM_INVALID;

}
qlonglong cColEnumType::str2set(const QString& s, eEx __ex) const
{
    for (int i = 0; i < enumValues.size(); ++i) {
        if (0 == enumValues[i].compare(s, Qt::CaseInsensitive)) return 1LL << i;
    }
    if (__ex) EXCEPTION(EDATA, -1, QObject::trUtf8("Nem megengedett enumerációs érték : %1.%2").arg(*this).arg(s));
    return 0;
}
qlonglong cColEnumType::lst2set(const QStringList &s, eEx __ex) const
{
    qlonglong r = 0;
    foreach (QString n, s) {
        r |= str2set(n, __ex);
    }
    return r;
}

tIntVector cColEnumType::lst2lst(const QStringList& _lst, enum eEx __ex) const
{
    tIntVector r;
    foreach (QString s, _lst) {
        int e = str2enum(s, __ex);
        if (e >= 0) r << e;
    }
    return r;
}
QStringList cColEnumType::lst2lst(const tIntVector& _lst, enum eEx __ex) const
{
    QStringList r;
    foreach (int e, _lst) {
        QString s = enum2str(e, __ex);
        if (isContIx(enumValues, e)) r << s;    // The empty string can be real value as well
    }
    return r;
}


const cColEnumType *cColEnumType::find(const QString& name)
{
     QSet<cColEnumType>::const_iterator i = colEnumTypes.find(cColEnumType(name));
     if (i == colEnumTypes.constEnd()) return NULL;
     return &(*i);
}

const cColEnumType& cColEnumType::get(const QString& name)
{
    const cColEnumType *p = find(name);
    if (p == NULL) EXCEPTION(EDATA, -1, name);
    return *p;
}

QString     cColEnumType::normalize(const QString& nm, bool *pok) const
{
    foreach (QString e, enumValues) {
        if (0 == e.compare(nm, Qt::CaseInsensitive)) {
            if (pok != NULL) *pok = true;
            return e;
        }
    }
    if (pok != NULL) *pok = false;
    return QString();
}

QStringList cColEnumType::normalize(const QStringList& lst, bool *pok) const
{
    qlonglong r = 0;
    bool ok = true;
    foreach (QString n, lst) {
        qlonglong b = str2set(n, EX_IGNORE);
        if (b == 0) ok = false;
        else        r |= b;
    }
    if (pok != NULL) *pok = ok;
    return set2lst(r);
}

void cColEnumType::checkEnum(tE2S e2s, tS2E s2e) const
{

    int n = enumValues.size();
    int i;
    if (n == 0) { // Ez nem hihető.
        EXCEPTION(EPROGFAIL,2, QObject::trUtf8("A %1 enumerációs típusnak nincs értékkészlete.").arg(toString()));
    }
    for (i = 0; i < n; ++i) {
        const QString& s = enumValues[i];
        int ii = (*s2e)(s, EX_IGNORE);
        if (i != ii) {
            EXCEPTION(EENUMVAL, 1, QObject::trUtf8("%1 : #%2/%3 : s2e(%3) ~ %4")
                      .arg(toString()).arg(i).arg(s).arg(ii) )
        }
        const QString& ss = (*e2s)(i, EX_IGNORE);
        if (0 != s.compare(ss, Qt::CaseInsensitive)) {
            EXCEPTION(EENUMVAL, 4, QObject::trUtf8("%1 : #%2/%3 : e2s(%4) ~ %5")
                      .arg(toString()).arg(i).arg(s).arg(ii).arg(ss) )
        }
    }
    const QString e = (*e2s)(i, EX_IGNORE);
    if (e.isEmpty() == false) {    // Nem lehet több elem a konverziós függvény szerint!
        EXCEPTION(EENUMVAL, 5, QObject::trUtf8("A %1 adatbázis típus hiényos, extra elem : #%2/%3")
                  .arg(toString()).arg(i).arg(e));
    }
}

void cColEnumType::checkEnum(QSqlQuery& q, const QString& _type, tE2S e2s, tS2E s2e)
{
    const cColEnumType *p = fetchOrGet(q, _type);
    p->checkEnum(e2s, s2e);
}

/* ******************************************************************************************************* */

cColStaticDescr::cColStaticDescr(const cRecStaticDescr *_p, int __t)
    : QString()
    , pParent(_p)
    , colType()
    , udtName()
    , colDefault()
    , fKeySchema()
    , fKeyTable()
    , fKeyField()
    , fKeyTables()
    , fnToName()
{
    isNullable  = false;
    pos =  ordPos = -1;
    eColType = __t;
    pEnumType = NULL;
    chrMaxLenghr = -1;
    isUpdatable = false;
    fKeyType = FT_NONE;
    pFRec    = NULL;
}

cColStaticDescr::cColStaticDescr(const cColStaticDescr& __o)
    : QString(__o)
    , pParent(__o.pParent)
    , colType(__o.colType)
    , udtName(__o.udtName)
    , colDefault(__o.colDefault)
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
    pEnumType    = __o.pEnumType;
    chrMaxLenghr= __o.chrMaxLenghr;
    isUpdatable = __o.isUpdatable;
    fKeyType    = __o.fKeyType;
    pFRec       = __o.pFRec;
}

cColStaticDescr::~cColStaticDescr()
{
    ;
}

cColStaticDescr& cColStaticDescr::operator=(const cColStaticDescr __o)
{
    *(QString *)this = __o;
    pParent     = __o.pParent;
    colType     = __o.colType;
    udtName     = __o.udtName;
    chrMaxLenghr= __o.chrMaxLenghr;
    colDefault  = __o.colDefault;
    isNullable  = __o.isNullable;
    ordPos      = __o.ordPos;
    pos         = __o.pos;
    pEnumType    = __o.pEnumType;
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
    QString r = dQuoted((QString&)*this);
    r += QChar('[') + QString::number(pos) + QChar('/') + QString::number(ordPos) + QChar(']');
    r += QChar(' ') + colType;
    if (chrMaxLenghr > 0) r += QString("(%1)").arg(chrMaxLenghr);
    r +=  "/" + udtName;
    if (pEnumType != NULL)     r += QChar('{') + pEnumType->enumValues.join(_sCommaSp) + QChar('}');
    if (!isNullable)           r += " NOT NULL ";
    if (!colDefault.isNull()) {
        r += " DEFAULT " + colDefault;
    }
    r += (isUpdatable ? QChar(' ') : QObject::trUtf8(" nem")) + QObject::trUtf8(" módosítható");
    return r;
}

QString cColStaticDescr::allToString() const
{
    QString r = toString() + '\n';
    r +=   "eColType = " + QString::number(eColType, 16);
    r += ", fKeyType = " + QString::number(fKeyType, 16);
    if (fKeyField.size() > 0) {
        QString k = mCat(fKeyTable, fKeyField);
        if (fKeySchema.size() > 0) k = mCat(fKeySchema, k);
        r += _sCommaSp + k;
        if (fKeyTables.size() > 0) {
            k += QChar('(') + fKeyTables.join(_sCommaSp) + QChar(')');
        }
    }
    if (fnToName.size() > 0) ", fnToName = " + fnToName;
    if (pFRec != NULL) ", pFRec : " + pFRec->tableName();
    return r;
}

// TEST!!!
enum cColStaticDescr::eValueCheck cColStaticDescr::check(const QVariant& v, cColStaticDescr::eValueCheck acceptable) const
{
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (v.isNull() || isNumNull(v)
     || (eColType != FT_TEXT && variantIsString(v) && v.toString().isEmpty())) {
        return ifExcep(checkIfNull(), acceptable, v);
    }
    switch (eColType) {
    case FT_INTEGER:
        if (v.canConvert(QVariant::LongLong)) switch (v.type()) {
        case QVariant::Int:
        case QVariant::LongLong:    r = VC_OK;      break;
        default:                    r = VC_CONVERT; break;
        }
        return ifExcep(r, acceptable, v);
    case FT_REAL:
        if (v.canConvert(QVariant::Double)) switch (v.type()) {
        case QVariant::Double:      r = VC_OK;      break;
        default:                    r = VC_CONVERT; break;
        }
        return ifExcep(r, acceptable, v);
    case FT_BINARY:
        return ifExcep(v.canConvert(QVariant::ByteArray)? VC_OK : VC_INVALID, acceptable, v);
    case FT_TEXT:
        if (v.canConvert(QVariant::String)) {
            if (chrMaxLenghr > 0 && chrMaxLenghr < v.toString().size()) r = VC_TRUNC;
            else r = v.type() == QVariant::String ? VC_OK : VC_CONVERT;
        }
        return ifExcep(r, acceptable, v);
    default:
        EXCEPTION(EPROGFAIL, eColType);
        break;
    }
    return ifExcep(VC_INVALID, acceptable, v);
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
- eColType == FT_BYNARY esetén QByteArray
Ha a paraméter típusa qlonglong, akkor a NULL_ID, ha int, akkor a NULL_IX is null objektumot jelent.
Ha a konverzió nem lehetséges, akkor a visszaadott érték egy üres objektum lessz, és a
statusban a ES_DEFECTIVE bit be lessz billentve (ha nincs hiba, akkor a bit nem törlődik).
Ha üres objektumot adunk meg, és isNullable értéke false, akkor szintén be lessz állítva a hiba bit.
@param _f A konvertálandó érték
@param str A status referenciája
@return A konvertált érték
@exception cError Ha eColType értéke nem a fenti három érték eggyike.
*/
QVariant cColStaticDescr::set(const QVariant& _f, qlonglong &str) const
{
    QVariant r =_f;
    bool ok = true;
    if ((eColType != FT_TEXT && variantIsString(_f) && _f.toString().isEmpty())
     || isNumNull(_f)) {
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
    case FT_BINARY:
        ok = _f.canConvert<QByteArray>();
        r = _f.toByteArray();
        break;
    default:
        EXCEPTION(EPROGFAIL, eColType);
        break;
    }
    if (!ok) str |= ES_DEFECTIVE;
    return r;
}

QString cColStaticDescr::toName(const QVariant& _f) const
{
    return QVariantToString(_f);
}

qlonglong cColStaticDescr::toId(const QVariant& _f) const
{
    return variantToId(_f, EX_IGNORE, NULL_ID);
}

QString cColStaticDescr::fKeyId2name(QSqlQuery& q, qlonglong id, eEx __ex) const
{
    if (fKeyType == FT_TEXT_ID) {
        return textId2text(q, (int)id, pParent->tableName(), 0);
    }
    QString n = QString::number(id);
    QString r = "#" + n;    // If fail
    if (fnToName.isEmpty() == false) {
        QString sql = "SELECT " + fnToName + parentheses(n);
        EXECSQL(q, sql);
        if (q.first()) return q.value(0).toString();
        return r;
    }
    if (fKeyTable.isEmpty() == false) {
        if (pFRec == NULL) {
            // Sajnos itt trükközni kell, mivel ezt máskor nem tehetjük meg, ill veszélyes, macerás, de itt meg konstans a pointer
            // A következő sor az objektum feltöltésekor, ahol még írható, akár végtelen rekurzióhoz is vezethet.
            // A rekurzió detektálása megvan, de kivédeni kellene, nem elég eszrevenni.
            *const_cast<cRecordAny **>(&pFRec) = new cRecordAny(fKeyTable, fKeySchema);
        }
        QString n = pFRec->getNameById(q, id , __ex);
        if (n.isEmpty()) return r;
        return n;
    }
    return r;
}

const QString  cColStaticDescr::rNul = "[NULL]";
const QString  cColStaticDescr::rBin = "[BINARY]";

QString cColStaticDescr::toView(QSqlQuery& q, const QVariant &_f) const
{
    if (_f.isNull())           return rNul;
    if (eColType == FT_BINARY) return rBin;
    if (eColType == FT_INTEGER && fKeyType != FT_NONE) {
        qlonglong id = toId(_f);
        if (id == NULL_ID) return rNul; //?!
        return fKeyId2name(q, id);
    }
    return toName(_f);
}

QString cColStaticDescr::toViewIx(QSqlQuery&q, const QVariant& _f, int _ix) const
{
    if (eColType == FT_INTEGER && fKeyType == FT_TEXT_ID) {
        return textId2text(q, (int)toId(_f), pParent->tableName(), _ix);
    }
    return toView(q, _f);
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
    TYPEDETECT(_sPoint,    FT_POINT)
    TYPEDETECT(_sBoolean,  FT_BOOLEAN)
    TYPEDETECT("macaddr",  FT_MAC)
    TYPEDETECT("inet",     FT_INET)
    TYPEDETECT("cidr",     FT_CIDR)
    TYPEDETUDT("time",     FT_TIME)
    TYPEDETUDT("date",     FT_DATE)
    TYPEDETUDT("timestamp",FT_DATE_TIME)
    TYPEDETUDT("interval", FT_INTERVAL)
    if      (pEnumType != NULL)                              eColType = FT_ENUM;
    else if (udtName.contains("int",   Qt::CaseInsensitive)) eColType = FT_INTEGER;
    else if (udtName.contains("real",  Qt::CaseInsensitive)) eColType = FT_REAL;
    else if (udtName.contains("double",Qt::CaseInsensitive)) eColType = FT_REAL;
    else if (udtName.contains("char",  Qt::CaseInsensitive)) eColType = FT_TEXT;
    else if (!udtName.compare("text",  Qt::CaseInsensitive)) eColType = FT_TEXT;
    else {
        EXCEPTION(ENOTSUPP, -1, QObject::trUtf8("Unknown %1 field type %2/%3 ").arg(colName(), colType, udtName));
    }
    if (colType == _sARRAY) eColType |= FT_ARRAY;
    return;
}

void cColStaticDescr::checkEnum(tE2S e2s, tS2E s2e, const char *src, int lin, const char *fn) const
{
    if (pEnumType == NULL) {       // Ez nem is enum.
        cError::exception(src, lin, fn, eError::EPROGFAIL,
                          1, QObject::trUtf8("A %1 mező men enumerációs típusú.").arg(colName()));
    }
    int n = pEnumType->enumValues.size();
    int i;
    if (n == 0) { // Ez nem hihető.
        cError::exception(src, lin, fn, eError::EPROGFAIL,
                          2, QObject::trUtf8("A %1 enumerációs típusnak nincs értékkészlete.").arg(*pEnumType));
    }
    for (i = 0; i < n; ++i) {
        const QString& s = pEnumType->enumValues[i];
/*        if (s.isEmpty()) {
            EXCEPTION(EENUMVAL, 3, QObject::trUtf8("%1 : #%2 is empty")
                      .arg(*pEnumType).arg(i) )
        }*/
        int ii = (*s2e)(s, EX_IGNORE);
        if (i != ii) {
            EXCEPTION(EENUMVAL, 1, QObject::trUtf8("%1 : #%2/%3 : s2e(%3) ~ %4")
                      .arg(*pEnumType).arg(i).arg(s).arg(ii) )
        }
        const QString& ss = (*e2s)(i, EX_IGNORE);
        if (0 != s.compare(ss, Qt::CaseInsensitive)) {
            EXCEPTION(EENUMVAL, 4, QObject::trUtf8("%1 : #%2/%3 : e2s(%4) ~ %5")
                      .arg(*pEnumType).arg(i).arg(s).arg(ii).arg(ss) )
        }
    }
    const QString e = (*e2s)(i, EX_IGNORE);
    if (e.isEmpty() == false) {    // Nem lehet több elem a konverziós függvény szerint!
        EXCEPTION(EENUMVAL, 5, QObject::trUtf8("A %1 adatvázis típus hiényos, extra elem : #%2/%3")
                  .arg(*pEnumType).arg(i).arg(e))
    }
}

const QString cColStaticDescr::valueCheck(int e)
{
    switch (e) {
    case VC_INVALID:    return "INVALID";
    case VC_TRUNC:      return "TRUNCATE";
    case VC_NULL:       return "NULL";
    case VC_DEFAULT:    return "DEFAULT";
    case VC_CONVERT:    return "CONVERT";
    case VC_OK:         return "OK";
    default:            return "UNKNOWN";
    }
}

cColStaticDescr::eValueCheck cColStaticDescr::ifExcep(cColStaticDescr::eValueCheck result, cColStaticDescr::eValueCheck acceptable, const QVariant& val) const
{
    if (result < acceptable) {
        EXCEPTION(EDATA, acceptable, QObject::trUtf8("A {%1} mező %2 érték ellenörzésének az eredménye '%3'")
                  .arg(toString())
                  .arg(debVariantToString(val))
                  .arg(valueCheck(result))
                  );
    }
    return result;
}

/* ------------------------------------------------------------------------------------------------------- */

enum cColStaticDescr::eValueCheck cColStaticDescrBool::check(const QVariant& v, cColStaticDescr::eValueCheck acceptable) const
{
    eValueCheck r = VC_INVALID;
    if      (v.isNull() || isNumNull(v))     r = checkIfNull();
    else if (variantIsString(v))             r = strIsBool(v.toString()) ? VC_OK : VC_CONVERT;
    else if (v.type() == QVariant::Bool)     r = VC_OK;
    else if (v.canConvert(QVariant::Bool))   r = VC_CONVERT;
    return ifExcep(r, acceptable, v);
}


QVariant  cColStaticDescrBool::fromSql(const QVariant& _f) const
{
    return str2bool(_f.toString(), EX_ERROR);
}

QVariant  cColStaticDescrBool::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::trUtf8("Data is NULL"));
    if (_f.userType() == QMetaType::Bool) return QVariant(QString(_f.toBool() ? "true":"false"));
    bool ok;
    qlonglong i = _f.toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA,-1,QObject::trUtf8("Az adat nem értelmezhető."));
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
QVariant  cColStaticDescrBool::set(const QVariant& _f, qlonglong & str) const
{
    bool ok = true;
    QVariant r =_f;
    if (isNumNull(_f)) r.clear();
    if (r.isNull()) {
        ok = isNullable || !colDefault.isEmpty();
    }
    else if (variantIsString(_f)) {
        r = str2bool(_f.toString(), EX_IGNORE);
    }
    else if (variantIsInteger(_f)) {
        r = _f.toLongLong(&ok) != 0;
    }
    else {
        ok = _f.canConvert<bool>();
        r = _f.toBool();
    }
    if (!ok) str |= ES_DEFECTIVE;
    return r;
}
QString   cColStaticDescrBool::toName(const QVariant& _f) const
{
    return _f.toBool() ? _sTrue : _sFalse;
}
qlonglong cColStaticDescrBool::toId(const QVariant& _f) const
{
    if (isNull()) return NULL_ID;
    return _f.toBool() ? 1 : 0;
}

QString cColStaticDescrBool::toView(QSqlQuery&, const QVariant& _f) const
{
    return langBool(_f.toBool());
}

void cColStaticDescrBool::init()
{
    const cColEnumType *pt = cColEnumType::find(_sBoolean);
    QStringList enumVals;
    enumVals << langBool(false);
    enumVals << langBool(true);
    if (pt == NULL) {
        pt = new cColEnumType(NULL_ID, _sBoolean, enumVals);
    }
    pEnumType = pt;
}

CDDUPDEF(cColStaticDescrBool)
/* ....................................................................................................... */
QVariant stringListToSql(const QStringList& sl)
{
    bool    empty = true;
    QString s = QChar('{');
    foreach (const QString& si, sl) {
        s += dQuoted(si) + QChar(',');
        empty = false;
    }
    if (empty == false) s.chop(1);
    s += QChar('}');
    return QVariant(s);
}

QStringList sqlToStringList(const QString& _s)
{
    QStringList sl = _s.split(QChar(','), QString::KeepEmptyParts);
    const   QChar   m('"');
    for (int i = 0; i < sl.size(); ++i) {
        QString s = sl[i];
        if (s[0] == m) {
            while (! s.endsWith(m)) {   // Ha szétdaraboltunk egy stringet, újra összerakjuk
                if (sl.size() <= (i +1)) EXCEPTION(EDBDATA, 8, "Invalid string array : " + s);
                sl[i] += QChar(',') + (s = sl[i +1]);
                sl.removeAt(i +1);
            }
            s = sl[i] = sl[i].mid(1, sl[i].size() -2);  // Lekapjuk az idézőjelet
        }
        if (s.isEmpty()) sl.removeAt(i--);  // Az üres stringek eltávolítása
    }
    return sl;
}

cColStaticDescr::eValueCheck  cColStaticDescrArray::check(const QVariant& v, cColStaticDescr::eValueCheck acceptable) const
{
    if (v.isNull() || isNumNull(v)) return ifExcep(checkIfNull(), acceptable, v);
    int t = v.userType();
    cColStaticDescr::eValueCheck r = VC_INVALID;
    switch (eColType) {
    case FT_INTEGER_ARRAY:
        // Lista, elemeinek típusa, kovertálhatósága
        if (t == QMetaType::QVariantList) {
            r = VC_OK;
            foreach (QVariant e, v.toList()) {
                if (variantIsInteger(e)) continue;
                if (!e.canConvert(QMetaType::LongLong)) {
                    r = VC_INVALID;
                    break;
                }
                r = VC_CONVERT;
            }
        }
        // Ha string list, elemek konvertálhatóak?
        else if (t == QMetaType::QStringList) {
            r = VC_CONVERT;
            foreach (QString s, v.toStringList()) {
                bool ok;
                (void)s.toLongLong(&ok);
                if (!ok) {
                    r = VC_INVALID;
                    break;
                }
            }
        }
        // egész skalár, vagy konvertálható skalár, egy elemű tömbként elfogadjuk.
        else if (metaIsInteger(t) || v.canConvert(QMetaType::LongLong)) r = VC_CONVERT;
        break;
    case FT_REAL_ARRAY:
        if (t == QMetaType::QVariantList) {
            r = VC_OK;
            foreach (QVariant e, v.toList()) {
                if (variantIsNum(e)) continue;
                if (!e.canConvert(QMetaType::Double)) {
                    r = VC_INVALID;
                    break;
                }
                r = VC_CONVERT;
            }
        }
        else if (t == QMetaType::QStringList) {
            r = VC_CONVERT;
            foreach (QString s, v.toStringList()) {
                bool ok;
                (void)s.toDouble(&ok);
                if (!ok) {
                    r = VC_INVALID;
                    break;
                }
            }
        }
        else if (metaIsFloat(t) || v.canConvert(QMetaType::Double)) r = VC_CONVERT;
        break;
    case FT_TEXT_ARRAY:
        if      (t == QMetaType::QStringList) r = VC_OK;
        else if (t == QMetaType::QVariantList) {
            r = VC_CONVERT;
            foreach (QVariant e, v.toList()) {
                if (!e.canConvert(QMetaType::QString)) {
                    r = VC_INVALID;
                    break;
                }
            }
        }
        else if (metaIsString(t) || v.canConvert(QMetaType::QString)) r = VC_CONVERT;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    return ifExcep(r, acceptable, v);
}

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
    sl = sqlToStringList(s);
    if (sl.isEmpty() && isNullable) return QVariant();
    return QVariant(sl);
}

QVariant  cColStaticDescrArray::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::trUtf8("Data is NULL"));
    bool    empty = true;
    QString s;
    switch (eColType) {
    case FT_INTEGER_ARRAY: {    // egész tömb
        QVariantList vl = _f.toList();
        s = QChar('{');
        bool ok;
        foreach (const QVariant& vi, vl) {
            s += QString::number(vi.toLongLong(&ok)) + QChar(',');
            if (!ok) EXCEPTION(EDATA,-1,QObject::trUtf8("Invalid number"));
            empty = false;
        }
        if (empty == false) s.chop(1);
        s += QChar('}');
        return QVariant(s);
    }
    case FT_REAL_ARRAY:  {
        QVariantList vl = _f.toList();
        s = QChar('{');
        bool ok;
        foreach (const QVariant& vi, vl) {
            s += QString::number(vi.toDouble(&ok)) + QChar(',');
            if (!ok) EXCEPTION(EDATA,-1,QObject::trUtf8("Invalid number"));
            empty = false;
        }
        if (empty == false) s.chop(1);
        s += QChar('}');
        return QVariant(s);
    }
    case FT_TEXT_ARRAY: {
        QStringList sl = _f.toStringList();
        return stringListToSql(sl);
    }
    default:
        EXCEPTION(EPROGFAIL);
    }
    return QVariant();  // Csak hogy ne legyen warning...
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
QVariant  cColStaticDescrArray::set(const QVariant& _f, qlonglong &str) const
{
    QVariant r =_f;
    if (isNumNull(_f)) r.clear();
    if (r.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
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
            if (!ok) str |= ES_DEFECTIVE;
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
            if (!ok) str |= ES_DEFECTIVE;
        }
        return QVariant(vl);
    case FT_REAL_ARRAY:
        foreach (QString s, _f.toStringList()) {
            vl << QVariant(s.toDouble(&ok));
            if (!ok) str |= ES_DEFECTIVE;
        }
        return QVariant(vl);
    default:
        EXCEPTION(EPROGFAIL);
    }
    switch (eColType) {
    case FT_TEXT_ARRAY:
        sl << QVariantToString(_f, &ok);
        if (!ok) str |= ES_DEFECTIVE;
        return QVariant(sl);
    case FT_INTEGER_ARRAY:
        if (_f.canConvert<qlonglong>()) vl << _f.toLongLong();
        else str |= ES_DEFECTIVE;
        return QVariant(vl);
    case FT_REAL_ARRAY:
        if (_f.canConvert<double>()) vl << _f.toDouble();
        else str |= ES_DEFECTIVE;
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
    if (isNull()) return NULL_ID;
    return _f.toList().size();
}

QString cColStaticDescrArray::toView(QSqlQuery &q, const QVariant &_f) const
{
    if (_f.isNull()) return cColStaticDescr::toView(q,_f);
    QStringList viewList;
    if (eColType == FT_INTEGER_ARRAY && fKeyType != FT_NONE) {
        foreach (QVariant vid, _f.toList()) {
            qlonglong id = vid.toLongLong();
            viewList << fKeyId2name(q, id);
        }
    }
    else {
        viewList = _f.toStringList();
    }
    return viewList.join(QChar(','));
}

/*
static bool getArray(const QString& def, QString& array)
{
    static const QString sBegin("'{");
    static const QString sEnd("}'");
    int n = def.size();
    if (n > 6 && def.indexOf(sBegin) == 0) {
        n = def.indexOf(sEnd, 2);
        if (n >= 2) {
            array = def.mid(1, n);
            return true;
        }
    }
    return false;
}
*/

CDDUPDEF(cColStaticDescrArray)
/* ....................................................................................................... */

cColStaticDescr::eValueCheck  cColStaticDescrEnum::check(const QVariant& v, cColStaticDescr::eValueCheck acceptable) const
{
    eValueCheck r = VC_INVALID;
    if (pEnumType == NULL) EXCEPTION(EPROGFAIL);
    if (v.isNull() || isNumNull(v)) r = checkIfNull();
    else {
        int t = v.userType();
        if (metaIsInteger(t)) {
            if (enumType().checkEnum(v.toLongLong())) return VC_OK;
            r = VC_INVALID;
        }
        else if (v.canConvert(QMetaType::QString) && enumType().check(v.toString())) {
            r = VC_OK;
        }
    }
    return ifExcep(r, acceptable, v);
}

QVariant  cColStaticDescrEnum::fromSql(const QVariant& _f) const
{
    return _f;
}
QVariant  cColStaticDescrEnum::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA, -1, QObject::trUtf8("Data is NULL"));
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
@exception cError Ha az enumType adattag NULL, és szükséges a konverzióhóz.
*/
QVariant  cColStaticDescrEnum::set(const QVariant& _f, qlonglong & str) const
{
    QVariant r =_f;
    bool ok = true;
    if (r.isNull() || isNumNull(r)) {
        ok = isNullable || !colDefault.isEmpty();
        r.clear();
    }
    else if (variantIsInteger(r)) {
        qlonglong i = r.toLongLong();
        r  = enumType().enum2str(i, EX_IGNORE);
        ok = enumType().checkEnum(i);
    }
    else if (!r.convert(QMetaType::QString) || !enumType().check(r.toString())) {
        ok = false;
    }
    if (!ok) {
        DWAR() << QObject::trUtf8("Invalid enum value : ") << debVariantToString(_f) << endl;
        str |= ES_DEFECTIVE;
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
/// Az enumerációk konzisztens kezelése a bool checkEnum(tE2S e2s, tS2E s2e);
/// függvénnyel vagy a CHKENUM makróval ellenőrizhető le.
qlonglong cColStaticDescrEnum::toId(const QVariant& _f) const
{
    QString s = _f.toString();
    if (s.isEmpty()) return NULL_ID;
    qlonglong i = enumType().str2enum(s);
    if (i < 0) EXCEPTION(EDATA, -1, s);
    return i;
}

CDDUPDEF(cColStaticDescrEnum)

/* ....................................................................................................... */

cColStaticDescr::eValueCheck  cColStaticDescrSet::check(const QVariant& v, cColStaticDescr::eValueCheck acceptable) const
{
    eValueCheck r = VC_INVALID;
    if (v.isNull() || isNumNull(v)) r = checkIfNull();
    else {
        int t = v.userType();
        if (metaIsInteger(t)) {
            if (enumType().checkSet(v.toLongLong())) r = VC_OK;
            else r = VC_TRUNC;
        }
        if (metaIsString(t)) {
            if (enumType().check(v.toString())) r = VC_CONVERT;
        }
        if (v.canConvert(QMetaType::QStringList)) {
            if (enumType().check(v.toStringList())) r = VC_OK;
        }
    }
    return ifExcep(r, acceptable, v);
}

QVariant  cColStaticDescrSet::fromSql(const QVariant& _f) const
{
    QString s = _f.toString();
    if (s.isEmpty()) return QVariant(); // NULL
    // Ez a hiba üzenethez kelhet
    QString em = QString("ARRAY %1 = %2").arg(colName()).arg(s);
    // A tömb { ... } zárójelek közt kell legyen
    if (s.size() < 2) EXCEPTION(EDATA, 0, em);  // ???
    if (s.at(0) != QChar('{') || !s.endsWith(QChar('}'))) EXCEPTION(EDBDATA, 1, em);
    s = s.mid(1, s.size() -2);  // Lekapjuk a kapcsos zárójelet
    QStringList sl;
    if (s.isEmpty()) return QVariant(sl);
    // Az elemek közötti szeparátor a vessző, elvileg nincsenek idézőjelek
    sl = s.split(QChar(','),QString::KeepEmptyParts);
    if (sl.isEmpty() && isNullable) return QVariant();
    return QVariant(sl);
}
QVariant  cColStaticDescrSet::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    QString s;
    QStringList sl = _f.toStringList();
    s = QChar('{') + sl.join(QChar(',')) + QChar('}');   // Feltételezzük, hogy nem kell idézőjelbe rakni
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
QVariant  cColStaticDescrSet::set(const QVariant& _f, qlonglong & str) const
{
    // _DBGFN() << debVariantToString(_f) << endl;
    int t = _f.userType();
    if (_f.isNull() || isNumNull(_f)) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
        return QVariant();
    }
    QStringList sl;
    QString s;
    bool ok = true;
    if (metaIsInteger(t)) {
        qlonglong bm = _f.toLongLong();
        sl = enumType().set2lst(bm, EX_IGNORE);
        if (!enumType().checkSet(bm)) str |= ES_DEFECTIVE;
        return QVariant(sl);
    }
    else if (metaIsString(t)) {
        s = _f.toString();
        if (!s.isEmpty()) sl = s.split(QChar(','));
    }
    else if (_f.canConvert(QMetaType::QStringList)) {
        sl = _f.toStringList();
    }
    else {
        str |= ES_DEFECTIVE;
        return QVariant();
    }
    sl = enumType().normalize(sl, &ok);
    if (!ok) str |= ES_DEFECTIVE;
    return QVariant(sl);
 }

QString   cColStaticDescrSet::toName(const QVariant& _f) const
{
    return _f.toStringList().join(QChar(','));
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
    return enumType().lst2set(_f.toStringList(), EX_IGNORE);
}


CDDUPDEF(cColStaticDescrSet)

/* ....................................................................................................... */

cColStaticDescr::eValueCheck  cColStaticDescrPoint::check(const QVariant& _f, cColStaticDescr::eValueCheck acceptable) const
{
    int t = _f.userType();
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else if (t == QVariant::PointF)   r = VC_OK;
    return ifExcep(r, acceptable, _f);
}

QVariant  cColStaticDescrPoint::fromSql(const QVariant& _f) const
{
    if (_f.isNull()) return _f;
    QString s = _f.toString();
    QString em = QObject::trUtf8("Point %1 = %2").arg(colName()).arg(s);
    // Az egésznek zárójelben kell lennie
    if (s.at(0) != QChar('(') || !s.endsWith(QChar(')'))) EXCEPTION(EDBDATA, 1, em);
    s = s.mid(1, s.size() -2);
    QStringList ss = s.split(QChar(','));
    if (ss.size() != 2) EXCEPTION(EDBDATA, 1, em);
    bool okX, okY;
    qreal x, y;
    x = ss.first().simplified().toDouble(&okX);
    y = ss.at(1).simplified().toDouble(&okY);
    if (!okX || !okY) EXCEPTION(EDBDATA, 1, em);
    QPointF    pnt(x,y);
    return QVariant::fromValue(pnt);
}

QVariant  cColStaticDescrPoint::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1, QObject::trUtf8("Data is NULL"));
    int t = _f.userType();
    if (t != QVariant::PointF) {
        EXCEPTION(EDATA, t, debVariantToString(_f));
    }
    QPointF    pnt = _f.value<QPointF>();
    QString     s = QString("(%2,%2)").arg(pnt.x(), pnt.y());
    return QVariant(s);
}

QVariant  cColStaticDescrPoint::set(const QVariant& _f, qlonglong& str) const
{
    // _DBGF() << QChar(' ') << debVariantToString(_f);
    if (_f.isNull() || isNumNull(_f)) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
        return QVariant();
    }
    int t = _f.userType();
    switch (t) {
    case QMetaType::QPoint:
        return QVariant::fromValue(QPointF(_f.value<QPoint>()));
    case QMetaType::QPointF:
        return _f;
    }
    str |= ES_DEFECTIVE;
    return QVariant();

}
QString   cColStaticDescrPoint::toName(const QVariant& _f) const
{
    return QVariantToString(_f);
}
qlonglong cColStaticDescrPoint::toId(const QVariant& _f) const
{
    if (isNull()) return NULL_ID;
    (void)_f;
    return 1;
}

CDDUPDEF(cColStaticDescrPoint)
/* ....................................................................................................... */

cColStaticDescr::eValueCheck  cColStaticDescrPolygon::check(const QVariant& _f, cColStaticDescr::eValueCheck acceptable) const
{
    int t = _f.userType();
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else if (t == _UMTID_tPolygonF)   r = VC_OK;
    return ifExcep(r, acceptable, _f);
}

QVariant  cColStaticDescrPolygon::fromSql(const QVariant& _f) const
{
    if (_f.isNull()) return _f;
    QString s = _f.toString();
    QString em = QObject::trUtf8("Polygon %1 = %2").arg(colName()).arg(s);
    // Az egésznek zárójelben kell lennie
    if (s.at(0) != QChar('(') || !s.endsWith(QChar(')'))) EXCEPTION(EDBDATA, 1, em);
    s = s.mid(1, s.size() -2);
    // PDEB(VVERBOSE) << "Point list : \"" << s << "\"" << endl;
    tPolygonF    pol;
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
    if (_f.isNull()) EXCEPTION(EDATA,-1, QObject::trUtf8("Data is NULL"));
    int t = _f.userType();
    bool    empty = true;
    if (t != _UMTID_tPolygonF) {
        EXCEPTION(EDATA, t, debVariantToString(_f));
    }
    tPolygonF    pol = _f.value<tPolygonF>();
    if (pol.isEmpty()) return QVariant();
    QString     s = QChar('(');
    foreach(const QPointF& pt, pol) {
        s += parentheses(QString::number(pt.x()) + QChar(',') + QString::number(pt.y())) + QChar(',');
        empty = false;
    }
    if (empty == false) s.chop(1);
    s += QChar(')');
    //PDEB(VVERBOSE) << "_toSql() polygon -> string : " << s << endl;
    return QVariant(s);
}

QVariant  cColStaticDescrPolygon::set(const QVariant& _f, qlonglong& str) const
{
    // _DBGF() << QChar(' ') << debVariantToString(_f);
    int t = _f.userType();
    if (_f.isNull() || isNumNull(_f)) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
        return QVariant();
    }
    if (t == _UMTID_tPolygonF) {
        return _f;
    }
/*  tPolygonF   pol;
    switch (t) {
    case QMetaType::QPolygon:
        foreach (QPoint p, _f.value<QPolygon>()) {
            pol << QPointF(p);
        }
        return QVariant::fromValue(pol);
    case QMetaType::QPolygonF:
        pol = _f.value<QPolygonF>();
        PDEB(VVERBOSE) << "Type QPolygonF : size = " << pol.size() << endl;
        return _f;
    case QMetaType::QPoint:
        pol << QPointF(_f.value<QPoint>());
        return QVariant::fromValue(pol);
    case QMetaType::QPointF:
        pol << _f.value<QPointF>();
        return QVariant::fromValue(pol);
    }
*/
    str |= ES_DEFECTIVE;
    return QVariant();

}
QString   cColStaticDescrPolygon::toName(const QVariant& _f) const
{
    return QVariantToString(_f);
}
qlonglong cColStaticDescrPolygon::toId(const QVariant& _f) const
{
    if (isNull()) return NULL_ID;
    return _f.value<tPolygonF>().size();
}

CDDUPDEF(cColStaticDescrPolygon)


/* ....................................................................................................... */

cColStaticDescr::eValueCheck  cColStaticDescrAddr::check(const QVariant& _f, cColStaticDescr::eValueCheck acceptable) const
{
    int mtid = _f.userType();
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else {
        switch (eColType) {
        case FT_MAC:
            if      (mtid == _UMTID_cMac)    r = VC_OK;
            else if (cMac::isValid(_f))      r = VC_CONVERT;
            else if (metaIsInteger(mtid))    r = cMac::isValid(_f.toLongLong()) ? VC_CONVERT : VC_INVALID;
            else if (metaIsString(mtid))     r = cMac::isValid(_f.toString())   ? VC_CONVERT : VC_INVALID;
            break;
        case FT_INET:
        case FT_CIDR:
            if      (mtid == _UMTID_netAddress)      r = VC_OK;
            else if (mtid == _UMTID_QHostAddress)    r = VC_OK;
            else if (metaIsString(mtid)) {
                if (netAddress(_f.toString()).isValid()) r = VC_CONVERT;
            }
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
    }

    return ifExcep(r, acceptable, _f);
}

QVariant  cColStaticDescrAddr::fromSql(const QVariant& _f) const
{
    qlonglong dummy;
    return set(_f, dummy);
}
QVariant  cColStaticDescrAddr::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::trUtf8("Data is NULL"));
    QVariant r = toName(_f);
    if (r.toString().isEmpty()) EXCEPTION(EDATA,-1,QObject::trUtf8("Invalid data type."));
    return r;
}

/// A mező adat letárolása.
/// MAC tíous esetén cMac típusú adat lessz a QVariant értékbe letárolva.
/// Más címek esetén pedig netAddress objektum.
QVariant  cColStaticDescrAddr::set(const QVariant& _f, qlonglong &str) const
{
    int mtid = _f.userType();
    if ((QMetaType::LongLong == mtid && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == mtid && NULL_IX == _f.toInt())
     || _f.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
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
    if (!ok) str |= ES_DEFECTIVE;
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
    if (!_f.isNull()) DERR() << QObject::trUtf8("Invalid data type : %1").arg(_f.typeName()) << endl;
    return QString();
}

qlonglong cColStaticDescrAddr::toId(const QVariant& _f) const
{
    if (isNull()) return NULL_ID;
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
    if (!_f.isNull()) DERR() << QObject::trUtf8("Invalid data type : %1").arg(_f.typeName()) << endl;
    return NULL_ID;
}

CDDUPDEF(cColStaticDescrAddr)

/* ....................................................................................................... */

/// Tárolási adattípus QTime

cColStaticDescr::eValueCheck  cColStaticDescrTime::check(const QVariant& _f, cColStaticDescr::eValueCheck acceptable) const
{
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else {
        int t = _f.userType();
        if      (t == QMetaType::QTime)  r = VC_OK;
        else if (_f.canConvert<QTime>()) r = VC_CONVERT;
        else if (variantIsNum(_f))       r = VC_CONVERT;
    }
    return ifExcep(r, acceptable, _f);
}

QVariant  cColStaticDescrTime::fromSql(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticDescrTime::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::trUtf8("Data is NULL"));
    return _f;
}
QVariant  cColStaticDescrTime::set(const QVariant& _f, qlonglong& str) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (isNumNull(_f) || _f.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
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
    str |= ES_DEFECTIVE;
    return QVariant();
}
QString   cColStaticDescrTime::toName(const QVariant& _f) const
{
    // _DBGF() << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f.toTime().toString("hh:mm:ss.zzz");
}
/// Ezred másodperc értéket ad vissza
qlonglong cColStaticDescrTime::toId(const QVariant& _f) const
{
    // _DBGF() << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (isNull()) return NULL_ID;
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

cColStaticDescr::eValueCheck  cColStaticDescrDate::check(const QVariant& _f, cColStaticDescr::eValueCheck acceptable) const
{
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else {
        int t = _f.userType();
        if      (t == QMetaType::QDate)  r = VC_OK;
        else if (_f.canConvert<QDate>()) r = VC_CONVERT;
        else if (variantIsNum(_f))       r = VC_CONVERT;
    }
    return ifExcep(r, acceptable, _f);
}

QVariant  cColStaticDescrDate::fromSql(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticDescrDate::toSql(const QVariant& _f) const
{
    // _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::trUtf8("Data is NULL"));
    return _f;
}
QVariant  cColStaticDescrDate::set(const QVariant& _f, qlonglong& str) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    int t = _f.userType();
    if (_f.isNull()
     || (QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
        return QVariant();
    }
    QDate   dt;
    if (_f.canConvert<QDate>()) dt = _f.toDate();
    else if (variantIsString(_f)) {
        dt = QDate::fromString(_f.toString());
    }
    if (dt.isValid()) return QVariant(dt);
    DERR() << QObject::trUtf8("Az adat nem értelmezhető  idő ként : ") << endl;
    str |= ES_DEFECTIVE;
    return QVariant();
}
QString   cColStaticDescrDate::toName(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f.toString();
}
qlonglong cColStaticDescrDate::toId(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isValid()) return 0;
    return NULL_ID;
}
CDDUPDEF(cColStaticDescrDate)

/* ....................................................................................................... */

cColStaticDescr::eValueCheck  cColStaticDescrDateTime::check(const QVariant& _f, cColStaticDescr::eValueCheck acceptable) const
{
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else {
        int t = _f.userType();
        if      (t == QMetaType::QDateTime)  r = VC_OK;
        else if (_f.canConvert<QDateTime>()) r = VC_CONVERT;
        else if (variantIsNum(_f))           r = VC_CONVERT;
    }
    return ifExcep(r, acceptable, _f);
}

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
QVariant  cColStaticDescrDateTime::set(const QVariant& _f, qlonglong& str) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    int t = _f.userType();
    if (_f.isNull()
     || (QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())
     || (QMetaType::QDateTime== t && _f.toDateTime().isNull())) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
        return QVariant();
    }
    QDateTime   dt;
    bool ok = false;

    if (_f.canConvert<QDateTime>()) {
        dt = _f.toDateTime();
        ok = !dt.isNull();
    }
    if (!ok && variantIsInteger(_f)) {
        dt = QDateTime::fromTime_t(_f.toUInt(&ok));
    }
    if (!ok && variantIsString(_f)) {
        QString sDt = _f.toString();
        if (sDt.compare(_sNOW, Qt::CaseInsensitive) == 0) return QVariant(sDt);
        dt = QDateTime::fromString(sDt);
    }
    if (ok && dt.isNull()) {
        DERR() << QObject::trUtf8("Az adat nem értelmezhető dátum és idő ként.") << endl;
        str |= ES_DEFECTIVE;
        return QVariant();
    }
    // _DBGFL() << " Return :" << dt.toString() << endl;
    return QVariant(dt);
}

QString   cColStaticDescrDateTime::toName(const QVariant& _f) const
{
    static const QString _tmstmpFrm = "yyyy-MM-dd HH:mm:ss";
 //   _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) return QString();
    if (variantIsString(_f)) return _f.toString();  // NOW
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
///            ha pOk nem NULL. és az s üres, vagy a konverzió nem sikerült, akkor mutatott változót false-ba írja.
/// @return A konvertált érték, vagy NULL_ID ha üres stringet adtunk meg, vagy nem sikerült a kovverzió.
qlonglong parseTimeInterval(const QString& s, bool *pOk)
{
    if (s.isEmpty()) {
        if (pOk != NULL) *pOk = false;
        return NULL_ID;
    }
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

inline static qlonglong __tconvs(qlonglong i, QString& s, int div)
{
    qlonglong j = i % div;
    s.prepend(QString::number(j));
    if (j < 10) s.prepend(QChar('0'));
    return i / div;
}

/// Egy mSec-ben megadott időintervallum stringgé konvertálása
QString intervalToStr(qlonglong i)
{
    QString is;
    if (i == NULL_ID) return is;
    qlonglong j = i % 1000;
    i /= 1000;
    if (j) {
        is = QString("00") + QString::number(j);
        is = QChar('.')  + is.right(3);
    }
    i = __tconvs(i, is, 60);
    is.prepend(QChar(':'));
    i = __tconvs(i, is, 60);
    is.prepend(QChar(':'));
    i = __tconvs(i, is, 24);
    if (i) {
        is = (i == 1 ? "DAY " : "DAYS ") + QString::number(i) + QChar(' ') + is;
    }
    return is;

}

cColStaticDescr::eValueCheck  cColStaticDescrInterval::check(const QVariant& _f, cColStaticDescr::eValueCheck acceptable) const
{
    cColStaticDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else {
        bool ok;
        qlonglong i = _f.toLongLong(&ok);
        if (ok) r = i < 0 ? VC_INVALID : VC_OK;
        else if (variantIsString(_f)) {
            parseTimeInterval(_f.toString(), &ok);
            if (ok) r = VC_OK;
        }
    }
    return ifExcep(r, acceptable, _f);
}

/// Az intervallum qlonglong-ban tárolódik, és mSec-ben értendő.
/// Negatív tartomány nincs értelmezve, és az óra:perc:másodperc formán túl csak a napok megadása támogatott.
QVariant  cColStaticDescrInterval::fromSql(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    bool ok = true;
    QString s = _f.toString();
    if (s.isEmpty()) return QVariant(); // A NULL üres stringként jön.
    qlonglong r = parseTimeInterval(s, &ok);
    if (!ok) EXCEPTION(EPARSE, -1, s);
    return QVariant(r);
}
QVariant  cColStaticDescrInterval::toSql(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::trUtf8("Data is NULL"));
    qlonglong i = variantToId(_f);
    return QVariant(intervalToStr(i));

}
/// Egy időintervallum értéket konvertál, a tárolási tíousra (mSec)
QVariant  cColStaticDescrInterval::set(const QVariant& _f, qlonglong& str) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    int t = _f.userType();
    if (_f.isNull()
     || (QMetaType::LongLong == t && NULL_ID == _f.toLongLong())
     || (QMetaType::Int      == t && NULL_IX == _f.toInt())
     || (metaIsString(t)          && _f.toString().isEmpty())) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
        return QVariant();
    }
    bool ok = false;
    qlonglong r = NULL_ID;
    if      (variantIsFloat(_f))   r = (qlonglong)(_f.toDouble(&ok) + 0.5);
    else if (variantIsString(_f))  r = parseTimeInterval(_f.toString(), &ok);
    else if (variantIsInteger(_f)) r = _f.toLongLong(&ok);
    if (!ok) str |= ES_DEFECTIVE;
    return r == NULL_ID || !ok ? QVariant() : QVariant(r);
}
QString   cColStaticDescrInterval::toName(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isValid()) return intervalToStr(variantToId(_f));
    return _sNul;
}
qlonglong cColStaticDescrInterval::toId(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return variantToId(_f, EX_IGNORE, NULL_ID);
}

CDDUPDEF(cColStaticDescrInterval)

/* ******************************************************************************************************* */

cColStaticDescrList::cColStaticDescrList(cRecStaticDescr *par)
    : QList<cColStaticDescr *>()
{
    pParent = par;
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
    if (size() != __o->pos) EXCEPTION(EDBDATA,0,QString("Table : %1, pos = %2, and list síze = %3, is not equal.").arg(tableName()).arg(__o->pos).arg(size()));
    return *this;
}

int cColStaticDescrList::toIndex(const QString& __n, eEx __ex) const
{
    if (__n.isEmpty()) {
        static QString msg = QObject::trUtf8("Empty field name, table %1").arg(tableName());
        if (__ex) EXCEPTION(EDATA,-1, msg);
        DERR() << msg << endl;
        return NULL_IX;
    }
    const_iterator  i;
    for (i = constBegin(); i != constEnd(); i++) if (**i == __n) return (*i)->pos -1;
    if (__ex) EXCEPTION(ENOFIELD, -1, fullColName(__n));
    return NULL_IX;
}


cColStaticDescr& cColStaticDescrList::operator[](const QString& __n)
{
    int i = toIndex(__n);
    if (i < 0 || i > size() -1) EXCEPTION(EPROGFAIL, i, fullColName(__n));
    return (*this)[i];
}

const cColStaticDescr& cColStaticDescrList::operator[](const QString& __n) const
{
    int i = toIndex(__n);
    if (i < 0 || i > size() -1) EXCEPTION(EPROGFAIL, i, fullColName(__n));
    return (*this)[i];
}

cColStaticDescr& cColStaticDescrList::operator[](int i)
{
    if (i < 0 || i >= size()) EXCEPTION(ENOINDEX, i, tableName());
    return *(*(list *)this)[i];
}

const cColStaticDescr& cColStaticDescrList::operator[](int i) const
{
    if (i < 0 || i >= size()) EXCEPTION(ENOINDEX, i, tableName());
    return *(*(const list *)this)[i];
}

QString cColStaticDescrList::fullColName(const QString& _col) const {
    if (pParent == NULL) return mCat(_sNULL, _col);
    return mCat(pParent->tableName(), _col);
}
QString cColStaticDescrList::tableName() const {
    if (pParent == NULL) return _sNULL;
    return pParent->tableName();
}

/* ******************************************************************************************************* */

QMap<qlonglong, cRecStaticDescr *>   cRecStaticDescr::_recDescrMap;
QMutex                               cRecStaticDescr::_mapMutex;

cRecStaticDescr::cRecStaticDescr(const QString &__t, const QString &__s)
    : _schemaName()
    , _tableName()
    , _viewName()
    , _tableRecord()
    , _columnDescrs(this)
    , _primaryKeyMask()
    , _nameKeyMask()
    , _uniqueMasks()
    , _autoIncrement()
    , _parents()
{
    _tableType      = UNKNOWN_TABLE;
    _columnsNum     = 0;
    _tableOId       = _schemaOId = NULL_ID;
    _idIndex        = _nameIndex = _noteIndex = _deletedIndex = _flagIndex = _textIdIndex = NULL_IX;
    _isUpdatable    = false;
    _set(__t,__s);  // Setting by database
    addMap();
    // Only now can we get it right, or maybe we'll call id-> name conversion functions
    int i, n = cols();
    QSqlQuery *pq = NULL;
    for (i = 0; i < n; ++i) {
        const cColStaticDescr& cd = colDescr(i);
        if (cd.fKeyType != cColStaticDescr::FT_NONE     // If is foreign key
         && cd.fKeyType != cColStaticDescr::FT_TEXT_ID  // but not a language text
         && cd.fnToName.isEmpty()) {                    // and there is no conversion function yet
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
    // _DBGF() << _tableName << endl;
    _mapMutex.lock();
    if (_recDescrMap.contains(_tableOId)) {
        _mapMutex.unlock();
        DWAR() << QObject::trUtf8("Remap static descriptor : %1").arg(toString()) << endl;
        return false;
    }
    _recDescrMap[_tableOId] = this;
    _mapMutex.unlock();
    // _DBGFL() << _sOk << endl;
    return true;
}

int cRecStaticDescr::_setReCallCnt = 0;

void cRecStaticDescr::_set(const QString& __t, const QString& __s)
{
    _DBGFN() << " " << dQuotedCat(__s, __t) << endl;
    // _DBGF() << " @(" << __t << _sCommaSp << __s << QChar(')') << endl << dec; // Valahol valaki hex-re állítja :(
    if (_setReCallCnt) DWAR() << QObject::trUtf8("******** RECURSION #%1 ********").arg(_setReCallCnt) << endl;
    ++_setReCallCnt;
    // Set table and schema name
    _schemaName     = __s.isEmpty() ? _sPublic : __s;
    _tableName      = __t;
    _viewName       = __t;

    QString baseName = __t;
    if (baseName.endsWith('s')) {
        baseName.chop(1);
        // s-el végződő bázis név esetén az 's' utótag helyett 'es' utótag is lehet.
        if (baseName.endsWith("se")) baseName.chop(1);
    }

    QSqlQuery   *pq  = newQuery();
    QSqlQuery   *pq2 = newQuery();
    QString sql;
    // *********************** set scheam and Table  OID
    _schemaOId = ::schemaoid(*pq, __s);
    _tableOId  = ::tableoid(*pq, __t, _schemaOId);
    // PDEB(INFO) << QObject::trUtf8("Table %1 tableoid : %2").arg(fullTableName()).arg(_tableOId) << endl;

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
    if (!pq->first()) EXCEPTION(EDBDATA, 0, QObject::trUtf8("Table %1.%2 not found.").arg(_schemaName, _tableName));
    _tableRecord = pq->record();
    if (pq->next()) EXCEPTION(EDBDATA, 0, QObject::trUtf8("Table : %1,%2.").arg(_schemaName, _tableName));

    QString n = _tableRecord.value("table_type").toString();
    if      (n == "BASE TABLE")
        _tableType = TT_BASE_TABLE;
    else if (n == "VIEW") {
        _tableType = TT_VIEW_TABLE | TT_BASE_TABLE;
        // Ha egy link tábláról van szó, akkor annak itt a view tábláját kell megadni, és ebben az estenben
        // a tábla név az a view név kiegészítve a "_table" utótaggal.
        QString tn = _viewName + "_table";
        qlonglong toid = ::tableoid(*pq, tn, _schemaOId, EX_IGNORE);
        if (NULL_ID != toid) {
            // PDEB(INFO) << "Table " << _viewName << QChar('/') << _tableName << " table type : LINK_TABLE." << endl;
            _tableName = tn;
            _tableOId  = toid;
            _tableType = TT_LINK_TABLE;
        }
    }
    else EXCEPTION(EDBDATA, 0, QObject::trUtf8("Invalid table type %1.%2 : %3").arg(_schemaName, _tableName, n));
    // PDEB(INFO) << QObject::trUtf8("%1 table is %2").arg(fullTableName()).arg(n) << endl;

    // ************************ get columns records
    sql = "SELECT * FROM information_schema.columns WHERE table_schema = ? AND table_name = ? ORDER BY ordinal_position";
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    pq->bindValue(0, _schemaName);
    pq->bindValue(1, _tableName);
    if (!pq->exec()) SQLQUERYERR(*pq);
    if (!pq->first()) EXCEPTION(EDBDATA, 0, QObject::trUtf8("Table columns %1,%2 not found.").arg(_schemaName).arg(_tableName));
    _columnsNum = pq->size();
    if (_columnsNum < 1) EXCEPTION(EPROGFAIL, _columnsNum, "Invalid size.");
    _autoIncrement.resize(_columnsNum);
    _primaryKeyMask.resize(_columnsNum);
    // PDEB(VVERBOSE) << VDEB(_columnsNum) << endl;
    int i = 0;
    do {
        ++i;    // Vigyázz, fizikus index! (1,2,...)
        cColStaticDescr columnDescr(this);
        columnDescr.colName() = pq->record().value("column_name").toString();
        columnDescr.pos       = i;
        columnDescr.ordPos    = pq->record().value("ordinal_position").toInt();
        columnDescr.colDefault= pq->record().value("column_default").toString();
        // PDEB(VVERBOSE) << "colDefault : " <<  (columnDescr.colDefault.isNull() ? "NULL" : dQuoted(columnDescr.colDefault)) << endl;
        columnDescr.colType   = pq->record().value("data_type").toString();
        columnDescr.udtName   = pq->record().value("udt_name").toString();
        QVariant cml = pq->record().value("character_maximum_length");
        columnDescr.chrMaxLenghr = cml.canConvert(QVariant::Int) ? cml.toInt() : -1;
        columnDescr.isNullable= pq->record().value("is_nullable").toString() == QString("YES");
        columnDescr.isUpdatable=pq->record().value("is_updatable").toString() == QString("YES");
        _isUpdatable = _isUpdatable || columnDescr.isUpdatable;
        // PDEB(INFO) << fullTableName() << " field #" << i << QChar('/') << columnDescr.ordPos << " : " << columnDescr.toString() << endl;
        // Is auto increment ?
        _autoIncrement.setBit(i -1, columnDescr.colDefault.indexOf("nextval('") == 0);
        // Megnézzük enum-e
        if (columnDescr.colType ==  _sUSER_DEFINED || columnDescr.colType ==  _sARRAY) {
            if (columnDescr.colType ==  _sARRAY && columnDescr.udtName.startsWith(QChar('_'))) {
                // Az ARRAY-nál a típus névhez, hozzá szokott bigyeszteni egy '_'-karakter (nem mindíg)
                columnDescr.udtName = columnDescr.udtName.mid(1);   // Ha a saját típust keressük, az '_'-t törölni kell.
            }
            columnDescr.pEnumType = cColEnumType::fetchOrGet(*pq2, columnDescr.udtName, EX_IGNORE);
        }
        // Deleted mező?
        if (columnDescr.colName() == _sDeleted) {
            if (columnDescr.colType == "boolean") {
                if (_deletedIndex >= 0) EXCEPTION(EPROGFAIL);
                _deletedIndex = i -1;
                // PDEB(VVERBOSE) << QObject::trUtf8("deleted field is found, index : %1").arg(_deletedIndex) << endl;
            }
            else {
                DWAR() << QObject::trUtf8("deleted field is found, but type is not boolean, index : %1, type : %2").arg(i -1).arg(columnDescr.colType) << endl;
            }
        }
        if (columnDescr.colName() == _sFlag) {
            if (columnDescr.colType == "boolean") {
                if (_flagIndex >= 0) EXCEPTION(EPROGFAIL);
                _flagIndex = i -1;
                // PDEB(VVERBOSE) << QObject::trUtf8("deleted field is found, index : %1").arg(_flagIndex) << endl;
            }
            else {
                DWAR() << QObject::trUtf8("flag field is found, but type is not boolean, index : %1, type : %2").arg(i -1).arg(columnDescr.colType) << endl;
            }
        }
        // Column type
        columnDescr.typeDetect();
        // Name or note : fix column name
        // Or determined column index and sufix
        if (columnDescr.eColType == cColStaticDescr::FT_TEXT) {
            #define NAME_INDEX  1
            #define DESCR_INDEX 2
            static QString nameSufix = "_name";
            static QString noteSufix = "_note";
            if (!baseName.isEmpty()) {
                if (_nameIndex < 0 && 0 == columnDescr.colName().compare(baseName + nameSufix)) _nameIndex = i -1;
                if (_noteIndex < 0 && 0 == columnDescr.colName().compare(baseName + noteSufix)) _noteIndex = i -1;
            }
            if (_nameIndex  < 0 && i == (NAME_INDEX  +1) && columnDescr.colName().endsWith(nameSufix)) _nameIndex = NAME_INDEX;
            if (_noteIndex  < 0 && i == (DESCR_INDEX +1) && columnDescr.colName().endsWith(noteSufix)) _noteIndex = DESCR_INDEX;
        }
        // Detect foreign keys. They are not regular too.
        if (columnDescr.eColType == cColStaticDescr::FT_INTEGER
         && columnDescr.colName() == _sTextId && __t != __sLocalizations) { // text_id? (--> Localization)
            columnDescr.fKeyType = cColStaticDescr::FT_TEXT_ID;
            sql = "SELECT column_name FROM fkey_types WHERE table_schema = ? AND table_name = ? AND unusual_fkeys_type = 'text'";
            execSql(*pq2, sql, _schemaName, _tableName);
            QString t = pq2->value(0).toString();
            columnDescr.pEnumType = cColEnumType::fetchOrGet(*pq2, t);
            _textIdIndex = i - 1;
        }
        else if (columnDescr.eColType == cColStaticDescr::FT_INTEGER        // Feltételezzük, hogy távoli kulcs csak egész szám (ID) lehet
         || columnDescr.eColType == cColStaticDescr::FT_INTEGER_ARRAY) {    // Vannak tömbjeink is, ami kulcs, és a pg nem támogatja
            bool regularFKey = false;
            // Regular foreign keys
            if (columnDescr.eColType == cColStaticDescr::FT_INTEGER) {
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
                regularFKey = pq2->first();
            }
            if (regularFKey) { // Ha ő egy szabályos/támogatott távoli kulcs, hova:
                columnDescr.fKeySchema= pq2->value(0).toString();
                columnDescr.fKeyTable = pq2->value(1).toString();
                columnDescr.fKeyField = pq2->value(2).toString();
                QString t = pq2->value(3).toString();
                if (t.isEmpty()) {   // Nincs megadva típus a távoli kulcsra (nincs fkey_types rekord)
                    if (_tableName == columnDescr.fKeyTable && _schemaName == columnDescr.fKeySchema)
                        columnDescr.fKeyType = cColStaticDescr::FT_SELF;
                    else
                        columnDescr.fKeyType = cColStaticDescr::FT_PROPERTY;
                    // FT_OWNER típus esetén kötelező definiáni a típust egy fkey_types vagy unusual_fkeys rekorddal !
                }
                else if (!t.compare(_sProperty, Qt::CaseInsensitive)) {
                    columnDescr.fKeyType = cColStaticDescr::FT_PROPERTY;
                }
                else if (!t.compare(_sOwner,    Qt::CaseInsensitive)) {
                    columnDescr.fKeyType = cColStaticDescr::FT_OWNER;
                    if ((_tableType & TT_MASK) != TT_BASE_TABLE) EXCEPTION(EDBDATA, _tableType, "Table type conflict.");
                    _tableType &= ~TT_MASK;
                    _tableType |= TT_CHILD_TABLE;
                }
                else if (!t.compare(_sSelf,     Qt::CaseInsensitive)) {
                    if (_tableName != columnDescr.fKeyTable || _schemaName != columnDescr.fKeySchema) EXCEPTION(EDBDATA);
                    columnDescr.fKeyType = cColStaticDescr::FT_SELF;
                }
                else EXCEPTION(EDATA, -1, t);   // There is also 'text' type, but there is no point here
                columnDescr.fKeyTables << columnDescr.fKeyTable;
                // columnDescr.fnToName = pq2->value(4).toString();
            }
            // Irregular foreign keys (inheritance or/and array)
            else  {
                sql = QString(
                            "SELECT f_table_schema, f_table_name, f_column_name, unusual_fkeys_type, f_inherited_tables"
                            "    FROM unusual_fkeys"
                            "    WHERE table_schema = '%1' AND table_name = '%2' AND column_name = '%3'"
                            ).arg(_schemaName, _tableName, columnDescr.colName());
                if (!pq2->exec(sql)) SQLPREPERR(*pq2, sql);
                if (pq2->first()) { // This is irregular foreign key
                    columnDescr.fKeySchema = pq2->value(0).toString();
                    columnDescr.fKeyTable  = pq2->value(1).toString();
                    columnDescr.fKeyField  = pq2->value(2).toString();
                    columnDescr.fKeyTables = stringArrayFromSql(pq2->value(4));
                    QString t = pq2->value(3).toString();
                    if      (!t.compare(_sProperty, Qt::CaseInsensitive)) {
                        columnDescr.fKeyType = cColStaticDescr::FT_PROPERTY;
                    }
                    else if (!t.compare(_sOwner,    Qt::CaseInsensitive)) {
                        if ((_tableType & TT_MASK) == TT_BASE_TABLE) {
                            _tableType &= ~TT_MASK;
                            _tableType |= TT_CHILD_TABLE;
                        }
                        else if ((_tableType & TT_MASK) == TT_LINK_TABLE) {
                            ;   // marad LINK
                        }
                        else {
                            EXCEPTION(EDBDATA, _tableType, "Table type conflict.");
                        }
                        columnDescr.fKeyType = cColStaticDescr::FT_OWNER;
                    }
                    else if (!t.compare(_sSelf,     Qt::CaseInsensitive)) {
                        columnDescr.fKeyType = cColStaticDescr::FT_SELF;
                    }
                    else EXCEPTION(EDATA, -1, t);
                }
            }
            if (columnDescr.fKeyType != cColStaticDescr::FT_NONE) {
                // Az id -> name konverziós függvény : (csak ha már megvan a cél tábla descriptora)
                columnDescr.fnToName = checkId2Name(*pq2, columnDescr.fKeySchema, columnDescr.fnToName, EX_IGNORE);
            }
        }
        // END Regular and irregular foreign keys detect
        // A konveziós függvények miatt a megfelelő típusu mező leíró objektumot kell a konténerbe rakni.
        switch (columnDescr.eColType) {
        case cColStaticDescr::FT_DATE:          _columnDescrs << new cColStaticDescrDate(columnDescr);      break;
        case cColStaticDescr::FT_DATE_TIME:     _columnDescrs << new cColStaticDescrDateTime(columnDescr);  break;
        case cColStaticDescr::FT_TIME:          _columnDescrs << new cColStaticDescrTime(columnDescr);      break;
        case cColStaticDescr::FT_INTERVAL:      _columnDescrs << new cColStaticDescrInterval(columnDescr);  break;

        case cColStaticDescr::FT_BOOLEAN:       _columnDescrs << new cColStaticDescrBool(columnDescr);      break;
        case cColStaticDescr::FT_POINT:         _columnDescrs << new cColStaticDescrPoint(columnDescr);     break;
        case cColStaticDescr::FT_POLYGON:       _columnDescrs << new cColStaticDescrPolygon(columnDescr);   break;

        case cColStaticDescr::FT_MAC:
        case cColStaticDescr::FT_INET:
        case cColStaticDescr::FT_CIDR:          _columnDescrs << new cColStaticDescrAddr(columnDescr);      break;

        case cColStaticDescr::FT_ENUM:          _columnDescrs << new cColStaticDescrEnum(columnDescr);      break;
        case cColStaticDescr::FT_SET:           _columnDescrs << new cColStaticDescrSet(columnDescr);       break;

        default:
            if (columnDescr.eColType & cColStaticDescr::FT_ARRAY) _columnDescrs << new cColStaticDescrArray(columnDescr);
            else                                                  _columnDescrs << columnDescr; // alap typus, new and copy
            break;
        }
        // cColStaticDescr *pp = ((cColStaticDescrList::list)_columnDescrs)[i -1];
        // PDEB(VERBOSE) << QObject::trUtf8("Field %1 type is %2").arg(pp->colName()).arg(typeid(*pp).name()) << endl;
    } while(pq->next());
    if (_columnsNum != i) EXCEPTION(EPROGFAIL, -1, "Nem egyértelmű mező szám");
    if ((_tableType & TT_VIEW_TABLE) == 0) {    // Nézettáblánál nincsenek explicit kulcsok
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
            // CONSTRAINT név /_uniqueMasks index
            QMap<QString, int>  map;
            do {
                QString constraintName = pq->record().value("constraint_name").toString();
                QString columnName     = pq->record().value("column_name").toString();
                QString constraintType = pq->record().value("constraint_type").toString();
                i = toIndex(columnName, EX_IGNORE);
                if (i < 0) EXCEPTION(EDBDATA,0, QObject::trUtf8("Invalid column name : %1").arg(fullColumnName(columnName)));
                if     (constraintType == "PRIMARY KEY") {
                    //PDEB(VVERBOSE) << "Set _primaryKeyMask bit, index = " << i << endl;
                    _primaryKeyMask.setBit(i);
                    if (columnName.endsWith(QString("_id"))) _idIndex = i;
                }
                else if(constraintType == "UNIQUE") {
                    // A map-ban van ilyen nevű CONSTRAINT (UNIQUE KEY név) ?
                    QMap<QString, int>::iterator    it = map.find(constraintName);
                    int j;
                    if (it == map.end()) {      // Nincs, új bitmap
                        j = _uniqueMasks.size();    // új maszk indexe
                        //PDEB(VVERBOSE) << "Insert #" << j << " bit vector to _uniqueMasks ..." << endl;
                        map.insert(constraintName, j);
                        _uniqueMasks << QBitArray(_columnsNum);
                    }
                    else j = it.value();            // A talált maszk indexe
                    //PDEB(VVERBOSE) << "Set _uniqueMasks[" << j << "] bit #" << i << " to true..." << endl;
                    _uniqueMasks[j].setBit(i);
                    // if (_nameIndex < 0 && columnName.endsWith(QString("_name"))) _nameIndex = i;
                }
            } while(pq->next());
        }
        // _nameKeyMask kitöltése (mivel együtt egyedi a név mező ?)
        if (_nameIndex >= 0) {
            foreach (QBitArray u, _uniqueMasks) {
                if (u[_nameIndex]) {
                    if (_nameKeyMask.count(true)) {
                        _nameKeyMask.fill(false);       // kétértelmüség
                        PDEB(VERBOSE) << "_nameKeyMask is ambiguos, unset." << endl;
                        break;
                    }
                    _nameKeyMask = u;
                    // PDEB(VERBOSE) << "_nameKeyMask set : " << list2string(u) << endl;
                }
            }
        }
        // else {
        //     PDEB(VERBOSE) << "No name field, _nameKeyMask is not set." << endl;
        // }
        // Ha találtunk ID-t, akkor az csak egyedüli egyedi kulcs lehet!
        if (_primaryKeyMask.count(true) != 1) _idIndex = NULL_IX;
        // Ha van ID-nk, akkor az elsődleges kulcs kell legyen
        if (_idIndex != NULL_IX && !_primaryKeyMask[_idIndex]) EXCEPTION(EDATA, _idIndex, fullColumnName(_idIndex));
        //PDEB(VERBOSE) << VDEB(_idIndex) << " ; " << VDEB(_nameIndex) << endl;
        //
        if (_tableType == TT_BASE_TABLE && _nameIndex < 0           // Típus még nem derült ki, és nincs neve
         && colDescr(1).fKeyType == cColStaticDescr::FT_PROPERTY    // A második mező egy távoli kulcs
         && colDescr(2).fKeyType == cColStaticDescr::FT_PROPERTY) { // és a harmadik mező is.
            _tableType &= ~TT_MASK;
            _tableType |= TT_SWITCH_TABLE;
        }
    }
    else {  // Nézet tábla...
        // Nincs PRIMARY KEY, de nekünk kellhet egy egyedi azonosító ID
        // Feltételezzük, hogy ha van akkor az az első mező, az ID mezőnév konvenció szerint:
        QString n = _tableName;
        n.chop(1);
        QString fn = n + "_id";
        if (colDescr(0).eColType == cColStaticDescr::FT_INTEGER && columnName(0) == fn) {
            _primaryKeyMask = _mask(_columnsNum, 0);
            _idIndex        = 0;
        }
        // Hasonlóan feltételezzők a név mező pozicióját (második) és nevét, ha van
        fn = n + "_name";
        if (colDescr(1).eColType == cColStaticDescr::FT_TEXT && columnName(1) == fn) {
            _nameIndex      = 1;
        }
    }
    sql = "SELECT field_name FROM field_attrs WHERE table_name = ? AND 'rewrite_protected' = ANY (attrs)";
    if (execSql(*pq, sql, QVariant(_tableName))) {
        _protectedForRewriting = QBitArray(cols());
        do {
            QString n = pq->value(0).toString();
            int ix = toIndex(n, EX_IGNORE);
            if (ix < 0) {
                EXCEPTION(EDATA, 0, QObject::trUtf8("Hibás mező név a field_attrs táblában : %1").arg(n));
            }
            _protectedForRewriting[ix] = true;
        } while (pq->next());
    }
    delete pq;
    delete pq2;
    // ************************ init O.K
    _setReCallCnt--;
  // DBGFL();
}

int cRecStaticDescr::toIndex(const QString& __n, eEx __ex) const
{
    // _DBGFN() << " @(" << __n << "): " << fullTableName() << endl;
    // Esetleges schema vagy tábla név leválasztása
    QStringList m = __n.split(QChar('.'));
    // csak három elem lehet: schema.table.field
    if (m.size() > 3) {
        if (__ex) EXCEPTION(EDATA, -1, QObject::trUtf8("Invalid field name %1").arg(__n));
        DERR() << "Invalid index : -1 / To many field in name"  << endl;
        return NULL_IX;
    }
    // Ha megvan adva schema név
    if (m.size() == 3) {
        QString sn = _schemaName;
        if (sn == unQuoted(m.first())) {
            m.pop_front();  // Schema név ok, eldobjuk
        }
        else{
            if (__ex) EXCEPTION(EDATA, -1, QObject::trUtf8("Invalid field name (other schema name) %1").arg(__n));
            DERR() << "Invalid index : -1 / mismatch shema : my schema is " << schemaName() << endl;
            return NULL_IX; // Másik schemarol van szó
        }
    }
    // Ha megvan adva tábla név
    if (m.size() == 2) {
        if (_tableName == unQuoted(m.first())
         || (_viewName.size() > 0 &&_viewName  == unQuoted(m.first()))) {
            m.pop_front();  // Tábla név ok, eldobjuk
        }
        else {
            if (__ex) EXCEPTION(EDATA, -1, QObject::trUtf8("Invalid field name (other table name) ") + __n);
            DERR() << QObject::trUtf8("Invalid index : -1 / mismatch table : my table is %1").arg(tableName()) << endl;
            return NULL_IX; // Másik tábláról van szó
        }
    }
    // maradt a mező név, keressük a listűban
    QString name = unQuoted(m.first());
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
        // PDEB(VERBOSE) << QObject::trUtf8("Found rdescr %1").arg(p->tableName()) << endl;
        return *i;
    }
    QSqlQuery *pq = newQuery();
    tStringPair tsn = tableoid2name(*pq, _oid);
    delete pq;
    _mapMutex.unlock();
    // Ha nem találjuk, létre kell hozni ?
    if (find_only) {    // Nem. Csak kerestünk
        // PDEB(VVERBOSE) << QObject::trUtf8("Not found rdescr %1.%2").arg(tsn.first).arg(tsn.second) << endl;
        return NULL;
    }
    if (_setReCallCnt >= 10) EXCEPTION(EPROGFAIL);
    // PDEB(VERBOSE) << QObject::trUtf8("Init rdescr obj. %1.%2").arg(tsn.first).arg(tsn.second) << endl;
    p = new cRecStaticDescr(tsn.first, tsn.second);
    return p;
}

const cRecStaticDescr *cRecStaticDescr::get(const QString& _t, const QString& _s, bool find_only)
{
    // _DBGF() << _t << _sCommaSp << _s << endl;
    QSqlQuery *pq = newQuery();
    eEx ex = find_only ? EX_IGNORE : EX_ERROR;
    qlonglong tableOId = ::tableoid(*pq, _t, schemaoid(*pq, _s), ex);
    delete pq;
    return tableOId == NULL_ID ? NULL : get(tableOId, find_only);
}

bool cRecStaticDescr::isKey(int i) const
{
    chkIndex(i);
    foreach (QBitArray a, uniques()) {
        if (a[i]) return true;
    }
    return false;
}


QString cRecStaticDescr::checkId2Name(QSqlQuery& q) const
{
    const QString& sIdName     = idName();
    const QString& sSchemaName = schemaName();
    // Az id -> name konverziós függvény :
    QString sFnToName = sIdName + "2name"; // Az alapértelmezett név
    // Csak a nevet ellenőrizzük, ilyen neve ne legyen més függvénynek !!
    QString sql = QString("SELECT COUNT(*) FROM  information_schema.routines WHERE routine_schema = '%1' AND routine_name = '%2'")
            .arg(sSchemaName).arg(sFnToName);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (!q.first() || q.value(0).toInt() == 1) return sFnToName;    // Definiálva van a keresett függvény, OK
    const QString& sNameName   = nameName(EX_IGNORE);
    const QString& sTableName  = tableName();
    if (sNameName.isEmpty()) {
        DWAR() << QObject::trUtf8("Function %1 not found, nothing create.").arg(sFnToName) << endl;
        return _sNul;
    }
    DWAR() << QObject::trUtf8("Function %1 not found, create ...").arg(sFnToName) << endl;
    sql = QString(
         "CREATE OR REPLACE FUNCTION %1(bigint) RETURNS text AS $$"
        " DECLARE"
            " name text;"
        " BEGIN"
            " IF $1 IS NULL THEN"
                " RETURN NULL; "
            " END IF;"
            " SELECT %4 INTO name FROM %2 WHERE %3 = $1;"
            " IF NOT FOUND THEN"
                " PERFORM error('IdNotFound', $1, '%3', '%1', '%2');"
            " END IF;"
            " RETURN name;"
        " END"
        " $$ LANGUAGE plpgsql"
            ).arg(sFnToName).arg(sTableName).arg(sIdName).arg(sNameName);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    // PDEB(INFO) << QObject::trUtf8("Created procedure : %1").arg(quotedString(sql)) << endl;
    return sFnToName;

}

QString cRecStaticDescr::checkId2Name(QSqlQuery& q, const QString& _tn, const QString& _sn, eEx __ex)
{
    const cRecStaticDescr *pDescr = get(_tn, _sn, __ex == EX_IGNORE);
    if (pDescr == NULL) {
        if (__ex) EXCEPTION(EPROGFAIL); // Lehetetlen
        return QString();
    }
    return pDescr->checkId2Name(q);
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

QStringList cRecStaticDescr::columnNames(QBitArray mask) const
{
    QStringList r;
    for (int i = 0; i < mask.size(); ++i) {
        if (mask[i]) r << columnName(i);
    }
    return r;
}

QStringList cRecStaticDescr::columnNames(tIntVector ixs) const
{
    QStringList r;
    foreach (int i, ixs) {
        r << columnName(i);
    }
    return r;
}


qlonglong cRecStaticDescr::getIdByName(QSqlQuery& __q, const QString& __n, eEx __ex) const
{
    QString sql = QString("SELECT %1 FROM %2 WHERE %3 = '%4'")
                  .arg(dQuoted(idName()), fullTableNameQ(), dQuoted(nameName()), __n);
    // PDEB(VERBOSE) << __PRETTY_FUNCTION__ << " SQL : " << sql << endl;
    EXECSQL(__q, sql);
    if (!__q.first()) {
        QString msg = QObject::trUtf8("A %1 táblában nincs %2 nevű rekord (ahol a név mező %3)")
                .arg(fullTableNameQ())
                .arg(dQuoted(__n))
                .arg(dQuoted(nameName()));
        if (__ex) EXCEPTION(EFOUND, 1, msg);
        DWAR() << msg << endl;
        return NULL_ID;
    }
    QVariant id = __q.value(0);
    if (__q.next()) {
        QString msg = QObject::trUtf8("A %1 táblában több %2 nevű rekord is van (ahol a név mező %3)")
                .arg(fullTableNameQ())
                .arg(dQuoted(__n))
                .arg(dQuoted(nameName()));
        if (__ex) EXCEPTION(AMBIGUOUS, 2, msg);
        DWAR() << msg << endl;
        return NULL_ID;
    }
    return variantToId(id, __ex, NULL_ID);
}

QString cRecStaticDescr::getNameById(QSqlQuery& __q, qlonglong __id, eEx ex) const
{
    QString sql = QString("SELECT %1 FROM %2 WHERE %3 = %4")
            .arg(dQuoted(nameName()), fullTableNameQ(), dQuoted(idName())). arg(__id);
    EXECSQL(__q, sql);
    if (!__q.first()) {
        if (ex) EXCEPTION(EFOUND, __id, fullTableNameQ());
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

QBitArray cRecStaticDescr::mask(const QStringList& __nl, eEx __ex) const
{
    QBitArray r(cols(), false);
    foreach (QString fn, __nl) {
        int ix = toIndex(fn, __ex);
        if (ix < 0) continue;
        r[ix] = 1;
    }
    return r;
}
QBitArray cRecStaticDescr::inverseMask(const QBitArray& __m) const
{
    QBitArray r = __m;
    r.resize(cols());
    return ~r;
}


QString cRecStaticDescr::toString() const
{
    QString s = "TABLE " + fullTableNameQ() + QChar(' ') + QChar('(') + '\n';
    cColStaticDescrList::ConstIterator i, n = columnDescrs().constEnd();
    for (i = columnDescrs().constBegin(); i != n; ++i) {
        const cColStaticDescr& fd = **i;
        s += QChar(' ') + fd.toString();
        if (_primaryKeyMask.count(true) == 1 && _primaryKeyMask[fd.pos -1]) {
            s += " PRIMARY KEY";
        }
        else {
            foreach (const QBitArray& um,  _uniqueMasks) {
                if (um.count(true) == 1 && um[fd.pos -1]) {
                    s += " UNIQUE";
                    break;
                }
            }
        }
        s += QChar(',');
        if (fd.fKeyType != cColStaticDescr::FT_NONE) {
            s += " -- Foreign key to " + fd.fKeySchema + QChar('.') + fd.fKeyTable + QChar('.') + fd.fKeyField + QChar('/');
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
        s += " PRIMARY KEY (";
        for (int i = 0; i < _columnsNum; i++) if (_primaryKeyMask[i]) {
            s += QChar(' ') + dQuoted(columnDescrs()[i].colName()) + QChar(',');
        }
        s += ")\n";
    }
    for (int i = 0; i < _uniqueMasks.size(); i++) if (_uniqueMasks[i].count(true) > 1) {
        s += " UNIQUE (";
        for (int j = 0; j < _uniqueMasks[i].size(); j++) if (_uniqueMasks[i][j]) {
            s += QChar(' ') + dQuoted(columnDescrs()[j].colName()) + QChar(',');
        }
        s += QChar(')') + '\n';
    }
    if (_idIndex >= 0)      s += "-- _idIndex   = "     + QString::number(_idIndex)   + '\n';
    if (_nameIndex >= 0)    s += "-- _nameIndex = "     + QString::number(_nameIndex) + '\n';
    if (_noteIndex >= 0)    s += "-- _noteIndex = "     + QString::number(_noteIndex) + '\n';
    if (_deletedIndex >= 0) s += "-- _deletedIndex = "  + QString::number(_deletedIndex) + '\n';
    if (_flagIndex >= 0)    s += "-- _flagIndex = "     + QString::number(_flagIndex) + '\n';
    if (_textIdIndex >= 0)  s += "-- _textIdIndex = "   + QString::number(_textIdIndex) + '\n';
    s += QChar(')');
    if (_parents.size()) {
        s += " INHERITS(";
        QVector<const cRecStaticDescr *>::ConstIterator ii, nn = parent().constEnd();
        for (ii = parent().constBegin(); ii != nn; ++ii) {
            s += QChar(' ') + (*ii)->fullTableNameQ() + QChar(',');
        }
        s.chop(1);
        s += QChar(')') + '\n';
    }
    s += ";\n-- Table type : ";
    switch (_tableType & TT_MASK) {
    case TT_BASE_TABLE:    s += "Base";    break;
    case TT_SWITCH_TABLE:  s += "Switch";  break;
    case TT_LINK_TABLE:    s += "Link";    break;
    case TT_CHILD_TABLE:   s += "Child";   break;
    default:            s += "?";       break;
    }
    if (_tableType & TT_VIEW_TABLE) s += ",View";
    return s;
}

int cRecStaticDescr::ixToOwner(eEx __ex) const
{
    int fix;
    QList<int> props;
    for (fix = 0; fix < _columnsNum; ++fix) {          // Az ownerre mutató mezőt keressük
        const cColStaticDescr& cd = columnDescrs()[fix];
        cColStaticDescr::eFKeyType t = cd.fKeyType;
        if (t == cColStaticDescr::FT_OWNER) break;
        if (t == cColStaticDescr::FT_PROPERTY) props << fix;

    }
    if (fix >= _columnsNum) {
        if (props.size() == 1) {   // Ha csak egy van, akkor elfogadjuk
            return props.first();
        }
        if (__ex) EXCEPTION(EDATA, -1, toString());
        return NULL_IX;
    }
    return fix;
}

int cRecStaticDescr::ixToParent(eEx __ex) const
{
    int fix;
    for (fix = 0; fix < _columnsNum; ++fix) {          // Egy önmagára mutató távoli kulcsot keresünk
        const cColStaticDescr& cd = columnDescrs()[fix];
        cColStaticDescr::eFKeyType t = cd.fKeyType;
        if (t == cColStaticDescr::FT_SELF) break;
    }
    if (fix >= _columnsNum) {
        if (__ex) EXCEPTION(EDATA, -1, toString());
        return NULL_IX;
    }
    return fix;
}

/* ******************************************************************************************************* */
cLangTexts::cLangTexts(cRecord *_par)
{
    parent = _par;
}

cLangTexts::~cLangTexts()
{
    ;
}

void cLangTexts::setTexts(const QString& _langName, const QStringList& texts)
{
    cRecordAny  lang(_sLanguages);
    qlonglong lid = lang.getIdByName(_langName);
    setTexts(lid, texts);
}

/*
QString cLangTexts::tableName2textTypeName(const QString& _tn)
{
    QString r = _tn;
    if (r.endsWith(QChar('s'))) r.chop(1);
    int ix;
    while (0 <= (ix = r.indexOf(QChar('_')))) r.remove(ix, 1);
    r += "text";
    return r;
}
*/
void cLangTexts::saveText(QSqlQuery& _q, const QStringList& _texts, cRecord *po, qlonglong lid)
{
    int tidix = po->descr().textIdIndex();
    qlonglong tid = po->getId(tidix);
    if (tid == NULL_ID) {
        QString sql = QString(
            "UPDATE %1 SET text_id = nextval('text_id_sequ') WHERE %2 = ? RETURNING text_id"
                ).arg(po->tableName(), po->idName());
        execSql(_q, sql, po->getId());
        bool ok;
        tid = _q.value(0).toLongLong(&ok);
        if (!ok) EXCEPTION(EPROGFAIL, po->getId(), po->identifying());
        po->setId(tidix, tid);
    }
    int n = po->colDescr(tidix).pEnumType->enumValues.size();
    QString qs;
    QVariantList vl;
    vl << tid << po->tableName();
    if (lid != NULL_ID) vl << lid;
    QStringList texts = _texts;
    for (int i = 0; i < n; ++i) {
        qs += "?,";
        vl << (isContIx(texts, i) ? texts.at(i) : _sNul);
    }
    qs.chop(1);
    QString sql = QString(
            "INSERT INTO localizations (text_id, table_for_text, language_id, texts) "
                " VALUES (?, ?, %1, ARRAY[%2]) "
            "ON CONFLICT ON CONSTRAINT localizations_pkey DO UPDATE SET texts = EXCLUDED.texts"
                ).arg(lid == NULL_ID ? "get_language_id()" : "?").arg(qs);
    execSql(_q, sql, vl);
}

void cLangTexts::saveTexts(QSqlQuery& q)
{
    foreach (qlonglong lid, textMap.keys()) {
        saveText(q, textMap[lid], parent, lid);
    }
}

void cLangTexts::loadTexts(QSqlQuery& q)
{
    QString sql = "SELECT language_id, texts FROM localizations WHERE text_id = ? AND table_for_text = ?";
    textMap.clear();
    if (execSql(q, sql, parent->get(_sTextId), parent->tableName())) {
        do {
            bool ok;
            qlonglong   lid   = q.value(0).toLongLong(&ok);
            if (!ok) EXCEPTION(EDATA, 0, parent->identifying());
            QStringList texts = sqlToStringList(q.value(1));
            setTexts(lid, texts);
        } while (q.next());
    }
}

/* ******************************************************************************************************* */
const QVariant  cRecord::_vNul;

cRecord::cRecord() : QObject(), _fields(), _likeMask()
{
   // _DBGFN() << QChar(' ') << VDEBPTR(this) << endl;
    _deletedBehavior = NO_EFFECT ;
    _stat = ES_NULL;
    pTextList = NULL;
    containerValid  = 0;
}

cRecord::cRecord(const cRecord& o) : QObject(), _fields(), _likeMask()
{
    EXCEPTION(ENOTSUPP, -1, o.identifying());
    //__o.descr();  // Inaktív kód, csak hogy ne ugasson a fordító.
}

cRecord::~cRecord()
{
    // _DBGFN() << QChar(' ') << VDEBPTR(this)
    pDelete(pTextList);
}

cRecord& cRecord::_clear()
{
    _fields.clear();
    _stat = ES_NULL;
    _likeMask.clear();
    delTextList();
    return *this;
}

cRecord& cRecord::_clear(int __ix)
{
    if (isNull()) return *this;
    if (_fields.size() <= __ix) EXCEPTION(EPROGFAIL, __ix, identifying());
    _fields[__ix].clear();
    if  (isEmpty()) _stat  = ES_NULL;
    else            _stat |= ES_MODIFY;
    condDelTextList(__ix);
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
    // text ??!!??
    return false;
}

cRecord& cRecord::_set(const cRecStaticDescr& __d, bool clear_text)
{
    _fields.clear();
    int i, n = __d.cols();
    for (i = 0; i < n; i++) _fields << QVariant();
    _stat = ES_EMPTY;
    if (clear_text) condDelTextList();
    return *this;
}

QStringList cRecord::defectiveFields() const
{
    QStringList r;
    if (isDefective()) {
        long mask = defectiveFieldMask() ;
        if (mask == 0) r << "[unknown]";
        else {
            int i;
            long b;
            for (i = 0, b = 1; i < cols(); ++i, b <<= 1) if (mask & b) {
                r << columnName(i);
            }
        }
    }
    return r;
}

cRecord& cRecord::set()
{
    return _set(descr());
    clearToEnd();
    emit cleared();
}

cRecord& cRecord::clear()
{
    _clear();
    clearToEnd();
    emit cleared();
    return *this;
}

cRecord& cRecord::clear(int __ix, eEx __ex)
{
    if (chkIndex(__ix, __ex) >= 0) {
        _clear(__ix);
        toEnd(__ix);
        fieldModified(__ix);
    }
    return *this;
}

cRecord& cRecord::clear(const QBitArray& __m, eEx __ex)
{
    int n = __m.size();
    if (n > cols()) {
        if (__ex) EXCEPTION(EDATA, n, identifying());
        n = cols();
    }
    for (int i = 0; i < n; ++i) if (__m[i]) clear(i);
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
    if (_fromp != NULL) {           // Ha nem az elején kezdjük
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
        DWAR() << QObject::trUtf8("Rekord túlolvasási kisérlet, rekord méret %1 ciklushatár %2.").arg(__r.count()).arg(n) << endl;
        n = __r.count();
    }
    int ix;                 // Cél index    (_record)r
    int m = __d.cols();     // Mezők száma a cél rekordban
    const int invalid = -1;
    QVector<int>    ixv(m, invalid); // Kereszt index: cél index -> forrás index
    for (i = first; i < n; i++) {       // csinálunk egy kereszt index táblát
        int ix = __d.toIndex(__r.fieldName(i), EX_IGNORE);
        if (ix >= 0) ixv[ix] = i;
    }
    _set(__d, false);  // isEmpty() == true, no clear texts
    if (ixv.count(invalid) == ixv.size()) {
        DWAR() << QObject::trUtf8("Nincs egy keresett mező sem a forrás rekordban.") << endl;
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
            if (!fd.isNullable) {
                _stat |= ES_INCOMPLETE;
                _setDefectivFieldBit(ix);
            }
        }
        condDelTextList(ix, f); // Ha megváltozott a text ID, akkor törli a nyelvi szövegeket, ha voltak.
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
        int ii = d.toIndex(fn, EX_IGNORE);  // A cél azonos nevű mezőjének az indexe
        if (!d.isIndex((ii))) continue;     // A cél rekordban nincs ilyen mező
        if (__o.isNull(i))    continue;     // vagy null a forrás mező
        QVariant v = __o.get(i);
        delTextList();
        _set(ii, v);
        m = true;
    }
    return m;
}

bool cRecord::isIdent() const
{
    if (isIdent(primaryKey())) return true;
    foreach (const QBitArray& m, uniques()) if (isIdent(m)) return true;
    return false;
}

bool cRecord::isIdent(const QBitArray& __m) const
{
    for (int i = 0; i < __m.size(); ++i) if (__m[i] && isNull(i)) return false;
    return true;
}

void cRecord::fieldsCopy(const cRecord& __o, const QBitArray& __m)
{
    if (descr() != __o.descr()) EXCEPTION(EDATA, 0, trUtf8("%1 != %2").arg(descr().fullTableName()).arg(__o.descr().fullTableName()));
    int n = __m.size();
    if (n > cols()) EXCEPTION(EDATA, n, identifying());
    for (int i = 0; i < n; i++) {
        if (__m[i]) {
            QVariant v = __o.get(i);
            condDelTextList(i);
            set(i, v);
        }
    }
}

void cRecord::fieldsCopy(QSqlQuery& __q, QString *pName, const QBitArray& __m)
{
    cRecord *pOld = newObj();
    if (pName == NULL && pName->isEmpty()) {
        pOld->setByName(__q, getName());
    }
    else {
        pOld->setByName(__q, *pName);
    }
    fieldsCopy(*pOld, __m);
    delete pOld;
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
    // _DBGFN() << " @(" << __i << _sCommaSp << __v.toString() << QChar(')') << endl;
    if (_fields.isEmpty()) set();
    qlonglong _s = 0;
    _set(__i, descr()[__i].set(__v, _s));
    if (__i == descr().textIdIndex(EX_IGNORE)) pDelete(pTextList);
    if (_s == ES_DEFECTIVE) {
        _stat |= ES_DEFECTIVE;
        _setDefectivFieldBit(__i);
    }
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

QString cRecord::getName() const {
    if (_isNull()) return QString();
    const cRecStaticDescr& d = descr();
    int ixN = d._nameIndex;
    if (isIndex(ixN) && !isNull(ixN)) return getName(ixN); // Ha van név mező és nem null
    int ixI = d._idIndex;
    if (!isIndex(ixI)) {
        if (isIndex(ixN)) return QString();     // Nincs ID elfogadjukk a névre a NULL-t
        EXCEPTION(EDATA, -1, trUtf8("A rekord neve nem állpítható meg. Nincs sem név, sem ID mező :\n") + identifying());
    }
    if (isNull(d.idIndex())) return QString();
    QSqlQuery q = getQuery();
    QString fn = d.checkId2Name(q);
    return execSqlTextFunction(q, fn, get(d.idIndex()));
}
const QVariant& cRecord::get(int __i) const
{
    if (isNull()) return _vNul;
    return _fields[chkIndex(__i)];
}

QString cRecord::view(QSqlQuery& q, int __i) const
{
    static const QString  rHaveNo = QObject::trUtf8("[HAVE NO]");
    if (isIndex(__i) == false) return rHaveNo;
    return descr()[__i].toView(q, get(__i));
}

bool cRecord::isContainerValid(qlonglong __mask) const
{
    if (descr()._textIdIndex < 0) {
        EXCEPTION(EPROGFAIL, -1, identifying());
    }
    return 0 != (containerValid & __mask);
}

void cRecord::setContainerValid(qlonglong __set, qlonglong __clr)
{
    if (descr()._textIdIndex < 0) {
        EXCEPTION(EPROGFAIL, -1, identifying());
    }
    containerValid |=  __set;
    containerValid &= ~__clr;
}

cMac    cRecord::getMac(int __i, eEx __ex) const
{
    cMac r;
    if (NULL_IX == chkIndex(__i, __ex)) return r;
    int t = colDescr(__i).eColType;
    if (t == cColStaticDescr::FT_MAC) {
        if (!isNull(__i)) r = get(__i).value<cMac>();
        return r;
    }
    if (__ex != EX_IGNORE) {
        EXCEPTION(EDATA, t, trUtf8("A %1 mező típusa nem MAC :\n").arg(fullColumnName(columnName(__i))) + identifying());
    }
    return r;
}

cRecord& cRecord::setMac(int __i, const cMac& __a, eEx __ex)
{
    int t = colDescr(__i).eColType;
    if (t != cColStaticDescr::FT_MAC) {
        if (__ex) EXCEPTION(EDATA, t, trUtf8("A %1 mező típusa nem MAC :\n").arg(fullColumnName(columnName(__i))) + identifying());
    }
    else {
        if (!__a) clear(__i);
        else      set(__i, QVariant::fromValue(__a));
    }
    return *this;
}

cRecord& cRecord::setIp(int __i, const QHostAddress& __a, eEx __ex)
{
    int t = colDescr(__i).eColType;
    if (t != cColStaticDescr::FT_INET) {
        if (__ex) EXCEPTION(EDATA, t, trUtf8("A %1 mező típusa nem IP cím :\n").arg(fullColumnName(columnName(__i))) + identifying());
    }
    else {
        if (__a.isNull()) clear(__i);
        else      set(__i, QVariant::fromValue(__a));
    }
    return *this;
}

QStringList cRecord::getStringList(int __i, eEx __ex) const
{
    if (isIndex(__i)) {
        int t = colDescr(__i).eColType;
        if (t & cColStaticDescr::FT_ARRAY) {
            return get(__i).toStringList();
        }
        if (__ex) EXCEPTION(EDATA, t, toString());
    }
    else if (__ex) EXCEPTION(ENOINDEX, __i, toString());
    return QStringList();
}

cRecord& cRecord::setStringList(int __i, const QStringList& __v, eEx __ex)
{
    if (isIndex(__i)) {
        set(__i, __v);
    }
    else {
        if (__ex) EXCEPTION(ENOINDEX, __i, toString());
        _stat |= ES_DEFECTIVE;
    }
    return *this;
}

cRecord& cRecord::addStringList(int __i, const QStringList &__v, eEx __ex)
{
    if (isIndex(__i)) {
        set(__i, getStringList(__i) + __v);
    }
    else {
        if (__ex) EXCEPTION(ENOINDEX, __i, toString());
        _stat |= ES_DEFECTIVE;
    }
    return *this;
}


QBitArray cRecord::areNull() const
{
    QBitArray r(cols(), true);
    if (isEmpty_()) return r;
    int i, n = cols();
    for (i = 0; i < n; ++i) {
        r[i] = isNull(i);
    }
    return r;
}

bool cRecord::insert(QSqlQuery& __q, eEx _ex)
{
    _DBGFN() << "@(," << DBOOL(_ex) << ") table : " << fullTableName() << endl;
    const cRecStaticDescr& recDescr = descr();
    __q.finish();
    if (!recDescr.isUpdatable()) EXCEPTION(EDATA, -1 , trUtf8("A %1 tábla nem módosítható.").arg(tableName()));
    QString sql, qms;
    if (descr()._deletedIndex != -1 && _deletedBehavior & REPLACED_BY_NAME) {
        cRecord *po = dup();
        po->setOn(deletedIndex());
        QBitArray where = nameKeyMask() | mask(deletedIndex());
        bool ed = fetchQuery(__q, false, where);
        delete po;
        if (ed) {     // Van egy azonos nevű deleted = true rekordunk.
            return rewrite(__q, _ex);
        }
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
    for (int ix = 0; ix < recDescr.cols(); ++ix) {           // ix: mező index a rekordban
        if (!isNull(ix)) bind(ix, __q, i++);
    }
    PDEB(VVERBOSE) << "Insert :" << sql << " / Bind { " + _sql_err_bound(__q) + "}" << endl;
    _EXECSQL(__q);
    if (__q.first()) {
        set(__q);
        _stat =  ES_EXIST | ES_COMPLETE | ES_IDENTIF;
        // PDEB(VERBOSE) << "Insert RETURNING :" << toString() << endl;
        return true;
    }
    if (_ex > EX_ERROR) {
        cError *pe = NEWCERROR(EDATA, 0,
            trUtf8("A beszúrás utáni újraolvasás sikertelen, nincs adat. Objektum azonosító : ") + identifying());
        _sql_err_ex(pe, __q.lastError(), sql, _sql_err_bound(__q));
    }
    return false;
}

cError *cRecord::tryInsert(QSqlQuery &__q, eTristate __tr, bool text)
{
    cError *pe = NULL;
    eTristate tr = trFlag(__tr);
    if (tr == TS_TRUE) sqlBegin(__q, tableName());
    try {
        insert(__q, EX_ERROR);
        if (text) {
            saveText(__q);
        }
    }
    CATCHS(pe);
    if (tr == TS_TRUE) {
        if (pe == NULL) sqlCommit(__q, tableName());
        else            sqlRollback(__q, tableName());
    }
    return pe;
}

bool cRecord::rewrite(QSqlQuery &__q, eEx __ex)
{
    _DBGFN() << "@(," << DBOOL(__ex) << ") table : " << fullTableName() << endl;
    QBitArray   sets(cols(), true);         // Minden mezőt kiírunk,
    QBitArray   where = nameKeyMask();
    sets &= ~where;                             // kivéve a név mezőt ...
    if (idIndex(EX_IGNORE) >= 0 && isNullId()) {// és az ID-t, ha van olyan és az értéke NULL
        sets.clearBit(idIndex());
    }
    const QBitArray& pfr = descr()._protectedForRewriting;  // Védet mezőket nem töröljük a NULL értékű mezökkel
    if (!pfr.isEmpty()) {
        for (int i = 0; i < pfr.size(); ++i) {
            if (pfr[i] && isNull(i)) sets[i] = false;
        }
    }
    int r = update(__q, true, sets, where,__ex);
    switch (r) {
    case 1: return true;
    case 0:
        if (__ex != EX_IGNORE) SQLQUERYDERR(__q, EFOUND, 0, trUtf8("Nothing rewrite any record : %1").arg(identifying()));
        break;
    default:    // Ez nagyon gáz, egyedinek kellene lennie!!
        EXCEPTION(AMBIGUOUS, r, trUtf8("Object : %1; unique key problem.").arg(identifying()));
    }
    return false;
}

cError *cRecord::tryRewrite(QSqlQuery& __q, eTristate __tr, bool text)
{
    cError *pe = NULL;
    eTristate tr = trFlag(__tr);
    if (tr == TS_TRUE) sqlBegin(__q, tableName());
    try {
        rewrite(__q, EX_ERROR);
        if (text) saveText(__q);
    }
    CATCHS(pe);
    if (tr == TS_TRUE) {
        if (pe == NULL) sqlCommit(__q, tableName());
        else            sqlRollback(__q, tableName());
    }
    return pe;
}

bool cRecord::rewriteById(QSqlQuery& __q, enum eEx __ex)
{
    if (isNullId()) {
        if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL, 0, trUtf8("Az ID értéke nem lehet NULL. ") + identifying());
        return false;
    }
    QBitArray set(cols(), 1);                   // Összes mező
    QBitArray where = _mask(cols(), idIndex()); // Az ID mező
    set = set & ~where;                         // Összes kivéve ID
    return 1 == update(__q, true, set, where, EX_NOOP);
}


int cRecord::replace(QSqlQuery& __q, eEx __ex)
{
    if (existByNameKey(__q)) return rewrite(__q, __ex) ? R_UPDATE : R_ERROR; // Ha van akkor replace
    else                     return insert( __q, __ex) ? R_INSERT : R_ERROR; // Ha nincs, akkor insert
}

cError *cRecord::tryReplace(QSqlQuery& __q, eTristate __tr, bool text)
{
    cError *pe = NULL;
    eTristate tr = trFlag(__tr);
    QStringList texts;
    if (text && pTextList != NULL) {
        texts = *pTextList;
        pDelete(pTextList);
    }
    if (tr == TS_TRUE) sqlBegin(__q, tableName());
    try {
        replace(__q, EX_ERROR);
        if (text && !texts.isEmpty()) {
            pTextList = new QStringList(texts);
            saveText(__q);
        }
    }
    CATCHS(pe);
    if (tr == TS_TRUE) {
        if (pe == NULL) sqlCommit(__q, tableName());
        else {
            sqlRollback(__q, tableName());
            if (text && !texts.isEmpty()) {
                pTextList = new QStringList(texts);
            }
        }
    }
    return pe;
}

void cRecord::query(QSqlQuery& __q, const QString& sql, const tIntVector& __arg) const
{
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    for (int i = 0; i < __arg.size(); ++i) {
        bind(__arg[i], __q, i);
    }
    _EXECSQL(__q);
}

void cRecord::query(QSqlQuery& __q, const QString& sql, const QBitArray& _fm) const
{
    // _DBGFN() << sql << endl;
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    int i,j;
    for (i = j = 0; _fm.size() > i; i++) {
        if (_fm.at(i) && false == get(i).isNull()) {
            bind(i, __q, j++);
        }
    }
    _EXECSQL(__q);
    // DBGFNL();
}

QString cRecord::whereString(QBitArray& _fm) const
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
        if (descr()._deletedIndex != -1 && _deletedBehavior & FILTERED && _fm.size() <= descr()._deletedIndex && _fm[descr()._deletedIndex] == false) {
            where += " AND deleted = FALSE";
        }
        where = " WHERE " + where;
    }
    else if (descr()._deletedIndex != -1 && _deletedBehavior & FILTERED) {
        where = " WHERE deleted = FALSE";
    }
    return where;
}

bool cRecord::fetchQuery(QSqlQuery& __q, bool __only, const QBitArray& _fm, const tIntVector& __ord, int __lim, int __off, QString __s, QString __w) const
{
    // _DBGFN() << " @(" << _fm << _sCommaSp << __ord << _sCommaSp << __lim << _sCommaSp << __off <<  _sCommaSp << __s << QChar(')') << endl;
    // PDEB(VVERBOSE) << "Record value : " << toString() << endl;
    const cRecStaticDescr& recDescr = descr();
    QBitArray fm = _fm;
    QString     sql;    // sql parancs
    sql  = "SELECT " + (__s.isEmpty() ? "*" : __s) + " FROM ";
    if (__only) sql += "ONLY ";
    sql += descr().fullViewNameQ();
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
            sql += (i < 0 ? " DESC," : " ASC,");
        }
        sql.chop(1);
    }
    if (__lim > 0) sql += " LIMIT "  + QString::number(__lim);
    if (__off > 0) sql += " OFFSET " + QString::number(__off);
    query(__q, sql, fm);
    return __q.first();
}

bool cRecord::fetch(QSqlQuery& __q, bool __only, const QBitArray& _fm, const tIntVector& __ord, int __lim, int __off)
{
    _DBGFN() << " @(" << _fm << _sCommaSp << __ord << _sCommaSp << __lim << _sCommaSp << __off << QChar(')') << identifying() << endl;
    if (fetchQuery(__q,__only, _fm, __ord, __lim, __off)) {
        set(__q.record());
        _stat |= ES_EXIST;
        _DBGFNL() << _sTrue << endl;
        return true;
    }
    set();
    _DBGFNL() << _sFalse << endl;
    return false;
}

QBitArray cRecord::getSetMap() const
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
    if (!fetch(__q, false, m)) return 0;
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

int cRecord::existId(QSqlQuery& q, qlonglong __id) const
{
    QString sql = QString("SELECT COUNT(*) FROM %1 WHERE %2 = ?").arg(fullTableNameQ(), dQuoted(idName()));
    bool ok = execSql(q, sql, __id);
    int  n;
    if (ok) n = q.value(0).toInt(&ok);
    return ok ? n : -1;
}

bool cRecord::existByNameKey(QSqlQuery& __q)
{
    QBitArray m = nameKeyMask() & areNull();
    // kitöltetlen mező a kulcsban?
    if (m.count(true)) {
        // Ezeknél meg kell adni a DEFAULT értéket, nem támogatjuk...
        EXCEPTION(EDBDATA, 0, trUtf8("%1 table %2 field, is NULL.").arg(tableName()).arg(columnNames(m).join(", ")));
    }
    int row = rows(__q, false, nameKeyMask());
    switch (row) {
    case 0: return false;
    case 1: return true;
    }
    EXCEPTION(EDBDATA, row);
    return false;
}

qlonglong cRecord::fetchTableOId(QSqlQuery& __q, eEx __ex) const
{
    QBitArray m = getSetMap();              // Mit ismerünk ?
    if (isFaceless() || isEmpty_() || m.count(true) == 0) {
        if (__ex) {
            QString tn = _stat == ES_FACELESS ? "<ismeretlen>" : descr().fullTableName();
            QString msg = trUtf8("Az objektum üres. Rekord típus %1").arg(tn);
            EXCEPTION(EDATA, 0, msg);
        }
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

    if (fetchQuery(__q, false, m, tIntVector(),0,0,QString("tableoid"))) {
        if (__q.size() == 1) {
            return variantToId(__q.value(0), __ex, NULL_ID);
        }
        else {
            qlonglong oid = variantToId(__q.value(0), __ex, NULL_ID);
            while (__q.next()) if (oid != variantToId(__q.value(0), __ex, NULL_ID)) {
                if (__ex) {
                    QString msg = trUtf8("Objektum rekord típus %1; adatok : %2").arg(descr().fullTableName()).arg(toString());
                    EXCEPTION(AMBIGUOUS, __q.size(), msg);
                }
                oid = NULL_ID;
                break;
            }
            return oid;
        }
    }
    if (__ex) {
        QString msg = trUtf8("Objektum rekord típus %1; adatok : %2").arg(descr().fullTableName()).arg(toString());
        EXCEPTION(EFOUND,0,msg);
    }
    return NULL_ID;
}

int cRecord::update(QSqlQuery& __q, bool __only, const QBitArray& __set, const QBitArray& __where, eEx __ex)
{
//    _DBGFN() << "@("
//             << this->toString() << _sCommaSp << DBOOL(__only) << _sCommaSp << list2string(__set) <<  _sCommaSp << list2string(__where) << _sCommaSp << DBOOL(__ex)
//             << ")" << endl;
    __q.finish();
    if (!isUpdatable()) EXCEPTION(EDATA, -1 , QObject::trUtf8("Az adat nem módosítható."));
    QBitArray bset  = __set;
    QBitArray where = __where;
    if (where.size() == 0) where = primaryKey();
    if (bset.count(true) == 0) {    // Alapértelmezett ?
        bset = QBitArray(cols(), true) & ~where;    // Amire keresünk azt nem modosítjuk
        int tixix = descr().textIdIndex(EX_IGNORE);
        if (tixix >= 0) bset.clearBit(tixix);       // Szöveg indexet sem bántjuk!!!
    }
    if (bset.count(true) == 0) {    // A nyelvi szövegek miatt előfordulhat!
        if (__ex >= EX_WARNING) EXCEPTION(EDATA);
        return 0;
    }
    QString sql = QString(__only ? "UPDATE ONLY %1 SET" : "UPDATE %1 SET").arg(fullTableNameQ());
    int i,j;
    for (i = 0; i < bset.size(); i++) {
        if (bset[i]) {
            sql += QChar(' ') + columnNameQ(i);
            if (get(i).isNull()) sql += " = DEFAULT,";
            else                 sql += _sCondW + QChar(' ') + QChar(',');
        }
    }
    sql.chop(1);
    sql += whereString(where) + " RETURNING *";
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql)
    for (j = i = 0; i < bset.size(); i++) {
        if (bset[i] && false == get(i).isNull()) {
            bind(i, __q, j++);
        }
    }
    for (i = 0; i < where.size(); i++) {
        if (where[i] && false == get(i).isNull()) {
            bind(i, __q, j++);
        }
    }

    _EXECSQL(__q);
    if (__q.first()) {
        set(__q);
        _stat =  ES_EXIST | ES_COMPLETE | ES_IDENTIF;
        // PDEB(VERBOSE) << "Update RETURNING :" << toString() << endl;
        return __q.numRowsAffected();
    }
    else {
        QString msg = trUtf8("Nothing modify any record : %1").arg(identifying());
        if (__ex == EX_NOOP) SQLQUERYDERR(__q, EFOUND, 0, msg);
        return 0;
    }
}

cError *cRecord::tryUpdate(QSqlQuery& __q, bool __only, const QBitArray& __set, const QBitArray& __where, eTristate __tr)
{
    cError *pe = NULL;
    eTristate tr = trFlag(__tr);
    if (tr == TS_TRUE) sqlBegin(__q, tableName());
    try {
        update(__q, __only, __set, __where, EX_ERROR);
    }
    CATCHS(pe);
    if (tr == TS_TRUE) {
        if (pe == NULL) sqlCommit(__q, tableName());
        else            sqlRollback(__q, tableName());
    }
    return pe;
}

cError *cRecord::tryUpdateById(QSqlQuery& __q, eTristate __tr, bool text)
{
    cError *pe = NULL;
    int n = 0;
    eTristate tr = trFlag(__tr);
    if (tr == TS_TRUE) sqlBegin(__q, tableName());
    try {
        n = update(__q, true, QBitArray(), QBitArray(), text ? EX_ERROR : EX_NOOP);
        if (n > 1) {
            EXCEPTION(EOID, n, identifying());
        }
        if (text) {
            saveText(__q);
        }
    }
    CATCHS(pe);
    if (tr == TS_TRUE) {
        if (pe == NULL) sqlCommit(__q, tableName());
        else            sqlRollback(__q, tableName());
    }
    return pe;
}

bool cRecord::updateFieldByName(QSqlQuery &__q, const QString& _name, const QString& _fn, const QVariant& val, eEx __ex)
{
    setName(_name);
    int ix = toIndex(_fn);
    set(ix, val);
    return update(__q, false, _bit(ix), _bit(nameIndex()), __ex);
}

bool cRecord::updateFieldById(QSqlQuery &__q, qlonglong _id, const QString& _fn, const QVariant& val, eEx __ex)
{
    setId(_id);
    int ix = toIndex(_fn);
    set(ix, val);
    return update(__q, false, _bit(ix), _bit(idIndex()), __ex);
}

int cRecord::mark(QSqlQuery& __q, const QBitArray &__where, bool __flag) const
{
    cRecord *p = dup();
    int ixFlag = p->descr()._flagIndex;
    if (ixFlag < 0) EXCEPTION(EDATA, 0, "Missing 'flag' field in " + tableName() + " table.");
    p->setBool(ixFlag, __flag);
    int r = p->update(__q, false, mask(ixFlag), __where, EX_ERROR);
    delete p;
    return r;
}

int cRecord::removeMarked(QSqlQuery& __q, const QBitArray& __where) const
{
    cRecord *p = dup();
    int ixFlag = p->descr()._flagIndex;
    if (ixFlag < 0) EXCEPTION(EDATA, 0, "Missing 'flag' field in " + tableName() + " table.");
    p->setBool(ixFlag, true);
    QBitArray where = __where;
    if (where.isEmpty()) where = p->primaryKey() | p->mask(ixFlag);
    else                 where = where           | p->mask(ixFlag);
    int r = p->remove(__q, false, where, EX_ERROR);
    delete p;
    return r;
}


int cRecord::remove(QSqlQuery& __q, bool __only, const QBitArray& _fm, eEx __ex)
{
    QBitArray fm = _fm;
    if (!isUpdatable()) EXCEPTION(EDATA, -1 , QObject::trUtf8("Az adat nem módosítható."));
    if (descr()._deletedIndex != -1 && _deletedBehavior & DELETE_LOGICAL) {
        set(descr()._deletedIndex, QVariant(true));
        return update(__q, __only, mask(descr()._deletedIndex), fm, __ex);
    }
    QString sql = "DELETE FROM ";
    if (__only) sql += "ONLY ";
    sql += tableName() + whereString(fm);
    query(__q, sql, fm);
    int r = __q.numRowsAffected();
    if (r == 0) {
        QString msg = trUtf8("Nothing delete any record : %1").arg(identifying());
        if (__ex == EX_NOOP) SQLQUERYDERR(__q, EFOUND, 0, msg);
    }
    return r;
}

cError *cRecord::tryRemove(QSqlQuery& __q, bool __only, const QBitArray& _fm, eTristate __tr)
{
    cError *pe = NULL;
    eTristate tr = trFlag(__tr);
    if (tr == TS_TRUE) sqlBegin(__q, tableName());
    try {
        remove(__q, __only, _fm, EX_ERROR);
    }
    CATCHS(pe);
    if (tr == TS_TRUE) {
        if (pe == NULL) sqlCommit(__q, tableName());
        else            sqlRollback(__q, tableName());
    }
    return pe;
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
            _stat |= ES_DEFECTIVE;
            _setDefectivFieldBit(i);
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
    QString s = QChar(' ') + mCat(schemaName(), tableName()) + QChar('{');
    for (int i = 0; isIndex(i); i++) {
        s += QChar(' ') + columnName(i);
        if (isNull(i)) s += " IS NULL;";
        else           s +=  " = " + debVariantToString(get(i)) + ";";
    }
    s.chop(1);
    s += QChar('}');
    return s;
}


int cRecord::delByName(QSqlQuery& q, const QString& __n, bool __pat, bool __only)
{
    static QString _sOnly = "ONLY ";
    QString only = __only ? _sOnly : _sNul;
    QString sql;
    if (descr().deletedIndex(EX_IGNORE) >= 0 && _deletedBehavior & DELETE_LOGICAL) {  // Nincs deleted mező, tényleges törlés
        sql = "UPDATE " + only
             + tableName() + " SET deleted = 't'  WHERE "
             + dQuoted(nameName()) + (__pat ? " LIKE " : " = ") + quoted(__n)
             + " AND NOT deleted";
    }
    else {                                  // Csak a deleted mezőt állítjuk true-ra
        sql = "DELETE FROM " + only + tableName()
            + " WHERE " + dQuoted(nameName()) + (__pat ? " LIKE " : " = ") + quoted(__n);
    }
    EXECSQL(q, sql);
    int n = q.numRowsAffected();
    // PDEB(VVERBOSE) << QObject::trUtf8("delByName SQL : \"%1\" removed %1 record(s).").arg(sql).arg(n) << endl;
    return  n;
}

int cRecord::touch(QSqlQuery& q, const QString& _fn, const QBitArray& _where)
{
    QString fn = _fn;
    if (fn.isEmpty()) fn = _sLastTime;
    int iLastTime = toIndex(fn);
    QBitArray bset  = mask(iLastTime);
    clear(iLastTime);
    QBitArray where = _where;
    QString sql = QString("UPDATE %1 SET %2 = NOW() ").arg(fullTableNameQ()).arg(fn);
    sql += whereString(where);
    sql += " RETURNING *";
    query(q, sql, where);
    if (q.first()) {
        set(q.record());
        _stat |= ES_EXIST;
        return q.size();
    }
    set();
    return 0;
}

int cRecord::updateFieldByNames(QSqlQuery& q, const QStringList& _nl, const QString& _fn, const QVariant& _v) const
{
    int r = 0;
    QString nfn = nameName();
    const QString sql = QString("UPDATE %1 SET %2 = ? WHERE %3 = ?").arg(fullTableNameQ(), _fn, nfn);
    int ix = toIndex(_fn);
    const cColStaticDescr& cd = colDescr(ix);
    QVariant v;
    qlonglong st;
    v = cd.set(_v, st);     // Conversion to local format
    v = cd.toSql(v);        // Conversion to SQL
    foreach (QString n, _nl) {
        execSql(q, sql, v, n);
        if (q.numRowsAffected() == 1) ++r;
        else {
            DERR() << trUtf8("Record not updated, name = %1; %2 = %3 : %4").
                      arg(n, _fn, debVariantToString(_v), identifying());
        }
    }
    return r;
}

int cRecord::addValueArrayFieldByNames(QSqlQuery& q, const QStringList& _nl, const QString& _fn, const QVariant& _v) const
{
    int r = 0;
    QString nfn = nameName();
    const QString sql = QString("UPDATE %1 SET %2 = array_append(%3, ?) WHERE %4 = ?").arg(fullTableNameQ(), _fn, _fn, nfn);
    int ix = toIndex(_fn);
    const cColStaticDescr& cd = colDescr(ix);
    QVariant v;
    qlonglong st;
    v = cd.set(_v, st);     // Conversion to local format
    v = cd.toSql(v);        // Conversion to SQL
    foreach (QString n, _nl) {
        execSql(q, sql, v, n);
        if (q.numRowsAffected() == 1) ++r;
        else {
            DERR() << trUtf8("Record not updated, name = %1; %2 += %3 : %4").
                      arg(n, _fn, debVariantToString(_v), identifying());
        }
    }
    return r;
}

int cRecord::updateFieldByIds(QSqlQuery& q, const QList<qlonglong>& _il, const QString& _fn, const QVariant& _v) const
{
    int r = 0;
    QString ifn = idName();
    const QString sql = QString("UPDATE %1 SET %2 = ? WHERE %3 = ?").arg(fullTableNameQ(), _fn, ifn);
    int ix = toIndex(_fn);
    const cColStaticDescr& cd = colDescr(ix);
    QVariant v;
    qlonglong st;
    v = cd.set(_v, st);     // Conversion to local format
    v = cd.toSql(v);        // Conversion to SQL
    foreach (qlonglong id, _il) {
        execSql(q, sql, v, id);
        if (q.numRowsAffected() == 1) ++r;
        else {
            DERR() << trUtf8("Record %1 not updated (not found) id = %2").arg(fullTableName()).arg(id);
        }
    }
    return r;
}

bool cRecord::isEmpty(int _ix) const
{
    if (isNull(_ix)) return true;
    if (colDescr(_ix).eColType & cColStaticDescr::FT_ARRAY) {
        return get(_ix).toList().isEmpty();
    }
    switch (colDescr(_ix).eColType) {
    case cColStaticDescr::FT_INTEGER:
    case cColStaticDescr::FT_INTERVAL:
        return getId(_ix) == 0;
    case cColStaticDescr::FT_REAL:
        return get(_ix).toDouble() == 0.0;
    case cColStaticDescr::FT_POLYGON:
        return get(_ix).value<tPolygonF>().isEmpty();
    case cColStaticDescr::FT_TEXT:
    case cColStaticDescr::FT_ENUM:
    case cColStaticDescr::FT_BINARY:
        return getName(_ix).isEmpty();
    case cColStaticDescr::FT_BOOLEAN:
        return getBool(_ix);
//  case cColStaticDescr::FT_MAC:
//  case cColStaticDescr::FT_INET:
//  case cColStaticDescr::FT_CIDR:
//  case cColStaticDescr::FT_TIME:
//  case cColStaticDescr::FT_DATE:
//  case cColStaticDescr::FT_DATE_TIME:
    }
    return true;
}

QString cRecord::identifying(bool t) const
{
    QString otype = typeid(*this).name();
    QString table = tableName();
    QString record;
    if (t) {
        record = trUtf8("Objektum típus : %1 (%2 tábla). ").arg(otype, table);
    }
    if (isEmpty_()) record += trUtf8("Üres objektum.");
    else {
        QSqlQuery q(getQuery());
        QString name = trUtf8("Name(%1) = %2");
        QString   id = trUtf8("ID(%1) = %2");
        int i, ix;
        ix = nameIndex(EX_IGNORE);
        if (ix == NULL_IX) {
            QBitArray m = nameKeyMask(EX_IGNORE);
            if (m.count(true)) {
                QStringList fl;
                QStringList vl;
                for (i = 0; i < cols(); ++i) if (m[i]) {
                    fl << columnName(i);
                    vl << quotedString(view(q, i));
                }
                name = name.arg(fl.join(_sCommaSp), vl.join(_sCommaSp));
            }
            else {
                name = trUtf8("No name");
            }
        }
        else {
            name = name.arg(columnName(ix), quotedString(view(q, ix)));
        }
        ix = idIndex(EX_IGNORE);
        if (ix == NULL_IX) {
            QBitArray m = primaryKey();
            if (m.count(true)) {
                QStringList fl;
                QStringList vl;
                for (i = 0; i < cols(); ++i) if (m[i]) {
                    fl << columnName(i);
                    vl << quotedString(view(q, i));
                }
                id = id.arg(fl.join(_sCommaSp), vl.join(_sCommaSp));
            }
            else {
                id = trUtf8("No ID, or primary key");
            }
        }
        else {
            id = id.arg(columnName(ix), quotedString(view(q, ix)));
        }
        record += QString(" %1 ; %2 .").arg(name, id);
        foreach (const cColStaticDescr *pCd, (QList<cColStaticDescr *>&)descr().columnDescrs()) {
            if (pCd->fKeyType == cColStaticDescr::FT_OWNER) {
                const cRecStaticDescr *pRd = cRecStaticDescr::get(pCd->fKeyTable, pCd->fKeySchema, true);
                record += trUtf8(" Tulajdonos : ");
                if (pRd == NULL) {
                    record += trUtf8(" jelenleg nem megállapítható (tábla : %1)").arg(dQuotedCat(pCd->fKeyTable, pCd->fKeySchema));
                }
                else {
                    cRecordAny o(pRd);
                    record += trUtf8(" tábla : %1.").arg(pRd->fullTableNameQ());
                    int fix = pCd->pos -1;
                    qlonglong oid = getId(fix);
                    if (oid == NULL_ID) record += trUtf8(" Tulajdonos rekord nincs (NULL).");
                    else {
                        if (o.fetchById(q, oid)) {
                            record += trUtf8(" Tulajdonos rekord : %1").arg(o.identifying());
                        }
                        else {
                            record += trUtf8(" Tulajdonos rekord nem található (ID = %1).").arg(oid);
                        }
                    }
                }
                break;  // Csak egy lehet
            }
        }
    }
    return record;
}

QString cRecord::show(bool t) const
{
    return identifying(t);
}

void cRecord::delTextList()
{
    if (pTextList == NULL) return;  // There is no list, there is nothing to delete
    pDelete(pTextList);
    setContainerValid(0, CV_LL_TEXT);
}

void cRecord::condDelTextList(int _ix, const QVariant& _tid)
{
    if (pTextList == NULL) return;  // There is no list, there is nothing to delete
    int tix = descr()._textIdIndex;
    if (tix < 0) {  // :-O
        EXCEPTION(EPROGFAIL, 0, trUtf8("If there is no text_id, there could be no list : %1").arg(identifying()));
    }
    if (_ix != NULL_IX && _ix != tix) return;   // no change text id
    qlonglong oldTid = getId(tix);              // Old text id
    bool      ok = true;
    qlonglong newTid = _tid.isNull() ? NULL_ID : _tid.toLongLong(&ok);    // New text_id
    if (!ok || oldTid != newTid) {              // Changed text_id (or new value is invalid)
        delTextList();
    }
}

QString cRecord::getText(int _tix, const QString& _d) const
{
    QString r;
    if (pTextList != NULL && isContIx(*pTextList, _tix)) r = pTextList->at(_tix);
    return r.isEmpty() ? _d : r;
}

QString cRecord::getText(const QString& _tn, const QString& _d) const
{
    int tidix = descr().textIdIndex();
    const cColStaticDescr& cd = colDescr(tidix);
    const cColEnumType *pcet = cd.pEnumType;
    if (pcet == NULL) EXCEPTION(EPROGFAIL, 0, trUtf8("No text field list, missing enum; Object : %1").arg(identifying()));
    int tix = pcet->str2enum(_tn);
    return getText(tix, _d);
}

cRecord&    cRecord::setText(int _tix, const QString& _t)
{
    int tidix = descr().textIdIndex();
    const cColEnumType *pet = colDescr(tidix).pEnumType;
    if (pet == NULL) EXCEPTION(EPROGFAIL, tidix, trUtf8("No text field list, missing enum; Object : %1").arg(identifying()));
    int s = pet->enumValues.size();
    if (pTextList == NULL) {
        pTextList = new QStringList;
        while (pTextList->size() < s) *pTextList << _sNul;
    }
    if (!isContIx(*pTextList, _tix)) EXCEPTION(ENOINDEX, _tix, identifying());
    (*pTextList)[_tix] = _t;
    return *this;
}

cRecord&    cRecord::setText(const QString& _tn, const QString& _t)
{
    int tidix = descr().textIdIndex();
    const cColEnumType *pet = colDescr(tidix).pEnumType;
    if (pet == NULL) EXCEPTION(EPROGFAIL, tidix, trUtf8("No text field list, missing enum; Object : %1").arg(identifying()));
    int tix = pet->str2enum(_tn);
    return setText(tix, _t);
}

cRecord&    cRecord::setTexts(const QStringList& _txts)
{
    if (_txts.isEmpty()) {
        pDelete(pTextList);
    }
    else if (pTextList == NULL) {
        pTextList = new QStringList(_txts);
    }
    else {
        *pTextList = _txts;
    }
    return *this;
}

bool cRecord::fetchText(QSqlQuery& _q, bool __force)
{
    if (!__force && pTextList != NULL && !pTextList->isEmpty()) return true;
    int tidix = descr().textIdIndex();
    qlonglong tid = getId(tidix);
    pDelete(pTextList);
    containerValid &= ~CV_LL_TEXT;
    if (tid == NULL_ID) return false;
    pTextList = new QStringList;
    *pTextList = textId2texts(_q, tid, tableName());
    const cColEnumType *pet = colDescr(tidix).pEnumType;
    if (pet == NULL) EXCEPTION(EPROGFAIL, tidix, trUtf8("No text field list, missing enum; Object : %1").arg(identifying()));
    int s = pet->enumValues.size();
    if (pTextList->size() > s) *pTextList = pTextList->mid(0, s);
    else while (pTextList->size() < s) *pTextList << _sNul;
    containerValid |= CV_LL_TEXT;
    return !pTextList->isEmpty();
}

void cRecord::saveText(QSqlQuery& _q)
{
    if (pTextList == NULL) return;
    cLangTexts::saveText(_q, *pTextList, this);
}


/* ******************************************************************************************************* */
cRecordFieldRef::cRecordFieldRef(const cRecordFieldRef& __r)
    : _record(__r._record)
{
    // _DBGF() << "Record " << __r._record.descr().fullTableName() << " index : " << __r._index << endl;
    _index = __r._index;
    // _DBGFL() << "Record " << _record.descr().fullTableName() << " index : " << _index << endl;
}

/* ******************************************************************************************************* */
// Dummy osztály dummy példány
no_init_ _no_init_;
/* ******************************************************************************************************* */
cRecordAny::cRecordAny() : cRecord()
{
    pStaticDescr = NULL;
    _stat = ES_FACELESS;
}

cRecordAny::cRecordAny(const QString& _tn, const QString& _sn) : cRecord()
{
    pStaticDescr = cRecStaticDescr::get(_tn, _sn);
    _set(*pStaticDescr);
}

cRecordAny::cRecordAny(const cRecStaticDescr *_p) : cRecord()
{
    pStaticDescr = _p;
    _set(*pStaticDescr);
}

cRecordAny::cRecordAny(const cRecordAny &__o)  : cRecord()
{
    *this = *(cRecord *)&__o;
}

cRecordAny::cRecordAny(const cRecord& __o) : cRecord()
{
    *this = __o;
}

cRecordAny::~cRecordAny()
{
    ;
}

cRecordAny& cRecordAny::setType(const QString& _tn, const QString& _sn, eEx __ex)
{
    pStaticDescr = cRecStaticDescr::get(_tn, _sn, __ex == EX_IGNORE);
    if (pStaticDescr == NULL) {
        _clear();
        _stat = ES_FACELESS;
    }
    else {
        _set(*pStaticDescr);
    }
    return *this;
}

cRecordAny& cRecordAny::setType(const cRecord& __o)
{
    pStaticDescr = &__o.descr();
    _set(*pStaticDescr);
    return *this;
}

cRecordAny& cRecordAny::setType(const cRecStaticDescr *_pd)
{
    pStaticDescr = _pd;
    _set(*pStaticDescr);
    return *this;
}

const cRecStaticDescr& cRecordAny::descr() const
{
    if (pStaticDescr == NULL) EXCEPTION(EPROGFAIL,-1,QObject::trUtf8("Nincs beállítva az adat típus."));
    return *pStaticDescr;
}

cRecord *cRecordAny::newObj() const
{
    cRecordAny *p = new cRecordAny();
    if (!isFaceless()) p->setType(*this);
    return p;
}
cRecord *cRecordAny::dup() const
{
    return new cRecordAny(*this);
}

cRecordAny& cRecordAny::operator=(const cRecord& __o)
{
    if (__o.isFaceless()) {
        clearType();
    }
    else {
        setType(__o);
        _cp(__o);
    }
    return *this;
}

cRecordAny& cRecordAny::clearType()
{
    _clear();
    pStaticDescr = NULL;
    _stat = ES_FACELESS;
    return *this;
}

int cRecordAny::updateFieldByNames(QSqlQuery& q, const cRecStaticDescr * _p, const QStringList& _nl, const QString& _fn, const QVariant& _v)
{
    cRecordAny ra(_p);
    return ra.cRecord::updateFieldByNames(q, _nl, _fn, _v);
}

int cRecordAny::updateFieldByIds(QSqlQuery& q, const cRecStaticDescr * _p, const QList<qlonglong>& _il, const QString& _fn, const QVariant& _v)
{
    cRecordAny ra(_p);
    return ra.cRecord::updateFieldByIds(q, _il, _fn, _v);
}

int cRecordAny::addValueArrayFieldByNames(QSqlQuery& q, const cRecStaticDescr * _p, const QStringList &_nl, const QString& _fn, const QVariant& _v)
{
    cRecordAny ra(_p);
    return ra.cRecord::addValueArrayFieldByNames(q, _nl, _fn, _v);
}
