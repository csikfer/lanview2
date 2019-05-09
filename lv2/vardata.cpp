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
        CHKENUM(_sServiceVarType, serviceVarType);
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
QBitArray                   cServiceVar::updateMask;
tRecordList<cServiceVar>    cServiceVar::serviceVars;
QMap<qlonglong, qlonglong>  cServiceVar::heartbeats;
QString cServiceVar::sInvalidValue;
QString cServiceVar::sNotCredible;
QString cServiceVar::sFirstValue;
QString cServiceVar::sRawIsNull;
cServiceVar::cServiceVar() : cRecord()
{
    _pFeatures = nullptr;
    pEnumVals = nullptr;
    pVarType = nullptr;
    lastCount = 0;
    _set(cServiceVar::descr());
    skeepCnt = 0;
    featuresIsMerged = false;
}
cServiceVar::cServiceVar(const cServiceVar& __o) : cRecord()
{
    _pFeatures = nullptr;
    pEnumVals  = nullptr;
    _cp(__o);
    _pFeatures = __o._pFeatures == nullptr ? nullptr : new cFeatures(*__o._pFeatures);
    pEnumVals  = __o.pEnumVals  == nullptr ? nullptr : new QStringList(*__o.pEnumVals);
    pVarType  = __o.pVarType;
    lastCount = __o.lastCount;
    lastTime  = __o.lastTime;
    skeepCnt = 0;
    featuresIsMerged = __o.featuresIsMerged;
}

cServiceVar::~cServiceVar()
{
    pDelete(pEnumVals);
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
        updateMask = _descr_cServiceVar().mask(_ixLastTime, _ixRawValue)
                   | _descr_cServiceVar().mask(_ixVarState, _ixServiceVarValue, _ixStateMsg);
        sInvalidValue = trUtf8("Value is invalid : %1");
        sNotCredible  = trUtf8("Data is not credible.");
        sFirstValue   = trUtf8("First value, no data.");
        sRawIsNull    = trUtf8("Raw value is NULL");
    }
    return *_pRecordDescr;
}

CRECDEF(cServiceVar)

void cServiceVar::clearToEnd()
{
    _pFeatures = nullptr;
    pEnumVals = nullptr;
    pVarType = nullptr;
    lastCount = 0;
    lastTime = QDateTime();
}

void cServiceVar::toEnd()
{
    toEnd(_ixServiceVarTypeId);
    toEnd(_ixFeatures);
    cRecord::toEnd();
}

bool cServiceVar::toEnd(int _ix)
{
    if (_ix == _ixServiceVarTypeId) {
        if (pVarType != nullptr && getId(_ixServiceVarTypeId) != pVarType->getId()) {
            pVarType = nullptr;
            pDelete( _pFeatures);
            pDelete(pEnumVals);
        }
        return true;
    }
    if (_ix == _ixFeatures) {
        pDelete( _pFeatures);
        return true;
    }
    return false;
}

const cServiceVarType *cServiceVar::varType(QSqlQuery& q, eEx __ex)
{
    if (pVarType == nullptr || pVarType->getId() != getId(_ixServiceVarTypeId)) {
        pVarType = cServiceVarType::srvartype(q, getId(_ixServiceVarTypeId), __ex);
    }
    return pVarType;
}

const cFeatures& cServiceVar::mergedFeatures()
{
    if (featuresIsMerged) {
        if (_pFeatures == nullptr) EXCEPTION(EPROGFAIL);
    }
    else {
        if (pVarType == nullptr) EXCEPTION(EPROGFAIL);
        cFeatures *pTmp = new cFeatures(pVarType->features());
        pTmp->merge(features());
        pDelete(_pFeatures);
        _pFeatures = pTmp;
    }
    return *_pFeatures;
}

