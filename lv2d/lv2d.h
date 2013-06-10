#ifndef LV2D_H
#define LV2D_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "lv2d"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  LV2D

class lv2d;

class cSupDaemon : public cInspector {
public:
    cSupDaemon(QSqlQuery& q, const QString& __sn, lv2d& _mo);
    ~cSupDaemon();
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

    //
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
