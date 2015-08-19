#include <QCoreApplication>
#include "glpiimp.h"


#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp += QObject::trUtf8("-H|--glpi-host              GLPI host\n");
    lanView::appHelp += QObject::trUtf8("-U|--glpi-user <name>       GLPI database user name\n");
    lanView::appHelp += QObject::trUtf8("-P|--password <path>        GLPI database user password\n");
}

QString glpi_host;
QString glpi_user;
QString glpi_passwd;
QSqlDatabase *pGlpiDb = NULL;

int main(int argc, char *argv[])
{
    cLv2QApp app(argc, argv);
    SETAPP();
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = true;
    lanView::gui        = false;
    lanView mo;
    int i;
    if (mo.lastError != NULL) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    if (0 < (i = findArg('H', "glpi-host", mo.args)) && (i + 1) < mo.args.count()) {
        glpi_host = mo.args[i + 1];
        mo.args.removeAt(i);
        mo.args.removeAt(i);
    }
    if (0 < (i = findArg('U', "glpi-user-name", mo.args)) && (i + 1) < mo.args.count()) {
        glpi_user = mo.args[i + 1];
        mo.args.removeAt(i);
        mo.args.removeAt(i);
    }
    if (0 < (i = findArg('P', "password", mo.args)) && (i + 1) < mo.args.count()) {
        glpi_passwd = mo.args[i + 1];
        mo.args.removeAt(i);
        mo.args.removeAt(i);
    }
    openGlpiDb();
    computerstFromGlpi();
}


void openGlpiDb()
{
    PDEB(VERBOSE) << "Open GLPI database ..." << endl;
    pGlpiDb = new  QSqlDatabase();
    *pGlpiDb = QSqlDatabase::addDatabase("QMYSQL", "GLPI");
    if (!pGlpiDb->isValid()) SQLOERR(*pGlpiDb);
    pGlpiDb->setHostName(glpi_host);
    pGlpiDb->setPort(3306);
    pGlpiDb->setUserName(glpi_user);
    pGlpiDb->setPassword(glpi_passwd);
    pGlpiDb->setDatabaseName("glpi");
    bool r = pGlpiDb->open();
    if (!r) {
        QSqlError le = (*pGlpiDb).lastError();
        _sql_err_deb_(le, __FILE__, __LINE__, __PRETTY_FUNCTION__);
        EXCEPTION(ESQLOPEN, le.number(), le.text());
    }
}


void computerstFromGlpi()
{
    QSqlQuery   glpi(*pGlpiDb);
    QSqlQuery   q = getQuery();

    QString sql =
"SELECT "
    "c.id, c.name AS node_name,serial,otherserial AS inventory, c.comment, l.completename AS place_name, "
    "c.date_mod, o.name AS opsys_name, d.name AS domain_name "
  "FROM   glpi_computers        AS c "
    "JOIN glpi_locations        AS l ON c.locations_id        = l.id "
    "JOIN glpi_domains          AS d ON c.domains_id          = d.id "
    "JOIN glpi_operatingsystems AS o ON c.operatingsystems_id = o.id "
  "WHERE c.is_deleted = 0";
  if (execSql(glpi, sql)) do {
      doComputer(glpi.record());
  } while (glpi.next());

}

void doComputer(const QSqlRecord& rec)
{
    qlonglong   id      = rec.value("id").toLongLong();
    QString     name    = rec.value("name").toString();
    QString     domain  = rec.value("domain_name").toString();
    QString     serial  = rec.value("serial").toString();
    QString     invent  = rec.value("inventory").toString();
    QString     place   = rec.value("place_name").toString();
    QDateTime   modify  = rec.value("date_mod").toDateTime();
    QString     opsys   = rec.value("opsys_name").toString();

    QStringList l;
    l << QString::number(id);
    l << name << domain << serial << invent << place << modify.toString() << opsys;
    PDEB(VERBOSE) << l.join(", ") << endl;

    cNode node;
    if (!name.contains(QChar('.')) && !domain.isEmpty()) name = mCat(name. domain);
    qlonglong nid = node.getIdByName(name, false);      // Van ilyen node ?
    if (nid != NULL_ID) {
        node.setById(nid);
    }
    else {
        node.setName(name);
    }
    if (!serial.isEmpty()) node.params[_sSerialNumber]    = serial;
    if (!invent.isEmpty()) node.params[_sInventoryNumber] = invent;
    qlonglong pid = placeId(place);
    if (pid != NULL_ID) node.setId(_sPlaceId, pid);

    QString sql =
"SELECT name, ip, mac "
  "FROM glpi_networkports AS p "
    "JOIN glpi:networkinterfaces AS i ON p.networkinterfaces_id = i.id "
  "WHERE p.itemtype = \"Computer\" AND i.name = \"Ethernet\" AND p.items_id = ?";
    QSqlQuery   glpi(*pGlpiDb);
    if (execSql(glpi, sql, QVariant(id))) do {

    } while (glpi.next());



}

qlonglong placeId(const QString& place_full_name)
{
    if (place_full_name.isEmpty()) return NULL_ID;
    QStringList pl = place_full_name.split(">");
    // 3 szint van az elsú a KKK, második az épület+emelet, harmadik a szoba száma
    if (pl.size() != 3 && 0 != pl.at(0).simplified().compare("KKK", Qt::CaseInsensitive)) return NULL_ID;
    QString parentName = pl.at(1).simplified().toUpper();   // Épület "név" + emelet mindíg nagybetű
    if (parentName.size() != 2) {                           // és mindíg 2 betű
        DEBP(INFO) << "Invalid place parent name : " << parentName << " full : " << place_full_name << endl;
        return NULL_ID;
    }
    QString placeName = pl.at(2).simplified().toLower();    // Szoba szám, és ha van betű jel, az kisbetü.
    QRegExp m("(\\d+[a-z]?).*");
    if (!m.exactMatch(placeName)) {                         // Ez nem hasonlít szobaszámra
        DEBP(INFO) << "Invalid place name : " << placeName << " full : " << place_full_name << endl;
        return NULL_ID;
    }
    placeName = parentName + m.cap();                       // Épület + emelet + szobaszám: a helyiság neve
    cPlace place;
    qlonglong id = place.getIdByName(placeName, false);     // Van ilyen az adatbázisban?
    if (id != NULL_ID) return id;                           // Ha van, örül
    // Ha nincs, akkor beszúrjuk
    qlonglong pid = place.getIdByName(parentName, false);   // A parent ID, ha van
    if (pid == NULL_ID) {   // Nincs parentünk se ???
        DEBP(INFO) << "Place parent not found : " << parentName << " full : " << place_full_name << endl;
    }
    place.clear().setName(placeName).setId(_sParentId, pid).setName(_sPlaceNote, "Import from GLPI");
    QSqlQuery q = getQuery();
    place.insert(q);
    return place.getId();
}
