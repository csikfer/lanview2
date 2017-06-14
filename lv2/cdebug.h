#ifndef CDEBUG_H
#define CDEBUG_H
#include "lv2_global.h"
#include <QtCore>
#include <QtSql>
#include "strings.h"

/*!
@file cdebug.h
Nyomkövetést segítő objektumok, függvények és makrók.
*/

#if defined (Q_OS_WIN)
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

/*!
Megvizsgálja, hogy a fő szálban (main thread) vagyunk-e
@return Ha a fő szálból hívtuk meg, akkor true, egyébként false.
 */
inline static bool isMainThread()
{
    return QCoreApplication::instance()->thread() == QThread::currentThread();
}
/*!
Az aktuális thread nevével tér vissza, feltételezi, hogy minden szál kapott nevet (QObject::setObjectName()). Ha az aktuális QThread objektumnak nincs neve
(akkor, ha az a main thread, a "mainThread", ha nem, az "anonimousThread" stringgel tér vissza.
 */
inline static QString currentThreadName()
{
    QString r = QThread::currentThread()->objectName();
    return r.isEmpty() ? (isMainThread() ? _sMainThread : _sAnonimousThread) : r;
}

EXT_ QString quotedString(const QString& __s, const QChar &__q = QChar('"'));
EXT_ QString quotedStringList(const QStringList& __sl, const QChar &__q = QChar('"'), const QChar &__s = QChar(','));

/// @def ONDB(mask)
/// Segéd makró, a debug maszkot kiegészíti a modul információval.
/// @relates cDebug
#define ONDB(mask)  (cDebug::pDeb(cDebug::mask | __DMOD__))

/// @def _PDEB(mask)
/// Debug üzenet kiíratásához használlt makró. Nincs fejléc.
/// @param mask Debug üzenet kiírási feltételét definiáló maszk (modul infó nélkül)
/// @relates cDebug
#define _PDEB(mask)  if (cDebug::pDeb(cDebug::mask | __DMOD__)) cDebug::cout()

/// @def HEAD()
#define _HEAD(fi, li, fu)   fi << "[" << li << "] " << fu << " : "
/// @def _HEAD()
#define HEAD()           head << _HEAD(__FILE__, __LINE__, __PRETTY_FUNCTION__)
/// @def PDEB(mask)
/// Debug üzenet kiíratásához használlt makró.
/// @param mask Debug üzenet kiírási feltételét definiáló maszk (modul infó nélkül)
/// @relates cDebug
#define PDEB(mask)  _PDEB(mask) << HEAD()

/// @def _DBGFN()
/// Debug üzenet kiírása egy nyomkövetendő függvény kezdetén, az üzenet nincs lezárva, a sor kiegészíthető,
/// és le kell zárni egy endl-vel.
/// @relates cDebug
#define _DBGFN()    PDEB(ENTERLEAVE) << QObject::trUtf8("Enter. ")

/// @def _DBGFNL()
/// Debug üzenet kiírása egy nyomkövetendő függvény végén (a returm(-ok) előtt), az üzenet nincs lezárva, a sor kiegészíthető,
/// és le kell zárni egy endl-vel.
/// @relates cDebug
#define _DBGFNL()   PDEB(ENTERLEAVE) << QObject::trUtf8("Leave. ")

/// @def DBGFN()
/// Debug üzenet kiírása egy nyomkövetendő függvény kezdetén, az üzenet le van zárva.
/// @relates cDebug
#define DBGFN()     PDEB(ENTERLEAVE) << QObject::trUtf8("Enter.") << endl

/// @def DBGFNL()
/// Debug üzenet kiírása egy nyomkövetendő függvény végén, az üzenet le van zárva.
/// @relates cDebug
#define DBGFNL()    PDEB(ENTERLEAVE) << QObject::trUtf8("Leave.") << endl

