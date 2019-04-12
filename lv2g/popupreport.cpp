#include "popupreport.h"
#include <QMessageBox>

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
    cFileDialog::textEditToFile(fn, pTextEdit, this);
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
        msg2 = toHtmlBold(QObject::trUtf8("Cím tábla :")) + msg2;
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

/***********************************************************************************************************/

const QString cMsgBox::sNotFOpen = QObject::trUtf8("A %2 nevű fájl nem nyitható meg : %1");
const QString cMsgBox::sNotFRead = QObject::trUtf8("A megadott %1 nevű fájl nem olvasható");
const QString cMsgBox::sNotFWrite= QObject::trUtf8("A megadott %1 nevű fájl nem irható");
const QString cMsgBox::sNotFRdWr = QObject::trUtf8("A megadott %1 nevű fájl nem irható, és nem olvasható");

cMsgBox::cMsgBox(eDataCharacter _dc, QWidget *par)
    : QMessageBox(par)
{
    const cEnumVal& eVal = cEnumVal::enumVal(_sDatacharacter, _dc, EX_IGNORE);
    if (eVal.isNull()) {
        setWindowTitle(dataCharacter(_dc));
        setStandardIcon(_dc);
    }
    else {
        setWindowTitle(dcViewLong(_dc));
        QString s = eVal.getName(_sIcon);
        QIcon icon;
        QList<QSize> sizes;
        if (s.isEmpty() || (sizes = (icon = resourceIcon(s)).availableSizes()).isEmpty()) {
            setStandardIcon(_dc);
        }
        else {
            setIconPixmap(icon.pixmap(sizes.first()));
        }
    }
}

void cMsgBox::info(const QString& _m, QWidget *par)
{
    cMsgBox o(DC_INFO, par);
    o.setText(_m);
    o.exec();
}

void cMsgBox::warning(const QString& _m, QWidget *par)
{
    cMsgBox o(DC_WARNING, par);
    o.setText(_m);
    o.exec();
}
void cMsgBox::error(const QString& _m, QWidget *par)
{
    cMsgBox o(DC_ERROR, par);
    o.setText(_m);
    o.exec();
}
bool cMsgBox::yesno(const QString& _m, QWidget *par)
{
    cMsgBox o(DC_QUESTION, par);
    o.setText(_m);
    o.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int r = o.exec();
    return r == QMessageBox::Yes;

}
bool cMsgBox::yes(const QString& _m, QWidget *par)
{
    cMsgBox o(DC_QUESTION, par);
    o.setText(_m);
    o.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    int r = o.exec();
    return r == QMessageBox::Yes;

}
eTristate tristate(const QString& _m, QWidget *par)
{
    cMsgBox o(DC_QUESTION, par);
    o.setText(_m);
    o.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    int r = o.exec();
    if (r == QMessageBox::Yes) return TS_TRUE;
    if (r == QMessageBox::No)  return TS_FALSE;
    return TS_NULL;
}

void cMsgBox::setStandardIcon(eDataCharacter _dc)
{
    switch (_dc) {
    case DC_ERROR:      setIcon(QMessageBox::Critical);     break;
    case DC_WARNING:    setIcon(QMessageBox::Warning);      break;
    case DC_QUESTION:   setIcon(QMessageBox::Question);     break;
    default:            setIcon(QMessageBox::Information);  break;
    }
}

bool cMsgBox::tryOpenRead(QFile& f, QWidget *par)
{
    bool ok = f.isOpen();
    QString msg;
    if (!ok) {
        ok = f.open(QIODevice::ReadOnly);
        if (!ok) {
            msg = sNotFOpen.arg(f.errorString());
        }
    }
    if (ok) {
        ok = f.isReadable();
        if (!ok) {
            msg = sNotFRead;
        }
    }
    if (ok) return true;
    warning(msg.arg(f.fileName()), par);
    return false;
}

bool cMsgBox::tryOpenWrite(QFile& f, QWidget *par)
{
    bool ok = f.isOpen();
    QString msg;
    if (!ok) {
        ok = f.open(QIODevice::WriteOnly);
        if (!ok) {
            msg = sNotFOpen.arg(f.errorString());
        }
    }
    if (ok) {
        ok = f.isWritable();
        if (!ok) {
            msg = sNotFWrite;
        }
    }
    if (ok) return true;
    warning(msg.arg(f.fileName()), par);
    return false;
}

