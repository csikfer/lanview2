#include "lanview.h"
#include "lv2user.h"

/* ------------------------------ groups ------------------------------ */
DEFAULTCRECDEF(cGroup, _sGroups)
/* ------------------------------ users ------------------------------ */
DEFAULTCRECDEF(cUser, _sUsers)

bool cUser::updatePassword(const QString &__passwd)
{
    QSqlQuery q;
    q.prepare("SELECT indalarm_update_password(:uid, :newpass)");
    q.bindValue(":uid", get(_sUserId));
    q.bindValue(":newpass", __passwd);
    bool ret = ( q.exec() && q.next() );
    if ( !ret ) {
        if ( q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(q)
        else                                                    SQLQUERYERR(q)
    }
    else         cRecord::set(q);

    return ret;
}

bool cUser::checkPassword(const QString &__passwd) const
{
    QSqlQuery q;
    q.prepare("SELECT user_id FROM users WHERE user_id = :uid AND passwd = crypt(:pass, passwd)");
    q.bindValue(":uid", getId());
    q.bindValue(":pass", __passwd);
    if ( !q.exec() || !q.next() )   return false;
    return (q.value(_sUserId).toLongLong() == getId());
}
