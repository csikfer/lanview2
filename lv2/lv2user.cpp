#include "lanview.h"
#include "lv2user.h"

/* ------------------------------ groups ------------------------------ */
DEFAULTCRECDEF(cGroup, _sGroups)
/* ------------------------------ users ------------------------------ */
DEFAULTCRECDEF(cUser, _sUsers)

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

