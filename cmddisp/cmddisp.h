#ifndef ARPD_H
#define ARPD_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "cmddisp"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2CmdDisp;

class cCmdDispatcher : public cInspector {
public:
    cCmdDispatcher(QSqlQuery& q, const QString& __sn);
    ~cCmdDispatcher();
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
};

/// @class cCmdDispItem
/// Az egy lekérdezendő eszközt/vagy csoportot reprezentál
class cCmdDispItem : public cInspector {
public:
    /// Konstruktor
    cCmdDispItem(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cCmdDispItem();
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
};

/// @class cCmdDispSubItem
/// Az egy lekérdezendő eszközt a csoportban reprezentál
class cCmdDispSubItem : public cInspector {
public:
    /// Konstruktor
    cCmdDispSubItem(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cCmdDispSubItem();
};

/// Lekédező APP main objektum
class lv2CmdDisp : public lanView {
    Q_OBJECT
public:
    lv2CmdDisp();
    ~lv2CmdDisp();
    /// Indulás
    void setup();
    ///
    QSqlQuery      *pq;
    /// A saját (superior) szolgáltatás és host objektumok
    cCmdDispatcher      *pSelf;
};

#endif // ARPD_H
