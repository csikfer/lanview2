#ifndef UPDT_OIU_H
#define UPDT_OIU_H
#include "lanview.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDateTime>
#include <QFile>

#define APPNAME "updt_oui"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class Downloader : public QObject
{
    Q_OBJECT
public:
    explicit Downloader(QQueue<QUrl>& qr, QSqlQuery *_pq, QObject *parent = 0);

    void doDownload();
private:
    QQueue<QUrl>& urlQueue;
    void prelude();

signals:

public slots:
    void replyFinished (QNetworkReply *reply);

private:
   QUrl url;
   QNetworkAccessManager *manager;
   QSqlQuery      *pq;
};

/// Main objektum
class lv2UpdateOui : public lanView {
    Q_OBJECT
public:
    lv2UpdateOui();
    ~lv2UpdateOui();
    QQueue<QUrl>    urlQueue;
    QSqlQuery      *pq;
    Downloader      d;
protected slots:
};

#endif // UPDT_OIU_H
