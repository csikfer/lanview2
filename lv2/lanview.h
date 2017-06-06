#ifndef LANVIEW_H
#define LANVIEW_H

/*!
@file lanview.h
@author Csiki Ferenc
@brief Az API init objektuma
*/

#include <signal.h>

#include "lv2_global.h"
#include "strings.h"
#include "cdebug.h"
#include "cerror.h"
#include "lv2types.h"
#include "others.h"
#include "lv2sql.h"
#include "usignal.h"
#include "lv2datab.h"
#include "qsnmp.h"
#include "lv2xml.h"


#define ORGNAME     "LanView"
#define ORGDOMAIN   ""
#define LIBNAME     "lv2lib"

#define SETAPP()    {\
    lanView::setApp(VERSION_MAJOR, VERSION_MINOR, APPNAME); \
    setAppHelp(); \
}

/// A paraméter listában megkeresi a megadott profram kapcsoló pozicióját
/// @param __c A program kapcsoló egy karakteres változata (feltételezzük, hogy egy '-' karakter előzi meg
/// @param __s A program kapcsoló string változata (feltételezzük, hogy egy '--' karakterpár előzi meg.
/// @param args A program argumentum lista
/// @return A két program kapcsoló változat valamelyikével megeggyező paraméter indexe a listában, vagy -1, ha nincs ilyen kapcsoló.
EXT_ int findArg(const QChar& __c, const QString& __s, const QStringList &args);
/// Find program switch.
/// QApplication elött (is) hivható, nem használl Qt objektumot.
/// @param __c A program kapcsoló egy karakteres változata (feltételezzük, hogy egy '-' karakter előzi meg
/// @param __s A program kapcsoló string változata (feltételezzük, hogy egy '--' karakterpár előzi meg.
/// @param argc A program argumentum lista hossza
/// @param argv A program argumentum lista
/// @return A két program kapcsoló változat valamelyikével megeggyező paraméter indexe a listában, vagy -1, ha nincs ilyen kapcsoló.
EXT_ int findArg(char __c, const char * __s, int argc, char * argv[]);

static inline QString langFileName(const QString& an, const QString& ln)
{
    QString ls;
    ls = an + "_" + ln /*.mid(0,2) + ".qm"*/;
    PDEB(VVERBOSE) << "App lang. file name : " << ls << endl;
    return ls;
}

#define ENUM_INVALID    -1

enum eIPV6Pol {
    IPV6_UNKNOWN = -1,
    IPV6_IGNORED =  0,
    IPV6_PERMISSIVE,
    IPV6_STRICT
};

enum eIPV4Pol {
    IPV4_UNKNOWN = -1,
    IPV4_IGNORED =  0,
    IPV4_PERMISSIVE,
    IPV4_STRICT
};

enum ePrivilegeLevel {
    PL_NONE,     ///< Nincs jogosultsága
    PL_VIEWER,   ///<
    PL_INDALARM, ///< Riasztások, riasztások nyugtázása
    PL_OPERATOR,
    PL_ADMIN,
    PL_SYSTEM
};

