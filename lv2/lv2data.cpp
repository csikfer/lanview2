#include "lanview.h"
#include "lv2data.h"
#include "scan.h"
// #include "lv2service.h"
#include "others.h"

EXT_ const QString& notifSwitch(int _ns, bool __ex = true);
EXT_ int notifSwitch(const QString& _nm, bool __ex = true);


int touch(QSqlQuery& q, cRecord& o)
{
    int iLastTime = o.toIndex(_sLastTime);
    QBitArray bset  = o.mask(iLastTime);
    o.clear(iLastTime);
    QBitArray where = o.getSetMap();
    if ((where & o.primaryKey()) == o.primaryKey()) where = o.primaryKey();
    else foreach (QBitArray u, o.uniques()) {
        if ((where & u) == u) { where = u; break; }
    }

    QString sql = QString("UPDATE %1 SET last_time = NOW() ").arg(o.fullTableNameQ());
    sql += o.whereString(where);
    sql += " RETURNING *";
    o.query(q, sql, where, true);
    if (q.first()) {
        o.set(q.record());
        o._stat |= cRecord::ES_EXIST;
        return q.size();
    }
    o.set();
    return 0;
}

/* ------------------------------ tpows ------------------------------ */
DEFAULTCRECDEF(cTpow, _sTpows)
/* ------------------------------ timeperiods ------------------------------ */
DEFAULTCRECDEF(cTimePeriod, _sTimePeriods)
/* ------------------------------ images ------------------------------ */
CRECCNTR(cImage)
CRECDEFD(cImage)

int  cImage::_ixImageType = NULL_IX;
const cRecStaticDescr&  cImage::descr() const
{
    if (initPDescr<cImage>(_sImages)) {
        _ixImageType = _pRecordDescr->toIndex(_sImageType);
        CHKENUM(_ixImageType, imageType);
    }
    return *_pRecordDescr;
}

bool cImage::toEnd(int __i)
{
    if (__i == _ixImageType) {
        if (_isNull(__i)) return true;
        QVariant& t = _get(__i);
        bool    b;
        qlonglong i = t.toLongLong(&b);
        if (b) {
            QString s = imageType(i, false);
            if (s.isEmpty()) _stat = ES_DEFECTIVE;
            else t = s;
        }
        else {
            if (IT_INVALID == imageType(t.toString(), false)) _stat = ES_DEFECTIVE;
        }
        return true;
    }
    return false;
}

void cImage::toEnd()
{
    toEnd(_ixImageType);
}

bool cImage::load(const QString& __fn, bool __ex)
{
    QFile   f(__fn);
    if (!f.open(QIODevice::ReadOnly)) {
        if (__ex) EXCEPTION(ENOTFILE, -1, __fn);
        return false;
    }
    setImage(f.readAll());
    return true;
}

bool cImage::save(const QString& __fn, bool __ex)
{
    QFile   f(__fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (__ex) EXCEPTION(ENOTFILE, -1, __fn);
        return false;
    }
    f.write(getImage());
    return true;
}

int imageType(const QString& __n, bool __ex)
{
    if (__n == _sBMP) return IT_BMP;
    if (__n == _sGIF) return IT_GIF;
    if (__n == _sJPG) return IT_JPG;
    if (__n == _sJPEG)return IT_JPG;
    if (__n == _sPNG) return IT_PNG;
    if (__n == _sPBM) return IT_PBM;
    if (__n == _sPGM) return IT_PGM;
    if (__n == _sPPM) return IT_PPM;
    if (__n == _sXBM) return IT_XBM;
    if (__n == _sXPM) return IT_XPM;
    if (__n == _sBIN) return IT_BIN;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return IT_INVALID;
}

const QString&  imageType(int __e, bool __ex)
{
    switch (__e) {
    case IT_BMP:        return _sJPG;
    case IT_GIF:        return _sGIF;
    case IT_JPG:        return _sJPG;
    case IT_PNG:        return _sPNG;
    case IT_PBM:        return _sPBM;
    case IT_PGM:        return _sPGM;
    case IT_PPM:        return _sPPM;
    case IT_XBM:        return _sXBM;
    case IT_XPM:        return _sXPM;
    case IT_BIN:        return _sBIN;
    default:
        if (__ex) EXCEPTION(EDATA, __e);
    }
    return _sNul;
}

const char *   _imageType(int __e, bool __ex)
{
    switch (__e) {
    case IT_BMP:        return __sJPG;
    case IT_GIF:        return __sGIF;
    case IT_JPG:        return __sJPG;
    case IT_PNG:        return __sPNG;
    case IT_PBM:        return __sPBM;
    case IT_PGM:        return __sPGM;
    case IT_PPM:        return __sPPM;
    case IT_XBM:        return __sXBM;
    case IT_XPM:        return __sXPM;
    case IT_BIN:        return __sBIN;
    default:
        if (__ex) EXCEPTION(EDATA, __e);
    }
    return __sNul;
}


/* ------------------------------ places ------------------------------ */
DEFAULTCRECDEF(cPlace, _sPlaces)

DEFAULTCRECDEF(cPlaceGroup, _sPlaceGroups)

qlonglong cPlaceGroup::insertNew(const QString& __n, const QString& __d)
{
    cPlaceGroup o;
    o.setName(__n);
    o.setName(_sPlaceGroupDescr, __d);
    QSqlQuery q = getQuery();
    o.insert(q);
    return o.getId();
}

/* ------------------------------ subnets ------------------------------ */
CRECCNTR(cSubNet)
CRECDEFD(cSubNet)

int cSubNet::_ixNetAddr    = NULL_IX;
int cSubNet::_ixVlanId     = NULL_IX;
int cSubNet::_ixSubNetType = NULL_IX;
const cRecStaticDescr&  cSubNet::descr() const
{
    if (initPDescr<cSubNet>(_sSubNets)) {
        _ixNetAddr     = _pRecordDescr->toIndex(_sNetAddr);
        _ixVlanId      = _pRecordDescr->toIndex(_sVlanId);
        _ixSubNetType  = _pRecordDescr->toIndex(_sSubnetType);
        CHKENUM(_ixSubNetType, subNetType);
    }
    return *_pRecordDescr;
}

void cSubNet::toEnd()
{
    cSubNet::toEnd(_ixVlanId);
}

bool cSubNet::toEnd(int __i)
{
    if (_ixVlanId == __i) {
        if (!_isNull(__i) && getId(__i) < 1) _clear(__i);    // A nulla és negatív ID-k egyenértéküek a NULL-lal.
        return true;
    }
    //DBGFNL();
    return false;
}
void cSubNet::clearToEnd()
{
    ;
}

qlonglong cSubNet::insertNew(const QString& _n, const QString& _d, const netAddress& _a, qlonglong _v, const QString& __t)
{
    cSubNet no;
    no.setName(_n);
    no.set(_sSubNetDescr, _d);
    no = _a;
    if (_v > 0) no.set(_sVlanId, QVariant(_v));
    no.set(_sSubnetType, QVariant(__t));
    QSqlQuery   *pq = newQuery();
    no.insert(*pq);
    return no.get(cSubNet::_pRecordDescr->toIndex(_sSubNetId)).toInt();
}

int cSubNet::getByAddress(QSqlQuery & __q, const QHostAddress& __addr)
{
    QString where = QString(" %1 << inet '%2' ").arg(dQuoted(_sNetAddr), __addr.toString());
    fetchQuery(__q, false, QBitArray(1), tIntVector(), 0, 0, QString(), where);
    if (!__q.first()) return 0;
    set(__q.record());
    return __q.size();
}

const QString& subNetType(int __at, bool __ex)
{
    switch (__at) {
    case NT_PRIMARY:    return _sPrimary;
    case NT_SECONDARY:  return _sSecondary;
    case NT_PSEUDO:     return _sPseudo;
    case NT_PRIVATE:    return _sPrivate;
    default:  if (__ex) EXCEPTION(EDATA, __at);
    }
    return _sNULL;
}

int subNetType(const QString& __at, bool __ex)
{
    if (__at == _sPrimary)  return NT_PRIMARY;
    if (__at == _sSecondary)return NT_SECONDARY;
    if (__at == _sPseudo)   return NT_PSEUDO;
    if (__at == _sPrivate)  return NT_PRIVATE;
    if (__ex) EXCEPTION(EDATA,-1,__at);
    return NT_INVALID;
}

/* ------------------------------ vlans ------------------------------ */
DEFAULTCRECDEF(cVLan, _sVLans)

long cVLan::insertNew(long __i, const QString& __n, const QString& __d, bool __s)
{
    cVLan vo;
    vo.setId(__i);
    vo.setName(__n);
    vo.set(_sVlanDescr, QVariant(__d));
    vo.set(_sVlanStat, QVariant(__s));
    QSqlQuery   *pq = newQuery();
    vo.insert(*pq);
    delete pq;
    return __i;
}

/* ------------------------------ ipadresses ------------------------------ */

cIpAddress::cIpAddress() : cRecord()
{
    _set(cIpAddress::descr());
    _initIpAddress(*cIpAddress::_pRecordDescr);
}

cIpAddress::cIpAddress(const cIpAddress& __o) : cRecord()
{
    _initIpAddress(*cIpAddress::_pRecordDescr);
    __cp(__o);
    _copy(__o, *_pRecordDescr);
}

cIpAddress::cIpAddress(const QHostAddress& __a, const QString& __t)  : cRecord()
{
    _set(cIpAddress::descr());
    _initIpAddress(*cIpAddress::_pRecordDescr);
    _set(_ixAddress, QVariant::fromValue(netAddress(__a)));
    _set(cIpAddress::_pRecordDescr->toIndex(_sIpAddressType), QVariant(__t));
}

CRECDEFD(cIpAddress)

// -- virtual
const cRecStaticDescr&  cIpAddress::descr() const
{
    if (initPDescr<cIpAddress>(_sIpAddresses)) {
        CHKENUM(_sIpAddressType, addrType);
    }
    return *_pRecordDescr;
}

cIpAddress& cIpAddress::setAddress(const QHostAddress& __a, const QString& __t)
{
    if (__t.isEmpty() == false) {
        (void)addrType(__t);  // check
        setName(_ixIpAddressType, __t);
    }
    if (isNull(_ixIpAddressType)) setName(_ixIpAddressType, _sFixIp);
    set(_ixAddress, QVariant::fromValue(netAddress(__a)));
    return *this;
}

QHostAddress cIpAddress::address() const
{
    if (isNull(_ixAddress)) return QHostAddress();
    const netAddress na = get(_ixAddress).value<netAddress>();
    return na.addr();
}


bool cIpAddress::thisIsExternal(QSqlQuery& q)
{
    if (isNull(_ixAddress)) return false;
    const netAddress a = get(_ixAddress).value<netAddress>();
    cSubNet subNet;
    if (subNet.getByAddress(q, a.addr())) {
        _set(_ixSubNetId, subNet.getId());
        return false;
    }
    _set(_ixIpAddressType, QVariant(_sExternal));
    return true;
}

