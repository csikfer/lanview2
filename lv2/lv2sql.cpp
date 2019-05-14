#include "lanview.h"

QString unTypeQuoted(const QString& _s)
{
    QString s = _s;
    int ix = s.lastIndexOf("::");
    if (ix > 0) s = s.mid(0, ix);
    return unQuoted(s);
}

QVariantList _sqlToIntegerList(const QString& _s)
{
    QVariantList vl;
    if (!_s.isEmpty()) {
        QStringList sl = _s.split(QChar(','),QString::KeepEmptyParts);
        foreach (const QString& s, sl) {
            bool ok;
            qlonglong i = s.toLongLong(&ok);
            if (ok) {
                vl << QVariant(i);
            }
            else {
                if (s == _sNULL) vl << QVariant();
                else EXCEPTION(EDBDATA, 0, "Invalid number : " + s);
            }
        }
    }
    return vl;

}

QVariantList _sqlToDoubleList(const QString& _s)
{
    QVariantList vl;
    if (!_s.isEmpty()) {
        QStringList sl = _s.split(QChar(','),QString::KeepEmptyParts);
        foreach (const QString& s, sl) {
            bool ok;
            double i = s.toDouble(&ok);
            if (ok) {
                vl << QVariant(i);
            }
            else {
                if (s == _sNULL) vl << QVariant();
                else EXCEPTION(EDBDATA, 0, "Invalid number : " + s);
            }
        }
    }
    return vl;
}

QStringList _sqlToStringList(const QString& _s)
{
    QStringList sl;
    if (!_s.isEmpty()) {
        sl = _s.split(QChar(','), QString::KeepEmptyParts);
        static const QChar   m('"');
        static const QString nm("\\\"");

        for (int i = 0; i < sl.size(); ++i) {
            QString s = sl[i];
            if (s[0] == m) {
                while (true) {   // Ha szétdaraboltunk egy stringet, újra összerakjuk
                    bool fragment = ! s.endsWith(m);
                    if (!fragment) {
                        if (s.endsWith(nm)) {
                            fragment = true;
                            s.chop(2);
                            s += m;
                        }
                    }
                    if (!fragment) break;
                    if (sl.size() <= (i +1)) EXCEPTION(EDBDATA, 8, QObject::trUtf8("Invalid (fragment) string array : ") + _s);
                    sl[i] += QChar(',') + (s = sl[i +1]);
                    sl.removeAt(i +1);
                }
                s = sl[i] = sl[i].mid(1, sl[i].size() -2);  // Lekapjuk az idézőjelet
            }
            else if (s == _sNULL) {
                sl[i].clear();
                continue;
            }
            else if (s.isEmpty()) {
                EXCEPTION(EDBDATA, 8, QObject::trUtf8("Invalid string array (empty text) : ") + _s);
            }
            sl[i].replace(nm, "\"");
        }
    }
    return sl;
}

static const QString sEmptyArray = "{}";

QString stringListToSql(const QStringList& sl)
{
    QString r = sEmptyArray;
    if (sl.isEmpty()) return r;
    r.chop(1);
    foreach (const QString& s, sl) {
        if (s.isNull()) r += "NULL,";
        else            r += dQuoted(QString(s).replace("\"", "\\\"")) + QChar(',');
    }
    r.chop(1);  // Drop last comma
    r += QChar('}');
    return r;
}

QString integerListToSql(const QVariantList& vl)
{
    QString r = sEmptyArray;
    if (vl.isEmpty()) return r;
    r.chop(1);
    foreach (QVariant v, vl) {
        if (v.isNull()) r += "NULL,";
        else {
            bool ok;
            r += QString::number(v.toLongLong(&ok)) + QChar(',');
            if (!ok) EXCEPTION(EDATA,-1,QObject::trUtf8("Invalid number"));
        }

    }
    r.chop(1);  // Drop last comma
    r += QChar('}');
    return r;
}

