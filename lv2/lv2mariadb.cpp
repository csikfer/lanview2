#include "lv2mariadb.h"
#include "others.h"

/* ******************************************************************************************************
   *                                         cMariaDb                                                   *
   ******************************************************************************************************/

cMariaDb *cMariaDb::_pInstance = nullptr;
const QString cMariaDb::sConnectionType = "QMYSQL";
const QString cMariaDb::sConnectionName = "GLPI";

/// Konstruktor, az adatbázis eléréséhez, csak egy példányban létezhet, elérése az init() metóduson keresztül.
/// Az adatbázis paramétereket a konfigból veszi. Ahhoz, hogy ezek a paraméterek belekerüljenek a konfigba, kell egy
/// config_ -sal kezdődő nevű rendszerváltozó. Pl.:
/// ":title=GLPI adatbázis elérése:glpi_host=text,GLPI szerver:glpi_db=text,Adatbázis név:glpi_user=passwd,Felhasználó név:glpi_psw=passwd,Adatbázis jelszó:glpi_prefix=text,Adattábla név prefix:"
/// A változó értékének a szintaxisa azonos a features mezőkkel.
/// Ha a fenti változó létezik, akkor a megadott paraméterek megjelennek a setup widget-en, és megadhatóak:
/// A title paraméter a paraméter csoport megjelenített neve. A többinél a név a config változó neve, az első érték a típus,
/// a második pedig a megjelenített név (mindíg két értéket kell megadni).
/// Az objetum által használt konfig változó nevek : glpi_host, glpi_db, glpi_user, glpi_psw, glpi_prefix
cMariaDb::cMariaDb()
    : QSqlDatabase(QSqlDatabase::addDatabase(sConnectionType, sConnectionName))
{
    QSettings& qset = *lanView::getInstance()->pSet;
    QString s;
    s = qset.value("glpi_host").toString();
    setHostName(s);
    s = qset.value("glpi_db").toString();
    setDatabaseName(s);
    s = scramble(qset.value("glpi_psw").toString());
    setPassword(s);
    s = scramble(qset.value("glpi_user").toString());
    setUserName(s);
    sTablePrefix = qset.value("glpi_prefix").toString();
    if (!QSqlDatabase::open()) {
        SQLOERR(*this);
    }
}

cMariaDb::~cMariaDb()
{
    if (isOpen()) close();
    removeDatabase(sConnectionName);
    _pInstance = nullptr;
}

bool cMariaDb::init(eEx __ex)
{
    if (_pInstance != nullptr) {
        if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
        return false;
    }
    _pInstance = new cMariaDb;
    return true;
}

bool cMariaDb::drop(eEx __ex)
{
    if (_pInstance == nullptr) {
        if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
        return false;
    }
    delete _pInstance;
    _pInstance = nullptr;
    return true;
}

