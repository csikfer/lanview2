#include "lanview.h"
#include "lv2data.h"
#include "scan.h"
// #include "lv2service.h"
#include "others.h"

#ifndef LV2SERVICE_H
EXT_ const QString& notifSwitch(int _ns, enum eEx __ex = EX_ERROR);
EXT_ int notifSwitch(const QString& _nm, enum eEx __ex = EX_ERROR);
#endif

int reasons(const QString& _r, eEx __ex)
{
    if (0 == _r.compare(_sNew,      Qt::CaseInsensitive)) return R_NEW;
    if (0 == _r.compare(_sInsert,   Qt::CaseInsensitive)) return R_INSERT;
    if (0 == _r.compare(_sRemove,   Qt::CaseInsensitive)) return R_REMOVE;
    if (0 == _r.compare(_sExpired,  Qt::CaseInsensitive)) return R_EXPIRED;
    if (0 == _r.compare(_sMove,     Qt::CaseInsensitive)) return R_MOVE;
    if (0 == _r.compare(_sRestore,  Qt::CaseInsensitive)) return R_RESTORE;
    if (0 == _r.compare(_sModify,   Qt::CaseInsensitive)) return R_MODIFY;
    if (0 == _r.compare(_sUpdate,   Qt::CaseInsensitive)) return R_UPDATE;
    if (0 == _r.compare(_sUnchange, Qt::CaseInsensitive)) return R_UNCHANGE;
    if (0 == _r.compare(_sFound,    Qt::CaseInsensitive)) return R_FOUND;
    if (0 == _r.compare(_sNotfound, Qt::CaseInsensitive)) return R_NOTFOUND;
    if (0 == _r.compare(_sDiscard,  Qt::CaseInsensitive)) return R_DISCARD;
    if (0 == _r.compare(_sCaveat,   Qt::CaseInsensitive)) return R_CAVEAT;
    if (0 == _r.compare(_sError,    Qt::CaseInsensitive)) return R_ERROR;
    if (0 == _r.compare(_sAmbiguous,Qt::CaseInsensitive)) return R_AMBIGUOUS;
    if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, _r);
    return R_INVALID;
}

const QString& reasons(int _r, eEx __ex)
{
    switch (_r) {
    case R_NEW:         return _sNew;
    case R_INSERT:      return _sInsert;
    case R_REMOVE:      return _sRemove;
    case R_EXPIRED:     return _sExpired;
    case R_MOVE:        return _sMove;
    case R_RESTORE:     return _sRestore;
    case R_MODIFY:      return _sModify;
    case R_UPDATE:      return _sUpdate;
    case R_UNCHANGE:    return _sUnchange;
    case R_FOUND:       return _sFound;
    case R_NOTFOUND:    return _sNotfound;
    case R_DISCARD:     return _sDiscard;
    case R_CAVEAT:      return _sCaveat;
    case R_ERROR:       return _sError;
    case R_AMBIGUOUS:   return _sAmbiguous;
    case REASON_OK:     return _sOk;
    case REASON_TO:     return _sTimeOut;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EDATA, _r);
    return _sNul;

}

/* ------------------------------ param_types ------------------------------ */
int paramTypeType(const QString& __n, eEx __ex)
{
    if (0 == __n.compare(_sBoolean,   Qt::CaseInsensitive))         return PT_BOOLEAN;
    if (0 == __n.compare(_sBigInt,    Qt::CaseInsensitive))         return PT_BIGINT;
    if (0 == __n.compare(_sDoublePrecision, Qt::CaseInsensitive))   return PT_DOUBLE_PRECISION;
    if (0 == __n.compare(_sText,      Qt::CaseInsensitive))         return PT_TEXT;
    if (0 == __n.compare(_sInterval,  Qt::CaseInsensitive))         return PT_INTERVAL;
    if (0 == __n.compare(_sDate,      Qt::CaseInsensitive))         return PT_DATE;
    if (0 == __n.compare(_sTime,      Qt::CaseInsensitive))         return PT_TIME;
    if (0 == __n.compare(_sTimestamp, Qt::CaseInsensitive))         return PT_TIMESTAMP;
    if (0 == __n.compare(_sINet,      Qt::CaseInsensitive))         return PT_INET;
    if (0 == __n.compare(_sByteA,     Qt::CaseInsensitive))         return PT_BYTEA;
    if (__ex != EX_IGNORE)  EXCEPTION(EDATA, -1, __n);
    return PT_INVALID;
}

const QString& paramTypeType(int __e, eEx __ex)
{
    switch (__e) {
    case PT_BOOLEAN:            return _sBoolean;
    case PT_BIGINT:             return _sBigInt;
    case PT_DOUBLE_PRECISION:   return _sDoublePrecision;
    case PT_TEXT:               return _sText;
    case PT_INTERVAL:           return _sInterval;
    case PT_DATE:               return _sDate;
    case PT_TIME:               return _sTime;
    case PT_TIMESTAMP:          return _sTimestamp;
    case PT_INET:               return _sINet;
    case PT_BYTEA:              return _sByteA;
    }
    if (__ex != EX_IGNORE)   EXCEPTION(EDATA, __e);
    return _sNul;
}

tRecordList<cParamType>  cParamType::paramTypes;
cParamType *cParamType::pNull;

CRECCNTR(cParamType)
CRECDEFD(cParamType)

const cRecStaticDescr& cParamType::descr() const
{
    if (initPDescr<cParamType>(_sParamTypes)) {
        CHKENUM(toIndex(_sParamTypeType), paramTypeType)
    }
    return *_pRecordDescr;
}

qlonglong cParamType::insertNew(QSqlQuery& q, const QString& __n, const QString& __de, const QString __t, const QString __di, eEx __ex)
{
    cParamType pp;
    pp.setName(__n);
    pp.set(_sParamTypeNote, __de);
    pp.set(_sParamTypeType,  __t);
    pp.set(_sParamTypeDim,   __di);
    if (pp.isDefective()) {
        if (__ex) EXCEPTION(EDATA, -1, __t);
        return NULL_ID;
    }
    pp.insert(q, __ex);
    return pp.getId();
}

qlonglong cParamType::insertNew(QSqlQuery& q, const QString& __n, const QString& __de, int __t, const QString __di, eEx __ex)
{
    cParamType pp;
    pp.setName(__n);
    pp.set(_sParamTypeNote, __de);
    pp.set(_sParamTypeType,  __t);
    pp.set(_sParamTypeDim,   __di);
    if (pp.isDefective()) {
        if (__ex) EXCEPTION(EDATA, __t);
        return NULL_ID;
    }
    pp.insert(q, __ex);
    return pp.getId();
}

QString cParamType::paramToString(eParamType __t, const QVariant& __v, eEx __ex)
{
    QString r;
    bool ok = false;
    if (__v.isNull()) {
        if (__t == PT_BOOLEAN) return _sFalse;
        ok = true;
    }
    else {
        switch ((int)__t) {
        case PT_TEXT:
            ok = __v.canConvert(QVariant::String);
            if (ok) r = __v.toString();
            break;
        case PT_BOOLEAN: {
            r = str2bool(__v.toString(), __ex) ? _sTrue : _sFalse;
            ok = true;
            break;
        }
        case PT_BIGINT:
            ok = __v.canConvert(QVariant::LongLong);
            if (ok) r = QString::number(__v.toLongLong());
            break;
        case PT_DOUBLE_PRECISION:
            ok = __v.canConvert(QVariant::Double);
            if (ok) r = QString::number(__v.toDouble());
            break;
        case PT_INTERVAL: {
            qlonglong i = 0;
            if (__v.canConvert(QVariant::LongLong)) {
                i = __v.toLongLong(&ok);
            }
            else {
                if (__v.canConvert(QVariant::String)) {
                    i = parseTimeInterval(__v.toString(), &ok);
                }
            }
            if (ok) {
                r = intervalToStr(i);
            }
            break;
        }
        case PT_INET: {
            int mtid = __v.userType();
            if (mtid == _UMTID_netAddress) {
                r = __v.value<netAddress>().toString();
                ok = true;
            }
            else if (mtid == _UMTID_QHostAddress) {
                r = __v.value<QHostAddress>().toString();
                ok = true;
            }
            else if (metaIsString(mtid)) {
                r = __v.toString();
                ok = true;
            }
        }
        case PT_INVALID:
            EXCEPTION(ENOTSUPP, __t);
        }
    }
    if (!ok && __ex) EXCEPTION(EDATA, __t, debVariantToString(__v));
    return r;
}

QVariant cParamType::paramFromString(eParamType __t, QString& __v, eEx __ex)
{
    QVariant r;
    bool    ok = true;
    if (__v.isNull()) {
        if (__t == PT_BOOLEAN) r = QVariant(false);
    }
    else {
        switch ((int)__t) {
        case PT_TEXT:               r = QVariant(__v);                   break;
        case PT_BOOLEAN:            r = QVariant(str2bool(__v));         break;
        case PT_BIGINT:             r = QVariant(__v.toLongLong(&ok));   break;
        case PT_DOUBLE_PRECISION:   r = QVariant(__v.toDouble(&ok));
        case PT_INTERVAL:           r = QVariant(parseTimeInterval(__v, &ok));   break;
        case PT_INET: {
            netAddress  na;
            ok = na.setr(__v).isValid();
            if (ok) r = QVariant::fromValue(na);
            break;
        }
        default:
            ok = false;
            break;
        }
    }
    if (!ok) {
        if (__ex) EXCEPTION(ENOTSUPP, (int)__t, __v);
        r.clear();
    }
    return r;
}

void cParamType::fetchParamTypes(QSqlQuery& __q)
{
    if (pNull == NULL) pNull = new cParamType();
    QBitArray   ba(1, false);   // Nem null, egyeseket nem tartalmazó maszk, minden rekord kiválasztásához
    if (paramTypes.count() == 0) {
        cParamType *p = new cParamType();
        if (0 == paramTypes.fetch(__q, true, ba, p)) EXCEPTION(EDATA, 0, trUtf8("Load cache: The if_types table is empty."));
        return;
    }
    // UPDATE
    int found = 0;              // Megtalált rekordok száma
    int n = paramTypes.count();    // Megtalálandó rekordok száma
    cParamType pt;
    if (!pt.fetch(__q, false, ba)) EXCEPTION(EDATA, 0, trUtf8("Update cache: The if_types table is empty."));
    do {
        cParamType *pPt = paramTypes.get(pt.getId(), EX_IGNORE);    // ID alapján rákeresünk
        if (pPt == NULL) {   // Ez egy új rekord
            paramTypes << pt;
        }
        else {                // Frissít
            pPt->set(pt);
            ++n;
        }
    } while (pt.next(__q));
    if (n < found) EXCEPTION(EPROGFAIL);
    if (n > found) EXCEPTION(EDATA, n - found, trUtf8("Deleted if_tpes record."));
}

const cParamType& cParamType::paramType(const QString& __nm, eEx __ex)
{
    checkParamTypes();
    int i = paramTypes.indexOf(_descr_cParamType().nameIndex(), QVariant(__nm));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, -1,QObject::trUtf8("Invalid ParamType name %1 or program error.").arg(__nm));
        return *pNull;
    }
    return *(paramTypes[i]);
}

const cParamType& cParamType::paramType(qlonglong __id, eEx __ex)
{
    checkParamTypes();
    int i = paramTypes.indexOf(_descr_cParamType().idIndex(), QVariant(__id));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, __id,QObject::trUtf8("Invalid ParamType id or program error."));
        return *pNull;
    }
    return *(paramTypes[i]);
}


/*  */

CRECCNTR(cSysParam)
CRECDEFD(cSysParam)

int cSysParam::_ixParamTypeId = NULL_IX;
int cSysParam::_ixParamValue  = NULL_IX;
const cRecStaticDescr&  cSysParam::descr() const
{
    if (initPDescr<cSysParam>(_sSysParams)) {
        _ixParamTypeId = _pRecordDescr->toIndex(_sParamTypeId);
        _ixParamValue  = _pRecordDescr->toIndex(_sParamValue);
    }
    return *_pRecordDescr;
}

void    cSysParam::toEnd()
{
    cSysParam::toEnd(_ixParamTypeId);
}

bool cSysParam::toEnd(int i)
{
    if (i == _ixParamTypeId) {
        qlonglong id = _isNull(i) ? NULL_ID : variantToId(_get(i));
        if (id == NULL_ID) {
            paramType.clear();
        }
        else {
            QSqlQuery q = getQuery();
            if (!paramType.fetchById(q, id)) {
                _stat |= ES_DEFECTIVE;
                _setDefectivFieldBit(i);
            }
        }
        return true;
    }
    return false;
}

void    cSysParam::clearToEnd()
{
    paramType.clear();
}

QVariant cSysParam::value(eEx __ex) const
{
    QString v = getName(_ixParamValue);
    return cParamType::paramFromString((enum eParamType)valueType(), v, __ex);
}

cSysParam& cSysParam::setValue(const QVariant& __v, eEx __ex)
{
    QString v = cParamType::paramToString((enum eParamType)valueType(), __v, __ex);
    setName(_ixParamValue, v);
    return *this;
}

/* ------------------------------ tpows ------------------------------ */
DEFAULTCRECDEF(cTpow, _sTpows)
/* ------------------------------ timeperiods ------------------------------ */
DEFAULTCRECDEF(cTimePeriod, _sTimePeriods)
/* ------------------------------ images ------------------------------ */
CRECCNTR(cImage)
CRECDEFD(cImage)

