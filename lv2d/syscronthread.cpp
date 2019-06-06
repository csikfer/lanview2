#include "syscronthread.h"
#include "lv2user.h"
#include "SmtpMime"

qlonglong cSysInspector::syscronId = NULL_ID;

cSysInspector::cSysInspector(QSqlQuery& q, const QString &sn)
    : cInspector(q, sn)
{
    if (syscronId == NULL_ID) {
        syscronId = cService::service(q, _sSyscron)->getId();
    }
}

cSysInspector::cSysInspector(QSqlQuery& q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *__par)
    : cInspector(q, __host_service_id, __tableoid, __par)
{
    ;
}


cInspector *cSysInspector::newSubordinate(QSqlQuery& _q, qlonglong _hsid, qlonglong _toid, cInspector * _par)
{
    return new cSysInspector(_q, _hsid, _toid, _par);
}

void cSysInspector::timerEvent(QTimerEvent *e)
{
    _DBGFN() << name() << endl;
    if (serviceId() != syscronId) {
        cInspector::timerEvent(e);
        return;
    }
    if (lastRun.isValid()) {
        lastElapsedTime = lastRun.restart();
    }
    else {
        lastElapsedTime = 0;
        lastRun.start();
    }
    internalStat = IS_RUN;
    statMsg.clear();
    state = RS_ON;
    dbCron();
    mailCron();
    smsCron();
    hostService.setState(*pq, notifSwitch(state), statMsg, lastRun.elapsed());
    internalStat = IS_SUSPENDED;
}

void cSysInspector::dbCron()
{
    DBGFN();
    if (!statMsg.isEmpty()) statMsg += "\n\n";
    cError *pe = nullptr;
    try {
        execSqlFunction(*pq, "service_cron", hostServiceId());
    } CATCHS(pe);
    if (pe != nullptr) {
        state = RS_CRITICAL;
        statMsg += tr("ERROR in dbCron() : ") + pe->msg();
        delete pe;
    }
    else {
        statMsg += tr("dbCron() OK.");
    }
}

/// Elküldésre váró levelek kézbesítése a riasztásokról.
/// Rendszerváltozók:
/// MailServer::text = a mail szerver címe, default: localhost
/// SenderAddress::text = a feladó e-mail címe
void cSysInspector::mailCron()
{
    DBGFN();
    bool r = true;
    QSqlQuery& q = *pq;
    if (!statMsg.isEmpty()) statMsg += "\n\n";
    // alrms, and user events
    cUserEvent e;
    cError *pe = nullptr;
    try {
        const QString sql =
                "SELECT user_id, alarm_id FROM user_events "
                "WHERE event_state = 'necessary' AND event_type = 'sendmail'";
        qlonglong uid, aid;
        QMap<qlonglong, QList<qlonglong> >  uidMapByAid;    // User ID list map by alarm ID
        QMap<qlonglong, QList<qlonglong> >  aidMapByUid;    // Alarm ID list map by User ID
        if (execSql(q, sql)) do {
            uid = q.value(0).toLongLong();
            aid = q.value(1).toLongLong();
            uidMapByAid[aid] << uid;
            aidMapByUid[uid] << aid;
        } while (e.next(q));
        else {
            statMsg += "No mail sent. ";
            return;
        }
        // Set message(s)
        QMap<qlonglong, QString >  MsgMapByUid;         // Messages map by user ID(s)
        foreach (aid, uidMapByAid.keys()) {             // For each all alarm ID(s)
            QString msg = cAlarm::htmlText(q, aid);
            foreach (uid, uidMapByAid[aid]) {
                QString emsg = MsgMapByUid[uid];
                if (!emsg.isEmpty()) emsg += "<hr width=\"80%\"><br>"; // Line separator
                MsgMapByUid[uid] = emsg + msg;
            }
        }


        // e-mail sender obj.
        QString mailServer = cSysParam::getTextSysParam(q, "MailServer", "localhost");
        int port = 25;
        if (mailServer.contains(':')) {
            QStringList sl = mailServer.split(':');
            bool ok;
            port = sl.at(1).toInt(&ok);
            if (!ok || sl.size() != 2) {
                APPMEMO(q, tr("Invalid parameter format MailServer = %1").arg(mailServer), RS_CRITICAL);
                return;
            }
            mailServer = sl.first();
        }
        SimpleMail::Sender sendmail(mailServer, port, SimpleMail::Sender::TcpConnection);
        // sender e-mail address
        QString sSenderAddress = cSysParam::getTextSysParam(q, "SenderAddress");
        SimpleMail::EmailAddress senderAddress(sSenderAddress, ORGNAME);

        // Send mesage(s) to recipient(s)
        foreach (uid, aidMapByUid.keys()) {
            cUser u; u.setById(q, uid);
            QStringList addresses = u.getStringList(_sAddresses);
            if (addresses.isEmpty()) {
                cUserEvent:: dropped(q, uid, aid, UE_SENDMAIL, tr("Not specified address"));
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
                    bool rr;
                    if (res) rr = cUserEvent::happened(q, uid, aid, UE_SENDMAIL, msg);
                    else     rr = cUserEvent:: dropped(q, uid, aid, UE_SENDMAIL, msg);
                    if (!rr) {
                        statMsg += tr("Unable to modify user event (%1 / alarm #%2) state to %3.\n")
                                .arg(cUser().getNameById(q, uid))
                                .arg(aid)
                                .arg(userEventState(res ? UE_HAPPENED : UE_DROPPED));
                    }
                }
            }
        }
        if (!r) statMsg += "Sendmail error. ";
        else    statMsg += "Sendmail OK. ";
    } CATCHS(pe);
    if (pe != nullptr) {
        state = RS_CRITICAL;
        statMsg += tr("ERROR in dbCron() send mail : ") + pe->msg();
        delete pe;
    }
}

void cSysInspector::smsCron()
{
    ;
}

