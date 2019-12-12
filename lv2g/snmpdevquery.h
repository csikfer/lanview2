#ifndef SNMPDEVQUERY_H
#define SNMPDEVQUERY_H

#include <QWidget>
#include "lv2widgets.h"

namespace Ui {
class cSnmpDevQuery;
}

class cSnmpDevQuery;

class cSnmpDevQPort : public QObject {
    friend class cSnmpDevQuery;
    Q_OBJECT
protected:
    cSnmpDevQPort(cSnmpDevQuery * _par, int _r, cNPort *_pp);
    cSnmpDevQuery  *parent;
    QTableWidget   *pTableWidget;
    cTable &        table;
    int             row;
    cNPort         *pPort;
    cInterface     *pInterface;
    const cIfType  *pIfType;
    cMac            mac;
    QComboBox      *pComboBoxIfType;  ///< Select ifType combo box
    cIfTypesModel  *pIfTypeModel;
    QComboBox      *pComboBoxIpType;  ///< IP address type
    cEnumListModel *pIpTypeModel;
    QCheckBox      *pCheckBoxSave;
    QComboBox      *pComboBoxTrunk;
    QString         name;
    QString         note;
    int             lastTrunkIx;        ///< Last trunk port index +1 in paren trunkPortList if not zero
    void strItem(int col, const QString& s, bool alignRight = false) {
        QTableWidgetItem *pi = new QTableWidgetItem(s);
        if (alignRight) pi->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        pi->setFlags(pi->flags() & ~Qt::ItemIsEditable);
        pTableWidget->setItem(row, col, pi);
    }
    void tabItem(int col, const QString& colName, bool alignRight = false) {
        strItem(col, table[colName][row].toString(), alignRight);
    }
    void editItem(int col, const QString& s) {
        QTableWidgetItem *pi = new QTableWidgetItem(s);
        pTableWidget->setItem(row, col, pi);
    }
    void ipTypeItem();
    void trunkItem();
private slots:
    void on_comboBoxIfType_currentIndexChenged(int);
    void on_checkBoxSave_toggled(bool checkState);
    void on_comboBoxIpType_currentIndexChenged(int);
    void on_tableWidget_itemChanged(const QTableWidgetItem * pi);
    void on_comboBoxTrunk_currentIndexChenged(int ix);
};

class cSnmpDevQuery : public cIntSubObj
{
    friend class cSnmpDevQPort;
    Q_OBJECT
public:
    explicit cSnmpDevQuery(QMdiArea *parent = nullptr);
    ~cSnmpDevQuery();
    static const enum ePrivilegeLevel rights;
    void ipTextChanged(const QString &s);
private slots:
    void on_pushButtonSave_clicked();
    void on_pushButtonQuery_clicked();
    void on_lineEditName_textChanged(const QString &s);
    void on_comboBoxIp_currentIndexChanged(const QString &arg1);
    void on_comboBoxIp_currentTextChanged(const QString &arg1);
    void on_checkBoxToSnmp_toggled(bool checked);

    void nodeNameChange(const QString &name);
    void refreshTrunks();

    void on_pushButtonLocalhost_clicked();

protected:
    void clearPorts();
    void setTrunkPortList();
    Ui::cSnmpDevQuery  *ui;
    QSqlQuery          *pq;
    cSelectNode        *pSelectNode;
    cNode              *pHost;
    cSnmpDevice        *pDev;
    cTableShape        *pDevShape;
    cSetWidget         *pTypeWidget;
    QButtonGroup       *pButtobGroupSnmpV;
    QHostAddress        a;
    QList<QHostAddress> listA;
    bool                convertToSnmp;
    cSnmpDevice o;
    static const QString sqlSnmp;
    static const QString sqlHost;
    void clearHost() {
        if (pDev != nullptr && pDev != pHost) delete pDev;
        pDev = nullptr;
        pDelete(pHost);
    }
    cTable                  table;
    QList<cInterface *>     trunkPortList;
    QList<cSnmpDevQPort *>  portRows;
};


#endif // SNMPDEVQUERY_H