int cServiceVar::setValue(QSqlQuery& q, const QVariant& _rawVal, int& state)
{
    if (skeep()) return ENUM_INVALID;
    if (_rawVal.isNull()) {
        addMsg(sRawIsNull);
        return noValue(q, state);
    }
    eParamType ptRaw = eParamType(rawDataType(q).getId(cParamType::ixParamTypeType()));
    eTristate rawChanged = preSetValue(q, ptRaw, _rawVal, state);
    if (rawChanged == TS_NULL) return RS_UNREACHABLE;
    bool ok = true;
    switch (ptRaw) {
    case PT_INTEGER: {
        qlonglong rwi = _rawVal.toLongLong(&ok);
        if (ok) return setValue(q, rwi, state, rawChanged);
    }   break;
    case PT_REAL: {
        double    rwd = _rawVal.toDouble(&ok);
        if (ok) return setValue(q, rwd, state, rawChanged);
    }   break;
    case PT_INTERVAL: {
        qlonglong rwi;
        if (variantIsInteger(_rawVal)) rwi = _rawVal.toLongLong(&ok);
        else                           rwi = parseTimeInterval(_rawVal.toString(), &ok);
        if (ok) return setValue(q, rwi, state, rawChanged);
    }   break;
    case PT_TEXT:
        if (nullptr != enumVals()) {   // ENUM?
            qlonglong i = enumVals()->indexOf(_rawVal.toString());
            return updateEnumVar(q, i, state);
        }
        break;
    default:
        break;
    }
    if (!ok) {
        addMsg(sInvalidValue.arg(debVariantToString(_rawVal)));
        return noValue(q, state);
    }
    int rs;
    if (rawChanged == TS_TRUE) {
        eParamType ptVal = eParamType(dataType(q).getId(cParamType::ixParamTypeType()));
        rs = postSetValue(q, ptVal, _rawVal, RS_ON, state);
    }
    else {
        touch(q);
        rs = int(getId(_ixVarState));
    }
    return rs;
}

int cServiceVar::setValue(QSqlQuery& q, double val, int& state, eTristate rawChg)
{
    int rpt = int(rawDataType(q).getId(_sParamTypeType));
    // Check raw type
    if (rpt == PT_INTEGER || rpt == PT_INTERVAL) {
        return setValue(q, qlonglong(val + 0.5), state, rawChg);
    }
    if (rpt != PT_REAL) {    // supported types
        addMsg(trUtf8("Invalid context : Non numeric raw data type %1.").arg(paramTypeType(rpt)));
        noValue(q, state);
        return RS_UNREACHABLE;
    }
    eTristate changed = TS_NULL;
    switch (rawChg) {
    case TS_NULL:
        if (skeep()) return ENUM_INVALID;
        changed = preSetValue(q, rpt, QVariant(val), state);
        if (changed == TS_NULL) return RS_UNREACHABLE;
        break;
    case TS_TRUE:
    case TS_FALSE:
        changed = rawChg;
        break;
    }
    // Var preprocessing methode
    qlonglong svt = varType(q)->getId(_sServiceVarType);
    switch (svt) {
    case SVT_COUNTER:
    case SVT_DCOUNTER:
        return setCounter(q, qlonglong(val + 0.5), int(svt), state);
    case SVT_DERIVE:
    case SVT_DDERIVE:
         return setDerive(q, val, state);
    }
    int rs = int(getId(_sVarState));
    if (changed == TS_FALSE && rs != RS_UNKNOWN) {
        touch(q);
        return rs;
    }
    rs = RS_INVALID;
    switch (svt) {
    case SVT_ABSOLUTE:
        if (val < 0) {
            val = - val;
        }
        break;
    case SVT_COMPUTE: {
        QString rpn = mergedFeatures().value("rpn");
        QString err;
        val = rpn_calc(val, rpn, mergedFeatures(), err);
        if (!err.isEmpty()) {
            addMsg(trUtf8("RPN error : %1").arg(err));
            rs = RS_UNKNOWN;
            postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
        }
    }   break;
    }
    int vpt = int(dataType(q).getId(_sParamTypeType));
    switch (svt) {
    case SVT_ABSOLUTE:
    case NULL_ID:
    case SVT_GAUGE:
    case SVT_COMPUTE: {
        switch (vpt) {
        case PT_INTEGER:    rs = updateVar(q, qlonglong(val + 0.5), state);   break;
        case PT_REAL:       rs = updateVar(q, val,                  state);   break;
        case PT_INTERVAL:   rs = updateIntervalVar(q, qlonglong(val), state); break;
        default:
            addMsg(trUtf8("Unsupported service Variable data type : %1").arg(paramTypeType(rpt)));
            rs = RS_UNKNOWN;
            postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
            return rs;
        }
    }   break;
    default:
        addMsg(trUtf8("Unsupported service Variable type : %1").arg(serviceVarType(int(svt))));
        rs = RS_UNKNOWN;
        postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
    }
    return rs;
}