/*!
@class lanView
@brief "main" bázis objektum. Minden az API-t hasznéló alkalmazásnak létre kell hoznia a saját példányát.

Minden az API-t haszáló alkalmazásnak induláskor létre kell hoznia a saját a lanView objektumból származtatott példányát.
A lanView leszármazott objektumból csak egy példány lehet. Ennek az egy példánynak a pointere elérhető a
lanView::getInstance() statikus metódus hívásával, mely ha az objektum nem létezik dob egy kizárást.
A program kilépésével kapcsolatos műveleteket a lanView destruktora végzi lsd.: ~lanView()

Egy példa a main() -re (a saját származtatott osztály a myLanView) :
@code
#define VERSION_MAJOR   1
#define VERSION_MINOR   23

// Saját kiegészítés a help üzenethez (jelenleg egy üres string)
const QString& setAppHelp()
{
    return _sNul;
}

int main (int argc, char * argv[])
{
    QCoreApplication app(argc, argv);

    SETAPP();
    lanView::snmpNeeded = true;         // Ha az SNMP objektumokat szeretnénk használni
    lanView::sqlNeeded  = SN_SQL_NEED;  // Ha szükség van az adatbázis kapcsolatra, és nem magunk nyitjuk meg a kapcsolatot
    lanView::gui        = false;        // Ha ez nem egy GUI alkalmazás

    myLanView   mo; // A saját példány létrehozása. A myLanView osztály a lanView osztály leszármazotja.

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet, ha volt
    }
    int r = app.exec();
    exit(mo.lastError == NULL ? r : mo.lastError->mErrorCode);
}
@endcode
A headerben is szükség van néhány definícióra. A header első néhány sora :
@code
#include "QtCore"

#include "lanview.h"

#define APPNAME "myapp"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP
@endcode
A megadott modul névnek szerepelnie kell a cDebug::eMask enumerációs értékek között. Ez alapján dönti el a cDebug objektum, hogy milyen
üzeneteket kell kiírni. Vitatható módszer, valószínüleg ki kell találni helyette valami mást.
A származtatott objektum konstruktorában meg kell viszgállni, hogy a lanView konstruktora nem dobott-e hibát pl.:
@code
myLanView::myLanView() : lanView()
{
    pq = NULL;
    if (lastError == NULL) {
        try {
            QSqlQuery *pq = newQuery();
            setup(pq); // saját init
        } CATCH(lastError)
    }
}
@endcode
Ha a lanView konstruktora olyan parancs paramétert dolgoz fel, ami után ki kell lépnie a programnak (pl. a verzió kiírása),
akkor dob egy kizárást, egy olyan cError objektummal, melyben a hibakód az EOK, vagyis a nincs hiba.
Az API hiba esetén kizárást dob egy cError objektum pointerével. Ezt a Qt eseménykezelője nem támogatja.
Ezért, ha ezeket a kizárásokat rendesen le szeretnénk kezelni, akkor a QApplication objektumból származtatnunk kell
egy osztályt, ahhol újra implementáljuk a notify metódust. Pl. egy GUI-s alkalmazás esetén a következőkeppen:
@code
class myApp : public QApplication {
public:
    myApp(int& argc, char ** argv) : QApplication(argc, argv) { ; }
    ~myApp();
    virtual bool notify(QObject * receiver, QEvent * event);
};

bool myApp::notify(QObject * receiver, QEvent * event)
{
    static cError *lastError = NULL;
    try {
        return QApplication::notify(receiver, event);
    }
    catch(no_init_&) { // Már letiltottuk a cError dobálást, hibakezelés közben újabb hiba
        PDEB(VERBOSE) << "Dropped cError..." << endl;
        return false;
    }
    CATCHS(lastError)
    DERR(DERROR) <<  PDEB(DERROR) << "Receiver : " << receiver->objectName() << "::" << typeid(*receiver).name() << endl
                 << "Event : " << typeid(*event).name() << endl;
    cError::mDropAll = true;    // A további hiba dobálások nem kellenek (dobja, de ezentul egy no_init_ objektummal)
    cErrorMessageBox::messageBox(lastError);    // Kiteszünk egy hiba ablakot
    QApplication::exit(lastError->mErrorCode);  // kilépünk,
    return false;
}
@endcode
Természetesen a main() függvényunkben nem a QApplication objektumot, hanemn a saját myApp objetumunkat kell használni.
*/

enum eSqlNeed {
    SN_NO_SQL,
    SN_SQL_NEED,
    SN_SQL_TRY
};

EXT_ bool checkDbVersion(QSqlQuery& q, QString& msg);

class cUser;
class cNode;
class cService;
class cHostService;
class cInspector;

class LV2SHARED_EXPORT lanView  : public QObject {
#ifdef MUST_USIGNAL
    friend void unixSignalHandler(int __i);
#endif
    friend LV2SHARED_EXPORT QSqlDatabase *  getSqlDb(void);
    friend LV2SHARED_EXPORT void dropThreadDb(const QString &tn, enum eEx __ex);
    friend LV2SHARED_EXPORT void sqlBegin(QSqlQuery& q, const QString& tn);
    friend LV2SHARED_EXPORT void sqlEnd(QSqlQuery& q, const QString& tn);
    friend LV2SHARED_EXPORT void sqlRollback(QSqlQuery& q, const QString& tn);

