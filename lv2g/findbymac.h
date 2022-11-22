#ifndef FINDBYMAC_H
#define FINDBYMAC_H

#include "basefind.h"

namespace Ui {
    class FindByMac;
}

class LV2GSHARED_EXPORT cFindByMac : public cBaseFind
{
    Q_OBJECT
public:
    explicit cFindByMac(QMdiArea *parent = 0);
    ~cFindByMac();
    void setMacAnIp(const cMac& mac, const QHostAddress& a);
private:
    void setAllMac();
    void setButtons();
    Ui::FindByMac *pUiHead;
    virtual void finishedPostWork() override;
    virtual void clearPostWork() override;
private slots:
    void comboBoxMAC_currentTextChanged(const QString& s);
    void comboBoxIP_currentTextChanged(const QString& s);
    void pushButtonFindMac_clicked();
    void pushButtonExplore_clicked();
    void toolButtonIP2MAC_clicked();
    void toolButtonMAC2IP_clicked();
    void pushButtonFindIp_clicked();
    void pushButtonNMap_clicked();
    void pushButtonLocalhost_clicked();
    void comboBoxLoIp_activated(const QString &arg1);
    void comboBoxLoMac_activated(int index);
    void pushButtonRDP_clicked();
};



#endif // FINDBYMAC_H