int  cImage::_ixImageType = NULL_IX;
int  cImage::_ixImageData = NULL_IX;
int  cImage::_ixImageHash = NULL_IX;
const cRecStaticDescr&  cImage::descr() const
{
    if (initPDescr<cImage>(_sImages)) {
        STFIELDIX(cImage, ImageType);
        STFIELDIX(cImage, ImageData);
        STFIELDIX(cImage, ImageHash);
        CHKENUM(_ixImageType, imageType);
    }
    return *_pRecordDescr;
}

bool cImage::toEnd(int _i)
{
    if (_i == _ixImageData) {
        clear(_ixImageHash);
        return true;
    }
    return false;
}

bool cImage::load(const QString& __fn, eEx __ex)
{
    QFile   f(__fn);
    if (!f.open(QIODevice::ReadOnly)) {
        if (__ex) EXCEPTION(ENOTFILE, -1, __fn);
        return false;
    }
    setImage(f.readAll());
    return true;
}

bool cImage::save(const QString& __fn, eEx __ex)
{
    QFile   f(__fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (__ex) EXCEPTION(ENOTFILE, -1, __fn);
        return false;
    }
    f.write(getImage());
    return true;
}

int imageType(const QString& __n, eEx __ex)
{
    if (__n == _sBMP) return IT_BMP;
    if (__n == _sGIF) return IT_GIF;
    if (__n == _sJPG) return IT_JPG;
    if (__n == _sJPEG)return IT_JPEG;
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

const QString&  imageType(int __e, eEx __ex)
{
    switch (__e) {
    case IT_BMP:        return _sBMP;
    case IT_GIF:        return _sGIF;
    case IT_JPG:        return _sJPG;
    case IT_JPEG:       return _sJPEG;
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

const char *   _imageType(int __e, eEx __ex)
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

qlonglong cPlace::parentImageId(QSqlQuery& q)
{
    execSqlFunction(q, "get_parent_image", getId());
    QVariant iid = q.value(0);
    return iid.isNull() ? NULL_ID : variantToId(iid);
}

/* ------------------------------ place_froups ------------------------------ */

DEFAULTCRECDEF(cPlaceGroup, _sPlaceGroups)

qlonglong cPlaceGroup::insertNew(QSqlQuery q, const QString& __n, const QString& __d)
{
    cPlaceGroup o;
    o.setName(__n);
    o.setName(_sPlaceGroupNote, __d);
    o.insert(q);
    return o.getId();
}

qlonglong cPlaceGroup::replaceNew(QSqlQuery q, const QString& __n, const QString& __d)
{
    cPlaceGroup o;
    o.setName(__n);
    o.setName(_sPlaceGroupNote, __d);
    o.replace(q);
    return o.getId();
}

/* ------------------------------ subnets ------------------------------ */
CRECCNTR(cSubNet)
CRECDEFD(cSubNet)

int cSubNet::_ixNetAddr    = NULL_IX;
int cSubNet::_ixVlanId     = NULL_IX;
int cSubNet::_ixSubnetType = NULL_IX;
const cRecStaticDescr&  cSubNet::descr() const
{
    if (initPDescr<cSubNet>(_sSubNets)) {
        STFIELDIX(cSubNet, NetAddr);
        STFIELDIX(cSubNet, VlanId);
        STFIELDIX(cSubNet, SubnetType);
        CHKENUM(_ixSubnetType, subNetType);
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

qlonglong cSubNet::insertNew(const QString& _n, const QString& _d, const netAddress& _a, qlonglong _v, const QString& __t)
{
    cSubNet no;
    no.setName(_n);
    no.set(_sSubNetNote, _d);
    no = _a;
    if (_v > 0) no.set(_sVlanId, QVariant(_v));
    no.set(_sSubnetType, QVariant(__t));
    QSqlQuery   *pq = newQuery();
    no.insert(*pq);
    return no.get(cSubNet::_pRecordDescr->toIndex(_sSubNetId)).toInt();
}

int cSubNet::getByAddress(QSqlQuery & __q, const QHostAddress& __addr)
{
    QString where = QString(" %1 >> inet '%2' ").arg(dQuoted(_sNetAddr), __addr.toString());
    fetchQuery(__q, false, QBitArray(1), tIntVector(), 0, 0, QString(), where);
    if (!__q.first()) return 0;
    set(__q.record());
    return __q.size();
}

const QString& subNetType(int __at, eEx __ex)
{
    switch (__at) {
    case NT_PRIMARY:    return _sPrimary;
    case NT_SECONDARY:  return _sSecondary;
    case NT_PSEUDO:     return _sPseudo;
    case NT_PRIVATE:    return _sPrivate;
    default:  if (__ex) EXCEPTION(EDATA, __at);
    }
    return _sNul;
}

int subNetType(const QString& __at, eEx __ex)
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
    vo.set(_sVlanNote, QVariant(__d));
    vo.set(_sVlanStat, QVariant(__s));
    QSqlQuery   *pq = newQuery();
    vo.insert(*pq);
    delete pq;
    return __i;
}

/* ------------------------------ ipaddresses ------------------------------ */

cIpAddress::cIpAddress() : cRecord()
{
    _set(cIpAddress::descr());
}

cIpAddress::cIpAddress(const cIpAddress& __o) : cRecord()
{
    __cp(__o);
    _copy(__o, *_pRecordDescr);
}

cIpAddress::cIpAddress(const QHostAddress& __a, const QString& __t)  : cRecord()
{
    _set(cIpAddress::descr());
    _set(_ixAddress, QVariant::fromValue(netAddress(__a)));
    _set(cIpAddress::_pRecordDescr->toIndex(_sIpAddressType), QVariant(__t));
}

CRECDEFD(cIpAddress)

// -- virtual
int cIpAddress::_ixPortId = NULL_IX;
int cIpAddress::_ixAddress = NULL_IX;
int cIpAddress::_ixSubNetId = NULL_IX;
int cIpAddress::_ixIpAddressType = NULL_IX;

const cRecStaticDescr&  cIpAddress::descr() const
{
    if (initPDescr<cIpAddress>(_sIpAddresses)) {
        _ixPortId        = _pRecordDescr->toIndex(_sPortId);
        _ixAddress       = _pRecordDescr->toIndex(_sAddress);
        _ixSubNetId      = _pRecordDescr->toIndex(_sSubNetId);
        _ixIpAddressType = _pRecordDescr->toIndex(_sIpAddressType);
        CHKENUM(_ixIpAddressType, addrType);
    }
    return *_pRecordDescr;
}

bool cIpAddress::rewrite(QSqlQuery &, eEx)
{
    EXCEPTION(ENOTSUPP);
    return false; // warning...
}

int cIpAddress::replace(QSqlQuery &, eEx)
{
    EXCEPTION(ENOTSUPP);
    return R_ERROR; // warning...
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
QString cIpAddress::lookup(const QHostAddress& ha, eEx __ex)
{
    QHostInfo hi = QHostInfo::fromName(ha.toString());
    if (hi.error() != QHostInfo::NoError) {
        if (__ex) EXCEPTION(EDATA, hi.error(), trUtf8("A host név nem állapítható meg a cím: '%1' alapján").arg(ha.toString()));
        return _sNul;
    }
    return hi.hostName();
}

QList<QHostAddress> cIpAddress::lookupAll(const QString& hn, eEx __ex)
{
    QHostInfo hi = QHostInfo::fromName(hn);
    if (hi.error() != QHostInfo::NoError) {
        if (__ex) EXCEPTION(EDATA, hi.error(), trUtf8("Az IP cím nem állapítható meg a host név: '%1' alapján; %2").arg(hn, hi.errorString()));
        return QList<QHostAddress>();
    }
    return hi.addresses();
}

QHostAddress cIpAddress::lookup(const QString& hn, eEx __ex)
{
    QList<QHostAddress> al = lookupAll(hn, __ex);
    int n = al.count();
    if (n != 1) {
        if (__ex && n > 1) {
            QString als;
            foreach (QHostAddress a, al) {
                als += a.toString() + ",";
            }
            als.chop(1);
            EXCEPTION(AMBIGUOUS, n, trUtf8("A hoszt cím a név: '%1' alapján nem egyértelmű : %2").arg(hn, als));
        }
        return QHostAddress();
    }
    return al.first();
}

QHostAddress cIpAddress::setIpByName(const QString& _hn, const QString& _t, eEx __ex)
{
    QHostAddress a = lookup(_hn, __ex);
    setAddress(a, _t);
    return a;
}

const QString& addrType(int __at, eEx __ex)
{
    switch (__at) {
    case AT_FIXIP:  return _sFixIp;
    case AT_DYNAMIC:return _sDynamic;
    case AT_PSEUDO: return _sPseudo;
    case AT_PRIVATE:return _sPrivate;
    case AT_EXTERNAL:return _sExternal;
    default: if (__ex) EXCEPTION(EDATA, __at);
    }
    return _sNul;
}

int addrType(const QString& __at, eEx __ex)
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

CRECCNTR(cPortParam)
CRECDEFD(cPortParam)

int cPortParam::_ixParamTypeId = NULL_IX;
int cPortParam::_ixPortId = NULL_IX;
const cRecStaticDescr&  cPortParam::descr() const
{
    if (initPDescr<cPortParam>(_sPortParams)) {
        _ixParamTypeId = _pRecordDescr->toIndex(_sParamTypeId);
        _ixPortId      = _pRecordDescr->toIndex(_sPortId);
        setNameKeyMask(_pRecordDescr, _pRecordDescr->mask(_ixParamTypeId, _ixPortId));
    }
    return *_pRecordDescr;
}

void    cPortParam::toEnd()
{
    cPortParam::toEnd(_ixParamTypeId);
}

bool cPortParam::toEnd(int i)
{
    if (i == _ixParamTypeId) {
        qlonglong id = _isNull(i) ? NULL_ID : variantToId(_get(i));
        if (id == NULL_ID) {
            paramType.clear();
        }
        else {
            QSqlQuery q = getQuery();
            if (!paramType.fetchById(q, id)) {
                _stat |= ES_DEFECTIVE;
                _setDefectivFieldBit(i);
            }
        }
        return true;
    }
    return false;
}

void    cPortParam::clearToEnd()
{
    paramType.clear();
}


/* ------------------------------ cIfType ------------------------------ */
DEFAULTCRECDEF(cIfType, _sIfTypes)

tRecordList<cIfType> cIfType::ifTypes;
cIfType *cIfType::pNull = NULL;

bool cIfType::insert(QSqlQuery &__q, eEx __ex)
{
    if (cRecord::insert(__q, __ex)) {
        ifTypes << *this;
        return true;
    }
    return false;
}

bool cIfType::update(QSqlQuery &__q, bool __only, const QBitArray &__set, const QBitArray &__where, eEx __ex)
{
    if (cRecord::update(__q, __only, __set, __where, __ex)) {
        cIfType *pIft = ifTypes.get(getId());
        pIft->set(*this);
        return true;
    }
    return false;
}

qlonglong cIfType::insertNew(QSqlQuery &__q, const QString& __nm, const QString& __no, int __iid, int __lid)
{
    cIfType it;
    it.setName(                __nm);
    it.setName(_sIfTypeNote,   __no);
    it.setId(  _sIfTypeIanaId, __iid);
    it.setId(  _sIanaIdLink,   __lid);
    it.insert(__q);
    return it.getId();
}

void cIfType::fetchIfTypes(QSqlQuery& __q)
{
    if (pNull == NULL) pNull = new cIfType();
    QBitArray   ba(1, false);   // Nem null, egyeseket nem tartalmazó maszk, minden rekord kiválasztásához
    if (ifTypes.count() == 0) {
        cIfType *p = new cIfType();
        if (0 == ifTypes.fetch(__q, true, ba, p)) EXCEPTION(EDATA, 0, trUtf8("Load cache: The if_types table is empty."));
        return;
    }
    // UPDATE
    int found = ifTypes.count();    // Megtalálandó rekordok száma
    int n = 0;                      // Megtalált rekordok számláló
    cIfType ift;
    if (!ift.fetch(__q, false, ba)) EXCEPTION(EDATA, 0, trUtf8("Update cache: The if_types table is empty."));
    do {
        cIfType *pIft = ifTypes.get(ift.getId(), EX_IGNORE);    // ID alapján rákeresünk
        if (pIft == NULL) {   // Ez egy új rekord
            ifTypes << ift;
        }
        else {                // Frissít
            pIft->set(ift);
            ++n;
        }
    } while (ift.next(__q));
    if (n < found) EXCEPTION(EPROGFAIL);
    if (n > found) EXCEPTION(EDATA, n - found, trUtf8("Deleted if_tpes record."));
}

const cIfType& cIfType::ifType(const QString& __nm, eEx __ex)
{
    checkIfTypes();
    int i = ifTypes.indexOf(_descr_cIfType().nameIndex(), QVariant(__nm));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, -1,QObject::trUtf8("Invalid iftype name %1 or program error.").arg(__nm));
        return *pNull;
    }
    return *(ifTypes[i]);
}

const cIfType& cIfType::ifType(qlonglong __id, eEx __ex)
{
    checkIfTypes();
    int i = ifTypes.indexOf(_descr_cIfType().idIndex(), QVariant(__id));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, __id,QObject::trUtf8("Invalid iftype id or program error."));
        return *pNull;
    }
    return *(ifTypes[i]);
}

const cIfType *cIfType::fromIana(int _iana_id)
{
    checkIfTypes();
    QList<cIfType *>::const_iterator    i;
    for (i = ifTypes.constBegin(); i < ifTypes.constEnd(); i++) {
        const cIfType *pift = *i;
        if (pift->getId(_sIfTypeIanaId) == _iana_id) {
            if (pift->isNull(_sIanaIdLink)) {
                if (pift->getBool(_sPreferred)) return pift;
            }
            else {
                return fromIana(pift->getId(_sIanaIdLink));
            }
        }
    }
    return NULL;
}

/* ------------------------------ cNPort ------------------------------ */

qlonglong cNPort::_tableoid_nports     = NULL_ID;
qlonglong cNPort::_tableoid_pports     = NULL_ID;
qlonglong cNPort::_tableoid_interfaces = NULL_ID;

cNPort::cNPort() : cRecord(), params(this)
{
    // DBGOBJ();
    _set(cNPort::descr());
}

cNPort::cNPort(const cNPort& __o) : cRecord(), params(this, __o.params)
{
    // DBGOBJ();
    __cp(__o);
    _copy(__o, _descr_cNPort());
    params.append(__o.params);
}

// -- virtual

CRECDEFNCD(cNPort)

/// A többi clone() metódushoz képest eltérés, hogy nem csak azonos típust (azonos descriptor) lehet másolni.
/// Megengedi a cPPort és cInterface típusból is a másolást, a többlet adatokat figyelmen kívül hagyja.
/// Ha __o eredeti typusa cRecordAny, akkor a params konténer üres lessz, mivel az __o nem tartalmazza.
cNPort& cNPort::clone(const cRecord& __o)
{
    if (!(__o.descr() >= descr())) EXCEPTION(EDATA, 0, __o.toString());
    clear();
    __cp(__o);
    set(__o);
    if (typeid(__o) != typeid(cRecordAny)) {
        params.append(dynamic_cast<const cNPort *>(&__o)->params);
    }
    return *this;
}

int cNPort::_ixNodeId = NULL_IX;
int cNPort::_ixPortIndex = NULL_IX;
int cNPort::_ixIfTypeId = NULL_IX;
const cRecStaticDescr&  cNPort::descr() const
{
    if (initPDescr<cNPort>(_sNPorts)) {
        _ixNodeId = _pRecordDescr->toIndex(_sNodeId);
        _ixPortIndex = _pRecordDescr->toIndex(_sPortIndex);
        _ixIfTypeId  = _pRecordDescr->toIndex(_sIfTypeId);
        _tableoid_nports     = _pRecordDescr->tableoid();
        _tableoid_pports     = cPPort().tableoid();
        _tableoid_interfaces = cInterface().tableoid();

    }
    return *_pRecordDescr;
}

void cNPort::clearToEnd()
{
    params.clear();
}

void cNPort::toEnd()
{
    toEnd(idIndex());
}

bool cNPort::toEnd(int i)
{
    if (idIndex() == i) {
        if (atEndCont(params, cPortParam::ixPortId())) {
            ;
        }
        return true;
    }
    return false;
}

/// A tulajdonos objektum containerValid értéke érdektelen (nem kütelező, hogy tulajdonosa (QObject::parent()) legyen).
bool cNPort::insert(QSqlQuery &__q, eEx __ex)
{
    bool r = cRecord::insert(__q, __ex);
    if (r) {
        return params.insert(__q, __ex) == params.size();
    }
    return false;
}

bool cNPort::rewrite(QSqlQuery &__q, eEx __ex)
{
   return tRewrite(__q, params, CV_PORT_PARAMS, __ex);
}

/// Feltételezi, hogy a parent (QObject *) pointer a tulajdonos objektumra mutat.
/// Ellenörzi, hogy a parent pointere konvertálható-e cPatch * típusű pinterré.
/// Ha nem, vagy a parent NULL, akkor kizárást dob.
/// A tulajdonos objektum containerValid adattagját maszkolja a __mask
/// paraméterrel, ha az eredmény nem null, akkor true értékkel tér vissza.
bool cNPort::isContainerValid(qlonglong __mask) const
{
    QObject *p = parent();
    if (p == NULL) EXCEPTION(EPROGFAIL, __mask);
    cPatch *pO = qobject_cast<cPatch *>(p);
    if (pO == NULL) EXCEPTION(EPROGFAIL, __mask);
    return pO->containerValid & __mask;
}

void cNPort::setContainerValid(qlonglong __set, qlonglong __clr)
{
    QObject *p = parent();
    if (p == NULL) EXCEPTION(EPROGFAIL);
    cPatch *pO = qobject_cast<cPatch *>(p);
    if (pO == NULL) EXCEPTION(EPROGFAIL);
    pO->containerValid = (pO->containerValid & ~__clr) | __set;
}


// --

bool cNPort::fetchPortByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id)
{
    clear();
    setId(_ixNodeId,  __node_id);
    setName(__port_name);
    int n = completion(__q);
    __q.finish();
    return n == 1;
}

