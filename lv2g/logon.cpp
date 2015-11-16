#include "logon.h"

#if   defined(Q_CC_MSVC)
#define SECURITY_WIN32
#include <security.h>
#include <secext.h>
#endif

int     cLogOn::_maxProbes = 5;
cLogOn::cLogOn(bool __needZone, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    _zoneIdList()
{
    _needZone = __needZone;
    ui->setupUi(this);
#if   defined(Q_CC_MSVC)
    // Csak próba !!!!
#define USER_NAME_MAXSIZE   64
    char charUserName[USER_NAME_MAXSIZE];
    DWORD userNameSize = USER_NAME_MAXSIZE;
    if (GetUserNameExA(NameDnsDomain, charUserName, &userNameSize)) {
        ui->userLE->setText(QString(charUserName));
    }
#endif
    _changeTxt   = ui->chgPswPB->text();
    _unChangeTxt = trUtf8("Ne legyen jelszócsere");
    _change = true;
    change();   // Megfordítja a _change flag-et, és eltünteti a felesleges elemeket
    _state  = LR_INITED;
    _probes = 0;

    connect(ui->okPB,     SIGNAL(clicked()), this, SLOT(ok()));
    connect(ui->cancelPB, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(ui->chgPswPB, SIGNAL(clicked()), this, SLOT(change()));
    if (_needZone) {
        connect(ui->userLE,   SIGNAL(textEdited(QString)), this, SLOT(userNameEdit(QString)));
        connect(ui->userLE,   SIGNAL(editingFinished()), this, SLOT(userNameEdited()));
    }
    else {
        ui->zoneCB->hide();
        ui->zoneLBL->hide();
    }
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

eLogOnResult cLogOn::logOn(qlonglong *pZoneId, QWidget *par)
{
    cLogOn  dialog(pZoneId != NULL, par);
    dialog.exec();
    if (pZoneId) {
        *pZoneId = dialog.getZoneId();
    }
    return dialog._state;
}

qlonglong cLogOn::getZoneId() const
{
    if (_zoneIdList.isEmpty()) return NULL_ID;
    int i = ui->zoneCB->currentIndex();
    if (isContIx(_zoneIdList, i)) return _zoneIdList[i];
    return NULL_ID;
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
            QSqlQuery q = getQuery();
            lanView::getInstance()->pUser->set(_sPasswd, np).update(q, false);
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

void cLogOn::userNameEdit(const QString &)
{
    // DBGFNL();
    ui->passwLE->clear();
    ui->zoneCB->clear();
    _zoneIdList.clear();
}

void cLogOn::userNameEdited()
{
    // DBGFNL();
    QSqlQuery q = getQuery();
    QString sql =
            "SELECT place_group_id, place_group_note"
            " FROM group_users"
            " JOIN groups USING (group_id)"
            " JOIN place_groups USING(place_group_id)"
            " JOIN users USING(user_id)"
            " WHERE user_name = ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, ui->userLE->text());
    if (!q.exec()) SQLQUERYERR(q);
    ui->zoneCB->clear();
    _zoneIdList.clear();
    bool ok;
    if (q.first()) {
        do {
            _zoneIdList << q.value(0).toLongLong(&ok);
            if (!ok) EXCEPTION(EDATA, -1, q.value(0).toString());
            ui->zoneCB->addItem(q.value(1).toString());
        } while (q.next());
        ui->zoneCB->setCurrentIndex(0);
    }
}
