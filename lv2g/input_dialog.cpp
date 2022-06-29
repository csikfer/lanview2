#include "input_dialog.h"

#include <QDateTimeEdit>
#include <QCalendarWidget>
#include <QButtonGroup>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>

cDateTimeDialog::cDateTimeDialog(QWidget *p) : QDialog(p)
{
    pUi = new Ui::DialogDateTime;
    pUi->setupUi(this);

    connect(pUi->pushButtonToday,    &QPushButton::clicked,              this, &cDateTimeDialog::_pushButtonToday_clicked);
    connect(pUi->pushButtonTomorrow, &QPushButton::clicked,              this, &cDateTimeDialog::_pushButtonTomorrow_clicked);
    connect(pUi->calendarWidget,     &QCalendarWidget::selectionChanged, this, &cDateTimeDialog::_calendarWidget_selectionChanged);
    connect(pUi->toolButtonDef,      &QPushButton::clicked,              this, &cDateTimeDialog::_toolButtonDef_clicked);

}

cDateTimeDialog::~cDateTimeDialog()
{
    delete pUi;
}

QDateTime cDateTimeDialog::popup(const QDateTime& _dt, const QDateTime& _def, const QDateTime& _min, const QDateTime& _max)
{
    cDateTimeDialog o;
    if (_min.isNull()) o.min = QDateTime::currentDateTime();
    else               o.min = _min;
    o.max = _max;
    o.pUi->calendarWidget->setSelectedDate(_dt.date());
    o.pUi->calendarWidget->setMinimumDate(o.min.date());
    o.pUi->timeEdit->setTime(_dt.time());
    o.def = _def;
    if (o.def.isNull()) o.pUi->toolButtonDef->setDisabled(true);
    if (_dt.date() <= o.min.date()) o.pUi->timeEdit->setMinimumTime(o.min.time());
    if (o.exec() != QDialog::Accepted) return QDateTime();
    QDateTime dt;
    dt.setDate(o.pUi->calendarWidget->selectedDate());
    dt.setTime(o.pUi->timeEdit->time());
    return dt;
}


void cDateTimeDialog::_pushButtonToday_clicked()
{
    pUi->calendarWidget->setSelectedDate(QDate::currentDate());
}

void cDateTimeDialog::_pushButtonTomorrow_clicked()
{
    pUi->calendarWidget->setSelectedDate(QDate::currentDate().addDays(1));
}

void cDateTimeDialog::_calendarWidget_selectionChanged()
{
    QDate d = pUi->calendarWidget->selectedDate();
    if (d <= min.date()) pUi->timeEdit->setMinimumTime(min.time());
    if (max.isValid() && d >= max.date()) pUi->timeEdit->setMinimumTime(max.time());
}

void cDateTimeDialog::_toolButtonDef_clicked()
{
    if (def.isValid()) {
        pUi->calendarWidget->setSelectedDate(def.date());
        pUi->timeEdit->setTime(def.time());
    }
}

cSelectDialog::cSelectDialog(QWidget *p) : QDialog(p)
{
    pLayout =        new QVBoxLayout;
    pButtonGroup   = new QButtonGroup(this);
    pDialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    setLayout(pLayout);
    connect(pDialogButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pDialogButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void cSelectDialog::setValues(const QList<QString>& values, bool _m)
{
    QAbstractButton *pButton;
    int id = 0;
    for (const QString& val: values) {
        if (_m) pButton = new QCheckBox(val);
        else    pButton = new QRadioButton(val);
        pButtonGroup->addButton(pButton, id);
        pLayout->addWidget(pButton);
        ++id;
    }
    pLayout->addWidget(pDialogButtons);
    pButtonGroup->setExclusive(!_m);
}

int cSelectDialog::radioButtons(const QString& _t, const QList<QString> &values, const QString &_txt, QWidget * _par)
{
    cSelectDialog o(_par);
    o.setWindowTitle(_t);
    if (!_txt.isEmpty()) o.pLayout->addWidget(new QLabel(_txt));
    o.setValues(values, false);
    int r = o.exec();
    if (r != QDialog::Accepted) return -1;
    return  o.pButtonGroup->checkedId();
}

qlonglong cSelectDialog::checkBoxs(const QString &_t, const QList<QString> &values, const QString &_txt, QWidget *_par)
{
    cSelectDialog o(_par);
    o.setWindowTitle(_t);
    if (!_txt.isEmpty()) o.pLayout->addWidget(new QLabel(_txt));
    o.setValues(values, true);
    if (o.exec() != QDialog::Accepted) return 0;
    const qsizetype n = o.pButtonGroup->buttons().count();
    qlonglong r = 0;
    for (int i = 0; i < n; ++i) {
        if (o.pButtonGroup->button(i)->isChecked()) r |= qlonglong(1) << i;
    }
    return  r;
}