qlonglong cNPort::getPortIdByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id, eEx ex)
{
    cNPort p;
    if (!p.fetchPortByName(__q, __port_name, __node_id) && ex) EXCEPTION(EDATA, __node_id, __port_name);
    return p.getId();
}

bool cNPort::fetchPortByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, eEx __ex)
{
    clear();
    // A patch a közös ős
    qlonglong nid = cPatch().getIdByName(__q, __node_name, __ex);
    if (nid == NULL_ID) return false;
    setId(_ixNodeId, nid);
    setName(nameIndex(), __port_name);
    int n = completion(__q);
    __q.finish();
    return n == 1;
}

qlonglong cNPort::getPortIdByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, eEx ex)
{
    cNPort p;
    if (!p.fetchPortByName(__q, __port_name, __node_name, ex) && ex)
        EXCEPTION(EDATA, -1, QString("%1:%2").arg(__node_name, __port_name));
    return p.getId();
}

bool cNPort::fetchPortByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id)
{
    clear();
    setId(_ixNodeId,   __node_id);
    setId(_ixPortIndex, __port_index);
    int n = completion(__q);
    __q.finish();
    return n == 1;
}

qlonglong cNPort::getPortIdByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id, eEx ex)
{
    cNPort p;
    if (!p.fetchPortByIndex(__q, __port_index, __node_id) && ex)
        EXCEPTION(EDATA, __port_index, QString::number(__node_id));
    return p.getId();
}

bool cNPort::fetchPortByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name, eEx __ex)
{
    clear();
    // A patch a közös ős
    qlonglong nid = cPatch().getIdByName(__q, __node_name, __ex);
    if (nid == NULL_ID) return false;
    setId(_ixNodeId, nid);
    setId(_ixPortIndex, __port_index);
    int n = completion(__q);
    __q.finish();
    return n == 1;
}

qlonglong cNPort::getPortIdByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name, eEx __ex)
{
    cNPort p;
    if (!p.fetchPortByIndex(__q, __port_index, __node_name, __ex) && __ex)
        EXCEPTION(EDATA, __port_index, __node_name);
    return p.getId();
}



cNPort * cNPort::newPortObj(const cIfType& __t)
{
    cNPort *r = NULL;
    QString portobjtype = __t.get(_sIfTypeObjType).toString();
    // PDEB(VVERBOSE) <<  __PRETTY_FUNCTION__ << " allocated: " << portobjtype << endl;
    if      (portobjtype == _sNPort)        r = new cNPort();
    else if (portobjtype == _sPPort)        r = new cPPort();
    else if (portobjtype == _sInterface)    r = new cInterface();
    else                                    EXCEPTION(EDATA, __t.getId(), portobjtype);
    r->set(_sIfTypeId, __t.getId());
    return r;
}

template<class P> static inline P * getPortObjByIdT(QSqlQuery& q, qlonglong  __id, eEx __ex)
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

cNPort * cNPort::getPortObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __port_id, eEx __ex)
{
    if (_tableoid_nports == NULL_ID) cNPort();
    if      (__tableoid == _tableoid_nports)     return getPortObjByIdT<cNPort>       (q, __port_id, __ex);
    else if (__tableoid == _tableoid_pports)     return getPortObjByIdT<cPPort>       (q, __port_id, __ex);
    else if (__tableoid == _tableoid_interfaces) return getPortObjByIdT<cInterface>   (q, __port_id, __ex);
    else                                        EXCEPTION(EDATA);
    return NULL;
}

cNPort * cNPort::getPortObjById(QSqlQuery& q, qlonglong __port_id, eEx __ex)
{
    qlonglong tableoid = cNPort().setId(__port_id).fetchTableOId(q, __ex);
    if (tableoid < 0LL) return NULL;
    return getPortObjById(q, tableoid, __port_id, __ex);
}

int cNPort::fetchParams(QSqlQuery& q)
{
    setContainerValid(CV_PORT_PARAMS);
    return params.fetch(q);
}

QString cNPort::getFullName(QSqlQuery& q, eEx __ex)
{
    return cNode().getNameById(q, getId(_ixNodeId), __ex) + ':' + getName();
}

/* ------------------------------ cPPort ------------------------------ */

 int portShare(const QString& _n, eEx __ex)
 {
     if (_n.isEmpty())                                return ES_;
     if (0 == _n.compare(_sA,   Qt::CaseInsensitive)) return ES_A;
     if (0 == _n.compare(_sAA,  Qt::CaseInsensitive)) return ES_AA;
     if (0 == _n.compare(_sAB,  Qt::CaseInsensitive)) return ES_AB;
     if (0 == _n.compare(_sB,   Qt::CaseInsensitive)) return ES_B;
     if (0 == _n.compare(_sBA,  Qt::CaseInsensitive)) return ES_BA;
     if (0 == _n.compare(_sBB,  Qt::CaseInsensitive)) return ES_BB;
     if (0 == _n.compare(_sC,   Qt::CaseInsensitive)) return ES_C;
     if (0 == _n.compare(_sD,   Qt::CaseInsensitive)) return ES_D;
     if (0 == _n.compare(_sNC,  Qt::CaseInsensitive)) return ES_NC;
     if (__ex) EXCEPTION(EDATA, -1, _n);
     return ES_INVALID;
 }

const QString& portShare(int _i, eEx __ex)
{
    switch (_i) {
    case ES_:   return _sNul;
    case ES_A:  return _sA;
    case ES_AA: return _sAA;
    case ES_AB: return _sAB;
    case ES_B:  return _sB;
    case ES_BA: return _sBA;
    case ES_BB: return _sBB;
    case ES_C:  return _sC;
    case ES_D:  return _sD;
    case ES_NC: return _sNC;
    default: if (__ex) EXCEPTION(EDATA, _i);
    }
    return _sNul;   // !!

}

cPPort::cPPort() : cNPort(_no_init_)
{
    //_DBGFN() << VDEBPTR(this) << endl;
    _set(cPPort::descr());
     setDefaultType();
}

cPPort::cPPort(const cPPort& __o) : cNPort(_no_init_)
{
    __cp(__o);
    _copy(__o, _descr_cPPort());
    params.append(__o.params);
}

// -- virtual
qlonglong       cPPort::_ifTypePatch = NULL_ID;
const cRecStaticDescr&  cPPort::descr() const
{
    if (initPDescr<cPPort>(_sPPorts)) {
        CHKENUM(_sSharedCable, portShare);
        _ifTypePatch = cIfType::ifTypeId(_sPatch); // Minden portnak ez a típusa
    }
    return *_pRecordDescr;
}
CRECDEFNCD(cPPort)

cPPort& cPPort::clone(const cRecord&__o)
{
    copy(__o);  // Ellenörzi az adat típust (descriptor)
    if (typeid(__o) != typeid(cRecordAny)) {    // A cRecordAny -ban nincs konténer, nem konvertálható
        params.clear().append(dynamic_cast<const cPPort *>(&__o)->params);
    }
    return *this;
}

/* ------------------------------ cInterface ------------------------------ */

cInterface::cInterface()
    : cNPort(_no_init_)
    , vlans(this)
    , addresses(this)
    , trunkMembers()
{
//    DBGOBJ();
    _set(cInterface::descr());
}
cInterface::cInterface(const cInterface& __o)
    : cNPort(_no_init_)
    , vlans(this, __o.vlans)
    , addresses(this, __o.addresses)
    , trunkMembers()
{
//    DBGOBJ();
    __cp(__o);
    _copy(__o, _descr_cInterface());
    params.append(__o.params);
    vlans.append(__o.vlans);
    addresses.append(__o.addresses);
    trunkMembers = __o.trunkMembers;
}
// -- virtual
int cInterface::_ixHwAddress = NULL_IX;
const cRecStaticDescr&  cInterface::descr() const
{
    if (initPDescr<cInterface>(_sInterfaces)) {
        _ixHwAddress = _pRecordDescr->toIndex(_sHwAddress);
        CHKENUM(_sPortOStat, ifStatus);
    }
    return *_pRecordDescr;
}

CRECDEFNCD(cInterface)

cInterface& cInterface::clone(const cRecord &__o)
{
    copy(__o);
    params.clear();
    vlans.clear();
    addresses.clear();
    trunkMembers.clear();
    if (typeid(cRecordAny) != typeid(__o)) {
        const cInterface& r = *dynamic_cast<const cInterface*>(&__o);
        params.append(r.params);
        vlans.append(r.vlans);
        addresses.append(r.addresses);
        trunkMembers = r.trunkMembers;
    }
    return *this;
}

void cInterface::clearToEnd()
{
    cNPort::clearToEnd();
    vlans.clear();
    trunkMembers.clear();
}

void cInterface::toEnd()
{
    cNPort::toEnd();
    cInterface::toEnd(idIndex());
}

bool cInterface::toEnd(int i)
{
    if (idIndex() == i) {
        bool f;
        f = atEndCont(params,    cPortParam::ixPortId());
        f = atEndCont(vlans,     cPortVlan::ixPortId())       || f;
        f = atEndCont(addresses, cIpAddress::ixPortId())      || f;
        if (f) trunkMembers.clear();    // Pontatlan, de nincs elég adat ...
        return true;
    }
    return false;
}