const QString& addrType(int __at, bool __ex)
{
    switch (__at) {
    case AT_FIXIP:  return _sFixIp;
    case AT_DYNAMIC:return _sDynamic;
    case AT_PSEUDO: return _sPseudo;
    case AT_PRIVATE:return _sPrivate;
    case AT_EXTERNAL:return _sExternal;
    default: if (__ex) EXCEPTION(EDATA, __at);
    }
    return _sNULL;
}

int addrType(const QString& __at, bool __ex)
{
    if (__at == _sFixIp)   return AT_FIXIP;
    if (__at == _sDynamic) return AT_DYNAMIC;
    if (__at == _sPseudo)  return AT_PSEUDO;
    if (__at == _sPrivate) return AT_PRIVATE;
    if (__at == _sExternal)return AT_EXTERNAL;
    if (__ex) EXCEPTION(EDATA, -1, __at);
    return AT_INVALID;
}

/* ******************************  ****************************** */
DEFAULTCRECDEF(cPortParam, _sPortParams)

long cPortParam::insertNew(const QString& __n, const QString& __de, const QString __t, const QString __di)
{
    cPortParam pp;
    pp.setName(__n);
    pp.set(_sPortParamDescr, __de);
    pp.set(_sPortParamType,  __t);
    pp.set(_sPortParamDim,   __di);
    QSqlQuery   *pq = newQuery();
    pp.insert(*pq);
    delete pq;
    return pp.getId();
}
/* ........................................................................ */

CRECCNTR(cPortParamValue)
CRECDEFD(cPortParamValue)

int              cPortParamValue::_ixPortParamId = -1;
const cRecStaticDescr&  cPortParamValue::descr() const
{
    if (initPDescr<cPortParamValue>(_sPortParamValues)) {
        _ixPortParamId = _pRecordDescr->toIndex(_sPortParamId);
    }
    return *_pRecordDescr;
}

void    cPortParamValue::toEnd()
{
    cPortParamValue::toEnd(_ixPortParamId);
}

bool cPortParamValue::toEnd(int i)
{
    if (i == _ixPortParamId) {
        qlonglong id = _isNull(i) ? NULL_ID : _get(i).toLongLong();
        if (id == NULL_ID) {
            portParam.clear();
        }
        else {
            QSqlQuery q = getQuery();
            portParam.fetchById(q, id);
        }
        return true;
    }
    return false;
}

void    cPortParamValue::clearToEnd()
{
    portParam.clear();
}

/* ........................................................................ */

cPortParamValues::cPortParamValues() : tRecordList<cPortParamValue>()
{
    if (_ixPortId < 0) {
        _ixPortId = cPortParamValue(_no_init_).descr().toIndex(_sPortId);
    }
}

cPortParamValues::cPortParamValues(const cPortParamValue& __v) : tRecordList<cPortParamValue>(__v)
{
    if (_ixPortId < 0) {
        _ixPortId = cPortParamValue(_no_init_).descr().toIndex(_sPortId);
    }
}

cPortParamValues::cPortParamValues(QSqlQuery& __q, qlonglong __port_id) : tRecordList<cPortParamValue>()
{
    if (_ixPortId < 0) {
        _ixPortId = cPortParamValue(_no_init_).descr().toIndex(_sPortId);
    }
    fetch(__q, __port_id);
}

cPortParamValues::cPortParamValues(const cPortParamValues& __o) : tRecordList<cPortParamValue>()
{
    *this = __o;
}

cPortParamValues& cPortParamValues::operator=(const cPortParamValues& __o)
{   // Az ős template osztály ugyanezt definiálja (más a visszaadott érték típusa)
    clear();
    const_iterator i;
    for (i = __o.constBegin(); i < __o.constEnd(); i++) {
        *this << new cPortParamValue(**i);
    }
    return *this;
}

int cPortParamValues::_ixPortId = -1;

const cPortParamValue& cPortParamValues::operator[](const QString& __n) const
{
    cPortParam pp;
    if (!pp.fetchByName(__n)) EXCEPTION(EFOUND, 1, __n);
    int i = indexOf(_sPortParamName, QVariant(__n));
    if (i < 0) EXCEPTION(EFOUND, 2, __n);
    return *at(i);
}

cPortParamValue&       cPortParamValues::operator[](const QString& __n)
{
    cPortParam pp;
    if (!pp.fetchByName(__n)) EXCEPTION(EFOUND, 1, __n);
    int i = indexOf(_sPortParamName, QVariant(__n));
    if (i < 0) {
        cPortParamValue * pr = new cPortParamValue();
        pr->setType(pp.getId());
        append(pr);
        return *pr;
    }
    return *at(i);
}

int       cPortParamValues::insert(QSqlQuery &__q, qlonglong __port_id, bool __ex)
{
    iterator i;
    for (i = begin(); i < end(); i++) {
        (*i)->set(_sPortId, QVariant(__port_id));
    }
    return tRecordList<cPortParamValue>::insert(__q, __ex);
}

/* ------------------------------ cIfType ------------------------------ */
DEFAULTCRECDEF(cIfType, _sIfTypes)

tRecordList<cIfType> cIfType::ifTypes;
cIfType *cIfType::pNull = NULL;

void cIfType::fetchIfTypes(QSqlQuery& __q)
{
    if (pNull == NULL) pNull = new cIfType();
    ifTypes.clear();
    cIfType *p = new cIfType();
    QBitArray   ba(1, false);
    ifTypes.fetch(__q, true, ba, p);
}

const cIfType& cIfType::ifType(const QString& __nm, bool __ex)
{
    checkIfTypes();
    int i = ifTypes.indexOf(_descr_cIfType().nameIndex(), QVariant(__nm));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, -1,"Invalid iftype name or program error.");
        return *pNull;
    }
    return *(ifTypes[i]);
}

const cIfType& cIfType::ifType(qlonglong __id, bool __ex)
{
    checkIfTypes();
    int i = ifTypes.indexOf(_descr_cIfType().idIndex(), QVariant(__id));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, -1,"Invalid iftype name or program error.");
        return *pNull;
    }
    return *(ifTypes[i]);
}

/* ------------------------------ cNPort ------------------------------ */

cNPort::cNPort() : cRecord()
{
    // _DBGFN() << VDEBPTR(this) << endl;
    _set(cNPort::descr());
    _initNPort(_descr_cNPort());
}

cNPort::cNPort(const cNPort& __o) : cRecord()
{
    _initNPort(_descr_cNPort());
    __cp(__o);
    _copy(__o, _descr_cNPort());
    params = __o.params;
}

CRECDEFD(cNPort)
CRECDDCR(cNPort, _sNPorts)
// -- virtual

bool cNPort::insert(QSqlQuery &__q, bool __ex)
{
    bool r = cRecord::insert(__q, __ex);
    if (r) {
        qlonglong id = getPortId();
        // PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << " new port_id = " << id << endl;
        cPortParamValues ps = params;
        return ps.insert(__q, id, __ex) == ps.size();
    }
    return false;
}

void cNPort::toEnd()
{
    toEnd(_ixPortId);
}

bool cNPort::toEnd(int i)
{
    if (_ixPortId == i) {
        params.clear();
        return true;
    }
    return false;
}

void cNPort::clearToEnd()
{
    params.clear();
}

cNPort * cNPort::newPort(const cIfType& __t, int __i)
{
    cNPort *r = NULL;
    QStringList portobjtypes = __t.get(_sIfTypeObjType).toStringList();
    if (portobjtypes.size() == 0) EXCEPTION(EDATA, 0, "port_obj_type is empty");
    QString portobjtype = portobjtypes[0];
    if (portobjtypes.size() > 1) {
        if (__i == -1) EXCEPTION(AMBIGUOUS);
        if (__i < 0 || __i >= portobjtypes.size()) EXCEPTION(ENOINDEX, __i);
        portobjtype = portobjtypes[__i];
    }
    else if (__i > 0) EXCEPTION(ENOINDEX, __i);
    // PDEB(VVERBOSE) <<  __PRETTY_FUNCTION__ << " allocated: " << portobjtype << endl;
    if      (portobjtype == _sNPort)      r = new cNPort();
    else if (portobjtype == _sInterface)  r = new cInterface();
    else if (portobjtype == _sIfaceAddr)  r = new cIfaceAddr();
    else                                  EXCEPTION(EDATA, __t.getId(), portobjtype);
    r->set(_sIfTypeId, __t.getId());
    return r;
}

bool cNPort::fetchPortByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id, bool __ex) {
    clear();
    set(_sNodeId,   QVariant(__node_id));
    set(_sPortName, __port_name);
    int n = completion(__q);
    if (__ex && n > 1) EXCEPTION(AMBIGUOUS, __node_id, __port_name);
    if (__ex && n < 1) EXCEPTION(EFOUND,    __node_id, __port_name);
    return n == 1;
}
qlonglong cNPort::getPortIdByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id, bool ex) const
{
    QString sql = QString("SELECT port_id FROM nports WHERE port_name = '%1' AND node_id = %2")
                  .arg(__port_name).arg(__node_id);
    // PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << " SQL : " << sql << endl;
    if (!__q.exec(sql)) {
        if (ex == false && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
        else                                                                    SQLQUERYERR(__q)
        return NULL_ID;
    }
    __q.first();
    QVariant id = __q.value(0);
    if (id.isNull()) {
        if (ex) EXCEPTION(EDATA, 1);
        return NULL_ID;
    }
    bool    ok;
    qlonglong i =  id.toLongLong(&ok);
    if (!ok) {
        if (ex) EXCEPTION(EDATA, 2, id.toString());
        return NULL_ID;
    }
    if (__q.next()) {
        if (ex) EXCEPTION(AMBIGUOUS, 2, id.toString());
        return NULL_ID;
    }
    return i;
}

bool cNPort::fetchPortByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, bool __ex)
{
    clear();
    qlonglong nid = cPatch().getIdByName(__q, __node_name, __ex);
    if (nid == NULL_ID) return false;
    setId(_sNodeId, nid);
    setName(_sPortName, __port_name);
    int n = completion(__q);
    if (__ex && n > 1) EXCEPTION(AMBIGUOUS, nid, __port_name);
    if (__ex && n < 1) EXCEPTION(EFOUND,    nid, __port_name);
    return n == 1;
}
qlonglong cNPort::getPortIdByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, bool ex) const
{
    QString sql = QString("SELECT port_id FROM nports JOIN patchs USING(node_id) WHERE port_name = '%1' AND node_name = '%2'")
                  .arg(__port_name).arg(__node_name);
    // PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << " SQL : " << sql << endl;
    if (!__q.exec(sql)) {
        if (ex == false && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
        else                                                                    SQLQUERYERR(__q)
        return NULL_ID;
    }
    __q.first();
    QVariant id = __q.value(0);
    if (id.isNull()) {
        if (ex) EXCEPTION(EDATA, 1);
        return NULL_ID;
    }
    bool    ok;
    qlonglong i =  id.toLongLong(&ok);
    if (!ok) {
        if (ex) EXCEPTION(EDATA, 2, id.toString());
        return NULL_ID;
    }
    if (__q.next()) {
        if (ex) EXCEPTION(AMBIGUOUS, 2, id.toString());
        return NULL_ID;
    }
    return i;
}

bool cNPort::fetchPortByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id, bool __ex)
{
    clear();
    set(_sNodeId,   QVariant(__node_id));
    set(_sPortIndex, QVariant(__port_index));
    int n = completion(__q);
    if (__ex && n > 1) EXCEPTION(AMBIGUOUS, __node_id, QString::number(__port_index));
    if (__ex && n < 1) EXCEPTION(EFOUND,    __node_id, QString::number(__port_index));
    return n == 1;
}

