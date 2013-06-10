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
    cLv2App app(argc, argv);

    SETAPP();
    lanView::gui = true;
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = false;    // Csak késöbb nyitunk adatbázist, hogy lehessen setup-olni.

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
        if (_titleWarning.isEmpty()) {
            _titleWarning       = trUtf8("Figyelmeztetés");
            _titleError         = trUtf8("Hiba");
            _titleInfo          = trUtf8("Megjegyzés");
        }
        dbIsOpen = lanView::openDatabase(false);
        pMainWindow = new cMainWindow(!dbIsOpen);
        pMainWindow->show();
    }
    DBGFNL();
}

lv2Gui::~lv2Gui()
{
    DBGFN();
    if (pMainWindow != NULL) delete pMainWindow;
    DBGFNL();
}

QString _titleWarning;
QString _titleError;
QString _titleInfo;

cLv2App::~cLv2App()
{
    ;
}

bool cLv2App::notify(QObject * receiver, QEvent * event)
{
    static cError *lastError = NULL;
    try {
        return QApplication::notify(receiver, event);
    }
    catch(no_init_&) { // Már letiltottuk a cError dobálást
        PDEB(VERBOSE) << "Dropped cError..." << endl;
        return false;
    }
    CATCHS(lastError)
    PDEB(DERROR) << "Error in " << __PRETTY_FUNCTION__ << endl;
    PDEB(DERROR) << "Receiver : " << receiver->objectName() << "::" << typeid(*receiver).name() << endl;
    PDEB(DERROR) << "Event : " << typeid(*event).name() << endl;
//    pAppErr = lastError;        // eltesszük, hogy ki tudjuk tenni az üzenetet
    cError::mDropAll = true;    // A további hiba dobálások nem kellenek
    cErrorMessageBox::messageBox(lastError);
    QApplication::exit(lastError->mErrorCode);  // kilépünk,
    return false;
}
