#ifndef LV2D_H
#define LV2D_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "lv2d"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2d;

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
    cInspector  *pSelf;
    ///
    QSqlQuery *pq;
    /// A futó démonok száma
    int     runingCnt;
private slots:
    void dbNotif(QString __s);
};


#endif // LV2D_H
