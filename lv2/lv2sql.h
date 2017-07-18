#ifndef LV2SQL_H
#define LV2SQL_H

#include "lv2_global.h"
#include <QtCore>
#include <QtSql>

/*!
Összefűzi a két stringet, a két érték közé beszúrva egy '.' karaktert.
@par Például.:
@code
QString a = "aaa";
QString b = "bbb";
QString c = mCat(a,b);  // c = "aaa.bbb"
@endcode
 */
static inline QString  mCat(const QString& a, const QString& b)
{
    return a + QChar('.') + b;
}
/*!
Kettős idézőjeleket szúr az első karakter elé, és az utolsó után.
@par Például.:
@code
QString a = "aaa";
QString c = dQuoted(a);  // c = "\"aaa\""
@endcode
 */
static inline QString dQuoted(const QString& __s)
{
    return QChar('"') + __s  + QChar('"');
}
/*!
Egyszeres idézőjeleket szúr az első karakter elé, és az utolsó után.
@par Például.:
@code
QString a = "aaa";
QString c = quoted(a);  // c = "'aaa'"
@endcode
 */
static inline QString quoted(const QString& __s)
{
    return QChar('\'') + __s  + QChar('\'');
}

/*!
Törli a idézőjelet, ha van. Az idézőjeleket csak az elő és utolsó karakterben keresi,
és az lehet egyszeres és kéttős idézőjel is, de csak azonos.
 */
static inline QString unQuoted(const QString& name)
{
    if      (name.startsWith(QChar('\'')) && name.endsWith(QChar('\''))) return name.mid(1, name.size() -2);
    else if (name.startsWith(QChar('"'))  && name.endsWith(QChar('"')))  return name.mid(1, name.size() -2);
    return name;
}

/*!
Összefűzi a két stringet, a két érték közé beszúrva egy '.' karaktert.
Majd kettős idézőjeleket szúr az első karakter elé, és az utolsó után.
@par Például.:
@code
QString a = "aaa";
QString b = "bbb;
QString c = dQuoted(a);  // c = "\"aaa.bbb\""
@endcode
 */
static inline QString dQuotedCat(const QString& a, const QString& b)
{
    return mCat(dQuoted(a), dQuoted(b));
}

/*!
Zárójelbe teszi a kapott stringet
@par Például.:
@code
QString a = "aaa";
QString c = parentheses(a);  // c = "(aaa)"
@endcode
 */
static inline QString parentheses(const QString& __s)
{
    return QChar('(') + __s  + QChar(')');
}

/*!
A lanView objektumpékdány pDb adattag értékével tér vissza, a fő thread esetén.
Ha még nincs létrehozva a lanView objektum (instance pointer értéke NULL), akkor dob egy kizárást.
Ha a fő thread-ből hívjuk. és a pDb adattag értéke NULL, vagy az adatbázis nincs nyitva, szintén dob egy kizárást.
@relates lanView
 */
EXT_ QSqlDatabase *  getSqlDb(void);

/*!
Allokál egy új QSqlQuery objektumot, a megadott vagy a getSqlDb() által visszaadott QSqlDatabase objektumhoz.
@relates lanView
 */
static inline QSqlQuery  *newQuery(QSqlDatabase *pDb = NULL) { return new QSqlQuery(*(pDb == NULL ? getSqlDb() : pDb)); }
/*!
Visszaad egy új QSqlQuery objektumot, a megadott vagy a getSqlDb() által visszaadott QSqlDatabase objektumhoz.
@relates lanView
 */
static inline QSqlQuery   getQuery(QSqlDatabase *pDb = NULL) { return     QSqlQuery(*(pDb == NULL ? getSqlDb() : pDb)); }

/// A thread-okhoz rendelt adatbázis objektumok körül törli a megadott nevű elemet
EXT_ void dropThreadDb(const QString& tn, enum eEx __ex = EX_ERROR);

/*!
Beolvas egy sql szövegfájlt, SQL parancsokra tördeli, és végrehalytja egyenként.
Hiba esetén, ha __e értéke true (vagy nem adtuk meg), akkor dob egy kizárást.
Ha file nincs megnyitva, akkor olvasásra megnyitja, és nem zárja be.
Feltételezi, hogy egy sorban noncs több SQL parancs, vagyis a kommentek és szóközök leválasztása után a pontosvessző
a sor végén lessz, minden parancs utolsó sorában.
Ha hibát dob, akkor ha a lanView objektum már létezik, a hiba objektum (cError) pointerét beírja a lastError adattagba.
Ha lanView objektumpéldány nem létezik, és __e értéke hamis, a hiba objektum pointerét eldobja.
Ha létrejön hiba objektum, azt nem szabadítja fel. Nem ellenörzi, volt-e érték lastError-ban.
@param file A beolvasandó fájl objektum
@param pq Feltételes paraméter, az adatbázis objektum pointere, ha nincs megadva, vagy értéke NULL, akkor hívja a getSqlDb()-t.
@param __e Ha értéke igaz (ill. nincs megadva), akkor hiba esetén kizárást dob.
@return Ha nincs hiba, akkor true. Hiba esetén ha __e false, akkor false. Ha __e true, és hiba van nem tér vissza, hibát dob.
@exception cError* Ha __e értéke true, ill. nem adtuk meg az __e paramétert, akkor hiba esetén dob egy kizárást.
 */