qlonglong cNPort::getPortIdByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id, bool ex) const
{
    QString sql = QString("SELECT port_id FROM nports WHERE port_index = %1 AND node_id = %2")
                  .arg(__port_index).arg(__node_id);
    // PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << " SQL : " << sql << endl;
    if (!__q.exec(sql)) {
        if (ex == false && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
        else                                                                    SQLQUERYERR(__q)
        return NULL_ID;
    }
    __q.first();
    QVariant id = __q.value(0);
    if (id.isNull()) {
        if (ex) EXCEPTION(EDATA, 1);
        return NULL_ID;
    }
    bool    ok;
    qlonglong i =  id.toLongLong(&ok);
    if (!ok) {
        if (ex) EXCEPTION(EDATA, 2, id.toString());
        return NULL_ID;
    }
    if (__q.next()) {
        if (ex) EXCEPTION(AMBIGUOUS, 2, id.toString());
        return NULL_ID;
    }
    return i;
}

template<class P> static inline P * getPortObjByIdT(QSqlQuery& q, qlonglong  __id, bool __ex)
{
    P *p = new P();
    p->setPortId(__id);
    if (p->completion(q) != 1) {
        delete p;
        if (__ex) EXCEPTION(EDATA);
        return NULL;
    }
    return p;
}

cNPort * cNPort::getPortObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __port_id, bool __ex)
{
    if      (__tableoid == cNPort().      tableoid()) return getPortObjByIdT<cNPort>       (q, __port_id, __ex);
    else if (__tableoid == cPPort().      tableoid()) return getPortObjByIdT<cPPort>       (q, __port_id, __ex);
    else if (__tableoid == cInterface().  tableoid()) return getPortObjByIdT<cInterface>   (q, __port_id, __ex);
    else if (__tableoid == cIfaceAddr().  tableoid()) return getPortObjByIdT<cIfaceAddr>   (q, __port_id, __ex);
    else if (__tableoid == cNode().       tableoid()) return getPortObjByIdT<cNPort>       (q, __port_id, __ex);
    else if (__tableoid == cHost().       tableoid()) return getPortObjByIdT<cIfaceAddr>   (q, __port_id, __ex);
    else if (__tableoid == cSnmpDevice(). tableoid()) return getPortObjByIdT<cIfaceAddr>   (q, __port_id, __ex);
    else                                            EXCEPTION(EDATA);
    return NULL;
}

cNPort * cNPort::getPortObjById(QSqlQuery& q, qlonglong __port_id, bool __ex)
{
    qlonglong tableoid = cNPort().setId(__port_id).fetchTableOId(q, __ex);
    if (tableoid < 0LL) return NULL;
    return getPortObjById(q, tableoid, __port_id, __ex);
}

/* ------------------------------ cPPort ------------------------------ */

cPPort::cPPort() : cNPort(_no_init_)
{
    //_DBGFN() << VDEBPTR(this) << endl;
    _set(cPPort::descr());
     setDefaultType();
}

cPPort::cPPort(const cPPort& __o) : cRecord(), cNPort(_no_init_)
{
    _cp(__o);
}

// -- virtual
int             cPPort::_ixIfTypeId  = NULL_IX;
int             cPPort::_ixPortIndex = NULL_IX;
qlonglong       cPPort::_ifTypePatch = NULL_ID;
const cRecStaticDescr&  cPPort::descr() const
{
    if (initPDescr<cPPort>(_sPPorts)) {
        _ixIfTypeId  = _pRecordDescr->toIndex(_sIfTypeId);
        _ifTypePatch = cIfType::ifTypeId(_sPatch);
        _ixPortIndex = _pRecordDescr->toIndex(_sPortIndex);
    }
    return *_pRecordDescr;
}
CRECDEFD(cPPort)

/* ------------------------------ cInterface ------------------------------ */

cInterface::cInterface() : cNPort(_no_init_), trunkMembers()
{
    _set(cInterface::descr());
    _initInterface(cInterface::_descr_cInterface());
}
cInterface::cInterface(const cInterface& __o) : cRecord(), cNPort(_no_init_), trunkMembers()
{
    _initInterface(_descr_cInterface());
    __cp(__o);
    _copy(__o, _descr_cInterface());
    params       = __o.params;
    trunkMembers = __o.trunkMembers;
    vlans        = __o.vlans;
}
// -- virtual
CRECDDCR(cInterface, _sInterfaces)

bool cInterface::insert(QSqlQuery &__q, bool __ex)
{
    if (!(cNPort::insert(__q, __ex) && (trunkMembers.size() == updateTrunkMembers(__q, __ex)))) return false;
    if (vlans.isEmpty()) return true;
    vlans.setId(cPortVlan::_ixPortId, getId());
    vlans.setName(cPortVlan::_ixSetType, _sManual);
    return vlans.size() == vlans.insert(__q, __ex);
}

void cInterface::toEnd()
{
    cNPort::toEnd();
    cInterface::toEnd(_ixPortId);
}

bool cInterface::toEnd(int i)
{
    if (cNPort::toEnd(i)) {
        if (i == _ixPortId && vlans.size()) {
            if (!(vlans.first()->isNull(cPortVlan::_ixPortId) || vlans.first()->getId(cPortVlan::_ixPortId) == getId(_ixPortId))) vlans.clear();
        }
        return true;
    }
    return false;
}

void cInterface::clearToEnd()
{
    vlans.clear();
}

CRECDEFD(cInterface)

int cInterface::updateTrunkMembers(QSqlQuery& q, bool __ex)
{
    if (trunkMembers.isEmpty()) return 0;
    int r = 0;
    qlonglong id = getId();
    if (id == NULL_ID) {
        if (isEmpty_() || 1 != completion(q)) EXCEPTION(EDATA);
        id = getId();
        if (id == NULL_ID) EXCEPTION(EPROGFAIL);
    }
    if (!isIfType(_sMultiplexor)) EXCEPTION(EDATA);
    cInterface  iface;
    iface.setId(_sNodeId, getId(_sNodeId));
    foreach (int ix, trunkMembers) {
        iface.setId(_sPortIndex, ix);
        iface.setId(_sPortStapleId, id);
        if (!iface.update(q, false, mask(_sPortStapleId), mask(_sNodeId, _sPortIndex),__ex)) {
            if (__ex) EXCEPTION(EDATA);
        }
        else r++;
    }
    return r;
}

void cInterface::joinVlan(qlonglong __id, enum eVlanType __t, enum eSetType __st)
{
    cPortVlan *pv = vlans.get(cPortVlan::_ixVlanId, __id, false);
    if (pv == NULL) vlans << (pv = new cPortVlan());
    pv->setVlanId(__id);
    pv->setVlanType(__t);
    pv->setSetType(__st);
    if (!isNull(_ixPortId)) pv->setPortId(getId(_ixPortId));
}

int cInterface::fetchByMac(QSqlQuery& q, const cMac& a)
{
    clear();
    set(_sHwAddress, QVariant::fromValue(a));
    return completion(q);
}

/* ------------------------------ cIfaceAddr ------------------------------ */

cIfaceAddr::cIfaceAddr() : cInterface(_no_init_), cIpAddress(_no_init_), addresses()
{
    _set(cIfaceAddr::descr());
    _initIfaceAddr(_descr_cIfaceAddr());
}
cIfaceAddr::cIfaceAddr(const cIfaceAddr& __o) : cRecord(), cNPort(_no_init_), cInterface(_no_init_), cIpAddress(_no_init_), addresses()
{
    _set(cIfaceAddr::descr());
    _initIfaceAddr(_descr_cIfaceAddr());
    __cp(__o);
    _copy(__o, _descr_cIfaceAddr());
    params       = __o.params;
    trunkMembers = __o.trunkMembers;
}
// -- virtual
CRECDDCR(cIfaceAddr, _sIfaceAddrs)
CRECDEFD(cIfaceAddr)

void cIfaceAddr::toEnd()
{
    cInterface::toEnd();
}

bool cIfaceAddr::toEnd(int i)
{
    return cInterface::toEnd(i);
}

void cIfaceAddr::clearToEnd()
{
    cInterface::toEnd();
}

bool cIfaceAddr::insert(QSqlQuery &__q, bool __ex)
{
    // DBGFN();
    bool r = cNPort::insert(__q, __ex);    // Beállítja a port_id -t is
    if (!r) return false;
    int i, n = addresses.size();
    //PDEB(VERBOSE) << "Alias addresses " << n << endl;
    if (n == 0) return true;
    qlonglong id = getId();
    if (id == NULL_ID) EXCEPTION(EPROGFAIL);
    for (i = 0; i < n && r; ++i) r = addresses[i]->setId(_sPortId, id).insert(__q, __ex);
    return r;
}

cIpAddress& cIfaceAddr::addIpAddress(const QHostAddress& __a, const QString& __t, const QString &__d)
{
    // _DBGFN() << " @(" << __a.toString() << ")" << endl;
    if (address() == __a || addresses.indexOf(cIpAddress::_ixAddress, __a.toString())) EXCEPTION(EDATA);
    cIpAddress *p = new cIpAddress();
    *p = __a;
    p->setName(_sIpAddressType, __t);
    p->setName(_sIpAddressDescr, __d);
    addresses <<  p;
    //PDEB(VERBOSE) << "Added : " << p->toString() << " size : " << addresses.size();
    return *p;
}

cIpAddress& cIfaceAddr::addIpAddress(const cIpAddress& __a) {
    _DBGFN() << " @(" << __a.toString() << ")" << endl;
    if (address() == __a.address() || addresses.indexOf(cIpAddress::_ixAddress, __a.get(_ixAddress))) EXCEPTION(EDATA);
    cIpAddress *p = new cIpAddress(__a);
    addresses << p;
    PDEB(VERBOSE) << "Added, size : " << addresses.size();
    return *p;
}

cIfaceAddr& cIfaceAddr::setAddress(const cMac& __mac, const QHostAddress& __a, const QString& __t, const QString &__d)
{
    cIpAddress::setAddress(__a, __t);
    set(_ixHwAddress, QVariant::fromValue(__mac));
    setName(_sIpAddressDescr, __d);
    return *this;
}

