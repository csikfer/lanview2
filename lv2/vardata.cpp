#include "vardata.h"

int varAggregateType(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sAVERAGE, Qt::CaseInsensitive)) return VAT_AVERAGE;
    if (0 == _n.compare(_sMIN,     Qt::CaseInsensitive)) return VAT_MIN;
    if (0 == _n.compare(_sMAX,     Qt::CaseInsensitive)) return VAT_MAX;
    if (0 == _n.compare(_sLAST,    Qt::CaseInsensitive)) return VAT_LAST;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, _n);
    return ENUM_INVALID;
}

const QString& varAggregateType(int _i, eEx __ex)
{
    switch (_i) {
    case VAT_AVERAGE:   return _sAVERAGE;
    case VAT_MIN:       return _sMIN;
    case VAT_MAX:       return _sMAX;
    case VAT_LAST:      return _sLAST;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _i);
    return _sNul;
}

int serviceVarType(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sGAUGE,    Qt::CaseInsensitive)) return SVT_GAUGE;
    if (0 == _n.compare(_sCOUNTER,  Qt::CaseInsensitive)) return SVT_COUNTER;
    if (0 == _n.compare(_sDCOUNTER, Qt::CaseInsensitive)) return SVT_DCOUNTER;
    if (0 == _n.compare(_sDERIVE,   Qt::CaseInsensitive)) return SVT_DERIVE;
    if (0 == _n.compare(_sDDERIVE,  Qt::CaseInsensitive)) return SVT_DDERIVE;
    if (0 == _n.compare(_sABSOLUTE, Qt::CaseInsensitive)) return SVT_ABSOLUTE;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, _n);
    return ENUM_INVALID;
}

const QString& serviceVarType(int _i, eEx __ex)
{
    switch (_i) {
    case SVT_GAUGE:     return _sGAUGE;
    case SVT_COUNTER:   return _sCOUNTER;
    case SVT_DCOUNTER:  return _sDCOUNTER;
    case SVT_DERIVE:    return _sDERIVE;
    case SVT_DDERIVE:   return _sDDERIVE;
    case SVT_ABSOLUTE:  return _sABSOLUTE;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _i);
    return _sNul;
}

int varDrawType(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sLINE,  Qt::CaseInsensitive)) return VDT_LINE;
    if (0 == _n.compare(_sAREA,  Qt::CaseInsensitive)) return VDT_AREA;
    if (0 == _n.compare(_sSTACK, Qt::CaseInsensitive)) return VDT_STACK;
    if (0 == _n.compare(_sNONE,  Qt::CaseInsensitive)) return VDT_NONE;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, _n);
    return ENUM_INVALID;
}

const QString& varDrawType(int _i, eEx __ex)
{
    switch (_i) {
    case VDT_LINE:  return _sLINE;
    case VDT_AREA:  return _sAREA;
    case VDT_STACK: return _sSTACK;
    case VDT_NONE:  return _sNONE;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _i);
    return _sNul;
}

/* **************************************************************************** */

CRECCNTR(cRrdBeat)

