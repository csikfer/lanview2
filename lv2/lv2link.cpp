#include "lanview.h"
#include "lv2link.h"

int phsLinkType(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sFront, Qt::CaseInsensitive)) return LT_FRONT;
    if (0 == n.compare(_sBack,  Qt::CaseInsensitive)) return LT_BACK;
    if (0 == n.compare(_sTerm,  Qt::CaseInsensitive)) return LT_TERM;
    if (__ex) EXCEPTION(EDATA, 0, n);
    return LT_INVALID;
}

const QString& phsLinkType(int e, eEx __ex)
{
    switch (e) {
    case LT_FRONT:  return _sFront;
    case LT_BACK:   return _sBack;
    case LT_TERM:   return _sTerm;
    default:        break;
    }
    if (__ex) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CRECCNTR(cPhsLink)
const cRecStaticDescr&  cPhsLink::descr() const
{
    if (initPDescr<cPhsLink>(_sPhsLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cPhsLink().tableType() != TT_LINK_TABLE || _descr_cPhsLink().tableName() != _sPhsLinksTable) EXCEPTION(EPROGFAIL);
        CHKENUM(_sPhsLinkType1, phsLinkType);
        CHKENUM(_sPhsLinkType2, phsLinkType);
        // CHKENUM(_sLinkType, linkType)    // There are no constants defined
    }
    return *_pRecordDescr;
}
CRECDEFD(cPhsLink)

int cPhsLink::replace(QSqlQuery &__q, eEx __ex)
{
    cError *pe = NULL;
    bool tr = false;
    int r = 0;
    eReasons reason = R_INVALID;
    try {
        sqlBegin(__q);
        tr = true;
        PDEB(VVERBOSE) << toString() << endl;
        // Ütköző linkek törlése a bal oldali porthoz (1)
        r += unxlinks(__q, getId(_sPortId1), (ePhsLinkType)getId(_sPhsLinkType1), (ePortShare)getId(__sPortShared));
        // Ütköző linkek törlése a jobb oldali porthoz (2)
        r += unxlinks(__q, getId(_sPortId2), (ePhsLinkType)getId(_sPhsLinkType2), (ePortShare)getId(__sPortShared));
        if (!insert(__q, __ex)) {
            sqlRollback(__q);
            reason = R_ERROR;
        }
        else {
            sqlEnd(__q);
            reason = r ? R_UPDATE : R_INSERT;
        }
        tr = false;
    }
    CATCHS(pe)
    if (pe == NULL) return reason;
    if (tr) sqlRollback(__q);
    if (__ex) pe->exception();
    lanView::sendError(pe);
    return R_ERROR;
}

bool cPhsLink::rewrite(QSqlQuery &, eEx __ex)
{
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP);
    return false;
}

int cPhsLink::unlink(QSqlQuery &q, const QString& __nn, const QString& __pn, ePhsLinkType __t, bool __pat)
{
    QString sql =
            "SELECT phs_link_id FROM phs_links AS l"
            " JOIN nports AS p ON p.port_id = l.port_id1"
            " JOIN patchs AS n ON n.node_id = p.node_id"
            " WHERE n.node_name = ? AND p.port_name " + QString(__pat ? "LIKE" : "=") + " ?";
    if (__t != LT_INVALID) sql += " AND " + _sPhsLinkType1 + " = ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    q.bindValue(1, __pn);
    if (__t != LT_INVALID) q.bindValue(2, phsLinkType(__t));
    if (!q.exec()) SQLQUERYERR(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << variantToId(q.value(0));
        } while (q.next());
        foreach (qlonglong id, idl) {
            setId(id);
            PDEB(VVERBOSE) << " Delete " << tableName() << " rekord, " << VDEBBOOL(id) << endl;
            if (!remove(q, false, primaryKey())) DERR() << "Nem sikerült törölni a host_services rekordot, ID = " << id << endl;
        }
    }
    else {
        DWAR() << "Egyetlen rekord sem kerül törlésre." << endl;
    }
    return idl.size();
}

