#ifndef CGSETUPWIDGET_H
#define CGSETUPWIDGET_H

#include <QObject>
#include <QWidget>
#include <QMediaPlayer>
#include "lv2g.h"
#include "ui_gsetup.h"
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QSound>
#endif

/// @class cGSetupWidget
/// Program GUI setup (tab) widget
class LV2GSHARED_EXPORT cGSetupWidget : public cIntSubObj
{
    Q_OBJECT
public:
    cGSetupWidget(QMdiArea *par);
    /// Minimális jogosultsági szint az eléréséhez.
    static const enum ePrivilegeLevel rights;
protected:
    void applicate();
    QSettings &qset;
    Ui_GSetup  *pUi;
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    QMediaPlayer *pSoundPlayer;
#else
    QSound     *pSound;
#endif
protected slots:
    void applicateAndRestart();
    void applicateAndExit();
    void applicateAndClose();
    void selectAlarmFile();
    void testAlarmFile();
};

#endif // CGSETUPWIDGET_H
