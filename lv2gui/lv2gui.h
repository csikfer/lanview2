#ifndef LV2GUI_H
#define LV2GUI_H

#include "lanview.h"
#include "lv2g.h"
#include "menu.h"
#include "mainwindow.h"
#include <QLabel>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>

#define APPNAME "lv2gui"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  LV2GUI

/// @class lv2Gui
/// LanView2 GUI main object
class lv2Gui : public lanView {
    Q_OBJECT
public:
    lv2Gui();
    ~lv2Gui();
    bool        dbIsOpen;       ///< Ha az adatbázis nyitva, akkor true, egyébként false
    cMainWindow *pMainWindow;
};

/// @class cLv2App
/// Saját QApplication osztály, a hiba kizárások elkapásához (újra definiált notify() metódus.)
class cLv2App : public QApplication {
public:
    /// Konstruktor. Nincs saját inicilizálás, csak a QApplication konstrujtort hívja.
    cLv2App(int& argc, char ** argv) : QApplication(argc, argv) { /* pAppErr = NULL */; }
    ~cLv2App();
    /// Az újra definiált notify() metódus.
    /// Az esetleges kizárásokat elkapja.
    virtual bool notify(QObject * receiver, QEvent * event);
    // cError *pAppErr;
};

#include "setup.h"

#endif // LV2GUI_H