bool cIfaceAddr::fetchByAddr(QSqlQuery& q, const QHostAddress& a)
{
    clear();
    set(_sAddress, QVariant::fromValue(a));
    return completion(q);
}


/* ****************************** NODES (patch, nodes, hosts, snmphosts ****************************** */
/* ----------------------------- PATCHS : cPatch ----------------------------- */


cShareBack& cShareBack::set(int __a, int __b, int __ab, int __bb, bool __cd)
{
    a = __a;
    b = __b;
    ab = __ab;
    bb = __bb;
    if ((a == NULL_IX || (a == b || a == ab || a == bb))
     || (b != NULL_IX && (          b == ab || b == bb))
     || (ab!= NULL_IX &&                      ab == bb ))
        EXCEPTION(EDATA);
    cd = __cd;
    return *this;
}

bool cShareBack::operator==(const cShareBack& __o) const
{
    return (*this) == __o.a || (*this) == __o.b || (*this) == __o.ab || (*this) == __o.bb;
}

bool cShareBack::operator==(int __ix) const
{
    if (__ix == NULL_IX) return a == NULL_IX;
    return __ix == a || __ix == b || __ix == ab || __ix == bb;
}

/* */

cPatch::cPatch() : cRecord(), ports()
{
    //_DBGFN() << VDEBPTR(this) << endl;
    _set(cPatch::descr());
}

cPatch::cPatch(const cPatch& __o) : cRecord(), ports()
{
    _cp(__o);
    ports = __o.ports;
}

cPatch::cPatch(const QString& __name, const QString& __descr) : cRecord(), ports()
{
    _set(cPatch::descr());
    _set(_descr_cPatch().nameIndex(),  __name);
    _set(_descr_cPatch().descrIndex(), __descr);
}

CRECDDCR(cPatch, _sPatchs)
CRECDEFD(cPatch)

void cPatch::toEnd()
{
    toEnd(idIndex());
}

bool cPatch::toEnd(int i)
{
    if (i == idIndex()) {
        // Ha üres nem töröljük, mert minek,
        if (ports.isEmpty()
        // ha a prortoknál nincs kitöltve az ID, akkor is békénhagyjuk. (csak az első elemet vizsgáljuk
         || ports.first()->getId() == NULL_ID
        // ha stimmel a node_id akkor sem töröljük
         || ports.first()->getId(_sNodeId) == getId())
            return true;
        ports.clear();
        clearShares();
        return true;
    }
    return false;
}

void cPatch::clearToEnd()
{
    ports.clear();
    clearShares();
}

bool cPatch::insert(QSqlQuery &__q, bool __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (ports.count()) {
        ports.clearId();
        ports.setId(_sNodeId, getId());
        int i = ports.insert(__q, __ex);
        if (i != ports.count()) return false;
        updateShares(__q, false, __ex);
    }
    return true;
}

