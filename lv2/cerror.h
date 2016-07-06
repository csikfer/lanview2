/***************************************************************************
 *   Copyright (C) 2005 by Csiki Ferenc                                    *
 *   csikfer@csikferpc                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef CERROR_H_INCLUDED
#define CERROR_H_INCLUDED
#include <errno.h>
#include "lv2_global.h"
#include "others.h"
#include <QtCore>
#include <QSqlError>


/*!
@file cerror.h
Hibakezelést segítő objektumok, függvények és makrók.
*/

/*!
@def LSTXGET(__lst, __ix, __v)
Egy elem lekérése egy lista/vektor -ból
@param __lst    A lista ill. vektor konténer objektum
@param __ix     Elem indexe (előjeles típus) a konténer objektumban
@param __v      Változó, ahova az indexelt elemet kértük
@throw cError*  Ha az index-en nincs valós elem. hibakód: ENOINDEX.
 */
#define LSTXGET(__lst, __ix, __v) { \
    if (__ix < 0 || __ix >= __lst.size()) EXCEPTION(ENOINDEX, (int)__ix); \
    __v = __lst[__ix]; \
}

/*!
@def LSTUXGET(__lst, __ix, __v)
Egy elem lekérése egy lista/vektor -ból
@param[in] __lst A lista ill. vektor konténer objektum
@param[in] __ix  Elem indexe (nem előjeles típus) a konténer objektumban
@param[out] __v  Változó, ahova az indexelt elemet kértük
@throw cError*  Ha az index-en nincs valós elem. hibakód: ENOINDEX.
 */
#define LSTUXGET(__lst, __ix, __v) { \
    if (__ix >= __lst.size()) EXCEPTION(ENOINDEX, (int)__ix); \
    __v = __lst[__ix]; \
}

#define NEWCERROR(ec, ...) new cError(__FILE__, __LINE__,__PRETTY_FUNCTION__,eError::ec, ##__VA_ARGS__)

/*!
@def EXCEPTION(ec...)
Létrehoz egy cError objekrumot, feltölti a paraméterek, és a hívási pozició alapján,
és az objektum pointerével dob egy kizárást.
@par Hívása:
@code
EXCEPTION(ec[, sc[, sm]]);
@endcode
@param ec Hiba kód. Egy az eError névtérben definiált enumerációs konstans érték
          ezt követheti sc az opcionális Másodlagos hiba kód, ill. numerikus hiba paraméter. Típusa int, ill. int-té lesz konvertálva,
          valamint sm az ugyancsak opcionális  Hiba szöveges paraméter. Típusa const QString&, vagy const char *.
@par Példa a használatára
@code
if (i < 0 || i >= size()) EXCEPTION(ENOINDEX, i);
@endcode
@relates cError
 */
#define EXCEPTION(ec, ...) (new cError(__FILE__, __LINE__,__PRETTY_FUNCTION__,eError::ec, ##__VA_ARGS__))->exception();

// Error codes
#include "errcodes.h"

