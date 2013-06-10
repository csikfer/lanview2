#ifndef LV2GUI_H
#define LV2GUI_H

#include "lanview.h"
#include "lv2g.h"
#include "mainwindow.h"
#include <QLabel>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>

#define APPNAME "lv2gui"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  LV2GUI

enum eDialogType {
    DT_INSERT,      ///< Insert dialog
    DT_MODIFY,      ///< Modify dialog
    DT_INSERTED     ///< Insert dialog, object saved
};
enum eNodeType      { NT_PATCH, NT_HUB, NT_NODE, NT_HOST, NT_SNMPDEV };
enum ePortType      { PT_PPORT, PT_NPORT, PT_IFACE, PT_IFADDR };
enum eDialogObject  { NODE_DIALOG, PORT_DIALOG };

/// @class lv2Setup LanView2 setup main object
class lv2Gui : public lanView {
    Q_OBJECT
public:
    lv2Gui();
    ~lv2Gui();
    bool        dbIsOpen;
    cMainWindow *pMainWindow;
};

extern QString _titleWarning;
extern QString _titleError;
extern QString _titleInfo;

class cLv2App : public QApplication {
public:
    cLv2App(int& argc, char ** argv) : QApplication(argc, argv) { /* pAppErr = NULL */; }
    ~cLv2App();
    virtual bool notify(QObject * receiver, QEvent * event);
    // cError *pAppErr;
};

#include "setup.h"

#endif // LV2GUI_H
