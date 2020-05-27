#ifndef RRDHELPER_H
#define RRDHELPER_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "rrdhelper"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2rrdHelper;

class cRrdFile {
public:
    cRrdFile() : varName("v") { enabled = false; ++cntAll; }
    void enable() { enabled = true; ++cntEna; }
    bool    enabled;
    QString fileName;
    QString varName;
    QString lastErrorMsg;
    void addErrorMsg(const QString& msg) { msgAppend(&lastErrorMsg, msg); }
    QString getErrorMsg() { QString r = lastErrorMsg; lastErrorMsg.clear(); return r; }
    static int  cntAll; ///< All object number
    static int  cntEna; ///< Enabled object number
};

enum eRRD_AccessType {
    RAT_LOCAL, RAT_LOCAL_BY_DAEMON, RAT_REAMOTE_BY_DAEMON
};

class cRrdHelper : public cInspector {
public:
    cRrdHelper(QSqlQuery& q, const QString& __sn);
    ~cRrdHelper();
    virtual void postInit(QSqlQuery &q);
    virtual int run(QSqlQuery& q, QString &runMsg);
    void execRrd(const QString& rrdCmd);
    bool createRrdFile(QSqlQuery& q, QMap<qlonglong, cRrdFile>::iterator i);
    QString getErrorMessages();
    QDir    baseDir;
    QMap<qlonglong, cRrdFile>    rrdFileMap;    ///< cRrdFile index by service_var_id
    int cntOk, cntFail;
    eRRD_AccessType accesType;
    QStringList daemonOption;
};

/// Helper APP main objektum
class lv2rrdHelper : public lanView {
    Q_OBJECT
public:
    lv2rrdHelper();
    ~lv2rrdHelper();
    static void staticInit(QSqlQuery *pq);
    /// (Újra) inicializáló metódus
    virtual void setup(eTristate _tr = TS_NULL);
protected slots:
    virtual void    dbNotif(const QString & name, QSqlDriver::NotificationSource source, const QVariant & payload);
};

#endif // RRDHELPER_H