bool cMsgBox::tryOpenReadWrite(QFile& f, QWidget *par)
{
    bool ok = f.isOpen();
    QString msg;
    if (!ok) {
        ok = f.open(QIODevice::ReadWrite);
        if (!ok) {
            msg = sNotFOpen.arg(f.errorString());
        }
    }
    if (ok) {
        bool rd = f.isReadable();
        bool wr = f.isWritable();
        ok = rd && wr;
        if (!ok) {
            if (!rd && !wr)  msg = sNotFRdWr;
            else if (!rd)    msg = sNotFRead;
            else             msg = sNotFWrite;
        }
    }
    if (ok) return true;
    warning(msg.arg(f.fileName()), par);
    return false;
}

/******************************************************************************************************************/

const QString cFileDialog::sInpFileFilter   = QObject::trUtf8("Szöveg fájlok (*.txt *.src *.imp)");
const QString cFileDialog::sHtmFileFilter   = QObject::trUtf8("Szöveg fájlok (*.txt *.html *.htm)");
const QString cFileDialog::sCsvFileFilter   = QObject::trUtf8("CSV fájlok (*.csv)");
const QString cFileDialog::sTrgFileTitle    = QObject::trUtf8("Cél fájl kiválasztása");
const QString cFileDialog::sSrcFileTitle    = QObject::trUtf8("Forrás fájl kiválasztása");


QFile *cFileDialog::srcFile(QString &fileName, const QString& filter, QWidget *par)
{
    QString fn;
    fn = QFileDialog::getOpenFileName(par, sSrcFileTitle, dirName(fileName), filter);
    if (fn.isEmpty()) return nullptr;
    fileName = fn;
    QFile* pFile = new QFile(fileName);
    if (!cMsgBox::tryOpenRead(*pFile, par)) {
        delete pFile;
        return nullptr;
    }
    return pFile;
}

QFile *cFileDialog::trgFile(QString &fileName, const QString& filter, QWidget *par)
{
    QString fn;
    fn = QFileDialog::getSaveFileName(par, sSrcFileTitle, fileName, filter);
    if (fn.isEmpty()) return nullptr;
    fileName = fn;
    QFile* pFile = new QFile(fileName);
    if (!cMsgBox::tryOpenWrite(*pFile, par)) {
        delete pFile;
        return nullptr;
    }
    return pFile;
}

bool cFileDialog::textToFile(QString &fileName, const QString &text, QWidget * par)
{
    QFile *pFile = trgFile(fileName, sHtmFileFilter, par);
    if (pFile == nullptr) return false;
    QByteArray b = text.toUtf8();
    bool r = b.size() == pFile->write(b);
    delete pFile;
    return r;
}

bool cFileDialog::textFromFile(QString &fileName, QString &text, QWidget * par)
{
    QFile *pFile = srcFile(fileName, sInpFileFilter, par);
    if (pFile == nullptr) return false;
    text = QString::fromUtf8(pFile->readAll());
    delete pFile;
    return !text.isEmpty();
}

bool cFileDialog::textEditToFile(QString &fileName, const QTextEdit *pTE, QWidget * par)
{
    QFile *pFile = trgFile(fileName, sInpFileFilter, par);
    if (pFile == nullptr) return false;
    QString ext = QFileInfo(*pFile).suffix();
    QString text;
    if (0 == ext.compare("html", Qt::CaseInsensitive) || 0 == ext.compare("htm", Qt::CaseInsensitive)) {
        text = pTE->toHtml();
    }
    else  {
        text = pTE->toPlainText();
    }
    QByteArray b = text.toUtf8();
    bool r = b.size() == pFile->write(b);
    delete pFile;
    return r;
}

QString cFileDialog::dirName(const QString& fileName)
{
    if (fileName.isEmpty()) return lanView::getInstance()->homeDir;
    QFileInfo   fi(fileName);
    return fi.dir().dirName();
}
