#include <QCoreApplication>
#include "updt_oui.h"
#include "srvdata.h"
#include "lv2daterr.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

int main(int argc, char *argv[])
{
    cLv2QApp a(argc, argv);

    SETAPP();
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2UpdateOui   mo;

    if (mo.lastError) {  // Ha hiba volt
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }

    mo.d.doDownload();

    int r = a.exec();

    if (r != 0) return r;
    return mo.success ? 0 : 127;

}

lv2UpdateOui::lv2UpdateOui()
    : urlQueue()
    , pq(newQuery())
    , d(urlQueue, pq, this)
{
    success = false;
    if (lastError == nullptr) {
        try {

            insertStart(*pq);

            QString sql =
                    "SELECT param_value FROM sys_params JOIN param_types USING(param_type_id)"
                    " WHERE param_type_name = 'text' AND sys_param_name LIKE 'oui_list_url%'";
            if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
            if (!pq->first()) EXCEPTION(EFOUND, -1, tr("Nincs megadva eggyetlen OUI forrás lista URL."));
            do {
                QString sUrl = pq->value(0).toString();
                QUrl url(sUrl);
                if (!url.isValid()) EXCEPTION(EDATA, -1, tr("Hibás URL név megadása : %1").arg(sUrl));
                urlQueue.push_front(url);
            } while (pq->next());

        } CATCHS(lastError)
    }
}

lv2UpdateOui::~lv2UpdateOui()
{
    ;
}

bool ouiParser(QSqlQuery * pq, QByteArray& text)
{
    DBGFN();
    try {
        QTextStream src(&text, QIODevice::ReadOnly);
        static const QRegularExpression pat1(ANCHORED("\\s*([\\da-f]{2})-([\\da-f]{2})-([\\da-f]{2})\\s+\\(hex\\)[\\s\\t]+(.+)\\s*"), QRegularExpression::CaseInsensitiveOption);
        if (pat1.isValid() == false) EXCEPTION(EPROGFAIL, -1, "pat1");
        static const QRegularExpression pat2(ANCHORED("\\s*[\\-\\da-f]+\\s+\\(base 16\\)[\\s\\t]+(.*)"), QRegularExpression::CaseInsensitiveOption);
        if (pat2.isValid() == false) EXCEPTION(EPROGFAIL, -1, "pat2");
        QString line;
        QRegularExpressionMatch ma;
        while ((line = src.readLine()).isNull() == false) {
            if (!(ma = pat1.match(line)).hasMatch()) {
                PDEB(VVERBOSE) << "Dropp line : " << line << endl;
                continue;
            }
            bool ok;
            QString cap = ma.captured(1);
            qlonglong mac_i = cap.toInt(&ok, 16);
            if (!ok) EXCEPTION(EPROGFAIL, 0, cap);
            cap = ma.captured(2);
            mac_i = (mac_i << 8) + cap.toInt(&ok, 16);
            if (!ok) EXCEPTION(EPROGFAIL, 1, cap);
            cap = ma.captured(3);
            mac_i = (mac_i << 8) + cap.toInt(&ok, 16);
            if (!ok) EXCEPTION(EPROGFAIL, 2);
            cOui    oui;
            cMac    mac(mac_i << 24);
            if (mac.isValid() == false) {
                // Lehet nulla (xerox), de a cMac objektumnál ez nem valid MAC, inkább eldobjuk.
                continue;
                // EXCEPTION(EPROGFAIL, mac_i, QString::number(mac_i, 16));
            }
            oui.setMac(_sOui, mac);
            QString name = ma.captured(4).simplified();
            oui.setName(name);
            line = src.readLine();
            if (!(ma = pat2.match(line)).hasMatch()) {
                DWAR() << "Drop " << QString::number(mac_i, 16) << " / " << oui.getName() << " OUI 2." << endl;
                continue;
            }
            QString note = ma.captured(1).simplified();
            while ((line = src.readLine()).isEmpty() == false) note += "\n" + line.simplified();
            oui.setName(_sOuiNote, note);
            PDEB(VVERBOSE) << "OUI REPLACE : " << oui.toString() << endl;
            int r = oui.replace(*pq);
            PDEB(VVERBOSE) << "Result : " << reasons(r) << endl;
        }
    } CATCHS(lanView::getInstance()->lastError)
    return lanView::getInstance()->lastError == nullptr;
    DBGFNL();
}

Downloader::Downloader(QQueue<QUrl> &qr, QSqlQuery *_pq, QObject *parent) :
    QObject(parent), urlQueue(qr), url()
{
    pq = _pq;
    manager = nullptr;
}

void Downloader::doDownload()
{
    if (manager == nullptr) {
        manager = new QNetworkAccessManager(this);

        connect(manager, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(replyFinished(QNetworkReply*)));
    }
    if (urlQueue.isEmpty()) {
        prelude();
        QCoreApplication::exit();
    }
    else {
        url = urlQueue.dequeue();
        PDEB(INFO) << "Dovload : " << url.toString() << endl;
        manager->get(QNetworkRequest(url));
    }
}

void Downloader::replyFinished (QNetworkReply *reply)
{
    if(reply->error())
    {
        QString msg = tr("A '%1' letöltése sikertelen. %2").arg(url.toString(), reply->errorString());
        cDbErr::insertNew(*pq, cDbErrType::_sDataWarn, msg, -1, cOui().descr().tableName(), QString(__PRETTY_FUNCTION__));
        APPMEMO(*pq, msg, RS_WARNING);
        DWAR() << msg << endl;
    }
    else
    {
        PDEB(VVERBOSE) << "ContentTypeHeader : \r"      << reply->header(QNetworkRequest::ContentTypeHeader).toString() << endl;
        PDEB(VVERBOSE) << "LastModifiedHeader :\r"      << reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().toString();
        PDEB(VVERBOSE) << "ContentLengthHeader :\r"     << reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
        PDEB(VVERBOSE) << "HttpStatusCodeAttribute :\r" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        PDEB(VVERBOSE) << "HttpReasonPhraseAttribute :\r"<<reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

        QByteArray list = reply->readAll();
        if (!ouiParser(pq, list)) {
            lanView& mo = *lanView::getInstance();
            sendError(mo.lastError);
            pDelete(mo.lastError);
        }
    }
    reply->deleteLater();
    doDownload();   // Next or exit
}

void Downloader::prelude()
{
    QString sql =
            "UPDATE mactab SET mactab_state = array_append(mactab_state, 'oui'::mactabstate)"
            "   WHERE 'oui'::mactabstate <> ALL (mactab_state) AND is_content_oui(hwaddress)";
    execSql(*pq, sql);
    static_cast<lv2UpdateOui *>(lanView::getInstance())->success = true;
}
