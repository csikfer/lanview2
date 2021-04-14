#include "lanview.h"
#include "lv2link.h"
#include "lv2user.h"

int phsLinkType(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sFront, Qt::CaseInsensitive)) return LT_FRONT;
    if (0 == n.compare(_sBack,  Qt::CaseInsensitive)) return LT_BACK;
    if (0 == n.compare(_sTerm,  Qt::CaseInsensitive)) return LT_TERM;
    if (__ex) EXCEPTION(EENUMVAL, 0, n);
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

bool cPhsLink::insert(QSqlQuery &, eEx __ex)
{
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP);
    return false;
}

int cPhsLink::replace(QSqlQuery &__q, eEx __ex)
{
    cError *pe = nullptr;
    bool tr = false;
    int r = 0;
    eReasons reason = REASON_INVALID;
    static const QString tn = "ReplacePhsLink";
    try {
        sqlBegin(__q, tn);
        tr = true;
        PDEB(VVERBOSE) << toString() << endl;
        // Ütköző linkek törlése a bal oldali porthoz (1)
        r += unxlinks(__q, getId(_sPortId1), ePhsLinkType(getId(_sPhsLinkType1)), ePortShare(getId(__sPortShared)));
        // Ütköző linkek törlése a jobb oldali porthoz (2)
        r += unxlinks(__q, getId(_sPortId2), ePhsLinkType(getId(_sPhsLinkType2)), ePortShare(getId(__sPortShared)));
        // User
        set(_sCreateUserId, lanView::getInstance()->getUserId(EX_IGNORE));
        if (!cRecord::insert(__q, __ex)) {
            sqlRollback(__q, tn);
            reason = R_ERROR;
        }
        else {
            sqlCommit(__q, tn);
            reason = r ? R_UPDATE : R_INSERT;
        }
        tr = false;
    }
    CATCHS(pe)
    if (pe == nullptr) return reason;
    if (tr) sqlRollback(__q, tn);
    if (__ex) pe->exception();
    sendError(pe);
    delete pe;
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
        where = "port_shared = '" + portShare(int(__s)) + "' AND (" + where + ")";
    }
    QString sql = "DELETE FROM " + descr().fullTableNameQ() + " WHERE " + where;
    execSql(q, sql);
    int r = q.numRowsAffected();
    PDEB(VVERBOSE) << "Deleted " << r << " recoed; SQL : " << quotedString(sql) << endl;
    return r;
}

int cPhsLink::isExist(QSqlQuery &q, qlonglong __pid, ePhsLinkType __t, ePortShare __s)
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
        return 0;
    }
    set(q);
    return q.size();
}

bool cPhsLink::addIfCollision(QSqlQuery &q, tRecordList<cPhsLink>& list, qlonglong __pid, ePhsLinkType __t, ePortShare __s)
{
    if (0 == isExist(q, __pid, __t, __s)) return false;
    list << *this;
    return true;
}

/// A porthoz magadandó fizikai link mely más fizikai linkekkel ütközik ? Az ütköző link rekordokat beolvassa.
/// Az objektumot mint átmeneti tárolót használja az ütköző rekordok beolvasásához, vagy a tartalmát törli.
/// @param __q
/// @param list Az ütköző link rekordok konténere (kimenet). Nem törli a konténert, hanem hozzáadja a találatokat.
/// @param __pid A port ID
/// @param __t a link típusa
/// @param __s megosztás típusa. Ha nem adható meg megosztás, akkor figyelmen kívül hagyja.
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
                    break;
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
                    break;
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

cPhsLink& cPhsLink::swap()
{
    int ix1, ix2;
    ix1 = toIndex(_sPortId1);
    ix2 = toIndex(_sPortId2);
    _fields[ix1].swap(_fields[ix2]);
    ix1 = toIndex(_sPhsLinkType1);
    ix2 = toIndex(_sPhsLinkType2);
    _fields[ix1].swap(_fields[ix2]);
    ix1 = toIndex(_sForward);
    setBool(ix1, !getBool(ix1));
    return *this;
}

QString cPhsLink::show12(QSqlQuery& q, bool _12) const
{
    QString sp;
    cNPort *p = cNPort::getPortObjById(q, getId(_12 ? _sPortId1 : _sPortId2));
    int ix = p->toIndex(_sSharedCable, EX_IGNORE);  // PPort ?
    if (ix == NULL_IX) {    // NPort or Interface
        sp = p->getFullName(q);
    }
    else {                  // Patch port
        sp  = getName(_12 ? _sPhsLinkType1 : _sPhsLinkType2) + " ";
        sp += p->getFullName(q);
        if (getId(_12 ? _sPhsLinkType1 : _sPhsLinkType2) == LT_FRONT) {
            QString sh = getName(_sPortShared);
            if (!sh.isEmpty()) sp += "/" + sh;
        }
        else {              // LT_BACK
            QString sh = p->getName(_sSharedCable);
            if (!sh.isEmpty()) {
                qlonglong sid = p->getId(_sSharedPortId);
                if (sid != NULL_ID) {
                    sh.prepend(cNPort::getFullNameById(q, sid) + "/");
                }
                else {
                    sh.prepend("?:?/");
                }
                sp.prepend(parentheses(sh));
            }
        }
    }
    return sp;
}