int  cRrdBeat::_ixFeatures = NULL_IX;
const cRecStaticDescr&  cRrdBeat::descr() const
{
    if (initPDescr<cRrdBeat>(_sRrdBeats)) {
        CHKENUM(_sDailyAggregates,   varAggregateType)
        // CHKENUM(_sWeeklyAggregates,  varAggregateType);
        // CHKENUM(_sMonthlyAggregates, varAggregateType);
        // CHKENUM(_sYearlyAggregates,  varAggregateType);
        _ixFeatures = _descr_cRrdBeat().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}

CRECDEFD(cRrdBeat)

/* ---------------------------------------------------------------------------- */

int cServiceVarType::_ixFeatures = NULL_IX;
int cServiceVarType::_ixParamTypeId = NULL_IX;
int cServiceVarType::_ixRawParamTypeId = NULL_IX;

cServiceVarType::cServiceVarType() : cRecord()
{
    _pFeatures = nullptr;
    _set(cServiceVarType::descr());
}
cServiceVarType::cServiceVarType(const cServiceVarType& __o) : cRecord()
{
    _pFeatures = nullptr;
    _cp(__o);
    _pFeatures = __o._pFeatures == nullptr ? nullptr : new cFeatures(*__o._pFeatures);
}


const cRecStaticDescr&  cServiceVarType::descr() const
{
    if (initPDescr<cServiceVarType>(_sServiceVarTypes)) {
        CHKENUM(_sServiceVarType, serviceVarType)
        STFIELDIX(cServiceVarType, Features);
        STFIELDIX(cServiceVarType, ParamTypeId);
        STFIELDIX(cServiceVarType, RawParamTypeId);

    }
    return *_pRecordDescr;
}

CRECDEF(cServiceVarType)

cServiceVarType::~cServiceVarType() {
    pDelete(_pFeatures);
}

RECACHEDEF(cServiceVarType, srvartype)

/* ---------------------------------------------------------------------------- */
int cServiceVar::_ixFeatures         = NULL_IX;
int cServiceVar::_ixServiceVarTypeId = NULL_IX;
int cServiceVar::_ixServiceVarValue  = NULL_IX;
int cServiceVar::_ixVarState         = NULL_IX;
int cServiceVar::_ixLastTime         = NULL_IX;
int cServiceVar::_ixRawValue         = NULL_IX;
int cServiceVar::_ixStateMsg         = NULL_IX;
int cServiceVar::_ixRarefaction      = NULL_IX;

cServiceVar::cServiceVar() : cRecord()
{
    _pFeatures = nullptr;
    _set(cServiceVar::descr());
}
cServiceVar::cServiceVar(const cServiceVar& __o) : cRecord()
{
    _pFeatures = nullptr;
    _cp(__o);
    _pFeatures = __o._pFeatures == nullptr ? nullptr : new cFeatures(*__o._pFeatures);
}

cServiceVar::~cServiceVar()
{
    pDelete(_pFeatures);
}

const cRecStaticDescr&  cServiceVar::descr() const
{
    if (initPDescr<cServiceVar>(_sServiceVars)) {
        STFIELDIX(cServiceVar, Features);
        STFIELDIX(cServiceVar, ServiceVarTypeId);
        STFIELDIX(cServiceVar, ServiceVarValue);
        STFIELDIX(cServiceVar, VarState);
        STFIELDIX(cServiceVar, LastTime);
        STFIELDIX(cServiceVar, RawValue);
        STFIELDIX(cServiceVar, StateMsg);
        STFIELDIX(cServiceVar, Rarefaction);
    }
    return *_pRecordDescr;
}

CRECDEF(cServiceVar)

void cServiceVar::clearToEnd()
{
    _pFeatures = nullptr;
}

void cServiceVar::toEnd()
{
    toEnd(_ixFeatures);
    cRecord::toEnd();
}

bool cServiceVar::toEnd(int _ix)
{
    if (_ix == _ixFeatures) {
        pDelete( _pFeatures);
        return true;
    }
    return false;
}

const cServiceVarType& cServiceVar::varType(QSqlQuery& q)
{
    return *cServiceVarType::srvartype(q, getId(ixServiceVarTypeId()), EX_ERROR);
}

cServiceVar *cServiceVar::getVarObjectById(QSqlQuery& q, qlonglong _id)
{
    cServiceVar *r = nullptr;
    cServiceVar v;
    qlonglong tableoid = v.setId(_id).fetchTableOId(q);
    if (tableoid == v.tableoid()) r = new cServiceVar;
    else                          r = new cServiceRrdVar;
    r->setById(q, _id);
    return r;
}

cServiceVar *cServiceVar::getVarObjectByName(QSqlQuery& q, const QString& _name, qlonglong _hsid, eEx __ex)
{
    if (_hsid == NULL_ID) {
        EXCEPTION(EDBDATA, 0, tr("_hsid is NULL"));
    }
    cServiceVar *r = nullptr;
    cServiceVar v;
    static const int ixHostServiceId = v.toIndex(_sHostServiceId);
    v.setName(_name);
    v.setId(ixHostServiceId, _hsid);
    qlonglong tableoid = v.fetchTableOId(q, __ex);
    if (tableoid == NULL_ID) return r;      // not found and __ex is EX_IGNORE
    if (tableoid == v.tableoid()) r = new cServiceVar;
    else                          r = new cServiceRrdVar;
    r->setName(_name);
    r->setId(ixHostServiceId, _hsid);
    int n = r->completion(q);
    switch (n) {
    case 0: // Not found
        if (__ex != EX_IGNORE) {
            EXCEPTION(EFOUND, _hsid, _name);
        }
        pDelete(r);
        break;
    case 1: // Fund
        break;
    default:// Incredible
        EXCEPTION(EDBDATA, n, r->identifying());
    }
    return r;
}

/* ---------------------------------------------------------------------------- */

cServiceRrdVar::cServiceRrdVar()
    : cServiceVar()
{
    _set(cServiceRrdVar::descr());
}
cServiceRrdVar::cServiceRrdVar(const cServiceRrdVar& __o)
    : cServiceVar (static_cast<const cServiceVar&>(__o))
{
    _cp(__o);
}

CRECDEFD(cServiceRrdVar)

const cRecStaticDescr&  cServiceRrdVar::descr() const
{
    if (initPDescr<cServiceRrdVar>(_sServiceRrdVars)) {

    }
    return *_pRecordDescr;
}
/* ---------------------------------------------------------------------------- */