cMariaDb * cMariaDb::pInstance(eEx __ex)
{
    if (_pInstance == nullptr) {
        if(__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
        _pInstance = new cMariaDb;
    }
    return _pInstance;
}

QSqlQuery cMariaDb::getQuery(eEx __ex)
{
    QSqlDatabase *pDb = pInstance(__ex);
    if (!pDb->isOpen()) EXCEPTION(EPROGFAIL);
    return QSqlQuery(*pDb);
}

QSqlQuery * cMariaDb::newQuery(eEx __ex)
{
    QSqlDatabase *pDb = pInstance(__ex);
    if (!pDb->isOpen()) EXCEPTION(EPROGFAIL);
    return new QSqlQuery(*pDb);
}

/* ******************************************************************************************************
   *                                         cColStaticMyDescr                                          *
   ******************************************************************************************************/

cColStaticMyDescr::cColStaticMyDescr(const cMyRecStaticDescr *_p, int __t)
    : cColStaticDescr(_p, __t)
{

}

cColStaticMyDescr::cColStaticMyDescr(const cColStaticMyDescr& __o)
    : cColStaticDescr(__o)
{

}

cColStaticMyDescr::~cColStaticMyDescr()
{
    ;
}

cColStaticMyDescr& cColStaticMyDescr::operator=(const cColStaticMyDescr __o)
{
    cColStaticDescr::operator=(__o);
    return *this;
}

QString cColStaticMyDescr::colNameQ() const
{
    return quoted(colName(), QChar('`'));
}

/// @def TYPEDETECT
/// A makró a colType mezőből azonosítható típusokra (tömb ezeknél a típusoknál nem támogatott)
#define TYPEDETECT(p,c)    if (0 == colType.compare(p, Qt::CaseInsensitive)) { eColType = c; return; }
void cColStaticMyDescr::typeDetect()
{
    TYPEDETECT("int",      FT_INTEGER)
    TYPEDETECT("tinyint",  FT_INTEGER)
    TYPEDETECT("smallint", FT_INTEGER)
    TYPEDETECT("bigint",   FT_INTEGER)
    TYPEDETECT("decimal",  FT_INTEGER)
    TYPEDETECT("char",     FT_TEXT)
    TYPEDETECT("varchar",  FT_TEXT)
    TYPEDETECT("longtext", FT_TEXT)
    TYPEDETECT("text",     FT_TEXT)
    TYPEDETECT("float",    FT_REAL)
    TYPEDETECT("time",     FT_TIME)
    TYPEDETECT("date",     FT_DATE)
    TYPEDETECT("datetime", FT_DATE_TIME)
    TYPEDETECT("timestamp",FT_DATE_TIME)
    TYPEDETECT("longblob", FT_BINARY)
    EXCEPTION(ENOTSUPP, -1, QObject::tr("Unknown or not supported %1 field type %2/%3 ").arg(colName(), colType, udtName));
}

#define CDDUPDEF(T)     cColStaticMyDescr *T::dup() const { return new T(*this); }

QString cColStaticMyDescr::fKeyId2name(QSqlQuery& q, qlonglong id, eEx __ex) const
{
    QString n = QString::number(id);
    QString r = "[#" + n + "]";    // If fail
    if (fKeyTable.isEmpty() == false) {
        if (pFRec == nullptr) {
            // Sajnos itt trükközni kell, mivel ezt máskor nem tehetjük meg, ill veszélyes, macerás, de itt meg konstans a pointer
            // A következő sor az objektum feltöltésekor, ahol még írható, akár végtelen rekurzióhoz is vezethet.
            // A rekurzió detektálása megvan, de kivédeni kellene, nem elég eszrevenni.
            *const_cast<cRecord **>(&pFRec) = new cMyRecAny(fKeyTable);
        }
        QString n = pFRec->getNameById(q, id , __ex);
        if (n.isEmpty()) return r;
        return n;
    }
    return r;
}


CDDUPDEF(cColStaticMyDescr)

/* ------------------------------------------------------------------------------------------------------- */
enum cColStaticMyDescr::eValueCheck cColStaticMyDescrBool::check(const QVariant& v, cColStaticMyDescr::eValueCheck acceptable) const
{
    eValueCheck r = VC_INVALID;
    if      (v.isNull() || isNumNull(v))     r = checkIfNull();
    else if (variantIsString(v))             r = strIsBool(v.toString()) ? VC_OK : VC_CONVERT;
    else if (v.type() == QVariant::Bool)     r = VC_OK;
    else if (v.canConvert(QVariant::Bool))   r = VC_CONVERT;
    return ifExcep(r, acceptable, v);
}


QVariant  cColStaticMyDescrBool::fromSql(const QVariant& _f) const
{
    if (_f.isNull()) return _f;
    bool ok;
    int i = _f.toInt(&ok);
    if (!ok) EXCEPTION(EDATA,-1,QObject::tr("Is not bool : ") + debVariantToString(_f));
    return i ? true : false;
}

QVariant  cColStaticMyDescrBool::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::tr("Data is NULL"));
    if (_f.userType() == QMetaType::Bool) return QVariant(_f.toBool() ? 1 : 0);
    bool ok;
    qlonglong i = _f.toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA,-1,QObject::tr("Az adat nem értelmezhető."));
    return QVariant(int(i));
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
QVariant  cColStaticMyDescrBool::set(const QVariant& _f, qlonglong & str) const
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
QString   cColStaticMyDescrBool::toName(const QVariant& _f) const
{
    return _f.toBool() ? _sTrue : _sFalse;
}
qlonglong cColStaticMyDescrBool::toId(const QVariant& _f) const
{
    if (isNull()) return NULL_ID;
    return _f.toBool() ? 1 : 0;
}

