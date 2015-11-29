#include "lv2gui.h"
#include "cerrormessagebox.h"

#define PREDEBUG    1

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp += QObject::trUtf8("-a|--app-name         APP name.\n");
}

int main(int argc, char * argv[])
{
    cError * pe = NULL;
    cLv2GQApp app(argc, argv);

    SETAPP();
    lanView::gui = true;
    lanView::snmpNeeded = false;
    if (0 <= findArg(QChar('s'),QString("setup"), app.arguments())) {
        lv2Gui::_setup = true;
        lanView::sqlNeeded = SN_NO_SQL;
    }
    int i;
    if (0 <= (i = findArg(QChar('a'),QString("app-name"), app.arguments()))
     && (i + 1) < app.arguments().count()) {
        lanView::appName = app.arguments()[i + 1];
    }

    lanView::sqlNeeded  = lv2Gui::_setup ? SN_NO_SQL : SN_SQL_TRY;

    lv2Gui   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        pe = mo.lastError;
    }
    else {
        try {
            mo.pMainWindow->show();
            app.exec();
        } CATCHS(pe);
    }
    if (pe != NULL) {
        if (pe->mErrorCode == eError::EOK) {
            PDEB(INFO) << "**** EOK exit ****" << endl;
        }
        else {
            PDEB(INFO) << "**** ERROR ****" << endl;
            cErrorMessageBox::messageBox(pe);
        }
        exit(pe->mErrorCode);
    }
    PDEB(INFO) << "**** OK ****" << endl;
    exit(0);
}

bool lv2Gui::_setup = false;
lv2Gui::lv2Gui() : lv2g()
{
    DBGFN();
    pMainWindow = NULL;
    if (!lastError) {
        try {
            pMainWindow = new cMainWindow();
            pMainWindow->show();
        } CATCHS(lastError)
    }
    DBGFNL();
}

lv2Gui::~lv2Gui()
{
    DBGFN();
    if (pMainWindow != NULL) delete pMainWindow;
    DBGFNL();
}

