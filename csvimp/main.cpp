#include "lv2service.h"
#include "others.h"
#include <QCoreApplication>
#include "lv2link.h"
#include "csvimp.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp  = QObject::trUtf8("-u|--user-id <id>           Set user id\n");
    lanView::appHelp += QObject::trUtf8("-U|--user-name <name>       Set user name\n");
    lanView::appHelp += QObject::trUtf8("-i|--input-file <path>      Set input file path\n");
    lanView::appHelp += QObject::trUtf8("-I|--input-stdin            Set input file is stdin\n");
    lanView::appHelp += QObject::trUtf8("-O|--to-source              Set output source file\n");
    lanView::appHelp += QObject::trUtf8("-s|--separator              Set field separator\n");
}

static const QString sComment = "// ";

int main (int argc, char * argv[])
{
    cLv2QApp app(argc, argv);
    SETAPP();
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = SN_SQL_NEED;

    // Elmentjük az aktuális könyvtárt
    QString actDir = QDir::currentPath();
    lv2csvimp   mo;
    if (mo.lastError != NULL) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    PDEB(VVERBOSE) << "Saved original current dir : " << actDir << "; actDir : " << QDir::currentPath() << endl;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    // Visszaállítjuk az aktuális könyvtárt
    QDir::setCurrent(actDir);
    PDEB(VVERBOSE) << "Restore act dir : " << QDir::currentPath() << endl;
    try {
        if (mo.fileNm.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Nincs megadva forrás fájl!"));
        PDEB(VVERBOSE) << "BEGIN transaction ..." << endl;
        if (mo.pOut == NULL) sqlBegin(*mo.pq, lanView::appName);
        mo.readcsv();
    } catch(cError *e) {
        mo.lastError = e;
    } catch(...) {
        mo.lastError = NEWCERROR(EUNKNOWN);
    }
    if (mo.lastError) {
        if (mo.pOut == NULL) sqlRollback(*mo.pq, lanView::appName);
        PDEB(DERROR) << "**** ERROR ****\n" << mo.lastError->msg() << endl;
    }
    else {
        if (mo.pOut == NULL) sqlEnd(*mo.pq, lanView::appName);
        PDEB(DERROR) << "**** OK ****" << endl;
    }

    cDebug::end();
    return mo.lastError == NULL ? 0 : mo.lastError->mErrorCode;
}

lv2csvimp::lv2csvimp() : lanView(), fileNm()
{
    pOutputFile = NULL;
    pOut        = NULL;
    separator   = QChar(',');

    if (lastError != NULL) {
        pq     = NULL;
        return;
    }

    int i;
    QString   userName;
    if (0 < (i = findArg('u', "user-id", args)) && (i + 1) < args.count()) {
        // ???
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('U', "user-name", args)) && (i + 1) < args.count()) {
        userName = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('i', "input-file", args)) && (i + 1) < args.count()) {
        fileNm = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('O', "to-source", args)) && (i + 1) < args.count()) {
        srcOutName = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('s', "separator", args)) && (i + 1) < args.count()) {
        separator = args[i + 1][0];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (args.count() > 1) DWAR() << trUtf8("Invalid arguments : ") << args.join(QChar(' ')) << endl;
    try {
        pq = newQuery();
        if (!userName.isNull()) setUser(userName);
    } CATCHS(lastError)
}

lv2csvimp::~lv2csvimp()
{
    DBGFN();
    if (pq     != NULL) {
        PDEB(VVERBOSE) << "delete pq ..." << endl;
        delete pq;
    }
    pDelete(pOut);
    pDelete(pOutputFile);
    DBGFNL();
}

// Mezők a CVS fájlban
enum csvIndex {
    CI_FLOR,    // Ápület és emelet
    CI_ROOM,    // Szobaszám
    CI_SIDE,    // Oldal (szaoba alszám)
    CI_WMC,     // Fali csatlakozó
    CI_WPORT,   // Fali csatlakozó port
    CI_SHARE,   // Megosztás (külső)
    CI_SZG,     // Számítógép neve (nem használt)
    CI_MAC,     // Számítógép MAC (nem használt)
    CI_SWITCH,  // Switch
    CI_SWPORT,  // Switch port
    CI_NOTE,    // Megjegyzés
    CI_CATEGORY,// Szoba kategória
    CI_NEED,    // Szükséges (tervezett) port szám
    CI_SIZE
};

