#ifndef GLPIIMP
#define GLPIIMP

#include <Qt>
#include "lanview.h"

#define APPNAME "import"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

extern QSqlDatabase *pGlpiDb;
extern void openGlpiDb();
extern void computerstFromGlpi();
extern void doComputer(const QSqlRecord& rec);

#endif // GLPIIMP