QString cColStaticMyDescrBool::toView(QSqlQuery&, const QVariant& _f) const
{
    return langBool(_f.toBool());
}

CDDUPDEF(cColStaticMyDescrBool)
/* ------------------------------------------------------------------------------------------------------- */

/// Tárolási adattípus QTime

cColStaticMyDescr::eValueCheck  cColStaticMyDescrTime::check(const QVariant& _f, cColStaticMyDescr::eValueCheck acceptable) const
{
    cColStaticMyDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else {
        int t = _f.userType();
        if      (t == QMetaType::QTime)  r = VC_OK;
        else if (_f.canConvert<QTime>()) r = VC_CONVERT;
        else if (variantIsNum(_f))       r = VC_CONVERT;
    }
    return ifExcep(r, acceptable, _f);
}

QVariant  cColStaticMyDescrTime::fromSql(const QVariant& _f) const
{
    // _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticMyDescrTime::toSql(const QVariant& _f) const
{
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::tr("Data is NULL"));
    return _f;
}
QVariant  cColStaticMyDescrTime::set(const QVariant& _f, qlonglong& str) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (isNumNull(_f) || _f.isNull()) {
        if (!isNullable && colDefault.isEmpty()) str |= ES_DEFECTIVE;
        return QVariant();
    }
    QTime   dt;
    if (_f.canConvert<QTime>()) dt = _f.toTime();
    else if (variantIsNum(_f)) {   // millicesc to QTime
        qlonglong h = variantIsFloat(_f) ? qlonglong(_f.toDouble()) : _f.toLongLong();
        int ms = h % 1000; h /= 1000;
        int s  = h %   60; h /=   60;
        int m  = h %   60; h /=   60;
        if (h < 24) {
            dt = QTime(int(h), m, s, ms);
        }
    }
    else if (variantIsString(_f)) {
        dt = QTime::fromString(_f.toString());
    }
    if (dt.isValid()) return QVariant(dt);
    DERR() << QObject::tr("Az adat nem értelmezhető  idő ként : ") << endl;
    str |= ES_DEFECTIVE;
    return QVariant();
}
QString   cColStaticMyDescrTime::toName(const QVariant& _f) const
{
    _DBGFN() << _f.typeName() << _sCommaSp << _f.toString() << endl;
    QTime   t = _f.toTime();
    QString s = t.toString("hh:mm:ss.zzz");
    return s;
}
/// Ezred másodperc értéket ad vissza
qlonglong cColStaticMyDescrTime::toId(const QVariant& _f) const
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

CDDUPDEF(cColStaticMyDescrTime)

/* ....................................................................................................... */

/// Tárolási adattípus QDate

cColStaticMyDescr::eValueCheck  cColStaticMyDescrDate::check(const QVariant& _f, cColStaticMyDescr::eValueCheck acceptable) const
{
    cColStaticMyDescr::eValueCheck r = VC_INVALID;
    if (_f.isNull() || isNumNull(_f)) r = checkIfNull();
    else {
        int t = _f.userType();
        if      (t == QMetaType::QDate)  r = VC_OK;
        else if (_f.canConvert<QDate>()) r = VC_CONVERT;
        else if (variantIsNum(_f))       r = VC_CONVERT;
    }
    return ifExcep(r, acceptable, _f);
}

QVariant  cColStaticMyDescrDate::fromSql(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticMyDescrDate::toSql(const QVariant& _f) const
{
    // _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) EXCEPTION(EDATA,-1,QObject::tr("Data is NULL"));
    return _f;
}
QVariant  cColStaticMyDescrDate::set(const QVariant& _f, qlonglong& str) const
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
    DERR() << QObject::tr("Az adat nem értelmezhető  idő ként : ") << endl;
    str |= ES_DEFECTIVE;
    return QVariant();
}
QString   cColStaticMyDescrDate::toName(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f.toString();
}
qlonglong cColStaticMyDescrDate::toId(const QVariant& _f) const
{
    // _DBGF() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isValid()) return 0;
    return NULL_ID;
}
CDDUPDEF(cColStaticMyDescrDate)

