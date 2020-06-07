#ifndef IMPORT_H
#define IMPORT_H

#include <Qt>
#include <QPoint>
#include "lanview.h"
#include "lv2service.h"

#define APPNAME "import"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2import : public lanView {
    Q_OBJECT
   public:
    lv2import();
    ~lv2import();
    void initDaemon();
    void abortOldRecords(QSqlQuery &q);
    void fetchAndExec();
    void execute();
    QSqlQuery   *pQuery;
    QString     actDir;
    QString     inFileName;
    QString     outFileName;
    bool        daemonMode;
protected slots:
    virtual void dbNotif(const QString & name, QSqlDriver::NotificationSource source, const QVariant & payload);
};

class cImportInspector : public cInspector {
public:
    cImportInspector() : cInspector() { }
    virtual int run(QSqlQuery&, QString &);
};

#endif // IMPORT_H
