#include "lanview.h"

QSqlDatabase *  getSqlDb(void)
{
    if (lanView::instance == NULL) EXCEPTION(EPROGFAIL, -1, "lanView::instance is NULL.");
    QSqlDatabase *pSqlDb;
    pSqlDb = lanView::instance->pDb;
    if (pSqlDb == NULL) EXCEPTION(EPROGFAIL, -1, "lanView::instance->pDb is NULL.");
    if (!pSqlDb->isOpen()) EXCEPTION(EPROGFAIL, -1, "cLanView::instance->pDb is set, but database is not opened.");
    if (!isMainThread()) {  // Minden thread-nak saját objektum kell!
        PDEB(INFO) << "Lock by threadMutex, in getSqlDb() ..." << endl;
        lanView::getInstance()->threadMutex.lock();
        QSqlDatabase *pDb;
        QString tn = currentThreadName();
        QMap<QString, QSqlDatabase *>&  map = lanView::getInstance()->dbThreadMap;
        QMap<QString, QSqlDatabase *>::iterator i = map.find(tn);
        if (i == map.end()) {
            pDb = new QSqlDatabase(QSqlDatabase::cloneDatabase(*pSqlDb, tn));
            if (!pDb->open()) SQLOERR(*pDb);
            map.insert(tn, pDb);
        }
        else {
            pDb = i.value();
        }
        lanView::getInstance()->threadMutex.unlock();
        pSqlDb = pDb;
        if (pSqlDb == NULL) EXCEPTION(EPROGFAIL, -1, QString("Thread %1 pdb is NULL.").arg(tn));
        if (!pSqlDb->isOpen()) EXCEPTION(EPROGFAIL, -1, QString("Thread %1 pdb is set, but database is not opened.").arg(tn));
    }
    return pSqlDb;
}

void dropThreadDb(const QString& tn, enum eEx __ex)
{

    lanView::getInstance()->threadMutex.lock();
    QMap<QString, QSqlDatabase *>&  map = lanView::getInstance()->dbThreadMap;
    QMap<QString, QSqlDatabase *>::iterator i = map.find(tn);
    if (i == map.end()) {
        lanView::getInstance()->threadMutex.unlock();
        if (__ex) EXCEPTION(EPROGFAIL);
        return;
    }
    delete i.value();
    map.erase(i);
    lanView::getInstance()->threadMutex.unlock();
}


bool executeSqlScript(QFile& file, QSqlDatabase *pDb, enum eEx __ex)
{
    cError *   pe = NULL;
    QSqlQuery *pq = NULL;
    try {
        QString sql;    // Egy SQL parancs
        QString line;   // Egy beolvasott sor (idézőjel esetén több sor)
        if (!file.exists()) EXCEPTION(ENOTFILE,-1,file.fileName());
        if (!file.isOpen() && !file.open(QIODevice::ReadOnly)) EXCEPTION(EFOPEN,-1,file.fileName());
        QTextStream script(&file);
        script.setCodec("UTF-8");
        pq = newQuery(pDb);
        while ((line = script.readLine()).isNull() == false) {
            PDEB(VVERBOSE) << "Line : <<<\n" << line << "\n>>>" << endl;
            const QChar   a('\''), q('"'), e('\\');
            QChar cc;
            const QString c("--");
            int ia, iq, ic, ii, i;
            i = 0;
            // A /* */ komment párt csak külön sorban fogadja el!!
            if (line == "/*") {
                while ((line = script.readLine()).isNull() == false) {
                    if (line == "*/") break;
                }
                continue;
            }
            // komment, és string határolók keresése, stringhatároló találat esetén belép a ciklusba
            while ((bool)(((ic = line.indexOf(c, i)), (ia = line.indexOf(a, i))) >= 0) | (bool)((iq = line.indexOf(q, i)) >= 0)) {
                PDEB(VVERBOSE) << "MATCH : i = " << i << ", ia = " << ia << ", iq = " << iq << ", ic = " << ic << endl;
                // Találtunk idézőjelet ill. aposztrófot.
                if (ia < 0 || (iq >= 0 && iq < ia)) { // Egy idézőjelet találltunk, nincs elötte aposztróf
                    if (ic >= 0 && ic < iq) break;   // A komment elöbb volt
                    cc = q;
                    ii = iq;
                }
                else {                              // Egy aposztrófot találltunk, nincs elötte idézőjel
                    if (ic >= 0 && ic < ia) break;   // A komment elöbb volt
                    cc = a;
                    ii = ia;
                }
                ic = -1;    // Ha találtunk komment jelet, az nem komment volt (ld fentebb: if ... break;)
                PDEB(VVERBOSE) << "Find end mark : ii = " << ii << ", cc ='" << cc << "'" << endl;
                while (true) {
                    while(true) {
                        i = line.indexOf(cc, ii +1);     // Keressük a párját
                        if (i > 0 && e == line[i -1]) {
                            ii = i;                     // Védett karakter volt, nem a string vége, újra
                        }
                        else break;
                    }
                    if (i > 0) {    // Megvan a záró idézőjel
                        ++i;
                        break;
                    }
                    else {          // Nincs meg a záró jel, következő sort be kell olvasni.
                        if (script.atEnd()) EXCEPTION(EPARSE,-1,line);    // Ha vége a fájlnak, az gáz
                        line += QChar('\n') + script.readLine();
                        PDEB(VVERBOSE) << "line appended : <<<\n" << line << "\n>>>" << endl;
                    }
                }
            }
            if (ic == 0) {
                PDEB(VVERBOSE) << "Drop line..." << endl;
                line.clear();
                continue;
            }
            if (ic > 0) {
                PDEB(VVERBOSE) << "Strip comment. pos = " << ic << endl;
                line = line.left(ic);
            }
            line = line.trimmed();
            if (line.isEmpty()) continue;
            sql += line;
            line.clear();
            if (sql.endsWith(";")) {
                sql.chop(1);    // Lekapjuk a végéről a ";"-t
                PDEB(SQL) << "SQL EXEC <<<\n" << sql << "\n>>>" << endl;
                if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
                sql.clear();
            }
            else {
                PDEB(VVERBOSE) << "No semicolon, read next line." << endl;
                sql += ' '; // Kell(het) egy határoló, mert lepucoltuk
            }
        }
        if (!line.isEmpty() || !sql.isEmpty()) EXCEPTION(EPARSE,-1,sql + line);
    } CATCHS(pe)
    if (pq) delete pq;
    if (pe) {
        if (lanView::exist()) lanView::getInstance()->lastError = pe;
        if (__ex) pe->exception();
        return false;
    }
    return true;
}

