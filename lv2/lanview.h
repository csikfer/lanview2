#ifndef LANVIEW_H
#define LANVIEW_H

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

/*!
@file lanview.h
Az API obkeltum kezelése
*/

#define ORGNAME     "LanView"
#define ORGDOMAIN   ""
#define LIBNAME     "lv2lib"

#define SETAPP()    {\
    lanView::setApp(VERSION_MAJOR, VERSION_MINOR, APPNAME); \
    setAppHelp(); \
}

// Find program switch.
EXT_ int findArg(const QChar& __c, const QString& __s, const QStringList &args);
// QApplication elött (is) hivható, nem használl Qt objektumot.
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

class LV2SHARED_EXPORT lanView  : public QObject {
#ifdef MUST_USIGNAL
    friend void unixSignalHandler(int __i);
#endif
    friend LV2SHARED_EXPORT QSqlDatabase *  getSqlDb(void);
    friend LV2SHARED_EXPORT void dropThreadDb(const QString &tn, bool __ex);
   Q_OBJECT
public:
    lanView();
    ~lanView();
    virtual bool uSigRecv(int __i);
    void parseArg(void);
    /// A hiba objektum tartalmának a kiírása a app_errs táblába.
    /// @param pe A hiba objektum pointere
    /// @param __t A hibát generáló thread neve. Opcionális
    /// @return Ha kiírta az adatbázisba a rekordot, akkor a rekord id-vel tér vissza, egyébként NULL_ID-vel
    qlonglong sendError(const cError *pe, const QString& __t = QString());
    void insertStart(QSqlQuery& q);
    void insertReStart(QSqlQuery& q);
    bool subsDbNotif(const QString& __n = QString(), bool __ex = true);
    bool openDatabase(bool __ex = true);
    void closeDatabase();

    static lanView*    getInstance(void) { if (instance == NULL) EXCEPTION(EPROGFAIL); return instance; }
    static bool        exist(void) { return instance != NULL; }

    const QString   libName;
    qlonglong       debug;      ///< Debug level
    QString         debFile;    ///< Debug/log file
    QString         homeDir;    ///< Home mappa neve
    QString         binPath;    ///< Bin kereső path
    QSettings      *pSet;       ///< Pointer to applicaton settings object
    cError *        lastError;  ///< Pointer to last error object or NULL
    QStringList     args;
    QString         lang;
    QTranslator    *libTranslator;
    QTranslator    *appTranslator;

    static QString    appName;
    static short      appVersionMinor;
    static short      appVersionMajor;
    static QString    appVersion;
    static void setApp(short _vMajor, short _vMinor, const char * _name) {
        appVersionMajor = _vMajor;
        appVersionMinor = _vMinor;
        appVersion      = QString::number(_vMajor) + _sPoint + QString::number(_vMinor);
        appName         = _name;
        QCoreApplication::setApplicationName(appName);
    }
    static const QString    orgName;
    static const QString    orgDomain;
    static const short      libVersionMinor;
    static const short      libVersionMajor;
    static const QString    libVersion;
    static QString          appHelp;
    static bool             gui;
    static bool             snmpNeeded;
    static bool             sqlNeeded;
    static qlonglong        debugDefault;
    static const QString    homeDefault;

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
   public slots:
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

typedef const QString& (*tE2S)(int e, bool __ex);
typedef int (*tS2E)(const QString& n, bool __ex);

/// Az enumeráció kezelés konzisztenciájának ellenörzése.
/// Enumeráció esetén a numerikus érték az adatbázisban az enum típusban megadott listabeli sorszáma (0,1 ...)
/// SET esetén pedig a numerikus értékben a megadott sorszámú bit reprezentál egy enumerációs értéket.
/// Az API-ban lévő sring - enumeráció kovertáló függvényeknél ügyelni kell, hogy a C-ben definiált
/// enumerációs értékek megfeleljenek az adatbázisban a megfelelő enumerációs érték sorrendjének.
/// A vizsgálat csak az adatbázis szerini értékek irányából ellenőriz, ha a konverziós függvények
/// több értéket is kezelnének, azt nem képes detektálni.
/// @param A mező leíró objektum referenciája
/// @param Az enumerációból stringgé konvertáló függvény pointere
/// @param A stringból enumerációs konstanba konvertáló függvény pointere.
/// @return true, ha nem sikerült eltérést detektálni a kétféle enum értELMEZÉS között, és true, ha eltérés van
EXT_ bool checkEnum(const cColStaticDescr& descr, tE2S e2s, tS2E s2e);

#endif // LANVIEW_H