/// @def _DBGOBJ()
/// Debug üzenet kiírása egy nyomkövetendő konstruktor vagy destruktor meghívásakor, az üzenet nincs lezárva, a sor kiegészíthető,
/// és le kell zárni egy endl-vel.
/// @relates cDebug
#define _DBGOBJ()    PDEB(OBJECT) << VDEBPTR(this)

/// @def DBGOBJ()
/// Debug üzenet kiírása egy nyomkövetendő konstruktor vagy destruktor meghívásakor, az üzenet le van zárva.
/// @relates cDebug
#define DBGOBJ()    _DBGOBJ() << endl

#define __DERRH(fi, li, fu) head << QObject::trUtf8("Error in ") << _HEAD(fi, li, fu)
#define _DERRH()    __DERRH(__FILE__, __LINE__, __PRETTY_FUNCTION__)
/// @def DERR()
/// Hasonló a PDEB() makróhoz, de itt a mask (PDEB() paramétere) DERROR, és kiírja az
/// aktuális forrásfájl nevét abban a sotrszámot, valamint az aktuális függvény teljes nevét is.
/// @relates cDebug
#define DERR()      _PDEB(DERROR)   << _DERRH()
/// @def DSYSERR()
/// Hasonló a DERR() makróhoz, de még kiírja az aktuálsz rendszer hiba üzenetet.
/// Az errno aktuális ártéke alapján.
/// @relates cDebug
#define DSYSERR()   _PDEB(DERROR)   << _DERRH() << qStr(strerror(errno))
/// @def DWAR()
/// Hasonló a PDEB() makróhoz, de itt a mask (PDEB() paramétere) WARNING, és kiírja az
/// aktuális forrásfájl nevét abban a sotrszámot, valamint az aktuális függvény teljes nevét is.
/// @relates cDebug
#define __DWARH(fi, li, fu) head << QObject::trUtf8("Warning in ") << _HEAD(fi, li, fu)
#define _DWARH()    __DWARH(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#define DWAR()      _PDEB(WARNING) << _DWARH()

/// @def DSYSWAR()
/// Hasonló a DWAR() makróhoz, de még kiírja az aktuálsz rendszer hiba üzenetet.
/// Az errno aktuális ártéke alapján.
/// @relates cDebug
#define DSYSWAR()   _PDEB(WARNING) << _DWARH << " : " << qStr(strerror(errno))

/// @def DBOOL(b)
/// Egy bool típusú érték konvertálása a 'true' cagy 'false' stringgé.
#define DBOOL(b)    ((b)?_sTrue:_sFalse)
/// @def VDEB(n)
/// Kiíratásnál használható makró, egy változó nevének és értékének a kiíratásához.
#define VDEB(n)     " " #n " = " << n
/// @def VDEBBOOL(n)
/// Kiíratásnál használható makró, egy bool változó nevének és értékének a kiíratásához.
#define VDEBBOOL(n) " " #n " = " << DBOOL(n)
/// @def VDEBSTR(n)
/// Kiíratásnál használható makró, egy string változó nevének és értékének a kiíratásához.
/// Az értéket kettős idézőjelbe teszi, és a kontrol karaktereket esc-szekveniává konvertálja
#define VDEBSTR(n)  " " #n " = " << quotedString(n)
/// @def VDEBPTR(n)
/// Kiíratásnál használható makró, egy pointer változó nevének és értékének a kiíratásához.
#define VDEBPTR(n)  " " #n " = " << p2string(n)
/// @def VDEBTOSTR(n)
/// Kiíratásnál használható makró, egy objektum nevének és értékének a kiíratásához, az objektum toString() metődusát hivja
#define VDEBTOSTR(n) " " #n " = " << debVariantToString(n)

template <class T> QString debPToString(T *p) {
    if (p == NULL) return "[NULL]";
    return quotedString(p->toString());
}
inline static QString debPString(QString *p) { return p == NULL ? "[NULL]" : quotedString(*p); }