EXT_ bool executeSqlScript(QFile& file, QSqlDatabase *pq = NULL, enum eEx __ex = EX_ERROR);

/// Tranzakció indítása
EXT_ void sqlBegin(QSqlQuery& q, const QString& tn);
/// Tranzakció befejezése
EXT_ void sqlCommit(QSqlQuery& q, const QString& tn);
/// Tranzakció visszagörgetése
EXT_ void sqlRollback(QSqlQuery& q, const QString& tn);

/// Minden nem alfanumerikus vagy nem ASCII karakter '_' karakterrel helyettesít.
/// Ha az első karakter szám, akkor beszúr elé egy '_' karaktert.
EXT_ QString toSqlName(const QString& _n);

#if 1
#define _EXECSQL(Q) \
    PDEB(SQL) << dQuoted(Q.lastQuery()) << endl; \
    if (!(Q).exec()) SQLQUERYERR(Q);

#define EXECSQL(Q, S) \
    PDEB(SQL) << dQuoted(S) << endl; \
    if (!(Q).exec(S)) SQLPREPERR(Q, S);
#else
#define _EXECSQL(Q) \
    if (!(Q).exec()) SQLQUERYERR(Q);

#define EXECSQL(Q, S) \
    if (!(Q).exec(S)) SQLPREPERR(Q, S);
#endif

/// Végrehajt egy query-t
/// @param q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
/// @param sql Az SQL query string.
/// @param v1 A query első paramétere, ha megadtuk, ha nem, akkor nincs egy paramétere sem.
/// @param v2 A query második paramétere, ha megadtuk, ha nem, akkor egy paramétere van.
/// @param v3 A query harmadik paramétere, ha megadtuk, ha nem, akkor kettő paramétere van.
/// @param v4 A query negyedik paramétere, ha megadtuk, ha nem, akkor három paramétere van.
/// @return amit a q.first() visszaad a query végrehajtása után
EXT_ bool execSql(QSqlQuery& q, const QString& sql, const QVariant& v1 = QVariant(), const QVariant& v2 = QVariant(), const QVariant& v3 = QVariant(), const QVariant& v4 = QVariant(), const QVariant& v5 = QVariant());

/// Végrehajt egy query-t
/// @param q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
/// @param sql Az SQL query string.
/// @param vl A query paraméterei
/// @return amit a q.first() visszaad a query végrehajtása után
EXT_ bool execSql(QSqlQuery& q, const QString& sql, const QVariantList& vl);

/// Végrehajt ill összeállít és végrehajt egy query-t ami egy SQL függvényhívás.
/// @param q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
/// @param fn Az SQL függvény neve. Maximum négy paramétert tudunk átadni.
/// @param v1 A hívott függvény első paramétere, ha megadtuk, ha nem, akkor a függvénynek nincs egy paramétere sem.
/// @param v2 A hívott függvény második paramétere, ha megadtuk, ha nem, akkor a függvénynek egy paramétere van.
/// @param v3 A hívott függvény harmadik paramétere, ha megadtuk, ha nem, akkor a függvénynek kettő paramétere van.
/// @param v4 A hívott függvény negyedik paramétere, ha megadtuk, ha nem, akkor a függvénynek három paramétere van.
/// @return a lekérdezés után a q.first() metódus visszatérési értéke
EXT_ bool execSqlFunction(QSqlQuery& q, const QString& fn, const QVariant& v1 = QVariant(), const QVariant& v2 = QVariant(), const QVariant& v3 = QVariant(), const QVariant& v4 = QVariant(), const QVariant& v5 = QVariant());

/// Végrehajt ill összeállít és végrehajt egy query-t ami egy SQL függvényhívás, egész szám visszatérési értékkel.
/// @param q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
/// @param pOk Egy pointer, ha nem NULL, akkor a lekérdezés eredményességét tartalmazza, true értéket kap, ha a lekérdezés eredménye egy egész szám.
/// @param fn Az SQL függvény neve. Maximum négy paramétert tudunk átadni.
/// @param v1 A hívott függvény első paramétere, ha megadtuk, ha nem, akkor a függvénynek nincs egy paramétere sem.
/// @param v2 A hívott függvény második paramétere, ha megadtuk, ha nem, akkor a függvénynek egy paramétere van.
/// @param v3 A hívott függvény harmadik paramétere, ha megadtuk, ha nem, akkor a függvénynek kettő paramétere van.
/// @param v4 A hívott függvény negyedik paramétere, ha megadtuk, ha nem, akkor a függvénynek három paramétere van.
/// @return A hívott függvény visszatérési értéke, vagy ha nincs ill. nem egész érték, akkor NULL_ID.
EXT_ qlonglong execSqlIntFunction(QSqlQuery& q, bool *pOk, const QString& fn, const QVariant& v1 = QVariant(), const QVariant& v2 = QVariant(), const QVariant& v3 = QVariant(), const QVariant& v4 = QVariant(), const QVariant& v5 = QVariant());