   Q_OBJECT
public:
    /// Konstruktor. Inicializálja az API-t. Az objektum csak egy példányban hozható létre.
    /// Ha az inicializálás sikeres, akkor a lastError adattag értéke NULL lessz, egyébként az
    /// egy hiba adatatit tartalmazó cError objektumra fog mutatni.
    lanView();
    /// Destruktor
    /// Ha a lastError pointer nem null, és a cError objektumban a hiba kód nem EOK, akkor kiírja a
    /// hibaüzenetet a debug rendszeren keresztül a DERROR paraméterrel, és megkisérli rögzíteni a hiba
    /// rekordot az adatbázisban, vagyis hívja a sendError metódust.
    ///
    /// Ha setSelfStateF adattag igaz, és a pSelfHostService pointer nem NULL, akkor a következők történnek még:
    /// Ha hiba volt, és sikerült kiírni a hiba rekordot, akkor megkísérli beállítani a szolgáltatás állapotát
    /// cHostService::setState() metódus hívással, ahol az állapot 'critical' lessz a megjegyzés prdig a teljes hibaüzenet.
    /// Ha nem volt hiba, és nem cError* kizárással léptünk ki, akkor egy 'on' status lessz kiírva.
    /// Ha cError* kizárással és OK hibakóddal léptünk ki, akkor a status a cError objektum másodlagos hiba kódja lessz,
    /// a megjegyzés pedig a másodlagos hiba üzenet. Feltéve, hogy a staus-ban nem jeleztük, hogy az állapot már ki van írva.
    ///
    /// A konstruktor minden általa inicializált objektumot megkísérel felszabadítani.
    ~lanView();
#ifdef MUST_USIGNAL
    /// Egy UNIX signal-t kezelő virtuális metódus.
    /// Windows esetén egy üres függvény.
    virtual bool uSigRecv(int __i);
#endif
    /// Minden adat újraolvasása, az alap metódus csak a
    virtual void reSet();
    /// A fő inspector objektum betöltése, inicializálása, és elindítása.
    /// Az objektumot csak akkor allokálja meg, ha a pSelfInspector pointer értéke NULL,
    /// ekkor feltételezi, hogy a servíz név azonos az applikáció nevével.
    /// Ha az opcionális _tr értéke true, akkor az inicializálást egy SQL tranzakcióba fogja.
    virtual void setup(eTristate _tr = TS_NULL);
    /// A fő inspector objektum betöltése, inicializálása, és elindítása.
    /// Az objektum típusa a template paraméter, mely a cInspector leszármazozja.
    /// feltételezi, hogy a servíz név azonos az alpplikáció nevével.
    /// Ha az opcionális _tr értéke true, akkor az inicializálást egy SQL tranzakcióba fogja.
    template <class T> void tSetup(eTristate _tr)
    {
        switch (_tr) {
        case TS_NULL:                                 break;
        case TS_FALSE:  setupTransactionFlag = false; break;
        case TS_TRUE:   setupTransactionFlag = true;  break;
        }
        QString tn;
        if (setupTransactionFlag) {
            tn = appName + "_setup";
            sqlBegin(*pQuery, tn);
        }
        T *p = new T(*pQuery, appName); // Saját (fő) inspector objektum
        pSelfInspector = p;
        p->postInit(*pQuery);           // init
        if (p->passive() && (p->pSubordinates == NULL || p->pSubordinates->isEmpty())) EXCEPTION(NOTODO);
        if (setupTransactionFlag) {
            sqlEnd(*pQuery, tn);
        }
        p->start();                         // és start
    }
    /// Törli a pSelfInspector -t, ha nem NULL
    virtual void down();
    /// Az alapértelmezett program paraméterek értelmezése
    void parseArg(void);
    /// A hiba objektum tartalmának a kiírása a app_errs táblába.
    /// @param pe A hiba objektum pointere
    /// @param __t A hibát generáló thread neve. Opcionális
    /// @return Ha kiírta az adatbázisba a rekordot, akkor a rekord id-vel tér vissza, egyébként NULL_ID-vel
    static qlonglong sendError(const cError *pe, const QString& __t = QString(), lanView *_instance = NULL);
    /// Az adatbázisban inzertál egy applikáció hiba rekordot, ahol a hiba kód a 'Start' lessz.
    /// Ha a művelet sikertelen, akkor dob egy kizárást.
    void insertStart(QSqlQuery& q);
    /// Az adatbázisban inzertál egy applikáció hiba rekordot, ahol a hiba kód a 'ReStart' lessz.
    /// Ha a művelet sikertelen, akkor dob egy kizárást.
    void insertReStart(QSqlQuery& q);
    /// Az adatbázis "notification" fogadásának az inicilizálása.
    /// Az adatbázis szerver NOTIFY \<csatorna\> parancsának hatására a dbNotif() virtuális slot lessz meghívva.
    /// @param __n csatorna név, ha üres, akkor a csatorna név az applikáció neve.
    /// @param __ex Ha értéke nem EX_IGNORE (alapértelmezés EX_ERROR), akkor ha az adatbázis nem támogatja ezt a funkciót dob egy kizárást.
    /// @return Ha minden rendben akkor true, hiba esetén (és ha __ex nem EX_IGNORE), akkor false.
    bool subsDbNotif(const QString& __n = QString(), enum eEx __ex = EX_ERROR);
    /// Az adatbázis megnyitása.
    /// @param __ex ha értéke nem EX_IGNORE, és nem sikerült az adatbázis megnyitása, akkor dob egy kizárást.
    /// @return Ha sikeresen megnyitotta az adatbázist, akkor true.
    bool openDatabase(enum eEx __ex = EX_ERROR);
    /// Az adatbázis bezárása.
    void closeDatabase();
    ///
    void setSelfObjects();
    /// Ha létre lett hozva a lanView (vagy laszármazotjának) a példánya, akkor annak a pointervel tér vissza, ha nem
    /// akkor dob egy kizárást.
    static lanView*    getInstance(void) { if (instance == NULL) EXCEPTION(EPROGFAIL); return instance; }
    /// Ha létre lett hozva a lanView (vagy laszármazotjának) a példánya, akkor true-val egyébként false-val tér vissza.
    static bool        exist(void) { return instance != NULL; }
    /// Megvizsgálja, hogy az adatbázist megnyitották-e.
    static bool dbIsOpen() {
        return instance != NULL && instance->pDb != NULL && instance->pDb->isOpen();
    }
    /// A megadott felhasználói név alapján betölti a magadott users rekordott,
    /// de elötte ellenörzi a megadott jelszót, hogy helyes-e.
    /// Ha a művelet sikeres, akkor a megallokált cUser objektum pointerét beírja a pUser statikus adattagba.
    /// @param un A felhasználónév
    /// @param pw A jelszó
    /// @param __ex Ha értéke nem EX_IGNORE, és a falhasználó név és/vagy jelszó nem egyezik, akkor dob egy kizárást.
    /// @retuen Ha létezik a megadott nevű felhasználó, és a jelszó is megfelelő, akkor a feltöltött cUser objekum címe.
    static const cUser *setUser(const QString& un, const QString& pw, enum eEx __ex = EX_ERROR);
    /// A megadott felhasználói név alapján betölti a magadott users rekordott.
    /// Ha a művelet sikeres, akkor a megallokált cUser objektum pointerét beírja a pUser statikus adattagba.
    /// @param un A felhasználónév
    /// @param __ex Ha értéke nem EX_IGNORE, és a falhasználó név nem létezik, akkor dob egy kizárást.
    /// @retuen Ha létezik a megadott nevű felhasználó, akkor a feltöltött cUser objekum címe.
    static const cUser *setUser(const QString& un, enum eEx __ex = EX_ERROR);
    /// A megadott felhasználói ID alapján betölti a magadott users rekordott.
    /// Ha a művelet sikeres, akkor a megallokált cUser objektum pointerét beírja a pUser statikus adattagba.
    /// @param un A felhasználó ID
    /// @param __ex Ha értéke igaz, és a falhasználó ID létezik, akkor dob egy kizárást.
    /// @retuen Ha létezik a megadott ID-jű felhasználó, akkor a feltöltött cUser objekum címe.
    static const cUser *setUser(qlonglong uid, enum eEx __ex = EX_ERROR);
    /// Az megadott felhasználói rekord lessz az aktuális felhaszbáló.
    /// @param __pUser A felhasználói objektum pointere, a pointer a lanView példány tulajdona lesz, az fogja felszíbadítani.
    /// @exception Ha az objektum ES_COMPLETE bitje nics beállítva a _stat adattagbanm vagy a _stat negatív.
    static const cUser *setUser(cUser *__pUser);