/// @def VDEBPTOSTR(n)
/// Kiíratásnál használható makró, egy pointer változó nevének és értékének a kiíratásához.
/// A típust a toString() metódus konvertálja stringgé
#define VDEBPTOSTR(n)     " " *#n " = " << debPToString(n)

/// @def VDEBPSTR(n)
/// Kiíratásnál használható makró, egy string pointer változó nevének és értékének a kiíratásához.
#define VDEBPSTR(n)     " " *#n " = " << debPString(n)

/// @def VDEBSTRPAIR(n)
/// Kiíratásnál használható makró, egy string pointer változó nevének és értékének a kiíratásához.
#define VDEBSTRPAIR(n)     " " *#n " = " << quotedString(n.first) << "&" << quotedString(n.second)

/// @def __DMOD__
/// Az aktuális modul debug maszk beli bitjét definiáló konstans nevét tartalmazza
/// A konstans a __MODUL_NAME__ újradefiniálásán keresztúl változtatható meg.
/// @relates cDebug
#define __DMOD__        cDebug::__MODUL_NAME__
/// @def __MODUL_NAME__
/// Az aktuális modul nevet definiálja.
/// 'LV2'-re van definiálva, ezt más modul hederében újra kell definiálni.
/// @relates cDebug
#define __MODUL_NAME__  LV2

/*!
@def SQLQUERYERRDEB(o)
SQL error, a hibaüzenetbe beleteszi az SQL paramcsot is.
Nem kizárás, a PDEB(DERROR) makróval csak a hibaüzenetet írja ki.
@param o Objektum, amelyel kapcsolatban a hiba történt (QSqlQuery)
@relates cDEBUG
@see DERR()
*/
#define SQLQUERYERRDEB(o)  { \
    QSqlError le = (o).lastError(); \
    DERR() << QObject::trUtf8("SQL PREPARE ERROR #") << le.number() << "\n" \
    << QObject::trUtf8("driverText   : ") << le.driverText() << "\n" \
    << QObject::trUtf8("databaseText : ") << le.databaseText() << "\n" \
    << QObject::trUtf8("SQL string   : ") << (o).lastQuery () << endl; \
  }

/**
@def SQLPREPERRDEB(o, t)
SQL error, a hibaüzenetbe beleteszi az SQL paramcsot is (lekérdezi).
Nem kizárás, a PDEB(DERROR) makróval csak a hibaüzenetet írja ki.
@param o Objektum, amelyel kapcsolatban a hiba történt (QSqlQuery)
@param t SQL parancs
@relates cDebug
@see DERR()
*/
#define SQLPREPERRDEB(o, t)  { \
    QSqlError le = (o).lastError(); \
    DERR() << QObject::trUtf8("SQL PREPARE ERROR #") << le.number() << "\n" \
    << QObject::trUtf8("driverText   : ") << le.driverText() << "\n" \
    << QObject::trUtf8("databaseText : ") << le.databaseText() << "\n" \
    << QObject::trUtf8("SQL string   : ") << t << endl; \
  }

/*!
  @class debugStream
  A debug üzenetek kiírásánál használt stream jellegű objektum.
  Minden szálhoz tartozik egy debugStream, ha a szál használja a cDebug objektumot.
  Csak a fő szálhoz tartozó debugStream objektum végez io műveletet. A többi szál esetén
  a debugStream csak puffereli az üzeneteket, és ez csak a flush metódus meghívása után,
  egy soron (QQueue) keresztűl íródik ki a fő szálhoz tartozó debugStream-en keresztül.
  Ha be van állítva a GUI mód (cDebug::setGui(bool)), akkor a fő szálhoz tartozó
  debugStream azon túl, hogy kiírja az üzenetet a megadott fájlba, vagy konzolra, azt beteszi
  egy sorba, és kiad egy readyDebugLine() szignált, és a szignál fogadója az üzenetet egy
  cDebug::dequeue() hívással kaphatja meg.
  */
