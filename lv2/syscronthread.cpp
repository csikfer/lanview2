#include "syscronthread.h"

cSysCronThread::cSysCronThread(cInspector *pp)
    : cInspectorThread(pp)
{
    ;
}

void cSysCronThread::timerEvent(QTimerEvent * e)
{
    (void)e;
    _DBGFN() << inspector.name() << endl;
    if (inspector.internalStat != IS_SUSPENDED) {
        APPMEMO(*inspector.pq, trUtf8("..."), RS_WARNING);
        return;
    }
    inspector.internalStat = IS_RUN;
    cron();
    inspector.internalStat = IS_SUSPENDED;
    _DBGFNL() << inspector.name() << endl;
}


void cSysCronThread::cron()
{
    statMsg.clear();
    state = RS_ON;
    dbCron();
    mailCron();
    smsCron();
    inspector.hostService.setState(*inspector.pq, notifSwitch(state), statMsg);
}

void cSysCronThread::dbCron()
{
    if (!statMsg.isEmpty()) statMsg += "\n\n";
    cError *pe = NULL;
    try {
        sqlBegin(*inspector.pq, _sSyscron);
        bool ok;
        int n = execSqlIntFunction(*inspector.pq, &ok, "service_cron", inspector.hostServiceId());
        QString msg = trUtf8("Number of expired states %1.").arg(n);
        PDEB(INFO) << msg << endl;
        sqlEnd(*inspector.pq, _sSyscron);
        statMsg += "dbCron : " + msg;
    } CATCHS(pe);
    if (pe != NULL) {
        sqlRollback(*inspector.pq, _sSyscron);
        state = RS_CRITICAL;
        statMsg += trUtf8("ERROR in dbCron() : ") + pe->msg();
        delete pe;
    }
}

void cSysCronThread::mailCron()
{
    QSqlQuery& q = *inspector.pq;
    QSqlQuery  q2 = getQuery();
    cUserEvent e;
    e.setId(_sEventType,  UE_SENDMAIL);
    e.setId(_sEventState, UE_NECESSARY);
    QMap<qlonglong, QList<qlonglong> >  uidMapByAid;
    QMap<qlonglong, QList<qlonglong> >  aidMapByUid;
    qlonglong uid, aid;
    if (e.completion(q)) do {
        uid = e.getId(_sUserId);
        aid = e.getId(_sAlarmId);
        uidMapByAid[aid] << uid;    // For sendmail
        aidMapByUid[uid] << aid;    // For set stats
    } while (e.next(q));

    QMap<qlonglong, QString >  MsgMapByUid;
    foreach (aid, uidMapByAid.keys()) {
        QString msg;
        // message ...
        foreach (uid, uidMapByAid[aid]) {
            QString emsg = MsgMapByUid[uid];
            if (!emsg.isEmpty()) emsg += "\n***************************\n";
            MsgMapByUid[uid] = emsg + msg;
        }
    }

    foreach (uid, aidMapByUid.keys()) {
        // sendmail ...
        foreach (aid, aidMapByUid[uid]) {
            // set status
        }
    }
}

void cSysCronThread::smsCron()
{
    ;
}
