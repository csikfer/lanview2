#include "logon.h"

#if   defined(Q_OS_WIN)
#define SECURITY_WIN32
#include <security.h>
#include <secext.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

int     cLogOn::_maxProbes = 5;
cLogOn::cLogOn(bool __needZone, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    _zoneIdList()
{
    _pMyUser = nullptr;
    _needZone = __needZone;
    ui->setupUi(this);
    _pMyUser = getOsUser(_myDomainName, _myUserName);
    _changeTxt   = ui->chgPswPB->text();
    _unChangeTxt = tr("Ne legyen jelszócsere");
    _change = true;
    change();   // Megfordítja a _change flag-et, és eltünteti a felesleges elemeket
    _state  = LR_INITED;
    _probes = 0;

    if (!connect(ui->okPB,     SIGNAL(clicked()), this, SLOT(ok()))
     || !connect(ui->cancelPB, SIGNAL(clicked()), this, SLOT(cancel()))
     || !connect(ui->chgPswPB, SIGNAL(clicked()), this, SLOT(change()))) {
        EXCEPTION(EPROGFAIL);
    }
    if (_needZone) {
        if (!connect(ui->userLE,   SIGNAL(textEdited(QString)), this, SLOT(userNameEdit(QString)))
         || !connect(ui->userLE,   SIGNAL(editingFinished()), this, SLOT(userNameEdited()))) {
            EXCEPTION(EPROGFAIL);
        }
    }
    else {
        ui->zoneCB->hide();
        ui->zoneLBL->hide();
    }
    if (_pMyUser != nullptr) {
        ui->myUserPB->setDisabled(false);
        _inUser = _pMyUser->getName();
        ui->userLE->setText(_inUser);
        userNameEdited();
        if (!connect(ui->myUserPB, SIGNAL(clicked()), this, SLOT(myUser()))) {
            EXCEPTION(EPROGFAIL);
        }
    }
    else {
        ui->myUserPB->setDisabled(true);
    }
}

cLogOn::~cLogOn()
{
    close();
    pDelete(_pMyUser);
}

eLogOnResult    cLogOn::checkState()
{
    if (nullptr != lanView::setUser(ui->userLE->text(), ui->passwLE->text(), EX_IGNORE)) {
        _state = LR_OK;
    }
    else {
        _state = LR_INVALID;
    }
    return _state;
}

eLogOnResult cLogOn::logOn(qlonglong *pZoneId, QWidget *par)
{
    cLogOn  *pDialog = new cLogOn(pZoneId != nullptr, par);
    pDialog->exec();
    if (pZoneId) {
        *pZoneId = pDialog->getZoneId();
    }
    eLogOnResult r = pDialog->_state;
    delete pDialog;
    return r;
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
                ui->msgLBL->setText(tr("A kétszer begépelt új jelszó nem eggyezik. Próbálja újra!"));
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
            ui->msgLBL->setText(tr("Nem megfelelő felhasználónév, vagy jelszó. Próbálja újra!"));
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

void cLogOn::userNameEdit(const QString &_txt)
{
    _inUser = _txt;
    if (_pMyUser != nullptr && 0 == _pMyUser->getName().compare(_txt, Qt::CaseInsensitive)) {
        userNameEdited();
        ui->myUserPB->setDisabled(false);
    }
    else {
        ui->passwLE->clear();
        ui->zoneCB->clear();
        _zoneIdList.clear();
        ui->myUserPB->setDisabled(true);
    }
}

void cLogOn::userNameEdited()
{
    // DBGFNL();
    QSqlQuery q = getQuery();
    QString sql =
            "SELECT DISTINCT(place_group_id), place_group_name || ' / ' || coalesce(place_group_note, '-') AS name"
            "  FROM group_users"
            "  JOIN groups USING (group_id)"
            "  JOIN place_groups USING(place_group_id)"
            "  JOIN users USING(user_id)"
            " WHERE place_group_type = 'zone' AND user_name = ?"
            " ORDER BY name ASC";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, _inUser);
    if (!q.exec()) SQLQUERYERR(q);
    ui->zoneCB->clear();
    _zoneIdList.clear();
    bool ok;
    if (q.first()) {
        QStringList zoneNameList;
        do {
            _zoneIdList << q.value(0).toLongLong(&ok);
            if (!ok) EXCEPTION(EDATA, -1, q.value(0).toString());
            zoneNameList << q.value(1).toString();
        } while (q.next());
        int ix = _zoneIdList.indexOf(ALL_PLACE_GROUP_ID);   // Ha "all" a listában van, az legyen az első/alapértelmezett
        if (ix > 0) {
            _zoneIdList.removeAt(ix);
            zoneNameList.removeAt(ix);
            _zoneIdList.push_front(ALL_PLACE_GROUP_ID);
            zoneNameList.push_front(_sAll);
        }
        ui->zoneCB->addItems(zoneNameList);
        ui->zoneCB->setCurrentIndex(0);
    }
}

void cLogOn::myUser()
{
    if (_pMyUser == nullptr) EXCEPTION(EPROGFAIL,0,"_pMyUser pointer is NULL.");
    lanView::setUser(_pMyUser);
    _pMyUser = nullptr;
    _state = LR_OK;
    accept();
}
