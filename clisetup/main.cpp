#include <QCoreApplication>
#include <QtCore>
#include <QSqlDatabase>
#include "lanview.h"


#define APPNAME "clisetup"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

static QTextStream stdo(stdout);
static QTextStream stdi(stdin);

static void set(const QString& _key, const QString& _text, qlonglong _def)
{
    bool ok;
    qlonglong i;
    do {
        i = lanView::getInstance()->pSet->value(_key, _def).toLongLong();
        stdo << QObject::tr("%1 (%2) : ").arg(_text).arg(i) << endl;
        QString s = stdi.readLine();
        s = s.simplified();
        if (s.isEmpty()) return;
        i = s.toLongLong(&ok);
    } while (!ok);
    lanView::getInstance()->pSet->setValue(_key, i);
}


static void set(const QString& _key, const QString& _text, const QString& _def = _sNul, bool _scr = false)
{
    QString s = lanView::getInstance()->pSet->value(_key, _def).toString();
    stdo << QObject::tr("%1 (%2) : ").arg(_text, s) << endl;
    s = stdi.readLine();
    s = s.simplified();
    if (s.isEmpty()) return;
    if (_scr) s = scramble(s);
    lanView::getInstance()->pSet->setValue(_key, s);
}

int main(int argc, char *argv[])
{
    cLv2QApp app(argc, argv);

    SETAPP()
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = SN_NO_SQL;

    lanView   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }

    cError *pe = nullptr;

    try {
        set(_sHomeDir,  QObject::tr("Munka könyvtér path"), lanView::homeDefault);
        set(_sSqlHost,  QObject::tr("SQL szerver címe"),    _sLocalHost);
        set(_sSqlPort,  QObject::tr("SQL szerver port"),    5432);
        set(_sSqlUser,  QObject::tr("SQL felhasználói név"),_sLanView2, true);
        set(_sSqlPass,  QObject::tr("SQL jelszó"),          "***",      true);
        set(_sDbName,   QObject::tr("Az adatbázis neve"),   _sLanView2);
        set(_sLogFile,  QObject::tr("Log fájl"),            _sStdErr);
        set(_sDebugLevel,QObject::tr("Nyomkövetési szint"), lanView::debugDefault);
        set(_sMibPath,  QObject::tr("MIB path lista"));
        lanView::getInstance()->pSet->sync();

        eTristate f = TS_NULL;
        do {
            stdo << QObject::tr("Kísérlet, az adatbázis megnyitására ? (I/n)") << endl;
            QString s = stdi.readLine();
            s = s.simplified();
            if (s.isEmpty() || s.compare("i", Qt::CaseInsensitive)) f = TS_TRUE;
            else if (s.compare("n", Qt::CaseInsensitive)) f = TS_FALSE;
        } while (f == TS_NULL);

        if (f == TS_TRUE) {
            QSqlDatabase *pDb = new  QSqlDatabase(QSqlDatabase::addDatabase(_sQPSql));
            if (!pDb->isValid()) {
                QSqlError le = pDb->lastError();
                stdo << QObject::tr("SQL DB ERROR #") + le.nativeErrorCode() << endl;
                stdo << "driverText   : " << le.driverText() << endl;
                stdo << "databaseText : " << le.databaseText() << endl;
            }
            else {  // OK
                pDb->setHostName(   lanView::getInstance()->pSet->value(_sSqlHost).toString());
                pDb->setPort(       lanView::getInstance()->pSet->value(_sSqlPort).toInt());
                pDb->setUserName(   scramble(lanView::getInstance()->pSet->value(_sSqlUser).toString()));
                pDb->setPassword(   scramble(lanView::getInstance()->pSet->value(_sSqlPass).toString()));
                pDb->setDatabaseName(lanView::getInstance()->pSet->value(_sDbName).toString());
                if (!pDb->open()) {
                    QSqlError le = pDb->lastError();
                    stdo << QObject::tr("SQL open ERROR #") + le.nativeErrorCode() << endl;
                    stdo << "driverText   : " << le.driverText() << endl;
                    stdo << "databaseText : " << le.databaseText() << endl;
                }
                else {
                    QSqlQuery q(*pDb);
                    QString msg;
                    if (!checkDbVersion(q, msg)) {
                        stdo << msg;
                    }
                }
            }
            delete pDb;
        }
    } CATCHS(pe)

    exit(pe == nullptr ? 0 : pe->mErrorCode);
}
