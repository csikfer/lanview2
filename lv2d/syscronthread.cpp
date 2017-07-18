#include "syscronthread.h"
#include "lv2user.h"
#include "SmtpMime"

cSysCronThread::cSysCronThread(cInspector *pp)
    : cInspectorThread(pp)
{
    DBGFN();
}

void cSysCronThread::timerEvent()
{
    _DBGFN() << inspector.name() << endl;
    inspector.internalStat = IS_RUN;
    statMsg.clear();
    state = RS_ON;
    dbCron();
    mailCron();
    smsCron();
    inspector.hostService.setState(*inspector.pq, notifSwitch(state), statMsg);
    inspector.internalStat = IS_SUSPENDED;
}

void cSysCronThread::dbCron()
{
    DBGFN();
    if (!statMsg.isEmpty()) statMsg += "\n\n";
    cError *pe = NULL;
    try {
        sqlBegin(*inspector.pq, _sSyscron);
        execSqlFunction(*inspector.pq, "service_cron", inspector.hostServiceId());
        sqlCommit(*inspector.pq, _sSyscron);
    } CATCHS(pe);
    if (pe != NULL) {
        sqlRollback(*inspector.pq, _sSyscron);
        state = RS_CRITICAL;
        statMsg += trUtf8("ERROR in dbCron() : ") + pe->msg();
        delete pe;
    }
}

/// Elküldésre váró levelek kézbesítése a riasztásokról.
/// Rendszerváltozók:
/// MailServer::text = a mail szerver címe, default: localhost
/// SenderAddress::text = a feladó e-mail címe
void cSysCronThread::mailCron()
{
    DBGFN();
    bool r = true;
    QSqlQuery& q = *inspector.pq;

    // alrms, and user events
    QSqlQuery  q2 = getQuery();
    cUserEvent e;
    e.setId(_sEventType,  UE_SENDMAIL);
    e.setId(_sEventState, UE_NECESSARY);
    QMap<qlonglong, QList<qlonglong> >  uidMapByAid;    // User ID list map by alarm ID
    QMap<qlonglong, QList<qlonglong> >  aidMapByUid;    // Alarm ID list map by User ID
    qlonglong uid, aid;
    if (e.completion(q)) do {
        uid = e.getId(_sUserId);
        aid = e.getId(_sAlarmId);
        uidMapByAid[aid] << uid;
        aidMapByUid[uid] << aid;
    } while (e.next(q));
    else {
        statMsg += "No mail sent. ";
        return;
    }

    // e-mail sender obj.
    QString mailServer = cSysParam::getTextSysParam(q, "MailServer", "localhost");
    int port = 25;
    if (mailServer.contains(':')) {
        QStringList sl = mailServer.split(':');
        bool ok;
        port = sl.at(1).toInt(&ok);
        if (!ok || sl.size() != 2) {
            APPMEMO(q, trUtf8("Invalid parameter format MailServer = %1").arg(mailServer), RS_CRITICAL);
            return;
        }
        mailServer = sl.first();
    }
    SimpleMail::Sender sendmail(mailServer, port, SimpleMail::Sender::TcpConnection);
    // sender e-mail address
    QString sSenderAddress = cSysParam::getTextSysParam(q, "SenderAddress");
    SimpleMail::EmailAddress senderAddress(sSenderAddress, ORGNAME);

    // Set message(s)
    QMap<qlonglong, QString >  MsgMapByUid;             // Messages map by user ID
    foreach (aid, uidMapByAid.keys()) {
        QString msg = cAlarm::htmlText(q, aid);
        foreach (uid, uidMapByAid[aid]) {
            QString emsg = MsgMapByUid[uid];
            if (!emsg.isEmpty()) emsg += "<hr width=\"80%\"><br>"; // Line separator
            MsgMapByUid[uid] = emsg + msg;
        }
    }

    // Send mesage(s) to recipient(s)
    foreach (uid, aidMapByUid.keys()) {
        cUser u; u.setById(q, uid);
        QStringList addresses = u.getStringList(_sAddresses);
        if (addresses.isEmpty()) {
            cUserEvent:: dropped(q, uid, aid, UE_SENDMAIL, trUtf8("Not specified address"));
            r = false;
        }
        else {
            SimpleMail::MimeHtml htmlText;
            htmlText.setHtml(MsgMapByUid[uid]);
            SimpleMail::MimeMessage message;
            message.setSender(senderAddress);
            foreach (QString ea, addresses) {
                SimpleMail::EmailAddress to(ea, u.fullName());
                message.addTo(to);
            }
            message.addPart(&htmlText);
            message.setSubject("Alarm");
            bool     res = sendmail.sendMail(message);
            QString  msg = sendmail.responseText();
            r = r && res;
            foreach (aid, aidMapByUid[uid]) {
                if (res) cUserEvent::happened(q, uid, aid, UE_SENDMAIL, msg);
                else     cUserEvent:: dropped(q, uid, aid, UE_SENDMAIL, msg);
            }
        }
    }
    if (!r) statMsg += "Sendmail error. ";
    else    statMsg += "Sendmail OK. ";
}

void cSysCronThread::smsCron()
{
    ;
}

cSysInspector::cSysInspector(QSqlQuery& q, const QString &sn)
    : cInspector(q, sn)
{
    ;
}

cSysInspector::cSysInspector(QSqlQuery& q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *__par)
    : cInspector(q, __host_service_id, __tableoid, __par)
{
    ;
}


cInspectorThread *cSysInspector::newThread()
{
    _DBGFN() << name() << endl;
    cInspectorThread *p = NULL;
    if (serviceId() == syscronId) {
#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX)) && defined(Q_PROCESSOR_X86_64)
        p = new cSysCronThread(this);
#else
        EXCEPTION(ENOTSUPP,0,_sSyscron);
#endif
    }
    else {
        p = new cInspectorThread(this);
    }
    p->setObjectName(name());
    return p;
}

cInspector *cSysInspector::newSubordinate(QSqlQuery& _q, qlonglong _hsid, qlonglong _toid, cInspector * _par)
{
    return new cSysInspector(_q, _hsid, _toid, _par);
}
