#include "munin.h"

/*cInspMunin::cInspMunin(QSqlQuery &q, const QString& sn) : cInspector(q, sn)
{

}*/

cInspMunin::cInspMunin(QSqlQuery& q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * __par)
    : cInspector(q, __host_service_id, __tableoid, __par)
{

}
