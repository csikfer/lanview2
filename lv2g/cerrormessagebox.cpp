#include "cerrormessagebox.h"
#include "record_dialog.h"
#include "popupreport.h"


void cErrorMessageBox::row(const QString& l,const QString& val, Qt::AlignmentFlag a)
{
    if (!text.isEmpty()) text += "\n";
    QWidget *pW;
    if (val.size() > 64 || val.contains('\n')) {
        QTextEdit   *pTE = new QTextEdit();
        pTE->setText(val);
        pTE->setReadOnly(true);
        QSizePolicy sp = pTE->sizePolicy();
        sp.setVerticalStretch(1);
        pTE->setSizePolicy(sp);
        pW = pTE;
        text += l + "\n" + val;
    }
    else {
        QLineEdit   *pLE = new QLineEdit();
        pLE->setText(val);
        pLE->setReadOnly(true);
        pLE->setAlignment(a);
        pW = pLE;
        text += l + " : " + val;
    }
    pForm->addRow(l + " : ", pW);
}

#define _R(l, v)    row(tr(l), v)
#define IR(l, v)    row(tr(l), QString::number(v), Qt::AlignRight)
#define NR(l, v)    if (v > 1) IR(l,v)
#define PR(l, v)    if (v != -1) IR(l,v)
#define ZR(l, v)    if (!v.isEmpty()) _R(l,v)
#define LIN         pForm->addWidget(newHLine(this)); text += "\n"

cErrorMessageBox::cErrorMessageBox(cError *_pe, QWidget *parent, const QString& sMainMsg) :
    QDialog(parent)
{
    QVBoxLayout *pVBox = new QVBoxLayout();
    setLayout(pVBox);

    cDialogButtons *pButtons = new cDialogButtons(ENUM2SET3(DBT_COPY,DBT_POPUP, DBT_CLOSE));
    connect(pButtons, SIGNAL(buttonClicked(int)), this, SLOT(pushed(int)));

    if (!sMainMsg.isEmpty()) {
        pVBox->addWidget(new QLabel(sMainMsg));
    }

    pForm = new QFormLayout();
    pVBox->addLayout(pForm);
    pVBox->addWidget(pButtons->pWidget());

    IR("Hiba kód",                              _pe->mErrorCode);
    _R("Hiba típus",                            _pe->errorMsg());
    PR("Hiba alkód, vagy paraméter",            _pe->mErrorSubCode);
    ZR("Hiba altípus, vagy paraméter",          _pe->mErrorSubMsg);
    if (_pe->mErrorSysCode != 0) {
        LIN;
        IR("Az utolsó rendszer hiba kód (errno)",   _pe->mErrorSysCode);
        _R("Az utoló rendszer hiba szöveg",         _pe->errorSysMsg());
    }
    LIN;
    _R("A hibát dobó szál",                     _pe->mThreadName);
    _R("A hibát dobó föggvény",                 _pe->mFuncName);
    _R("A hibát dobó kód forrás neve",          _pe->mSrcName);
    IR("A hibát dobó kód forrás sor száma",     _pe->mSrcLine);
    NR("Hiba objektum számláló",                cError::errCount());
    if (_pe->mSqlErrType != QSqlError::NoError) {
        LIN;
        _R("SQL hiba típus",                        SqlErrorTypeToString(_pe->mSqlErrType));
        ZR("SQL meghajtó üzenet",                   _pe->mSqlErrDrText);
        ZR("SQL adatbázis üzenet",                  _pe->mSqlErrDbText);
        ZR("SQL lekérdezés",                        _pe->mSqlQuery);
        ZR("SQL adatok",                            _pe->mSqlBounds.split(QChar(';')).join(QChar('\n')));
    }
    if (_pe->mDataLine >= 0) {
        LIN;
        _R("Adat név", _pe->mDataName);
        IR("Adat sor", _pe->mDataLine);
        _R("Adat megjegyzés", _pe->mDataMsg);
    }
    if (!_pe->slBackTrace.isEmpty()) {
        QString bt = _pe->slBackTrace.join("\n");
        _R("Stack", bt);
    }
}

void cErrorMessageBox::pushed(int id)
{
    switch (id) {
    case DBT_CLOSE:
        accept();
        break;
    case DBT_COPY:
        PDEB(INFO) << "Text to clipboard : \n" << text << endl;
        QApplication::clipboard()->setText(text, QClipboard::Clipboard);
        break;
    case DBT_POPUP:
        popupTextWindow(parentWidget(), text, _sError);
        break;
    }
}

/// Feltételes hiba ablak megjelenítése:
/// @param _pe Hiba objektum pointere. Ha értéke NULL, akkor visszatér egy true értékkel,
///            ha nem NULL, akkor megjeleníti, majd a hiba ablak bezárásakor visszetér egy false értékkel.
/// @param parent Parent widget pointere.
/// @return Ha a _pe értéke NULL volt, akkor true-val tér vissza
bool cErrorMessageBox::condMsgBox(cError * _pe, QWidget *parent, const QString &sMainMsg)
{
    if (_pe == nullptr) return true;
    cErrorMessageBox(_pe, parent, sMainMsg).exec();
    delete _pe;
    return false;
}