/* ....................................................................................................... */

cColStaticMyDescr::eValueCheck  cColStaticMyDescrDateTime::check(const QVariant& _f, cColStaticMyDescr::eValueCheck acceptable) const
{
    cColStaticMyDescr::eValueCheck r = VC_INVALID;
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
QVariant  cColStaticMyDescrDateTime::fromSql(const QVariant& _f) const
{
//    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    return _f;
}
QVariant  cColStaticMyDescrDateTime::toSql(const QVariant& _f) const
{
//    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) EXCEPTION(EDATA,-1,"Data is NULL");
    return _f;
}
QVariant  cColStaticMyDescrDateTime::set(const QVariant& _f, qlonglong& str) const
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
        static const QString sNowF = _sNOW + "()";
        if (sDt.compare(_sNOW, Qt::CaseInsensitive) == 0) return QVariant(sNowF);
        if (sDt.compare(sNowF, Qt::CaseInsensitive) == 0) return QVariant(sNowF);
        dt = QDateTime::fromString(sDt);
    }
    if (ok && dt.isNull()) {
        DERR() << QObject::tr("Az adat nem értelmezhető dátum és idő ként.") << endl;
        str |= ES_DEFECTIVE;
        return QVariant();
    }
    // _DBGFL() << " Return :" << dt.toString() << endl;
    return QVariant(dt);
}

QString   cColStaticMyDescrDateTime::toName(const QVariant& _f) const
{
 //   _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) return QString();
    if (variantIsString(_f)) return _f.toString();  // NOW
    return _f.toDateTime().toString(lanView::sDateTimeForm);
}
qlonglong cColStaticMyDescrDateTime::toId(const QVariant& _f) const
{
//    _DBGFN() << "@(" << _f.typeName() << _sCommaSp << _f.toString() << endl;
    if (_f.isNull()) return NULL_ID;
    return _f.toDateTime().toTime_t();
}

CDDUPDEF(cColStaticMyDescrDateTime)

/* ******************************************************************************************************* */

QMap<QString, cMyRecStaticDescr *>   cMyRecStaticDescr::_recMyDescrMap;

cMyRecStaticDescr::cMyRecStaticDescr(const QString &__t)
    : cRecStaticDescr()
{
    _setting(__t);
    _recMyDescrMap[__t] = this;
}

cMyRecStaticDescr::~cMyRecStaticDescr()
{
    ;
}