bool cInterface::insert(QSqlQuery &__q, eEx __ex)
{
    if (!(cNPort::insert(__q, __ex) && (trunkMembers.size() == updateTrunkMembers(__q, __ex)))) return false;
    bool r = true;
    r =     vlans.insert(__q, __ex) ==     vlans.size() && r;
    r = addresses.insert(__q, __ex) == addresses.size() && r;
    return r;
}

/// Az cInterface nem önálló objektum.
/// A Hívása elött a cNode törli az összes IP cím rekordot, igy az IP címekhet nem a replace() hanem az insert() metódust kell hívni,
bool cInterface::rewrite(QSqlQuery &__q, eEx __ex)
{
    bool r = cNPort::rewrite(__q, __ex) && (trunkMembers.size() == updateTrunkMembers(__q, __ex));
    if (!r) return false;
    r = vlans.replace(__q, __ex);
    if (!r) return false;
    if (addresses.count() > 0) {
        r = 0 < addresses.insert(__q, __ex);
    }
    return r;
}

QString cInterface::toString() const
{
    return cRecord::toString() + " @addresses = " + addresses.toString() + " ";
}

int cInterface::updateTrunkMembers(QSqlQuery& q, eEx __ex)
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
    cPortVlan *pv = vlans.get(cPortVlan::ixVlanId(), __id, EX_IGNORE);
    if (pv == NULL) vlans << (pv = new cPortVlan());
    pv->setVlanId(__id);
    pv->setVlanType(__t);
    pv->setSetType(__st);
    if (!isNull(idIndex())) pv->setPortId(getId());
}

bool cInterface::splitVlan(qlonglong __id)
{
    cPortVlan * p = vlans.pull(__id, EX_IGNORE);
    if (p == NULL) return false;
    delete p;
    return true;
}

int cInterface::fetchByMac(QSqlQuery& q, const cMac& a)
{
    clear();
    set(_sHwAddress, QVariant::fromValue(a));
    return completion(q);
}

bool cInterface::fetchByIp(QSqlQuery& q, const QHostAddress& a)
{
    clear();
    QString sql = "SELECT interfaces.* FROM interfaces JOIN ipaddresses USING(port_id) WHERE address = ?";
    if (execSql(q, sql, a.toString())) {
        set(q);
        return true;
    }
    return false;
}

int cInterface::fetchVlans(QSqlQuery& q)
{
    return vlans.fetch(q, getId());
}

int cInterface::fetchAddressess(QSqlQuery& q)
{
    return addresses.fetch(q, getId());
}

cIpAddress& cInterface::addIpAddress(const cIpAddress& __a)
{
    _DBGFN() << " @(" << __a.toString() << ")" << endl;
    if (addresses.indexOf(cIpAddress::ixAddress(), __a.get(cIpAddress::ixAddress()))) EXCEPTION(EDATA);
    addresses << __a;
    PDEB(VERBOSE) << QObject::trUtf8("Added, size : %1").arg(addresses.size());
    return *addresses.last();
}

cIpAddress& cInterface::addIpAddress(const QHostAddress& __a, const QString& __t, const QString &__d)
{
    // _DBGFN() << " @(" << __a.toString() << ")" << endl;
    // if (addresses.indexOf(cIpAddress::_ixAddress, __a.get(cIpAddress::_ixAddress))) EXCEPTION(EDATA);
    cIpAddress *p = new cIpAddress();
    *p = __a;
    p->setName(_sIpAddressType, __t);
    p->setName(_sIpAddressNote, __d);
    addresses <<  p;
    //PDEB(VERBOSE) << "Added : " << p->toString() << " size : " << addresses.size();
    return *p;
}

int ifStatus(const QString& _n, enum eEx __ex)
{
    if (0 == _n.compare(_sUp,             Qt::CaseInsensitive)) return cInterface::UP;
    if (0 == _n.compare(_sDown,           Qt::CaseInsensitive)) return cInterface::DOWN;
    if (0 == _n.compare(_sTesting,        Qt::CaseInsensitive)) return cInterface::TESTING;
    if (0 == _n.compare(_sUnknown,        Qt::CaseInsensitive)) return cInterface::UNKNOWN;
    if (0 == _n.compare(_sDormant,        Qt::CaseInsensitive)) return cInterface::DORMANT;
    if (0 == _n.compare(_sNotPresent,     Qt::CaseInsensitive)) return cInterface::NOTPRESENT;
    if (0 == _n.compare(_sLowerLayerDown, Qt::CaseInsensitive)) return cInterface::LOWERLAYERDOWN;
    if (0 == _n.compare(_sInvert,         Qt::CaseInsensitive)) return cInterface::IA_INVERT;
    if (0 == _n.compare(_sShort,          Qt::CaseInsensitive)) return cInterface::IA_SHORT;
    if (0 == _n.compare(_sBroken,         Qt::CaseInsensitive)) return cInterface::IA_BROKEN;
    if (0 == _n.compare(_sError,          Qt::CaseInsensitive)) return cInterface::IA_ERROR;
    if (__ex) EXCEPTION(EDATA, -1, _n);
    return cInterface::PS_INVALID;
}

const QString& ifStatus(int _i, enum eEx __ex)
{
    switch (_i) {
    case cInterface::UP:                return _sUp;
    case cInterface::DOWN:              return _sDown;
    case cInterface::TESTING:           return _sTesting;
    case cInterface::UNKNOWN:           return _sUnknown;
    case cInterface::DORMANT:           return _sDormant;
    case cInterface::NOTPRESENT:        return _sNotPresent;
    case cInterface::LOWERLAYERDOWN:    return _sLowerLayerDown;
    case cInterface::IA_INVERT:         return _sInvert;
    case cInterface::IA_SHORT:          return _sShort;
    case cInterface::IA_BROKEN:         return _sBroken;
    case cInterface::IA_ERROR:          return _sError;
    default: if (__ex) EXCEPTION(EDATA, _i);
    }
    return _sNul;
}


/* ****************************** NODES (patch => nodes => snmphosts ****************************** */
/* ----------------------------- PATCHS : cPatch ----------------------------- */


cShareBack& cShareBack::set(int __a, int __b, int __ab, int __bb, bool __cd)
{
    a = __a;
    b = __b;
    ab = __ab;
    bb = __bb;
    if ((a  < 0 || (a == b || a == ab || a == bb))
     || (b >= 0 && (          b == ab || b == bb))
     || (ab>= 0 &&                      ab == bb ))
        EXCEPTION(EDATA);
    cd = __cd;
    return *this;
}

bool cShareBack::operator==(const cShareBack& __o) const
{
    return (*this) == __o.a || (*this) == __o.b || (*this) == __o.ab || (*this) == __o.bb;
}

bool cShareBack::operator==(int __i) const
{
    if (__i < 0) return false;
    return __i == a || __i == b || __i == ab || __i == bb;
}

/* -------------------------------------------------------------------------- */

cPatch::cPatch()
    : cRecord()
    , ports(this)
    , params(this)
    , pShares(new QSet<cShareBack>)
{
    DBGOBJ();
    containerValid = 0;
    _set(cPatch::descr());
}

cPatch::cPatch(const cPatch& __o)
    : cRecord()
    , ports(this, __o.ports)
    , params(this, __o.params)
    , pShares(new QSet<cShareBack>)
{
    DBGOBJ();
    __cp(__o);
    _copy(__o, _descr_cPatch());
    containerValid = __o.containerValid;
    if (__o.pShares == NULL) EXCEPTION(EPROGFAIL,0,__o.toString());
    *pShares = *__o.pShares;
}

CRECDEFNCD(cPatch)

cPatch& cPatch::clone(const cRecord &__o)
{
    copy(__o);
    if (typeid(cRecordAny) != typeid(__o)) {
        const cPatch& r = *dynamic_cast<const cPatch*>(&__o);
        containerValid = r.containerValid;
        params.clear().append(r.params);
        ports.clear().append(r.ports);
        if (r.pShares == NULL) EXCEPTION(EPROGFAIL,0,__o.toString());
        *pShares = *r.pShares;
    }
    return *this;
}

const cRecStaticDescr&  cPatch::descr() const
{
    if (initPDescr<cPatch>(_sPatchs)) {
        CHKENUM(_sNodeType, nodeType);
    }
    return *_pRecordDescr;
}

void cPatch::clearToEnd()
{
    ports.clear();
    params.clear();
    if (pShares != NULL) clearShares();
    containerValid &= ~(CV_NODE_PARAMS | CV_PORTS);
}

void cPatch::toEnd()
{
    toEnd(idIndex());
}

bool cPatch::toEnd(int i)
{
    if (i == idIndex()) {
        if (atEndCont(ports, _sNodeId)) {
            params.clear();
            containerValid &= ~(CV_NODE_PARAMS | CV_PORTS);
            if (pShares != NULL) clearShares();
        }
        return true;
    }
    return false;
}

bool cPatch::insert(QSqlQuery &__q, eEx __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (params.count()) {
        if (params.insert(__q, __ex) != params.count()) return false;
    }
    if (ports.count()) {
        if (ports.insert(__q, __ex) != ports.count()) return false;
        if (pShares == NULL) return true;
        return updateShares(__q, false, __ex);
    }
    return true;
}

bool cPatch::rewrite(QSqlQuery &__q, eEx __ex)
{
    bool r = tRewrite(__q, ports, CV_PORTS, params, CV_NODE_PARAMS, __ex);
    if (!r) return false;
    if (pShares == NULL) return true;
    return updateShares(__q, false, __ex);
}

bool cPatch::isContainerValid(qlonglong __mask) const
{
    return containerValid & __mask;
}

void cPatch::setContainerValid(qlonglong __set, qlonglong __clr)
{
    containerValid = (containerValid & ~__clr) | __set;
}

int cPatch::fetchPorts(QSqlQuery& __q)
{
    int n = 0;
    // A ports objektum fetch metódusa csak azonos rekord típusok esetén jó. Igaz, hogy egy típus van, de az nam az alap típus.
    cPPort p;
    p.setId(ports.ixOwnerId, getId());
    if (p.completion(__q)) do {
        ports << p;
        ++n;
    } while (__q.next());
    containerValid |= CV_PORTS;
    return n;
}

int cPatch::fetchParams(QSqlQuery& q)
{
    containerValid |= CV_NODE_PARAMS;
    return params.fetch(q);
}

void cPatch::setParam(const QString& __par, const QVariant& __val)
{
    qlonglong typeId = cParamType::paramTypeId(__par);
    cNodeParam *param = params.get(_sParamTypeId, typeId);
    if (param == NULL) {
        param = new cNodeParam();
        param->setId(_sParamTypeId, typeId);
        params << param;
    }
    param->value() = __val;
}

void cPatch::clearShares()
{
    shares().clear();
}

bool cPatch::setShare(int __a, int __ab, int __b, int __bb, bool __cd)
{
    // Csak létező portra lehet megosztást csinálni
    if ((             !isContIx(ports, __a))
     || (__b  >= 0 && !isContIx(ports, __b))
     || (__ab >= 0 && !isContIx(ports, __ab))
     || (__bb >= 0 && !isContIx(ports, __bb))) return false;
    // A konstruktor is ellenőriz, egy port csak egyszer mind nem lehet negatív, ha mégis dob egy kizárást.
    cShareBack  s(__a, __b, __ab, __bb, __cd);
    // Egy port csak egy megosztásban szerepelhet
    if (shares().contains(s)) return false;
    shares() << s;
    return true;
}
bool cPatch::updateShares(QSqlQuery& __q, bool __clr, eEx __ex)
{
    if (__clr) {
        QString sql = "UPDATE pports SET shared_cable = DEFAULT, shared_port_id = DEFAULT WHERE port_id = " + QString::number(getId());
        if (__q.exec(sql)) {
            if (__ex == EX_IGNORE && __q.lastError().type() == QSqlError::StatementError) SQLQUERYERRDEB(__q)
            else                                                                      SQLQUERYERR(__q)
            return false;
        }
    }
    if (shares().isEmpty()) return true;
    int sci = DESCR(cPPort).toIndex(_sSharedCable);
    int spi = DESCR(cPPort).toIndex(_sSharedPortId);
    QBitArray um = _bits(sci, spi);
    QBitArray wm = DESCR(cPPort).primaryKey();
    QString ss;
    foreach (const cShareBack& s, shares()) {
        qlonglong pid = ports[s.getA()]->getId();
        cPPort *p = ports[s.getA()]->reconvert<cPPort>();   // A bázis port A vagy AA megosztás
        p->clear(spi);
        if (s.isNC()) {     // NC
            if (!p->setName(sci, _sNC ).update(__q, true, um, wm, __ex)) return false;
            return true;
        }
        if (s.getAB() < 0) { // A
            if (!p->setName(sci, _sA ).update(__q, true, um, wm, __ex)) return false;
        }
        else {                      // AA, AB / C
            if (!p->setName(sci, _sAA).update(__q, true, um, wm, __ex)) return false;
            p = ports[s.getAB()]->reconvert<cPPort>();
            ss = s.isCD() ? _sC : _sAB;
            if (!p->setName(sci, ss).setId(spi, pid).update(__q, true, um, wm, __ex)) return false;
        }                           // B , BA / D
        if (s.getB() >= 0) {
            p = ports[s.getB()]->reconvert<cPPort>();
            ss = s.getBB() < 0 ? _sB : s.isCD() ? _sD : _sBA;
            if (!p->setName(sci, ss).setId(spi, pid).update(__q, true, um, wm, __ex)) return false;
        }
        if (s.getBB() >= 0) { // BB
            p = ports[s.getB()]->reconvert<cPPort>();
            if (!p->setName(sci, _sBB).setId(spi, pid).update(__q, true, um, wm, __ex)) return false;
        }
    }
    return true;
}

