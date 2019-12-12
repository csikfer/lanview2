#ifndef PICKERS_H
#define PICKERS_H

#include "lv2g_global.h"
#include <QDialog>


#include "ui_datetimedialog.h"


class LV2GSHARED_EXPORT cDateTimeDialog : public QDialog {
    Q_OBJECT
protected:
    cDateTimeDialog(QWidget *p = nullptr);
    ~cDateTimeDialog();
    Ui::DialogDateTime  *pUi;
    QDateTime   min;
    QDateTime   max;
    QDateTime   def;
public:
    static QDateTime popup(const QDateTime &_dt, const QDateTime &_def = QDateTime(), const QDateTime &_min = QDateTime(), const QDateTime &_max = QDateTime());
private slots:
    void on_pushButtonToday_clicked();
    void on_pushButtonTomorrow_clicked();
    void on_calendarWidget_selectionChanged();
    void on_toolButtonDef_clicked();
};

class LV2GSHARED_EXPORT cSelectDialog : public QDialog {
    Q_OBJECT
protected:
    cSelectDialog(QWidget *p = nullptr);
    void setValues(const QStringList& _vl, bool _m);
    QStringList     values;
    bool            multiSelect;
    QVBoxLayout   * pLayout;
    QDialogButtonBox *pDialogButtons;
    QButtonGroup *  pButtonGroup;
public:
    static int radioButtons(const QString &_t, const QStringList& _vl, const QString& _txt = QString(), QWidget *_par = nullptr);
    static qlonglong checkBoxs(const QString &_t, const QStringList& _vl, const QString& _txt = QString(), QWidget *_par = nullptr);
};


#endif // PICKERS_H
