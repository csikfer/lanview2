#ifndef FINDBYNAME_H
#define FINDBYNAME_H

#include "basefind.h"
#include "lv2widgets.h"

namespace Ui {
    class FindByName;
}

class LV2GSHARED_EXPORT cFindByName : public cBaseFind
{
    Q_OBJECT
public:
    explicit cFindByName(QMdiArea *parent = 0);
    ~cFindByName();
    void setMacAnIp(const cMac& mac, const QHostAddress& a);
private:
    void setButtons();
    virtual void finishedPostWork() override;
    virtual void clearPostWork() override;
    Ui::FindByName *pUiHead;
    bool fN;
    QList<QHostAddress> addressList;
    cSelectNode    * pSelectNode;
    cNode            currentNode;
private slots:
    void selectedNodeChanged(qlonglong nid);
    void pushButtonExplore_clicked();
    void pushButtonRDP_clicked();
    void pushButtonNMap_clicked();
    void pushButtonLocalhost_clicked();
    void pushButtonReport_clicked();
};



#endif // FINDBYNAME_H