int cServiceVar::setValue(QSqlQuery& q, qlonglong val, int &state, eTristate rawChg)
{
    int rpt = int(rawDataType(q).getId(_sParamTypeType));
    // Check raw type
    if (rpt == PT_REAL) {
        return setValue(q, double(val), state, rawChg);
    }
    if (rpt != PT_INTEGER && rpt != PT_INTERVAL) {    // supported types
        addMsg(trUtf8("Invalid context : Non numeric raw data type %1.").arg(paramTypeType(rpt)));
        noValue(q, state);
        return RS_UNREACHABLE;
    }
    eTristate changed;
    switch (rawChg) {
    case TS_NULL:
        if (skeep()) return ENUM_INVALID;
        changed = preSetValue(q, rpt, QVariant(val), state);
        if (changed == TS_NULL) return RS_UNREACHABLE;
        break;
    case TS_TRUE:
    case TS_FALSE:
        changed = rawChg;
        break;
    }
    // Var preprocessing methode
    qlonglong svt = varType(q)->getId(_sServiceVarType);
    switch (svt) {
    case SVT_COUNTER:
    case SVT_DCOUNTER:
    case SVT_DERIVE:
    case SVT_DDERIVE:
         return setCounter(q, val, int(svt), state);
    }
    int rs = int(getId(_sVarState));
    if (changed == TS_FALSE && rs != RS_UNKNOWN) {
        touch(q);
        return rs;
    }
    rs = RS_INVALID;
    double d = val;
    switch (svt) {
    case SVT_ABSOLUTE:
        if (val < 0) {
            val = - val;
            d   = - d;
        }
        break;
    case SVT_COMPUTE: {
        QString rpn = mergedFeatures().value("rpn");
        QString err;
        d = rpn_calc(d, rpn, mergedFeatures(), err);
        if (!err.isEmpty()) {
            addMsg(trUtf8("RPN error : %1").arg(err));
            rs = RS_UNKNOWN;
            postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
            return rs;
        }
        val = qlonglong(d + 0.5);
        break;
    }
    }
    const cParamType& pt = dataType(q);
    int vpt = int(pt.getId(cParamType::ixParamTypeType()));
//    int vpt = int(dataType(q).getId(_sParamTypeType));
    switch (svt) {
    case SVT_ABSOLUTE:
    case NULL_ID:
    case SVT_GAUGE:
    case SVT_COMPUTE: {
        switch (vpt) {
        case PT_INTEGER:    return updateVar(q, val,         state);
        case PT_REAL:       return updateVar(q, d,           state);
        case PT_INTERVAL:   return updateIntervalVar(q, val, state);
        case PT_TEXT:
            if (nullptr != enumVals()) {   // ENUM ?
                return updateEnumVar(q, val, state);
            }
        }
        addMsg(trUtf8("Invalid context. Unsupported service Variable data type : %1").arg(paramTypeType(rpt)));
        rs = RS_UNKNOWN;
        postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
    }   break;
    default:
        addMsg(trUtf8("Unsupported service Variable type : %1").arg(serviceVarType(int(svt))));
        rs = RS_UNKNOWN;
        postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
    }
    return rs;
}

int cServiceVar::setUnreachable(QSqlQuery q, const QString& msg)
{
    setName(_ixVarState, _sUnreachable);
    setName(_ixLastTime, _sNOW);
    setName(_ixStateMsg, msg);
    return update(q, false, mask(_ixVarState, _ixLastTime));
}

const QStringList * cServiceVar::enumVals()
{
    if (pEnumVals == nullptr) {
        pEnumVals = new QStringList(mergedFeatures().slValue(_sEnumeration));
    }
    return pEnumVals;
}


