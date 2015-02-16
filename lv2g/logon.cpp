#include "logon.h"

int     cLogOn::_maxProbes = 5;
cLogOn::cLogOn(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    _changeTxt   = ui->chgPswPB->text();
    _unChangeTxt = trUtf8("Ne legyen jelszócsere");
    _change = true;
    change();   // Megfordítja a _change flag-et, és eltünteti a felesleges elemeket
    _state  = LR_INITED;
    _probes = 0;

    connect(ui->okPB,     SIGNAL(clicked()), this, SLOT(ok()));
    connect(ui->cancelPB, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(ui->chgPswPB, SIGNAL(clicked()), this, SLOT(change()));
}

cLogOn::~cLogOn()
{
    ;
}

eLogOnResult    cLogOn::checkState()
{
    if (NULL != lanView::setUser(ui->userLE->text(), ui->passwLE->text(), false)) {
        _state = LR_OK;
    }
    else {
        _state = LR_INVALID;
    }
    return _state;
}

eLogOnResult cLogOn::logOn(QWidget *par)
{
    cLogOn  dialog(par);
    dialog.exec();
    return dialog._state;
}

void cLogOn::cancel()
{
    _state = LR_CANCEL;
    reject();
}

void cLogOn::ok()
{
    if (checkState() == LR_OK) {    // A jelszó rendben
        if (_change) {              // Jelszó csere?
            QString np = ui->newPswLE->text();
            if (np != ui->newPsw2LE->text()) {
                ui->msgLBL->setText(trUtf8("A kétszer begépelt új jelszó nem eggyezik. Próbálja újra!"));
                return;
            }
            lanView::getInstance()->pUser->updatePassword(np);
        }
        accept();
        return;
    }
    else {
        if (_probes < _maxProbes) {
            _probes++;
            ui->msgLBL->setText(trUtf8("Nem megfelelő felhasználónév, vagy jelszó. Próbálja újra!"));
            return;
            return;
        }
    }
    reject();
}


void cLogOn::change()
{
    _change = !_change;
    if (_change) {
        ui->newPswLBL->show();
        ui->newPsw2LBL->show();
        ui->newPswLE->show();
        ui->newPsw2LE->show();
        ui->chgPswPB->setText(_unChangeTxt);
    }
    else {
        ui->newPswLBL->hide();
        ui->newPsw2LBL->hide();
        ui->newPswLE->hide();
        ui->newPsw2LE->hide();
        ui->chgPswPB->setText(_changeTxt);
    }
}
