#include "popupreport.h"

cPopupReportWindow::cPopupReportWindow(QWidget* _par, const QString& _text, const QString& _title, bool isHtml)
    : QWidget(_par, Qt::Window)
{
    QString title = _title.isEmpty() ? trUtf8("Report") : _title;
    setWindowTitle(title);
    pVLayout = new QVBoxLayout;
    setLayout(pVLayout);
    pTextEdit = new QTextEdit();
    if (isHtml) pTextEdit->setHtml(_text);
    else        pTextEdit->setPlainText(_text);
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
    screenSize = QApplication::screens().first()->size();
    screenSize *= 7; screenSize /= 10;    // 70%
    // p->resize(s);   // Minimalizáljuk a sorok tördelését
    pTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    resizeByText();
}

void cPopupReportWindow::resizeByText()
{
    QSizeF sf = pTextEdit->document()->size();
    int h, w, sp, bs;
    sp = pHLayout->spacing();
    bs = pButtonClose->size().height();
    h = std::min(screenSize.height(), (int)sf.height());
    w = std::min(screenSize.width(),  (int)sf.width());
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

cPopupReportWindow* popupReportNode(QWidget *par, QSqlQuery& q, qlonglong nid)
{
    cPatch *po = cPatch::getNodeObjById(q, nid);
    cPopupReportWindow *r = popupReportNode(par, q, po);
    delete po;
    return r;
}

cPopupReportWindow* popupReportNode(QWidget *par, QSqlQuery& q, cRecord *po)
{
    tStringPair msg1;
    QString msg2;

    msg1 = htmlReportNode(q, *po);
    QVariantList bind;
    bind << po->getId();
    msg2 = query2html(q, "mactab_node", "node_id = ?", bind, "ORDER BY port_index ASC");
    if (!msg2.isEmpty()) {
        msg2 = toHhtmlBold(QObject::trUtf8("Cím tábla :")) + msg2;
    }
    return popupReportWindow(par, msg1.second + msg2, msg1.first);
}

cPopupReportWindow* popupReportByIp(QWidget *par, QSqlQuery& q, const QString& sIp)
{
    QString msg, title;
    title = QObject::trUtf8("Riport a %1 IP cím alapján").arg(sIp);
    msg = htmlReportByIp(q, sIp);
    if (msg.isEmpty()) return nullptr;
    return popupReportWindow(par, msg, title);
}

cPopupReportWindow* popupReportByMAC(QWidget *par, QSqlQuery& q, const QString& sMAC)
{
    QString msg, title;
    title = QObject::trUtf8("Riport a %1 MAC alapján").arg(sMAC);
    msg = htmlReportByMac(q, sMAC);
    if (msg.isEmpty()) return nullptr;
    return popupReportWindow(par, msg, title);
}
