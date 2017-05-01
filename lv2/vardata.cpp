#include "vardata.h"

int varAggregateType(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sAVARAGE, Qt::CaseInsensitive)) return VAT_AVARAGE;
    if (0 == _n.compare(_sMIN,     Qt::CaseInsensitive)) return VAT_MIN;
    if (0 == _n.compare(_sMAX,     Qt::CaseInsensitive)) return VAT_MAX;
    if (0 == _n.compare(_sLAST,    Qt::CaseInsensitive)) return VAT_LAST;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, _n);
    return ENUM_INVALID;
}

const QString& varAggregateType(int _i, eEx __ex)
{
    switch (_i) {
    case VAT_AVARAGE:   return _sAVARAGE;
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
    if (0 == _n.compare(_sCOMPUTE,  Qt::CaseInsensitive)) return SVT_COMPUTE;
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
    case SVT_COMPUTE:   return _sCOMPUTE;
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
        CHKENUM(_sDailyAggregates,   varAggregateType);
        CHKENUM(_sWeeklyAggregates,  varAggregateType);
        CHKENUM(_sMonthlyAggregates, varAggregateType);
        CHKENUM(_sYearlyAggregates,  varAggregateType);
        _ixFeatures = _descr_cRrdBeat().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}

CRECDEFD(cRrdBeat)

/* ---------------------------------------------------------------------------- */

CRECCNTR(cServiceVarType)

int  cServiceVarType::_ixFeatures = NULL_IX;
const cRecStaticDescr&  cServiceVarType::descr() const
{
    if (initPDescr<cServiceVarType>(_sServiceVarTypes)) {
        CHKENUM(_sServiceVarType, serviceVarType);
        _ixFeatures = _descr_cServiceVarType().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}

CRECDEFD(cServiceVarType)

/* ---------------------------------------------------------------------------- */
CRECCNTR(cServiceVar)
int cServiceVar::_ixFeatures         = NULL_IX;
int cServiceVar::_ixServiceVarTypeId = NULL_IX;

const cRecStaticDescr&  cServiceVar::descr() const
{
    if (initPDescr<cServiceVar>(_sServiceVars)) {
        _ixFeatures = _descr_cServiceVar().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}

CRECDEFD(cServiceVar)

void cServiceVar::clearToEnd()
{
    _varType.clear();
}

void cServiceVar::toEnd()
{
    toEnd(_ixServiceVarTypeId);
}

bool cServiceVar::toEnd(int _ix)
{
    if (_ix == _ixServiceVarTypeId) {
        if (getId(_ixServiceVarTypeId) != _varType.getId()) _varType.clear();
        return true;
    }
    return false;
}

cServiceVarType& cServiceVar::varType(QSqlQuery& q, eEx __ex)
{
    qlonglong tid = getId(_ixServiceVarTypeId);
    if (_varType.getId() != tid) {
        if (!_varType.fetchById(q, tid)) {
            if (__ex != EX_IGNORE) EXCEPTION(EFOUND, tid, identifying(false));
        }
    }
    return _varType;
}

/* ---------------------------------------------------------------------------- */
CRECCNTR(cGraph)
int  cGraph::_ixFeatures = NULL_IX;
const cRecStaticDescr&  cGraph::descr() const
{
    if (initPDescr<cGraph>(_sGraphs)) {
        _ixFeatures = _descr_cGraph().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}
CRECDEFD(cGraph)
/* ---------------------------------------------------------------------------- */

CRECCNTR(cGraphVar)

int  cGraphVar::_ixFeatures = NULL_IX;
const cRecStaticDescr&  cGraphVar::descr() const
{
    if (initPDescr<cGraphVar>(_sGraphVars)) {
        CHKENUM(_sDrawType, varDrawType);
    }
    return *_pRecordDescr;
}

CRECDEFD(cGraphVar)