/*!
@class cError
@brief Hiba kezelő objektum

Hiba kezelő objktum.
Az objektum makróhívásokon keresztül hozható létre, és tárolja a hiba jellemzóit, majd a makró ezzel a hiba objektummal
dob egy kizárást.
A hibakezelő makrók listálya:
  - #EXCEPTION(ec...)    álltalános hiba kezelő
  - SQLERR(o, e)        SQL hiba
  - SQLOERR(o)          SQL adatbázis megnyitási hiba.
  - SQLQUERYERR(o)      SQL query hiba (prepare utáni exec)
  - SQLPREPERR(o, t)    SQL prepare hiba
.
A cError objektum konstruktora ha érzékeli a töbszörös hibát, akkor kiírja a hiba üzenetet az stderr-re és -1 kóddal kilép,
mehelőzve ezzel egy töbszörös ciklikus hibakezelésből adódó végtelen ciklust.Ld.: circulation() metódust.
A Hiba kódok az eError névtérben vannak definiálva. Ha egy modulnak további hibakódokra van szüksége, akkor azokat is
ebben a névtérben kell definiálni. Az alap hiba kódokat az LV2_errcodes.h fejállomány tartalmazza:
@include "LV2_errcodes.h"
A fejállományban az ERRCOD és ERRCODE makrók első paramétere a hiba kód enumerációs konstans, a második paraméter pedig a
hozzá tartozó szöveges hiba string. A hiba kódokat tartalmazó fejállomány deklarációkor egy névtelen enumeráció törzsében
lessz kifelytve. Ekkor a második paramétert az aktuális ERRCOD ill. ERRCODE makró eldobja, és csak az enumerációs konstans
neve fejtődik ki az ERRCOD-ban, ill. az ERRCODE egy értékadást is tartalmaz az enumerációs értékre.
A hibakódokat tartalmazó fejállományt az errcodes.h-n keresztül lehet behúzni. Ebben a __MODUL_NAME__ konstans
alapján kerül behúzásra a hibakódokat tartalmazó állomány. Ha az errcodes.h állományt másodszor is behúzzuk,
akkor az ERRCOD és ERRCODE máshogy fejtődnek ki (definíciós mód). Az állományt egy inicializáló fügvény törzsébe kell
beilleszteni, ekkor a hibakódhosz tartozó hiba stringek beillesztődnek a cErrot objektum hibaüzenet konténerébe,
lehetővé téve hiba esetén a új hibakód szöveges megjelenítését.
Új hibakódok deklazációja (praktukusan egy fej állományban):
@code
#include "cdebug.h"
#include "cerror.h"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  MODX

#include "errcodes.h"
@endcode
A fenti esetben a modul név MODX, és a konkrét hiba kódokat és stringeket a MODX_errcodes.h fejállományba kell elhelyezni
pl.:
@code
ERRCODE( MODX_HIBA_A,   "'A' hiba a MODX-ben", __LAST_ERROR_CODE__ +1)
ERRCOD(  MODX_HIBA_B,   "'B' hiba a MODX-ben")
#undef  __LAST_ERROR_CODE__
#define __LAST_ERROR_CODE__ MODX_HIBA_B
@endcode
Ezzel további két hiba konstan-t definiáltunk az eError névtérben.
Ha azt akarjuk, hogy a cError objektum a fenti hiba kódoknál az ismeretlen hibakód helyett a fenti szövehget
jelenítse meg, akkor egy inicializáló föggvény törzzsében újra be kell húzni a fenti a errcodes.h fejállományt.
@code
void modul_error_init()
{
    #include "errcodes.h"
}
@endcode
Saját hibakódok megadásánál, amikor a megfelelő inicializáló függvénybe illesztjük be a errcodes-h állományt,
akkor a kódba illesztett cError::addErrorString() metódusok egy kizárást fognak generálni,
ha olyan hibakódot adunk meg, ami már létezik.

@def ERRCODE(id.title,i)
A makró csak az errcodes.h által inkludált modul hibakód fejállományban használható.
Azon kívül nincs definiálva, és a errcodes.h törli, ha esetleg definiálva voltak.
Fejállományban csak ezek a makrók lehetnek. Az első hibakód az ERRCODE makróval adható meg.
A többihez használható az ERRCOD makró is.
Az ERRCODE és az ERRCOD két arcú makrók. Az errcodes.h első behúzásakor egy enumeráciős típus egy elemét definiálja.
Második alkalommal pedig egy függvényhívás, ami a CError objektum hiba string konténerébe beilleszti a hibakodhoz a
hiba stringet.
A cerror.h fejállomány egyszer behuzza a errcodes.h fejállományt, ekkor a __MODUL_NAME__ értéke LV2.
Ujra definiálva a modul nevet, és törölva a ERCODES_H_DEFINE makrót, majd beillesztve az ercodes.h állományt az elöszőr megint egy enumerációt deklarációját
fogja beilleszteni. És második (összesen ugye harmadik, de töröltük ERCODES_H_DEFINE -t) beillesztésnél illeszti
be a függvényhívásokat a hiba string konténerének az új hibastringekkel való kiegészítéséhez.
@param  id  A hiba kód konstans neve (egy névtelen  enumeráció lista elemeként lessz deklarálva az eError névtérben).
@param title    A hibakódhoz tartozó hiba szting (A numerikus kód mellett ez is belekerül a hiba üzenetbe, megkünyítve a hiba értelmezését.
@param i A hiba kód numerikus értéke. Egyedibek kell lennie.

@def ERRCOD(id,title)
Ugyanúgy használható, mint az ERRCODE makró, de itt nem adjuk meg a hiba kód értékét, az egyel nagyobb lessz mint az előző.
*/
class LV2SHARED_EXPORT cError {
   public:
/*!
@brief Alapértelmezett 'üres' konstruktor

Alapértelmezett konstruktor. Minden adattagot alaphelyzetbe állít (hibakódol 0, hiba stringek nul string),
kivéve az mErrorSysCode adattagot, ebbe belásolja az errno aktuális értékét.
Az adattagok inicializálása után hívja a circulation() metódust
 */
    cError();
/*!
@brief Konstruktor.

Konstruktor. Hibakezelésnél a hibakezelő makró által hívott konstruktir.
@param _mSrcName    Forrás program fájl neve.
@param _mSrcLine    Forrás program fájlban a sor pozició.
@param _mFuncName   A függvény teljes neve
@param _mErrorCode  Numerikus hiba kód
@param _mErrorSubCode   Numerikus másodlagos hiba kód
@param _mErrorSubMsg    Szöveges másodlagos hiba kód, név, vagy azonosító.

A paraméterként megadott értékeket beírja a megfelelő adattagokba.
Az mErrorSysCode adattagba belásolja az errno aktuális értékét
Az adattagok inicializálása után hívja a circulation() metódust
*/
    /*!
    @brief Konstruktor.

    Konstruktor. Hibakezelésnél a hibakezelő makró által hívott konstruktir.
    @param _mSrcName    Forrás program fájl neve.
    @param _mSrcLine    Forrás program fájlban a sor pozició.
    @param _mFuncName   A függvény teljes neve
    @param _mErrorCode  Numerikus hiba kód
    @param _mErrorSubCode   Numerikus másodlagos hiba kód
    @param _mErrorSubMsg    Szöveges másodlagos hiba kód, név, vagy azonosító.

    A paraméterként megadott értékeket beírja a megfelelő adattagokba.
    Az mErrorSysCode adattagba belásolja az errno aktuális értékét
    Az adattagok inicializálása után hívja a circulation() metódust
    */
    cError(const char * _mSrcName, int _mSrcLine, const char * _mFuncName, int _mErrorCode,
           int _mErrorSubCode = 0, const QString& _mErrorSubMsg = QString());
    /*!
    Destruktor
    */
    virtual ~cError();
    /// Dob egy kizárást a this pointerrel.
    /// @return A metódus nem tér vissza
    virtual void exception(void);

