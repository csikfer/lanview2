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
    void ipTextChanged(const QString &s);
private slots:
    void on_pushButtonSave_clicked();
    void on_pushButtonQuery_clicked();
    void on_lineEditName_textChanged(const QString &s);
    void nodeNameChange(const QString &name);
    void on_comboBoxIp_currentIndexChanged(const QString &arg1);
    void on_comboBoxIp_currentTextChanged(const QString &arg1);

private:
    Ui::cSnmpDevQuery *ui;
    QSqlQuery         *pq;
    cSelectNode       *pSelectNode;
    cSnmpDevice       *pDev;
    cTableShape       *pDevShape;
    cSetWidget        *pTypeWidget;
    QButtonGroup      *pButtobGroupSnmpV;
    QHostAddress       a;
    QList<QHostAddress>listA;
};

#endif // SNMPDEVQUERY_H