cPPort *cPatch::addPorts(const QString& __n, int __noff, int __from, int __to, int __off)
{
    cPPort *p = NULL;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(nameAndNumber(__n, i + __noff), _sNul, i + __off);
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

cPPort *cPatch::addPort(const QString& __name, const QString& __descr, int __ix)
{
    if (ports.count()) {
        if (0 <= ports.indexOf(__name))
            EXCEPTION(EDATA, -1, QString("Ilyen port név már létezik: %1").arg(__name));
        if (0 <= ports.indexOf(cPPort::_ixPortIndex, QVariant(__ix)))
            EXCEPTION(EDATA, __ix, QString("Ilyen port index már létezik."));
    }
    cPPort  *p = new cPPort();
    p->setName(__name);
    p->setId(cPPort::_ixPortIndex, __ix);
    p->setName(_sPortDescr, __descr);
    ports.append(p);
    return p;
}

cPPort *cPatch::portSet(const QString& __name, const QString& __fn, const QVariant& __v)
{
    int i = ports.indexOf(cPPort::_ixPortIndex, __name);
    if (i < 0) EXCEPTION(ENONAME, -1, QString(QObject::trUtf8("%1 port name not found.")).arg(__name));
    cPPort * p = ports[i];
    p->set(__fn, __v);
    return p;
}

cPPort *cPatch::portSet(int __ix, const QString& __fn, const QVariant& __v)
{
    int i = ports.indexOf(cPPort::_ixPortIndex, __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::trUtf8("Port index not found"));
    cPPort *p = ports[i];
    p->set(__fn, __v);
    return p;
}

cPPort *cPatch::portSet(int __ix, const QString& __fn, const QVariantList& __v)
{
    int ix = __ix;
    cPPort *p = NULL;
    foreach (const QVariant& v, __v) {
        p = portSet(ix, __fn, v);
        ++ix;
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

bool cPatch::setShare(int __a, int __ab, int __b, int __bb, bool __cd)
{
    // Csak létező portra lehet megosztást csinálni
    if ((__a  != NULL_IX && 0 > ports.indexOf(cPPort::_ixPortIndex, QVariant(__a)))
     || (__b  != NULL_IX && 0 > ports.indexOf(cPPort::_ixPortIndex, QVariant(__b)))
     || (__ab != NULL_IX && 0 > ports.indexOf(cPPort::_ixPortIndex, QVariant(__ab)))
     || (__bb != NULL_IX && 0 > ports.indexOf(cPPort::_ixPortIndex, QVariant(__bb)))) return false;
    // A konstruktor is ellenőriz, egy port csak egyszer mind nem lehet NULL_IX, ha mégis dob egy kizárást.
    cShareBack  s(__a, __b, __ab, __bb, __cd);
    // Egy port csak egy megosztásban szerepelhet
    if (shares.contains(s)) return false;
    shares << s;
    return true;
}
bool cPatch::updateShares(QSqlQuery& __q, bool __clr, bool __ex)
{
    if (__clr) {
        QString sql = "UPDATE pports SET shared_cable = DEFAULT, shared_port_id = DEFAULT WHERE port_id = " + QString::number(getId());
        if (__q.exec(sql)) {
            if (__ex == false && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
            else                                                                      SQLQUERYERR(__q)
            return false;
        }
    }
    if (shares.isEmpty()) return true;
    QBitArray um = cPPort::_descr().mask(_sSharedCable, _sSharedPortId);
    QBitArray wm = cPPort::_descr().primaryKey();
    QString ss;
    foreach (const cShareBack& s, shares) {
        cPPort *p = ports.get(cPPort::_ixPortIndex, QVariant(s.getA()));   // A bázis port A vagy AA megosztás
        qlonglong pid = p->getId();
        if (s.getAB() == NULL_IX) { // A
            if (!p->setName(_sSharedCable, _sA ).update(__q, true, um, wm, __ex)) return false;
        }
        else {                      // AA, AB / C
            if (!p->setName(_sSharedCable, _sAA).update(__q, true, um, wm, __ex)) return false;
            p = ports.get(cPPort::_ixPortIndex, QVariant(s.getAB()));
            ss = s.isCD() ? _sC : _sAB;
            if (!p->setName(_sSharedCable, ss).setId(_sSharedPortId, pid).update(__q, true, um, wm, __ex)) return false;
        }                           // B / BA / D
        if (s.getB() != NULL_IX) {
            p = ports.get(cPPort::_ixPortIndex, QVariant(s.getB()));
            ss = s.getBB() == NULL_IX ? _sB : s.isCD() ? _sD : _sBA;
            if (!p->setName(_sSharedCable, ss).setId(_sSharedPortId, pid).update(__q, true, um, wm, __ex)) return false;
        }
        if (s.getBB() != NULL_IX) { // BB
            p = ports.get(cPPort::_ixPortIndex, QVariant(s.getB()));
            if (!p->setName(_sSharedCable, _sBB).setId(_sSharedPortId, pid).update(__q, true, um, wm, __ex)) return false;
        }
    }
    return true;
}

/* ----------------------------- HUBS : cHub ----------------------------- */

cHub::cHub() : cRecord()
{
    //_DBGFN() << VDEBPTR(this) << endl;
    _set(cHub::descr());
}

cHub::cHub(const cHub& __o) : cRecord(), ports()
{
    _cp(__o);
    ports = __o.ports;
}
cHub::cHub(const QString& __name, const QString& __descr, eType __t) : cRecord(), ports()
{
    _set(cHub::descr());
    _set(_descr_cHub().nameIndex(),  __name);
    _set(_descr_cHub().descrIndex(), __descr);
    _set(_ixIsVirtual, QVariant((bool)(__t & T_VIRTUAL)));
    _set(_ixIsSwitch,  QVariant((bool)(__t & T_SWITCH)));
}

int cHub::_ixIsVirtual = NULL_IX;
int cHub::_ixIsSwitch  = NULL_IX;

const cRecStaticDescr&  cHub::descr() const
{
    if (initPDescr<cHub>(_sHubs)) {
        _ixIsVirtual = _descr_cHub().toIndex(_sIsVirtual);
        _ixIsSwitch  = _descr_cHub().toIndex(_sIsSwitch);
    }
    return *_pRecordDescr;
}

void cHub::toEnd()
{
    toEnd(idIndex());
}

bool cHub::toEnd(int i)
{
    if (i == idIndex()) {
        // Ha üres nem töröljük, mert minek,
        if (ports.isEmpty()
        // ha a portoknál nincs kitöltve az ID, akkor is békénhagyjuk. (csak az első elemet vizsgáljuk
         || ports.first()->getId() == NULL_ID
        // ha stimmel a node_id akkor sem töröljük
         || ports.first()->getId(_sNodeId) == getId())
            return true;
        ports.clear();
        return true;
    }
    return false;
}

void cHub::clearToEnd()
{
    ports.clear();
}

bool cHub::insert(QSqlQuery &__q, bool __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (ports.count()) {
        ports.clearId();
        ports.setId(_sNodeId, getId());
        int i = ports.insert(__q, __ex);
        return i == ports.count();
    }
    return true;
}

cNPort *cHub::addPorts(const QString& __n, int __noff, int __from, int __to, int __off)
{
    cNPort *p = NULL;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(nameAndNumber(__n, i + __noff), _sNul, i + __off);
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

cNPort *cHub::addPort(const QString& __name, const QString& __descr, int __ix)
{
    static int ixPortIndex = NULL_IX;   // Ez konstans, de nem tudjuk csak az adatbázisból inicializálni.
    if (ixPortIndex == NULL_IX) ixPortIndex = cNPort().toIndex(_sPortIndex);
    if (ports.count()) {
        if (0 <= ports.indexOf(__name))
            EXCEPTION(EDATA, -1, QString("Ilyen port név már létezik: %1").arg(__name));
        if (__ix != NULL_IX) {
            if (0 <= ports.indexOf(ixPortIndex, QVariant(__ix)))
                EXCEPTION(EDATA, __ix, QString("Ilyen port index már létezik."));
        }
    }
    cNPort  *p = new cNPort();
    p->setName(__name);
    if (__ix != NULL_IX) p->setId(ixPortIndex, __ix);
    p->setName(_sPortDescr, __descr);
    p->setId(_sIfTypeId, cIfType::ifTypeId(getBool(_ixIsSwitch) ? _sEPort : _sBus));
    ports.append(p);
    return p;
}

cNPort *cHub::portSet(const QString& __name, const QString& __fn, const QVariant& __v)
{
    int i = ports.indexOf(__name);
    if (i < 0) EXCEPTION(ENONAME, -1, QString(QObject::trUtf8("%1 port name not found.")).arg(__name));
    cNPort *p = ports[i];
    p->set(__fn, __v);
    return p;
}

cNPort *cHub::portSet(int __ix, const QString& __fn, const QVariant& __v)
{
    static int ixPortIndex = NULL_IX;   // Ez konstans, de nem tudjuk csak az adatbázisból inicializálni.
    if (ixPortIndex == NULL_IX) ixPortIndex = cNPort().toIndex(_sPortIndex);
    int i = ports.indexOf(ixPortIndex, __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::trUtf8("Port index not found"));
    cNPort * p = ports[i];
    p->set(__fn, __v);
    return p;
}

cNPort *cHub::portSet(int __ix, const QString& __fn, const QVariantList& __v)
{
    cNPort *p = NULL;
    int ix = __ix;
    foreach (const QVariant& v, __v) {
        p = portSet(ix, __fn, v);
        ++ix;
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

CRECDEFD(cHub)
/* ------------------------------ NODES : cNode ------------------------------ */

cNode::cNode() : cRecord(), cNPort(_no_init_), ports(), _mainPortType(_sNPort)
{
    _set(cNode::descr());
    _initNode(_descr_cNode());
}

cNode::cNode(const cNode& __o) : cRecord(), cNPort(_no_init_), ports(), _mainPortType()
{
    _initNode(_descr_cNode());
    __cp(__o);
    _copy(__o, _descr_cNode());
    _mainPortType = __o._mainPortType; // !!!
    if (_mainPortType != _sNPort) EXCEPTION(EDATA, -1, _mainPortType);
    ports         = __o.ports;
    params        = __o.params;
}

cNode::cNode(const QString& __name, const QString& __descr) : cRecord(), cNPort(_no_init_), ports(), _mainPortType(_sNPort)
{
    _set(cNode::descr());
    _initNode(_descr_cNode());
    _set(_descr_cNode().nameIndex(),  __name);
    _set(_descr_cNode().descrIndex(), __descr);
}

cNode& cNode::operator=(const cNode& __o)
{
    __cp(__o);
    _copy(__o, _descr_cNode());
    _mainPortType = __o._mainPortType; // !!!
    if (_mainPortType != _sNPort) _stat |= ES_DEFECTIVE;
    ports         = __o.ports;
    params        = __o.params;
    return *this;
}

// -- virtual
const cRecStaticDescr&  cNode::descr() const
{
    if (initPDescr<cNode>(_sNodes)) {
        CHKENUM(_sNodeStat, notifSwitch);
    }
    return *_pRecordDescr;
}
bool cNode::insert(QSqlQuery &__q, bool __ex)
{
    return cNPort::insert(__q, __ex);

    if (!cNPort::insert(__q, __ex)) return false;
    if (ports.count()) {
        ports.clearId();
        ports.setId(_sNodeId, getId());
        int i = ports.insert(__q, __ex);
        return i == ports.count();
    }
    return true;
}

void cNode::toEnd_NoMainPort()
{
    toEnd(idIndex());
}

void cNode::toEnd()
{
    cNPort::toEnd();
    toEnd_NoMainPort();
}

bool cNode::toEnd_NoMainPort(int i)
{
    if (i == idIndex()) {
        // Ha üres nem töröljük, mert minek,
        if (ports.isEmpty()
        // ha a prortoknál nincs kitöltve az ID, akkor is békénhagyjuk. (csak az első elemet vizsgáljuk
         || ports.first()->getId() == NULL_ID
        // ha stimmel a node_id akkor sem töröljük
         || ports.first()->getId(_sNodeId) == getId())
            return true;
        ports.clear();
        return true;
    }
    return false;
}

bool cNode::toEnd(int i)
{
    if (cNPort::toEnd(i)) return true;
    return toEnd_NoMainPort(i);
}

void cNode::clearToEnd()
{
    cNPort::clearToEnd();
    ports.clear();
}

CRECDEFD(cNode)

int  cNode::fetchPorts(QSqlQuery& __q, bool __ex) {
    QSqlQuery q = getQuery(); // A copy construktor vagy másolás az nem jó!!
    qlonglong myPortId = getPortId();
    // Összeszedjük, a port ID-ket, és hogy milyen objektum típusok (tableoid), a benfoglalt portot kihagyjuk
    QString sql = QString("SELECT tableoid, port_id FROM nports WHERE node_id = %1 AND port_id <> %2").arg(getId()).arg(myPortId);
    if (!__q.exec(sql)) {
        if (__ex == false && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
        else                                                                      SQLQUERYERR(__q)
        return -1;
    }

    if (__q.first()) do {
        bool ok;
        qlonglong tableoid = __q.value(0).toLongLong(&ok);
        if (!ok) EXCEPTION(EDATA, -1, QString(QObject::trUtf8("Invalid table OID : %1")).arg(__q.value(0).toString()));
        qlonglong port_id  = __q.value(1).toLongLong(&ok);
        if (!ok) EXCEPTION(EDATA, -1, QString(QObject::trUtf8("Invalid port_id : %1")).arg(__q.value(1).toString()));
        cNPort *p = cNPort::getPortObjById(q, tableoid, port_id, __ex);
        if (p == NULL) return -1;
        ports << p;
    } while (__q.next());
    return ports.count();
}

cNPort *cNode::addPorts(const cIfType& __t, const QString& __np, int __noff, int __from, int __to, int __off)
{
    cNPort *p = NULL;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(__t, 0, nameAndNumber(__np, i + __noff), _sNul, i + __off);
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

cNPort *cNode::addPort(const cIfType& __t, int n, const QString& __name, const QString& __descr, int __ix)
{
    if (ports.count()) {
        if (NULL != getPort(__name, false)) EXCEPTION(EDATA, -1, QString("Ilyen port név már létezik: %1").arg(__name));
        if (NULL != getPort(__ix,   false)) EXCEPTION(EDATA, __ix, QString("Ilyen port index már létezik."));
    }
    cNPort  *p = cNPort::newPort(__t, n);
    p->setName(__name);
    if (__ix != NULL_IX) p->setId(_sPortIndex, __ix);
    p->setName(_sPortDescr, __descr);
    ports.append(p);
    return p;
}

cNPort *cNode::portSet(const QString& __name,  const QString& __fn, const QVariant& __v)
{
    if (__fn == _sNodeId || !cNPort::descr().isFieldName(__fn)) EXCEPTION(ENOFIELD, -1, __fn);
    cNPort *pp = getPort(__name);
    pp->set(__fn, __v);
    return pp;
}

cNPort *cNode::portSet(int __ix, const QString& __fn, const QVariant& __v)
{
    if (__fn == _sNodeId || !cNPort::descr().isFieldName(__fn)) EXCEPTION(ENOFIELD, -1, __fn);
    cNPort *pp = getPort(__ix);
    pp->set(__fn, __v);
    return pp;
}

cNPort *cNode::portSet(int __ix, const QString& __fn, const QVariantList& __v)
{
    cNPort *p = NULL;
    int ix = __ix;
    foreach (const QVariant& v, __v) {
        p = portSet(ix, __fn, v);
        ++ix;
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

bool cNode::setMainPortType(const cIfType& __t, bool __ex)
{
    QStringList tl = __t.get(_sIfTypeObjType).toStringList();
    if (tl.empty()) {
        if (__ex) return false;
        EXCEPTION(EDATA, __t.getId(), QObject::trUtf8("Invalid ifType object, empty object type list."));
    }
    foreach (const QString& s, tl) {
        if (_mainPortType == s) {
            setId(_sIfTypeId, __t.getId());
            return true;
        }
    }
    if (__ex) EXCEPTION(EDATA, __t.getId(), QObject::trUtf8("SET invalid ifType object."));
    return false;
}

cNPort *cNode::portSetParam(const QString& __name, const QString& __par, const QString& __val)
{
    cNPort *p = getPort(__name);
    p->params[__par] = __val;
    return p;
}

cNPort *cNode::portSetParam(int __ix, const QString& __par, const QString& __val)
{
    cNPort *p = getPort(__ix);
    p->params[__par] = __val;
    return p;
}

cNPort *cNode::portSetParam(int __ix, const QString& __par, const QStringList& __val)
{
    cNPort *p = NULL;
    int ix = __ix;
    foreach (const QString& v, __val) {
        p = portSetParam(ix, __par, v);
        ++ix;
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

cNPort * cNode::getPort(int __ix, bool __ex)
{
    if (__ix != NULL_IX) {
        if (__ix == getId(_sPortIndex)) return this;
        int i = ports.indexOf(_sPortIndex, __ix);
        if (i >= 0) return ports[i];
    }
    if (__ex) EXCEPTION(ENONAME, -1, QString(QObject::trUtf8("%1 port index not found.")).arg(__ix))
    return NULL;
}

cNPort * cNode::getPort(const QString& __pn, bool __ex)
{
    if (__pn == getName(_sPortName)) return this;
    int i = ports.indexOf(_sPortName, __pn);
    if (i >= 0) return ports[i];
    if (__ex) EXCEPTION(ENONAME, -1, QString(QObject::trUtf8("%1 port name not found.")).arg(__pn));
    return NULL;
}

template<class P> static inline P * getNodeObjByIdT(QSqlQuery& q, qlonglong  __id, bool __ex)
{
    P *p = new P();
    p->setId(__id);
    if (p->completion(q) != 1) {
        delete p;
        if (__ex) EXCEPTION(EDATA);
        return NULL;
    }
    return p;
}

cNode * cNode::getNodeObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __node_id, bool __ex)
{
    if      (__tableoid == cNode().       tableoid()) return getNodeObjByIdT<cNode>      (q, __node_id, __ex);
    else if (__tableoid == cHost().       tableoid()) return getNodeObjByIdT<cHost>      (q, __node_id, __ex);
    else if (__tableoid == cSnmpDevice(). tableoid()) return getNodeObjByIdT<cSnmpDevice>(q, __node_id, __ex);
    else                                              EXCEPTION(EDATA);
    return NULL;
}

cNode * cNode::getNodeObjById(QSqlQuery& q, qlonglong __node_id, bool __ex)
{
    qlonglong tableoid = cNode().setId(__node_id).fetchTableOId(q, __ex);
    if (tableoid < 0LL) return NULL;
    return getNodeObjById(q, tableoid, __node_id, __ex);
}

// --
/* ------------------------------ HOSTS : cHost ------------------------------ */

cHost::cHost() : cNode(_no_init_), cIfaceAddr(_no_init_)
{
    _set(cHost::descr());
    _initHost(_descr_cHost());
}

cHost::cHost(const cHost& __o) : cRecord(), cNPort(), cNode(_no_init_), cIfaceAddr(_no_init_)
{
    _set(cHost::descr());
    _initHost(_descr_cHost());
    __cp(__o);
    _copy(__o, _descr_cHost());
    _mainPortType = __o._mainPortType; // !!!
    if (_mainPortType != _sIfaceAddr)  _stat |= ES_DEFECTIVE;
    ports         = __o.ports;
    params        = __o.params;
    vlans         = __o.vlans;
    trunkMembers  = __o.trunkMembers;
}

cHost::cHost(const QString& __n, const QString &__d)
 : cRecord(), cNPort(), cNode(_no_init_), cIfaceAddr(_no_init_)
{
    _set(cHost::descr());
    _initHost(_descr_cHost());
    _set(_descr_cHost().nameIndex(),  __n);
    _set(_descr_cHost().descrIndex(), __d);
}

cHost& cHost::operator=(const cHost& __o)
{
    set();
    __cp(__o);
    _copy(__o, _descr_cHost());
    _mainPortType = __o._mainPortType; // !!!
    if (_mainPortType != _sIfaceAddr) EXCEPTION(EDATA, -1, _mainPortType);
    ports         = __o.ports;
    params        = __o.params;
    vlans         = __o.vlans;
    trunkMembers  = __o.trunkMembers;
    return *this;
}

// -- virtual
CRECDDCR(cHost, _sHosts)
CRECDEFD(cHost)

bool cHost::insert(QSqlQuery &__q, bool __ex)
{
    if (!cIfaceAddr::insert(__q, __ex)) return false;
    if (ports.count()) {
        ports.clearId();
        ports.setId(_sNodeId, getId());
        int i = ports.insert(__q, __ex);
        return i == ports.count();
    }
    return true;
}
void cHost::toEnd()
{
    cNode::toEnd_NoMainPort();
    cIfaceAddr::toEnd();
}

bool cHost::toEnd(int i)
{
    if (cNode::toEnd_NoMainPort(i)) return true;
    if (cIfaceAddr::toEnd(i)) return true;
    return false;
}

void cHost::clearToEnd()
{
    cNode::clearToEnd();
    cIfaceAddr::clearToEnd();
}

cNPort *cHost::portSet(const QString& __name,  const QString& __fn, const QVariant& __v)
{
    if (__fn == _sNodeId || !cIfaceAddr::descr().isFieldName(__fn)) EXCEPTION(ENOFIELD, -1, __fn);
    cNPort *pp = getPort(__name);
    pp->set(__fn, __v);
    return pp;
}

cNPort *cHost::portSet(int __ix, const QString& __fn, const QVariant& __v)
{
    if (__fn == _sNodeId || !cIfaceAddr::descr().isFieldName(__fn)) EXCEPTION(ENOFIELD, -1, __fn);
    cNPort *pp = getPort(__ix);
    pp->set(__fn, __v);
    return pp;
}

// --

bool cHost::fetchSelf(QSqlQuery& q, bool __ex)
{
    QString name = getEnvVar("HOSTNAME");
    if (!name.isEmpty()) {
        setName(name);
        if (fetch(q, false, mask(_sNodeName))) return true;
    }
    QList<QNetworkInterface> ii = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface i, ii) {
        cMac m = i.hardwareAddress();
        if (!m) continue;
        PDEB(VVERBOSE) << "My MAC : " << m.toString() << endl;
        cInterface in;
        in.set(_sHwAddress, QVariant::fromValue(m));
        if (in.completion(q)) {
            clear();
            setId(in.getId(_sNodeId));
            if (1 != completion(q)) {
                if (__ex) { EXCEPTION(EDATA); }
                else      { continue; }
            }
            return true;
        }
    }
    QList<QHostAddress>  aa = QNetworkInterface::allAddresses();
    foreach (QHostAddress a, aa) {
        PDEB(VVERBOSE) << "My adress : " << a.toString() << endl;
        if (a.isNull() || a.isLoopback()) continue;
        set(_sAddress, QVariant::fromValue(a));
        if (fetch(q, false, mask(_sAddress))) return true;
    }
    if (__ex) EXCEPTION(EFOUND, -1, QObject::trUtf8("Self host record"));
    return false;
}

cNPort *cHost::addSensors(const QString& __np, int __noff, int __from, int __to, int __off, const QHostAddress& __pip)
{
    const cIfType&  t = cIfType::ifType(_sSensor);
    QStringList a = __pip.toString().split(QChar('.'));
    if (a.size() != 4) EXCEPTION(EDATA);
    bool ok;
    int d = a[3].toInt(&ok);
    if (!ok) EXCEPTION(EDATA);
    cNPort *p = NULL;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(t, 1, nameAndNumber(__np, i + __noff), _sNul, i + __off);
        a[3] = QString::number(d++);
        p->setName(_sAddress, a.join(_sPoint));
        p->setName(_sIpAddressType, _sPseudo);
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}


cHost& cHost::portSetAddress(const QString& __port, const cMac& __mac, const QHostAddress& __a, const QString& __t, const QString &__d)
{
    ports.get(_sPortName, __port)->reconvert<cIfaceAddr>()->setAddress(__mac, __a, __t, __d);
    return *this;
}

cHost& cHost::portSetAddress(int __port_index, const cMac& __mac, const QHostAddress& __a, const QString& __t, const QString &__d)
{
    ports.get(_sPortIndex, __port_index)->reconvert<cIfaceAddr>()->setAddress(__mac, __a, __t, __d);
    return *this;
}


cIfaceAddr *cHost::portAddAddress(const QString& __port, const QHostAddress& __a, const QString& __t, const QString &__d)
{
    cIfaceAddr * p = getPort(__port)->reconvert<cIfaceAddr>();
    p->addIpAddress(__a, __t, __d);
    return p;
}

cIfaceAddr *cHost::portAddAddress(int __port_index, const QHostAddress& __a, const QString& __t, const QString &__d)
{
    cIfaceAddr *p = getPort(__port_index)->reconvert<cIfaceAddr>();
    p->addIpAddress(__a, __t, __d);
    return p;
}

cInterface *cHost::portSetVlan(const QString& __port, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st)
{
    cInterface *p = getPort(__port)->reconvert<cInterface>();
    p->joinVlan(__vlanId, __t, __st);
    return p;
}

cInterface * cHost::portSetVlan(int __port_index, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st)
{
    cInterface *p = getPort(__port_index)->reconvert<cInterface>();
    p->joinVlan(__vlanId, __t, __st);
    return p;
}

cInterface *cHost::portSetVlans(const QString& __port, const QList<qlonglong>& _ids)
{
    cInterface *p = getPort(__port)->reconvert<cInterface>();
    enum eVlanType t = VT_UNTAGGED;
    foreach(qlonglong id, _ids) {
        p->joinVlan(id, t, ST_MANUAL);
        t = VT_TAGGED;
    }
    return p;
}

cInterface *cHost::portSetVlans(int __port_index, const QList<qlonglong>& _ids)
{
    cInterface *p = getPort(__port_index)->reconvert<cInterface>();
    enum eVlanType t = VT_UNTAGGED;
    foreach(qlonglong id, _ids) {
        p->joinVlan(id, t, ST_MANUAL);
        t = VT_TAGGED;
    }
    return p;
}

bool cHost::fetchByAddr(QSqlQuery& q, const QHostAddress& a)
{
    clear();
    cIfaceAddr  ia;
    if (0 == ia.fetchByAddr(q, a)) return 0;
    setId(ia.getId(_sNodeId));
    return completion(q);
}

int cHost::fetchByMac(QSqlQuery& q, const cMac& a)
{
    clear();
    cInterface  ia;
    if (0 == ia.fetchByMac(q, a)) return 0;
    setId(ia.getId(_sNodeId));
    return completion(q);
}


/* ------------------------------ SNMPDEVICES : cSnmpDevice -----------setMainPort(ifType($3), $4, $5);------------------- */

cSnmpDevice::cSnmpDevice() : cHost(_no_init_)
{
    _set(cSnmpDevice::descr());
    _initHost(_descr_cSnmpDevice());
}

cSnmpDevice::cSnmpDevice(const cSnmpDevice& __o) : cRecord(), cNPort(), cHost(_no_init_)
{
    _set(cSnmpDevice::descr());
    _initHost(_descr_cSnmpDevice());
    __cp(__o);
    _copy(__o, _descr_cSnmpDevice());
    _mainPortType = __o._mainPortType; // !!!
    if (_mainPortType != _sIfaceAddr) _stat |= ES_DEFECTIVE;
    ports         = __o.ports;
    params        = __o.params;
    trunkMembers  = __o.trunkMembers;
}

cSnmpDevice::cSnmpDevice(const QString& __n, const QString &__d)
 : cRecord(), cNPort(), cHost(_no_init_)
{
    _set(cSnmpDevice::descr());
    _initHost(_descr_cSnmpDevice());
    _set(_descr_cSnmpDevice().nameIndex(),  __n);
    _set(_descr_cSnmpDevice().descrIndex(), __d);
}

// -- virtual
CRECDDCR(cSnmpDevice, _sSnmpDevices)
CRECDEFD(cSnmpDevice)

cNPort *cSnmpDevice::addPort(const cIfType& __t, int n, const QString& __name, const QString& __descr, int __ix)
{
    if (__ix < 0) EXCEPTION(EDATA, __ix);
    return cNode::addPort(__t, n, __name, __descr, __ix);
}

int cSnmpDevice::snmpVersion() const
{
    QString v = getName(_sSnmpVer);
    if (v.isEmpty()) return SNMP_VERSION_2c;
    if (v == "1")    return SNMP_VERSION_1;
    if (v == "2c")   return SNMP_VERSION_2c;
    if (v == "3")    return SNMP_VERSION_3;
    EXCEPTION(EDATA);
    return -1;  // Inactive
}

bool cSnmpDevice::setBySnmp(const QString& __com, bool __ex)
{
#ifdef MUST_SCAN
    QString community = __com;
    if (community.isEmpty()) {
        community = getName(_sCommunityRd);
        if (community.isEmpty()) {
            community = getName(_sCommunityWr);
            if (community.isEmpty()) community = _sPublic;
        }
    }
    setName(_sCommunityRd, community);
    // if (!assAddr()) EXCEPTION(EFOUND, -1, _sAddress);
    return getSysBySnmp(*this) && getPortsBySnmp(*this, __ex);
#else // MUST_SCAN
    (void)__com;
    if (__ex) EXCEPTION(ENOTSUPP, -1, "A snmp modul nincs engedélyezve");
    return false;
#endif // MUST_SCAN
}

int cSnmpDevice::open(cSnmp& snmp, bool __ex) const
{
#ifdef MUST_SCAN
    QString addr = getName(_sAddress);
    QString comn = getName(_sCommunityRd);
    int     ver  = snmpVersion();
    int r = snmp.open(addr, comn, ver);
    if (__ex && r) EXCEPTION(ESNMP, r, snmp.emsg);
    return r;
#else // MUST_SCAN
    (void)__ex;
    (void)snmp;
    if (__ex) EXCEPTION(ENOTSUPP, -1, "A snmp modul nincs engedélyezve");
    return -1;
#endif // MUST_SCAN
}

// --
/* ******************************  ****************************** */
DEFAULTCRECDEF(cIpProtocol, _sIpProtocols)
/* -------------------------------------------------------------- */
template void _SplitMagicT<cService>(cService& o, bool __ex);
template void _SplitMagicT<cHostService>(cHostService& o, bool __ex);

template void _Magic2PropT<cService>(cService& o);
template void _Magic2PropT<cHostService>(cHostService& o);
/* .............................................................. */
cService::cService() : cRecord(), _protocol()
{
    _pMagicMap = NULL;
    _set(cService::descr());
}

cService::cService(const cService& __o) : cRecord(), _protocol(__o._protocol)
{
    _fields = __o._fields;      // Nincs öröklés !
    _stat    = __o._stat;
    _pMagicMap = __o._pMagicMap == NULL ? NULL : new tMagicMap(*__o._pMagicMap);
}

cService::~cService()
{
    if (_pMagicMap != NULL) delete _pMagicMap;
}

int             cService::_ixProtocolId = NULL_IX;
int             cService::_ixProperties = NULL_IX;
const cRecStaticDescr&  cService::descr() const
{
    if (initPDescr<cService>(_sServices)) {
        _ixProtocolId = _descr_cService().toIndex(_sProtocolId);
        _ixProperties = _descr_cService().toIndex(_sProperties);
    }
    return *_pRecordDescr;
}
void cService::toEnd()
{
    cService::toEnd(_ixProtocolId);
    cService::toEnd(_ixProperties);
}

bool cService::toEnd(int i)
{
    if (i == _ixProtocolId) {
        if (get(i).isValid()) {
            _protocol.fetchById(get(i).toLongLong());
        }
        else _protocol.set();
        return true;
    }
    if (i == _ixProperties) {
        if (_pMagicMap != NULL) {
            delete _pMagicMap;
            _pMagicMap = NULL;
            return true;
        }
    }
    return false;
}

void cService::clearToEnd()
{
    _protocol.set();
}
CRECDEF(cService)


tMagicMap&  cService::splitMagic(bool __ex)
{
    _SplitMagicT<cService>(*this, __ex);
    return *_pMagicMap;
}


const tMagicMap&  cService::magicMap(bool __ex) const
{
    if (_pMagicMap == NULL) _SplitMagicT<cService>(*(const_cast<cService *>(this)), __ex);
    return *_pMagicMap;
}

tMagicMap&  cService::magicMap(bool __ex)
{
    if (_pMagicMap == NULL) _SplitMagicT<cService>(*this, __ex);
    return *_pMagicMap;
}

cService& cService::magic2prop()
{
    _Magic2PropT<cService>(*this);
    return *this;
}

tRecordList<cService> cService::services;
cService *cService::pNull;
const cService& cService::service(QSqlQuery& __q, const QString& __nm, bool __ex)
{
    int i = services.indexOf(_nul().nameIndex(), QVariant(__nm));
    if (i < 0) {
        cService *p = new cService();
        if (!p->fetchByName(__q, __nm)) {
            if (__ex) EXCEPTION(EFOUND, -1, QString(QObject::trUtf8("Ismeretlen szolgáltatás név : %1")).arg(__nm));
            delete p;
            return _nul();
        }
        services << p;
        return *p;
    }
    return *services.at(i);
}

const cService& cService::service(QSqlQuery &__q, qlonglong __id, bool __ex)
{
    int i = services.indexOf(_nul().idIndex(), QVariant(__id));
    if (i < 0) {
        cService *p = new cService();
        if (!p->fetchById(__q, __id)) {
            if (__ex) EXCEPTION(EFOUND, __id, QObject::trUtf8("Ismeretlen szolgáltatás azonosító."));
            delete p;
            return _nul();
        }
        services << p;
        return *p;
    }
    return *services.at(i);
}

/* ----------------------------------------------------------------- */

cHostService::cHostService() : cRecord()
{
    _pMagicMap = NULL;
    _set(cHostService::descr());
}

cHostService::cHostService(const cHostService& __o) : cRecord()
{
    (*this) = __o;
}

cHostService::~cHostService()
{
    if (_pMagicMap != NULL) delete _pMagicMap;
}

cHostService& cHostService::operator=(const cHostService& __o)
{
    __cp(__o);
    _fields = __o._fields;              // Nincs öröklés !
    _pMagicMap = __o._pMagicMap == NULL ? NULL : new tMagicMap(*__o._pMagicMap);
    return *this;
}

int cHostService::_ixProperties  = NULL_IX;
const cRecStaticDescr&  cHostService::descr() const
{
    if (initPDescr<cHostService>(_sHostServices)) {
        // Ez egy SWITCH tábla kell legyen ...
        if (_descr_cHostService().tableType() != SWITCH_TABLE) EXCEPTION(EPROGFAIL, _descr_cHostService().tableType(), _descr_cHostService().toString());
        _ixProperties = _descr_cHostService().toIndex(_sProperties);
    }
    return *_pRecordDescr;
}

void cHostService::toEnd()
{
    toEnd(_ixProperties);
}

bool cHostService::toEnd(int i)
{
    if (i == _ixProperties) {
        if (_pMagicMap != NULL) {
            delete _pMagicMap;
            _pMagicMap = NULL;
            return true;
        }
    }
    return false;
}

void cHostService::clearToEnd()
{
    ;
}

CRECDEF(cHostService)

cHostService&  cHostService::setState(QSqlQuery& __q, const QString& __st, const QString& __note, qlonglong __did)
{
    QString sql = QString("SELECT * FROM set_service_stat(%1, '%2', '%3'%4)")
            .arg(getId())
            .arg(__st)
            .arg(__note)
            .arg(__did == NULL_ID ? _sNul : (_sCommaSp + QString::number(__did)) );
    PDEB(VVERBOSE) << "setState() : " << sql << endl;
    if (!__q.exec(sql)) SQLPREPERR(__q, sql);
    if (!__q.first()) SQLERR(__q, EQUERY);
    set(__q);
    // DBGFNL();
    return *this;
}

int cHostService::fetchByNames(QSqlQuery& q, QString& __hn, const QString& __sn, bool __ex)
{
    set();
    (*this)[_sNodeId]    = cNode().descr().getIdByName(q, __hn, __ex);
    (*this)[_sServiceId] = cService().descr().getIdByName(q, __sn, __ex);
    (*this)[_sDeleted]   = false;
    int r = completion(q);
    if (__ex && !r) EXCEPTION(EFOUND, -1, __hn + _sColon + __sn);
    return r;
}

bool cHostService::fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, bool __ex)
{
    set();
    (*this)[_sNodeId]    = __hid;
    (*this)[_sServiceId] = __sid;
    (*this)[_sDeleted]   = false;
    int r = completion(q);
    if (__ex && r != 1) EXCEPTION(EFOUND, __hid, QString::number(__sid));
    return r == 1;
}

int cHostService::delByNames(QSqlQuery& q, const QString& __nn, const QString& __sn, bool __pat)
{
    QString sql =
            "SELECT host_service_id FROM host_services JOIN nodes USING(node_id) JOIN services USING(service_id)"
            " WHERE node_name = ? AND service_name %1 ?";
    sql = sql.arg(__pat ? "LIKE" : "=");
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    q.bindValue(1, __sn);
    if (!q.exec()) SQLQUERYERR(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << q.value(0).toLongLong();
        } while (q.next());
        foreach (qlonglong id, idl) {
            setId(id);
            if (!remove(q, false, primaryKey())) DERR() << "Nem sikerült törölni a host_services rekordot, ID = " << id << endl;
        }
    }
    return idl.size();
}

bool cHostService::fetchSelf(QSqlQuery& q, cHost& __h, const cService &__s, bool __ex)
{
    if (!__h.fetchSelf(q, __ex)) return false;
    if (!fetchByIds(q, __h.getId(), __s.getId(), __ex)) return false;
    return true;
}

int cHostService::fetchSubs(QSqlQuery& __q, cHostService& __sup)
{
    _DBGFN() << " @(-, " << __sup.names(__q) << ")" << endl;
    clear();
    if (__sup.isNullId()) {
        int r = __sup.completion(__q);
        if (r > 1) EXCEPTION(AMBIGUOUS,-1, __sup.names(__q));
        if (r < 1) EXCEPTION(EFOUND,   -1, __sup.names(__q));
    }
    setId(_sSuperiorHostServiceId, __sup.getId());
    int r = completion(__q);
    _DBGFNL() << " found : " << r << endl;
    return r;
}

QVariant cHostService::value(QSqlQuery& q, const cService& s, const QString& f)
{
    if (isNull(f)) {
        if (s.isNull(f)) {
            QString e = toString() + _sSlash + s.toString() + _sSlash + f;
            q.clear();
            EXCEPTION(EFOUND, 0, e);
        }
        (*this)[f] = s[f];
        update(q, true, mask(f));
    }
    return get(f);
}

QString cHostService::names(QSqlQuery& q)
{
    QString r = cNode().getNameById(q, getId(_sNodeId), false)
              + _sColon
              + cService().getNameById(q, getId(_sServiceId), false);
    q.clear();
    return r;
}

tMagicMap&  cHostService::splitMagic(bool __ex)
{
    _SplitMagicT<cHostService>(*this, __ex);
    return *_pMagicMap;
}

const tMagicMap&  cHostService::magicMap(bool __ex) const
{
    if (_pMagicMap == NULL) _SplitMagicT<cHostService>(*(const_cast<cHostService *>(this)), __ex);
    return *_pMagicMap;
}

tMagicMap&  cHostService::magicMap(bool __ex)
{
    if (_pMagicMap == NULL) _SplitMagicT<cHostService>(*this, __ex);
    return *_pMagicMap;
}

cHostService& cHostService::magic2prop()
{
    _Magic2PropT<cHostService>(*this);
    return *this;
}

/* ----------------------------------------------------------------- */
CRECCNTR(cArp);

int cArp::_ixIpAddress = NULL_IX;
int cArp::_ixHwAddress = NULL_IX;

const cRecStaticDescr&  cArp::descr() const
{
    if (initPDescr<cArp>(_sArps)) {
        _ixIpAddress = _descr_cArp().toIndex(_sIpAddress);
        _ixHwAddress = _descr_cArp().toIndex(_sHwAddress);
    }
    return *_pRecordDescr;
}
CRECDEFD(cArp)

cArp::operator QHostAddress() const
{
    if (isNull(_ixIpAddress)) return QHostAddress();
    const netAddress na = get(_ixIpAddress).value<netAddress>();
    return na.addr();
}
cArp::operator cMac() const
{
    if (isNull(_ixHwAddress)) return cMac();
    return get(_ixHwAddress).value<cMac>();
}

enum cArp::eReplaceResult cArp::replace(QSqlQuery& __q)
{
    QString sql = "SELECT insert_or_update_arp(?,?)";
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    __q.bindValue(0, descr().colDescr(_ixIpAddress).toSql(get(_ixIpAddress)));
    __q.bindValue(1, descr().colDescr(_ixHwAddress).toSql(get(_ixHwAddress)));
    if (!__q.exec()) SQLQUERYERR(__q);
    bool ok = false;
    __q.first();
    enum eReplaceResult r = (enum eReplaceResult) __q.value(0).toInt(&ok);
    return ok ? r : RR_ERROR;
}

#ifdef MUST_SCAN
int cArp::replaces(QSqlQuery& __q, const cArpTable& __t)
{
    int r = 0;
    cArp arp;
    for (cArpTable::const_iterator i = __t.constBegin(); i != __t.constEnd(); ++i) {
        arp = i.key();
        arp = i.value();
        if (0 < arp.replace(__q)) ++r;
    }
    return r;
}
#else  // MUST_SCAN
int cArp::replaces(QSqlQuery&, const cArpTable&)
{
    EXCEPTION(ENOTSUPP);
    return 0;
}
#endif // MUST_SCAN

QList<QHostAddress> cArp::mac2ips(QSqlQuery& __q, const cMac& __m)
{
    QList<QHostAddress> r;
    cArp arp;
    arp = __m;
    if (arp.fetch(__q, false, arp.mask(_ixHwAddress))) {
        r << arp;
        while (arp.next(__q)) r << arp;
    }
    return r;
}

cMac cArp::ip2mac(QSqlQuery& __q, const QHostAddress& __a)
{
    cArp arp;
    arp = __a;
    if (arp.fetch(__q, false, arp.mask(_ixIpAddress))) return arp;
    return cMac();
}

/* ----------------------------------------------------------------- */
bool LinkUnlink(QSqlQuery& q, cRecord& o, qlonglong __pid)
{
    o.clear();
    o.setId(_sPortId1, __pid);
    if (o.completion(q) != 1) return false;
    return o.remove(q);
}

qlonglong LinkGetLinked(QSqlQuery& q, cRecord& o, qlonglong __pid)
{
    o.clear();
    o.setId(_sPortId1, __pid);
    if (o.completion(q) == 1) return o.getId(_sPortId2);
    return NULL_ID;
}
bool LinkIsLinked(QSqlQuery& q, cRecord& o, qlonglong __pid1, qlonglong __pid2)
{
    o.clear();
    o.setId(_sPortId1, __pid1);
    o.setId(_sPortId1, __pid2);
    return (o.completion(q) == 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
CRECCNTR(cPhsLink)
const cRecStaticDescr&  cPhsLink::descr() const
{
    if (initPDescr<cPhsLink>(_sPhsLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cPhsLink().tableType() != LINK_TABLE || _descr_cPhsLink().tableName() != _sPhsLinksTable) EXCEPTION(EPROGFAIL);
    }
    return *_pRecordDescr;
}
CRECDEFD(cPhsLink)

int cPhsLink::unlink(QSqlQuery &q, const QString& __nn, const QString& __pn, bool __pat)
{
    QString sql =
            "SELECT phs_link_id FROM phs_links AS l"
            " JOIN nports AS p ON p.port_id = l.port_id1"
            " JOIN nodes  AS n ON n.node_id = p.node_id"
            " WHERE n.node_name = ? AND p.port_name %1 ?";
    sql = sql.arg(__pat ? "LIKE" : "=");
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    q.bindValue(1, __pn);
    if (!q.exec()) SQLQUERYERR(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << q.value(0).toLongLong();
        } while (q.next());
        foreach (qlonglong id, idl) {
            setId(id);
            PDEB(VVERBOSE) << " Delete " << tableName() << " rekord, " << VDEBBOOL(id) << endl;
            if (!remove(q, false, primaryKey())) DERR() << "Nem sikerült törölni a host_services rekordot, ID = " << id << endl;
        }
    }
    else {
        DWAR() << "Egyetlen rekord sem kerül törlésre." << endl;
    }
    return idl.size();
}

int cPhsLink::unlink(QSqlQuery &q, const QString& __nn, int __ix, int __ei)
{
    QString sql =
            "SELECT phs_link_id FROM phs_links AS l"
            " JOIN nports AS p ON p.port_id = l.port_id1"
            " JOIN nodes  AS n ON n.node_id = p.node_id"
            " WHERE n.node_name = ? AND p.port_index ";
    if (__ei == NULL_IX || __ei == __ix) {
        sql += "= " + QString::number(__ix);
    }
    else {
        // if (__ix > __ie) EXCEPTION(EDATA);
        sql += ">= " + QString::number(__ix) + "AND p.port_index <= " + QString::number(__ei);
    }

    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    if (!q.exec()) SQLQUERYERR(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << q.value(0).toLongLong();
        } while (q.next());
        foreach (qlonglong id, idl) {
            setId(id);
            PDEB(VVERBOSE) << " Delete " << tableName() << " rekord, " << VDEBBOOL(id) << endl;
            if (!remove(q, false, primaryKey())) DERR() << "Nem sikerült törölni a host_services rekordot, ID = " << id << endl;
        }
    }
    else {
        DWAR() << "Egyetlen rekord sem kerül törlésre." << endl;
    }
    return idl.size();
}

/* ----------------------------------------------------------------- */

CRECCNTR(cLogLink)
const cRecStaticDescr&  cLogLink::descr() const
{
    if (initPDescr<cLogLink>(_sLogLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cLogLink().tableType() != LINK_TABLE || _descr_cLogLink().tableName() != _sLogLinksTable) EXCEPTION(EPROGFAIL);
    }
    return *_pRecordDescr;
}

bool cLogLink::insert(QSqlQuery &, bool)
{
    EXCEPTION(ENOTSUPP);
    return false;
}

bool cLogLink::update(QSqlQuery &, bool, const QBitArray &, const QBitArray &, bool)
{
    EXCEPTION(ENOTSUPP);
    return false;
}

CRECDEFD(cLogLink)

/* ----------------------------------------------------------------- */

CRECCNTR(cLldpLink)
const cRecStaticDescr&  cLldpLink::descr() const
{
    if (initPDescr<cLldpLink>(_sLldpLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cLldpLink().tableType() != LINK_TABLE || _descr_cLldpLink().tableName() != _sLldpLinksTable) EXCEPTION(EPROGFAIL);
    }
    return *_pRecordDescr;
}
CRECDEFD(cLldpLink)

/* ----------------------------------------------------------------- */
DEFAULTCRECDEF(cImport, _sImports)
/* ---------------------------------------------------------------------------------------------------- */
DEFAULTCRECDEF(cImportTemplate, _sImportTemplates)

cTemplateMap::cTemplateMap(const QString& __type) : QMap<QString, QString>(), type(__type)
{
    ;
}

cTemplateMap::cTemplateMap(const cTemplateMap& __o) : QMap<QString, QString>(__o), type(__o.type)
{
    ;
}

const QString& cTemplateMap::get(QSqlQuery& __q, const QString& __name)
{
    QMap<QString, QString>::iterator i = find(__name);
    if (i != end()) return i.value();
    cImportTemplate it;
    it[_sTemplateType] = type;
    it[_sTemplateName] = __name;
    if (it.completion(__q) < 1) EXCEPTION(EFOUND, -1, QString(QObject::trUtf8("%1 template name %2 not found.")).arg(type, __name));
    i = insert(__name, it.getName(_sTemplateText));
    return i.value();
}

void cTemplateMap::set(const QString& __name, const QString& __cont)
{
    insert(__name, __cont);
}

void cTemplateMap::save(QSqlQuery& __q, const QString& __name, const QString& __cont, const QString& __descr)
{
    cImportTemplate it;
    it[_sTemplateType] = type;
    it[_sTemplateName] = __name;
    it[_sTemplateText] = __cont;
    it[_sTemplateDescr]= __descr;
    it.insert(__q);
    insert(__name, __cont);
}

void cTemplateMap::del(QSqlQuery& __q, const QString &__name)
{
    remove(__name);
    cImportTemplate it;
    it[_sTemplateType] = type;
    it[_sTemplateName] = __name;
    it.remove(__q);
}

/* ---------------------------------------------------------------------------------------------------- */

int vlanType(const QString& __n, bool __ex)
{
    if (__n == _sNo)        return VT_NO;
    if (__n == _sUnknown)   return VT_NOTKNOWN;
    if (__n == _sForbidden) return VT_FORBIDDEN;
    if (__n == _sAuto)      return VT_AUTO;
    if (__n == _sTagged)    return VT_TAGGED;
    if (__n == _sUntagged)  return VT_UNTAGGED;
    if (__n == _sVirtual)   return VT_VIRTUAL;
    if (__n == _sHard)      return VT_HARD;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return VT_INVALID;
}

const QString& vlanType(int __e, bool __ex)
{
    switch (__e) {
    case VT_NO:         return _sNo;
    case VT_NOTKNOWN:    return _sUnknown;
    case VT_FORBIDDEN:  return _sForbidden;
    case VT_AUTO:       return _sAuto;
    case VT_TAGGED:     return _sTagged;
    case VT_UNTAGGED:   return _sUntagged;
    case VT_VIRTUAL:    return _sVirtual;
    case VT_HARD:       return _sHard;
    default:            if (__ex) EXCEPTION(EDATA, (qlonglong)__e);
                        return _sNULL;
    }
}

int  setType(const QString& __n, bool __ex)
{
    if (__n == _sAuto)   return ST_AUTO;
    if (__n == _sQuery)  return ST_QUERY;
    if (__n == _sManual) return ST_MANUAL;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return ST_INVALID;
}

const QString& setType(int __e, bool __ex)
{
    switch (__e) {
    case ST_AUTO:   return _sAuto;
    case ST_QUERY:  return _sQuery;
    case ST_MANUAL: return _sManual;
    default:        if (__ex) EXCEPTION(EDATA, (qlonglong)__e);
                    return _sNULL;
    }
}

CRECCNTR(cPortVlan)

int cPortVlan::_ixPortId   = NULL_IX;
int cPortVlan::_ixVlanId   = NULL_IX;
int cPortVlan::_ixVlanType = NULL_IX;
int cPortVlan::_ixSetType  = NULL_IX;

const cRecStaticDescr&  cPortVlan::descr() const
{
    if (initPDescr<cPortVlan>(_sPortVlans)) {
        _ixPortId   = _descr_cPortVlan().toIndex(_sPortId);
        _ixVlanId   = _descr_cPortVlan().toIndex(_sVlanId);
        _ixVlanType = _descr_cPortVlan().toIndex(_sVlanType);
        _ixSetType  = _descr_cPortVlan().toIndex(_sSetType);
        CHKENUM(_ixVlanType, vlanType);
        CHKENUM(_ixSetType, setType);
    }
    return *_pRecordDescr;
}

CRECDEFD(cPortVlan)