    static void exception(const char * _mSrcName, int _mSrcLine, const char * _mFuncName, int _mErrorCode,
           int _mErrorSubCode = 0, const QString& _mErrorSubMsg = QString())
    {
        (new cError(_mSrcName, _mSrcLine, _mFuncName, _mErrorCode, _mErrorSubCode, _mErrorSubMsg))->exception();
    }

    /*! Az objektum tartalmát egy stringgé konvertálja. */
    virtual QString msg(void) const;
    /*!
      Minden konstruktor a cError inicializálása után hívja ezt a metódust.
      A metódus feladata, hogy detektálja a töbszörös hibákat, és kivédje ebben az esetben ciklikus hibakezelésből
      adódó végtelen ciklust.
      Abban az esetben, ha mMaxErrCount aktuális értékénél több cError objektumot allokálunk meg, valamint
      kétszer egymás után ugyanabban a szálban hozunk létre cError objektumot, akkor a függvény nem tér vissza,
      hanem -1-es kilépési kóddal kilép a programból.
      Kilépés elött megkisérli kiírni az stderr-re az összes eddig létrehozott cError objektum tartalmát,
      vagyis az eddigi hibaüzeneteket. Kezdve a legutolsóval, időben visszafelé haladva.
     */
    void    circulation();
    /// Az mErrorStringMap konténerhez hozzáad egy hiba kód - hiba string párost.
    /// Olyan hiba kódot nem lehet megadni, ami már van a konténerben.
    /// @exception Ha olyan hibakódot adunk meg, ami már van a konténerben.
    static void addErrorString(int __ErrorCode, const QString& __ErrorString);
    /*!
      Feltölti a mErrorStringMap konténert a hiba stringekkel. A modulokban definiált hibákat a
      modulnak kell beillesztenie a konténerbe az addErrorString(int __ErrorCode, const QString& __ErrorString)
      metódussal (ld.: errcode.h).
     */
    static void init();
    /*!
      Az aktuális hibakódhoz tartozó hiba string kikeresése a mErrorStringMap konténerből.
      Ha a konténerben nincs ilyen kulcsú hiba string, akkor az "Ismeretlen hiba kód"-al tér vissza.
     */
    static QString errorMsg(int __ErrorCode);
    QString errorMsg(void) const { return errorMsg(mErrorCode); }
    /*!
     Rendszer hiba kód string megfelelőjének a lekérése.
     */
    QString errorSysMsg(void) const { return getSysError(mErrorSysCode); }
    QString mFuncName;              ///< Function name where called EXCEPTION macro
    QString mSrcName;               ///< Source file name where called EXCEPTION macro
    int     mSrcLine;               ///< Source file line number where called EXCEPTION macro
    int     mErrorCode;             ///< Error code
    qlonglong     mErrorSubCode;          ///< Error sub code
    int     mErrorSysCode;          ///< Sytem error code (errno)
    QString mErrorSubMsg;           ///< Error sub message
    QString mThreadName;            ///< Thread name if available
    int     mSqlErrNum;
    int     mSqlErrType;            ///< SQL hiba esetén a hiba típusa
    QString mSqlErrDrText;          ///< SQL hiba esetén a driver hiba szöveg
    QString mSqlErrDbText;          ///< SQL hiba esetén a adatbázis hiba szöveg
    QString mSqlQuery;              ///< SQL hiba esetén a query string
    QString mSqlBounds;             ///< SQL Query adatok
    int     mDataLine;
    int     mDataPos;
    QString mDataMsg;               ///< Source file name
    QString mDataName;              ///< Source file name

