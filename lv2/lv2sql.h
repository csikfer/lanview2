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
 */
static inline QString unQuoted(const QString& name)
{
    if (name.startsWith(QChar('\'')) && name.endsWith(QChar('\''))) return name.mid(1, name.size() -2);
    return name;
}
/*!
 */
static inline QString unDQuoted(const QString& name)
{
    if (name.startsWith(QChar('"')) && name.endsWith(QChar('"'))) return name.mid(1, name.size() -2);
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
static inline QSqlQuery   getQuery(QSqlDatabase *pDb = NULL) { return     QSqlQuery(*(pDb == NULL ? getSqlDb() : pDb)); }

/// A thread-okhoz rendelt adatbázis objektumok körül törli a megadott nevű elemet
EXT_ void dropThreadDb(const QString& tn, bool __ex = true);

/*!
Beolvas egy sql szövegfájlt, SQL parancsokra tördeli, és végrehalytja egyenként.
Hiba esetén, ha __e értéke true (vagy nem adtuk meg), akkor dob egy kizárást.
Ha file nincs megnyitva, akkor olvasásra megnyitja, és nem zárja be.
Feltételezi, hogy egy sorban noncs több SQL parancs, vagyis a kommentek és szóközök leválasztása után a pontosvessző
a sor végén lessz, minden parancs utolsó sorában.
Ha hibét dob, akkor ha a lanView objektum már létezik, a hiba objektum (cError) pointerét beírja a lastError adattagba.
Ha lanView objektumpéldány nem létezik, és __e értéke hamis, a hiba objektum pointerét eldobja.
Ha létrejön hiba objektum, azt nem szabadítja fel. Nem ellenörzi, volt-e érték lastError-ban.
@param file A beolvasandó fájl objektum
@param pq Feltételes paraméter, az adatbázis objektum pointere, ha nincs megadva, vagy értéke NULL, akkor hívja a getSqlDb()-t.
@param __e Ha értéke igaz (ill. nincs megadva), akkor hiba esetén kizárást dob.
@return Ha nincs hiba, akkor true. Hiba esetén ha __e false, akkor false. Ha __e true, és hiba van nem tér vissza, hibát dob.
@exception cError* Ha __e értéke true, ill. nem adtuk meg az __e paramétert, akkor hiba esetén dob egy kizárást.
 */
EXT_ bool executeSqlScript(QFile& file, QSqlDatabase *pq = NULL, bool __e = true);

/// Tranzakció indítása
EXT_ bool sqlBegin(QSqlQuery& q, bool __ex = true);
/// Tranzakció befejezése
EXT_ bool sqlEnd(QSqlQuery& q, bool __ex = true);
/// Tranzakció visszagörgetése
EXT_ bool sqlRollback(QSqlQuery& q, bool __ex = true);


#endif // LV2SQL_H