QString doubleListToSql(const QVariantList& vl)
{
    QString r = sEmptyArray;
    if (vl.isEmpty()) return r;
    r.chop(1);
    foreach (QVariant v, vl) {
        if (v.isNull()) r += "NULL,";
        else {
            bool ok;
            r += QString::number(v.toDouble(&ok)) + QChar(',');
            if (!ok) EXCEPTION(EDATA,-1,QObject::trUtf8("Invalid number"));
        }

    }
    r.chop(1);  // Drop last comma
    r += QChar('}');
    return r;
}

netAddress sql2netAddress(QString as)
{
    netAddress a;
    if (as.count() > 0) {
        // néha hozzábigyeszt valamit az IPV6 címhez
        int i = as.indexOf(QRegExp("[^\\d\\.:A-Fa-f/]"));
        // Ha a végén van szemét, azt levágjuk
        if (i > 0) as = as.mid(0, i);
        a.set(as);
    }
    return a;
}


/// Az intervallum qlonglong-ban tárolódik, és mSec-ben értendő.
/// Konvertál egy ISO 8601 idő intervallum stringet mSec-re.
/// Az előjel megadását nem támogatja.
/// A bevezető 'P' jelenlétét nem ellenörzi, feltételezi, hogy az első karakter egy 'P'
/// A space és tab karaktereket konverzió elött eltávolítja.
/// @param _s A konvertálandó string
/// @param pOk egy bool pointer, ha nem NULL, és az s sem üres, akkor ha sikerült a konverzió a mutatott változót true-ba írja.
///            ha pOk nem NULL. és az s üres, vagy a konverzió nem sikerült, akkor mutatott változót false-ba írja.
/// @return A konvertált érték, vagy NULL_ID ha nem sikerült a kovverzió.
qlonglong _parseTimeIntervalISO8601(const QString& _s, bool *pOk)
{
    qlonglong   r = 0;
    bool ok = true;
    int n = 0;
    bool alternate = false;
    QString s = _s;
    s.remove(QChar(' '));
    s.remove(QChar('\t'));
    s = s.mid(1);   // Dropp 'P'
    for (int i = 1; ok == true && i < s.size(); ++i) {
        QChar c = s[i].toUpper();
        bool day = true;
        switch (c.toLatin1()) {
        case '-':
            ok = false;         // exit for
            alternate = true;   // Alternate format ?
            break;
         case 'Y':
            if (!day) {
                ok = false;
                r = NULL_ID;
            }
            else {  // Year
                r += n * (1000LL * 3600 * 24 * 365);  // ?
                n = 0;
            }
            break;
        case 'M':
            if (day) {  // day
                r += n * (1000LL * 3600 * 24 * 30);
                n = 0;
            }
            else {      // min
                r += n * (1000LL * 60);
                n = 0;
            }
            break;
        case 'W':
            if (!day) {
                ok = false;
                r = NULL_ID;
            }
            else {  // Week
                r += n * (1000LL * 3600 * 24 * 7);
                n = 0;
            }
            break;
        case 'D':
            if (!day) {
                ok = false;
                r = NULL_ID;
            }
            else {  // Day
                r += n * (1000LL * 3600 * 24);
                n = 0;
            }
            break;
        case 'T':
            day = false;
            break;
        case 'H':
            if (day) {
                ok = false;
                r = NULL_ID;
            }
            else {  // Hour
                r += n * (1000LL * 3600);
                n = 0;
            }
            break;
        case 'S':
            if (day) {
                ok = false;
                r = NULL_ID;
            }
            else {  // Hour
                r += n * 1000LL;
                n = 0;
            }
            break;
        default:
            if (c.isNumber()) {
                n = c.toLatin1() - '0' + (n * 10);
            }
            else {
                ok = false;
                r = NULL_ID;
            }
        }
    }
    if (alternate) {
        QRegExp pattern("(\\d*)-(\\d*)-(\\d*)T(\\d*):(\\d*):(\\d*)");
        ok = pattern.exactMatch(s);
        if (ok) {
            r  = pattern.cap(1).toInt() * (1000LL * 3600 * 24 * 365);  // ?
            r += pattern.cap(2).toInt() * (1000LL * 3600 * 24 * 30);
            r += pattern.cap(3).toInt() * (1000LL * 3600 * 24);
            r += pattern.cap(4).toInt() * (1000LL * 3600);
            r += pattern.cap(5).toInt() * (1000LL *   60);
            r += pattern.cap(6).toInt() *  1000LL;
        }
    }
    if (pOk != nullptr) *pOk = ok;
    return ok ? r : NULL_ID;
}

