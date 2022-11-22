#ifndef BASEFIND_H
#define BASEFIND_H

#include <QWidget>
#include "lv2g.h"
#include "popupreport.h"

namespace Ui {
    class BaseFind;
}
class cBaseFind;
class cBaseFindDiscover : public QThread
{
    friend class cBaseFind;
    Q_OBJECT
protected:
    cBaseFindDiscover(const cMac &_mac, const QHostAddress &_ip, cSnmpDevice& _st, cBaseFind * _par);
    virtual void run();
    cExportQueue *pEQ;
    const cMac         mac;
    const QHostAddress ip;
    cSnmpDevice  st;
private slots:
    void readyLine();
signals:
    void expLine(QString s);
};

class cPopUpNMap : protected cPopupReportWindow {
    Q_OBJECT
public:
    cPopUpNMap(QWidget *par, const QString& sIp);
protected:
    QProcess   *process;
    QString     text;
    bool        first;
private slots:
    void processStarted();
    void processFinished(int ec);
    void processReadyRead();
    void processError(QProcess::ProcessError e);
};

class LV2GSHARED_EXPORT cBaseFind : public cIntSubObj
{
    Q_OBJECT
public:
    explicit cBaseFind(QMdiArea *parent = 0);
    ~cBaseFind();
    static const enum ePrivilegeLevel rights;
protected:
    void discover(const QHostAddress &ip, const cMac &mac, const QString &swName);
    void NMap(const QString& sIp);
    virtual void finishedPostWork() = 0;
    virtual void clearPostWork() = 0;
    void setSwitchs();
    Ui::BaseFind *pUi;
    cBaseFindDiscover *pThread;
    QSqlQuery *pq;
    bool    fMAC, fIP, fSw;
    QString sReport;    // Report (html) text
    static eTristate nmapExists;
    QList<QNetworkInterface>    myInterfaces;
    void appendReport(const QString &s);
    QHostAddress rdpClientAddr;
    int centralSwitchIndex;
    QStringList switchItems;
private slots:
    void expLine(QString s);
    void finished();
    void save_clicked();
    void clear_clicked();
};



#endif // BASEFIND_H
