#ifndef FINDBYMAC_H
#define FINDBYMAC_H

#include <QWidget>
#include "lv2g.h"
#include "popupreport.h"

namespace Ui {
    class FindByMac;
}
class cFindByMac;
class cFBMExpThread : public QThread
{
    friend class cFindByMac;
    Q_OBJECT
protected:
    cFBMExpThread(cMac &_mac, QHostAddress &_ip, cSnmpDevice& _st, cFindByMac * _par);
    virtual void run();
    cExportQueue *pEQ;
    cMac         mac;
    QHostAddress ip;
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

class LV2GSHARED_EXPORT cFindByMac : public cIntSubObj
{
    Q_OBJECT
public:
    explicit cFindByMac(QMdiArea *parent = 0);
    ~cFindByMac();
    static const enum ePrivilegeLevel rights;
    void setMacAnIp(const cMac& mac, const QHostAddress& a);
private:
    void setAllMac();
    void setSwitchs();
    void setButtons();
    Ui::FindByMac *pUi;
    cFBMExpThread *pThread;
    QSqlQuery *pq;
    bool    fMAC, fIP, fSw;
    static eTristate nmapExists;
private slots:
    void changeMAC(const QString& s);
    void changeIP(const QString& s);
    void hit_clear();
    void hit_find();
    void hit_explore();
    void ip2mac();
    void mac2ip();
    void expLine(QString s);
    void finished();
    void on_pushButtonFindIp_clicked();
    void on_pushButtonNMap_clicked();
};



#endif // FINDBYMAC_H