static qlonglong dim2msec(const QString& dim)
{
    if (dim.isEmpty() || 0 == dim.compare("day", Qt::CaseInsensitive) || 0 == dim.compare("days", Qt::CaseInsensitive)) {
        return 1000LL * 3600 * 24;
    }
    if (0 == dim.compare("hour",   Qt::CaseInsensitive) || 0 == dim.compare("hours",   Qt::CaseInsensitive)) {
        return 1000LL * 3600;
    }
    if (0 == dim.compare("minute", Qt::CaseInsensitive) || 0 == dim.compare("minutes", Qt::CaseInsensitive)) {
        return 1000LL * 60;
    }
    if (0 == dim.compare("second", Qt::CaseInsensitive) || 0 == dim.compare("seconds", Qt::CaseInsensitive)) {
        return 1000LL;
    }
    if (0 == dim.compare("mount",  Qt::CaseInsensitive) || 0 == dim.compare("mounts",  Qt::CaseInsensitive)) {
        return 1000LL * 3600 * 24 * 30;
    }
    if (0 == dim.compare("year",   Qt::CaseInsensitive) || 0 == dim.compare("years",   Qt::CaseInsensitive)) {
        return 1000LL * 3600 * 24 * 365;
    }
    return -1;
}

qlonglong parseTime(const QString& _s, bool *pOk)
{
    bool ok;
    qlonglong r = NULL_ID;
    QRegExp pat("(\\d+):(\\d+):(\\d+\\.?\\d*)");    // hh:mm:ss.ss
    ok = pat.exactMatch(_s);
    if (ok) {
        r  = pat.cap(1).toInt() * 1000LL * 3600;                // Hour(s)
        r += pat.cap(2).toInt() * 1000LL *   60;                // minute(s)
        r += qlonglong(pat.cap(3).toDouble() * 1000LL + 0.5);   // second(s)
    }
    if (pOk != nullptr) *pOk = ok;
    return r;
}

qlonglong _parseTimeIntervalSQL(const QString& _s, bool& ago, bool *pOk)
{
    qlonglong r = 0;
    bool ok = true;
    QStringList sl = _s.split(QRegExp("\\s+"));
    if (0 == sl.last().compare("ago", Qt::CaseInsensitive)) {
        sl.pop_back();
        if (ago == true || sl.isEmpty()) {
            ok = false;
        }
        else {
            ago = true;
        }
    }
    while (ok && !sl.isEmpty()) {
        QString s = sl.takeFirst();
        double n = s.toDouble(&ok);    // number ?
        if (ok) {
            s = sl.isEmpty() ? _sNul : sl.takeFirst();  // dim.
            qlonglong m = dim2msec(s);
            if (m > 0) {
                r += qlonglong((n * m) + 0.5);
            }
            else {
                if (!sl.isEmpty()) { // Last field ?
                    ok = false;
                    break;
                }
                r  = qlonglong((n * 1000LL * 3600 * 24) + 0.5);  // Day(s)   (no dim)
                r += parseTime(s, &ok);
                break;
            }
        }
        else {
            if (sl.isEmpty()) r = parseTime(s, &ok);
            else              ok = false;
            break;
        }
    }
    if (pOk != nullptr) *pOk = ok;
    return ok ? r : NULL_ID;
}