// A CSV beolvasása, feldolgozása
void lv2csvimp::readcsv()
{
    QFileInfo fi(fileNm);
    if (fi.fileName() == fileNm) {
        fi = QFileInfo(QDir(homeDir), fileNm);
        fileNm = fi.filePath();
    }
    inputFile.setFileName(fileNm);
    if (srcOutName.isEmpty() == false) {
        fi = QFileInfo(srcOutName);
        if (fi.fileName() == srcOutName) {
            fi = QFileInfo(QDir(homeDir), srcOutName);
            srcOutName = fi.filePath();
        }
        pOutputFile = new QFile(srcOutName);
        if (!pOutputFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            EXCEPTION(EFOPEN, 0, pOutputFile->errorString());
        }
        pOut = new QTextStream(pOutputFile);
    }
    if (!inputFile.open(QIODevice::ReadOnly)) {
        EXCEPTION(EFOPEN, 0, fileNm);
    }

    QByteArray rawlin;  // Az aktuálisan beolvasott egy sor
    inputFile.readLine();       // Első sor a fejléc, eldobjuk
    while ((rawlin = inputFile.readLine()).isEmpty() == false) {
        QString line = QString::fromUtf8(rawlin);;
        if (line.endsWith(QChar('\n'))) line.chop(0);
        else {
            DWAR() << trUtf8("Input line nothing new line char : %1").arg(quotedString(line)) << endl;
            continue;
        }
        QStringList fields = line.split(separator);
        int siz = fields.size();
        if (siz < CI_SIZE) {
            if (siz > 1) {
                DWAR() << trUtf8("Input line is short : %1").arg(quotedString(line)) << endl;
                if (pOut) *pOut << "// SHORT : " << line << endl;
            }
            continue;
        }
        // Az esetleges idézöjelek törlése (üres sorok szűrése)
        QMutableListIterator<QString>   i(fields);
        bool empty = true;
        while (i.hasNext()) {
            QString s = i.next();
            s = s.simplified();
            if (s.size() >= 2 && s.startsWith(QChar('"')) && s.endsWith(QChar('"'))) {
                s = s.mid(1, s.size() - 2).simplified();
            }
            i.value() = s;
            empty = empty && s.isEmpty();
        }
        if (empty) continue;
        PDEB(VERBOSE) << "Line : " << quotedString(line) << " => " << fields.join(",") << endl;
        csvrow(fields);
    }
}