void cMyRecStaticDescr::_setting(const QString& __t)
{
    _DBGFN() << " " << __t << endl;
    cMariaDb::init();
    // Set table naame
    _tableName = _viewName = __t;
    _schemaName = cMariaDb::schemaName();
    QString baseName = __t;
    if (__t.startsWith(cMariaDb::tablePrefix())) baseName = baseName.mid(cMariaDb::tablePrefix().size());

    QSqlQuery   *pq  = cMariaDb::newQuery();
    QSqlQuery   *pq2 = newQuery();      // lanview2 database!!
    QString sql;
    // *********************** set scheam and Table  OID
    _schemaOId = _tableOId  = NULL_ID;
    // *********************** get Parents table(s) **********************

    // *********************** get table record
    sql = "SELECT table_type FROM information_schema.tables WHERE table_schema = ? AND table_name = ?";
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    pq->bindValue(0, _schemaName);
    pq->bindValue(1, _tableName);
    if (!pq->exec()) SQLQUERYERR(*pq);
    if (!pq->first()) EXCEPTION(EDBDATA, 0, QObject::tr("Table %1.%2 not found.").arg(_schemaName, _tableName));
    QString n = pq->value(0).toString();    // table_type
    if (pq->next()) EXCEPTION(EDBDATA, 0, QObject::tr("Table : %1,%2.").arg(_schemaName, _tableName));

    if      (n == "BASE TABLE") {
        _tableType = TT_BASE_TABLE;
    }
    else if (n == "VIEW") {     // ??
        _tableType = TT_VIEW_TABLE;
    }
    else EXCEPTION(EDBDATA, 0, QObject::tr("Invalid table type %1.%2 : %3").arg(_schemaName, _tableName, n));
    PDEB(INFO) << QObject::tr("%1 table is %2").arg(fullTableName()).arg(n) << endl;

    // ************************ get columns records
    enum { IX_COLUMN_NAME, IX_ORDINAL_POSITION, IX_COLMN_DFAULT, IX_DATA_TYPE, IX_CHARACTER_MAXIMUM_LENGHT,
           IX_IS_NULLABLE, IX_COLUMN_TYPE, IX_PRIVILEGES, IX_EXTRA };
    sql = "SELECT column_name, ordinal_position, column_default, data_type, character_maximum_length,"
                 " is_nullable, column_type, privileges, extra"
          " FROM information_schema.columns"
          " WHERE table_schema = ? AND table_name = ?"
          " ORDER BY ordinal_position";
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    pq->bindValue(0, _schemaName);
    pq->bindValue(1, _tableName);
    if (!pq->exec()) SQLQUERYERR(*pq);
    if (!pq->first()) EXCEPTION(EDBDATA, 0, QObject::tr("Table columns %1,%2 not found.").arg(_schemaName).arg(_tableName));
    _columnsNum = pq->size();
    if (_columnsNum < 1) EXCEPTION(EPROGFAIL, _columnsNum, "Invalid size.");
    _autoIncrement.resize(_columnsNum);
    _primaryKeyMask.resize(_columnsNum);
    // PDEB(VVERBOSE) << VDEB(_columnsNum) << endl;
    int i = 0;
    do {
        ++i;    // Warning, index: 1,2,...
        cColStaticMyDescr columnDescr(this);
        columnDescr.colName() = pq->value(IX_COLUMN_NAME).toString();
        columnDescr.pos       = i;
        columnDescr.ordPos    = pq->value(IX_ORDINAL_POSITION).toInt();
        columnDescr.colDefault= pq->value(IX_COLMN_DFAULT).toString();
        // PDEB(VVERBOSE) << "colDefault : " <<  (columnDescr.colDefault.isNull() ? "NULL" : dQuoted(columnDescr.colDefault)) << endl;
        columnDescr.colType   = pq->value(IX_DATA_TYPE).toString();
        columnDescr.udtName   = pq->value(IX_COLUMN_TYPE).toString();
        columnDescr.chrMaxLenghr = pq->isNull(IX_CHARACTER_MAXIMUM_LENGHT) ? -1 : pq->value(IX_CHARACTER_MAXIMUM_LENGHT).toInt();
        columnDescr.isNullable= str2bool(pq->value(IX_IS_NULLABLE).toString());
        columnDescr.isUpdatable=pq->value(IX_PRIVILEGES).toString().contains("update", Qt::CaseInsensitive);
        _isUpdatable = _isUpdatable || columnDescr.isUpdatable;
        // PDEB(INFO) << fullTableName() << " field #" << i << QChar('/') << columnDescr.ordPos << " : " << columnDescr.toString() << endl;
        // Is auto increment ? Ez nem derül ki, a mező mindíg üres.
        _autoIncrement.setBit(i -1, 0 == pq->value(IX_EXTRA).toString().compare("auto_increment", Qt::CaseInsensitive));
        // Megnézzük enum-e ... A GLPI-ben nincs, kihagyjuk
        // Deleted mező? ...    A GLPI-ben nincs, kihagyjuk
        // flag mező sincs
        // Column type
        columnDescr.typeDetect();
        // Id, name or note : fix column name
        // Detect foreign keys.  It's just not regular
        // END Regular and irregular foreign keys detect
        if (columnDescr.eColType == cColStaticMyDescr::FT_INTEGER) {
            if (0 == columnDescr.compare("id", Qt::CaseInsensitive)) {
                _idIndex = i -1;
            }
            else if (columnDescr.startsWith("is_", Qt::CaseInsensitive)) {
                columnDescr.eColType = cColStaticMyDescr::FT_BOOLEAN;
                if (0 == columnDescr.compare("is_deleted")) {
                    _deletedIndex = i -i;
                }
            }
            else {
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
                    QString t = pq2->value(3).toString();
                    columnDescr.fKeyTables = sqlToStringList(pq2->value(4));
                    if (columnDescr.fKeySchema != _schemaName
                     || columnDescr.fKeyTables.size() != 1) {
                        EXCEPTION(EDATA, 0, _schemaName + '.' + _tableName + "." + columnDescr);
                    }
                    if      (!t.compare(_sProperty, Qt::CaseInsensitive)) {
                        columnDescr.fKeyType = cColStaticDescr::FT_PROPERTY;
                    }
                    else if (!t.compare(_sOwner,    Qt::CaseInsensitive)) {
                        if ((_tableType & TT_MASK) == TT_BASE_TABLE) {
                            _tableType &= ~TT_MASK;
                            _tableType |= TT_CHILD_TABLE;
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
                else if (columnDescr.endsWith("_id", Qt::CaseInsensitive)) {
                    columnDescr.fKeySchema = _schemaName;
                    columnDescr.fKeyTable  = columnDescr;
                    columnDescr.fKeyTable.chop(3);
                    columnDescr.fKeyField  = "id";
                    columnDescr.fKeyType = columnDescr.fKeyTable == _tableName
                            ? cColStaticDescr::FT_SELF : cColStaticDescr::FT_PROPERTY;
                }
            }

        }
        if (columnDescr.eColType == cColStaticMyDescr::FT_TEXT ) {
            if (0 == columnDescr.compare("name", Qt::CaseInsensitive)) {
                _nameIndex = i -1;
            }
            else if (0 == columnDescr.compare("comment", Qt::CaseInsensitive)) {
                _noteIndex = i -1;
            }
        }
        // A konveziós függvények miatt a megfelelő típusu mező leíró objektumot kell a konténerbe rakni.
        switch (columnDescr.eColType) {
        case cColStaticDescr::FT_INTEGER:
        case cColStaticDescr::FT_TEXT:
        case cColStaticDescr::FT_REAL:      _columnDescrs << columnDescr.dup();                         break;
        case cColStaticDescr::FT_BOOLEAN:   _columnDescrs << new cColStaticMyDescrBool(columnDescr);    break;
        case cColStaticDescr::FT_DATE:      _columnDescrs << new cColStaticMyDescrDate(columnDescr);    break;
        case cColStaticDescr::FT_DATE_TIME: _columnDescrs << new cColStaticMyDescrDateTime(columnDescr);break;
        case cColStaticDescr::FT_TIME:      _columnDescrs << new cColStaticMyDescrTime(columnDescr);    break;
        default:                            EXCEPTION(EPROGFAIL);
        }
    } while(pq->next());
    if (_columnsNum != i) EXCEPTION(EPROGFAIL, -1, "Nem egyértelmű mező szám");
    // Index (there is no VIEW TABLE either)
    enum { II_Table, II_Non_unique, II_Key_name, II_Seq_in_index, II_Column_name, II_Collation, II_Cardinality,
           II_Sub_part, II_Packed, II_Null, II_Index_type, II_Comment, II_Index_comment };
    sql = "SHOW INDEX FROM " + fullTableNameQ();
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
    // PDEB(VVERBOSE) << "Read keys in " << _fullTableName() << " table..." << endl;
    if (pq->first()) {
        // CONSTRAINT név /_uniqueMasks index
        QMap<QString, int>  map;
        do {
            QString constraintName = pq->value(II_Key_name).toString();
            QString columnName     = pq->value(II_Column_name).toString();
            int     non_unique     = pq->value(II_Non_unique).toInt();
            i = toIndex(columnName, EX_IGNORE);
            if (i < 0) EXCEPTION(EDBDATA,0, QObject::tr("Invalid column name : %1").arg(fullColumnName(columnName)));
            if (0 == constraintName.compare("PRIMARY", Qt::CaseInsensitive)) {
                //PDEB(VVERBOSE) << "Set _primaryKeyMask bit, index = " << i << endl;
                _primaryKeyMask.setBit(i);
            }
            else if(0 == non_unique) {
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
    delete pq;
    delete pq2;
  // DBGFL();
}

const cMyRecStaticDescr *cMyRecStaticDescr::get(const QString& _t, bool find_only)
{
    QMap<QString, cMyRecStaticDescr *>::iterator i = _recMyDescrMap.find(_t);
    if (i != _recMyDescrMap.end()) return *i;
    if (find_only) return nullptr;
    return _recMyDescrMap[_t] = new cMyRecStaticDescr(_t);
}

QString cMyRecStaticDescr::checkId2Name(QSqlQuery&) const
{
    EXCEPTION(ENOTSUPP);
}

qlonglong cMyRecStaticDescr::getIdByName(QSqlQuery& __q, const QString& __n, eEx __ex) const
{
    QString sql = QString("SELECT %1 FROM %2 WHERE %3 = %4")
                  .arg(quoted(idName()), fullTableNameQ(), quoted(nameName()), dQuoted(__n));
    // PDEB(VERBOSE) << __PRETTY_FUNCTION__ << " SQL : " << sql << endl;
    EXECSQL(__q, sql);
    if (!__q.first()) {
        QString msg = QObject::tr("A %1 táblában nincs %2 nevű rekord (ahol a név mező %3)")
                .arg(fullTableNameQ())
                .arg(dQuoted(__n))
                .arg(dQuoted(nameName()));
        if (__ex) EXCEPTION(EFOUND, 1, msg);
        DWAR() << msg << endl;
        return NULL_ID;
    }
    QVariant id = __q.value(0);
    if (__q.next()) {
        QString msg = QObject::tr("A %1 táblában több %2 nevű rekord is van (ahol a név mező %3)")
                .arg(fullTableNameQ())
                .arg(dQuoted(__n))
                .arg(dQuoted(nameName()));
        if (__ex) EXCEPTION(AMBIGUOUS, 2, msg);
        DWAR() << msg << endl;
        return NULL_ID;
    }
    return variantToId(id, __ex, NULL_ID);
}

QString cMyRecStaticDescr::getNameById(QSqlQuery& __q, qlonglong __id, eEx ex) const
{
    QString sql = QString("SELECT %1 FROM %2 WHERE %3 = %4")
            .arg(quoted(nameName()), fullTableNameQ(), quoted(idName())). arg(__id);
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

/* ******************************************************************************************************* */


cMyRec::cMyRec()
    : cRecord()
{
    _toReadBack = _toReadBackDefault = RB_NO;
}

cMyRec::~cMyRec()
{

}

QString cMyRec::view(QSqlQuery& q, int __i, const cFeatures *pFeatures) const
{
    bool raw = pFeatures != nullptr && pFeatures->contains(_sRaw);
    if (isIndex(__i) == false) return _sNul;
    if (!isNull(__i) && pFeatures != nullptr) {
        if (pFeatures->keys().contains(_sViewExpr)) {
            QStringList args = pFeatures->slValue(_sViewExpr);
            QString expr = args.takeFirst();
            static const QString m = "$";
            if (args.isEmpty()) args << m;
            QVariantList binds;
            bool ok = true;
            foreach (QString arg, args) {
                if (arg == m) {
                    binds << descr()[__i].toSql(get(__i));
                }
                else {
                    int ix = toIndex(arg, EX_IGNORE);
                    if (ix < 0) {
                        ok = false;
                        break;
                    }
                    binds << descr()[ix].toSql(get(ix));
                }
            }
            QString r = "?!";
            if (ok && execSql(q, "SELECT " + expr, binds)) {
                r = q.value(0).toString();
            }
            return r;
        }
    }
    if (raw) return getName(__i);
    return descr()[__i].toView(q, get(__i));
}

cMac    cMyRec::getMac(int __i, eEx) const
{
    cMac r;
    r.set(getName(__i));
    return r;
}

cRecord &cMyRec::setMac(int __i, const cMac& __a, eEx )
{
    setName(__i, __a.toString());
    return *this;
}

cRecord& cMyRec::setIp(int __i, const QHostAddress& __a, eEx)
{
    setName(__i, __a.toString());
    return *this;
}

static const QString stringListSeparator = ",";

QStringList cMyRec::getStringList(int __i, eEx) const
{
    return getName(__i).split(stringListSeparator);
}

cRecord &cMyRec::setStringList(int __i, const QStringList& __v, eEx)
{
    return setName(__i, __v.join(stringListSeparator));
}

qlonglong cMyRec::fetchTableOId(QSqlQuery&, eEx __ex) const
{
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, getId(), identifying());
    return NULL_ID;
}

/* ******************************************************************************************************* */
cMyRecAny::cMyRecAny() : cMyRec()
{
    pStaticDescr = nullptr;
    _stat = ES_FACELESS;
}

cMyRecAny::cMyRecAny(const QString& _tn) : cMyRec()
{
    pStaticDescr = cMyRecStaticDescr::get(_tn);
    _set(*pStaticDescr);
}

cMyRecAny::cMyRecAny(const cMyRecStaticDescr *_p) : cMyRec()
{
    pStaticDescr = _p;
    _set(*pStaticDescr);
}

cMyRecAny::cMyRecAny(const cMyRecAny &__o)  : cMyRec()
{
    *this = static_cast<const cMyRec&>(__o);
}

cMyRecAny::cMyRecAny(const cMyRec& __o) : cMyRec()
{
    *this = __o;
}

cMyRecAny::~cMyRecAny()
{
    ;
}

cMyRecAny& cMyRecAny::setType(const QString& _tn, eEx __ex)
{
    pStaticDescr = cMyRecStaticDescr::get(_tn, __ex == EX_IGNORE);
    if (pStaticDescr == nullptr) {
        _clear();
        _stat = ES_FACELESS;
    }
    else {
        _set(*pStaticDescr);
    }
    return *this;
}

cMyRecAny& cMyRecAny::setType(const cMyRecStaticDescr *_pd)
{
    pStaticDescr = _pd;
    _set(*pStaticDescr);
    return *this;
}

const cMyRecStaticDescr& cMyRecAny::descr() const
{
    if (pStaticDescr == nullptr) EXCEPTION(EPROGFAIL,-1,QObject::tr("Nincs beállítva az adat típus."));
    return *pStaticDescr;
}

cMyRec *cMyRecAny::newObj() const
{
    cMyRecAny *p = new cMyRecAny();
    if (!isFaceless()) p->setType(static_cast<const cMyRecStaticDescr *>(&descr()));
    return p;
}
cMyRec *cMyRecAny::dup() const
{
    return new cMyRecAny(*this);
}

cMyRecAny& cMyRecAny::operator=(const cMyRec& __o)
{
    if (__o.isFaceless()) {
        clearType();
    }
    else {
        setType(static_cast<const cMyRecStaticDescr *>(&__o.descr()));
        _cp(__o);
    }
    return *this;
}

cMyRecAny& cMyRecAny::clearType()
{
    _clear();
    pStaticDescr = nullptr;
    _stat = ES_FACELESS;
    return *this;
}

int cMyRecAny::updateFieldByNames(QSqlQuery& q, const cMyRecStaticDescr * _p, const QStringList& _nl, const QString& _fn, const QVariant& _v)
{
    cMyRecAny ra(_p);
    return ra.cMyRec::updateFieldByNames(q, _nl, _fn, _v);
}

int cMyRecAny::updateFieldByIds(QSqlQuery& q, const cMyRecStaticDescr * _p, const QList<qlonglong>& _il, const QString& _fn, const QVariant& _v)
{
    cMyRecAny ra(_p);
    return ra.cMyRec::updateFieldByIds(q, _il, _fn, _v);
}

int cMyRecAny::addValueArrayFieldByNames(QSqlQuery& q, const cMyRecStaticDescr * _p, const QStringList &_nl, const QString& _fn, const QVariant& _v)
{
    cMyRecAny ra(_p);
    return ra.cMyRec::addValueArrayFieldByNames(q, _nl, _fn, _v);
}