/// Az intervallum qlonglong-ban tárolódik, és mSec-ben értendő.
/// Konvertál egy intervallum stringet mSec-re.
/// @param s A konvertálandó string
/// @param pOk egy bool pointer, ha nem NULL, és az s sem üres, akkor ha sikerült a konverzió a mutatott változót true-ba írja.
///            ha pOk nem NULL. és az s üres, vagy a konverzió nem sikerült, akkor mutatott változót false-ba írja.
/// @return A konvertált érték, vagy NULL_ID ha üres stringet adtunk meg, vagy nem sikerült a kovverzió.
qlonglong parseTimeInterval(const QString& _s, bool *pOk)
{
    if (_s.isEmpty()) {
        if (pOk != nullptr) *pOk = false;
        return NULL_ID;
    }
    QString s = _s;
    qlonglong   r = 0;
    bool negativ = false;
    if (s.startsWith(QChar('+'))) s = s.mid(1);
    else if (s.startsWith(QChar('-'))) {
        s = s.mid(1);
        negativ = true;
    }
    bool ok = true;
    if (s.startsWith(QChar('P'))) { // ISO 8601
        r = _parseTimeIntervalISO8601(s, &ok);
    }
    else {
        r = _parseTimeIntervalSQL(s, negativ, &ok);
    }
    if (pOk != nullptr) *pOk = ok;
    if (!ok) return NULL_ID;
    return negativ ? -r : r;
}

QSqlDatabase *  getSqlDb(void)
{
    if (lanView::instance == nullptr) EXCEPTION(EPROGFAIL, -1, "lanView::instance is NULL.");
    QSqlDatabase *pSqlDb;
    pSqlDb = lanView::instance->pDb;
    if (pSqlDb == nullptr) EXCEPTION(EPROGFAIL, -1, "lanView::instance->pDb is NULL.");
    if (!pSqlDb->isOpen()) EXCEPTION(EPROGFAIL, -1, "cLanView::instance->pDb is set, but database is not opened.");
    if (!isMainThread()) {  // Minden thread-nak saját objektum kell!
        // PDEB(INFO) << "Lock by threadMutex, in getSqlDb() ..." << endl;
        bool newDb;
        lanView::getInstance()->threadMutex.lock();
        QSqlDatabase *pDb;
        QString tn = currentThreadName();
        if (tn.isEmpty()) EXCEPTION(EPROGFAIL, 0, QObject::trUtf8("Thread name is empty."));
        QMap<QString, QSqlDatabase *>&  map = lanView::getInstance()->dbThreadMap;
        QMap<QString, QSqlDatabase *>::iterator i = map.find(tn);
        newDb = i == map.end();
        if (newDb) {
            pDb = new QSqlDatabase(QSqlDatabase::cloneDatabase(*pSqlDb, tn));
            if (!pDb->open()) SQLOERR(*pDb);
            map.insert(tn, pDb);
            QMap<QString, QStringList>& tm = lanView::getInstance()->trasactionsThreadMap;
            tm.insert(tn, QStringList());
        }
        else {
            pDb = i.value();
        }
        lanView::getInstance()->threadMutex.unlock();
        pSqlDb = pDb;
        if (pSqlDb == nullptr) EXCEPTION(EPROGFAIL, -1, QString("Thread %1 pdb is NULL.").arg(tn));
        if (!pSqlDb->isOpen()) EXCEPTION(EPROGFAIL, -1, QString("Thread %1 pdb is set, but database is not opened.").arg(tn));
        if (newDb) {
            QSqlQuery q(*pSqlDb);
            dbOpenPost(q, tn);
        }
    }
    return pSqlDb;
}

void dropThreadDb(const QString& tn, enum eEx __ex)
{

    lanView::getInstance()->threadMutex.lock();
    {
        QMap<QString, QSqlDatabase *>&  map = lanView::getInstance()->dbThreadMap;
        QMap<QString, QSqlDatabase *>::iterator i = map.find(tn);
        if (i == map.end()) {
            lanView::getInstance()->threadMutex.unlock();
            if (__ex) EXCEPTION(EPROGFAIL);
            return;
        }
        delete i.value();
        map.erase(i);
    }
    {
        QMap<QString, QStringList>&  map = lanView::getInstance()->trasactionsThreadMap;
        QMap<QString, QStringList>::iterator i = map.find(tn);
        if (i == map.end()) {
            lanView::getInstance()->threadMutex.unlock();
            if (__ex) EXCEPTION(EPROGFAIL);
            return;
        }
        map.erase(i);
    }
    lanView::getInstance()->threadMutex.unlock();
}