int cPhsLink::unlink(QSqlQuery &q, const QString& __nn, ePhsLinkType __t, int __ix, int __ei)
{
    QString sql =
            "SELECT phs_link_id FROM phs_links AS l"
            " JOIN nports AS p ON p.port_id = l.port_id1"
            " JOIN patchs AS n ON n.node_id = p.node_id"
            " WHERE n.node_name = ?";
    if (__t != LT_INVALID) sql += " AND phs_link_type1 = ?";
    if (__ei == NULL_IX || __ei == __ix) {
        sql += " AND p.port_index = " + QString::number(__ix);
    }
    else {
        // if (__ix > __ie) EXCEPTION(EDATA);
        sql += " AND p.port_index >= " + QString::number(__ix);
        sql += " AND p.port_index <= " + QString::number(__ei);
    }

    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    if (__t != LT_INVALID) q.bindValue(1, phsLinkType(__t));
    if (!q.exec()) SQLQUERYERR(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << variantToId(q.value(0));
        } while (q.next());
        foreach (qlonglong id, idl) {
            setId(id);
            PDEB(VVERBOSE) << " Delete " << tableName() << " rekord, " << VDEBBOOL(id) << endl;
            if (!remove(q, false, primaryKey())) DERR() << "Nem sikerült törölni a host_services rekordot, ID = " << id << endl;
        }
    }
    else {
        DWAR() << "Egyetlen rekord sem kerül törlésre." << endl;
    }
    return idl.size();
}

int cPhsLink::unlink(QSqlQuery &q, qlonglong __pid, ePhsLinkType __t, ePortShare __s) const
{
    QString pid = QString::number(__pid);
    QString w1 = "port_id1 = " + pid;
    QString w2 = "port_id2 = " + pid;
    if (__t != LT_INVALID) {
        QString t = quoted(phsLinkType(__t));
        w1 = "(" + w1 + " AND phs_link_type1 = " + t + ")";
        w2 = "(" + w2 + " AND phs_link_type2 = " + t + ")";
    }
    QString where = w1 + " OR " + w2;
    if (__s != ES_NC) {
        where = "port_shared = '" + portShare((int)__s) + "' AND (" + where + ")";
    }
    QString sql = "DELETE FROM " + descr().fullTableNameQ() + " WHERE " + where;
    execSql(q, sql);
    int r = q.numRowsAffected();
    PDEB(VVERBOSE) << "Deleted " << r << " recoed; SQL : " << quotedString(sql) << endl;
    return r;
}

bool cPhsLink::isExist(QSqlQuery &q, qlonglong __pid, ePhsLinkType __t, ePortShare __s)
{
    clear();
    if (__pid == NULL_ID) return false;
    QString sql = "SELECT * FROM phs_links"
                  " WHERE port_id1 = ? AND phs_link_type1 = ?";
    QVariant s;
    if (__s != ES_NC) {
        sql += "AND port_shared = ?";
        s = portShare(__s);
    }
    if (!execSql(q, sql, __pid, phsLinkType(__t), s)) {
        return false;
    }
    set(q);
    return true;
}

bool cPhsLink::addIfCollision(QSqlQuery &q, tRecordList<cPhsLink>& list, qlonglong __pid, ePhsLinkType __t, ePortShare __s)
{
    if (!isExist(q, __pid, __t, __s)) return false;
    list << *this;
    return true;
}

int cPhsLink::collisions(QSqlQuery& __q, tRecordList<cPhsLink>& list, qlonglong __pid, ePhsLinkType __t, ePortShare __s)
{
    switch (__t) {
    case LT_BACK:   // A megosztások nem itt adahatóak meg
    case LT_TERM:   // Nincs/nem lehet megosztás erre a portra
                    addIfCollision(__q, list, __pid, __t);
                    break;
    case LT_FRONT:
        switch (__s) {  // Minden ütközö link törlendő
        case ES_:   addIfCollision(__q, list, __pid, LT_FRONT, ES_A);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AA);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AB);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_B);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BA);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BB);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_C);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_D);
            break;
        case ES_A:  addIfCollision(__q, list, __pid, LT_FRONT, ES_A);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AA);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AB);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_C);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_D);
                    break;
        case ES_AA: addIfCollision(__q, list, __pid, LT_FRONT, ES_A);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AA);
                    break;
        case ES_AB: addIfCollision(__q, list, __pid, LT_FRONT, ES_A);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AB);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_C);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_D);
                    break;
        case ES_B:  addIfCollision(__q, list, __pid, LT_FRONT, ES_B);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BA);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BB);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_C);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_D);
                    break;
        case ES_BA: addIfCollision(__q, list, __pid, LT_FRONT, ES_B);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BA);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_C);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_D);
                    break;
        case ES_BB: addIfCollision(__q, list, __pid, LT_FRONT, ES_B);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BB);
                    ;
        case ES_C:  addIfCollision(__q, list, __pid, LT_FRONT, ES_C);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_A);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_B);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AB);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BA);
                    break;
        case ES_D:  addIfCollision(__q, list, __pid, LT_FRONT, ES_D);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_A);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_B);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_AB);
                    addIfCollision(__q, list, __pid, LT_FRONT, ES_BA);
                    break;
        default:    EXCEPTION(EDATA);
        }
        addIfCollision(__q, list, __pid, LT_FRONT, ES_);
        break;
    default:    EXCEPTION(EDATA);
    }
    return list.count();
}