    /// Az aktuális felhasználó adatait tartalmazó cUser objektum referenciája.
    /// A pUser adattag által mutatott objektum referenciájával tér vissza.
    /// Ha még nem példányosítottuk a lanView osztályt, vagy a pUser egy NULL pointer, akkor dob egy kizárást.
    static const cUser& user();
    static qlonglong getUserId(eEx __ex);
    static cNode&          selfNode()        { cNode        *p = getInstance()->pSelfNode;        if (p == NULL) EXCEPTION(EPROGFAIL); return *p; }
    static const cService& selfService()     {const cService*p = getInstance()->pSelfService;     if (p == NULL) EXCEPTION(EPROGFAIL); return *p; }
    static cHostService&   selfHostService() { cHostService *p = getInstance()->pSelfHostService; if (p == NULL) EXCEPTION(EPROGFAIL); return *p; }
    /// Ellenörzi az aktuális felhasználó jogosultsági szintjét
    static bool isAuthorized(enum ePrivilegeLevel pl);
    /// Ellenörzi az aktuális felhasználó jogosultsági szintjét
    static bool isAuthorized(qlonglong pl) {
        if (pl < 0 || pl > PL_SYSTEM) return false;
        return isAuthorized((enum ePrivilegeLevel)pl);
    }
    /// Ellenörzi az aktuális felhasználó jogosultsági szintjét, NULL_ID esetén OK.
    static bool isAuthOrNull(qlonglong pl) {
        if (pl == NULL_ID) return true;
        return isAuthorized((enum ePrivilegeLevel)pl);
    }
    /// Ellenörzi az aktuális felhasználó jogosultsági szintjét
    static bool isAuthorized(const QString& pl);
    /// Az api könynvtér által gyorstárazott adatokat újra tülti,
    static void resetCacheData();
    /// A könyvtár neve, statikus konstans string, a LIBNAME makró által definiált: "lv2lib"
    const QString   libName;
    qlonglong       debug;      ///< Debug level
    QString         debFile;    ///< Debug/log file
    QString         homeDir;    ///< Home mappa neve
    QString         binPath;    ///< Bin kereső path
    QSettings      *pSet;       ///< Pointer to applicaton settings object
    cError         *lastError;  ///< Pointer to last error object or NULL
    QStringList     args;       ///< Argumentum lista
    QString         lang;       ///< nyelv
    QTranslator    *libTranslator;  ///< translator az API-hoz
    QTranslator    *appTranslator;  ///< translator az APP-hoz

