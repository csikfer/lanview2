#ifndef LV2D_H
#define LV2D_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "lv2d"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2d;

class cSupDaemon : public cInspector {
public:
    cSupDaemon(QSqlQuery& q, const QString& __sn, lv2d& _mo);
    ~cSupDaemon();
    /// Az lv2d szolgáltatás teljes inicializálását végzi (az alap osztály inicializálásán túl).
    /// Feltölti a pSubordinates konténert is, ezért sem a setSubs() sem a newSubordinate()
    /// metódus fellüldefiniálása nem szükséges, és azok nem kerülnek meghívásra.
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());
    virtual enum eNotifSwitch run(QSqlQuery &q);
    virtual void stop(bool __ex);
    lv2d&   mo;
};


class lv2d : public lanView {
    Q_OBJECT
public:
    lv2d();
    ~lv2d();
    void setup();
    void down();
    void reSet();
    bool uSigRecv(int __i);
    /// Saját hoszt/szolgáltatás cInspector objektum
    cSupDaemon  *pSelf;
    ///
    QSqlQuery *pq;
    /// A futó démonok száma
    int     runingCnt;
private slots:
    void dbNotif(QString __s);
};

class cDaemon : public cInspector {
    Q_OBJECT
public:
    cDaemon(QSqlQuery &q, cSupDaemon *par);
    ~cDaemon();
    virtual eNotifSwitch run(QSqlQuery &q);
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());
    virtual void stop(bool __ex);

    /// Ha true, akkor a processz önállóan loggol a kimenetét eldobjuk
    bool            logNull;
    /// Rotálás esetén a maximális log fájl hossz (bype), ha ennél nagyobb új log fájlt kell nyitni
    qint64          maxLogSize;
    /// Hány log fájlt kell megtartani
    int             maxArcLog;
    /// A process objektum.
    QProcess        process;
    /// Hiba számláló (egymást követő hibák)
    int             crashCnt;
    /// Maximális egymásutáni hibák száma
    static int      maxCrashCnt;
    /// Az aktuális log fájl
    QFile           actLogFile;
    /// Az előző hiba számláló
    int             oldCrashCnt;
protected slots:
    void procError(QProcess::ProcessError error);
    void procFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void procStarted();
    void procReadyStdOut();
};

#endif // LV2D_H
