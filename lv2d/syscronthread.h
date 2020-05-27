#ifndef CSYSCRONTHREAD_H
#define CSYSCRONTHREAD_H
#include "lv2service.h"

#ifdef SYSCRON

class cSysInspector : public cInspector {
//    Q_OBJECT
public:
    cSysInspector(QSqlQuery &q, const QString& sn);
    cSysInspector(QSqlQuery& q, qlonglong __host_service_id = NULL_ID, qlonglong __tableoid = NULL_ID, cInspector * __par = NULL);
    ~cSysInspector();

    virtual cInspector *newSubordinate(QSqlQuery& q, qlonglong _hsid, qlonglong _toid = NULL_ID, cInspector *_par = NULL);
    virtual void timerEvent(QTimerEvent *);
private:
    void dbCron();
    void mailCron();
    void smsCron();
    eNotifSwitch state;
    QString statMsg;
    static qlonglong syscronId;
};
#else

#define cSysInspector cInspector

#endif

#endif // CSYSCRONTHREAD_H