    static cError  *pLastError;     ///< Az utolsó hiba objektum pointere, vagy NULL
    cError         *pPrevError;     ///< Az előző hiba objektum pointere, vagy NULL
    static QThread *pLastThread;    ///< Az utolsó hiba objektum ot létrehozó szál neve
    static int      mErrCount;      ///< Az eddig létrehozott cError objektumok száma
    static int      mMaxErrCount;   ///< A maximálisan létrehozhatü cError objektumok széma
    static bool     mDropAll;       ///< Ha értéke true, akkor nem hoz létre hiba objektumokat, a továbbiakban no_init_ objektummal dobja a hibát
protected:
    static  QMap<int, QString>      mErrorStringMap;///< Hiba string konténer
public:
    static const QMap<int, QString>& errorMap() { return mErrorStringMap; }
};

/// Egy SQL hiba típus konstanst stringgé kovertál.
EXT_ QString SqlErrorTypeToString(int __et);
///
static inline QString _sql_err_bound(QSqlQuery& q)
{
    QMapIterator<QString, QVariant> i(q.boundValues());
    QString r;
    while (i.hasNext()) {
        i.next();
        r += i.key() + " = " + debVariantToString(i.value()) + "; ";
    }
    if (r.size() > 0) r.chop(2);
    return  r;
}
///
static inline void _sql_err_deb_(const QSqlError& le, const char * _fi, int _li, const char * _fu, const QString& s = QString())
{
    if (ONDB(DERROR)) {
        cDebug::cout() << head << __DERRH(_fi, _li, _fu) << " ...\n";
        cDebug::cout() << QObject::trUtf8("SQL ERROR #")     << le.number()
                       << QObject::trUtf8("; type:") << SqlErrorTypeToString(le.type()) << "\n";
        cDebug::cout() << QObject::trUtf8("driverText   : ") << le.driverText() << "\n";
        cDebug::cout() << QObject::trUtf8("databaseText : ") << le.databaseText();
        if (s.size() > 0) cDebug::cout() << QObject::trUtf8("SQL string   : ") << s << "\n";
        cDebug::cout() << flush;
    }
}

