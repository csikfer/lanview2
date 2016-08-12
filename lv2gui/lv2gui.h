#ifndef LV2GUI_H
#define LV2GUI_H

#include "lv2g.h"
#include "menu.h"
#include "mainwindow.h"
#include <QLabel>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>

#define APPNAME "lv2gui"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

/// @class lv2Gui
/// LanView2 GUI main object
class lv2Gui : public lv2g {
    Q_OBJECT
public:
    lv2Gui();
    ~lv2Gui();
    bool        dbIsOpen;       ///< Ha az adatbázis nyitva, akkor true, egyébként false
    cMainWindow *pMainWindow;
    static bool _setup;
    static enum eScreen { NORMAL, MAXIMIZE, FULLSCREEN } _screen;
    static int  _nScreen;
};

#include "setup.h"

#endif // LV2GUI_H