cPPort *cPatch::addPort(const QString& __name, const QString& __note, int __ix)
{
    if (ports.count()) {
        if (0 <= ports.indexOf(__name))
            EXCEPTION(EDATA, -1, trUtf8("Ilyen port név már létezik: %1").arg(__name));
        if (0 <= ports.indexOf(cNPort::_ixPortIndex, QVariant(__ix)))
            EXCEPTION(EDATA, __ix, trUtf8("Ilyen port index már létezik."));
    }
    cPPort  *p = new cPPort();
    p->setName(__name);
    p->setId(cNPort::ixPortIndex(), __ix);
    p->setName(_sPortNote, __note);
    ports << p;
    return p;
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

cNPort *cPatch::portSet(const QString& __name, const QString& __fn, const QVariant& __v)
{
    int i = ports.indexOf(cNPort().nameIndex(), __name);
    if (i < 0) EXCEPTION(ENONAME, -1, QString(QObject::trUtf8("%1 port name not found.")).arg(__name));
    cNPort * p = ports[i];
    p->set(__fn, __v);
    return p;
}

cNPort *cPatch::portSet(int __ix, const QString& __fn, const QVariant& __v)
{
    int i = ports.indexOf(cNPort::ixPortIndex(), __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::trUtf8("Port index not found"));
    cNPort *p = ports[i];
    p->set(__fn, __v);
    return p;
}

cNPort *cPatch::portSet(int __ix, const QString& __fn, const QVariantList& __vl)
{
    int ix = __ix;
    cNPort *p = NULL;
    foreach (const QVariant& v, __vl) {
        p = portSet(ix, __fn, v);
        ++ix;
    }
    return p;
}

cNPort *cPatch::portSetParam(cNPort *port, const QString& __par, const QVariant& __val)
{
    qlonglong typeId = cParamType::paramTypeId(__par);
    cPortParam *param = port->params.get(_sParamTypeId, typeId);
    if (param == NULL) {
        param = new cPortParam();
        param->setId(_sParamTypeId, typeId);
        port->params << param;
    }
    param->value() = __val;
    return port;
}

cNPort *cPatch::portSetParam(int __ix, const QString& __par, const QVariantList &__val)
{
    cNPort *p = NULL;
    int ix = __ix;
    foreach (const QVariant& v, __val) {
        p = portSetParam(ix, __par, v);
        ++ix;
    }
    return p;
}

cNPort * cPatch::getPort(int __ix, eEx __ex)
{
    if (__ix != NULL_IX) {
        int i = ports.indexOf(cNPort::ixPortIndex(), __ix);
        if (i >= 0) return ports[i];
    }
    if (__ex) EXCEPTION(ENONAME, -1, QString(QObject::trUtf8("%1 port index not found.")).arg(__ix))
    return NULL;
}

cNPort * cPatch::getPort(const QString& __pn, eEx __ex)
{
    int i = ports.indexOf(cNPort().nameIndex(), __pn);
    if (i >= 0) return ports[i];
    if (__ex) EXCEPTION(ENONAME, -1, QString(QObject::trUtf8("%1 port name not found.")).arg(__pn));
    return NULL;
}

cPatch * cPatch::getNodeObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __node_id, eEx __ex)
{
    if      (__tableoid == cPatch().      tableoid()) return getObjByIdT<cPatch>     (q, __node_id, __ex);
    else if (__tableoid == cNode().       tableoid()) return getObjByIdT<cNode>      (q, __node_id, __ex);
    else if (__tableoid == cSnmpDevice(). tableoid()) return getObjByIdT<cSnmpDevice>(q, __node_id, __ex);
    else if (__ex)                                    EXCEPTION(EDATA);
    return NULL;
}

cPatch * cPatch::getNodeObjById(QSqlQuery& q, qlonglong __node_id, eEx __ex)
{
    qlonglong tableoid = cPatch().setId(__node_id).fetchTableOId(q, __ex);
    if (tableoid < 0LL) return NULL;
    return getNodeObjById(q, tableoid, __node_id, __ex);
}

/* --------------------------------------------------------------------------- */

CRECCNTR(cNodeParam)
CRECDEFD(cNodeParam)

int cNodeParam::_ixParamTypeId = NULL_IX;
int cNodeParam::_ixNodeId = NULL_IX;
const cRecStaticDescr&  cNodeParam::descr() const
{
    if (initPDescr<cNodeParam>(_sNodeParams)) {
        _ixParamTypeId = _pRecordDescr->toIndex(_sParamTypeId);
        _ixNodeId      = _pRecordDescr->toIndex(_sNodeId);
        setNameKeyMask(_pRecordDescr, _pRecordDescr->mask(_ixParamTypeId, _ixNodeId));
    }
    return *_pRecordDescr;
}

void    cNodeParam::toEnd()
{
    cNodeParam::toEnd(_ixParamTypeId);
}

bool cNodeParam::toEnd(int i)
{
    if (i == _ixParamTypeId) {
        qlonglong id = _isNull(i) ? NULL_ID : variantToId(_get(i));
        if (id == NULL_ID) {
            paramType.clear();
        }
        else {
            QSqlQuery q = getQuery();
            if (!paramType.fetchById(q, id)) {
                _stat |= ES_DEFECTIVE;
                _setDefectivFieldBit(i);
            }
        }
        return true;
    }
    return false;
}

void    cNodeParam::clearToEnd()
{
    paramType.clear();
}

/* ------------------------------ NODES : cNode ------------------------------ */

int nodeType(const QString& __n, eEx __ex)
{
    if (0 == __n.compare(_sPatch,       Qt::CaseInsensitive)) return NT_PATCH;
    if (0 == __n.compare(_sNode,        Qt::CaseInsensitive)) return NT_NODE;
    if (0 == __n.compare(_sHost,        Qt::CaseInsensitive)) return NT_HOST;
    if (0 == __n.compare(_sSwitch,      Qt::CaseInsensitive)) return NT_SWITCH;
    if (0 == __n.compare(_sHub,         Qt::CaseInsensitive)) return NT_HUB;
    if (0 == __n.compare(_sVirtual,     Qt::CaseInsensitive)) return NT_VIRTUAL;
    if (0 == __n.compare(_sSnmp,        Qt::CaseInsensitive)) return NT_SNMP;
    if (0 == __n.compare(_sConverter,   Qt::CaseInsensitive)) return NT_CONVERTER;
    if (0 == __n.compare(_sPrinter,     Qt::CaseInsensitive)) return NT_PRINTER;
    if (0 == __n.compare(_sGateway,     Qt::CaseInsensitive)) return NT_GATEWAY;
    if (__ex != EX_IGNORE)   EXCEPTION(EDATA, -1, __n);
    return NT_INVALID;
}

const QString& nodeType(int __e, eEx __ex)
{
    switch (__e) {
    case NT_PATCH:      return _sPatch;
    case NT_NODE:       return _sNode;
    case NT_HOST:       return _sHost;
    case NT_SWITCH:     return _sSwitch;
    case NT_HUB:        return _sHub;
    case NT_VIRTUAL:    return _sVirtual;
    case NT_SNMP:       return _sSnmp;
    case NT_CONVERTER:  return _sConverter;
    case NT_PRINTER:    return _sPrinter;
    case NT_GATEWAY:    return _sGateway;
    }
    if (__ex != EX_IGNORE)   EXCEPTION(EDATA, __e);
    return _sNul;
}


cNode::cNode() : cPatch(_no_init_)
{
//  DBGOBJ();
    _set(cNode::descr());
}

cNode::cNode(const cNode& __o) : cPatch(_no_init_)
{
//  DBGOBJ();
    __cp(__o);
    _copy(__o, _descr_cNode());
    if (__o.pShares != NULL) EXCEPTION(EPROGFAIL,0,__o.toString());
    ports.append(__o.ports);
    params.append(__o.params);
}

/// A többi clone() metódushoz képest eltérés, hogy nem csak azonos típust (azonos descriptor) lehet másolni.
/// Megengedi a cSnmpDevice típusból is a másolást, a többlet adatokat figyelmen kívül hagyja.
cNode& cNode::clone(const cRecord &__o)
{
    if (!(__o.descr() >= descr())) EXCEPTION(EDATA, 0, __o.toString());
    clear();
    __cp(__o);
    set(__o);
    if (typeid(__o) != typeid(cRecordAny)) {
        const cNode& r = *dynamic_cast<const cNode*>(&__o);
        ports.append(r.ports);
        params.append(r.params);
    }
    return *this;
}

cNode::cNode(const QString& __name, const QString& __descr) : cPatch(_no_init_)
{
//    DBGOBJ();
    _set(cNode::descr());
    _set(_descr_cNode().nameIndex(),  __name);
    _set(_descr_cNode().noteIndex(), __descr);
}

// -- virtual
const cRecStaticDescr&  cNode::descr() const
{
    if (initPDescr<cNode>(_sNodes)) {
        CHKENUM(_sNodeStat, notifSwitch);
    }
    return *_pRecordDescr;
}

void cNode::clearShares()                                       { EXCEPTION(ENOTSUPP); }
bool cNode::setShare(int, int, int , int, bool)                 { EXCEPTION(ENOTSUPP); return false; }
bool cNode::updateShares(QSqlQuery&, bool, eEx)                 { EXCEPTION(ENOTSUPP); return false; }
cPPort *cNode::addPort(const QString&, const QString &, int)    { EXCEPTION(ENOTSUPP); return NULL; }
cPPort *cNode::addPorts(const QString&, int , int , int , int ) { EXCEPTION(ENOTSUPP); return NULL; }

CRECDEFNCD(cNode)

bool cNode::insert(QSqlQuery &__q, eEx __ex)
{
    int ixNodeType = toIndex(_sNodeType);
    if (isNull(ixNodeType)) {
        qlonglong nt = enum2set(NT_NODE);
        QListIterator<cNPort *> i(ports);
        while (i.hasNext()) {
            const cNPort *pp = i.next();
            if (pp->tableoid() == cInterface::_descr_cInterface().tableoid()) {
                const cInterface *pi = (const cInterface *)pp;
                if (!pi->addresses.isEmpty()) {
                    nt = enum2set(NT_HOST);
                    break;
                }
            }
        }
        setId(ixNodeType, nt);
    }
    return cPatch::insert(__q, __ex);
}

bool cNode::rewrite(QSqlQuery &__q, eEx __ex)
{
    // Töröljők az összes régi IP címet (IP címekre nincs replace/rewrite metódus)
    QString sql = "DELETE FROM ipaddresses WHERE port_id IN "
            "(SELECT port_id FROM nports WHERE node_id = ?)";
    qlonglong id = cNode().getIdByName(__q, getName());
    execSql(__q, sql, id);

    if (!tRewrite(__q, ports, CV_PORTS, params, CV_NODE_PARAMS, __ex)) return false;
    return true;
}

int  cNode::fetchPorts(QSqlQuery& __q)
{
    ports.clear();
    // A ports objektum fetch metódusa csak azonos rekord típusok esetén jó...
    QSqlQuery q = getQuery(); // A copy construktor vagy másolás az nem jó!! (shadow copy)
    QString sql = "SELECT tableoid, port_id FROM nports WHERE node_id = ? AND NOT deleted";
    if (execSql(__q, sql, getId())) do {
        qlonglong tableoid = variantToId(__q.value(0));
        qlonglong port_id  = variantToId(__q.value(1));
        cNPort *p = cNPort::getPortObjById(q, tableoid, port_id, EX_ERROR);
        q.finish();
        if (p == NULL) return -1;
        if (tableoid == cNPort::tableoid_interfaces()) {
            cInterface *pi = p->reconvert<cInterface>();
            pi->fetchAddressess(q);
            pi->fetchVlans(q);
            q.finish();
        }
        ports << p;
    } while (__q.next());
    __q.finish();
    return ports.count();
}

qlonglong cNode::getIdByName(QSqlQuery& __q, const QString& __n, eEx __ex) const
{
    qlonglong id = descr().getIdByName(__q, __n, EX_IGNORE);
    if (id != NULL_ID) return id;
    QString sql =
            "SELECT nodes.node_id FROM nodes WHERE "
                "'host' = ANY (node_type) AND "
                "node_name IN "
                    "( SELECT ? || '.' || param_value FROM sys_params JOIN param_types USING(param_type_id) WHERE param_type_name = ? ORDER BY sys_param_name ) "
                "LIMIT 1";
    __q.bindValue(1,_sSearchDomain);
    __q.bindValue(0,__n);
    if (!__q.exec()) SQLQUERYERR(__q);
    if (__q.first()) return __q.value(0).toLongLong();
    if (__ex) EXCEPTION(EFOUND,0,__n);
    return NULL_ID;
}

QString cNode::toString() const
{
    QString r;
    r  = cRecord::toString();
    r += " @ports = ";
    r += ports.toString();
    return r + " ";
}

cNPort *cNode::addPort(const cIfType& __t, const QString& __name, const QString& __note, int __ix)
{
    if (ports.count()) {
        if (NULL != getPort(__name, EX_IGNORE)) EXCEPTION(EDATA, -1, QString("Ilyen port név már létezik: %1").arg(__name));
        if (__ix != NULL_IX && NULL != getPort(__ix, EX_IGNORE)) EXCEPTION(EDATA, __ix, QString("Ilyen port index már létezik."));
    }
    cNPort  *p = cNPort::newPortObj(__t);
    p->setName(__name);
    if (__ix != NULL_IX) p->setId(_sPortIndex, __ix);
    p->setName(_sPortNote, __note);
    ports << p;
    return p;
}

