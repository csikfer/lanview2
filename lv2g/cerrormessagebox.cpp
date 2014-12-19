#include "cerrormessagebox.h"
#include "record_dialog.h"


static inline QWidget *row(const QString& val, Qt::AlignmentFlag a = Qt::AlignLeft)
{
    if (val.size() > 64 || val.contains('\n')) {
        QTextEdit   *pTE = new QTextEdit();
        pTE->setText(val);
        pTE->setReadOnly(true);
        return pTE;
    }
    else {
        QLineEdit   *pLE = new QLineEdit();
        pLE->setText(val);
        pLE->setReadOnly(true);
        pLE->setAlignment(a);
        return pLE;
    }
}

#define _R(l, v)    pForm->addRow(trUtf8(l " : "), row(v))
#define IR(l, v)    pForm->addRow(trUtf8(l " : "), row(QString::number(v), Qt::AlignRight))
#define NR(l, v)    if (v > 1) IR(l,v)
#define PR(l, v)    if (v != -1) IR(l,v)
#define ZR(l, v)    if (!v.isEmpty()) _R(l,v)
#define LIN         pForm->addWidget(newHLine(this))

cErrorMessageBox::cErrorMessageBox(cError *_pe, QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *pVBox = new QVBoxLayout();
    setLayout(pVBox);

    QHBoxLayout *pHBox = new QHBoxLayout();
    QPushButton *pPush = new QPushButton(trUtf8("Bezár"));
    connect(pPush, SIGNAL(clicked()), this, SLOT(endIt()));
    pHBox->addStretch();
    pHBox->addWidget(pPush);
    pHBox->addStretch();

    QFormLayout *pForm = new QFormLayout();
    pVBox->addLayout(pForm);
    pVBox->addWidget(pPush);

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
    NR("Hiba objektum számláló",                _pe->mErrCount);
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
}

void cErrorMessageBox::endIt()
{
    accept();
}

int cErrorMessageBox::condMsgBox(cError * _pe, QWidget *parent)
{
    if (_pe == NULL) return true;
    cErrorMessageBox(_pe, parent).exec();
    delete _pe;
    return false;
}
