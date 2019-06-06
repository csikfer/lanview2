#ifndef RRDHELPER_H
#define RRDHELPER_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "rrdhelper"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2rrdHelper;

class cRrdHelper : public cInspector {
public:
    cRrdHelper(QSqlQuery& q, const QString& __sn);
    ~cRrdHelper();
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());
    void execRrd(const QString& rrdCmd);
protected:
    bool createRrdFile(QSqlQuery& q, qlonglong _id, const QString& _fn);
    QDir    baseDir;
    QMap<qlonglong, QString>    rrdFilePathMap;
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
