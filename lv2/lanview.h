#ifndef LANVIEW_H
#define LANVIEW_H

/*!
@file lanview.h
@author Csiki Ferenc
@brief Az API init objektuma
*/

#include "lv2_global.h"
#include <signal.h>
#include "cdebug.h"
#include "cerror.h"
#include "usignal.h"
#include "qsnmp.h"
#include "lv2sql.h"
#include "lv2types.h"
#include "lv2data.h"
#include "lv2user.h"
#include "lv2xml.h"
#include "scan.h"

#define ORGNAME     "LanView"
#define ORGDOMAIN   ""
#define LIBNAME     "lv2lib"

#define SETAPP()    {\
    lanView::setApp(VERSION_MAJOR, VERSION_MINOR, APPNAME); \
    setAppHelp(); \
}

/// Find program switch.
EXT_ int findArg(const QChar& __c, const QString& __s, const QStringList &args);
/// Find program switch.
/// QApplication elött (is) hivható, nem használl Qt objektumot.
EXT_ int findArg(char __c, const char * __s, int argc, char * argv[]);

static inline QString langFileName(const QString& an, const QString& ln)
{
    QString ls;
    ls = an + "_" + ln /*.mid(0,2) + ".qm"*/;
    PDEB(VVERBOSE) << "App lang. file name : " << ls << endl;
    return ls;
}

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

/*!
@class lanView
@brief "main" bázis objektum. Minden az API-t hasznéló alkalmazásnak létre kell hoznia a saját példányát.

Minden az API-t haszáló alkalmazásnak induláskor létre kell hoznia a saját a lanView objektumból származtatott példányát.
Egy példa a main() -re (a saját származtatott osztály a myLanView) :
@code
#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

// Saját kiegészítés a help üzenethez (jelenleg egy üres string)
const QString& setAppHelp()
{
    return _sNul;
}

int main (int argc, char * argv[])
{
    QCoreApplication app(argc, argv);

    SETAPP();
    lanView::snmpNeeded = true;     // Ha az SNMP objektumokat szeretnénk használni
    lanView::sqlNeeded  = true;     // Ha szükség van az adatbázis kapcsolatra, és nem magunk nyitjuk meg a kapcsolatot
    lanView::gui        = false;    // Ha ez nem egy GUI alkalmazás

    myLanView   mo; // A saját példány létrehozása

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
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
#define __MODUL_NAME__  MYAPP
@endcode
A megadott modul névnek szerepelnie kell a cDebug::eMask enumerációs értékek között. Ez alapján dönti el a cDebug objektum, hogy milyen
üzeneteket kell kiírni. Vitatható módszer, valószínüleg ki kell találni helyette valami mást.
A származtatott objektum konstruktorában meg kell viszgállni, hogy a labView konstruktora ne dobott-e hibát pl.:
@code
lv2ArpD::lv2ArpD() : lanView()
{
    if (lastError == NULL) {
        try {
            setup(); // saját init
        } CATCH(lastError)
    }
}
@endcode
Ha a lanView konstruktora olyan parancs paramétert dolgoz fel, ami után ki kell lépnie a programnak (pl. a verzió kiírása),
akkor dob egy kizárást, egy olyan cError objektummal, melyben a hibakód az EOK, vagyis a nincs hiba.
Az API hiba esetén dob egy kizárást a cError objektum pointerével. Ezt a Qt eseménykezelője nem támogatja.
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
    catch(no_init_&) { // Már letiltottuk a cError dobálást
        PDEB(VERBOSE) << "Dropped cError..." << endl;
        return false;
    }
    CATCHS(lastError)
    PDEB(DERROR) << "Error in " << __PRETTY_FUNCTION__ << endl;
    PDEB(DERROR) << "Receiver : " << receiver->objectName() << "::" << typeid(*receiver).name() << endl;
    PDEB(DERROR) << "Event : " << typeid(*event).name() << endl;
    cError::mDropAll = true;    // A további hiba dobálások nem kellenek (dobja, de ezentul egy no_init_ objektummal)
    cErrorMessageBox::messageBox(lastError);
    QApplication::exit(lastError->mErrorCode);  // kilépünk,
    return false;
}
@endcode
Természetesen a main() függvényunkben nem a QApplication objektumot, hanemn a saját myApp objetumunkat kell használni.
*/
class LV2SHARED_EXPORT lanView  : public QObject {
#ifdef MUST_USIGNAL
    friend void unixSignalHandler(int __i);
#endif
    friend LV2SHARED_EXPORT QSqlDatabase *  getSqlDb(void);
    friend LV2SHARED_EXPORT void dropThreadDb(const QString &tn, bool __ex);
   Q_OBJECT
public:
    /// Konstruktor. Inicializálja az API-t. Az objektum csak egy példányban hozható létre.
    /// Ha az inicializálás sikeres, akkor a lastError adattag értéke NULL lessz, egyébként az
    /// egy hiba adatatit tartalmazó cError objektumra fog mutatni.
    lanView();
    /// Destruktor
    /// Ha a lastError pointer nem null, és a cError objektumban a hiba kós nem EOK, akkor kiírja a
    /// hibaüzenetet a debug rendszeren keresztül a DERROR paraméterrel, és megkisérli rögzíteni a hoba
    /// rekordot az adatbázisban, vagyis hívja a sendError metódust.
    /// Minden általa inicializált objektumot felszabadít.
    ~lanView();
    /// Egy UNIX signal-t kezelő virtuális metódus.
    /// Windows esetén egy üres függvény.
    virtual bool uSigRecv(int __i);
    /// Az alapértelmezett program paraméterek értelmezése
    void parseArg(void);
    /// A hiba objektum tartalmának a kiírása a app_errs táblába.
    /// @param pe A hiba objektum pointere
    /// @param __t A hibát generáló thread neve. Opcionális
    /// @return Ha kiírta az adatbázisba a rekordot, akkor a rekord id-vel tér vissza, egyébként NULL_ID-vel
    qlonglong sendError(const cError *pe, const QString& __t = QString());
    /// Az adatbázisban inzertál egy applikáció hiba rekordot, ahol a hiba kód a 'Start' lessz.
    /// Ha a művelet sikertelen, akkor dob egy kizárást.
    void insertStart(QSqlQuery& q);
    /// Az adatbázisban inzertál egy applikáció hiba rekordot, ahol a hiba kód a 'ReStart' lessz.
    /// Ha a művelet sikertelen, akkor dob egy kizárást.
    void insertReStart(QSqlQuery& q);
    /// Az adatbázis "notification" fogadásának az inicilizálása.
    /// Az adatbázis szerver NOTIFY \<csatorna\> parancsának hatására a dbNotif() virtuális slot lessz meghívva.
    /// @param __n csatorna név, ha üres, akkor a csatorna név az applikáció neve.
    /// @param __ex Ha értéke true (alapértelmezés), akkor ha az adatbázis nem támogatja ezt a funkciót dob egy kizárást.
    /// @return Ha minden rendben akkor true, hiba esetén (és ha __ex nem true), akkor false.
    bool subsDbNotif(const QString& __n = QString(), bool __ex = true);
    /// Az adatbázis megnyitása.
    /// @param __ex ha értéke true, és nem sikerült az adatbázis megnyitása, akkor dob egy kizárást.
    /// @return Ha sikeresen megnyitotta az adatbázist, akkor true.
    bool openDatabase(bool __ex = true);
    /// Az adatbázis bezárása.
    void closeDatabase();
    /// Ha létre lett hozva a lanView (vagy laszármazotjának) a példánya, akkor annak a pointervel tér vissza, ha nem
    /// akkor dob egy kizárást.
    static lanView*    getInstance(void) { if (instance == NULL) EXCEPTION(EPROGFAIL); return instance; }
    /// Ha létre lett hozva a lanView (vagy laszármazotjának) a példánya, akkor true-val egyébként false-val tér vissza.
    static bool        exist(void) { return instance != NULL; }
    /// Megvizsgálja, hogy az adatbázist megnyitották-e.
    static bool dbIsOpen() {
        return instance != NULL && instance->pDb != NULL && instance->pDb->isOpen();
    }

