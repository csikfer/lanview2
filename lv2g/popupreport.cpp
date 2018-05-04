#include "popupreport.h"

cPopupReportWindow::cPopupReportWindow(QWidget* _par, const QString& _text, const QString& _title)
    : QWidget(_par, Qt::Window)
{
    QString title = _title.isEmpty() ? trUtf8("Report") : _title;
    setWindowTitle(title);
    pVLayout = new QVBoxLayout;
    setLayout(pVLayout);
    pTextEdit = new QTextEdit(_text);
    pVLayout->addWidget(pTextEdit, 1);
    pHLayout = new QHBoxLayout;
    pVLayout->addLayout(pHLayout, 0);
    pHLayout->addStretch();
    pButtonSave = new QPushButton(trUtf8("Save"));
    connect(pButtonSave, SIGNAL(clicked()), this, SLOT(save()));
    pHLayout->addWidget(pButtonSave);
    pHLayout->addStretch();
    pButtonClose = new QPushButton(trUtf8("Close"));
    connect(pButtonClose, SIGNAL(clicked()), this, SLOT(deleteLater()));
    pHLayout->addWidget(pButtonClose);
    pHLayout->addStretch();
    show();      // Enélkül dokumentum méret, és ezzel s
    QSize  s = QApplication::screens().first()->size();
    s *= 7; s /= 10;    // 70%
    // p->resize(s);   // Minimalizáljuk a sorok tördelését
    pTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    QSizeF sf = pTextEdit->document()->size();
    int h, w, sp, bs;
    sp = pHLayout->spacing();
    bs = pButtonClose->size().height();
    h = std::min(s.height(), (int)sf.height());
    w = std::min(s.width(),  (int)sf.width());
    // pTextEdit->resize(w, h); // nem műkodik
    h +=  bs * 2 + sp * 3 + 8;
    w +=  sp * 4 + 16;
    resize(w, h);
}

void cPopupReportWindow::save()
{
    static QString fn;
    textEditToFile(fn, pTextEdit, this);
}

QWidget * popupReportWindow(QWidget* _par, const QString& _text, const QString& _title)
{
    return new cPopupReportWindow(_par, _text, _title);
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
