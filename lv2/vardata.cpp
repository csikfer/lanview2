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
RECACHEDEF(cServiceVarType, srvartype)

/* ---------------------------------------------------------------------------- */
int cServiceVar::_ixFeatures         = NULL_IX;
int cServiceVar::_ixServiceVarTypeId = NULL_IX;
int cServiceVar::_ixServiceVarValue  = NULL_IX;
int cServiceVar::_ixStateMsg         = NULL_IX;
int cServiceVar::_ixVarState         = NULL_IX;
QBitArray                   cServiceVar::updateMask;
tRecordList<cServiceVar>    cServiceVar::serviceVars;
QMap<qlonglong, qlonglong>  cServiceVar::heartbeats;

cServiceVar::cServiceVar() : cRecord()
{
    pVarType = NULL;
    lastCount = 0;
    _set(cServiceVar::descr());
}
cServiceVar::cServiceVar(const cServiceVar& __o) : cRecord()
{
    pVarType  = __o.pVarType;
    lastCount = __o.lastCount;
    lastTime  = __o.lastTime;
    _cp(__o);
}

const cRecStaticDescr&  cServiceVar::descr() const
{
    if (initPDescr<cServiceVar>(_sServiceVars)) {
        _ixFeatures         = _descr_cServiceVar().toIndex(_sFeatures);
        _ixServiceVarTypeId = _descr_cServiceVar().toIndex(_sServiceVarTypeId);
        _ixServiceVarValue  = _descr_cServiceVar().toIndex(_sServiceVarValue);
        _ixStateMsg         = _descr_cServiceVar().toIndex(_sStateMsg);
        _ixVarState         = _descr_cServiceVar().toIndex(_sVarState);
        updateMask = _descr_cServiceVar().mask(_sLastTime, _sRawValue)
                   | _descr_cServiceVar().mask(_ixVarState, _ixServiceVarValue, _ixStateMsg);
    }
    return *_pRecordDescr;
}

CRECDEFD(cServiceVar)

void cServiceVar::clearToEnd()
{
    pVarType = NULL;
    lastCount = 0;
    lastTime = QDateTime();
}

void cServiceVar::toEnd()
{
    toEnd(_ixServiceVarTypeId);
}

bool cServiceVar::toEnd(int _ix)
{
    if (_ix == _ixServiceVarTypeId) {
        if (pVarType != NULL && getId(_ixServiceVarTypeId) != pVarType->getId()) pVarType = NULL;
        return true;
    }
    return false;
}

const cServiceVarType *cServiceVar::varType(QSqlQuery& q, eEx __ex)
{
    if (pVarType == NULL || pVarType->getId() != getId(_ixServiceVarTypeId)) {
        pVarType = cServiceVarType::srvartype(q, getId(_ixServiceVarTypeId), __ex);
    }
    return pVarType;
}

int cServiceVar::setValue(QSqlQuery& q, double val, int& state, QString *pMsg)
{
    preSetValue(QString::number(val));
    qlonglong svt = varType(q)->getId(_sServiceVarType);
    int r = RS_INVALID;
    switch (svt) {
    case SVT_ABSOLUTE:
        if (val < 0) {
            val = - val;
            addMsg("Negált érték.");
        }
        r = updateVar(q, val, state);
        break;
    case NULL_ID:
    case SVT_GAUGE:
        r = updateVar(q, val, state);
        break;
    case SVT_COUNTER:
    case SVT_DCOUNTER:
        r = setCounter(q, (qulonglong)(val + 0.5), (int)svt, state);
        break;
    case SVT_DERIVE:
    case SVT_DDERIVE:
         r = setDerive(q, val, state);
        break;
    case SVT_COMPUTE:
    default:
        EXCEPTION(EDATA, svt, identifying(false));
    }
    QString m = getName(_ixStateMsg);
    if (!m.isEmpty()) msgAppend(pMsg, trUtf8("Service variable %1 => '%2' ; %3 : \n").arg(val).arg(getName().arg(view(q, _ixVarState))) + m);
    return r;
}

