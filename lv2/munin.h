#ifndef CMUNIN_H
#define CMUNIN_H
#include "lv2service.h"



class cInspMunin : public cInspector
{
public:
    // cInspMunin(QSqlQuery &q, const QString& sn);
    cInspMunin(QSqlQuery& q, qlonglong __host_service_id = NULL_ID, qlonglong __tableoid = NULL_ID, cInspector * __par = NULL);


};

#endif // CMUNIN_H