    cNode          *pSelfNode;          ///< Saját host objektum pointere, vagy NULL, ha nem ismert
    const cService *pSelfService;       ///< Saját service objektum pointere, vagy NULL, ha nem ismert
    cHostService   *pSelfHostService;   ///< Saját service példány objektum pointere, vagy NULL, ha nem ismert
    bool            setSelfStateF;      ///< Ha igaz, akkor kilépéskor (destruktor) be kell állítani az aktuális service példány állapotát.
    cUser          *pUser;              ///< A felhasználót azonosító objektum pointere, vagy NULL
    qlonglong       selfHostServiceId;  ///< Ha megadták a saját gyökér hostService rekord ID-je, vagy null_id;
    cInspector     *pSelfInspector;
    QSqlQuery      *pQuery;


    static QString    appName;          ///< Az APP neve
    static short      appVersionMinor;  ///< Az APP al verzió száma
    static short      appVersionMajor;  ///< Az APP fő verzió száma
    static QString    appVersion;       ///< APP verzioó string
    /// Program név és verzió beállítása
    /// @param _vMajor Fő verziószám
    /// @param _vMinor Al verziószám
    /// @param _name Program neve
    static void setApp(short _vMajor, short _vMinor, const char * _name) {
        appVersionMajor = _vMajor;
        appVersionMinor = _vMinor;
        appVersion      = QString::number(_vMajor) + QChar('.') + QString::number(_vMinor);
        appName         = _name;
        QCoreApplication::setApplicationName(appName);
    }
    static const QString    orgName;
    static const QString    orgDomain;
    static const short      libVersionMinor;    ///< API al verziószám
    static const short      libVersionMajor;    ///< API fő verzió szám
    static const QString    libVersion;         ///< API verzió string
    static QString          appHelp;            ///< Az applikáció help string kiegészítésa
    static bool             gui;                ///< Ha GUI alkalmazás, akkor true
    static bool             snmpNeeded;         ///< Ha kell az SNMP akkor true
    static eSqlNeed         sqlNeeded;          ///< Ha a konstruktor nyissa meg az adatbázist, akkor true
    static qlonglong        debugDefault;       ///< Debug maszk alapértelmezett értéke.
    static const QString    homeDefault;        ///< a home mappa alapértelmezett értéke.
    /// Amennyiben be van állítva, akkor ezel a saját hosznévvel fog dolgozni a rendszer. Lsd.: class cInspector;
    /// Akkor használható, ha egy teszt gépen akarjuk futtatni a programot a célgép helyett.
    /// A lanview objektum létrehozásánál, ha értéke NULL string, és létezik a "LV2TEST_SET_SELF_NAME"
    /// környezeti vagy konfigurációs változó, akkor annak értékét állítja be.
    /// Beállítható az értéke a '-S' vagy '--lv2test-set-self-name' kapcsolókkal.
    /// Elsődleges a kapcsolóval magadott érték, aztán lanview konstruktora előtt beállított,
    /// majd a környezeti változóként megadott, végül a konfigurációs állományban megadott érték.
    static QString          testSelfName;