cNPort *cNode::addPorts(const cIfType& __t, const QString& __np, int __noff, int __from, int __to, int __off)
{
    cNPort *p = NULL;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(__t, nameAndNumber(__np, i + __noff), _sNul, i + __off);
    }
    if (p == NULL) EXCEPTION(EDATA);
    return p;
}

cNPort *cNode::portModName(int __ix, const QString& __name, const QString& __note)
{
    int i = ports.indexOf(cNPort::ixPortIndex(), __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::trUtf8("Port index not found"));
    cNPort *p = ports[i];
    p->setName(__name);
    if (!__note.isNull()) p->setName(_sPortNote, __note);
    return p;
}

cNPort *cNode::portModType(int __ix, const QString& __type, const QString& __name, const QString& __note, int __new_ix)
{
    int i = ports.indexOf(cNPort::ixPortIndex(), __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::trUtf8("Port index not found"));
    cNPort *p = ports[i];
    const cIfType& ot = cIfType::ifType(p->getId(_sIfTypeId));
    const cIfType& nt = cIfType::ifType(__type);
    if (nt.getName(_sIfTypeObjType) != ot.getName(_sIfTypeObjType))
        EXCEPTION(EDATA, -1, trUtf8("Invalid port type: %1 to %2").arg(ot.getName().arg(__type)));
    p->setId(_sIfTypeId, nt.getId());
    p->setName(__name);
    if (!__note.isEmpty()) p->setName(_sPortNote, __note);
    if (__new_ix >= 0)     p->setId(_sPortIndex, __new_ix);
    return p;
}

cNPort *cNode::addSensors(const QString& __np, int __noff, int __from, int __to, int __off, const QHostAddress& __pip)
{
    const cIfType&  t = cIfType::ifType(_sSensor);
    QStringList a = __pip.toString().split(QChar('.'));
    if (a.size() != 4) EXCEPTION(EDATA);
    bool ok;
    int d = a[3].toInt(&ok);
    if (!ok) EXCEPTION(EDATA);
    cNPort *p = NULL;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(t, nameAndNumber(__np, i + __noff), _sNul, i + __off);
        cInterface *pIf = p->reconvert<cInterface>();
        a[3] = QString::number(d++);
        QHostAddress ha(a.join(QChar('.')));
        if (ha.isNull()) EXCEPTION(EDATA);
        pIf->addIpAddress(ha, _sPseudo);
    }
    return p;
}

bool cNode::fetchByIp(QSqlQuery& q, const QHostAddress& a, eEx __ex)
{
    clear();
    QString sql = QString("SELECT DISTINCT %1.* FROM %1 JOIN interfaces USING(node_id) JOIN ipaddresses USING(port_id) WHERE address = ?").arg(tableName());
    QString as = hostAddressToString(a);
    if (execSql(q, sql, as)) {
        set(q);
        if (q.next()) {
            if (__ex) EXCEPTION(AMBIGUOUS,0, trUtf8("A %1 IP nem egyedi").arg(a.toString()));
            return false;
        }
        return true;
    }
    if (__ex) EXCEPTION(EFOUND,0, trUtf8("A %1 IP.").arg(a.toString()));
    return false;
}

bool cNode::fetchOnePortByIp(QSqlQuery& q, const QHostAddress& a, eEx __ex)
{
    ports.clear();
    QString sql =
            "SELECT interfaces.*, ipaddresses.*  FROM interfaces JOIN ipaddresses USING(port_id)"
            " WHERE address = ? AND node_id = ?";
    QString as = hostAddressToString(a);
    if (execSql(q, sql, as, getId(_sNodeId))) {
        int from = 0;
        cInterface *pIf = new cInterface();
        cIpAddress *pIp = new cIpAddress();
        pIf->set(q, &from, pIf->cols());
        pIp->set(q, &from);
        if (q.next()) {
            if (__ex) EXCEPTION(AMBIGUOUS,0, trUtf8("A %1 IP nem egyedi").arg(a.toString()));
            delete pIf;
            delete pIp;
            return false;
        }
        pIf->addresses << pIp;
        ports << pIf;
        return true;
    }
    if (__ex) EXCEPTION(EFOUND,0, trUtf8("A %1 IP.").arg(a.toString()));
    return false;
}

bool cNode::fetchByMac(QSqlQuery& q, const cMac& a)
{
    clear();
    QString sql = QString("SELECT DISTINCT %1.* FROM %1 JOIN interfaces USING(node_id) WHERE hwaddress = ?").arg(tableName());
    if (execSql(q, sql, a.toString())) {
        set(q);
        if (q.next()) EXCEPTION(AMBIGUOUS,0, trUtf8("A %1 MAC nem egyedi").arg(a.toString()));
        return true;
    }
    return false;
}

bool cNode::fetchSelf(QSqlQuery& q, eEx __ex)
{
    if (lanView::testSelfName.isEmpty()) {
        QString name = getEnvVar("HOSTNAME");
        if (!name.isEmpty()) {
            setName(name);
            if (fetch(q, false, mask(_sNodeName))) return true;
        }
        QList<QHostAddress>  aa = QNetworkInterface::allAddresses();
        foreach (QHostAddress a, aa) {
            PDEB(VVERBOSE) << "My adress : " << a.toString() << endl;
            if (a.isNull() || a.isLoopback()) continue;
            if (fetchByIp(q, a, EX_IGNORE)) return true;
        }
        QList<QNetworkInterface> ii = QNetworkInterface::allInterfaces();
        foreach (QNetworkInterface i, ii) {
            cMac m = i.hardwareAddress();
            if (!m) continue;
            PDEB(VVERBOSE) << "My MAC : " << m.toString() << endl;
            if (fetchByMac(q, m)) return true;
        }
    }
    else {
        setName(lanView::testSelfName);
        if (fetch(q, false, mask(_sNodeName))) return true;
    }
    if (__ex) EXCEPTION(EFOUND, -1, trUtf8("Self host record"));
    return false;
}


cInterface *cNode::portAddAddress(const QString& __port, const QHostAddress& __a, const QString& __t, const QString &__d)
{
    cInterface * p = getPort(__port)->reconvert<cInterface>();
    p->addIpAddress(__a, __t, __d);
    return p;
}

cInterface *cNode::portAddAddress(int __port_index, const QHostAddress& __a, const QString& __t, const QString &__d)
{
    cInterface *p = getPort(__port_index)->reconvert<cInterface>();
    p->addIpAddress(__a, __t, __d);
    return p;
}

cInterface *cNode::portSetVlan(const QString& __port, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st)
{
    cInterface *p = getPort(__port)->reconvert<cInterface>();
    p->joinVlan(__vlanId, __t, __st);
    return p;
}

cInterface * cNode::portSetVlan(int __port_index, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st)
{
    cInterface *p = getPort(__port_index)->reconvert<cInterface>();
    p->joinVlan(__vlanId, __t, __st);
    return p;
}

cInterface *cNode::portSetVlans(const QString& __port, const QList<qlonglong>& _ids)
{
    cInterface *p = getPort(__port)->reconvert<cInterface>();
    enum eVlanType t = VT_UNTAGGED;
    foreach(qlonglong id, _ids) {
        p->joinVlan(id, t, ST_MANUAL);
        t = VT_TAGGED;
    }
    return p;
}

cInterface *cNode::portSetVlans(int __port_index, const QList<qlonglong>& _ids)
{
    cInterface *p = getPort(__port_index)->reconvert<cInterface>();
    enum eVlanType t = VT_UNTAGGED;
    foreach(qlonglong id, _ids) {
        p->joinVlan(id, t, ST_MANUAL);
        t = VT_TAGGED;
    }
    return p;
}

QList<QHostAddress> cNode::fetchAllIpAddress(QSqlQuery& q, qlonglong __id) const
{
    QString sql =
            "SELECT address FROM interfaces JOIN ipaddresses USING(port_id)"
               " WHERE node_id = ?"
               " ORDER BY preferred ASC";
    QList<QHostAddress> r;
    qlonglong id = __id < 0 ? getId() : __id;
    PDEB(VERBOSE) << VDEB(id) << endl;
    if (execSql(q, sql, id)) do {
        QVariant v = q.value(0);
        QHostAddress a;
        a.setAddress(v.toString());
        if (a.isNull()) EXCEPTION(EDATA);
        r << a;
    } while (q.next());
    return r;
}

QHostAddress cNode::getIpAddress() const
{
    DBGFN();
    if (ports.isEmpty()) {
        _DBGFNL() << trUtf8(" A %1 elemnek nincsen portja, így IP címe sem.").arg(getName()) << endl;
        return QHostAddress();
    }
    for (tRecordList<cNPort>::const_iterator pi = ports.constBegin(); pi < ports.constEnd(); ++pi) {
        const cNPort *pp = *pi;
        if (pp->tableoid() == cInterface::_descr_cInterface().tableoid()) {
            const cInterface& i = *(const cInterface *)pp;
            for (tOwnRecords<cIpAddress, cInterface>::const_iterator ii = i.addresses.constBegin(); ii < i.addresses.constEnd(); ++ii) {
                const cIpAddress& ip = **ii;
                QHostAddress a = ip.address();
                if (a.isNull()) {
                    _DBGFNL() << trUtf8(" A %1 elemnek nincsen fix IP címe.").arg(getName()) << endl;
                }
                return a;
            }
        }
    }
    _DBGFNL() << trUtf8(" A %1 elemnek nincsen IP címe.").arg(getName()) << endl;
    return QHostAddress();
}

cNode& cNode::asmbAttached(const QString& __n, const QString& __d, qlonglong __place)
{
    clear();
    setName(__n);
    setName(_sNodeNote, __d);
    setId(_sPlaceId, __place);
    addPort(_sAttach, _sAttach, _sNul, 1);
    return *this;
}

cNode& cNode::asmbWorkstation(QSqlQuery& q, const QString& __n, const cMac& __mac, const QString& __d, qlonglong __place)
{
    setName(__n);
    setName(_sNodeNote, __d);
    setId(_sPlaceId, __place);
    addPort(_sEthernet, _sEthernet, _sNul, 1);
    cInterface *pi = ports.first()->reconvert<cInterface>();
    *pi = __mac;
    QHostAddress ha;
    QList<QHostAddress> al = cArp().mac2ips(q, __mac);
    if (al.size() == 1) ha = al[0];
    if (ha.isNull()) {
        QHostInfo hi = QHostInfo::fromName(__n);         // Név alapján lekérdezzük az IP címet
        al = hi.addresses();
        if (al.size() == 1) ha = al[0];
    }
    if (!ha.isNull()) {     // Ha van ip, akkor beállítjuk
        pi->addIpAddress(ha, _sDynamic);
    }
    return *this;
}

cNPort& cNode::asmbHostPort(QSqlQuery& q, int ix, const QString& pt, const QString& pn, const QStringPair *ip, const QVariant *mac, const  QString& d)
{
    QHostAddress a;
    cMac m;
    if (ix != NULL_IX && getPort(ix, EX_IGNORE) != NULL) EXCEPTION(EDATA, ix, trUtf8("Nem egyedi port index"));
    if (                 getPort(pn, EX_IGNORE) != NULL) EXCEPTION(EDATA, -1, trUtf8("Nem egyedi portnév: %1").arg(pn));
    if (mac != NULL) {
        if (variantIsInteger(*mac)) {
            int i = ports.indexOf(cPPort::ixPortIndex(), *mac);
            if (i < 0) EXCEPTION(EDATA, mac->toInt(), trUtf8("Hibás port index hivatkozás"));
            m = ports[i]->getId(_sHwAddress);
        }
        else if (variantIsString(*mac)) {
            QString sm = mac->toString();
            if (sm != _sARP && !m.set(sm)) EXCEPTION(EDATA, -1, trUtf8("Nem értelmezhető MAC : %1").arg(sm))
        }
        else EXCEPTION(EDATA);
    }

    cNPort *p = cNPort::newPortObj(pt); // Létrehozzuk a port objektumot
    ports << p;
    p->setName(_sPortName, pn);
    if (ix != NULL_IX) p->setId(_sPortIndex, ix);
    if (!d.isEmpty()) p->setName(_sPortNote, d);
    if (ip != NULL) {   // Hozzá kell adni egy IP címet
        cInterface *pIf = p->reconvert<cInterface>();    // de akkor a port cInterface kell legyen
        QString sip  = ip->first;   // cím
        QString type = ip->second;  // típusa
        if (sip == _sARP) {     // A címet az ARP adatbázisból kell előkotorni
            if (!m) EXCEPTION(EDATA, -1, trUtf8("Az ip cím felderítéséhez nincs megadva MAC."));
            QList<QHostAddress> al = cArp::mac2ips(q, m);
            if (al.size() < 1) EXCEPTION(EDATA, -1, trUtf8("Az ip cím felderítés sikertelen."));
            if (al.size() > 1) EXCEPTION(EDATA, -1, trUtf8("Az ip cím felderítés nem egyértelmű."));
            a = al.first();
        }
        else if (!sip.isEmpty()) {
            a.setAddress(sip);
        }
        pIf->addIpAddress(a, type);
    }
    if (mac != NULL) {
        if (mac->toString() == _sARP) {
            if (!a.isNull()) EXCEPTION(EDATA, -1, trUtf8("Az ip cím hányában a MAC nem felderíthető."));
            m = cArp::ip2mac(q, a);
            if (!m) EXCEPTION(EDATA, -1, trUtf8("A MAC cím felderítés sikertelen."));
        }
        if (!m) EXCEPTION(EPROGFAIL);
        p->set(_sHwAddress, QVariant::fromValue(m));
    }
    return *p;
}