QString cPhsLink::show(bool t) const
{
    QSqlQuery q = getQuery();
    QString r;
    if (t) r = tr("Fizikai link : ");
    r += show12(q, true) + " <==> " + show12(q, false);
    QString note = getNote();
    QString   tm  = getName(_sCreateTime);
    qlonglong uid = getId(_sCreateUserId);
    if (uid == NULL_ID) r += " ?(";
    else                r += " " + cUser().getNameById(q, uid);
    r += tm + ")";
    if (!note.isEmpty()) r += " " + parentheses(note);
    return r;
}

bool cPhsLink::nextLink(QSqlQuery& q, qlonglong pid, enum ePhsLinkType type, enum ePortShare sh)
{
    static const QString sql = "SELECT * FROM next_patch(?, ?, ?)";
    execSql(q, sql, pid, phsLinkType(type), portShare(sh));
    set(q);
    return getId() != NULL_ID;
}

bool cPhsLink::compare(const cPhsLink& _o, bool _swap) const
{
    if (getName(_sPortShared) != _o.getName(_sPortShared))    return false;
    while (true) {
        if (getId(_sPortId1)      != _o.getId(_sPortId1))       break;
        if (getId(_sPortId2)      != _o.getId(_sPortId2))       break;
        if (getId(_sPhsLinkType1) != _o.getId(_sPhsLinkType1))  break;
        if (getId(_sPhsLinkType2) != _o.getId(_sPhsLinkType2))  break;
        return true;        // mind egyezett
        if (_swap) break;   // megprobáljuk fordítva ?
        return false;       // kész, nem egyezett
    }
    if (getId(_sPortId1)      != _o.getId(_sPortId2))       return false;
    if (getId(_sPortId2)      != _o.getId(_sPortId1))       return false;
    if (getId(_sPhsLinkType1) != _o.getId(_sPhsLinkType2))  return false;
    if (getId(_sPhsLinkType2) != _o.getId(_sPhsLinkType1))  return false;
    return true;
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

bool cLogLink::insert(QSqlQuery &, eEx __ex)
{
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP);
    return false;
}

int cLogLink::replace(QSqlQuery &, eEx __ex)
{
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP);
    return false;
}

int cLogLink::update(QSqlQuery &, bool, const QBitArray &, const QBitArray &, enum eEx)
{
    EXCEPTION(ENOTSUPP);
    return 0;
}

CRECDEFD(cLogLink)

QString cLogLink::show(bool t) const
{
    QSqlQuery q = getQuery();
    QString r;
    if (!t) r = tr("Logikai link. ");
    r += cNPort::getFullNameById(q, getId(_sPortId1)) + " <==> " + cNPort::getFullNameById(q, getId(_sPortId2));
    QString note = getNote();
    if (!note.isEmpty()) r += " " + parentheses(note);
    return r;
}

QStringList cLogLink::showChain() const
{
    QVariantList vids = get(__sPhsLinkChain).toList();
    if (vids.isEmpty()) return QStringList(tr("Üres lánc"));
    static const QString eChain = tr("A lánc hibás! Hibás port azonosító.");
    static const QString eNotFd = tr("A lánc hibás! A hivatkozott fizikai link (#%) nem létezik.");
    qlonglong apid = getId(_sPortId1);  // Megyünk portról - portra, ez az első
    qlonglong rpid = getId(_sPortId2);  // Ha fordított az irány, akkor ez az első
    bool first   = true;
    bool reverse = false;
    QStringList lines;
    QSqlQuery q = getQuery();
    foreach (QVariant sid, vids) {
        cPhsLink pl;
        bool ok;
        qlonglong id = sid.toLongLong(&ok);
        if (!ok) EXCEPTION(EDATA, -1, sid.toString());
        if (!pl.fetchById(q, id)) {
            lines << eNotFd.arg(id);
            continue;
        }
        else if (first) {
            first = false;
            if      (pl.getId(_sPortId1) == apid) { apid = pl.getId(_sPortId2); }
            else if (pl.getId(_sPortId2) == apid) { apid = pl.swap().getId(_sPortId2);  }
            else if (pl.getId(_sPortId1) == rpid) { apid = pl.getId(_sPortId2);         reverse = true; }
            else if (pl.getId(_sPortId2) == rpid) { apid = pl.swap().getId(_sPortId2);  reverse = true; }
            else                                  { lines << eChain; }
        }
        else {
            if      (pl.getId(_sPortId1) == apid) { apid = pl.getId(_sPortId2); }
            else if (pl.getId(_sPortId2) == apid) { apid = pl.swap().getId(_sPortId2);  }
            else                                  { lines << eChain; }
        }
        lines << pl.show();
    }
    if (reverse) {
        QStringList tmp;
        while (!lines.isEmpty()) tmp << lines.takeLast();
        return tmp;
    }
    return lines;
}


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