class LV2SHARED_EXPORT debugStream : public QObject {
    Q_OBJECT
    friend class cDebug;
protected:
    /// Egy szálhoz tartozó debugStream objektum konstruktora
    /// @param pMain A fő szálhoz tartozó debugStream objektum pointere.
    /// @param pThread A szél objektum pointere.
    debugStream(debugStream *pMain, QObject *pThread);
    /// A fő szálhoz tartozó debugStream objektum konstruktora
    /// @param __f a kimeneti fájl pointere
    debugStream(QFile *__f);
    /// A fő szálhoz tartozó debugStream objektum konstruktora
    /// @param __f a kimeneti fájl pointere
    debugStream(FILE  *__f);
    /// destruktor
    ~debugStream();
public:
    /// Az üzenet kiírása. A fő szálban egy flush() az aktuális kimeneti sztrímre.
    /// Ugyancsak a fő szálban, ha GUI mód van beállítva, akkor az aktuális üzenetet beteszi egy sor-ba,
    /// és kiad egy readyDebugLine() szignált.
    /// Mellék szál esetén az üzenet egy sorba karül, amit majd a fő szál debugSztream-ja ír ki.
    /// A sor puffer törlődik.
    debugStream& flush();
    bool isMain() { return mainInstance == this; }
    /// Az aktuális valódi stream objektum
    QTextStream stream;
protected:
    /// A stream-hez hozzárendelt string (csak írható módban)
    /// Ez a debug üzenet sor buffere.
    QString     buff;
    /// A debug üzenetek cél fájlja. Minden sor végén ide íiródik ki a debug üzenet
    /// Ha NULL, akkor az objektum egy szálhoz van rendelve, és közvetlenül nem végez io müveletet
    QFile *     oFile;
    /// Ha a cél fájl egy FILE * típusú értékkel lett megadva, akkor annak az értéke.
    /// Jelzi, hogy az oFile-t az objektum konstruktor allokálta.
    FILE *      pFILE;
    /// A fő szálhoz tartozó debugStream objektum pointere.
    static debugStream *mainInstance;
signals:
    /// Ha GUI módban vagyunk, akkor itt jelezzük, hogy kész egy debug üzenet sor.
    void readyDebugLine();
    /// Egy mellék szálhoz tartozó debugStream objektum ezzel a szignállal jelzi a fő szálnak, hogy vban egy kész üzenet sora.
    void redyLineFromThread();
private slots:
    void sRedyLineFromThread();
    void sDestroyThread(QObject * __p);
};

/*!
@brief A debugStream manipulátorait végrehalytó operátor-
@relates debugStream
@relates cDebug
 */
static inline debugStream& operator<< (debugStream& __s, debugStream &(*__pf)(debugStream &)) { return (*__pf)(__s); }

/*!
@brief debugStream manipulátor template.
@relates debugStream
@relates cDebug
debugStream manipuéátor. A QTextStream manilulátorainak használatát lehetővétevő
template függvények.
*/
template<class T> debugStream& operator<<(debugStream& s, const T& v)  { s.stream << v; return s; }

/*!
@brief A debugStream manipulátor. A sor puffer kiküldése, és törlése.
@relates debugStream
@relates cDebug
A sor puffert kiküldi, és üríti ld.: debugStream::flush()
 */
inline debugStream & flush(debugStream & __ds) { return __ds.flush(); }
/*!
@brief A debugStream manipulátor. A sor puffer lezárása egy soremelés karakterrel, kiküldése, és törlése.
@relates debugStream
@relates cDebug
A sorpufferbe kiír egy soremelés karaktert, majd hívja a flush(debugStream & __ds) manipulátort
 */
static inline debugStream &  endl(debugStream & __ds) { return __ds << QChar('\n') << flush; }

