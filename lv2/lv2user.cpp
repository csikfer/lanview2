#include "lanview.h"
#include "lv2user.h"

int privilegeLevel(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sNone,     Qt::CaseInsensitive)) return PL_NONE;
    if (0 == n.compare(_sViewer,   Qt::CaseInsensitive)) return PL_VIEWER;
    if (0 == n.compare(_sIndalarm, Qt::CaseInsensitive)) return PL_INDALARM;
    if (0 == n.compare(_sOperator, Qt::CaseInsensitive)) return PL_OPERATOR;
    if (0 == n.compare(_sAdmin,    Qt::CaseInsensitive)) return PL_ADMIN;
    if (0 == n.compare(_sSystem,   Qt::CaseInsensitive)) return PL_SYSTEM;
    if (__ex) EXCEPTION(EDATA, -1, n);
    return PL_INVALID;
}

const QString& privilegeLevel(int e, eEx __ex)
{
    switch(e) {
    case PL_NONE:       return _sNone;
    case PL_VIEWER:     return _sViewer;
    case PL_INDALARM:   return _sIndalarm;
    case PL_OPERATOR:   return _sOperator;
    case PL_ADMIN:      return _sAdmin;
    case PL_SYSTEM:     return _sSystem;
    default:    if (__ex) EXCEPTION(EDATA, e);
    }
    return _sNul;
}

/* ------------------------------ groups ------------------------------ */
CRECCNTR(cGroup)
CRECDEFD(cGroup)

const cRecStaticDescr&  cGroup::descr() const
{
    if (initPDescr<cGroup>(_sGroups)) {
        CHKENUM(_sGroupRights, privilegeLevel);
    }
    return *_pRecordDescr;
}
/* ------------------------------ users ------------------------------ */
CRECDEFD(cUser)
CRECDDCR(cUser, _sUsers)

cUser::cUser() : cRecord()
{
    _set(cUser::descr());
    _privilegeLevel = PL_INVALID;
}

cUser::cUser(const cUser& __o) : cRecord()
{
    _cp(__o);
    _privilegeLevel = __o._privilegeLevel;
}

bool cUser::checkPassword(QSqlQuery& q, const QString &__passwd) const
{
    QString sql;
    if (isNullId()) {
        if (isNullName()) EXCEPTION(EDATA);
        sql = "SELECT user_id FROM users WHERE user_name = :uname AND passwd = crypt(:pass, passwd) AND enabled";
        if (!q.prepare(sql)) SQLPREPERR(q, sql);
        q.bindValue(":uname", getName());

    }
    else {
        sql = "SELECT user_id FROM users WHERE user_id = :uid AND passwd = crypt(:pass, passwd) AND enabled";
        if (!q.prepare(sql)) SQLPREPERR(q, sql);
        q.bindValue(":uid", getId());
    }
    q.bindValue(":pass", __passwd);
    if (!q.exec()) SQLQUERYERR(q);
    return q.first() && getId() == q.value(0).toLongLong();
}

bool cUser::checkPasswordAndFetch(QSqlQuery& q, const QString &__passwd, qlonglong __id)
{
    qlonglong id = __id == NULL_ID ? getId() : __id;
    if (id == NULL_ID) {
        QString nm = getName();
        if (nm.isEmpty()) return false;
        QString sql = "SELECT * FROM users WHERE user_name = :unm AND passwd = crypt(:pass, passwd) AND enabled";
        if (!q.prepare(sql)) SQLPREPERR(q, sql);
        q.bindValue(":unm",  nm);
    }
    else {
        QString sql = "SELECT * FROM users WHERE user_id = :uid AND passwd = crypt(:pass, passwd) AND enabled";
        if (!q.prepare(sql)) SQLPREPERR(q, sql);
        q.bindValue(":uid",  id);
    }
    q.bindValue(":pass", __passwd);
    if (!q.exec()) SQLQUERYERR(q);
    if (!q.first()) return false;
    set(q);
    return true;
}

enum ePrivilegeLevel cUser::getRights(QSqlQuery& q)
{
    if (isNullId()) {
        if (isNullName()) EXCEPTION(EDATA, 0, trUtf8("Nincs megadva a felhasználói ID, vagy név"));
        setByName(q);
    }
    QString sql = "SELECT MAX(group_rights) FROM group_users JOIN groups USING(group_id) WHERE user_id = ?";
    if (!execSql(q, sql, get(idIndex()))) EXCEPTION(EDATA, 0, trUtf8("Nem azonosítható a felhasználói ID, vagy név"));
    _privilegeLevel = (enum ePrivilegeLevel)::privilegeLevel(q.value(0).toString());
    return _privilegeLevel;
}

enum ePrivilegeLevel cUser::privilegeLevel() const
{
    if (_privilegeLevel == PL_INVALID) {
        QSqlQuery q = getQuery();
        return const_cast<cUser *>(this)->getRights(q);
    }
    return _privilegeLevel;
}