cNode& cNode::asmbNode(QSqlQuery& q, const QString& __name, const QStringPair *__port, const QStringPair *__addr, const QString *__sMac, const QString& __note, qlonglong __place, eEx __ex)
{
    QString em;
    QString name = __name;
    clear();
    setName(_sNodeNote, __note);
    setId(_sPlaceId, __place);
    QString ips, ipt, ifType, pnm;
    if (__addr != NULL) { ips = __addr->first; ipt = __addr->second; }      // IP cím és típus
    if (__port != NULL) { pnm = __port->first; ifType = __port->second; }   // port név és típus
    if (ifType.isEmpty()) ifType = _sEthernet;
    if (pnm.isEmpty())    pnm    = _sEthernet;
    QList<QHostAddress> hal;
    QHostAddress ha;
    cMac ma;
    if (__sMac != NULL) {
        if (*__sMac != _sARP && !ma.set(*__sMac)) {
            _stat |= ES_DEFECTIVE;
            em = trUtf8("Nem értelmezhető MAC : %1; Node : %2").arg(*__sMac).arg(toString());
            if (__ex) EXCEPTION(EDATA, -1, em);
            DERR() << em << endl;
            APPMEMO(q, em, RS_CRITICAL);
            return *this;
        }
    }

    if (name == _sLOOKUP) {     // Ki kell deríteni a nevet, van címe és portja
        if (ips == _sARP) {     // sőt az ip-t is
            if (!ma) {
                _stat |= ES_DEFECTIVE;
                em = trUtf8("A név nem deríthető ki, nincs adat. Node : %1").arg(toString());
                if (__ex) EXCEPTION(EDATA, -1, ma);
                DERR() << em << endl;
                APPMEMO(q, em, RS_CRITICAL);
                return *this;
            }
            hal = cArp::mac2ips(q, ma);
            int n = hal.size();
            if (n != 1) {
                _stat |= ES_DEFECTIVE;
                if (n < 1) {
                    em = trUtf8("A név nem deríthető ki, a %1 MAC-hez nincs IP cím.  Node : %2").arg(*__sMac).arg(toString());
                    if (__ex) EXCEPTION(EFOUND, n, em);
                }
                else{
                    em = trUtf8("A név nem deríthető ki, a %1 MAC-hez több IP cím tartozik. Node : %2").arg(*__sMac).arg(toString());
                    if (__ex) EXCEPTION(AMBIGUOUS,n, em);
                }
                DERR() << em << endl;
                APPMEMO(q, em, RS_CRITICAL);
                return *this;
            }
            ha = hal.first();
        }
        else {
            if (!ha.setAddress(ips)) {
                _stat |= ES_DEFECTIVE;
                em = trUtf8("Nem értelmezhatő IP cím %1. Node : %2").arg(ips).arg(toString());
                if (__ex) EXCEPTION(EDATA, -1, em);
                DERR() << em << endl;
                APPMEMO(q, em, RS_CRITICAL);
                return *this;
            }
            else if (!ma) {  // Nincs MAC, van IP
                if (__sMac == NULL) {
                    em = trUtf8("Hiányzó MAC cím. Node : %1").arg(toString());
                    if (__ex) EXCEPTION(EDATA, -1, em);
                }
                else if (*__sMac == _sARP) ma = cArp::ip2mac(q, ha);
                if (!ma) {
                    _stat |= ES_DEFECTIVE;
                    if (em.isEmpty()) em = trUtf8("A %1 cím alapján nem deríthatő ki a MAC.").arg(ips);
                    if (__ex) EXCEPTION(EDATA, -1, em);
                    DERR() << em << endl;
                    APPMEMO(q, em, RS_CRITICAL);
                    return *this;
                }
            }
        }
        name = cIpAddress::lookup(ha);
        setName(name);
        cInterface *p = addPort(ifType, pnm, _sNul, 1)->reconvert<cInterface>();
        if (ipt.isEmpty()) ipt = _sFixIp;
        p->addIpAddress(ha, ipt);
        return *this;
    }
    // Van név
    setName(name);
    if (__addr == NULL) {                   // Nincs IP címe
        if (__port == NULL && __sMac == NULL) {    // Portja sincs
            return *this;                   // Akkor kész
        }
        if (__sMac != NULL && *__sMac == _sARP) {
            _stat |= ES_DEFECTIVE;
            em = trUtf8("Ebben a kontexusban a MAC nem kideríthető. Node : %1").arg(toString());
            if (__ex) EXCEPTION(EDATA, -1, em);
            DERR() << em << endl;
            APPMEMO(q, em, RS_CRITICAL);
            return *this;
        }
        if (ifType.isEmpty()) ifType = _sEthernet;
        if (pnm.isEmpty())    pnm    = ifType;
        cNPort *p = addPort(ifType, pnm, _sNul, 1);
        if (__sMac == NULL) return *this;      // Ha nem adtunk meg MAC-et
        if (!ma) EXCEPTION(EPROGFAIL);
        *p->reconvert<cInterface>() = ma;   // A MAC-et is beállítjuk.
        return *this;
    }
    // Van név, kell port is, és ip cím is
    if (ips == _sLOOKUP) { // A név OK, de a névből kell az IP
        ha = cIpAddress::lookup(name);
    }
    else if (ips == _sARP) {    // Az IP a MAC-ból derítendő ki
        if (!ma) {
            _stat |= ES_DEFECTIVE;
            em = trUtf8("Az IP csak helyes MAC-ból deríthető ki. Node : %1").arg(toString());
            if (__ex) EXCEPTION(EDATA, -1, em);
            DERR() << em << endl;
            APPMEMO(q, em, RS_CRITICAL);
            return *this;
        }
        ha = cArp::mac2ip(q, ma);
    }
    else if (!ips.isEmpty()) {
        ha.setAddress(ips);
        if (ha.isNull()) {
            _stat |= ES_DEFECTIVE;
            em = trUtf8("Nem értelmezhető ip cím : %1. Node : %2").arg(ips).arg(toString());
            if (__ex) EXCEPTION(EIP,-1,em);
            DERR() << em << endl;
            APPMEMO(q, em, RS_CRITICAL);
            return *this;
        }
    }
    if (__sMac != NULL && *__sMac == _sARP) {
        ma = cArp::ip2mac(q, ha);
    }
    if (ifType.isEmpty()) ifType = _sEthernet;
    if (pnm.isEmpty())    pnm    = ifType;
    cInterface *p = addPort(ifType, pnm, _sNul, 1)->reconvert<cInterface>();
    if (ma) *p = ma;
    p->addIpAddress(ha, ipt).setOn(_sPreferred);
    PDEB(VERBOSE) << toString() << endl;
    return *this;
}

/* ------------------------------ SNMPDEVICES : cSnmpDevice ------------------------------ */

cSnmpDevice::cSnmpDevice() : cNode(_no_init_)
{
    _set(cSnmpDevice::descr());
}

cSnmpDevice::cSnmpDevice(const cSnmpDevice& __o) : cNode(_no_init_)
{
    _set(cSnmpDevice::descr());
    __cp(__o);
    _copy(__o, _descr_cSnmpDevice());
    ports.append(__o.ports);
    params.append(__o.params);
}

cSnmpDevice::cSnmpDevice(const QString& __n, const QString &__d)
 : cNode(_no_init_)
{
    _set(cSnmpDevice::descr());
    _set(_descr_cSnmpDevice().nameIndex(),  __n);
    _set(_descr_cSnmpDevice().noteIndex(), __d);
}

cSnmpDevice& cSnmpDevice::clone(const cRecord& __o)
{
    copy(__o);
    if (typeid(cRecordAny) != typeid(__o)) {
        const cSnmpDevice& r = *dynamic_cast<const cSnmpDevice*>(&__o);
        ports.clear().append(r.ports);
        params.clear().append(r.params);
    }
    return *this;
}

// -- virtual
CRECDDCR(cSnmpDevice, _sSnmpDevices)
CRECDEFNCD(cSnmpDevice)

bool cSnmpDevice::insert(QSqlQuery &__q, eEx __ex)
{
    int ixNodeType = toIndex(_sNodeType);
    if (isNull(ixNodeType)) {
        clearDefectivFieldBit(ixNodeType);
        qlonglong nt = enum2set(NT_HOST, NT_SNMP);
        if (ports.size() > 8) nt |= enum2set(NT_SWITCH);
        setId(ixNodeType, nt);
    }
    return cPatch::insert(__q, __ex);
}

bool cSnmpDevice::rewrite(QSqlQuery &__q, eEx __ex)
{
    int ixNodeType = toIndex(_sNodeType);
    if (isNull(ixNodeType)) {
        qlonglong nt = enum2set(NT_HOST, NT_SNMP);
        if (ports.size() > 8) nt |= enum2set(NT_SWITCH);
        setId(ixNodeType, nt);
    }
    return cNode::rewrite(__q, __ex);
}

cNPort *cSnmpDevice::addPort(const cIfType& __t, const QString& __name, const QString& __descr, int __ix)
{
    if (__ix < 0) EXCEPTION(EDATA, __ix);
    return cNode::addPort(__t, __name, __descr, __ix);
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

bool cSnmpDevice::setBySnmp(const QString& __com, eEx __ex, QString *pEs)
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
    if (setSysBySnmp(*this, __ex, pEs) > 0 && setPortsBySnmp(*this, __ex, pEs)) {
        if (isNull(_sNodeType)) {   // Ha nics típus beállítva
            qlonglong nt = enum2set(NT_HOST, NT_SNMP);
            if (ports.size() > 7) nt |= enum2set(NT_SWITCH); // több mint 7 port, meghasaljuk, hogy switch
            setId(_sNodeType, nt);
        }
        return true;
    }
#else // MUST_SCAN
    (void)__com;
    if (pEs != NULL) *pEs = snmpNotSupMsg();
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, -1, snmpNotSupMsg());
#endif // MUST_SCAN
    return false;
}

int cSnmpDevice::open(QSqlQuery& q, cSnmp& snmp, eEx __ex) const
{
#ifdef MUST_SCAN
    QList<QHostAddress> la = fetchAllIpAddress(q);
    if (la.isEmpty()) {
        QString em = trUtf8("A %1 SNMP eszköznek nincs IP címe").arg(getName());
        if (__ex) EXCEPTION(EDATA, -1, em);
        DERR() << em << endl;
        return -1;
    }
    QString comn = getName(_sCommunityRd);
    int     ver  = snmpVersion();
    int r = -1;
    foreach (QHostAddress a, la) {
        r = snmp.open(a.toString(), comn, ver);
        if (r != 0) continue;
        r = snmp.getNext(QString("SNMPv2-MIB::sysDescr"));
        if (r == 0) break;  // O.K.
    }
    if (__ex && r) EXCEPTION(ESNMP, r, snmp.emsg);
    return r;
#else // MUST_SCAN
    (void)snmp;
    (void)q;
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, -1, snmpNotSupMsg());
    return -1;
#endif // MUST_SCAN
}


/* ----------------------------------------------------------------- */

int phsLinkType(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sFront, Qt::CaseInsensitive)) return LT_FRONT;
    if (0 == n.compare(_sBack,  Qt::CaseInsensitive)) return LT_BACK;
    if (0 == n.compare(_sTerm,  Qt::CaseInsensitive)) return LT_TERM;
    if (__ex) EXCEPTION(EDATA, 0, n);
    return LT_INVALID;
}

const QString& phsLinkType(int e, eEx __ex)
{
    switch (e) {
    case LT_FRONT:  return _sFront;
    case LT_BACK:   return _sBack;
    case LT_TERM:   return _sTerm;
    default:        break;
    }
    if (__ex) EXCEPTION(EDATA, e);
    return _sNul;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CRECCNTR(cPhsLink)
const cRecStaticDescr&  cPhsLink::descr() const
{
    if (initPDescr<cPhsLink>(_sPhsLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cPhsLink().tableType() != TT_LINK_TABLE || _descr_cPhsLink().tableName() != _sPhsLinksTable) EXCEPTION(EPROGFAIL);
        CHKENUM(_sPhsLinkType1, phsLinkType);
        CHKENUM(_sPhsLinkType2, phsLinkType);
    }
    return *_pRecordDescr;
}
CRECDEFD(cPhsLink)

int cPhsLink::unxlinks(QSqlQuery& __q, qlonglong __pid, ePhsLinkType __t, ePortShare __s) const
{
    int r = 0;
    switch (__t) {
    case LT_BACK:
    case LT_TERM:   // Nincs/nem lehet megosztás erre a portra
                    r += unlink(__q, __pid, __t);
                    break;
    case LT_FRONT:
        switch (__s) {  // Minden ütközö link törlendő
        case ES_:   r += unlink(__q, __pid, LT_FRONT);
                    break;
        case ES_A:  r += unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_AA);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_AA: r += unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_AA);
                    break;
        case ES_AB: r += unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_B:  r += unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    r += unlink(__q, __pid, LT_FRONT, ES_BB);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_BA: r += unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_D);
                    break;
        case ES_BB: r += unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_BB);
                    ;
        case ES_C:  r += unlink(__q, __pid, LT_FRONT, ES_C);
                    r += unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    break;
        case ES_D:  r += unlink(__q, __pid, LT_FRONT, ES_D);
                    r += unlink(__q, __pid, LT_FRONT, ES_A);
                    r += unlink(__q, __pid, LT_FRONT, ES_B);
                    r += unlink(__q, __pid, LT_FRONT, ES_AB);
                    r += unlink(__q, __pid, LT_FRONT, ES_BA);
                    break;
        default:    EXCEPTION(EDATA);
        }
    default:    EXCEPTION(EDATA);
    }
    return r;
}

