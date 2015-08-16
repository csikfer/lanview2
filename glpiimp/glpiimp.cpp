#include <QCoreApplication>
#include "glpiimp.h"


#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp += QObject::trUtf8("-U|--glpi-user <name>       Set GLPI database user name\n");
    lanView::appHelp += QObject::trUtf8("-i|--input-file <path>      Set input file path\n");
    lanView::appHelp += QObject::trUtf8("-I|--input-stdin            Set input file is stdin\n");
    lanView::appHelp += QObject::trUtf8("-D|--daemon-mode            Set daemon mode\n");
}

QString Glpi_host;
QString Glpi_user;
QString Glpi_passwd;

int main(int argc, char *argv[])
{
    cLv2QApp app(argc, argv);
    SETAPP();
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = true;
    lanView::gui        = false;
    lanView mo;
    if (mo.lastError != NULL) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    openGlpiDb();

}


void openGlpiDb()
{

}