/// Végrehajt ill összeállít és végrehajt egy query-t ami egy SQL függvényhívás, boolean visszatérési értékkel.
/// @param q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
/// @param fn Az SQL függvény neve. Maximum négy paramétert tudunk átadni.
/// @param v1 A hívott függvény első paramétere, ha megadtuk, ha nem, akkor a függvénynek nincs egy paramétere sem.
/// @param v2 A hívott függvény második paramétere, ha megadtuk, ha nem, akkor a függvénynek egy paramétere van.
/// @param v3 A hívott függvény harmadik paramétere, ha megadtuk, ha nem, akkor a függvénynek kettő paramétere van.
/// @param v4 A hívott függvény negyedik paramétere, ha megadtuk, ha nem, akkor a függvénynek három paramétere van.
/// @return A hívott függvény visszatérési értéke.
EXT_ bool execSqlBoolFunction(QSqlQuery& q, const QString& fn, const QVariant& v1 = QVariant(), const QVariant& v2 = QVariant(), const QVariant& v3 = QVariant(), const QVariant& v4 = QVariant(), const QVariant& v5 = QVariant());

/// Végrehajt ill összeállít és végrehajt egy query-t ami egy SQL függvényhívás, szöveges visszatérési értékkel.
/// @param q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
/// @param fn Az SQL függvény neve. Maximum négy paramétert tudunk átadni.
/// @param v1 A hívott függvény első paramétere, ha megadtuk, ha nem, akkor a függvénynek nincs egy paramétere sem.
/// @param v2 A hívott függvény második paramétere, ha megadtuk, ha nem, akkor a függvénynek egy paramétere van.
/// @param v3 A hívott függvény harmadik paramétere, ha megadtuk, ha nem, akkor a függvénynek kettő paramétere van.
/// @param v4 A hívott függvény negyedik paramétere, ha megadtuk, ha nem, akkor a függvénynek három paramétere van.
/// @return A hívott függvény visszatérési értéke.
EXT_ QString execSqlTextFunction(QSqlQuery& q, const QString& fn, const QVariant& v1 = QVariant(), const QVariant& v2 = QVariant(), const QVariant& v3 = QVariant(), const QVariant& v4 = QVariant(), const QVariant& v5 = QVariant());

/// Végrehajt ill összeállít és végrehajt egy query-t ami egy SQL függvényhívás, rekord visszatérési értékkel.
/// @param q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
/// @param fn Az SQL függvény neve. Maximum négy paramétert tudunk átadni.
/// @param v1 A hívott függvény első paramétere, ha megadtuk, ha nem, akkor a függvénynek nincs egy paramétere sem.
/// @param v2 A hívott függvény második paramétere, ha megadtuk, ha nem, akkor a függvénynek egy paramétere van.
/// @param v3 A hívott függvény harmadik paramétere, ha megadtuk, ha nem, akkor a függvénynek kettő paramétere van.
/// @param v4 A hívott függvény negyedik paramétere, ha megadtuk, ha nem, akkor a függvénynek három paramétere van.
/// @return a lekérdezés után a q.first() metódus visszatérési értéke
EXT_ bool execSqlRecFunction(QSqlQuery& q, const QString& fn, const QVariant& v1 = QVariant(), const QVariant& v2 = QVariant(), const QVariant& v3 = QVariant(), const QVariant& v4 = QVariant(), const QVariant& v5 = QVariant());

EXT_ void sqlNotify(QSqlQuery& q, const QString& channel, const QString& payload = QString());


class LV2SHARED_EXPORT cNamedList {
private:
    QStringList     _names;
    QVariantList    _values;
public:
    cNamedList() :_names(), _values() { ; }
    cNamedList& add(const QString& name, const QVariant& value) { _names << name; _values << value; return *this; }
    int size() const { int r = _names.size(); if (r != _values.size()) EXCEPTION(EDATA); return r; }
    const QStringList     names() const { return _names; }
    const QVariantList    values() const { return _values; }
    const QVariant        value(int i) const { return _values.at(i); }
    QString quotes() const { QString r = QString("?,").repeated(size()); r.chop(1); return r; }
};

#endif // LV2SQL_H