    const QString   libName;
    qlonglong       debug;      ///< Debug level
    QString         debFile;    ///< Debug/log file
    QString         homeDir;    ///< Home mappa neve
    QString         binPath;    ///< Bin kereső path
    QSettings      *pSet;       ///< Pointer to applicaton settings object
    cError *        lastError;  ///< Pointer to last error object or NULL
    QStringList     args;       ///< Argumentum lista
    QString         lang;       ///< nyelv
    QTranslator    *libTranslator;  ///< translator az API-hoz
    QTranslator    *appTranslator;  ///< translator az APP-hoz

    static QString    appName;          ///< Az APP neve
    static short      appVersionMinor;  ///< Az APP al verzió száma
    static short      appVersionMajor;  ///< Az APP fő verzió száma
    static QString    appVersion;       ///< APP verzioó string
    static void setApp(short _vMajor, short _vMinor, const char * _name) {
        appVersionMajor = _vMajor;
        appVersionMinor = _vMinor;
        appVersion      = QString::number(_vMajor) + _sPoint + QString::number(_vMinor);
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
    static bool             sqlNeeded;          ///< Ha a konstruktor nyissa meg az adatbázist, akkor true
    static qlonglong        debugDefault;       ///< Debug maszk alapértelmezett értéke.
    static const QString    homeDefault;        ///< a home mappa alapértelmezett értéke.

    static eIPV4Pol         ipv4Pol;    ///< IPV4 cím kezelési policy (nincs kifejtve!)
    static eIPV6Pol         ipv6Pol;    ///< IPV6 cím kezelési policy (nincs kifejtve!)

   protected:
    QSqlDatabase *  pDb;            ///< SQL database object
    /// A szálanként klónozott QSqlDatabase objektumok konténere, kulcs a szál kötelezően egyedi neve
    QMap<QString, QSqlDatabase *>   dbThreadMap;
    /// Mutex objektum a dbThreadMap konténer kezeléséhez
    QMutex                          threadMutex;
#ifdef MUST_USIGNAL
    cXSignal *      unixSignals;    ///< Unix signal helper object pointer
#endif // MUST_USIGNAL
    static lanView *   instance;
    void            instAppTransl();
protected slots:
    virtual void    dbNotif(QString __s);
    void            uSigSlot(int __i);
};

#define INSERROR(ec, ...) { \
    cError *pe = new cError(__FILE__, __LINE__,__PRETTY_FUNCTION__,eError::ec); \
    lanView::getInstance()->sendError(pe); \
    delete pe; \
}


EXT_ const QString& IPV4Pol(int e, bool __ex = true);
EXT_ int IPV4Pol(const QString& n, bool __ex = true);

EXT_ const QString& IPV6Pol(int e, bool __ex = true);
EXT_ int IPV6Pol(const QString& n, bool __ex = true);

#endif // LANVIEW_H
