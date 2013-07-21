#ifndef IMPORT_H
#define IMPORT_H

#include <Qt>
#include <QPoint>
#include "lanview.h"
#include "ping.h"
#include "guidata.h"

#define APPNAME "import"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  IMPORT

class lv2import : public lanView {
    Q_OBJECT
   public:
    lv2import();
    ~lv2import();
    void abortOldRecords();
    QSqlQuery   *pq;
    QString     fileNm;
    QFile       in;
    bool        daemonMode;
protected slots:
    virtual void dbNotif(QString __s);
};

#endif // IMPORT_H
