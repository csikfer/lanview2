#ifndef LV2DATERR_H
#define LV2DATERR_H

#include "lv2datab.h"
#include "lv2cont.h"
#include "qsnmp.h"

/*!
@file lv2daterr.h
Az adatbázis interfész adat hiba objektum.
*/

/*!
@class cDbErrType
@brief A errors tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cDbErrType : public cRecord {
    CRECORD(cDbErrType);
public:
    static const QString    _sOk;
    static const QString    _sStart;
    static const QString    _sReStart;
    static const QString    _sInfo;
    static const QString    _sParam;
    static const QString    _sNotFound;
    static const QString    _sDisabled;
    static const QString    _sDataWarn;
    static const QString    _sDrop;
    static const QString    _sRunTime;
};

/*!
@class cDbErr
@brief A db_errs tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cDbErr : public cRecord {
    CRECORD(cDbErr);
public:
    static int insertNew(const QString& type, const QString& msg, int code, const QString& table, const QString& func);
    static int insertNew(QSqlQuery &q, const QString& type, const QString& msg, int code, const QString& table, const QString& func);
};


#endif // LV2DATERR_H