/* ********************************************************************************************* */

bool sqlBegin(QSqlQuery& q)
{
    if (q.exec(_sBEGIN)) return true;
    SQLPREPERR(q, _sBEGIN);
    return false;
}

bool sqlEnd(QSqlQuery& q)
{
    if (q.exec(_sEND)) return true;
    SQLPREPERR(q, _sEND);
    return false;
}

bool sqlRollback(QSqlQuery& q)
{
    if (q.exec(_sROLLBACK)) return true;
    SQLPREPERR(q, _sROLLBACK);
    return false;
}

EXT_ bool execSql(QSqlQuery& q, const QString& sql, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    if (!v1.isValid()) {
        if (!q.exec(sql)) SQLQUERYERR(q);
    }
    else {
        if (!q.prepare(sql)) SQLPREPERR(q, sql);
        q.bindValue(0,v1);
        if (v2.isValid()) {
            q.bindValue(1,v2);
            if (v3.isValid()) {
                q.bindValue(2,v3);
                if (v4.isValid()) {
                    q.bindValue(3,v4);
                    if (v5.isValid()) {
                        q.bindValue(4,v5);
                    }
                }
            }
        }
        if (!q.exec()) SQLQUERYERR(q);
    }
    return q.first();
}

EXT_ bool execSqlFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    QString sql = "SELECT " + fn + QChar('(');
    if (v1.isValid()) {
        sql += QChar('?');
        if (v2.isValid()) {
            sql += _sCommaQ;
            if (v3.isValid()) {
                sql += _sCommaQ;
                if (v4.isValid()) {
                    sql += _sCommaQ;
                    if (v5.isValid()) {
                        sql += _sCommaQ;
                    }
                }
            }
        }
    }
    sql += QChar(')');
    return execSql(q, sql,v1, v2, v3, v4, v5);
}

EXT_ qlonglong execSqlIntFunction(QSqlQuery& q, bool *pOk, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    bool ok = execSqlFunction(q, fn, v1, v2, v3, v4, v5);
    qlonglong r = NULL_ID;
    if (ok) {
        QVariant v = q.value(0);
        r = v.toLongLong(&ok);
        if (!ok) r = NULL_ID;
    }
    if (pOk != NULL) *pOk = ok;
    return r;
}

EXT_ bool execSqlBoolFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    bool ok = execSqlFunction(q, fn, v1, v2, v3, v4, v5);
    QString r;
    if (ok) r = q.value(0).toString();
    return str2bool(r);
}

EXT_ QString execSqlTextFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    bool ok = execSqlFunction(q, fn, v1, v2, v3, v4, v5);;
    QString r;
    if (ok) r = q.value(0).toString();
    return r;
}

EXT_ bool execSqlRecFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    static const QString s = "* FROM ";
    return execSqlFunction(q, s + fn, v1, v2, v3, v4, v5);
}

EXT_ void sqlNotify(QSqlQuery& q, const QString& channel, const QString& payload)
{
    QString sql = "NOTIFY " + channel;
    if (!payload.isEmpty()) sql += ", " + payload;
    if (!q.exec(sql)) SQLQUERYERR(q);
}

