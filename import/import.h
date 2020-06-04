#ifndef IMPORT_H
#define IMPORT_H

#include <Qt>
#include <QPoint>
#include "lanview.h"
#include "guidata.h"

#define APPNAME "import"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2import : public lanView {
    Q_OBJECT
   public:
    lv2import();
    ~lv2import();
    void initDaemon();
    void abortOldRecords();
    QSqlQuery   *pQuery;
    QString     inFileName;
    QFile       inFile;
    QString     outFileName;
    QFile       outFile;
    bool        daemonMode;
protected slots:
    virtual void dbNotif(const QString & name, QSqlDriver::NotificationSource source, const QVariant & payload);
};

#endif // IMPORT_H
