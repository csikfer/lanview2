#ifndef FINDBYMAC_H
#define FINDBYMAC_H

#include <QWidget>
#include "lv2g.h"

#include "lv2g.h"
namespace Ui {
class FindByMac;
}

class LV2GSHARED_EXPORT cFindByMac : public cOwnTab
{
    Q_OBJECT

public:
    explicit cFindByMac(QWidget *parent = 0);
    ~cFindByMac();

private:
    Ui::FindByMac *pUi;
    QSqlQuery *pq;
protected slots:
    void find();
};

#endif // FINDBYMAC_H