int cServiceVar::setValue(QSqlQuery& q, qlonglong _hsid, const QString& _name, const QVariant& val, int& state)
{
    int rs;
    int r;
    cServiceVar *p = serviceVar(q, _hsid, _name);
    r = p->setValue(q, val, rs);
    if (p->getBool(_sDelegateServiceState) && state < rs) state = rs;
    if (rs > state) state = rs;
    return r;
}

int cServiceVar::setValues(QSqlQuery& q, qlonglong _hsid, const QStringList& _names, const QVariantList& vals, int& state)
{
    int r = RS_ON;
    int n = _names.size();
    n = std::min(n, vals.size());
    for (int i = 0; i < n; ++i) {
        int rr = setValue(q, _hsid, _names.at(i), vals.at(i), state);
        if (r < rr) r = rr;
    }
    return r;
}

eTristate cServiceVar::preSetValue(QSqlQuery& q, int ptRaw, const QVariant& rawVal, int& state)
{
    now = QDateTime::currentDateTime();
    clear(_ixStateMsg);
    if (rawVal.isNull()) {
        addMsg(trUtf8("Raw data is NULL."));
        noValue(q, state);
        return TS_NULL;
    }
    bool ok;
    QString s = cParamType::paramToString(eParamType(ptRaw), rawVal, EX_IGNORE, &ok);
    if (!ok) {
        addMsg(trUtf8("Conversion of raw data (%1) failed.").arg(debVariantToString(ptRaw)));
        postSetValue(q, ENUM_INVALID, QVariant(), RS_UNREACHABLE, state);
        return TS_NULL;
    }
    bool changed = s.compare(getName(_ixRawValue));
    if (changed) setName(_ixRawValue, s);
    lastLast = get(_ixLastTime).toDateTime();
    set(_ixLastTime, now);
    return bool2ts(changed); // Raw value is changed ?
}

bool cServiceVar::postSetValue(QSqlQuery& q, int ptVal, const QVariant& val, int rs, int& state)
{
    bool ok = true;
    if (ptVal == ENUM_INVALID || val.isNull()) clear(_ixServiceVarValue);
    else setName(_ixServiceVarValue, cParamType::paramToString(eParamType(ptVal), val, EX_IGNORE, &ok));
    if (!ok) {
        rs = RS_UNREACHABLE;
        addMsg(trUtf8("Conversion of target data (%1) failed.").arg(debVariantToString(ptVal)));
    }
    if (getBool(_sDelegateServiceState) && state < rs) state = rs;
    setId(_ixVarState, rs);
    return 1 == update(q, false, updateMask);
}