static inline void _sql_err_ex(cError *pe, const QSqlError& le, const QString& sql = QString(), const QString& bound = QString())
{
    pe->mSqlErrNum    = le.number();
    pe->mSqlErrType   = le.type();
    pe->mSqlErrDrText = le.driverText();
    pe->mSqlErrDbText = le.databaseText();
    pe->mSqlQuery     = sql;
    pe->mSqlBounds    = bound;
    pe->exception();
}

/**
@def SQLERR(o, e)
Exception SQL error
@param o Objektum, amelyel kapcsolatban a hiba történt (QSqlDatabase, vagy QSqlQuery)
@param e Error code
@relates cError
 */
#define SQLERR(o, e)  { \
        QSqlError le = (o).lastError(); \
        _sql_err_deb_(le, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        _sql_err_ex(NEWCERROR(e, le.number(), le.text()), le); \
    }

/**
@def SQLOERR(o)
Exception SQL open error
@param o Objektum, amelyel kapcsolatban a hiba történt (QSqlDatabase)
@relates cError
 */
#define SQLOERR(o)  SQLERR(o,ESQLOPEN)

/**
Exception SQL error, a hibaüzenetbe beleteszi az SQL paramcsot is.
@param o Objektum, amelyel kapcsolatban a hiba történt (QSqlQuery)
@param t SQL parancs
@relates cError
 */
#define SQLPREPERR(o, t)  { \
        QSqlError le = (o).lastError(); \
        _sql_err_deb_(le, __FILE__, __LINE__, __PRETTY_FUNCTION__, t); \
        _sql_err_ex(NEWCERROR(EQUERY, le.number(), le.text()), le, t); \
    }

/**
@def SQLQUERYERR(o)
Exception SQL error, a hibaüzenetbe beleteszi az SQL paramcsot is (lekérdezi).
@param o Objektum, amelyel kapcsolatban a hiba történt (QSqlQuery)
@relates cError
 */
#define SQLQUERYERR(o)  { \
        QSqlError le = (o).lastError(); \
        _sql_err_deb_(le, __FILE__, __LINE__, __PRETTY_FUNCTION__, (o).lastQuery() + " / Bind { " + _sql_err_bound(o) + "}"); \
        _sql_err_ex(NEWCERROR(EQUERY, le.number(), le.text()), le, (o).lastQuery(), _sql_err_bound(o)); \
    }

#define CATCHS(pe) \
    catch (cError *__pe)        { pe = __pe; } \
    catch (std::exception& __e) { pe = NEWCERROR(ESTD, -1, __e.what()); } \
    catch (...)                 { pe = NEWCERROR(EUNKNOWN); }

// QString emNoField(const QString& __t, const QString& __f);


#endif // CERROR_H_INCLUDED