/* --------------------------------------------------------------------------------------------- */

eTristate trFlag(eTristate __tf)
{
    switch (__tf) {
    case TS_TRUE:
    case TS_FALSE:  return __tf;
    case TS_NULL:   break;
    }
    QStringList *pTrl = lanView::getInstance()->getTransactioMapAndCondLock();  // If trhread, then mutex is locked !!
    eTristate tf = pTrl->isEmpty() ? TS_TRUE : TS_FALSE;
    if (!isMainThread()) {
        lanView::getInstance()->threadMutex.unlock();
    }
    return tf;
}

void sqlBegin(QSqlQuery& q, const QString& tn)
{
    QStringList *pTrl = lanView::getInstance()->getTransactioMapAndCondLock();  // If trhread, then mutex is locked !!
    bool r;
    QString sql;
    if (pTrl->isEmpty()) {
        sql = _sBEGIN;
        PDEB(SQL) << sql << VDEBSTR(tn) << endl;
    }
    else {
        sql = "SAVEPOINT " + tn;
        PDEB(SQL) << sql << " (" << pTrl->join(",") << ")" << VDEBSTR(tn) << endl;
    }
    r = q.exec(sql);
    if (r) pTrl->push_back(tn);
    if (!isMainThread()) {
        lanView::getInstance()->threadMutex.unlock();
    }
    if (!r) SQLQUERYERR(q);
}

void sqlCommit(QSqlQuery& q, const QString& tn)
{
    QString msg;
    QStringList *pTrl = lanView::getInstance()->getTransactioMapAndCondLock();
    cError *pe = nullptr;
    bool r = false;
    if (pTrl->isEmpty()) {
        msg = QObject::trUtf8("End transaction, invalid name : %1; no pending transaction").arg(tn);
        pe = NEWCERROR(EPROGFAIL, 0, msg);
    }
    else if (pTrl->last() != tn) {
        QString msg = QObject::trUtf8("End transaction, invalid name : %1; pending transactions : %2").arg(tn, pTrl->join(", "));
        pe = NEWCERROR(EPROGFAIL, 0, msg);
    }
    else {
        QString sql;
        if (pTrl->size() == 1) {
            sql = "COMMIT";
            PDEB(SQL) << sql << VDEBSTR(tn) << endl;
        }
        else {
            sql = "RELEASE SAVEPOINT " + tn;
            PDEB(SQL) << sql << " (" << pTrl->join(_sCommaSp) << ")" << endl;
        }
        r = q.exec(sql);
        if (r) pTrl->pop_back();
    }
    if (!isMainThread()) {
        lanView::getInstance()->threadMutex.unlock();
    }
    if (pe != nullptr) pe->exception();
    if (!r) SQLQUERYERR(q);
}

cError *sqlRollback(QSqlQuery& q, const QString& tn, eEx __ex)
{
    QString msg;
    QStringList *pTrl = lanView::getInstance()->getTransactioMapAndCondLock();
    cError *pe = nullptr;
    bool r = false;
    int i = pTrl->indexOf(tn);
    if (pTrl->isEmpty()) {
        msg = QObject::trUtf8("Rollback transaction, invalid name : %1; no pending transaction").arg(tn);
        pe = NEWCERROR(EPROGFAIL, 0, msg);
    }
    else if (i < 0) {
        QString msg = QObject::trUtf8("Rollback transaction, invalid name : %1; pending transactions : %2").arg(tn, pTrl->join(", "));
        pe = NEWCERROR(EPROGFAIL, 0, msg);
    }
    else {
        QString sql;
        if (i == 0) {
            sql = _sROLLBACK;
            PDEB(SQL) << sql << VDEBSTR(tn) << endl;
        }
        else {
            sql = _sROLLBACK + " TO SAVEPOINT " + tn;
            PDEB(SQL) << sql << " (" << pTrl->join(_sCommaSp) << ")" << endl;
        }
        r = q.exec(sql);
        if (r) {
            *pTrl = pTrl->mid(0, i);
        }
    }
    if (!isMainThread()) {
        lanView::getInstance()->threadMutex.unlock();
    }
    if (pe != nullptr) {
        if (__ex == EX_IGNORE) {
            delete pe;
            return nullptr;

        }
        if (__ex == EX_ERROR) {
            pe->exception();
        }
        return pe;
    }
    if (r) return nullptr;
    if (__ex == EX_IGNORE) {
        return nullptr;
    }
    if (__ex == EX_ERROR) {
        SQLQUERYERR(q);
    }
    SQLQUERYNEWERR(pe, q);
    return pe;
}