/*!
@brief debugStream manipulátor. Azonosító szövek kiírása
@relates debugStream
@relates cDebug
debugStream manipuéátor.
Egy debug üzenet sor azonosítását lehetővé tevő szövehget köld ki a debugStream-be.
Az azonosító szövek tartalma:
  - az aktuális dátum
  - az aplikáció neve
  - a program PID-je
  - ha nem a fő szál, akkor a szál neve
.
@param  __ds A debugStream objektum referenciája
@return A paraméterként megadott objektum referencia,
*/
EXT_ debugStream &  head(debugStream & __ds);

/*!
@class cDebug
A nyomkövetési üzenetek kiírását teszi lehetővé. Az üzeneteket a debugStream QTextStream-hoz hasonó
objektumon keresztül valósítja meg. Csak egy objektumpéldány létezhet.
A nyomkövetési üzenetek elhelyezésére a PDEB(m) makró használható:\n
Ahol m az üzenethez tartozó maszk, a modult azonosító bit nélkül.
A makró egy if utasítás, aminek utasítéstörzse nincs utasítászárójelbe,
az eggyetlen kifelyezés, melynek balértáke egy debugStream objektum.
Az üzenet nem kerül kiíratásra (az if feltétel hamis), ha a megadott maszk (plussz a modul azonosító bit) összes bitje
nem szerepel az aktuális debug maszkban.
A makróban az if utáni kifelyezés : 'cDebug::cout() << head' ld.: debugStream &  head(debugStream & __ds)
A makró úgy használható, mintha egy debugSrteam objektum lenne (ami részben a QTextStrea felületét valósítja meg).
pl.:
@code
PDEB(INFO) << "Ez egy információs üzenet" << endl;
@endcode
További a PDEB()-hez hasonló módon használható makrók:
  - _DBGFN()    Kiírja az "Enter: " szöveget és a függvény nevét is. mask = ENTERLEAVE
  - _DBGFN()    Kiírja az "Leave: " szöveget és a függvény nevét is. mask = ENTERLEAVE
  - DERR()      Kiírja az "Error in: " szöveget, valamint a forrás fájl nevét, abban a pozíciót, az aktuális függvény nevét. mask = DERROR
  - DSYSERR()   Hasonló, mint az előző, de kiírja a rendszer hiba üzenetet is (errno értéke alapján). mask = DERROR
  - DWAR()      Kiírja a "Warning in " szöveget és az aktuális függvény nevét. mask = WARNING
  - DSYSWAR()   Hasonló, mint az előző, de kiírja a rendszer hiba üzenetet is (errno értéke alapján). mask = WARNING
.
További nyomkövetésben használható makrók:
  - DBGFN()     Hasonló mint a _BDGFN(), de az üzenetet lezárja egy endl-al.
  - DBGFNL()    Hasonló mint a _BDGFNL(), de az üzenetet lezárja egy endl-al.
  - SQLQUERYERRDEB(o)  SQL query hiba kiíratása a DERR() makróval
  - SQLPREPERRDEB(o, t) SQL prepare hiba kiíratása a DERR() makróval
  - DBOOL(b)    egy logikai érték kiírása (a 'true' vagy 'false' szöveg fig kiíródni).
    @code
    PDEB(INFO) << "Alogikai érték : " << DBOOL(x) << endl;
    @endcode
  - VDEB(n)     váltózó kiírása <változó név> = <változó értéke> formában. Az érták az alapértelmezett formában íródik ki.
  - VDEBBOOL(n) Hasonló az előzőhöz, de a változó értéke logikai értékként iródik ki ld.: DBOOL() makró.
  - VDEBSTR(n)  Hasonló az előzőhöz, de a változó értéke idézőjelek közé lessz téve
  - VDEBPTR(n)  Hasonló az előzőhöz, de a változónak pointernek kell lennie, és a pointer értéke (a cím) íródik ki.
-
*/
class LV2SHARED_EXPORT cDebug {
    friend class debugStream;
   public:
    /*!
     Debug maszk értékek és bitek definíciüi
     */
    typedef enum {
        SILENT     =  0x00000000UL,   ///< Debug mask érték: Nincsennek debug üzenetek
        EXCEPT     =  0x00000001UL,   ///< Debug mask bit: A fatális hibáknál a debug üzeneteket is kiírja
        DERROR     =  0x00000002UL,   ///< Debug mask bit: A nem fatális hiba üzenetek kiírása (mindíg kiírja!)
        WARNING    =  0x00000004UL,   ///< Debug mask bit: A figyelmeztető üzenetek kiírása
        INFO       =  0x00000008UL,   ///< Debug mask bit: A információs üzenetek kiírása
        VERBOSE    =  0x00000010UL,   ///< Debug mask bit: A beszédesebb információs üzenetek kiírása
        VVERBOSE   =  0x00000020UL,   ///< Debug mask bit: A mégbeszédesebb információs üzenetek kiírása
        ENTERLEAVE =  0x00000040UL,   ///< Debug mask bit: A függvények (ahol van) be és kilépési üzenetek liírása
        PARSEARG   =  0x00000080UL,   ///< Debug mask bit: Az argumentum feldolgozás debug üzeneteinek a kiírása
        SQL        =  0x00000100UL,   ///< Debug mask bit: Az SQL műveletek debug üzeneteinek a kiírása
        OBJECT     =  0x00000200UL,   ///< Debug mask bit: Az objektumokkal kapcsolatos debug üzenetek kiírása
        ADDRESS    =  0x00000400UL,   ///< Debug mask bit: A címkezeléssel kapcsolatos debug üzenetek kiírása
        SNMP       =  0x000008000L,   ///< Debug mask bit: Az SNMP lekérdezés
        ALL        =  0xffffffffUL,   ///< Debug mask érték: Minden üzenet kiírása
        LV2        =  0x40000000UL,   ///< Debug mask bit: Az lv2 modul üzeneteinek a kiírása
        LV2G       =  0x20000000UL,   ///< Debug mask bit: Az lv2g modul üzeneteinek a kiírása
        PARSER     =  0x10000000UL,
        APP        =  0x08000000UL,   ///< Debug mask bit: Az APP  üzeneteinek a kiírása
        MODMASK    =  0xFF000000UL
/*        TEST       =  APP,
        LV2D       =  APP,
        LV2GUI     =  APP,
        ICONTSRV   =  APP,
        IMPORT     =  APP,
        PORTSTAT   =  APP,
        IAGUI      =  APP,
        ARPD       =  APP,
        PORTMAC    =  APP,
        UPDT_OUI   =  APP,*/
    }   eMask;

