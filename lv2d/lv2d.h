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

enum eDaemonType {
    DT_CONTINUE,    ///< Folyamatosan futó, csak hiba esetén vagy parancsra áll le. Alapértelmezett.
    DT_RESPAWN,     ///< Ha kérés érkezik hozzá, akkor végrehajtja, majd kilép, újra kell indítani. Magic: "#RESPAWN"
    DT_POLLING      ///< Időzítve indítandó, elvégzi a dolgát, majd leáll. Magic: "#POLLING"
};

class cDaemon : public cInspector {
    Q_OBJECT
public:
    cDaemon(QSqlQuery &q, cSupDaemon *par);
    ~cDaemon();
    virtual eNotifSwitch run(QSqlQuery &q);
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());
    virtual void stop(bool __ex);

    /// A check_cmd mezőben a $<paraméter név> típusú kifejezésben a paraméter névhez tartozó
    /// értéket adja vissza, a megadott név alapján, ha a névben a pont karakter szerepel: \n
    /// host_services.<mező név>    Az aktuális host_services rekord megadott nevű mezőjének az értéke stringgé konvertálva \n
    /// services.<mező név>         Az aktuális services rekord megadott nevű mezőjének az értéke stringgé konvertálva \n
    /// nodes.<mező név>            Az aktuális nodes rekord megadott nevű mezőjének az értéke stringgé konvertálva \n
    /// Ha nem szerepel a pont karakter, akkor az aktuális host_services, services ill. a nodes rekordokban keresi az azonos nevű
    /// mezőt, a megadott sorrendben, és találat esetén a mező strinngé konvertált értékével tér vissza \n
    /// Ha nem volt eddig találat, akkor a következő nevekre a kovetkező értéket adja vissza: \n
    /// parent_id   A saját (lv2d) host_service_id értékkel \n
    /// address     Az aktuális node ip címével (ha meg van adva port, akkor a port ip címével). \n
    /// S           Ha megvolt adva a -S <név> parancs opció, akkor annak értékét adja vissza, ha nem akkor egy üres stringget \n
    /// Ha nem tudja értelmezni a nevet, akkor dob egy kizárást
    QString getParValue(const QString& n);
    //
    void getCmd();
    /// A futtatandó shell parancs
    QString         cmd;
    /// A daemon típusa
    enum eDaemonType    daemonType;
    /// Ha true, akkor a processz kimeneteit gyűjtő log fájlt "rotálni" kell
    bool            logRot;
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
