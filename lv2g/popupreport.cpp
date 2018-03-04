#include "popupreport.h"

QWidget * popupReportWindow(QObject* _par, const QString& _text, const QString& _title)
{
    QString title = _title.isEmpty() ? QObject::trUtf8("Riport") : _title;
    QWidget *p = new QWidget(NULL, Qt::Window);
    p->QObject::setParent(_par);
    p->setWindowTitle(title);
    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->addWidget(new QTextEdit(_text), 1);
    QHBoxLayout *pHLayout = new QHBoxLayout;
    pVLayout->addLayout(pHLayout, 0);
    pHLayout->addStretch();
    QPushButton *pButton = new QPushButton(QObject::trUtf8("Bezár"));
    QObject::connect(pButton, SIGNAL(clicked()), p, SLOT(deleteLater()));
    pHLayout->addWidget(pButton);
    pHLayout->addStretch();
    p->setLayout(pVLayout);
    p->show();
    return p;
}

QWidget * popupReportNode(QObject *par, QSqlQuery& q, qlonglong nid)
{
    cPatch *po = cPatch::getNodeObjById(q, nid);
    QWidget *r = popupReportNode(par, q, po);
    delete po;
    return r;
}

QWidget * popupReportNode(QObject *par, QSqlQuery& q, cRecord *po)
{
    QString msg1, msg2;
    QString title = "?";

    msg1 = htmlReportNode(q, *po, title, -1);
    QVariantList bind;
    bind << po->getId();
    msg2 = query2html(q, "mactab_node", "node_id = ?", bind, "ORDER BY port_index ASC");
    if (!msg2.isEmpty()) {
        msg2 = htmlBold(QObject::trUtf8("Cím tábla :")) + msg2;
    }
    return popupReportWindow(par, msg1 + msg2, title);
}

QWidget * popupReportByIp(QObject *par, QSqlQuery& q, const QString& sIp)
{
    QString msg, title;
    title = QObject::trUtf8("Riport a %1 IP cím alapján").arg(sIp);
    msg = htmlReportByIp(q, sIp);
    if (msg.isEmpty()) return NULL;
    return popupReportWindow(par, msg, title);
}

QWidget * popupReportByMAC(QObject *par, QSqlQuery& q, const QString& sMAC)
{
    QString msg, title;
    title = QObject::trUtf8("Riport a %1 MAC alapján").arg(sMAC);
    msg = htmlReportByMac(q, sMAC);
    if (msg.isEmpty()) return NULL;
    return popupReportWindow(par, msg, title);
}
