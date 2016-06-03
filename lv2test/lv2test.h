#ifndef LV2TEST
#define LV2TEST

#include <Qt>
#include <QPoint>
#include "lanview.h"
#include "guidata.h"

#define APPNAME "test"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2test : public lanView {
    Q_OBJECT
   public:
    lv2test();
    ~lv2test();
    QSqlQuery   *pq;
protected slots:
    virtual void dbNotif(QString __s);
};


#endif // LV2TEST