QString toSqlName(const QString& _n)
{
    int i, n = _n.size();
    QString r;
    for (i = 0; i < n; ++i) {
        QChar c = _n[i];
        if (c.unicode() < 128 && c.isLetterOrNumber()) r += c;
        else                                           r += '_';
    }
    if (r[0].isNumber()) r.prepend('_');
    return r;
}

/* ********************************************************************************************* */

// Unused
bool executeSqlScript(QFile& file, QSqlDatabase *pDb, enum eEx __ex)
{
    cError *   pe = nullptr;
    QSqlQuery *pq = nullptr;
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

int _execSql(QSqlQuery& q, const QString& sql, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    if (!v1.isValid()) {
        PDEB(SQL) << dQuoted(sql) << endl; \
        if (!q.exec(sql)) return -2;
    }
    else {
        if (!q.prepare(sql)) return -1;
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
        PDEB(SQL) << dQuoted(q.lastQuery()) << endl; \
        if (!q.exec()) return -1;
    }
    return q.first() ? 1 : 0;
}

bool execSql(QSqlQuery& q, const QString& sql, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    if (!v1.isValid()) {
        EXECSQL(q, sql);
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
        _EXECSQL(q);
    }
    return q.first();
}

bool execSql(QSqlQuery& q, const QString& sql, const QVariantList& vl)
{
    if (vl.isEmpty()) {
        EXECSQL(q, sql);
    }
    else {
        if (!q.prepare(sql)) SQLPREPERR(q, sql);
        int i = 0;
        foreach (QVariant v, vl) {
            q.bindValue(i, v);
            ++i;
        }
        _EXECSQL(q);
    }
    return q.first();
}

bool execSqlFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
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

qlonglong execSqlIntFunction(QSqlQuery& q, bool *pOk, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    bool ok = execSqlFunction(q, fn, v1, v2, v3, v4, v5);
    qlonglong r = NULL_ID;
    if (ok) {
        QVariant v = q.value(0);
        r = v.toLongLong(&ok);
        if (!ok) r = NULL_ID;
    }
    if (pOk != nullptr) *pOk = ok;
    return r;
}

bool execSqlBoolFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    bool ok = execSqlFunction(q, fn, v1, v2, v3, v4, v5);
    QString r;
    if (ok) r = q.value(0).toString();
    return str2bool(r);
}

QString execSqlTextFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    bool ok = execSqlFunction(q, fn, v1, v2, v3, v4, v5);;
    QString r;
    if (ok) r = q.value(0).toString();
    return r;
}

bool execSqlRecFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5)
{
    static const QString s = "* FROM ";
    return execSqlFunction(q, s + fn, v1, v2, v3, v4, v5);
}

bool sqlNotify(QSqlQuery& q, const QString& channel, const QString& payload)
{
    QString sql = "NOTIFY " + channel;
    if (!payload.isEmpty()) sql += ", " + quoted(payload);
    return q.exec(sql);
}

int getListFromQuery(QSqlQuery q, QStringList& list, int __ix)
{
    int n = 0;
    if (q.first()) do {
        ++n;
        list << q.value(__ix).toString();
    } while (q.next());
    return n;
}