    static eIPV4Pol         ipv4Pol;    ///< IPV4 cím kezelési policy (nincs kifejtve!)
    static eIPV6Pol         ipv6Pol;    ///< IPV6 cím kezelési policy (nincs kifejtve!)

    static bool             setupTransactionFlag;

   protected:
    QStringList * getTransactioMapAndCondLock();
    QSqlDatabase *  pDb;                ///< SQL database object
    QStringList     mainTrasactions;    ///< Fő szál SQL tranzakciói (stack)
    /// A szálanként klónozott QSqlDatabase objektumok konténere, kulcs a szál kötelezően egyedi neve
    QMap<QString, QSqlDatabase *>   dbThreadMap;
    /// A szálanként létrehozott tranzakció stack
    QMap<QString, QStringList>      trasactionsThreadMap;
    /// Mutex objektum a *ThreadMap konténerek kezeléséhez
    QMutex                          threadMutex;
#ifdef MUST_USIGNAL
    cXSignal *      unixSignals;    ///< Unix signal helper object pointer
#endif // MUST_USIGNAL
    static lanView *   instance;
    void            instAppTransl();
protected slots:
    virtual void    dbNotif(const QString & name, QSqlDriver::NotificationSource source, const QVariant & payload);
#ifdef MUST_USIGNAL
    void            uSigSlot(int __i);
#endif
};

#define INSERROR(ec, ...) { \
    cError *pe = new cError(__FILE__, __LINE__,__PRETTY_FUNCTION__,eError::ec); \
    lanView::sendError(pe); \
    delete pe; \
}


EXT_ const QString& IPV4Pol(int e, enum eEx __ex = EX_ERROR);
EXT_ int IPV4Pol(const QString& n, enum eEx __ex = EX_ERROR);

EXT_ const QString& IPV6Pol(int e, enum eEx __ex = EX_ERROR);
EXT_ int IPV6Pol(const QString& n, enum eEx __ex = EX_ERROR);

/// @class cLv2QApp
/// Saját QApplication osztály, a hiba kizárások elkapásához (újra definiált notify() metódus.)
class LV2SHARED_EXPORT cLv2QApp : public QCoreApplication {
public:
    /// Konstruktor. Nincs saját inicilizálás, csak a QApplication konstrujtort hívja.
    cLv2QApp(int& argc, char ** argv);
    ~cLv2QApp();
    /// Az újra definiált notify() metódus.
    /// Az esetleges kizárásokat is elkapja.
    virtual bool notify(QObject * receiver, QEvent * event);
};

/// Elvileg titkosít, gyakorlatilag egy túró, de mégse virít pucéran a konfigban a jelszó.
EXT_ QString scramble(const QString& _s);


#endif // LANVIEW_H
