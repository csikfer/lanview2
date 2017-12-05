#ifndef FINDBYMAC_H
#define FINDBYMAC_H

#include <QWidget>

#include "lv2g.h"
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

class LV2GSHARED_EXPORT cFindByMac : public cIntSubObj
{
    Q_OBJECT
public:
    explicit cFindByMac(QMdiArea *parent = 0);
    ~cFindByMac();
    static const enum ePrivilegeLevel rights;
private:
    void setAllMac();
    void setSwitchs();
    void setButtons();
    Ui::FindByMac *pUi;
    cFBMExpThread *pThread;
    QSqlQuery *pq;
    bool    fMAC, fIP, fSw;
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
};



#endif // FINDBYMAC_H