// A mezőkre bontott CVS sor feldolgozása
void lv2csvimp::csvrow(QStringList &fields)
{
    if (pOut) *pOut << "\n// *** " << fields.join(",") << endl;
    // +1 mező értéke "skip", eldobjuk
    if (fields.size() > CI_SIZE && 0 == fields.at(CI_SIZE).compare("skip", Qt::CaseInsensitive)) return;
    category = fields[CI_CATEGORY];
    // Nem létező vagy érdektelen szobaszámok
    if (0 == category.compare("nincs", Qt::CaseInsensitive)) return;
    if (0 == category.compare("WC",    Qt::CaseInsensitive)) return;
    const static QString note = QString("By CSVIMP %1").arg(QDateTime::currentDateTime().toString());
    if (fields[CI_FLOR].isEmpty() == false) floor = fields[CI_FLOR].toUpper();
    if (fields[CI_ROOM].isEmpty() == false) {
        side.clear();
        _room  = fields[CI_ROOM].toUpper();
    }
    if (fields[CI_SIDE].isEmpty() == false) side = fields[CI_SIDE].toUpper();
    if (floor.isEmpty() || _room.isEmpty()) {
        DWAR() << "Floor or room is empty" << endl;
        return;
    }
    // A szoba neve
    if (side.isEmpty()) room = _room;
    else                room = _room + "/" + side;
    if (placeFloor.getName() != floor) {                    // új
        placePatch.clear();
        nodeSwitch.clear();
        intSwitch.clear();
        pport.clear();
        placeRoom.clear();
        if (!placeFloor.fetchByName(*pq, floor)) {          // Nincs ilyenünk, eldobjuk
            QString msg = trUtf8("Floor : %1 place record not found.").arg(floor);
            DWAR() << msg << endl;
            if (pOut) *pOut << sComment << msg << endl;
            return;
        }
    }
    bool firstPort;
    // Helyiség, kategória
    if (placeRoom.getName() != room) {                      // Új
        firstPort = true;
        placePatch.clear();
        nodeSwitch.clear();
        intSwitch.clear();
        pport.clear();
        if (!placeRoom.fetchByName(*pq, room)) {            // Ha még nincs, Insert
            // Insert ROOM
            placeRoom.setName(room);
            placeRoom.setId(_sParentId, placeFloor.getId());
            placeRoom.setNote(note);
            if (pOut) {     // Nem közvetlen insert
                *pOut << placeRoom.codeInsert(*pq) << endl;
            }
            else {
                placeRoom.insert(*pq);
            }
        }
        if (pOut) {
            *pOut << "SET PLACE " << room << ";" << endl;
        }
        bool ok;
        int n = fields[CI_NEED].toInt(&ok);
        if (ok && n) category += QString("_%1").arg(n);
        if (category.isEmpty() == false) {  // Ha van kategória
            if (!plgrCat.fetchByName(category)) {   // Adatbázisban még nincs ilyen
                plgrCat.setName(category);
                plgrCat.setNote(note);
                plgrCat.setId(_sPlaceGroupType, PG_CATEGORY);
                if(pOut) {
                    if (!insertedCat.contains(plgrCat.getName())) { // Generált szövegben sem inzertáltuk még?
                        *pOut << plgrCat.codeInsert(*pq) << endl;
                        insertedCat << plgrCat.getName();
                    }
                }
                else {
                    plgrCat.insert(*pq);
                }
            }
            // A helyiséget betesszük a kategória csoportba
            tGroup<cPlaceGroup, cPlace> gsw(plgrCat, placeRoom);
            if (NULL_ID == gsw.test(*pq)) {     // már tagja ??
                if (pOut) {
                    *pOut << "PLACE GROUP " << quotedString(plgrCat.getName())
                          << " ADD " << quotedString(placeRoom.getName()) << _sSemicolonNl;
                }
                else {
                    gsw.insert(*pq);
                }
            }
        }
    }
    else {
        firstPort = false;
    }
    if (placeRoom.isIdent() == false) EXCEPTION(EPROGFAIL);
    // Rendező, ha meg van adva switch
    if (firstPort) {
        placePatch.clear();
        nodeSwitch.clear();
        intSwitch.clear();
        pport.clear();
        roomSocket.clear();
        roomSocket.setName(room + "_WS");
        roomSocket.setNote(note);
        if (pOut) {
            *pOut << roomSocket.codeInsert_() << ";" << endl;
        }
        else {
            roomSocket.setName(_sNodeType, _sPatch);
            roomSocket.setId(_sPlaceId, placeRoom.getId());
            roomSocket.cRecord::replace(*pq);
        }
    }
    swname = fields[CI_SWITCH];
    if (false == nodeSwitch.isEmpty() && false == swname.isEmpty()) {
        QString name = nodeSwitch.getName();
        if (swname != name && !name.startsWith(swname + ".")) {
            nodeSwitch.clear();
            intSwitch.clear();
            pport.clear();
        }
    }
    if (nodeSwitch.isEmpty() && false == swname.isEmpty()) {
        static const QString sql =
                "SELECT * FROM nodes "
                    "WHERE 'switch' = ANY (node_type) "
                      "AND (node_name = ? OR node_name LIKE ?)"
                    "ORDER BY node_name ASC";
        if (execSql(*pq, sql, swname, swname + ".%")) {
            nodeSwitch.set(*pq);
            if (pq->next() && nodeSwitch.getName() != swname) {   // Egynél több találat, és nem teljesen azonos név
                nodeSwitch.clear();
            }
        }
    }
    qlonglong swPlaceId = nodeSwitch.getId(_sPlaceId);
    if (placePatch.isNull() && swPlaceId > 0LL) {     // Van switch, helye, és az nem az "unknown" aminek az ID-je nulla.
        placePatch.setById(swPlaceId);
    }
    pport.clear();
    // Van egy rendezőnk?
    if (false == placePatch.isEmpty()) {
        wport = fields[CI_WPORT];
        // Valahogy meg kellene keresni a patch portot a rendezőben
        if (false == wport.isEmpty()) {
            static const QString sql =
                    "SELECT pports.* FROM pports JOIN patchs USING(node_id) "
                        "WHERE port_tag = ? AND place_id = ?";
            if (execSql(*pq, sql, wport, placePatch.getId())) {
                pport.set(*pq);
            }
        }
    }
    share = fields.at(CI_SHARE).toUpper().simplified();
    if (ES_INVALID == portShare(share, EX_IGNORE)) {
        DWAR() << "Invalid share : " << quotedString(share) << endl;
        share.clear();
    }
    // Patch a rendezőben
    if (false == nodeSwitch.isEmpty() && false == pport.isEmpty()) {
        swport = fields[CI_SWPORT];
        if (false == swport.isEmpty()) {
            if (nodeSwitch.ports.isEmpty()) nodeSwitch.fetchPorts(*pq);
            int ix = nodeSwitch.ports.indexOf(_sPortName, swport);
            if (ix >= 0) {
                if (pOut) {
                    *pOut << "LINKS FRONT TO TERM {"
                          << quotedString(cPatch().getNameById(pport.getId(_sNodeId)))
                          << ":" << pport.getId(_sPortIndex);
                    if (!share.isEmpty()) *pOut <<"/" << share;
                    *pOut << " & "
                          << quotedString(nodeSwitch.getName())
                          << ":" << nodeSwitch.ports.at(ix)->getId(_sPortIndex)
                          << " }" << endl;
                }
                else {
                    cPhsLink lnk;
                    lnk.setId(_sPortId1, pport.getId());
                    lnk.setId(_sPhsLinkType1, LT_FRONT);
                    lnk.setId(_sPortId2, nodeSwitch.ports.at(ix)->getId());
                    lnk.setId(_sPhsLinkType2, LT_TERM);
                    lnk.setName(_sPortShared, share);
                    lnk.setNote(note);
                    lnk.replace(*pq);
                }
            }
        }
    }
    // Fali csatlakozó (nincs munkaállomás, igy csak a megosztásnak nincs jelentősége)
    if (share.isEmpty() || share == _sA) {
        int n = roomSocket.ports.size() +1;
        QString pn = QString("J%1").arg(n);
        cNPort *pp = roomSocket.addPort(pn, note, n);
        pp->setName(_sPortTag, wport);
        pp->setId(_sNodeId, roomSocket.getId());
        if (pOut) {
            *pOut << "ADD PATCH " << quotedString(roomSocket.getName())
                  << " PORT " << n << " \"" << pn << "\" TAG " << quotedString(wport)
                  << _sSemicolon << endl;
        }
        else {
            pp->replace(*pq);
        }
        // Fali kábel
        if (false == pport.isEmpty()) {
            if (pOut) {
                *pOut << "LINKS BACK TO BACK {"
                      << quotedString(cPatch().getNameById(pport.getId(_sNodeId)))
                      << ":" << pport.getId(_sPortIndex)
                      << " & "
                      << quotedString(roomSocket.getName())
                      << ":" << pp->getId(_sPortIndex)
                      << " }" << endl;
            }
            else {
                cPhsLink lnk;
                lnk.setId(_sPortId1, pport.getId());
                lnk.setId(_sPhsLinkType1, LT_BACK);
                lnk.setId(_sPortId2, pp->getId());
                lnk.setId(_sPhsLinkType2, LT_BACK);
                lnk.setName(_sLinkType, _sPTP);
                lnk.setNote(note);
                lnk.replace(*pq);
            }
        }
    }
}

