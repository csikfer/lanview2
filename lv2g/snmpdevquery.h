#ifndef SNMPDEVQUERY_H
#define SNMPDEVQUERY_H

#include <QWidget>
#include "lv2widgets.h"

namespace Ui {
class cSnmpDevQuery;
}

class cSnmpDevQuery : public cIntSubObj
{
    Q_OBJECT
public:
    explicit cSnmpDevQuery(QMdiArea *parent = 0);
    ~cSnmpDevQuery();
    static const enum ePrivilegeLevel rights;
private slots:
    void on_pushButtonSave_clicked();
    void on_pushButtonQuery_clicked();
    void on_lineEditIp_textChanged(const QString &s);
    void on_lineEditName_textChanged(const QString &s);
    void nodeNameChange(const QString &name);

private:
    Ui::cSnmpDevQuery *ui;
    QSqlQuery         *pq;
    cSelectNode       *pSelectNode;
    cSnmpDevice       *pDev;
    cTableShape       *pDevShape;
    cSetWidget        *pTypeWidget;
    QButtonGroup      *pButtobGroupSnmpV;
    QHostAddress       a;
};

#endif // SNMPDEVQUERY_H