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
    QString sReport;    // Report (html) text
    static eTristate nmapExists;
    QList<QNetworkInterface>    myInterfaces;
    void appendReport(const QString &s);
#if defined(Q_OS_WINDOWS)
    QHostAddress rdpClientAddr;
#endif
private slots:
    void on_comboBoxMAC_currentTextChanged(const QString& s);
    void on_comboBoxIP_currentTextChanged(const QString& s);
    void on_pushButtonClear_clicked();
    void on_pushButtonFindMac_clicked();
    void on_pushButtonExplore_clicked();
    void on_toolButtonIP2MAC_clicked();
    void on_toolButtonMAC2IP_clicked();
    void expLine(QString s);
    void finished();
    void on_pushButtonFindIp_clicked();
    void on_pushButtonNMap_clicked();
    void on_pushButtonLocalhost_clicked();
    void on_comboBoxLoIp_activated(const QString &arg1);
    void on_comboBoxLoMac_activated(int index);
    void on_pushButtonRDP_clicked();
};



#endif // FINDBYMAC_H