int cServiceVar::setValue(QSqlQuery& q, qulonglong val, int &state, QString *pMsg)
{
    preSetValue(QString::number(val));
    qlonglong svt = varType(q)->getId(_sServiceVarType);
    int r = RS_INVALID;
    switch (svt) {
    case SVT_ABSOLUTE:
    case NULL_ID:
    case SVT_GAUGE:
        r = updateVar(q, val, state);
        break;
    case SVT_COUNTER:
    case SVT_DCOUNTER:
    case SVT_DERIVE:
    case SVT_DDERIVE:
         r = setCounter(q, val, (int)svt, state);
        break;
    case SVT_COMPUTE:
    default:
        EXCEPTION(EDATA, svt, identifying(false));
    }
    QString m = getName(_ixStateMsg);
    if (!m.isEmpty()) msgAppend(pMsg, trUtf8("Service variable %1 => '%2' ; %3 : \n").arg(val).arg(getName().arg(view(q, _ixVarState))) + m);
    return r;
}

int cServiceVar::setValue(QSqlQuery& q, qlonglong _hsid, const QString& _name, const QVariant& val)
{
    int state;  // Dummy
    cServiceVar *p = serviceVar(q, _hsid, _name);
    if (variantIsInteger(val)) {
        bool ok;
        qulonglong u = val.toULongLong(&ok);
        if (ok) return p->setValue(q, u, state);
        EXCEPTION(ENOTSUPP);
    }
    else if (variantIsFloat(val)) {
        return p->setValue(q, val.toDouble(), state);
    }
    EXCEPTION(ENOTSUPP);
    return RS_UNREACHABLE;
}

int cServiceVar::setValues(QSqlQuery& q, qlonglong _hsid, const QStringList& _names, const QVariantList& vals)
{
    int r = RS_ON;
    int n = _names.size();
    n = std::min(n, vals.size());
    for (int i = 0; i < n; ++i) {
        int rr = setValue(q, _hsid, _names.at(i), vals.at(i));
        if (r < rr) r = rr;
    }
    return r;
}

void cServiceVar::preSetValue(const QString& val)
{
    oldStateMsg = getName(_ixStateMsg);
    clear(_ixStateMsg);
    setName(_sRawValue, val);
    lastLast = get(_sLastTime).toDateTime();
    setName(_sLastTime, _sNOW);
}

int cServiceVar::setCounter(QSqlQuery& q, qulonglong val, int svt, int &state)
{
    QDateTime now = QDateTime::currentDateTime();
    if (!lastTime.isValid()) {
        lastCount = val;
        lastTime  = now;
        addMsg(trUtf8("Első érték, nincs elég adat."));
        return noValue(q, state);
    }
    qulonglong delta = 0;
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
        break;
    }
    lastCount = val;
    qulonglong dt = lastTime.secsTo(now);
    lastTime = now;
    double dVal = (double)delta / (double)dt;
    return updateVar(q, dVal, state);
}

int cServiceVar::setDerive(QSqlQuery &q, double val, int& state)
{
    QDateTime now = QDateTime::currentDateTime();
    if (!lastTime.isValid()) {
        lastValue = val;
        lastTime  = now;
        addMsg(trUtf8("Első érték, nincs elég adat."));
        return noValue(q, state);
    }
    double delta = val - lastValue;
    lastValue = val;
    qulonglong dt = lastTime.secsTo(now);
    lastTime = now;
    double dVal = delta / (double)dt;
    return updateVar(q, dVal, state);

}

