#ifndef CSYSCRONTHREAD_H
#define CSYSCRONTHREAD_H
#include "lv2service.h"

#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX)) && defined(Q_PROCESSOR_X86_64)
class cSysCronThread : public cInspectorThread {
public:
    cSysCronThread(cInspector * pp);
    virtual void timerEvent();
private:
    void dbCron();
    void mailCron();
    void smsCron();
    eNotifSwitch state;
    QString statMsg;
};


class cSysInspector : public cInspector {
public:
    cSysInspector(QSqlQuery &q, const QString& sn);
    cSysInspector(QSqlQuery& q, qlonglong __host_service_id = NULL_ID, qlonglong __tableoid = NULL_ID, cInspector * __par = NULL);

    virtual cInspectorThread *newThread();
    virtual cInspector *newSubordinate(QSqlQuery& q, qlonglong _hsid, qlonglong _toid = NULL_ID, cInspector *_par = NULL);
};
#else
#define cSysInspector cInspector
#endif

#endif // CSYSCRONTHREAD_H