int cServiceVar::setCounter(QSqlQuery& q, qlonglong val, int svt, int &state)
{
    if (!lastTime.isValid()) {
        lastCount = val;
        lastTime  = now;
        addMsg(sFirstValue);
        return noValue(q, state, ENUM_INVALID);
    }
    qlonglong delta = 0;
    switch (svt) {
    case SVT_COUNTER:
    case SVT_DERIVE:    // A számláló tulcsordulás (32 bit)
        if (lastCount > val) {
            addMsg(trUtf8("A számláló túlcsordult."));
            delta = val + 0x100000000LL - lastCount;
            if (delta > 0x100000000LL) {
                lastCount = val;
                lastTime  = now;
                addMsg(trUtf8("Többszörös túlcsordulás?"));
                return noValue(q, state);
            }
            break;
        }
        delta = val - lastCount;
        break;
    case SVT_DDERIVE:   //
    case SVT_DCOUNTER:
        delta = val - lastCount;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    lastCount = val;
    double dt = double(lastTime.msecsTo(now)) / 1000;
    lastTime = now;
    double dVal = double(delta) / dt;
    return updateVar(q, dVal, state);
}

int cServiceVar::setDerive(QSqlQuery &q, double val, int& state)
{
    QDateTime now = QDateTime::currentDateTime();
    if (!lastTime.isValid()) {
        lastValue = val;
        lastTime  = now;
        addMsg(sFirstValue);
        return noValue(q, state, ENUM_INVALID);
    }
    double delta = val - lastValue;
    lastValue = val;
    double dt = double(lastTime.msecsTo(now)) / 1000;
    lastTime = now;
    double dVal = delta / double(dt);
    return updateVar(q, dVal, state);
}


int cServiceVar::updateIntervalVar(QSqlQuery& q, qlonglong val, int &state)
{
    int rs = RS_ON;
    if (TS_TRUE != checkIntervalValue(val, varType(q)->getId(_sPlausibilityType), varType(q)->getName(_sPlausibilityParam1), varType(q)->getName(_sPlausibilityParam2), varType(q)->getBool(_sPlausibilityInverse), true)) {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    if (TS_TRUE == checkIntervalValue(val, varType(q)->getId(_sCriticalType), varType(q)->getName(_sCriticalParam1), varType(q)->getName(_sCriticalParam2), varType(q)->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkIntervalValue(val, varType(q)->getId(_sWarningType), varType(q)->getName(_sWarningParam1), varType(q)->getName(_sWarningParam2), varType(q)->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    postSetValue(q, PT_INTERVAL, val, rs, state);
    return rs;
}

int cServiceVar::updateVar(QSqlQuery& q, qlonglong val, int &state)
{
    if (TS_TRUE != checkIntValue(val, varType(q)->getId(_sPlausibilityType), varType(q)->getName(_sPlausibilityParam1), varType(q)->getName(_sPlausibilityParam2), varType(q)->getBool(_sPlausibilityInverse), true)) {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkIntValue(val, varType(q)->getId(_sCriticalType), varType(q)->getName(_sCriticalParam1), varType(q)->getName(_sCriticalParam2), varType(q)->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkIntValue(val, varType(q)->getId(_sWarningType), varType(q)->getName(_sWarningParam1), varType(q)->getName(_sWarningParam2), varType(q)->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    postSetValue(q, PT_INTEGER, val, rs, state);
    return rs;
}

int cServiceVar::updateVar(QSqlQuery& q, double val, int& state)
{
    if (TS_TRUE != checkRealValue(val, varType(q)->getId(_sPlausibilityType), varType(q)->getName(_sPlausibilityParam1), varType(q)->getName(_sPlausibilityParam2), varType(q)->getBool(_sPlausibilityInverse), true)) {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkRealValue(val, varType(q)->getId(_sCriticalType), varType(q)->getName(_sCriticalParam1), varType(q)->getName(_sCriticalParam2), varType(q)->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkRealValue(val, varType(q)->getId(_sWarningType), varType(q)->getName(_sWarningParam1), varType(q)->getName(_sWarningParam2), varType(q)->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    postSetValue(q, PT_REAL, val, rs, state);
    return rs;
}

int cServiceVar::updateEnumVar(QSqlQuery& q, qlonglong i, int& state)
{
    int ix = int(i);
    if (pEnumVals == nullptr) EXCEPTION(EPROGFAIL);
    const QStringList& evals = *pEnumVals;
    if (!isContIx(evals, i) || evals[ix] == "!") {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkEnumValue(ix, evals, varType(q)->getId(_sCriticalType), varType(q)->getName(_sCriticalParam1), varType(q)->getName(_sCriticalParam2), varType(q)->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkEnumValue(ix, evals, varType(q)->getId(_sWarningType), varType(q)->getName(_sWarningParam1), varType(q)->getName(_sWarningParam2), varType(q)->getBool(_sWarningInverse))) {
        rs = RS_CRITICAL;
    }
    postSetValue(q, PT_TEXT, evals[ix], rs, state);
    return rs;
}


int cServiceVar::noValue(QSqlQuery& q, int &state, int _st)
{
    qlonglong hbt = heartbeat(q, EX_ERROR);
    if (hbt != NULL_ID && hbt < lastLast.msecsTo(QDateTime::currentDateTime())) {
        _st = RS_UNREACHABLE;
        setId(_ixVarState, _st);
        clear(_ixServiceVarValue);
        update(q, false, updateMask);
        if (getBool(_sDelegateServiceState)) state = _st;
    }
    else {  // Nincs adat, türelmi idő nem járt le
        ;
    }
    return _st;
}

eTristate cServiceVar::checkIntValue(qlonglong val, qlonglong ft, const QString &_p1, const QString &_p2, bool _inverse, bool ifNo)
{
    if (ft == NULL_ID || ft == FT_NO) return bool2ts(ifNo);
    bool ok1 = true, ok2 = true;
    qlonglong p1 = 0, p2 = 0;
    eTristate r = TS_NULL;
    // required parameters
    switch (ft) {
    case FT_BOOLEAN:
        r = str2tristate(_p1, EX_IGNORE);
        if (r != TS_NULL) p1 = r;
        else              ok1 = false;
        break;
    case FT_EQUAL:
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toLongLong(&ok1);
        break;
    case FT_INTERVAL:
        p1 = _p1.toLongLong(&ok1);
        p2 = _p2.toLongLong(&ok2);
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(trUtf8("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(trUtf8("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_BOOLEAN:
            r = (val == 0) == (p1 == 0) ? TS_TRUE : TS_FALSE;
            break;
        case FT_EQUAL:
            r = (val == p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_LITLE:
            r = (val  < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (val  > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(trUtf8("Invalid filter %1(%2) for integer type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;
}

eTristate cServiceVar::checkRealValue(double val, qlonglong ft, const QString &_p1, const QString &_p2, bool _inverse, bool ifNo)
{
    if (ft == NULL_ID || ft == FT_NO) return bool2ts(ifNo);
    bool ok1 = true, ok2 = true;
    eTristate r = TS_NULL;
    double p1 = 0.0;
    double p2 = 0.0;
    // required parameters
    switch (ft) {
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toDouble(&ok1);
        break;
    case FT_INTERVAL:
        p1 = _p1.toDouble(&ok1);
        p2 = _p2.toDouble(&ok2);
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(trUtf8("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(trUtf8("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_LITLE:
            r = (val < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (val > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(trUtf8("Invalid filter %1(%2) for real numeric type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;
}

eTristate cServiceVar::checkIntervalValue(qlonglong val, qlonglong ft, const QString& _p1, const QString& _p2, bool _inverse, bool ifNo)
{
    if (ft == NULL_ID || ft == FT_NO) return bool2ts(ifNo);
    bool ok1 = true, ok2 = true;
    qlonglong p1 = 0, p2 = 0;
    eTristate r = TS_NULL;
    // required parameters
    switch (ft) {
    case FT_BOOLEAN:
        r = str2tristate(_p1, EX_IGNORE);
        if (r != TS_NULL) p1 = r;
        else              ok1 = false;
        break;
    case FT_EQUAL:
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toLongLong(&ok1);
        if (!ok1) {
            p1 = parseTimeInterval(_p1, &ok1);
        }
        break;
    case FT_INTERVAL:
        p1 = _p1.toLongLong(&ok1);
        p2 = _p2.toLongLong(&ok2);
        if (!ok1) {
            p1 = parseTimeInterval(_p1, &ok1);
        }
        if (!ok2) {
            p2 = parseTimeInterval(_p2, &ok2);
        }
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(trUtf8("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(trUtf8("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_BOOLEAN:
            r = (val == 0) == (p1 == 0) ? TS_TRUE : TS_FALSE;
            break;
        case FT_EQUAL:
            r = (val == p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_LITLE:
            r = (val  < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (val  > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(trUtf8("Invalid filter %1(%2) for interval type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;
}

eTristate cServiceVar::checkEnumValue(int ix, const QStringList& evals, qlonglong ft, const QString& _p1, const QString& _p2, bool _inverse)
{
    if (ft == NULL_ID || ft == FT_NO) return TS_FALSE;
    bool ok1 = true, ok2 = true;
    int p1 = 0, p2 = 0;
    eTristate r = TS_NULL;
    // required parameters
    switch (ft) {
    case FT_BOOLEAN:
        r = str2tristate(_p1, EX_IGNORE);
        if (r != TS_NULL) p1 = r;
        else              ok1 = false;
        break;
    case FT_EQUAL:
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toInt(&ok1);
        if (!ok1) {
            p1 = evals.indexOf(_p1);
            ok1 = p1 >= 0;
        }
        break;
    case FT_INTERVAL:
        p1 = _p1.toInt(&ok1);
        p2 = _p2.toInt(&ok2);
        if (!ok1) {
            p1 = evals.indexOf(_p1);
            ok1 = p1 >= 0;
        }
        if (!ok2) {
            p2 = evals.indexOf(_p2);
            ok2 = p2 >= 0;
        }
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(trUtf8("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(trUtf8("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_BOOLEAN:
            r = (ix == 0) == (p1 == 0) ? TS_TRUE : TS_FALSE;
            break;
        case FT_EQUAL:
            r = (ix == p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_LITLE:
            r = (ix  < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (ix  > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (ix > p1 && ix < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(trUtf8("Invalid filter %1(%2) for interval type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;

}

bool   cServiceVar::skeep() {
    --skeepCnt;
    bool r = skeepCnt > 0;
    if (!r) {
        skeepCnt = int(getId(_ixRarefaction));
    }
    else {
        PDEB(VERBOSE) << "Skeep" << endl;
    }
    return r;
}


cServiceVar * cServiceVar::serviceVar(QSqlQuery&__q, qlonglong hsid, const QString& name, eEx __ex)
{
    int i = serviceVars.indexOf(name, hsid);
    if (i >= 0) return serviceVars.at(i);
    cServiceVar *p = new cServiceVar();
    p->setName(name);
    p->setId(_sHostServiceId, hsid);
    if (1 != p->completion(__q)) {
        if (__ex != EX_IGNORE) EXCEPTION(EFOUND, -1, QString(QObject::trUtf8("Rekord azonosító : %1")).arg(hsid));
        pDelete(p);
    }
    else {
        serviceVars << p;
    } \
    return p;
}

cServiceVar * cServiceVar::serviceVar(QSqlQuery&__q, qlonglong svid, eEx __ex)
{
    int i = serviceVars.indexOf(svid);
    if (i >= 0) return serviceVars.at(i);
    cServiceVar *p = new cServiceVar();
    if (!p->fetchById(__q, svid)) {
        if (__ex != EX_IGNORE) EXCEPTION(EFOUND, -1, QString(QObject::trUtf8("Rekord azonosító : %1")).arg(svid));
        pDelete(p);
    }
    else {
        serviceVars << p;
    }
    return p;
}

qlonglong cServiceVar::heartbeat(QSqlQuery&__q, eEx __ex)
{
    qlonglong id = getId();
    if (NULL_ID == id) {
        if (__ex != EX_IGNORE) EXCEPTION(EOID, 0, identifying(false));
        return NULL_ID;
    }
    qlonglong hbt;
    if (heartbeats.contains(id)) {
        hbt = heartbeats[id];
    }
    else {
        id = getId(_sHostServiceId);
        cHostService hs;
        if (id == NULL_ID || !hs.fetchById(__q, id)) {
            if (__ex != EX_IGNORE) EXCEPTION(EOID, id, identifying(false));
            return NULL_ID;
        }
        hbt = hs.getId(_sHeartbeatTime);
        if (hbt <=  0) {
            id = hs.getId(_sServiceId);
            cService s;
            if (id == NULL_ID || !s.fetchById(__q, id)) {
                if (__ex != EX_IGNORE) EXCEPTION(EOID, id, hs.identifying(false));
                return NULL_ID;
            }
            qlonglong hbt = s.getId(_sHeartbeatTime);
            if (hbt <= 0) {
                bool f;
                f = (id = hs.getId(_sPrimeServiceId)) != NULL_ID
                  && s.fetchById(__q, id)
                  && (hbt = s.getId(_sHeartbeatTime)) > 0;
                f = f || (
                    (id = hs.getId(_sProtoServiceId)) != NULL_ID
                  && s.fetchById(__q, id)
                  && (hbt = s.getId(_sHeartbeatTime)) > 0
                  );
                if (!f) {
                    if (__ex >= EX_WARNING) EXCEPTION(EDATA, hbt, identifying(false));
                }
            }
        }
        heartbeats[getId()] = hbt;
    }
    int rarefaction = int(getId(_ixRarefaction));
    if (rarefaction > 1) hbt *= rarefaction;
    return hbt;
}

bool cServiceVar::initSkeepCnt(int& delayCnt)
{
    int rarefaction = int(getId(_ixRarefaction));
    if (rarefaction <= 1) return false;
    skeepCnt = (delayCnt % rarefaction) + 1;
    ++delayCnt;
    return true;
}

/* ---------------------------------------------------------------------------- */
// ERROR: _pFeatures inicializálás, törlés !!! (nem használlt class)
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

// ERROR: _pFeatures inicializálás, törlés !!! (nem használlt class)
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

/* ---------------------------------------------------------------------------- */

/// Reverse Polish Notation
double rpn_calc(double _v, const QString _expr, const cFeatures _f, QString& st)
{
    QStack<double>  stack;
    stack << _v;
    QRegExp sep(QString("\\s+"));
    QStringList tokens = _expr.split(sep);
    while (!tokens.isEmpty()) {
        QString token = tokens.takeFirst();
        int n = stack.size();
        char c = token[0].toLatin1();
        bool ok;
        if (token.size() == 1) {    // Character token
            static const char * ctokens = "+-*/";   // all
            static const char * binToks = "+-*/";   // binary
            if (nullptr == strchr(ctokens, c)) {
                st = QObject::trUtf8("One character token %1 unknown.").arg(token);
                return 0.0;
            }
            if (n < 2 && nullptr != strchr(binToks, c)) {
                st = QObject::trUtf8("A binary operator %1 expects two parameters.").arg(token);
                return 0.0;
            }
            switch (c) {
            case '+':   _v = stack.pop();   stack.top() += _v;  break;
            case '-':   _v = stack.pop();   stack.top() -= _v;  break;
            case '*':   _v = stack.pop();   stack.top() *= _v;  break;
            case '/':   _v = stack.pop();   stack.top() /= _v;  break;
            }
            continue;
        }
        if (c == '$') {     // variable/macro for features
            QString key = token.mid(1);
            QString val = _f.value(key);
            if (val.isEmpty()) {
                st = QObject::trUtf8("Unknown feature variable %1.").arg(key);
                return 0.0;
            }
            QStringList sl = val.split(sep);
            while (!sl.isEmpty()) tokens.prepend(sl.takeLast());
            continue;
        }
        _v = token.toDouble(&ok);
        if (ok) {   // numeric
            stack.push(_v);
            continue;
        }
        enum eTokens { T_NULL, UT_NEG, UT_DROP, UT_DUP, BT_SWAP };
#define UNARYTOK(s)     { #s, UT_##s, 1 }
#define BINARYTOK(s)    { #s, BT_##s, 2 }
        struct tTokens { QString name; int key; int ops; }
                       tokens[] = {
            UNARYTOK(NEG), UNARYTOK(DROP), UNARYTOK(DROP), UNARYTOK(DUP),
            BINARYTOK(SWAP),
            { "",  T_NULL, 0 }
        };
        tTokens *pTok;
        for (pTok = tokens; pTok->key != T_NULL; ++pTok) {
            if (pTok->name.compare(token, Qt::CaseInsensitive)) {
                break;
            }
        }
        if (pTok->key != T_NULL) {
            st = QObject::trUtf8("Invalid tokan %1.").arg(token);
            return 0.0;
        }
        if (pTok->ops < n) {
            st = QObject::trUtf8("There is not enough operand (%1 instead of just %2) for the %3 token.").arg(pTok->ops).arg(n).arg(token);
            return 0.0;
        }
        switch (pTok->key) {
        case UT_NEG:    stack.top() = - stack.top();    break;
        case UT_DROP:   stack.pop();                    break;
        case UT_DUP:    stack.push(stack.top());        break;
        case BT_SWAP:   _v = stack.pop(); std::swap(_v, stack.top()); stack.push(_v);    break;
        }
    }
    if (stack.isEmpty()) {
        st = QObject::trUtf8("No return value.");
        return 0.0;
    }
    return stack.top();
}
