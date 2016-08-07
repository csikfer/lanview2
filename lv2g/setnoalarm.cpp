#include "setnoalarm.h"

const enum ePrivilegeLevel cSetNoAlarm::rights = PL_OPERATOR;

enum filterType {
    FT_PATTERN, FT_SELECT
};

cSetNoAlarm::cSetNoAlarm(QWidget *par)
    : cOwnTab(par)
{
    pUi = new Ui_setNoAlarm;
    pUi->setupUi(this);

    pButtonGroupPlace   = new QButtonGroup(this);
    pButtonGroupPlace->addButton(pUi->radioButtonPlacePattern, FT_PATTERN);
    pButtonGroupPlace->addButton(pUi->radioButtonPlaceSelect,  FT_SELECT);
    pUi->radioButtonPlacePattern->setChecked(true);

    pButtonGroupService = new QButtonGroup(this);
    pButtonGroupService->addButton(pUi->radioButtonServicePattern, FT_PATTERN);
    pButtonGroupService->addButton(pUi->radioButtonServiceSelect,  FT_SELECT);
    pUi->radioButtonServicePattern->setChecked(true);

    pButtonGroupNode    = new QButtonGroup(this);
    pButtonGroupNode->addButton(pUi->radioButtonNodePattern, FT_PATTERN);
    pButtonGroupNode->addButton(pUi->radioButtonNodeSelect,  FT_SELECT);
    pUi->radioButtonNodePattern->setChecked(true);

    pUi->dateTimeEditFrom->setMinimumDate(QDate::currentDate());
    pUi->dateTimeEditTo->setMinimumDate(QDate::currentDate());

    connect(pUi->pushButtonClose,   SIGNAL(clicked()),  this,   SLOT(endIt()));
    connect(pUi->pushButtonSet,     SIGNAL(clicked()),  this,   SLOT(set()));
    connect(pUi->pushButtonFetch,   SIGNAL(clicked()),  this,   SLOT(fetch()));
    connect(pUi->pushButtonAll,     SIGNAL(clicked()),  this,   SLOT(all()));
    connect(pUi->pushButtonNone,    SIGNAL(clicked()),  this,   SLOT(none()));

    connect(pUi->radioButtonOff,    SIGNAL(toggled(bool)), this,SLOT(off(bool)));
    connect(pUi->radioButtonOn,     SIGNAL(toggled(bool)), this,SLOT(on(bool)));
    connect(pUi->radioButtonTo,     SIGNAL(toggled(bool)), this,SLOT(to(bool)));
    connect(pUi->radioButtonFrom,   SIGNAL(toggled(bool)), this,SLOT(from(bool)));
    connect(pUi->radioButtonFromTo, SIGNAL(toggled(bool)), this,SLOT(fromTo(bool)));
}


void cSetNoAlarm::fetch()
{

}

void cSetNoAlarm::set()
{

}

void cSetNoAlarm::all()
{

}

void cSetNoAlarm::none()
{

}

void cSetNoAlarm::off(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(false);
    pUi->dateTimeEditTo->setEnabled(false);
}

void cSetNoAlarm::on(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(false);
    pUi->dateTimeEditTo->setEnabled(false);
}

void cSetNoAlarm::to(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(false);
    pUi->dateTimeEditTo->setEnabled(true);
}

void cSetNoAlarm::from(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(true);
    pUi->dateTimeEditTo->setEnabled(false);
}

void cSetNoAlarm::fromTo(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(true);
    pUi->dateTimeEditTo->setEnabled(true);
}