int cServiceVar::updateVar(QSqlQuery& q, qulonglong val, int &state)
{
    if (TS_FALSE == checkIntValue(val, varType(q)->getId(_sPlausibilityType), varType(q)->get(_sPlausibilityParam1), varType(q)->get(_sPlausibilityParam2), varType(q)->getBool(_sPlausibilityInverse))) {
        addMsg(trUtf8("Az érték nem hihető."));
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkIntValue(val, varType(q)->getId(_sCriticalType), varType(q)->get(_sCriticalParam1), varType(q)->get(_sCriticalParam2), varType(q)->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkIntValue(val, varType(q)->getId(_sWarningType), varType(q)->get(_sWarningParam1), varType(q)->get(_sWarningParam2), varType(q)->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    if (getBool(_sDelegateServiceState) && state < rs) state = rs;
    setId(_ixVarState, rs);
    setName(_ixServiceVarValue, QString::number(val));
    update(q, false, updateMask);
    return rs;
}

int cServiceVar::updateVar(QSqlQuery& q, double val, int& state)
{
    if (TS_FALSE == checkRealValue(val, varType(q)->getId(_sPlausibilityType), varType(q)->get(_sPlausibilityParam1), varType(q)->get(_sPlausibilityParam2), varType(q)->getBool(_sPlausibilityInverse))) {
        addMsg(trUtf8("Az érték nem hihető."));
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkRealValue(val, varType(q)->getId(_sCriticalType), varType(q)->get(_sCriticalParam1), varType(q)->get(_sCriticalParam2), varType(q)->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkRealValue(val, varType(q)->getId(_sWarningType), varType(q)->get(_sWarningParam1), varType(q)->get(_sWarningParam2), varType(q)->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    if (getBool(_sDelegateServiceState) && state < rs) state = rs;
    setId(_ixVarState, rs);
    setName(_ixServiceVarValue, QString::number(val));
    update(q, false, updateMask);
    return rs;
}

int cServiceVar::noValue(QSqlQuery& q, int &state)
{
    qlonglong hbt = heartbeat(q, EX_ERROR);
    if (lastLast.isNull() || isNull(_ixServiceVarValue)
     || (hbt != NULL_ID && hbt < lastLast.msecsTo(QDateTime::currentDateTime()))) {
        setId(_ixVarState, RS_UNREACHABLE);
        clear(_ixServiceVarValue);
        update(q, false, updateMask);
        if (getBool(_sDelegateServiceState)) state = RS_UNREACHABLE;
    }
    else {  // Nincs adat, türelmi idő nem járt le, csak az eredeti üzenethez csapjuk hozzá a dátumot, és az okot.
        oldStateMsg += "\n" + QDateTime::currentDateTime().toString() + "\n";
        oldStateMsg += getName(_ixStateMsg);
        setName(_ixStateMsg, oldStateMsg);
        update(q, false, _bit(_ixStateMsg));
    }
    return RS_UNREACHABLE;
}

eTristate cServiceVar::checkIntValue(qulonglong val, qlonglong ft, const QVariant& _p1, const QVariant& _p2, bool _inverse)
{
    if (ft == NULL_ID) return TS_NULL;
    bool ok1, ok2;
    eTristate r = TS_NULL;
    qulonglong p1 = _p1.toULongLong(&ok1);
    qulonglong p2 = _p2.toULongLong(&ok2);
    switch (ft) {
    case FT_BOOLEAN:
        r = str2tristate(_p1.toString(), EX_IGNORE);
        if (r != TS_NULL) r = ((val == 0) == (r == TS_FALSE)) ? TS_TRUE : TS_FALSE;
        break;
    case FT_EQUAL:
        r = !ok1 ? TS_NULL : (val == p1) ? TS_TRUE : TS_FALSE;
        break;
    case FT_LITLE:
        r = !ok1 ? TS_NULL : (val < p1) ? TS_TRUE : TS_FALSE;
        break;
    case FT_BIG:
        r = !ok1 ? TS_NULL : (val > p1) ? TS_TRUE : TS_FALSE;
        break;
    case FT_INTERVAL:
        r = !(ok1 && ok2) ? TS_NULL : (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
        break;
    default:                EXCEPTION(EDATA, ft, identifying());
    }
    return _inverse ? inverse(r) : r;
}

eTristate cServiceVar::checkRealValue(qulonglong val, qlonglong ft, const QVariant& _p1, const QVariant& _p2, bool _inverse)
{
    if (ft == NULL_ID) return TS_NULL;
    bool ok1, ok2;
    eTristate r = TS_NULL;
    double p1 = _p1.toDouble(&ok1);
    double p2 = _p2.toDouble(&ok2);
    switch (ft) {
    case FT_EQUAL:
        r = !ok1 ? TS_NULL : (val = p1) ? TS_TRUE : TS_FALSE;
        break;
    case FT_LITLE:
        r = !ok1 ? TS_NULL : (val < p1) ? TS_TRUE : TS_FALSE;
        break;
    case FT_BIG:
        r = !ok1 ? TS_NULL : (val > p1) ? TS_TRUE : TS_FALSE;
        break;
    case FT_INTERVAL:
        r = !(ok1 && ok2) ? TS_NULL : (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
        break;
    default:                EXCEPTION(EDATA, ft, identifying());
    }
    return _inverse ? inverse(r) : r;
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
    if (heartbeats.contains(id)) return heartbeats[id];
    qlonglong hbt;
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
            f = (id = getId(_sPrimeServiceId)) != NULL_ID
              && s.fetchById(__q, id)
              && (hbt = s.getId(_sHeartbeatTime)) > 0;
            f = f || (
                (id = getId(_sProtoServiceId)) != NULL_ID
              && s.fetchById(__q, id)
              && (hbt = s.getId(_sHeartbeatTime)) > 0
              );
            if (!f) {
                if (__ex >= EX_WARNING) EXCEPTION(EDATA, hbt, identifying(false));
            }
        }
    }
    heartbeats[getId()] = hbt;
    return hbt;
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
