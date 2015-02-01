#include "lv2gui.h"
#include "cerrormessagebox.h"

#define PREDEBUG    1

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    ;
}

int main(int argc, char * argv[])
{
    cError * pe = NULL;
    cLv2QApp app(argc, argv);

    SETAPP();
    lanView::gui = true;
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = false;    // Csak késöbb nyitunk adatbázist, hogy lehessen setup-olni.

    initLV2GUI();

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
//  if (app.pAppErr != NULL) pe = app.pAppErr;
    if (pe != NULL) {
        PDEB(INFO) << "**** ERROR ****" << endl;
        cErrorMessageBox::messageBox(pe);
        exit(pe->mErrorCode);
    }
    PDEB(INFO) << "**** OK ****" << endl;
    exit(0);
}

lv2Gui::lv2Gui() : lanView()
{
    DBGFN();
    pMainWindow = NULL;
    if (!lastError) {
        try {
            dbIsOpen = lanView::openDatabase(false);
            pMainWindow = new cMainWindow(!dbIsOpen);
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

