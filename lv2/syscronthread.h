#ifndef CSYSCRONTHREAD_H
#define CSYSCRONTHREAD_H
#include "lv2service.h"

class cSysCronThread : cInspectorThread {
    friend class cInspector;
protected:
    cSysCronThread(cInspector * pp);
    virtual void timerEvent();
private:
    void dbCron();
    void mailCron();
    void smsCron();
    eNotifSwitch state;
    QString statMsg;
};

#endif // CSYSCRONTHREAD_H