int cPhsLink::unxlinks(QSqlQuery& __q, qlonglong __pid, ePhsLinkType __t, ePortShare __s) const
{
    int r = 0;
    switch (__t) {
    case LT_BACK:   // A megosztások nem itt adahatóak meg
    case LT_TERM:   // Nincs/nem lehet megosztás erre a portra
                    return unlink(__q, __pid, __t);
    case LT_FRONT:
        if (__s == ES_) return unlink(__q, __pid, LT_FRONT);;
        switch (__s) {  // Minden ütközö link törlendő
        case ES_A:  r  = unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_AA);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_AA: r  = unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_AA);
                    break;
        case ES_AB: r  = unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_B:  r  = unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    r += unlink(__q, __pid, LT_FRONT, ES_BB);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_BA: r  = unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_BB: r  = unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_BB);
                    ;
        case ES_C:  r  = unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    break;
        case ES_D:  r  = unlink(__q, __pid, LT_FRONT, ES_D);
                    r += unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    break;
        default:    EXCEPTION(EDATA);
        }
        return r + unlink(__q, __pid, LT_FRONT, ES_);
    default:    EXCEPTION(EDATA);
    }
    return -1;
}

/* ----------------------------------------------------------------- */
qlonglong LinkGetLinked(QSqlQuery& q, cRecord& o, qlonglong __pid)
{
    o.clear();
    o.setId(_sPortId1, __pid);
    int n = o.completion(q);
    switch (n) {
    case 0: break;
    case 1: return o.getId(_sPortId2);
    default:EXCEPTION(EDATA, n, o.descr().toString());
    }
    return NULL_ID;
}
bool LinkIsLinked(QSqlQuery& q, cRecord& o, qlonglong __pid1, qlonglong __pid2)
{
    o.clear();
    o.setId(_sPortId1, __pid1);
    o.setId(_sPortId2, __pid2);
    int n = o.completion(q);
    switch (n) {
    case 0: break;
    case 1: return true;
    default:EXCEPTION(EDATA, n, o.descr().toString());
    }
    return false;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CRECCNTR(cLogLink)
const cRecStaticDescr&  cLogLink::descr() const
{
    if (initPDescr<cLogLink>(_sLogLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cLogLink().tableType() != TT_LINK_TABLE || _descr_cLogLink().tableName() != _sLogLinksTable) EXCEPTION(EPROGFAIL);
    }
    return *_pRecordDescr;
}

bool cLogLink::insert(QSqlQuery &, bool)
{
    EXCEPTION(ENOTSUPP);
    return false;
}

int cLogLink::replace(QSqlQuery &, bool)
{
    EXCEPTION(ENOTSUPP);
    return false;
}

bool cLogLink::update(QSqlQuery &, bool, const QBitArray &, const QBitArray &, bool)
{
    EXCEPTION(ENOTSUPP);
    return false;
}

CRECDEFD(cLogLink)

/* ----------------------------------------------------------------- */

CRECCNTR(cLldpLink)
const cRecStaticDescr&  cLldpLink::descr() const
{
    if (initPDescr<cLldpLink>(_sLldpLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cLldpLink().tableType() != TT_LINK_TABLE || _descr_cLldpLink().tableName() != _sLldpLinksTable) EXCEPTION(EPROGFAIL);
    }
    return *_pRecordDescr;
}
CRECDEFD(cLldpLink)

bool cLldpLink::unlink(QSqlQuery& q, qlonglong __pid)
{
    QString pid = QString::number(__pid);
    QString sql = "DELETE FROM " + descr().fullTableNameQ() + " WHERE port_id1 = " + pid + " OR port_id2 = " + pid;
    execSql(q, sql);
    return q.numRowsAffected() != 0;
}