QString cLldpLink::show(bool t) const
{
    QSqlQuery q = getQuery();
    QString r;
    if (!t) r = tr("LLDP link. ");
    r += tr("(%1 - %2) ").arg(getName(_sFirstTime), getName(_sLastTime));
    r += tr("%1 <==> %2").arg(cNPort::getFullNameById(q, getId(_sPortId1)), cNPort::getFullNameById(q, getId(_sPortId2)));
    return r;
}

/* ----------------------------------------------------------------- */

eLinkResult getLinkedPort(QSqlQuery& q, qlonglong pid, qlonglong& lpid, cLldpLink *_pLldp, cLogLink *_pLogl)
{
    cLldpLink *pLldp = _pLldp;
    cLogLink  *pLogl = _pLogl;
    if (pLldp == nullptr) pLldp = new cLldpLink;
    if (pLogl == nullptr) pLogl = new cLogLink;
    qlonglong lpid1, lpid2;
    eLinkResult r;

    lpid1 = pLldp->getLinked(q, pid);
    lpid2 = pLogl->getLinked(q, pid);
    if (lpid1 == NULL_ID && lpid2 == NULL_ID) {
        lpid = NULL_ID;
        r = LINK_NOT_FOUND;
    }
    else if (lpid1 == NULL_ID && lpid2 != NULL_ID) {
        lpid1 = pLldp->getLinked(q, lpid2);
        if (lpid1 == NULL_ID) {
            lpid = lpid2;
            r = LINK_LOGIC_ONLY;
        }
        else {
            lpid = NULL_ID;
            r = LINK_RCONFLICT;
        }
    }
    else if (lpid1 != NULL_ID && lpid2 == NULL_ID) {
        lpid2 = pLogl->getLinked(q, lpid1);
        if (lpid2 == NULL_ID) {
            lpid = lpid1;
            r = LINK_LLDP_ONLY;
        }
        else {
            lpid = NULL_ID;
            r = LINK_RCONFLICT;
        }
    }
    else if (lpid1 == lpid2) {
        lpid = lpid1;
        r = LINK_LLDP_AND_LOGIC;
    }
    else {
        lpid = NULL_ID;
        r = LINK_CONFLICT;
    }

    if (_pLldp == nullptr) delete pLldp;
    if (_pLogl == nullptr) delete pLogl;

    return r;
}

QString reportComparedLinks(QSqlQuery& q, eLinkResult r, qlonglong pid, const cLldpLink& lldp, const cLogLink& logl, bool details)
{
    static const QString nl = "\n";
    QString sr;
    switch (r) {
    case LINK_LLDP_AND_LOGIC:
        sr = QObject::tr("A logikai és az LLDP link azonos.");
        if (details) {
            sr += nl;
            sr += QObject::tr("LLDP és logikai : ") + logl.show(true) + nl;
            sr += QObject::tr("A fizikai linkek lánca :") + nl;
            sr += logl.showChain().join(nl);

        }
        break;
    case LINK_NOT_FOUND:
        sr = QObject::tr("Nincs se LLDP, se logikai link.");
        break;
    case LINK_LLDP_ONLY:
        sr = QObject::tr("Nincs logikai link, csak LLDP.");
        if (details) {
            sr += nl;
            sr += QObject::tr("LLDP    : ") + lldp.show(true);
        }
        break;
    case LINK_LOGIC_ONLY:
        sr = QObject::tr("Nincs LLDP link, csak logikai.");
        if (details) {
            sr += nl;
            sr += QObject::tr("Logikai : ") + logl.show(true) + nl;
            sr += QObject::tr("A fizikai linkek lánca :") + nl;
            sr += logl.showChain().join(nl);
        }
        break;
    case LINK_CONFLICT:
    case LINK_RCONFLICT:
        sr  = QObject::tr("Az %1 ellentmondás van a logikai és LLDP linkek között.").arg(cNPort::getFullNameById(q, pid)) + nl;
        sr += QObject::tr("LLDP    : ") + lldp.show(true) + nl;
        sr += QObject::tr("Logikai : ") + logl.show(true) + nl;
        sr += QObject::tr("A fizikai linkek lánca :") + nl;
        sr += logl.showChain().join(nl);
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    return sr;
}

eLinkResult getLinkedPort(QSqlQuery& q, qlonglong pid, qlonglong& lpid, QString& msg, bool details)
{
    cLldpLink lldp;
    cLogLink  logl;
    eLinkResult r = getLinkedPort(q, pid, lpid, &lldp, &logl);
    msg = reportComparedLinks(q, r, pid, lldp, logl, details);
    return r;
}
