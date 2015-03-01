#include "lv2gui.h"
#include "cerrormessagebox.h"

#define PREDEBUG    1

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp += QObject::trUtf8("-s|--setup            Setup.\n");
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
    }
    int i;
    if (0 <= (i = findArg(QChar('a'),QString("app-name"), app.arguments()))
     && (i + 1) < app.arguments().count()) {
        lanView::appName = app.arguments()[i + 1];
    }

    lanView::sqlNeeded  = !lv2Gui::_setup;

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
        PDEB(INFO) << "**** ERROR ****" << endl;
        cErrorMessageBox::messageBox(pe);
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
            pMainWindow = new cMainWindow(_setup);
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