int cPhsLink::replace(QSqlQuery &__q, eEx __ex)
{
    int r = 0;
    // Ütköző linkek törlése a bal oldali porthoz (1)
    r += unxlinks(__q, getId(_sPortId1), (ePhsLinkType)getId(_sPhsLinkType1), (ePortShare)getId(__sPortShared));
    // Ütköző linkek törlése a jobb oldali porthoz (2)
    r += unxlinks(__q, getId(_sPortId2), (ePhsLinkType)getId(_sPhsLinkType2), (ePortShare)getId(__sPortShared));
    if (!insert(__q, __ex)) return R_ERROR;
    return r ? R_UPDATE : R_INSERT;
}

bool cPhsLink::rewrite(QSqlQuery &, eEx)
{
    EXCEPTION(ENOTSUPP);
    return false;
}

int cPhsLink::unlink(QSqlQuery &q, const QString& __nn, const QString& __pn, ePhsLinkType __t, bool __pat)
{
    QString sql =
            "SELECT phs_link_id FROM phs_links AS l"
            " JOIN nports AS p ON p.port_id = l.port_id1"
            " JOIN patch  AS n ON n.node_id = p.node_id"
            " WHERE n.node_name = ? AND p.port_name " + QString(__pat ? "LIKE" : "=") + " ?";
    if (__t != LT_INVALID) sql += " AND " + _sPhsLinkType1 + " = ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    q.bindValue(1, __pn);
    if (__t != LT_INVALID) q.bindValue(2, phsLinkType(__t));
    if (!q.exec()) SQLQUERYERR(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << variantToId(q.value(0));
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

int cPhsLink::unlink(QSqlQuery &q, const QString& __nn, ePhsLinkType __t, int __ix, int __ei)
{
    QString sql =
            "SELECT phs_link_id FROM phs_links AS l"
            " JOIN nports AS p ON p.port_id = l.port_id1"
            " JOIN patch  AS n ON n.node_id = p.node_id"
            " WHERE n.node_name = ? AND phs_link_type1 = ? AND p.port_index";
    if (__ei == NULL_IX || __ei == __ix) {
        sql += "= " + QString::number(__ix);
    }
    else {
        // if (__ix > __ie) EXCEPTION(EDATA);
        sql += ">= " + QString::number(__ix) + "AND p.port_index <= " + QString::number(__ei);
    }

    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    q.bindValue(1, phsLinkType(__t));
    if (!q.exec()) SQLQUERYERR(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << variantToId(q.value(0));
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

int cPhsLink::unlink(QSqlQuery &q, qlonglong __pid, ePhsLinkType __t, ePortShare __s) const
{
    QString pid = QString::number(__pid);
    QString w1 = "port_id1 = " + pid;
    QString w2 = "port_id2 = " + pid;
    if (__t != LT_INVALID) {
        QString t = quoted(phsLinkType(__t));
        w1 = "(" + w1 + " AND phs_link_type1 = " + t + ")";
        w2 = "(" + w1 + " AND phs_link_type2 = " + t + ")";
    }
    QString where = w1 + " OR " + w2;
    if (__s != ES_NC) {
        where = "port_shared = '" + portShare((int)__s) + "' AND (" + where + ")";
    }
    QString sql = "DELETE FROM " + descr().fullTableNameQ() + " WHERE " + where;
    execSql(q, sql);
    return q.numRowsAffected();
}

/* ----------------------------------------------------------------- */
qlonglong LinkGetLinked(QSqlQuery& q, cRecord& o, qlonglong __pid)
{
    o.clear();
    o.setId(_sPortId1, __pid);
    int n = o.completion(q);
    switch (n) {
    case 0: break;
    case 1: return o.getId(_sPortId2);
    default:EXCEPTION(EDATA, n, o.descr().toString());
    }
    return NULL_ID;
}
bool LinkIsLinked(QSqlQuery& q, cRecord& o, qlonglong __pid1, qlonglong __pid2)
{
    o.clear();
    o.setId(_sPortId1, __pid1);
    o.setId(_sPortId2, __pid2);
    int n = o.completion(q);
    switch (n) {
    case 0: break;
    case 1: return true;
    default:EXCEPTION(EDATA, n, o.descr().toString());
    }
    return false;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

CRECCNTR(cLogLink)
const cRecStaticDescr&  cLogLink::descr() const
{
    if (initPDescr<cLogLink>(_sLogLinks)) {
        // Link, a fenti a view tábla neve, ellenőrizzük a tábla nevet, és a típust:
        if (_descr_cLogLink().tableType() != TT_LINK_TABLE || _descr_cLogLink().tableName() != _sLogLinksTable) EXCEPTION(EPROGFAIL);
    }
    return *_pRecordDescr;
}

bool cLogLink::insert(QSqlQuery &, bool)
{
    EXCEPTION(ENOTSUPP);
    return false;
}

int cLogLink::replace(QSqlQuery &, bool)
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
        if (_descr_cLldpLink().tableType() != TT_LINK_TABLE || _descr_cLldpLink().tableName() != _sLldpLinksTable) EXCEPTION(EPROGFAIL);
    }
    return *_pRecordDescr;
}
CRECDEFD(cLldpLink)

bool cLldpLink::unlink(QSqlQuery& q, qlonglong __pid)
{
    QString pid = QString::number(__pid);
    QString sql = "DELETE FROM " + descr().fullTableNameQ() + " WHERE port_id1 = " + pid + " OR port_id2 = " + pid;
    execSql(q, sql);
    return q.numRowsAffected() != 0;
}

/* ----------------------------------------------------------------- */
int execState(const QString& _n, enum eEx __ex)
{
    if (0 == _n.compare(_sWait,     Qt::CaseInsensitive)) return ES_WAIT;
    if (0 == _n.compare(_sExecute,  Qt::CaseInsensitive)) return ES_EXECUTE;
    if (0 == _n.compare(_sOk,       Qt::CaseInsensitive)) return ES_OK;
    if (0 == _n.compare(_sFaile,    Qt::CaseInsensitive)) return ES_FAILE;
    if (0 == _n.compare(_sAborted,  Qt::CaseInsensitive)) return ES_ABORTED;
    if (__ex) EXCEPTION(EDATA, -1, _n);
    return ES_INVALID;
}

const QString& execState(int _e, enum eEx __ex)
{
    switch (_e) {
    case ES_WAIT:       return _sWait;
    case ES_EXECUTE:    return _sExecute;
    case ES_OK:         return _sOk;
    case ES_FAILE:      return _sFaile;
    case ES_ABORTED:    return _sAborted;
    default:            break;
    }
    if (__ex) EXCEPTION(EDATA, _e);
    return _sNul;
}

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

const QString& cTemplateMap::get(QSqlQuery& __q, const QString& __name, eEx __ex)
{
    QMap<QString, QString>::iterator i = find(__name);
    if (i != end()) return i.value();
    cImportTemplate it;
    it[_sTemplateType] = type;
    it[_sTemplateName] = __name;
    if (it.completion(__q) < 1) {
        if (__ex) EXCEPTION(EFOUND, -1, QString(QObject::trUtf8("%1 template name %2 not found.")).arg(type, __name));
        return _sNul;
    }
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
    it[_sTemplateNote]= __descr;
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

int vlanType(const QString& __n, eEx __ex)
{
    if (0 == __n.compare(_sNo,        Qt::CaseInsensitive)) return VT_NO;
    if (0 == __n.compare(_sUnknown,   Qt::CaseInsensitive)) return VT_NOTKNOWN;
    if (0 == __n.compare(_sForbidden, Qt::CaseInsensitive)) return VT_FORBIDDEN;
    if (0 == __n.compare(_sAuto,      Qt::CaseInsensitive)) return VT_AUTO;
    if (0 == __n.compare(_sAutoTagged,Qt::CaseInsensitive)) return VT_AUTO_TAGGED;
    if (0 == __n.compare(_sTagged,    Qt::CaseInsensitive)) return VT_TAGGED;
    if (0 == __n.compare(_sUntagged,  Qt::CaseInsensitive)) return VT_UNTAGGED;
    if (0 == __n.compare(_sVirtual,   Qt::CaseInsensitive)) return VT_VIRTUAL;
    if (0 == __n.compare(_sHard,      Qt::CaseInsensitive)) return VT_HARD;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return VT_INVALID;
}

const QString& vlanType(int __e, eEx __ex)
{
    switch (__e) {
    case VT_NO:         return _sNo;
    case VT_NOTKNOWN:   return _sUnknown;
    case VT_FORBIDDEN:  return _sForbidden;
    case VT_AUTO:       return _sAuto;
    case VT_AUTO_TAGGED:return _sAutoTagged;
    case VT_TAGGED:     return _sTagged;
    case VT_UNTAGGED:   return _sUntagged;
    case VT_VIRTUAL:    return _sVirtual;
    case VT_HARD:       return _sHard;
    default:            if (__ex) EXCEPTION(EDATA, (qlonglong)__e);
                        return _sNul;
    }
}

int  setType(const QString& __n, eEx __ex)
{
    if (0 == __n.compare(_sAuto,   Qt::CaseInsensitive)) return ST_AUTO;
    if (0 == __n.compare(_sQuery,  Qt::CaseInsensitive)) return ST_QUERY;
    if (0 == __n.compare(_sConfig, Qt::CaseInsensitive)) return ST_CONFIG;
    if (0 == __n.compare(_sManual, Qt::CaseInsensitive)) return ST_MANUAL;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return ST_INVALID;
}

const QString& setType(int __e, eEx __ex)
{
    switch (__e) {
    case ST_AUTO:   return _sAuto;
    case ST_QUERY:  return _sQuery;
    case ST_CONFIG: return _sConfig;
    case ST_MANUAL: return _sManual;
    default:        if (__ex) EXCEPTION(EDATA, (qlonglong)__e);
                    return _sNul;
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


/* ------------------------------ app_memos ------------------------------ */
DEFAULTCRECDEF(cAppMemo, _sAppMemos)

qlonglong cAppMemo::memo(QSqlQuery &q, const QString &_memo, int _imp, const QString& _func_name, const QString& _src, int _lin)
{
    cAppMemo o;
    o.herein(_memo, _imp, _func_name, _src, _lin);
    o.insert(q);
    return o.getId();
}

cAppMemo& cAppMemo::herein(const QString &_memo, int _imp, const QString &_func_name, const QString &_src, int _lin)
{
    lanView *pI = lanView::getInstance();
    setName(_sAppName, lanView::appName);
    setId(_sPid, QCoreApplication::applicationPid());
    setName(_sThreadName, currentThreadName());
    setName(_sAppVer, lanView::appVersion);
    setName(_sLibVer, lanView::libVersion);
    setName(_sFuncName, _func_name);
    setName(_sSrcName, _src);
    setId(_sSrcLine, _lin);
    if (pI->pSelfNode        != NULL) setId(_sNodeId,        pI->pSelfNode->getId());
    if (pI->pSelfHostService != NULL) setId(_sHostServiceId, pI->pSelfHostService->getId());
    if (pI->pUser            != NULL) setId(_sUserId,        pI->pUser->getId());
    setId(_sImportance, _imp);
    setName(_sMemo, _memo);
    return *this;
}

/* -------------------------------------------- selects ---------------------------------------- */
CRECCNTR(cSelect)

const cRecStaticDescr&  cSelect::descr() const
{
    if (initPDescr<cSelect>(_sSelects)) {
        _ixChoice   = _descr_cSelect().toIndex(_sChoice);
        _ixFeatures = _descr_cSelect().toIndex(_sFeatures);
//        CHKENUM(_ixPatternType, patternType);
    }
    return *_pRecordDescr;
}

int cSelect::_ixChoice   = NULL_IX;
int cSelect::_ixFeatures = NULL_IX;

CRECDEFD(cSelect)

cSelect& cSelect::choice(QSqlQuery q, const QString& _type, const QString& _val, eEx __ex)
{
    static QString sql =
            "SELECT * FROM selects "
                "WHERE :styp = select_type "
                  "AND ((pattern_type = 'equal'   AND :sval = pattern) "
                    "OR (pattern_type = 'equali'  AND lower(:sval) = lower(pattern)) "
                    "OR (pattern_type = 'similar' AND :sval similar TO pattern) "
                    "OR (pattern_type = 'regexp'  AND :sval ~  pattern) "
                    "OR (pattern_type = 'regexpi' AND :sval ~* pattern)) "
                "ORDER BY precedence "
                "LIMIT 1;";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(":styp", _type);
    q.bindValue(":sval", _val);
    if (!q.exec()) SQLQUERYERR(q);
    if (q.first()) set(q);
    else {
        if (__ex >= EX_WARNING) EXCEPTION(EFOUND, 0, _type + " : " + _val);
        set();
    }
    return *this;
}

cSelect& cSelect::choice(QSqlQuery q, const QString& _type, const cMac _val, eEx __ex)
{
    static QString sql =
            "SELECT * FROM selects "
                "WHERE :styp = select_type AND pattern_type = 'oui'"
                  "AND pattern::macaddr = trunc(:mac::macaddr) "
                "ORDER BY precedence "
                "LIMIT 1;";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(":styp", _type);
    q.bindValue(":mac", _val.toString());
    if (!q.exec()) SQLQUERYERR(q);
    if (q.first()) set(q);
    else {
        if (__ex >= EX_WARNING) EXCEPTION(EFOUND, 0, _type + " : " + _val.toString());
        set();
    }
    return *this;
}
