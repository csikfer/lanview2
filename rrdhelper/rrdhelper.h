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
    static int  cntAll; ///< All object number
    static int  cntEna; ///< Enabled object number
};

class cRrdHelper : public cInspector {
public:
    cRrdHelper(QSqlQuery& q, const QString& __sn);
    ~cRrdHelper();
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());
    virtual int run(QSqlQuery& q, QString &runMsg);
    void execRrd(const QString& rrdCmd);
    bool createRrdFile(QSqlQuery& q, QMap<qlonglong, cRrdFile>::iterator i);
    QDir    baseDir;
    QMap<qlonglong, cRrdFile>    rrdFileMap;    ///< cRrdFile index by service_var_id
    int cntOk, cntFail;
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
