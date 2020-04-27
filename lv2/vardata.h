#ifndef VARDATA
#define VARDATA

#include "lanview.h"
#include "guidata.h"
#include "srvdata.h"

enum eVarAggregateType {
    VAT_AVERAGE =  0,
    VAT_MIN,
    VAT_MAX,
    VAT_LAST,
    VAT_NUMBER
};

EXT_ int varAggregateType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& varAggregateType(int _i, eEx __ex = EX_ERROR);


enum eServiceVarType {
    SVT_GAUGE  =  0,
    SVT_COUNTER,
    SVT_DCOUNTER,
    SVT_DERIVE,
    SVT_DDERIVE,
    SVT_ABSOLUTE
};

EXT_ int serviceVarType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& serviceVarType(int _i, eEx __ex = EX_ERROR);

enum eVarDrawType {
    VDT_LINE = 0,
    VDT_AREA,
    VDT_STACK,
    VDT_NONE
};

EXT_ int varDrawType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& varDrawType(int _i, eEx __ex = EX_ERROR);


/*!
@class cRrdBeats
@brief
*/
class LV2SHARED_EXPORT cRrdBeat : public cRecord {
    CRECORD(cRrdBeat);
    FEATURES(cRrdBeat)
public:
};

/*!
@class cServiceVarType
@brief
 */
class LV2SHARED_EXPORT cServiceVarType : public cRecord {
    CRECORD(cServiceVarType);
    FEATURES(cServiceVarType)
    RECACHEHED(cServiceVarType, srvartype)
    STATICIX(cServiceVarType, ParamTypeId)
    STATICIX(cServiceVarType, RawParamTypeId)
};

/*!
@class cServiceVar
@brief
 */
class LV2SHARED_EXPORT cServiceVar : public cRecord {
    CRECORD(cServiceVar);
    FEATURES(cServiceVar)
public:
    virtual void clearToEnd();
    virtual void toEnd();
    virtual bool toEnd(int _ix);
    const cServiceVarType &varType(QSqlQuery& q);
    const cParamType& dataType(QSqlQuery& q)    { return cParamType::paramType(varType(q).getId(cServiceVarType::ixParamTypeId())); }
    const cParamType& rawDataType(QSqlQuery& q) { return cParamType::paramType(varType(q).getId(cServiceVarType::ixRawParamTypeId())); }
    QString  valToString(QSqlQuery& q, const QVariant& val)  { return dataType(q).paramToString(val); }
    QString  rawValToString(QSqlQuery& q, const QVariant& val)  { return rawDataType(q).paramToString(val); }
    static cServiceVar *getVarObjectById(QSqlQuery& q, qlonglong _id);
    static cServiceVar *getVarObjectByName(QSqlQuery& q, const QString& _name, qlonglong _hsid, eEx __ex = EX_ERROR);

    STATICIX(cServiceVar, ServiceVarTypeId)
    STATICIX(cServiceVar, ServiceVarValue)
    STATICIX(cServiceVar, VarState)
    STATICIX(cServiceVar, LastTime)
    STATICIX(cServiceVar, RawValue)
    STATICIX(cServiceVar, StateMsg)
    STATICIX(cServiceVar, Rarefaction)
};

/*!
@class cServiceRrdVar
@brief
 */
class LV2SHARED_EXPORT cServiceRrdVar : public cServiceVar {
    CRECORD(cServiceRrdVar);
public:
};

#if 0
/*!
@class cSrvDiagramTypVar
@brief
 */
class LV2SHARED_EXPORT cSrvDiagramTypeVar : public cRecord {
    CRECORD(cSrvDiagramTypeVar);
    FEATURES(cSrvDiagramTypeVar)
    STATICIX(cSrvDiagramTypeVar, SrvDiagramTypeId)
public:
};

/*!
@class cSrvDiagramType
@brief
 */
class LV2SHARED_EXPORT cSrvDiagramType : public cRecord {
    CRECORD(cSrvDiagramType);
    FEATURES(cSrvDiagramType)
    STATICIX(cSrvDiagramType, ServiceId)
public:
    tOwnRecords<cSrvDiagramTypeVar, cSrvDiagramType>    vars;

};

#endif

#endif // VARDATA

