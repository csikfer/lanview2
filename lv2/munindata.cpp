#include "munindata.h"

int varAggregateType(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sAVARAGE, Qt::CaseInsensitive)) return VAT_AVARAGE;
    if (0 == _n.compare(_sMIN,     Qt::CaseInsensitive)) return VAT_MIN;
    if (0 == _n.compare(_sMAX,     Qt::CaseInsensitive)) return VAT_MAX;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, _n);
    return VAT_INVALID;
}

const QString& varAggregateType(int _i, eEx __ex)
{
    switch (_i) {
    case VAT_AVARAGE:   return _sAVARAGE;
    case VAT_MIN:       return _sMIN;
    case VAT_MAX:       return _sMAX;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _i);
    return _sNul;
}

int serviceVarType(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sDERIVE,   Qt::CaseInsensitive)) return SVT_DERIVE;
    if (0 == _n.compare(_sCOUNTER,  Qt::CaseInsensitive)) return SVT_COUNTER;
    if (0 == _n.compare(_sGAUGE,    Qt::CaseInsensitive)) return SVT_GAUGE;
    if (0 == _n.compare(_sABSOLUTE, Qt::CaseInsensitive)) return SVT_ABSOLUTE;
    if (0 == _n.compare(_sCOMPUTE,  Qt::CaseInsensitive)) return SVT_COMPUTE;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, _n);
    return SVT_INVALID;
}

const QString& serviceVarType(int _i, eEx __ex)
{
    switch (_i) {
    case SVT_DERIVE:    return _sDERIVE;
    case SVT_COUNTER:   return _sCOUNTER;
    case SVT_GAUGE:     return _sGAUGE;
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
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, _n);
    return SVT_INVALID;
}

const QString& varDrawType(int _i, eEx __ex)
{
    switch (_i) {
    case VDT_LINE:  return _sLINE;
    case VDT_AREA:  return _sAREA;
    case VDT_STACK: return _sSTACK;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _i);
    return _sNul;
}

/* **************************************************************************** */

CRECCNTR(cRrdBeat)

const cRecStaticDescr&  cRrdBeat::descr() const
{
    if (initPDescr<cRrdBeat>(_sRrdBeats)) {
        CHKENUM(_sDailyAggregates,   varAggregateType);
        CHKENUM(_sWeeklyAggregates,  varAggregateType);
        CHKENUM(_sMonthlyAggregates, varAggregateType);
        CHKENUM(_sYearlyAggregates,  varAggregateType);
    }
    return *_pRecordDescr;
}

CRECDEFD(cRrdBeat)