    /*! @brief A debug rendszer inicializálása, ill. újra inicializálása.
     *
     *  Létrehozza a cDebug opbjektumot.
     *  Ha már létezett a cDebug objektum, akkor törli azt, majd újra létrehozza.
     *  @param fn a debug üzeneteket ide kell kiírni. Ha értéke "-", vagy "stdout", akkor a sztenderd kimenetre,
     *          ha értéke "stderr", vagy üres, akkor a sztenderd hiba kimenetre, egyébként a megadott nevű fájlba.
     *  @param m Debug maszk. Definiálja mely üzenetek kerülnek kiírásra.
     *  @sa cDebug::cDebug(qlonglong _mMask, const QString& _fn)
     */
    static void init(const QString& fn, qlonglong m);
    /*! @brief A debug üzenet feltételeit vizsgáló fügvény.
     *
     *  Ha még nincs létrehozva a cDebug objektum, akkor létrehozza az alapértelmezett objektumot.
     *  Egy üzenet akkor kerül kiírásra, ha a az aktuális maszkban minden a paraméterben megydott bit be van kapcsolva.
     *  @param m  a debug maszk (az üzenet kategóriájt, és a modult) definiáló maszk)
     *  @return true ha a megadott maszk alapján az üzenetet ki kell írni.
     */
    static bool pDeb(qlonglong mask);
    /// Az aktuális debug objektum, fő szálához tartozó debugStream objektumának a pointerével tér vissza
    /// @throw cError*  Feltételezi, hogy van cDebug objektum (cDebug::instance != NULL).
    ///                 Valamint mCout nem NULL pointer.
    ///                 Ellenkerő esetben dob egy kizárást (hibakód : EPROGFAIL)
    static debugStream * pCout(void);
    /// Az aktuális debug objektum egy debugStream objektumának a referenciájával tér vissza.
    /// Ha a fő szálban vagyunk, akkor a fő szálhoz tartozó objektum referenciával @sa _cout(void)
    /// Ha nem a fő szálban vagyunk, akkor megkeresi a szálhoz tartozó debugStream objektumot, ill.
    /// létrehozza azt, ha még nem létezik, és azzal tér vissza.
    /// Ha nem létezik a szálak debugStream objektumainak a tárolója (mThreadStreamsMap == NULL),
    /// akkor létrehozza azt is, a QMutex objektummal együtt (mThreadStreamsMapMutex).
    /// @throw cError*  Feltételezi, hogy van cDebug objektum (cDebug::instance != NULL).
    ///                 Valamint mCout nem NULL pointer.
    ///                 Ellenkerő esetben dob egy kizárást (hibakód : EPROGFAIL)
    static debugStream& cout(void);
    /// Megvizsgálja, hogy létezik-e a cDebug objektum (cDebug::instance != NULL)
    /// @throw cError Ha nincs cDebug objektum (cDebug::instance == NULL), hibakód: EPROGFAIL
    static void chk();
    /// Törli a megadott debug maszk biteket.
    /// @param __m A paraméterben lévő 1-es biteket törli az aktuális maszkban.
    /// @return A maszk új értéke.
    /// @throw cError Ha nincs cDebug objektum (cDebug::instance == NULL), hibakód: EPROGFAIL
    static qlonglong off(qlonglong __m) { chk(); return instance->mMask &= ~__m; }
    /// Beállítja a megadott debug maszk biteket.
    /// @param __m A paraméterben lévő 1-es biteket állítja 1-be az aktuális maszkban.
    /// @return A maszk új értéke.
    /// @throw cError Ha nincs cDebug objektum (cDebug::instance == NULL), hibakód: EPROGFAIL
    static qlonglong on(qlonglong __m)  { chk(); return instance->mMask |=  __m; }
    /// Beállítja az új debug maszkot.
    /// @param __m Az új aktuális maszk.
    /// @return A maszk új értéke.
    /// @throw cError Ha nincs cDebug objektum (cDebug::instance == NULL), hibakód: EPROGFAIL
    static qlonglong set(qlonglong __m) { chk(); return instance->mMask  =  __m; }
    //! Ha van aktuális cDebug objektum, akkor törli.
    //! A disables statikus adattagot true-ra állítja
    static void end(void) { if (instance) { delete instance; instance = NULL; }; disabled = true; }
    /// Az aktuális cDebug objektum pointerének a lekérdezése.
    /// @return Az aktuális cDebug pointer. Ha nincs objektum, akkor NULL
    static cDebug *getInstance(void) { return instance; }
    /// A megadott debug mask bitek nevének a listályával tér vissza
    /// @return A visszaadott lista objektum, a bit nevek vesszővel vannak elválastva.
    /// A lista szgletes zárójelbe van téve, és a záró karakter egy Space. pl.: "[bit1,bit3,bitn] ".
    static QString maskName(qlonglong __msk);
    /// GUI mód beállítása.
    static void setGui(bool __f = true);
    /// Gui módban az üzenet sorból a következő üzenet lekérése.
    /// @return A sorból lekért üzenet objektum.
    /// Ha az üzenetsor öres, akkor a "***** *cDebug::instance->mMsgQueue is empty *****" tartalmú stringet adja vissza.
    /// @throw cError* Ha nincs cDebug objektum (cDebug::instance == NULL), vagy az üzenet sor, ill. a hozzá tartozó
    /// QMutex objektum (pl. mert nem true-val lett meghívva a cDebug::setGui(bool __f) metódus )
    static QString dequeue();
    /// Fájl név konverzió.
    /// @param __fn A konvertálandó fájlnév.
    /// @return Ha __fn értéke '-', vagy nem kisbetú érzékenyen 'stdout', akkor 'stdout' stringgel tár vissza.
    ///         Ha __fn nem kisbetú érzékenyen 'stderr', akkor 'stderr' stringgel tár vissza.
    ///         Minden más esetben __fn-el tér vissza.
    static QString fNameCnv(const QString& __fn);
    /// Az aktuális kimeneti fájl nevével, vagy az 'stdout' stringgel, ha a debug üzenetek az stdout-ra,
    /// ill. az 'stderr' stringgel, ha a debug üzenetek az stderr-re mennek.
    /// @throw cError* ha nincs cDebug objektum.
    static const QString& fName()       { chk(); return instance->mFName; }
   protected:
     /*!
      * @brief A cDebug objektum alapértelmezett konstruktora.
      *
      * A felhasználói programból közvetlenül nem hívható. Ha inicializálás elött kerül meghívásra a pDeb()
      * metódus, akkor ezzel hozza létre ideiglenesen (az init() törli) a cDebug objektumot.
      */
    cDebug();
    /*!
     * @brief A cDebug konstruktora.
     *
     * A felhasználói programból közvetlenül nem hívható. Az init() hívja.
     */
    cDebug(qlonglong _mMask, const QString& fn);
    /*! A cDdebug objektum destruktora.
     *
     * Hasonlóan a konstruktorokhoz, csak statikus metóduson keresztül hívható.
     */
    ~cDebug();
    QString                 mFName; ///< A kimeneti fájl neve
    QFile                  *mFile;  ///< A kimeneti fájl objektum pointere
    qlonglong               mMaxLogSize;
    int                     mArcNum;
    qlonglong               mMask;  ///< Aktuális debug maszk.
    static cDebug          *instance;   ///< a cDebug példány pointere, vagy NULL.
    static bool             disabled;   ///< Ha értéke true, akkor a debug tiltva.
    debugStream            *mCout;      ///< A fő szál debugStream objektuma
    QQueue<QString>        *mMsgQueue;  ///< GUI módban minden üzenetsor ebbe a sorba is bekerül.
    mutable QMutex         *mMsgQueueMutex; ///< Mutex objektum az *mMsgQueue objektumhoz.
    QMap<QString, debugStream *> *mThreadStreamsMap;  ///< Szálak Név-objektum pointer tárolója
    mutable QMutex         *mThreadStreamsMapMutex; ///< Mutex objektum az *mThreadStreamsMap objektumhoz.
    QQueue<QString>        *mThreadMsgQueue;    ///< A szálak ezen a soron keresztül küldik el a fő szálnak az üzeneteket.
    mutable QMutex         *mThreadMsgQueueMutex;   ///< Mutex objektum az *mThreadMsgQueue objektumhoz.
};

/*!
  Egy pointer értékének a stringé konvertálása
  */
static inline QString p2string(void * __p) { return QString().sprintf("%p", __p); }
/// Egy QVariantList típusú árték stringgé konvertálása egy debug vagy hiba üzenetben.
EXT_ QString list2string(const QVariantList& __vl);
/// Egy QBitArra típusú árték stringgé konvertálása egy debug vagy hiba üzenetben.
EXT_ QString list2string(const QBitArray& __vl);
/// Egy QStringList típusú árték stringgé konvertálása egy debug vagy hiba üzenetben.
EXT_ QString list2string(const QStringList& __vl);

EXT_ QString debVariantToString(const QVariant& v);


#endif // CDEBUG_H
