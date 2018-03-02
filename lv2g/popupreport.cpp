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
    QString msg1, msg2;
    cPatch *pn = cPatch::getNodeObjById(q, nid);
    QString title = QObject::trUtf8("") + pn->getName() + " #" + QString::number(pn->getId()) + " ";
    if      (pn->descr() == cPatch::     _descr_cPatch())      title += QObject::trUtf8("Patch port vagy fali csatlakozó");
    else if (pn->descr() == cNode::      _descr_cNode())       title += QObject::trUtf8("Passzív vagy aktív hálózati eszköz");
    else if (pn->descr() == cSnmpDevice::_descr_cSnmpDevice()) title += QObject::trUtf8("Aktív SNMP eszköz");
    pn->fetchPorts(q);
    msg1 = htmlReportNode(q, *pn, _sNul, true);
    QVariantList bind;
    bind << nid;
    msg2 = query2html(q, "mactab_node", "node_id = ?", bind, "ORDER BY port_index ASC");
    if (!msg2.isEmpty()) {
        msg2 = htmlBold(QObject::trUtf8("Cím tábla :")) + msg2;
    }
    delete pn;
    return popupReportWindow(par, msg1 + msg2, title);
}
