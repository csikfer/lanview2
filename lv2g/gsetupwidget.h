#ifndef CGSETUPWIDGET_H
#define CGSETUPWIDGET_H

#include <QObject>
#include <QWidget>
#include "lv2g.h"
#include "ui_gsetup.h"

class LV2GSHARED_EXPORT cGSetupWidget : public cOwnTab
{
    Q_OBJECT
public:
    cGSetupWidget(QSettings &__s, QWidget *par);
    static const enum ePrivilegeLevel rights;
protected:
    void applicate();
    QSettings &qset;
    Ui_GSetup  *pUi;
protected slots:
    void applicateAndRestart();
    void applicateAndExit();
    void applicateAndClose();
};

#endif // CGSETUPWIDGET_H
