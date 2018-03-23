#include "popupreport.h"

QWidget * popupReportWindow(QWidget* _par, const QString& _text, const QString& _title)
{
    QString title = _title.isEmpty() ? QObject::trUtf8("Riport") : _title;
    QWidget *p = new QWidget(_par, Qt::Window);
    p->setWindowTitle(title);
    QVBoxLayout *pVLayout = new QVBoxLayout;
    p->setLayout(pVLayout);
    QTextEdit *pTextEdit = new QTextEdit(_text);
    pVLayout->addWidget(pTextEdit, 1);
    QHBoxLayout *pHLayout = new QHBoxLayout;
    pVLayout->addLayout(pHLayout, 0);
    pHLayout->addStretch();
    QPushButton *pButton = new QPushButton(QObject::trUtf8("Bezár"));
    QObject::connect(pButton, SIGNAL(clicked()), p, SLOT(deleteLater()));
    pHLayout->addWidget(pButton);
    pHLayout->addStretch();
    p->show();      // Enélkül dokumentum méret, és ezzel s
    QSize  s = QApplication::screens().first()->size();
    s *= 7; s /= 10;    // 70%
    // p->resize(s);   // Minimalizáljuk a sorok tördelését
    pTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    QSizeF sf = pTextEdit->document()->size();
    int h, w, sp, bs;
    sp = pHLayout->spacing();
    bs = pButton->size().height();
    h = std::min(s.height(), (int)sf.height());
    w = std::min(s.width(),  (int)sf.width());
    // pTextEdit->resize(w, h); // nem műkodik
    h +=  bs * 2 + sp * 3 + 8;
    w +=  sp * 4 + 16;
    p->resize(w, h);
    return p;
}

QWidget * popupReportNode(QWidget *par, QSqlQuery& q, qlonglong nid)
{
    cPatch *po = cPatch::getNodeObjById(q, nid);
    QWidget *r = popupReportNode(par, q, po);
    delete po;
    return r;
}

QWidget * popupReportNode(QWidget *par, QSqlQuery& q, cRecord *po)
{
    tStringPair msg1;
    QString msg2;

    msg1 = htmlReportNode(q, *po);
    QVariantList bind;
    bind << po->getId();
    msg2 = query2html(q, "mactab_node", "node_id = ?", bind, "ORDER BY port_index ASC");
    if (!msg2.isEmpty()) {
        msg2 = htmlBold(QObject::trUtf8("Cím tábla :")) + msg2;
    }
    return popupReportWindow(par, msg1.second + msg2, msg1.first);
}

QWidget * popupReportByIp(QWidget *par, QSqlQuery& q, const QString& sIp)
{
    QString msg, title;
    title = QObject::trUtf8("Riport a %1 IP cím alapján").arg(sIp);
    msg = htmlReportByIp(q, sIp);
    if (msg.isEmpty()) return NULL;
    return popupReportWindow(par, msg, title);
}

QWidget * popupReportByMAC(QWidget *par, QSqlQuery& q, const QString& sMAC)
{
    QString msg, title;
    title = QObject::trUtf8("Riport a %1 MAC alapján").arg(sMAC);
    msg = htmlReportByMac(q, sMAC);
    if (msg.isEmpty()) return NULL;
    return popupReportWindow(par, msg, title);
}
