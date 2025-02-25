#include <algorithm>
#include "lanview.h"
#include "lv2data.h"
#include "srvdata.h"
#include "lv2user.h"
#include "scan.h"
// #include "lv2service.h"
#include "report.h"


const QString& notifSwitch(int _ns, eEx __ex)
{
    switch (_ns & RS_STAT_MASK) {
    case RS_ON:             return _sOn;
    case RS_RECOVERED:      return _sRecovered;
    case RS_WARNING:        return _sWarning;
    case RS_CRITICAL:       return _sCritical;
    case RS_DOWN:           return _sDown;
    case RS_UNREACHABLE:    return _sUnreachable;
    case RS_FLAPPING:       return _sFlapping;
    case RS_UNKNOWN:        return _sUnknown;
    default:                if (__ex)EXCEPTION(EDATA, _ns);
    }
    return _sNul;
}

int notifSwitch(const QString& _nm, eEx __ex)
{
    if (0 == _nm.compare(_sOn,         Qt::CaseInsensitive)) return RS_ON;
    if (0 == _nm.compare(_sRecovered,  Qt::CaseInsensitive)) return RS_RECOVERED;
    if (0 == _nm.compare(_sWarning,    Qt::CaseInsensitive)) return RS_WARNING;
    if (0 == _nm.compare(_sCritical,   Qt::CaseInsensitive)) return RS_CRITICAL;
    if (0 == _nm.compare(_sDown,       Qt::CaseInsensitive)) return RS_DOWN;
    if (0 == _nm.compare(_sUnreachable,Qt::CaseInsensitive)) return RS_UNREACHABLE;
    if (0 == _nm.compare(_sFlapping,   Qt::CaseInsensitive)) return RS_FLAPPING;
    if (0 == _nm.compare(_sUnknown,    Qt::CaseInsensitive)) return RS_UNKNOWN;
    if (0 ==__ex) EXCEPTION(EDATA, -1, _nm);
    return RS_INVALID;
}

int reasons(const QString& _r, eEx __ex)
{
    if (0 == _r.compare(_sOk,       Qt::CaseInsensitive)) return REASON_OK;
    if (0 == _r.compare(_sNew,      Qt::CaseInsensitive)) return R_NEW;
    if (0 == _r.compare(_sInsert,   Qt::CaseInsensitive)) return R_INSERT;
    if (0 == _r.compare(_sRemove,   Qt::CaseInsensitive)) return R_REMOVE;
    if (0 == _r.compare(_sExpired,  Qt::CaseInsensitive)) return R_EXPIRED;
    if (0 == _r.compare(_sMove,     Qt::CaseInsensitive)) return R_MOVE;
    if (0 == _r.compare(_sRestore,  Qt::CaseInsensitive)) return R_RESTORE;
    if (0 == _r.compare(_sModify,   Qt::CaseInsensitive)) return R_MODIFY;
    if (0 == _r.compare(_sUpdate,   Qt::CaseInsensitive)) return R_UPDATE;
    if (0 == _r.compare(_sInProgress,Qt::CaseInsensitive)) return R_IN_PROGRESS;
    if (0 == _r.compare(_sUnchange, Qt::CaseInsensitive)) return R_UNCHANGE;
    if (0 == _r.compare(_sFound,    Qt::CaseInsensitive)) return R_FOUND;
    if (0 == _r.compare(_sNotfound, Qt::CaseInsensitive)) return R_NOTFOUND;
    if (0 == _r.compare(_sDiscard,  Qt::CaseInsensitive)) return R_DISCARD;
    if (0 == _r.compare(_sCaveat,   Qt::CaseInsensitive)) return R_CAVEAT;
    if (0 == _r.compare(_sAmbiguous,Qt::CaseInsensitive)) return R_AMBIGUOUS;
    if (0 == _r.compare(_sError,    Qt::CaseInsensitive)) return R_ERROR;
    if (0 == _r.compare(_sClose,    Qt::CaseInsensitive)) return R_CLOSE;
    if (0 == _r.compare(_sTimeout,  Qt::CaseInsensitive)) return R_TIMEOUT;
    if (0 == _r.compare(_sUnknown,  Qt::CaseInsensitive)) return R_UNKNOWN;
    if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, _r);
    return ENUM_INVALID;
}

const QString& reasons(int _r, eEx __ex)
{
    switch (_r) {
    case REASON_OK:     return _sOk;
    case R_NEW:         return _sNew;
    case R_INSERT:      return _sInsert;
    case R_REMOVE:      return _sRemove;
    case R_EXPIRED:     return _sExpired;
    case R_MOVE:        return _sMove;
    case R_RESTORE:     return _sRestore;
    case R_MODIFY:      return _sModify;
    case R_UPDATE:      return _sUpdate;
    case R_IN_PROGRESS: return _sInProgress;
    case R_UNCHANGE:    return _sUnchange;
    case R_FOUND:       return _sFound;
    case R_NOTFOUND:    return _sNotfound;
    case R_DISCARD:     return _sDiscard;
    case R_CAVEAT:      return _sCaveat;
    case R_AMBIGUOUS:   return _sAmbiguous;
    case R_ERROR:       return _sError;
    case R_TIMEOUT:     return _sTimeout;
    case R_CLOSE:       return _sClose;
    case R_UNKNOWN:     return _sUnknown;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EDATA, _r);
    return _sNul;

}

/* ------------------------------ param_types ------------------------------ */
// paramTypeType() -> lv2types

tRecordList<cParamType>  cParamType::paramTypes;
cParamType *cParamType::pNull;
int cParamType::_ixParamTypeType = NULL_IX;

CRECCNTR(cParamType)
CRECDEFD(cParamType)

const cRecStaticDescr& cParamType::descr() const
{
    if (initPDescr<cParamType>(_sParamTypes)) {
        CHKENUM(toIndex(_sParamTypeType), paramTypeType)
        STFIELDIX(cParamType, ParamTypeType);
    }
    return *_pRecordDescr;
}

qlonglong cParamType::insertNew(QSqlQuery& q, const QString& __n, const QString& __de, const QString __t, const QString __di, bool _replace, eEx __ex)
{
    cParamType pp;
    _replace = _replace && pp.fetchByName(q, __n);
    pp.setName(__n);
    pp.set(_sParamTypeNote, __de);
    pp.set(_sParamTypeType,  __t);
    pp.set(_sParamTypeDim,   __di);
    if (pp.isDefective()) {
        if (__ex) EXCEPTION(EDATA, -1, __t);
        return NULL_ID;
    }
    bool f;
    if (_replace) {
        f = 1 == pp.update(q, false, QBitArray(), QBitArray(), __ex);
    }
    else {
        f = pp.insert(q, __ex);
    }
    return f ? pp.getId() : NULL_ID;
}

QString cParamType::paramToString(eParamType __t, const QVariant& __v, eEx __ex, bool *pOk)
{
    QString r;
    bool ok = false;
    if (__v.isNull()) {
        if (__t == PT_BOOLEAN) return _sFalse;
        ok = true;
    }
    else {
        switch (__t) {
        case PT_TEXT:
            ok = __v.canConvert<QString>();
            if (ok) r = __v.toString();
            break;
        case PT_BOOLEAN: {
            r = str2bool(__v.toString(), __ex) ? _sTrue : _sFalse;
            ok = true;
            break;
        }
        case PT_INTEGER:
            ok = __v.canConvert<qlonglong>();
            if (ok) r = QString::number(__v.toLongLong());
            break;
        case PT_REAL:
            ok = __v.canConvert<double>();
            if (ok) r = QString::number(__v.toDouble());
            break;
        case PT_INTERVAL: {
            qlonglong i = 0;
            if (__v.canConvert<qlonglong>()) {
                i = __v.toLongLong(&ok);
            }
            else {
                if (__v.canConvert<QString>()) {
                    i = parseTimeInterval(__v.toString(), &ok);
                }
            }
            if (ok) {
                r = intervalToStr(i);
            }
            break;
        }
        case PT_INET:
        case PT_CIDR:
        {
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
            break;
        }
        default:
            EXCEPTION(ENOTSUPP, __t);
        }
    }
    if (!ok && __ex) EXCEPTION(EDATA, __t, debVariantToString(__v));
    if (pOk != nullptr) *pOk = ok;
    return r;
}

QVariant cParamType::paramFromString(eParamType __t, const QString& __v, eEx __ex)
{
    QVariant r;
    bool    ok = true;
    if (__v.isNull()) {
        if (__t == PT_BOOLEAN) r = QVariant(false);
    }
    else {
        switch (__t) {
        case PT_TEXT:       r = QVariant(__v);                   break;
        case PT_BOOLEAN:    r = QVariant(str2bool(__v));         break;
        case PT_INTEGER:    r = QVariant(__v.toLongLong(&ok));   break;
        case PT_REAL:       r = QVariant(__v.toDouble(&ok));     break;
        case PT_INTERVAL:   r = QVariant(parseTimeInterval(__v, &ok));   break;
        case PT_INET:
        case PT_CIDR:
        {
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
        if (__ex) EXCEPTION(ENOTSUPP, __t, __v);
        r.clear();
    }
    return r;
}

void cParamType::fetchParamTypes(QSqlQuery& __q)
{
    if (pNull == nullptr) pNull = new cParamType();
    QBitArray   ba(1, false);   // Nem null, egyeseket nem tartalmazó maszk, minden rekord kiválasztásához
    if (paramTypes.count() == 0) {
        cParamType *p = new cParamType();
        if (0 == paramTypes.fetch(__q, false, ba, p)) EXCEPTION(EDATA, 0, tr("Load cache: The if_types table is empty."));
        return;
    }
    // UPDATE
    int found = 0;              // Megtalált rekordok száma
    int n = paramTypes.count();    // Megtalálandó rekordok száma
    cParamType pt;
    if (!pt.fetch(__q, false, ba)) EXCEPTION(EDATA, 0, tr("Update cache: The if_types table is empty."));
    do {
        cParamType *pPt = paramTypes.get(pt.getId(), EX_IGNORE);    // ID alapján rákeresünk
        if (pPt == nullptr) {   // Ez egy új rekord
            paramTypes << pt;
        }
        else {                // Frissít
            pPt->set(pt);
            ++n;
        }
    } while (pt.next(__q));
    if (n < found) EXCEPTION(EPROGFAIL);
    if (n > found) EXCEPTION(EDATA, n - found, tr("Deleted if_tpes record."));
}

const cParamType& cParamType::paramType(const QString& __nm, eEx __ex)
{
    checkParamTypes();
    int i = paramTypes.indexOf(_descr_cParamType().nameIndex(), QVariant(__nm));
    if (i < 0) {
        cParamType pt;
        QSqlQuery q = getQuery();
        if (pt.fetchByName(q, __nm)) {
            paramTypes << pt;
            return *paramTypes.last();
        }
        else {
            if (__ex) EXCEPTION(EDATA, -1,QObject::tr("Invalid ParamType name %1 or program error.").arg(__nm));
            return *pNull;
        }
    }
    return *(paramTypes[i]);
}

const cParamType& cParamType::paramType(qlonglong __id, eEx __ex)
{
    checkParamTypes();
    int i = paramTypes.indexOf(_descr_cParamType().idIndex(), QVariant(__id));
    if (i < 0) {
        cParamType pt;
        QSqlQuery q = getQuery();
        if (pt.fetchById(q, __id)) {
            paramTypes << pt;
            return *paramTypes.last();
        }
        else {
            if (__ex) EXCEPTION(EDATA, __id,QObject::tr("Invalid ParamType id or program error."));
            return *pNull;
        }
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
    cRecord::toEnd();
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
    return cParamType::paramFromString(eParamType(valueType()), v, __ex);
}

cSysParam& cSysParam::setValue(const QVariant& __v, eEx __ex)
{
    QString v = cParamType::paramToString(eParamType(valueType()), __v, __ex);
    setName(_ixParamValue, v);
    return *this;
}

/* ------------------------------ tpows ------------------------------ */
DEFAULTCRECDEF(cTpow, _sTpows)
/* ------------------------------ timeperiods ------------------------------ */
DEFAULTCRECDEF(cTimePeriod, _sTimePeriods)

bool cTimePeriod::isOnTime(QSqlQuery& q, const QDateTime& dt)
{
    if (isNull(idIndex())) setByName(q, getName());
    qlonglong id = getId();
    return isOnTime(q, id, dt);
}

bool cTimePeriod::isOnTime(QSqlQuery &q, qlonglong id, const QDateTime& dt)
{
    switch (id) {
    case NEVER_TIMEPERIOD_ID:   return false;   // never
    case ALWAYS_TIMEPERIOD_ID:  return true;    // always
    }
    bool r = execSqlBoolFunction(q, "time_in_timeperiod", id, dt);
    return r;
}

bool cTimePeriod::isOnTime(QSqlQuery &q, const QString& name, const QDateTime& dt)
{
    if (name == _sNever)  return false;
    if (name == _sAlways) return true;
    cTimePeriod o;
    qlonglong id = o.getIdByName(q, name);
    return isOnTime(q, id, dt);
}

QDateTime cTimePeriod::nextOnTime(QSqlQuery& q, const QDateTime& dt)
{
    if (isNull(idIndex())) setByName(q, getName());
    qlonglong id = getId();
    return nextOnTime(q, id, dt);
}

QDateTime cTimePeriod::nextOnTime(QSqlQuery &q, qlonglong id, const QDateTime& dt)
{
    switch (id) {
    case NEVER_TIMEPERIOD_ID:   return QDateTime(); // never
    case ALWAYS_TIMEPERIOD_ID:  return dt;          // always
    }
    execSqlFunction(q, "timeperiod_next_on_time", id, dt);
    return q.value(0).toDateTime();
}

QDateTime cTimePeriod::nextOnTime(QSqlQuery &q, const QString& name, const QDateTime& dt)
{
    if (name == _sNever)  return QDateTime();
    if (name == _sAlways) return dt;
    cTimePeriod o;
    qlonglong id = o.getIdByName(q, name);
    return nextOnTime(q, id, dt);
}

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
        CHKENUM(_ixImageType, imageType)
        CHKENUM(_sUsabilityes, usability)
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
    if (0 == __n.compare(_sBMP, Qt::CaseInsensitive)) return IT_BMP;
    if (0 == __n.compare(_sGIF, Qt::CaseInsensitive)) return IT_GIF;
    if (0 == __n.compare(_sJPG, Qt::CaseInsensitive)) return IT_JPG;
    if (0 == __n.compare(_sPNG, Qt::CaseInsensitive)) return IT_PNG;
    if (0 == __n.compare(_sPBM, Qt::CaseInsensitive)) return IT_PBM;
    if (0 == __n.compare(_sPGM, Qt::CaseInsensitive)) return IT_PGM;
    if (0 == __n.compare(_sPPM, Qt::CaseInsensitive)) return IT_PPM;
    if (0 == __n.compare(_sXBM, Qt::CaseInsensitive)) return IT_XBM;
    if (0 == __n.compare(_sXPM, Qt::CaseInsensitive)) return IT_XPM;
    if (0 == __n.compare(_sBIN, Qt::CaseInsensitive)) return IT_BIN;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return ENUM_INVALID;
}

const QString&  imageType(int __e, eEx __ex)
{
    switch (__e) {
    case IT_BMP:        return _sBMP;
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

int usability(const QString& __n, eEx __ex)
{
    if (0 == __n.compare(_sMap,  Qt::CaseInsensitive)) return US_MAP;
    if (0 == __n.compare(_sFlag, Qt::CaseInsensitive)) return US_FLAG;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return ENUM_INVALID;
}

const QString&  usability(int __e, eEx __ex)
{
    switch (__e) {
    case US_MAP:    return _sMap;
    case US_FLAG:   return _sFlag;
    default:
        if (__ex) EXCEPTION(EDATA, __e);
    }
    return _sNul;
}

/* ------------------------------ places ------------------------------ */
DEFAULTCRECDEF(cPlace, _sPlaces)

qlonglong cPlace::parentImageId(QSqlQuery& q)
{
    execSqlFunction(q, "get_parent_image", getId());
    QVariant iid = q.value(0);
    return iid.isNull() ? NULL_ID : variantToId(iid);
}

QString cPlace::placeCategoryName()
{
    static const QString sql =
            "SELECT place_group_name"
            " FROM places"
            " JOIN place_group_places USING(place_id)"
            " JOIN place_groups USING(place_group_id)"
            " WHERE place_id = ? AND place_group_type = 'category'"
            " LIMIT 1";
    QSqlQuery q = getQuery();
    return execSql(q, sql, getId()) ? q.value(0).toString() : _sNul;
}

const QString _sNode2Place = "node2place";

cPlace& cPlace::nodeName2place(QSqlQuery& q, const QString& _nodeName)
{
    clear();
    QStringList exps = cSysParam::getTextSysParam(q, _sNode2Place).split(QRegularExpression(",\\s?"));
    if (exps.isEmpty() || exps.first().isEmpty()) {
        setNote(tr("Nincs definiálva konverziós kifejezés a hely meghatározásra a név alapján!"));
        return *this;
    }
    QString sc = exps.first();
    enum eConv { C_SENSITIVE, C_UPPER, C_LOWER, C_INSENSITIVE } c;
    if      (0 == sc.compare("sensitive",   Qt::CaseInsensitive)) c = C_SENSITIVE;
    else if (0 == sc.compare("upper",       Qt::CaseInsensitive)) c = C_UPPER;
    else if (0 == sc.compare("lower",       Qt::CaseInsensitive)) c = C_LOWER;
    else if (0 == sc.compare("insensitive", Qt::CaseInsensitive)) c = C_INSENSITIVE;
    else {
        setNote(tr("Helytelen name2place rendszer paraméter : %1").arg(exps.join(_sCommaSp)));
        return *this;
    }
    exps.pop_front();
    QString emsg;
    for (auto exp: exps) {
        QRegularExpression re(exp);
        QRegularExpressionMatch ma;
        if (_nodeName.indexOf(re, 0, &ma) >= 0) {
            QString name = ma.captured(1);   // $1
            if (c == C_INSENSITIVE) {
                const QString sql = "SELECT * FROM places WHERE place_name ILIKE ?";
                if (execSql(q, sql, name)) set(q);
                else name.prepend(QChar('~'));
            }
            else {
                switch (c) {
                case C_UPPER:   name = name.toUpper();  break;
                case C_LOWER:   name = name.toLower();  break;
                default:        break;
                }
                fetchByName(q, name);
            }
            if (isNullId()) {
                msgAppend(&emsg, tr("A konvertált helyiség név : %1. Nincs ilyen helyiség!").arg(name));
            }
        }
        break;
    }
    if (isNullId()) {
        msgAppend(&emsg, tr("A konverziós kifelyezés nem illeszkedik a névre : %1.").arg(_nodeName));
        setNote(emsg);
    }
    return *this;
}


/* ------------------------------ place_froups ------------------------------ */

const QString& placeGroupType(int e, eEx __ex)
{
    switch (e) {
    case PG_GROUP:      return _sGroup;
    case PG_CATEGORY:   return _sCategory;
    case PG_ZONE:       return _sZone;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

int placeGroupType(const QString& s, eEx __ex)
{
    if (0 == s.compare(_sGroup,    Qt::CaseInsensitive)) return PG_GROUP;
    if (0 == s.compare(_sCategory, Qt::CaseInsensitive)) return PG_CATEGORY;
    if (0 == s.compare(_sZone,     Qt::CaseInsensitive)) return PG_ZONE;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, s);
    return ENUM_INVALID;
}

CRECCNTR(cPlaceGroup)
CRECDEFD(cPlaceGroup)

const cRecStaticDescr&  cPlaceGroup::descr() const
{
    if (initPDescr<cPlaceGroup>(_sPlaceGroups)) {
        CHKENUM(_sPlaceGroupType, placeGroupType)
    }
    return *_pRecordDescr;
}

qlonglong cPlaceGroup::insertNew(QSqlQuery q, const QString& __n, const QString& __d, int _type)
{
    cPlaceGroup o;
    o.setName(__n);
    o.setName(_sPlaceGroupNote, __d);
    o.setId(_type);
    o.insert(q);
    return o.getId();
}

qlonglong cPlaceGroup::replaceNew(QSqlQuery q, const QString& __n, const QString& __d, int _type)
{
    cPlaceGroup o;
    o.setName(__n);
    o.setName(_sPlaceGroupNote, __d);
    o.setId(_sPlaceGroupType, _type);
    o.replace(q);
    return o.getId();
}

QStringList cPlaceGroup::getAllZones(QSqlQuery q, QList<qlonglong> *pIds, eEx __ex)
{
    QStringList zones;
    // Nem üres zónák listája
    static const QString sql =
            "SELECT place_group_name, place_group_id FROM place_groups"
            " WHERE place_group_type = 'zone'"
             " AND 0 < (SELECT COUNT(*) FROM place_group_places WHERE place_group_places.place_group_id = place_groups.place_group_id)"
            " ORDER BY place_group_name ASC";
    // Az összes zóna neve (place_groups rekord, zone_type = 'zone', betürendben, de az all az első
    if (!execSql(q, sql)) {
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, 0, "No any zone.");
        return zones;
    }
    zones << _sAll;
    if (pIds != nullptr) {
        pIds->clear();
        *pIds << ALL_PLACE_GROUP_ID;
    }
    do {
        QString name = q.value(0).toString();
        if (name != _sAll) {
            zones << name;
            if (pIds != nullptr) {
                bool ok;
                *pIds << q.value(1).toLongLong(&ok);
                if (!ok) EXCEPTION(EDATA);
            }
        }
    } while (q.next());
    return zones;
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
        CHKENUM(_ixSubnetType, subNetType)
    }
    return *_pRecordDescr;
}

void cSubNet::toEnd()
{
    cSubNet::toEnd(_ixVlanId);
    cRecord::toEnd();
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
    if (0 == __at.compare(_sPrimary,   Qt::CaseInsensitive)) return NT_PRIMARY;
    if (0 == __at.compare(_sSecondary, Qt::CaseInsensitive)) return NT_SECONDARY;
    if (0 == __at.compare(_sPseudo,    Qt::CaseInsensitive)) return NT_PSEUDO;
    if (0 == __at.compare(_sPrivate,   Qt::CaseInsensitive)) return NT_PRIVATE;
    if (__ex) EXCEPTION(EDATA,-1,__at);
    return ENUM_INVALID;
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

/* ------------------------------ ip_addresses ------------------------------ */

cIpAddress::cIpAddress() : cRecord()
{
    _set(cIpAddress::descr());
}

cIpAddress::cIpAddress(const cIpAddress& __o) : cRecord()
{
    _copy(__o, *_pRecordDescr);
    __cp(__o);
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
        CHKENUM(_ixIpAddressType, addrType)
    }
    return *_pRecordDescr;
}

bool cIpAddress::rewrite(QSqlQuery &, eEx __ex)
{
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP);
    return false;
}

int cIpAddress::replace(QSqlQuery &, eEx __ex)
{
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP);
    return R_ERROR;
}

cIpAddress& cIpAddress::setAddress(const QHostAddress& __a, const QString& __t)
{
    if (__t.isEmpty() == false) {
        (void)addrType(__t);  // check
        setName(_ixIpAddressType, __t);
    }
//  if (isNull(_ixIpAddressType)) setName(_ixIpAddressType, _sFixIp);
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

eTristate cIpAddress::thisIsJoint(QSqlQuery &q, qlonglong _nodeId)
{
    cIpAddress o;
    if (isNull(_sAddress)) return TS_NULL;
    o = address();
    if (o.completion(q) == 0) return TS_NULL;
    if (_nodeId != NULL_ID) {   // Lehet saját cím is?
        QSqlQuery qq = getQuery();
        cNPort p;
        do {
            p.setById(qq, o.getId(_sPortId));
            // Ha nem saját cím, megvan az egy vizsgálandó azonos cím
            if (p.getId(_sNodeId) != _nodeId) break;
        } while (o.next(q));
        // Volt nem saját cím?
        if (o.isEmptyRec_()) return TS_NULL;
    }
    // Csak egy rekordot vizsgálunk, ha több van akkor is
    if (o.getId(_sIpAddressType) == AT_JOINT) {
        setId(_sIpAddressType, AT_JOINT);
        return TS_TRUE;
    }
    return TS_FALSE;
}

QString cIpAddress::lookup(const QHostAddress& ha, eEx __ex)
{
    QHostInfo hi = QHostInfo::fromName(ha.toString());
    if (hi.error() != QHostInfo::NoError) {
        if (__ex) EXCEPTION(EDATA, hi.error(), tr("A host név nem állapítható meg a cím: '%1' alapján").arg(ha.toString()));
        return _sNul;
    }
    return hi.hostName();
}

QList<QHostAddress> cIpAddress::lookupAll(const QString& hn, eEx __ex)
{
    QHostInfo hi = QHostInfo::fromName(hn);
    if (hi.error() != QHostInfo::NoError) {
        if (__ex) EXCEPTION(EDATA, hi.error(), tr("Az IP cím nem állapítható meg a host név: '%1' alapján; %2").arg(hn, hi.errorString()));
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
            EXCEPTION(AMBIGUOUS, n, tr("A hoszt cím a név: '%1' alapján nem egyértelmű : %2").arg(hn, als));
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
    case AT_JOINT:  return _sJoint;
    case AT_MANUAL: return _sManual;
    default: if (__ex) EXCEPTION(EDATA, __at);
    }
    return _sNul;
}

int addrType(const QString& __at, eEx __ex)
{
    if (0 == __at.compare(_sFixIp,   Qt::CaseInsensitive)) return AT_FIXIP;
    if (0 == __at.compare(_sDynamic, Qt::CaseInsensitive)) return AT_DYNAMIC;
    if (0 == __at.compare(_sPseudo,  Qt::CaseInsensitive)) return AT_PSEUDO;
    if (0 == __at.compare(_sPrivate, Qt::CaseInsensitive)) return AT_PRIVATE;
    if (0 == __at.compare(_sExternal,Qt::CaseInsensitive)) return AT_EXTERNAL;
    if (0 == __at.compare(_sJoint,   Qt::CaseInsensitive)) return AT_JOINT;
    if (0 == __at.compare(_sManual,  Qt::CaseInsensitive)) return AT_MANUAL;
    if (__ex) EXCEPTION(EDATA, -1, __at);
    return ENUM_INVALID;
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
    cRecord::toEnd();
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


int linkType(const QString& __n, eEx __ex)
{
    if (0 == _sPTP.     compare(__n, Qt::CaseInsensitive)) return LT_PTP;
    if (0 == _sBus.     compare(__n, Qt::CaseInsensitive)) return LT_BUS;
    if (0 == _sPatch.   compare(__n, Qt::CaseInsensitive)) return LT_PATCH;
    if (0 == _sLogical. compare(__n, Qt::CaseInsensitive)) return LT_LOGICAL;
    if (0 == _sWireless.compare(__n, Qt::CaseInsensitive)) return LT_WIRELESS;
    if (0 == _sUnknown. compare(__n, Qt::CaseInsensitive)) return LT_UNKNOWN;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return ENUM_INVALID;
}

const QString&  linkType(int __e, eEx __ex)
{
    switch (__e) {
    case LT_PTP:        return _sPTP;
    case LT_BUS:        return _sBus;
    case LT_PATCH:      return _sPatch;
    case LT_LOGICAL:    return _sLogical;
    case LT_WIRELESS:   return _sWireless;
    case LT_UNKNOWN:    return _sUnknown;
    default:  if (__ex) EXCEPTION(EDATA, __e);
    }
    return _sNul;
}

int portObjeType(const QString& __n, eEx __ex)
{
    if (0 == _sNPort.    compare(__n, Qt::CaseInsensitive)) return PO_NPORT;
    if (0 == _sPPort.    compare(__n, Qt::CaseInsensitive)) return PO_PPORT;
    if (0 == _sInterface.compare(__n, Qt::CaseInsensitive)) return PO_INTERFACE;
    if (0 == _sUnknown.  compare(__n, Qt::CaseInsensitive)) return PO_UNKNOWN;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return ENUM_INVALID;
}

const QString&  portObjeType(int __e, eEx __ex)
{
    switch (__e) {
    case PO_NPORT:      return _sNPort;
    case PO_PPORT:      return _sPPort;
    case PO_INTERFACE:  return _sInterface;
    case PO_UNKNOWN:    return _sUnknown;
    default:  if (__ex) EXCEPTION(EDATA, __e);
    }
    return _sNul;
}


CRECCNTR(cIfType) CRECDEFD(cIfType)

const cRecStaticDescr&  cIfType::descr() const
{
    if (initPDescr<cIfType>(_sIfTypes)) {
        CHKENUM(_sIfTypeLinkType, linkType)
        CHKENUM(_sIfTypeObjType, portObjeType)
    }
    return *_pRecordDescr;
}

tRecordList<cIfType> cIfType::_ifTypes;
cIfType *cIfType::_pNull = nullptr;

bool cIfType::insert(QSqlQuery &__q, eEx __ex)
{
    if (cRecord::insert(__q, __ex)) {
        _ifTypes << *this;
        return true;
    }
    return false;
}

int cIfType::update(QSqlQuery &__q, bool __only, const QBitArray &__set, const QBitArray &__where, eEx __ex)
{
    int n = cRecord::update(__q, __only, __set, __where, __ex);
    switch (n) {
    case 0:
        break;
    case 1: {
        cIfType *pIft = _ifTypes.get(getId());
        pIft->set(*this);
      }
        break;
    default: {
        QSqlQuery q = getQuery();
        foreach (cIfType *p, _ifTypes) {
            p->setById(q);
        }
        break;
      }
    }
    return n;
}

qlonglong cIfType::insertNew(QSqlQuery &__q, bool _ir, const QString& __nm, const QString& __no, int __iid, int __lid)
{
    cIfType it;
    it.setName(                __nm);
    it.setName(_sIfTypeNote,   __no);
    it.setId(  _sIfTypeIanaId, __iid);
    it.setId(  _sIanaIdLink,   __lid);
    if (_ir) it.replace(__q);
    else     it.insert(__q);
    return it.getId();
}

qlonglong cIfType::insertNew(QSqlQuery &__q, bool _ir, const QString& __nm, const QString& __no, int __iid, const QString& __ot, const QString& __lt, bool _pr)
{
    cIfType it;
    it.setName(                __nm);
    it.setName(_sIfTypeNote,   __no);
    it.setId(  _sIfTypeIanaId, __iid);
    it.setId(  _sIfTypeObjType, portObjeType(__ot));
    it.setId(  _sIfTypeLinkType,linkType(__lt));
    it.setBool(_sPreferred,    _pr);
    if (_ir) it.replace(__q);
    else     it.insert(__q);
    return it.getId();
}

void cIfType::fetchIfTypes(QSqlQuery& __q)
{
    if (_pNull == nullptr) _pNull = new cIfType();
    QBitArray   ba(1, false);   // Nem null, egyeseket nem tartalmazó maszk, minden rekord kiválasztásához
    if (_ifTypes.count() == 0) {
        cIfType *p = new cIfType();
        if (0 == _ifTypes.fetch(__q, false, ba, p)) EXCEPTION(EDATA, 0, tr("Load cache: The if_types table is empty."));
    }
    else {
        // UPDATE
        int found = _ifTypes.count();    // Megtalálandó rekordok száma
        int n = 0;                      // Megtalált rekordok számláló
        cIfType ift;
        if (!ift.fetch(__q, false, ba)) EXCEPTION(EDATA, 0, tr("Update cache: The if_types table is empty."));
        do {
            cIfType *pIft = _ifTypes.get(ift.getId(), EX_IGNORE);    // ID alapján rákeresünk
            if (pIft == nullptr) {   // Ez egy új rekord
                _ifTypes << ift;
            }
            else {                // Frissít
                pIft->set(ift);
                ++n;
            }
        } while (ift.next(__q));
        if (n > found) EXCEPTION(EPROGFAIL);
        if (n < found) EXCEPTION(EDATA, n - found, tr("Deleted if_tpes record."));
    }
    std::sort(_ifTypes.begin(), _ifTypes.end(),
              [](cIfType *pa, cIfType *pb)
                {
                    if (pa->getId(_sIfTypeIanaId) == pb->getId(_sIfTypeIanaId)) {
                        if (pb->getBool(_sPreferred)) return true;
                        if (pa->getBool(_sPreferred)) return false;
                        return pa->getId() < pb->getId();
                    }
                    return pa->getId(_sIfTypeIanaId) < pb->getId(_sIfTypeIanaId);
                }
              );
}

const cIfType& cIfType::ifType(const QString& __nm, eEx __ex)
{
    checkIfTypes();
    int i = _ifTypes.indexOf(_descr_cIfType().nameIndex(), QVariant(__nm));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, -1,QObject::tr("Invalid iftype name %1 or program error.").arg(__nm));
        return *_pNull;
    }
    return *(_ifTypes[i]);
}

const cIfType& cIfType::ifType(qlonglong __id, eEx __ex)
{
    checkIfTypes();
    int i = _ifTypes.indexOf(_descr_cIfType().idIndex(), QVariant(__id));
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, __id,QObject::tr("Invalid iftype id or program error."));
        return *_pNull;
    }
    return *(_ifTypes[i]);
}

const cIfType *cIfType::fromIana(int _iana_id, bool recursive, bool preferdOnly)
{
    checkIfTypes();
    const cIfType * r = nullptr;
    QList<cIfType *>::const_iterator    i;
    for (i = _ifTypes.constBegin(); i < _ifTypes.constEnd(); i++) {
        const cIfType *pift = *i;
        if (pift->getId(_sIfTypeIanaId) == _iana_id) {
            if (recursive == false || pift->isNull(_sIanaIdLink)) {
                if (pift->getBool(_sPreferred)) return pift;
                if (preferdOnly == false && r == nullptr) r = pift;
            }
            else {
                return fromIana(int(pift->getId(_sIanaIdLink)), false, preferdOnly);
            }
        }
    }
    return r;
}

/* ------------------------------ cNPort ------------------------------ */

qlonglong cNPort::_tableoid_nports     = NULL_ID;
qlonglong cNPort::_tableoid_pports     = NULL_ID;
qlonglong cNPort::_tableoid_interfaces = NULL_ID;

cNPort::cNPort() : cRecord(), params(this, _sNul)
{
    // DBGOBJ();
    _set(cNPort::descr());
}

cNPort::cNPort(const cNPort& __o) : cRecord(), params(this, __o.params)
{
    // DBGOBJ();
    _copy(__o, _descr_cNPort());
    __cp(__o);
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
    cRecord::toEnd();
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
        return params.size() == 0 || params.insert(__q, __ex) == params.size();
    }
    return false;
}

bool cNPort::rewrite(QSqlQuery &__q, eEx __ex)
{
   return tRewrite(__q, params, CV_PORT_PARAMS, __ex);
}

bool cNPort::rewriteById(QSqlQuery &__q, eEx __ex)
{
   return tRewriteById(__q, params, CV_PORT_PARAMS, __ex);
}

/// Feltételezi, hogy a parent (QObject *) pointer a tulajdonos objektumra mutat.
/// Ellenörzi, hogy a parent pointere konvertálható-e cPatch * típusű pinterré.
/// Ha nem, vagy a parent NULL, akkor kizárást dob.
/// A tulajdonos objektum containerValid adattagját maszkolja a __mask
/// paraméterrel, ha az eredmény nem nulla, akkor true értékkel tér vissza.
bool cNPort::isContainerValid(qlonglong __mask) const
{
    QObject *p = parent();
    if (p == nullptr) EXCEPTION(EPROGFAIL, __mask);
    cPatch *pO = qobject_cast<cPatch *>(p);
    if (pO == nullptr) EXCEPTION(EPROGFAIL, __mask);
    return pO->containerValid & __mask;
}

void cNPort::setContainerValid(qlonglong __set, qlonglong __clr)
{
    QObject *p = parent();
    if (p == nullptr) EXCEPTION(EPROGFAIL);
    cPatch *pO = qobject_cast<cPatch *>(p);
    if (pO == nullptr) EXCEPTION(EPROGFAIL);
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
    cNPort *r = nullptr;
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
        return nullptr;
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
}

cNPort * cNPort::getPortObjById(QSqlQuery& q, qlonglong __port_id, eEx __ex)
{
    qlonglong tableoid = cNPort().setId(__port_id).fetchTableOId(q, __ex);
    if (tableoid < 0LL) return nullptr;
    return getPortObjById(q, tableoid, __port_id, __ex);
}

int cNPort::fetchParams(QSqlQuery& q)
{
    // setContainerValid(CV_PORT_PARAMS);
    return params.fetch(q);
}

QString cNPort::getFullName(QSqlQuery& q, eEx __ex) const
{
    if (__ex == EX_IGNORE && isNull(_ixNodeId) && isNull(nameIndex())) return QString();
    return cPatch().getNameById(q, getId(_ixNodeId), __ex) + ':' + getName();
}

QString cNPort::getFullNameById(QSqlQuery& q, qlonglong _id)
{
    return execSqlTextFunction(q, "port_id2full_name", _id);
}

int cNPort::delPortByName(QSqlQuery &q, const QString &_nn, const QString &_pn, bool __pat)
{
    qlonglong nid = cPatch().getIdByName(_nn);
    QString sql = "DELETE FROM nports"
          " WHERE port_name" + QString(__pat ? " LIKE " : " = ") + quoted(_pn)
          + " AND node_id = " + QString::number(nid);
    EXECSQL(q, sql)
    int n = q.numRowsAffected();
    return  n;
}

eTristate cNPort::getBoolParam(const QString& name, eEx __ex) const
{
    const cPortParam *ppp = params.get(name, __ex);
    return ppp == nullptr ? TS_NULL : str2tristate(ppp->getName(_sParamValue), __ex);
}

bool portLessThanByIndex(const cNPort * pp1, const cNPort * pp2)
{
    return pp1->getId(cNPort::ixPortIndex()) < pp2->getId(cNPort::ixPortIndex());
}

bool portLessThanByName(const cNPort * pp1, const cNPort * pp2)
{
    return pp1->getName() < pp2->getName();
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

ePortShare shareResultant(ePortShare _sh1, ePortShare _sh2)
{
    static ePortShare m[ES_COUNT][ES_COUNT] = {
      // ES_,  ES_A, ES_AA,ES_AB,ES_B, ES_BA,ES_BB,ES_C, ES_D, ES_NC
        {ES_,  ES_A, ES_AA,ES_AB,ES_B, ES_BA,ES_BB,ES_C, ES_D, ES_NC}, // ES_
        {ES_A, ES_A, ES_AA,ES_AB,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_A
        {ES_AA,ES_AA,ES_AA,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_AA
        {ES_AB,ES_NC,ES_NC,ES_AB,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_AB
        {ES_B, ES_NC,ES_NC,ES_NC,ES_B, ES_BA,ES_BB,ES_NC,ES_NC,ES_NC}, // ES_B
        {ES_BA,ES_NC,ES_NC,ES_NC,ES_BA,ES_BA,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_BA
        {ES_BB,ES_NC,ES_NC,ES_NC,ES_BB,ES_NC,ES_BB,ES_NC,ES_NC,ES_NC}, // ES_BB
        {ES_C ,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_C ,ES_NC,ES_NC}, // ES_C
        {ES_D ,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_D ,ES_NC,ES_NC}, // ES_D
        {ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}  // ES_NC
    };
    if (_sh1 < 0 || _sh2 < 0 || _sh1 >= ES_COUNT || _sh2 >= ES_COUNT) EXCEPTION(EDATA);
    return m[_sh1][_sh2];
}

ePortShare shareConnect(ePortShare _sh1, ePortShare _sh2)
{
    static ePortShare m[ES_COUNT][ES_COUNT] = {
      // ES_,  ES_A, ES_AA,ES_AB,ES_B, ES_BA,ES_BB,ES_C, ES_D, ES_NC
        {ES_,  ES_A, ES_AA,ES_AB,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_
        {ES_A, ES_A, ES_AA,ES_AB,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_A
        {ES_AA,ES_AA,ES_AA,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_AA
        {ES_AB,ES_NC,ES_NC,ES_AB,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_AB
        {ES_NC,ES_NC,ES_NC,ES_NC,ES_B, ES_BA,ES_BB,ES_NC,ES_NC,ES_NC}, // ES_B
        {ES_NC,ES_NC,ES_NC,ES_NC,ES_BA,ES_BA,ES_NC,ES_NC,ES_NC,ES_NC}, // ES_BA
        {ES_NC,ES_NC,ES_NC,ES_NC,ES_BB,ES_NC,ES_BB,ES_NC,ES_NC,ES_NC}, // ES_BB
        {ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_C ,ES_NC,ES_NC}, // ES_C
        {ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_D ,ES_NC,ES_NC}, // ES_D
        {ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC,ES_NC}  // ES_NC
    };
    if (_sh1 < 0 || _sh2 < 0 || _sh1 >= ES_COUNT || _sh2 >= ES_COUNT) EXCEPTION(EDATA);
    return m[_sh1][_sh2];
}


cPPort::cPPort() : cNPort(_no_init_)
{
    //_DBGFN() << VDEBPTR(this) << endl;
    _set(cPPort::descr());
     setDefaultType();
}

cPPort::cPPort(const cPPort& __o) : cNPort(_no_init_)
{
    _copy(__o, _descr_cPPort());
    __cp(__o);
    params.append(__o.params);
}

// -- virtual
qlonglong       cPPort::_ifTypePatch = NULL_ID;
const cRecStaticDescr&  cPPort::descr() const
{
    if (initPDescr<cPPort>(_sPPorts)) {
        CHKENUM(_sSharedCable, portShare)
        _ifTypePatch = cIfType::ifTypeId(_sPatch); // Minden portnak ez a típusa
    }
    return *_pRecordDescr;
}
CRECDEFNCD(cPPort)

cPPort& cPPort::clone(const cRecord&__o)
{
    copy(__o);  // Ellenörzi az adat típust (descriptor)
    if (typeid(__o) != typeid(cRecordAny)) {    // A cRecordAny -ban nincs konténer, nem konvertálható
        params.clear();
        params.append(dynamic_cast<const cPPort *>(&__o)->params);
    }
    return *this;
}

/* ------------------------------ cInterface ------------------------------ */

cInterface::cInterface()
    : cNPort(_no_init_)
    , vlans(this, _sInterfaces)
    , addresses(this, _sNul)
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
    _copy(__o, _descr_cInterface());
    __cp(__o);
    trunkMembers = __o.trunkMembers;
}
// -- virtual
int cInterface::_ixHwAddress = NULL_IX;
const cRecStaticDescr&  cInterface::descr() const
{
    if (initPDescr<cInterface>(_sInterfaces)) {
        _ixHwAddress = _pRecordDescr->toIndex(_sHwAddress);
        CHKENUM(_sPortOStat, ifStatus)
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
    addresses.clear();
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
/// A Hívása elött a cNode törli az összes IP cím rekordot, igy az IP címekhez nem a replace() hanem az insert() metódust kell hívni,
///
/// Vlami nem stimmel a port_vlan rekordok kiírásánál, elvileg hasonló mint a port_params, de mégsem működik a rewrite !!!!!!!
///
bool cInterface::rewrite(QSqlQuery &__q, eEx __ex)
{
    bool r = cNPort::rewrite(__q, __ex) && (trunkMembers.size() == updateTrunkMembers(__q, __ex));
    if (!r) return false;
    if (isContainerValid(CV_PORT_VLANS)) {
        vlans.setsOwnerId();
        r = vlans.replace(__q, __ex);
        if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
    }
    if (addresses.count() > 0) {
        r = 0 < addresses.insert(__q, __ex);
    }
    return r;
}

/// Az cInterface nem önálló objektum.
/// A Hívása elött a cNode törli az összes IP cím rekordot, igy az IP címekhez nem a replace() hanem az insert() metódust kell hívni,
bool cInterface::rewriteById(QSqlQuery &__q, eEx __ex)
{
    bool r = cNPort::rewriteById(__q, __ex) && (trunkMembers.size() == updateTrunkMembers(__q, __ex));
    if (!r) return false;
    if (isContainerValid(CV_PORT_VLANS)) {
        r = vlans.replaceById(__q, __ex);
        if (!r) return false;
    }
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
        if (isEmptyRec_() || 1 != completion(q)) EXCEPTION(EDATA);
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
    if (pv == nullptr) vlans << (pv = new cPortVlan());
    pv->setVlanId(__id);
    pv->setVlanType(__t);
    pv->setSetType(__st);
    if (!isNull(idIndex())) pv->setPortId(getId());
}

bool cInterface::splitVlan(qlonglong __id)
{
    cPortVlan * p = vlans.pull(__id, EX_IGNORE);
    if (p == nullptr) return false;
    delete p;
    return true;
}

int cInterface::fetchByMac(QSqlQuery& q, const cMac& a)
{
    clear();
    set(_sHwAddress, QVariant::fromValue(a));
    return completion(q);
}

int cInterface::fetchByIp(QSqlQuery& q, const QHostAddress& a)
{
    int r = 0;
    QString sql = "SELECT interfaces.* FROM interfaces JOIN ip_addresses USING(port_id) WHERE address = ?";
    if (execSql(q, sql, a.toString())) {
        r = q.size();
        set(q);
    }
    else {
        clear();
    }
    return r;
}

int cInterface::fetchVlans(QSqlQuery& q)
{
    return vlans.fetch(q);
}

int cInterface::fetchAddressess(QSqlQuery& q)
{
    return addresses.fetch(q);
}

cIpAddress& cInterface::addIpAddress(const cIpAddress& __a)
{
    _DBGFN() << " @(" << __a.toString() << ")" << endl;
    if (addresses.indexOf(cIpAddress::ixAddress(), __a.get(cIpAddress::ixAddress()))) EXCEPTION(EDATA);
    addresses << __a;
    PDEB(VERBOSE) << QObject::tr("Added, size : %1").arg(addresses.size());
    return *addresses.last();
}

cIpAddress& cInterface::addIpAddress(const QHostAddress& __a, const QString& __t, const QString &__d)
{
    // _DBGFN() << " @(" << __a.toString() << ")" << endl;
    // if (addresses.indexOf(cIpAddress::_ixAddress, __a.get(cIpAddress::_ixAddress))) EXCEPTION(EDATA);
    cIpAddress *p = new cIpAddress();
    *p = __a;
    if (!__t.isEmpty()) p->setName(_sIpAddressType, __t);
    if (!__d.isEmpty()) p->setName(_sIpAddressNote, __d);
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
    return ENUM_INVALID;
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
    , ports(this, _sNul)
    , params(this, _sNul)
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
    _copy(__o, _descr_cPatch());
    __cp(__o);
    containerValid = __o.containerValid;
    if (__o.pShares == nullptr) EXCEPTION(EPROGFAIL,0,__o.toString());
    *pShares = *__o.pShares;
}

CRECDEFNCD(cPatch)

cPatch& cPatch::clone(const cRecord &__o)
{
    copy(__o);
    if (typeid(cRecordAny) != typeid(__o)) {
        const cPatch& r = *dynamic_cast<const cPatch*>(&__o);
        containerValid = r.containerValid;
        params.clear();
        ports.clear();
        params.append(r.params);
        ports.append(r.ports);
        if (r.pShares == nullptr) EXCEPTION(EPROGFAIL,0,__o.toString());
        *pShares = *r.pShares;
    }
    return *this;
}

const cRecStaticDescr&  cPatch::descr() const
{
    if (initPDescr<cPatch>(_sPatchs)) {
        CHKENUM(_sNodeType, nodeType)
    }
    return *_pRecordDescr;
}

void cPatch::clearToEnd()
{
    ports.clear();
    params.clear();
    clearShares();
    containerValid = 0;
}

void cPatch::toEnd()
{
    toEnd(idIndex());
    cRecord::toEnd();
}

bool cPatch::toEnd(int i)
{
    if (i == idIndex()) {
        if (atEndCont(ports, _sNodeId)) {
            containerValid &= ~CV_PORTS;
            clearShares();
        }
        if (atEndCont(params, _sNodeId)) {
            containerValid &= ~CV_NODE_PARAMS;
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
        if (pShares == nullptr) return true;
        return updateShares(__q, false, __ex);
    }
    return true;
}

bool cPatch::rewrite(QSqlQuery &__q, eEx __ex)
{
    bool r = tRewrite(__q, ports, CV_PORTS, params, CV_NODE_PARAMS, __ex);
    if (!r) return false;
    if (pShares == nullptr) return true;
    return updateShares(__q, false, __ex);
}

bool cPatch::rewriteById(QSqlQuery &__q, eEx __ex)
{
    bool r = cRecord::rewriteById(__q, __ex);
    if (isContainerValid(CV_PORTS)) {
        ports.setsOwnerId();
        r = ports.replaceById(__q, __ex);
        if (!r) return false;
        if (isContainerValid(CV_PATCH_BACK_SHARE)) {
            r = updateShares(__q, true, __ex);
        }
    }
    if (isContainerValid(CV_NODE_PARAMS)) {
        params.setsOwnerId();
        r = params.replace(__q, __ex);
        if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
    }
    return false;
}


bool cPatch::isContainerValid(qlonglong __mask) const
{
    return containerValid & __mask;
}

void cPatch::setContainerValid(qlonglong __set, qlonglong __clr)
{
    containerValid = (containerValid & ~__clr) | __set;
}

void cPatch::clearIdsOnly()
{
    _get(idIndex()).clear();
    ports.clearId();
    ports.setsOwnerId();
}


int cPatch::fetchPorts(QSqlQuery& __q, int flags)
{
    (void)flags;
    ports.clear();
    int n = 0;
    // A ports objektum fetch metódusa csak azonos rekord típusok esetén jó. Igaz, hogy egy típus van, de az nam az alap típus.
    cPPort p;
    p.setId(ports.ixOwnerId, getId());
    if (p.fetch(__q, true, p.mask(ports.ixOwnerId), p.iTab(_sPortIndex))) do {
        ports << p;
        ++n;
    } while (p.next(__q));
    containerValid |= CV_PORTS;
    return n;
}

void cPatch::insertPort(QSqlQuery& q, int ix, const QString& _na, const QString& _no, const QString& _tag)
{
    cPPort  p;
    p.setId(_sNodeId, getId());
    p.setId(_sPortIndex, ix);
    p.setName(_na);
    p.setNote(_no);
    p.setName(_sPortTag, _tag);
    p.insert(q);
}

void cPatch::updateShared(QSqlQuery& q, int __a, int __ab, int __b, int __bb, bool __cd)
{
    // Csak létező portra lehet megosztást csinálni
    if ((             !isContIx(ports, __a))
     || (__b  >= 0 && !isContIx(ports, __b))
     || (__ab >= 0 && !isContIx(ports, __ab))
     || (__bb >= 0 && !isContIx(ports, __bb))) EXCEPTION(EDATA);
    // A konstruktor is ellenőriz, egy port csak egyszer mind nem lehet negatív, ha mégis dob egy kizárást.
    cShareBack  s(__a, __b, __ab, __bb, __cd);
    updateShare(q, s);
}

int cPatch::fetchParams(QSqlQuery& q)
{
    containerValid |= CV_NODE_PARAMS;
    return params.fetch(q);
}

void cPatch::setParam(const QString& __name, const QVariant& __val, const QString& __type)
{
    qlonglong typeId = cParamType::paramTypeId(__type);
    cNodeParam *param = params.get(_sNodeParamName, __name, EX_IGNORE);
    if (param == nullptr) {
        param = new cNodeParam();
        param->setName(__name);
        param->setType(typeId);
        params << param;
    }
    cParamType type = cParamType::paramType(typeId);
    param->value() = type.paramToString(__val);
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
    return addShare(s);
}


bool cPatch::updateShare(QSqlQuery& __q, const cShareBack& s, eEx __ex)
{
    static const int sci = DESCR(cPPort).toIndex(_sSharedCable);
    static const int spi = DESCR(cPPort).toIndex(_sSharedPortId);
    static const QBitArray um = _bits(sci, spi);
    static const QBitArray wm = DESCR(cPPort).primaryKey();
    QString ss;
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
    return true;
}

bool cPatch::updateShares(QSqlQuery& __q, bool __clr, eEx __ex)
{
    if (__clr) {
        QString sql = "UPDATE pports SET shared_cable = DEFAULT, shared_port_id = DEFAULT WHERE port_id = " + QString::number(getId());
        EXECSQL(__q, sql)
    }
    if (shares().isEmpty()) return true;
    foreach (const cShareBack& s, shares()) {
        updateShare(__q, s, __ex);
    }
    return true;
}

cPPort *cPatch::addPort(const QString& __name, const QString& __note, int __ix)
{
    if (ports.count()) {
        if (0 <= ports.indexOf(__name)) {
            EXCEPTION(EDATA, -1, tr("Ilyen port név már létezik: %1").arg(__name));
        }
        if (0 <= ports.indexOf(cNPort::ixPortIndex(), QVariant(__ix))) {
            EXCEPTION(EDATA, __ix, tr("Ilyen port index már létezik."));
        }
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
    cPPort *p = nullptr;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(nameAndNumber(__n, i + __noff), _sNul, i + __off);
    }
    if (p == nullptr) EXCEPTION(EDATA);
    return p;
}

cNPort *cPatch::portSet(const QString& __name, const QString& __fn, const QVariant& __v)
{
    int i = ports.indexOf(cNPort().nameIndex(), __name);
    if (i < 0) EXCEPTION(ENONAME, -1, QString(QObject::tr("%1 port name not found.")).arg(__name));
    cNPort * p = ports[i];
    p->set(__fn, __v);
    return p;
}

cNPort *cPatch::portSet(int __ix, const QString& __fn, const QVariant& __v)
{
    int i = ports.indexOf(cNPort::ixPortIndex(), __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::tr("Port index not found"));
    cNPort *p = ports[i];
    p->set(__fn, __v);
    return p;
}

cNPort *cPatch::portSet(int __ix, const QString& __fn, const QVariantList& __vl)
{
    int ix = __ix;
    cNPort *p = nullptr;
    foreach (const QVariant& v, __vl) {
        p = portSet(ix, __fn, v);
        ++ix;
    }
    return p;
}

cNPort *cPatch::portSetParam(cNPort *port, const QString& __name, const QVariant& __val, const QString &__type)
{
    qlonglong typeId = cParamType::paramTypeId(__type);
    cPortParam *param = port->params.get(__name);
    if (param == nullptr) {
        param = new cPortParam();
        param->setName(__name);
        param->setId(_sParamTypeId, typeId);
        port->params << param;
    }
    cParamType type = cParamType::paramType(typeId);
    param->value() = type.paramToString(__val);
    return port;
}

cNPort *cPatch::portSetParam(int __ix, const QString& __name, const QVariantList &__val, const QString &__type)
{
    cNPort *p = nullptr;
    int ix = __ix;
    foreach (const QVariant& v, __val) {
        p = portSetParam(ix, __name, v, __type);
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
    if (__ex) EXCEPTION(ENONAME, -1, QString(QObject::tr("%1 port index not found.")).arg(__ix));
    return nullptr;
}

cNPort * cPatch::getPort(const QString& __pn, eEx __ex)
{
    int i = ports.indexOf(cNPort().nameIndex(), __pn);
    if (i >= 0) return ports[i];
    if (__ex) EXCEPTION(ENONAME, -1, QString(QObject::tr("%1 port name not found.")).arg(__pn));
    return nullptr;
}

const cRecStaticDescr *cPatch::getOriginalDescr(QSqlQuery& q, eEx __ex)
{
    qlonglong tableOID = fetchTableOId(q, __ex);
    if (tableOID < 0LL) {
        EXCEPTION(EDATA, getId(), tr("Get tableoid"));
    }
    if (tableOID ==    tableoid()) return &descr();
    cPatch pa;
    if (tableOID == pa.tableoid()) return &pa.descr();
    cNode   no;
    if (tableOID == no.tableoid()) return &no.descr();
    cSnmpDevice sn;
    if (tableOID == sn.tableoid()) return &sn.descr();
    if (__ex) EXCEPTION(EDATA, tableOID, tr("Invalid tableoid"));
    return nullptr;

}

cPatch * cPatch::getNodeObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __node_id, eEx __ex)
{
    if      (__tableoid == cPatch().      tableoid()) return getObjByIdT<cPatch>     (q, __node_id, __ex);
    else if (__tableoid == cNode().       tableoid()) return getObjByIdT<cNode>      (q, __node_id, __ex);
    else if (__tableoid == cSnmpDevice(). tableoid()) return getObjByIdT<cSnmpDevice>(q, __node_id, __ex);
    else if (__ex)                                    EXCEPTION(EDATA);
    return nullptr;
}

cPatch * cPatch::getNodeObjById(QSqlQuery& q, qlonglong __node_id, eEx __ex)
{
    qlonglong tableoid = cPatch().setId(__node_id).fetchTableOId(q, __ex);
    if (tableoid < 0LL) return nullptr;
    return getNodeObjById(q, tableoid, __node_id, __ex);
}

cPatch * cPatch::getNodeObjByName(QSqlQuery& q, const QString&  __node_name, enum eEx __ex)
{
    cPatch o;
    qlonglong tableoid = o.setName(__node_name).fetchTableOId(q, __ex);
    if (tableoid < 0LL) return nullptr;
    qlonglong id       = o.getIdByName(q, __node_name);
    return getNodeObjById(q, tableoid, id, __ex);
}


QString  cPatch::getTextParam(qlonglong _typeId, eEx __ex) const
{
    const cNodeParam *ppp = params.get(_sParamTypeId, _typeId, __ex);
    if (ppp == nullptr) return QString();
    return ppp->getName(_sParamValue);

}

qlonglong cPatch::getIntParam(qlonglong _typeId, eEx __ex) const
{
    const cNodeParam *ppp = params.get(_sParamTypeId, _typeId, __ex);
    if (ppp == nullptr) return NULL_ID;
    return ppp->getId(_sParamValue);
}

eTristate cPatch::getBoolParam(qlonglong _typeId, eEx __ex) const
{
    return str2tristate(getTextParam(_typeId, __ex), __ex);
}

void cPatch::sortPortsByIndex()
{
    std::sort(ports.begin(), ports.end(), portLessThanByIndex);
}

void cPatch::sortPortsByName()
{
    std::sort(ports.begin(), ports.end(), portLessThanByName);
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
    cRecord::toEnd();
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
    if (0 == __n.compare(_sAp,          Qt::CaseInsensitive)) return NT_AP;
    if (0 == __n.compare(_sWorkstation, Qt::CaseInsensitive)) return NT_WORKSTATION;
    if (0 == __n.compare(_sMobile,      Qt::CaseInsensitive)) return NT_MOBILE;
    if (0 == __n.compare(_sDevice,      Qt::CaseInsensitive)) return NT_DEVICE;
    if (0 == __n.compare(_sController,  Qt::CaseInsensitive)) return NT_CONTROLLER;
    if (0 == __n.compare(_sUps,         Qt::CaseInsensitive)) return NT_UPS;
    if (0 == __n.compare(_sWindows,     Qt::CaseInsensitive)) return NT_WINDOWS;
    if (0 == __n.compare(_sServer,      Qt::CaseInsensitive)) return NT_SERVER;
    if (0 == __n.compare(_sCluster,     Qt::CaseInsensitive)) return NT_CLUSTER;
    if (0 == __n.compare(_sUnixLike,    Qt::CaseInsensitive)) return NT_UNIX_LIKE;
    if (0 == __n.compare(_sThinClient,  Qt::CaseInsensitive)) return NT_THIN_CLIENT;
    if (0 == __n.compare(_sDisplay,     Qt::CaseInsensitive)) return NT_DISPLAY;
    if (__ex != EX_IGNORE)   EXCEPTION(EDATA, -1, __n);
    return ENUM_INVALID;
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
    case NT_AP:         return _sAp;
    case NT_WORKSTATION:return _sWorkstation;
    case NT_MOBILE:     return _sMobile;
    case NT_DEVICE:     return _sDevice;
    case NT_CONTROLLER: return _sController;
    case NT_UPS:        return _sUps;
    case NT_WINDOWS:    return _sWindows;
    case NT_SERVER:     return _sServer;
    case NT_CLUSTER:    return _sCluster;
    case NT_UNIX_LIKE:  return _sUnixLike;
    case NT_THIN_CLIENT:return _sThinClient;
    case NT_DISPLAY:    return _sDisplay;
    }
    if (__ex != EX_IGNORE)   EXCEPTION(EDATA, __e);
    return _sNul;
}

int cNode::_ixFeatures = NULL_IX;

cNode::cNode() : cPatch(_no_init_)
{
//  DBGOBJ();
    _pFeatures = nullptr;
    toEnd(idIndex());
    cRecord::toEnd();
    _set(cNode::descr());
    bDelCollisionByMac = bDelCollisionByIp = false;
}

cNode::cNode(const cNode& __o) : cPatch(_no_init_)
{
//  DBGOBJ();
    if (__o._pFeatures == nullptr) _pFeatures = nullptr;
    else                           _pFeatures = new cFeatures(*__o._pFeatures);
    _copy(__o, _descr_cNode());
    __cp(__o);
    if (__o.pShares != nullptr) EXCEPTION(EPROGFAIL,0,__o.toString());
    ports.append(__o.ports);
    params.append(__o.params);
    bDelCollisionByMac = __o.bDelCollisionByMac;
    bDelCollisionByIp  = __o.bDelCollisionByIp;
    containerValid = __o.containerValid;
}

/// A többi clone() metódushoz képest eltérés, hogy nem csak azonos típust (azonos descriptor) lehet másolni.
/// Megengedi a cSnmpDevice típusból is a másolást, a többlet adatokat figyelmen kívül hagyja.
cNode& cNode::clone(const cRecord &__o)
{
    if (!(__o.descr() >= descr())) EXCEPTION(EDATA, 0, __o.toString());
    clear();
    __cp(__o);
    set(__o);
    bDelCollisionByMac = false;
    bDelCollisionByIp  = false;
    containerValid     = 0;
    ports.clear();
    params.clear();
    if (typeid(__o) != typeid(cRecordAny)) {
        const cNode& o = *dynamic_cast<const cNode*>(&__o);
        ports.append(o.ports);
        params.append(o.params);
        bDelCollisionByMac = o.bDelCollisionByMac;
        bDelCollisionByIp  = o.bDelCollisionByIp;
        containerValid     = o.containerValid;
    }
    const cNode *po = __o.creconvert<cNode>();
    if (po->_pFeatures == nullptr) pDelete(_pFeatures);
    else features() = po->features();
    return *this;
}

cNode::cNode(const QString& __name, const QString& __descr) : cPatch(_no_init_)
{
//    DBGOBJ();
    _pFeatures = nullptr;
    _set(cNode::descr());
    _set(_descr_cNode().nameIndex(),  __name);
    _set(_descr_cNode().noteIndex(), __descr);
    bDelCollisionByMac = bDelCollisionByIp = false;
}

// -- virtual
const cRecStaticDescr&  cNode::descr() const
{
    if (initPDescr<cNode>(_sNodes)) {
        _ixFeatures = _descr_cNode().toIndex(_sFeatures);
        CHKENUM(_sNodeStat, notifSwitch)
    }
    return *_pRecordDescr;
}

void cNode::clearShares()                                       { EXCEPTION(ENOTSUPP); }
bool cNode::setShare(int, int, int , int, bool)                 { EXCEPTION(ENOTSUPP); }
bool cNode::updateShares(QSqlQuery&, bool, eEx)                 { EXCEPTION(ENOTSUPP); }
cPPort *cNode::addPort(const QString&, const QString &, int)    { EXCEPTION(ENOTSUPP); }
cPPort *cNode::addPorts(const QString&, int , int , int , int ) { EXCEPTION(ENOTSUPP); }
void cNode::insertPort(QSqlQuery&, int, const QString&, const QString&, const QString&) { EXCEPTION(ENOTSUPP); }
void cNode::updateShared(QSqlQuery&, int, int, int, int, bool)  { EXCEPTION(ENOTSUPP); }

void cNode::clearToEnd()
{
    pDelete(_pFeatures);
    ports.clear();
    params.clear();
    containerValid = 0;
}

void cNode::toEnd()
{
    toEnd(idIndex());
    toEnd(_ixFeatures);
    cRecord::toEnd();
}

bool cNode::toEnd(int i)
{
    if (i == idIndex()) {
        if (atEndCont(ports, _sNodeId)) {
            containerValid &= ~(CV_PORTS | CV_PORT_PARAMS | CV_PORT_VLANS);
        }
        if (atEndCont(params, _sNodeId)) {
            containerValid &= ~CV_NODE_PARAMS;
        }
        return true;
    }
    else if (i == _ixFeatures) {
        pDelete(_pFeatures);
    }
    return false;
}


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
                const cInterface *pi = pp->creconvert<cInterface>();
                if (!pi->addresses.isEmpty()) {
                    nt = enum2set(NT_HOST);
                    break;
                }
            }
        }
        setId(ixNodeType, nt);
    }
    // Ütköző objektumok feltételes törlése
    delCollisionByIp(__q);
    delCollisionByMac(__q);
    return cPatch::insert(__q, __ex);
}

bool cNode::rewrite(QSqlQuery &__q, eEx __ex)
{
    // Töröljők az összes régi IP címet (IP címekre nincs replace/rewrite metódus)
    QString sql = "DELETE FROM ip_addresses WHERE port_id IN "
            "(SELECT port_id FROM nports WHERE node_id = ?)";
    qlonglong id = cNode().getIdByName(__q, getName());
    execSql(__q, sql, id);
    // Ütköző objektumok feltételes törlése
    setId(id);
    delCollisionByIp(__q);
    delCollisionByMac(__q);
    if (!tRewrite(__q, ports, CV_PORTS, params, CV_NODE_PARAMS, __ex)) return false;
    return true;
}

bool cNode::rewriteById(QSqlQuery &__q, eEx __ex)
{
    if (isNullId()) EXCEPTION(EDATA, 0, identifying());
    // Töröljők az összes régi IP címet (IP címekre nincs replace/rewrite metódus)
    QString sql = "DELETE FROM ip_addresses WHERE port_id IN "
            "(SELECT port_id FROM nports WHERE node_id = ?)";
    qlonglong id = getId();
    execSql(__q, sql, id);
    // Törölni kell az adatbázisban a port_index mezőket, mert ha változott a sorrend, akkor az probléma.
    sql = "UPDATE nports SET port_index = NULL WHERE node_id = ?";
    execSql(__q, sql, id);
    // Ütköző objektumok feltételes törlése
    delCollisionByIp(__q);
    delCollisionByMac(__q);
    if (!tRewriteById(__q, ports, CV_PORTS, params, CV_NODE_PARAMS, __ex)) return false;
    return true;
}

int  cNode::fetchPorts(QSqlQuery& __q, int flags)
{
    if (flags == 0) flags = (CV_PORTS_ADDRESSES | CV_PORT_VLANS);
    ports.clear();
    // A ports objektum fetch metódusa csak azonos rekord típusok esetén jó...
    QSqlQuery q = getQuery(); // A copy construktor vagy másolás az nem jó!! (shadow copy)
    QString sql = "SELECT tableoid, port_id FROM nports WHERE node_id = ? AND NOT deleted"
                  " ORDER BY port_index NULLS LAST, port_name";
    if (execSql(__q, sql, getId())) do {
        qlonglong tableoid = variantToId(__q.value(0));
        qlonglong port_id  = variantToId(__q.value(1));
        cNPort *p = cNPort::getPortObjById(q, tableoid, port_id, EX_ERROR);
        q.finish();
        if (p == nullptr) return -1;
        ports << p;
        if (tableoid == cNPort::tableoid_interfaces()) {
            cInterface *pi = p->reconvert<cInterface>();
            if (flags & CV_PORTS_ADDRESSES) pi->fetchAddressess(q);
            if (flags & CV_PORT_VLANS)      pi->fetchVlans(q);
            q.finish();
        }
        if (flags & CV_PORT_PARAMS) p->fetchParams(q);
    } while (__q.next());
    __q.finish();
    containerValid |= CV_PORTS | (flags & (CV_PORTS_ADDRESSES | CV_PORT_VLANS | CV_PORT_PARAMS));
    return ports.count();
}

qlonglong cNode::getIdByName(QSqlQuery& __q, const QString& __n, eEx __ex) const
{
    qlonglong id = descr().getIdByName(__q, __n, EX_IGNORE);
    if (id != NULL_ID) return id;
    static const QString sql =
            "SELECT nodes.node_id FROM nodes WHERE "
                "'host' = ANY (node_type) AND "
                "node_name IN "
                    "( SELECT ? || '.' || param_value FROM sys_params JOIN param_types USING(param_type_id) WHERE param_type_name = ? ORDER BY sys_param_name ) "
                "LIMIT 1";
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    __q.bindValue(0,__n);
    __q.bindValue(1,_sSearchDomain);
    _EXECSQL(__q)
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
    bool isSnmpDev = chkObjType<cSnmpDevice>(EX_IGNORE) >= 0;
    if (isSnmpDev && __ix == NULL_IX) EXCEPTION(EDATA, __ix, tr("A port index kötelező"));
    if (__ix != NULL_IX && __ix <= 0) EXCEPTION(EDATA, __ix, tr("Az index nem lehet negatív, vagy nulla"));
    if (ports.count()) {
        if (nullptr != getPort(__name, EX_IGNORE)) EXCEPTION(EDATA, -1, QString("Ilyen port név már létezik: %1").arg(__name));
        if (__ix != NULL_IX && nullptr != getPort(__ix, EX_IGNORE)) EXCEPTION(EDATA, __ix, QString("Ilyen port index már létezik."));
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
    cNPort *p = nullptr;
    for (int i = __from; i <= __to; ++i) {
        p = addPort(__t, nameAndNumber(__np, i + __noff), _sNul, i + __off);
    }
    if (p == nullptr) EXCEPTION(EDATA);
    return p;
}

cNPort *cNode::portModName(int __ix, const QString& __name, const QString& __note)
{
    int i = ports.indexOf(cNPort::ixPortIndex(), __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::tr("Port index not found"));
    cNPort *p = ports[i];
    p->setName(__name);
    if (!__note.isNull()) p->setName(_sPortNote, __note);
    return p;
}

cNPort *cNode::portModType(int __ix, const QString& __type, const QString& __name, const QString& __note, int __new_ix)
{
    int i = ports.indexOf(cNPort::ixPortIndex(), __ix);
    if (i < 0) EXCEPTION(ENOINDEX, __ix, QObject::tr("Port index not found"));
    cNPort *p = ports[i];
    const cIfType& ot = cIfType::ifType(p->getId(_sIfTypeId));
    const cIfType& nt = cIfType::ifType(__type);
    if (nt.getName(_sIfTypeObjType) != ot.getName(_sIfTypeObjType))
        EXCEPTION(EDATA, -1, tr("Invalid port type: %1 to %2").arg(ot.getName().arg(__type)));
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
    cNPort *p = nullptr;
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

int cNode::fetchByIp(QSqlQuery& q, const QHostAddress& a, eEx __ex)
{
    clear();
    QString sql = QString("SELECT %1.* FROM %1 JOIN interfaces USING(node_id) JOIN ip_addresses USING(port_id) WHERE address = ?").arg(tableName());
    QString as = hostAddressToString(a);
    if (execSql(q, sql, as)) {
        set(q);
        int n = q.size();
        if (n > 1) {
            if (__ex >= EX_WARNING) EXCEPTION(AMBIGUOUS,0, tr("A %1 IP nem egyedi").arg(a.toString()));
        }
        return n;
    }
    if (__ex) EXCEPTION(EFOUND,0, tr("A %1 IP.").arg(a.toString()));
    return 0;
}

bool cNode::fetchOnePortByIp(QSqlQuery& q, const QHostAddress& a, eEx __ex)
{
    ports.clear();
    QString sql =
            "SELECT interfaces.*, ip_addresses.*  FROM interfaces JOIN ip_addresses USING(port_id)"
            " WHERE address = ? AND node_id = ?";
    QString as = hostAddressToString(a);
    if (execSql(q, sql, as, getId(_sNodeId))) {
        int from = 0;
        cInterface *pIf = new cInterface();
        cIpAddress *pIp = new cIpAddress();
        pIf->set(q, &from, pIf->cols());
        pIp->set(q, &from);
        if (q.next()) {
            if (__ex) EXCEPTION(AMBIGUOUS,0, tr("A %1 IP nem egyedi").arg(a.toString()));
            delete pIf;
            delete pIp;
            return false;
        }
        pIf->addresses << pIp;
        ports << pIf;
        return true;
    }
    if (__ex) EXCEPTION(EFOUND,0, tr("A %1 IP.").arg(a.toString()));
    return false;
}

int cNode::fetchByMac(QSqlQuery& q, const cMac& a, eEx __ex)
{
    clear();
    QString sql = QString("SELECT DISTINCT ON (node_id) %1.* FROM %1 JOIN interfaces USING(node_id) WHERE hwaddress = ?").arg(tableName());
    if (execSql(q, sql, a.toString())) {
        int n = q.size();
        set(q);
        if (n > 1 && __ex >= EX_WARNING) EXCEPTION(AMBIGUOUS,0, tr("A %1 MAC nem egyedi").arg(a.toString()));
        return n;
    }
    return 0;
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
    if (__ex) EXCEPTION(EFOUND, -1, tr("Self host record"));
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

cInterface *cNode::portSetVlans(const QString& __port, const QVariantList& _ids)
{
    cInterface *p = getPort(__port)->reconvert<cInterface>();
    enum eVlanType t = VT_UNTAGGED;
    int n = 0;
    foreach(QVariant v, _ids) {
        bool ok;
        qlonglong id = v.toLongLong(&ok);
        if (!ok) {
            EXCEPTION(EDATA, n, debVariantToString(v));
        }
        p->joinVlan(id, t, ST_MANUAL);
        t = VT_TAGGED;
        ++n;
    }
    return p;
}

cInterface *cNode::portSetVlans(int __port_index, const QVariantList& _ids)
{
    cInterface *p = getPort(__port_index)->reconvert<cInterface>();
    enum eVlanType t = VT_UNTAGGED;
    int n = 0;
    foreach(QVariant v, _ids) {
        bool ok;
        qlonglong id = v.toLongLong(&ok);
        if (!ok) {
            EXCEPTION(EDATA, n, debVariantToString(v));
        }
        p->joinVlan(id, t, ST_MANUAL);
        t = VT_TAGGED;
        ++n;
    }
    return p;
}

QHostAddress cNode::fetchPreferedAddress(QSqlQuery& q, qlonglong _nid, qlonglong _iid)
{
    bool r;
    if (_iid == NULL_ID) {
        static const QString sql =
                "SELECT address FROM interfaces JOIN ip_addresses USING(port_id)"
                   " WHERE node_id = ?"
                   " ORDER BY preferred ASC";
        r = execSql(q, sql, _nid);
    }
    else {
        static const QString sql =
                "SELECT address FROM ip_addresses"
                   " WHERE port_id = ?"
                   " ORDER BY preferred ASC";
        r = execSql(q, sql, _iid);
    }
    QHostAddress a;
    if (r) {
        const netAddress na = sql2netAddress(q.value(0).toString());
        a = na.addr();
    }
    return a;
}


int cNode::fetchAllAddress(QSqlQuery& q, tRecordList<cIpAddress>& cont, qlonglong __id) const
{
    QString sql =
            "SELECT * FROM interfaces JOIN ip_addresses USING(port_id)"
               " WHERE node_id = ?"
               " ORDER BY preferred ASC";

    qlonglong id = __id < 0 ? getId() : __id;
    PDEB(VERBOSE) << VDEB(id) << endl;
    if (execSql(q, sql, id)) do {
        cIpAddress *pA = new cIpAddress();
        pA->set(q);
        cont << pA;
    } while (q.next());
    return cont.size();
}

QList<QHostAddress> cNode::fetchAllIpAddress(QSqlQuery& q, qlonglong __id) const
{
    QString sql =
            "SELECT address FROM interfaces JOIN ip_addresses USING(port_id)"
               " WHERE node_id = ?"
               " ORDER BY preferred ASC";
    QList<QHostAddress> r;
    qlonglong id = __id < 0 ? getId() : __id;
    PDEB(VERBOSE) << VDEB(id) << endl;
    if (execSql(q, sql, id)) do {
        QVariant v = q.value(0);
        if (!v.isNull()) {
            QHostAddress a;
            a.setAddress(v.toString());
            if (a.isNull()) EXCEPTION(EDATA);
            r << a;
        }
    } while (q.next());
    return r;
}



QHostAddress cNode::getIpAddress(QSqlQuery& q)
{
    if ((containerValid & CV_PORTS_ADDRESSES) == 0) {
        fetchPorts(q, CV_PORTS_ADDRESSES);
    }
    return getIpAddress();
}

QHostAddress cNode::getIpAddress(tRecordList<cNPort>::const_iterator *ppi) const
{
    DBGFN();
    if (ports.isEmpty()) {
        _DBGFNL() << tr(" A %1 elemnek nincsen portja, így IP címe sem.").arg(getName()) << endl;
        return QHostAddress();
    }
    QHostAddress ra;
    qlonglong preferred = LLONG_MAX;
    int ixPreferred = cIpAddress().toIndex(_sPreferred);
    for (tRecordList<cNPort>::const_iterator pi = ports.constBegin(); pi < ports.constEnd(); ++pi) {
        const cNPort *pp = *pi;
        if (pp->tableoid() == cInterface::_descr_cInterface().tableoid()) {
            const cInterface& i = *pp->creconvert<cInterface>();
            for (tOwnRecords<cIpAddress, cInterface>::const_iterator ii = i.addresses.constBegin(); ii < i.addresses.constEnd(); ++ii) {
                const cIpAddress& ip = **ii;
                QHostAddress a = ip.address();
                if (a.isNull()) {
                    continue;
                }
                if (ra.isNull() || (!ip.isNull(ixPreferred) && preferred > ip.getId(ixPreferred))) {
                    ra =  a;
                    if (ppi != nullptr) *ppi = pi;
                    if (!ip.isNull(ixPreferred)) preferred = ip.getId(ixPreferred);
                }
            }
        }
    }
    if (ra.isNull()) {
        _DBGFNL() << tr(" A %1 elemnek nincsen IP címe.").arg(getName()) << endl;
    }
    else {
        _DBGFNL() << tr(" A %1 elem IP címe : %2.").arg(getName(), ra.toString()) << endl;
    }
    return ra;
}

QList<cMac> cNode::getMacs() const
{
    DBGFN();
    QList<cMac> r;
    if (ports.isEmpty()) {
        _DBGFNL() << tr(" A %1 elemnek nincsen portja, így MAC címe sem.").arg(getName()) << endl;
        return r;
    }
    int ix = cInterface().ixHwAddress();
    for (tRecordList<cNPort>::const_iterator pi = ports.constBegin(); pi < ports.constEnd(); ++pi) {
        const cNPort *pp = *pi;
        if (pp->tableoid() == cInterface::_descr_cInterface().tableoid()) {
            const cInterface& i = *static_cast<const cInterface *>(pp);
            if (!i.isNull(ix)) {
                cMac mac = i.getMac(ix);
                if (r.contains(mac)) continue;  // csak egy példány a listába
                r << mac;
            }
        }
    }
    return r;
}

cNode& cNode::asmbAttached(const QString& __n, const QString& __d, qlonglong __place)
{
    clear();
    setName(__n);
    setNote(__d);
    setId(_sPlaceId, __place);
    setId(_sNodeType, enum2set(NT_NODE));
    addPort(_sAttach, _sAttach, _sNul, 1);
    return *this;
}

cNode& cNode::asmbWorkstation(QSqlQuery& q, const QString& __n, const tStringPair *__addr, const QString * __mac, const QString& __d, qlonglong __place)
{
    tStringPair port(_sEthernet, _sEthernet);
    asmbNode(q, __n, &port, __addr, __mac, __d, __place);
    return *this;
}

cNPort& cNode::asmbHostPort(QSqlQuery& q, int ix, const QString& pt, const QString& pn, const tStringPair *ip, const QVariant *mac, const  QString& d)
{
    QHostAddress a;
    cMac m;
    if (ix != NULL_IX && getPort(ix, EX_IGNORE) != nullptr) EXCEPTION(EDATA, ix, tr("Nem egyedi port index"));
    if (                 getPort(pn, EX_IGNORE) != nullptr) EXCEPTION(EDATA, -1, tr("Nem egyedi portnév: %1").arg(pn));
    if (mac != nullptr) {
        if (variantIsInteger(*mac)) {
            int i = ports.indexOf(cPPort::ixPortIndex(), *mac);
            if (i < 0) EXCEPTION(EDATA, mac->toInt(), tr("Hibás port index hivatkozás"));
            m = ports[i]->getId(_sHwAddress);
        }
        else if (variantIsString(*mac)) {
            QString sm = mac->toString();
            if (sm != _sARP && !m.set(sm)) EXCEPTION(EDATA, -1, tr("Nem értelmezhető MAC : %1").arg(sm));
        }
        else EXCEPTION(EDATA);
    }

    cNPort *p = cNPort::newPortObj(pt); // Létrehozzuk a port objektumot
    ports << p;
    p->setName(_sPortName, pn);
    if (ix != NULL_IX) p->setId(_sPortIndex, ix);
    if (!d.isEmpty()) p->setName(_sPortNote, d);
    if (ip != nullptr) {   // Hozzá kell adni egy IP címet
        cInterface *pIf = p->reconvert<cInterface>();    // de akkor a port cInterface kell legyen
        QString sip  = ip->first;   // cím
        QString type = ip->second;  // típusa
        if (sip == _sARP) {     // A címet az ARP adatbázisból kell előkotorni
            if (!m) EXCEPTION(EDATA, -1, tr("Az ip cím felderítéséhez nincs megadva MAC."));
            QList<QHostAddress> al = cArp::mac2ips(q, m);
            if (al.size() < 1) EXCEPTION(EDATA, -1, tr("Az ip cím felderítés sikertelen."));
            if (al.size() > 1) EXCEPTION(EDATA, -1, tr("Az ip cím felderítés nem egyértelmű."));
            a = al.first();
        }
        else if (!sip.isEmpty()) {
            a.setAddress(sip);
        }
        pIf->addIpAddress(a, type);
    }
    if (mac != nullptr) {
        if (mac->toString() == _sARP) {
            if (!a.isNull()) EXCEPTION(EDATA, -1, tr("Az ip cím hányában a MAC nem felderíthető."));
            m = cArp::ip2mac(q, a);
            if (!m) EXCEPTION(EDATA, -1, tr("A MAC cím felderítés sikertelen."));
        }
        if (!m) EXCEPTION(EPROGFAIL);
        p->set(_sHwAddress, QVariant::fromValue(m));
    }
    return *p;
}


cNode& cNode::asmbNode(QSqlQuery& q, const QString& __name, const tStringPair *__port, const tStringPair *__addr, const QString *__sMac, const QString& __note, qlonglong __place, eEx __ex)
{
    QString em;
    QString name = __name;
    clear();
    setNote(__note);
    setId(_sPlaceId, __place);
    QString ips, ipt, ifType, pnm;
    if (__addr != nullptr) { ips = __addr->first; ipt = __addr->second; }      // IP cím és típus
    if (__port != nullptr) { pnm = __port->first; ifType = __port->second; }   // port név és típus
    if (ifType.isEmpty()) ifType = _sEthernet;
    if (pnm.isEmpty())    pnm    = _sEthernet;
    QList<QHostAddress> hal;
    QHostAddress ha;
    cMac ma;
    if (__sMac != nullptr && !__sMac->isEmpty()) {
        if (*__sMac != _sARP && !ma.set(*__sMac)) {
            _stat |= ES_DEFECTIVE;
            em = tr("Nem értelmezhető MAC : %1; Node : %2").arg(*__sMac).arg(toString());
            if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, em);
            DERR() << em << endl;
            APPMEMO(q, em, RS_CRITICAL);
            return *this;
        }
    }

    if (name == _sLOOKUP) {     // Ki kell deríteni a nevet, van címe és portja
        if (ips == _sARP) {     // sőt az ip-t is
            if (!ma) {
                _stat |= ES_DEFECTIVE;
                em = tr("A név nem deríthető ki, nincs adat. Node : %1").arg(toString());
                if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, ma);
                DERR() << em << endl;
                APPMEMO(q, em, RS_CRITICAL);
                return *this;
            }
            hal = cArp::mac2ips(q, ma);
            int n = hal.size();
            if (n != 1) {
                _stat |= ES_DEFECTIVE;
                if (n < 1) {
                    em = tr("A név nem deríthető ki, a %1 MAC-hez nincs IP cím.  Node : %2").arg(*__sMac).arg(toString());
                    if (__ex != EX_IGNORE) EXCEPTION(EFOUND, n, em);
                }
                else{
                    em = tr("A név nem deríthető ki, a %1 MAC-hez több IP cím tartozik. Node : %2").arg(*__sMac).arg(toString());
                    if (__ex != EX_IGNORE) EXCEPTION(AMBIGUOUS,n, em);
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
                em = tr("Nem értelmezhatő IP cím %1. Node : %2").arg(ips).arg(toString());
                if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, em);
                DERR() << em << endl;
                APPMEMO(q, em, RS_CRITICAL);
                return *this;
            }
            else if (!ma) {  // Nincs MAC, van IP
                if (__sMac == nullptr) {
                    em = tr("Hiányzó MAC cím. Node : %1").arg(toString());
                    if (__ex >= EX_WARNING) EXCEPTION(EDATA, -1, em);
                }
                else if (*__sMac == _sARP) ma = cArp::ip2mac(q, ha, __ex);
                if (!ma) {
                    _stat |= ES_DEFECTIVE;
                    if (em.isEmpty()) em = tr("A %1 cím alapján nem deríthatő ki a MAC.").arg(ips);
                    if (__ex != EX_IGNORE) EXCEPTION(EDATA, -1, em);
                    if (__ex >= EX_WARNING) {
                        DERR() << em << endl;
                        APPMEMO(q, em, RS_CRITICAL);
                        return *this;
                    }
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
    if (__addr == nullptr) {                   // Nincs IP címe
        if (__port == nullptr && __sMac == nullptr) {    // Portja sincs
            return *this;                   // Akkor kész
        }
        if (__sMac != nullptr && *__sMac == _sARP) {
            _stat |= ES_DEFECTIVE;
            em = tr("Ebben a kontexusban a MAC nem kideríthető. Node : %1").arg(toString());
            if (__ex) EXCEPTION(EDATA, -1, em);
            DERR() << em << endl;
            APPMEMO(q, em, RS_CRITICAL);
            return *this;
        }
        if (ifType.isEmpty()) ifType = _sEthernet;
        if (pnm.isEmpty())    pnm    = ifType;
        cNPort *p = addPort(ifType, pnm, _sNul, 1);
        if (__sMac == nullptr) return *this;      // Ha nem adtunk meg MAC-et
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
            em = tr("Az IP csak helyes MAC-ból deríthető ki. Node : %1").arg(toString());
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
            em = tr("Nem értelmezhető ip cím : %1. Node : %2").arg(ips).arg(toString());
            if (__ex) EXCEPTION(EIP,-1,em);
            DERR() << em << endl;
            APPMEMO(q, em, RS_CRITICAL);
            return *this;
        }
    }
    if (__sMac != nullptr && *__sMac == _sARP) {
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

int cNode::delCollisionByMac(QSqlQuery &__q)
{
    int n = -1;
    QList<cMac> macs;
    if (bDelCollisionByMac) {
        int ixHwAddress = cInterface().toIndex(_sHwAddress);
        QListIterator<cNPort *> i(ports);
        n = 0;
        while (i.hasNext()) {
            const cNPort *pp = i.next();
            if (!pp->isIndex(ixHwAddress)) continue;
            cMac mac = pp->getMac(ixHwAddress);
            if (macs.indexOf(mac) < 0) macs << mac;
        }
        if (!macs.isEmpty()) {
            QString sql;
            QVariant id = get(idIndex());
            if (id.isNull()) {
                sql = "DELETE FROM nodes WHERE node_id IN "
                        "(SELECT DISTINCT(node_id) FROM interfaces WHERE hwaddress = ?) "
                      "RETURNING *";
            }
            else {
                sql = "DELETE FROM nodes WHERE node_id IN "
                        "(SELECT DISTINCT(node_id) FROM interfaces WHERE hwaddress = ? AND node_id <> ?) "
                      "RETURNING *";
            }
            foreach (cMac mac, macs) {
                execSql(__q, sql, mac.toString(), id);
                int nn = __q.numRowsAffected();
                if (nn) {
                    n += nn;
                    cNode o;
                    o.set(__q);
                    QString msg = tr("Delete %1 node for collision MAC : %2").arg(o.toString(), mac.toString());
                    PDEB(INFO) << msg << endl;
                    APPMEMO(__q, msg, RS_WARNING);
                }
            }
        }
    }
    return n;
}

int cNode::delCollisionByIp(QSqlQuery& __q)
{
    int n = -1;
    if (bDelCollisionByIp) {
        QList<QHostAddress> ips;    // Saját egyedi IP címek listája
        QListIterator<cNPort *> i(ports);
        n = 0;
        while (i.hasNext()) {
            const cNPort *pp = i.next();
            if (pp->tableName() != _sInterfaces) continue;
            QListIterator<cIpAddress *> j(pp->creconvert<cInterface>()->addresses);
            while (j.hasNext()) {
                cIpAddress *pip = j.next();
                qlonglong type = pip->getId(_sIpAddressType);
                if (type != AT_DYNAMIC && type != AT_PRIVATE && type != AT_JOINT) {
                    ips << pip->address();
                }
            }
        }
        if (!ips.isEmpty()) {
            QString sql =
                    "DELETE FROM nodes WHERE node_id IN "
                    "(SELECT node_id FROM interfaces JOIN ip_addresses USING(port_id)"
                       " WHERE  address = ? AND ip_address_type NOT IN ('dynamic', 'private', 'joint')";
            QVariant id = get(idIndex());
            if (id.isNull()) {
                sql += ")";
            }
            else {
                sql += " AND node_id <> ?)";
            }
            sql += " RETURNING *";
            foreach (QHostAddress ip, ips) {
                execSql(__q, sql, ip.toString(), id);
                int nn = __q.numRowsAffected();
                if (nn) {
                    n += nn;
                    cNode o;
                    o.set(__q);
                    QString msg = tr("Delete %1 node for collision IP : %2").arg(o.toString(), ip.toString());
                    PDEB(INFO) << msg << endl;
                    APPMEMO(__q, msg, RS_WARNING);
                }
            }
        }
    }
    return n;
}

cNode * cNode::getSelfNodeObjByMac(QSqlQuery& q)
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    static const QString sql =
            "SELECT DISTINCT(node_id), nodes.tableoid"
            " FROM nodes"
            " JOIN interfaces USING(node_id)"
            " WHERE hwaddress = ?";
    foreach (QNetworkInterface interf, interfaces) {
        cMac m = interf.hardwareAddress();
        if (!m) continue;
        if (execSql(q, sql, m.toString())) {
            qlonglong nid = q.value(0).toLongLong();
            qlonglong oid = q.value(1).toLongLong();
            PDEB(VVERBOSE) << "MAC : " << m.toString() << VDEB(nid) << VDEB(oid) << endl;
            return getNodeObjById(q, oid, nid)->reconvert<cNode>();
        }
    }
    return nullptr;
}


/* ------------------------------ SNMPDEVICES : cSnmpDevice ------------------------------ */

cSnmpDevice::cSnmpDevice() : cNode(_no_init_)
{
    _set(cSnmpDevice::descr());
}

cSnmpDevice::cSnmpDevice(const cSnmpDevice& __o) : cNode(_no_init_)
{
    _set(cSnmpDevice::descr());
    _copy(__o, _descr_cSnmpDevice());
    __cp(__o);
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
    ports.clear();
    params.clear();
    bDelCollisionByMac = false;
    bDelCollisionByIp  = false;
    containerValid = 0;
    if (typeid(cRecordAny) != typeid(__o)) {
        const cSnmpDevice& o = *dynamic_cast<const cSnmpDevice*>(&__o);
        ports.append(o.ports);
        params.append(o.params);
        bDelCollisionByMac = o.bDelCollisionByMac;
        bDelCollisionByIp  = o.bDelCollisionByIp;
        containerValid = o.containerValid;
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
    // Ütköző objektumok feltételes törlése
    delCollisionByIp(__q);
    delCollisionByMac(__q);
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

int cSnmpDevice::snmpVersion() const
{
    QString v = getName(_sSnmpVer);
    if (v.isEmpty()) return SNMP_VERSION_2c;
    if (v == "1")    return SNMP_VERSION_1;
    if (v == "2c")   return SNMP_VERSION_2c;
    if (v == "3")    return SNMP_VERSION_3;
    EXCEPTION(EDATA);
}

bool cSnmpDevice::setBySnmp(const QString& __com, eEx __ex, QString *__pEs, QHostAddress *ip, cTable * _pTable)
{
    QString msgDummy;
    QString *pEs = __pEs == nullptr ? &msgDummy :__pEs;
#ifdef SNMP_IS_EXISTS
    QString community = __com;
    if (community.isEmpty()) {
        community = getName(_sCommunityRd);
        if (community.isEmpty()) {
            community = getName(_sCommunityWr);
            if (community.isEmpty()) community = cSnmp::defComunity;
        }
    }
    setName(_sCommunityRd, community);
    if (ip != nullptr && !ip->isNull()) {
        ;
    }
    else if (ports.isEmpty()) {
        if (isNullId()) {
            if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
            return false;
        }
        QSqlQuery q = getQuery();
        if (!fetchPorts(q)) {
            if (__ex != EX_IGNORE) EXCEPTION(EDATA);
            return false;
        }
    }
    if (setSysBySnmp(*this, __ex, pEs, ip) > 0 && setPortsBySnmp(*this, __ex, pEs, ip, _pTable)) {
        if (isNull(_sNodeType)) {   // Ha nics típus beállítva
            qlonglong nt = enum2set(NT_HOST, NT_SNMP);
            if (ports.size() > 7) nt |= enum2set(NT_SWITCH); // több mint 7 port, meghasaljuk, hogy switch
            setId(_sNodeType, nt);
        }

        containerValid = CV_ALL_NODE | CV_PORTS_ADDRESSES; // VLAN-ok nincsenek (még) lekérdezve!! | CV_PORT_VLANS;
        return true;
    }
    if (__pEs == nullptr) expError(*pEs, true);
#else // SNMP_IS_EXISTS
    (void)__com;
    (void)ip;
    (void)_pTable;
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, -1, snmpNotSupMsg());
    if (pEs != NULL) *pEs = snmpNotSupMsg();
#endif // SNMP_IS_EXISTS
    return false;
}

int cSnmpDevice::open(QSqlQuery& q, cSnmp& snmp, eEx __ex, QString *pEMsg)
{
#ifdef SNMP_IS_EXISTS
    int r = -1;
    QString comn = getName(_sCommunityRd);
    int     ver  = snmpVersion();
    QString lastMsg;
#if 0
    // Open and test, probe IP addresses
    QList<QHostAddress> la = fetchAllIpAddress(q);
    if (la.isEmpty()) {
        lastMsg = tr("A %1 SNMP eszköznek nincs IP címe").arg(getName());
        if (__ex) EXCEPTION(EDATA, -1, lastMsg);
        DERR() << lastMsg << endl;
        if (pEMsg != nullptr) *pEMsg = lastMsg;
        return -1;
    }
    foreach (QHostAddress a, la) {
        r = snmp.open(a.toString(), comn, ver);
        if (r != 0) continue;
        static const QString o = "SNMPv2-MIB::sysDescr";
        r = snmp.getNext(o);
        if (r == 0) break;  // O.K.
        lastMsg = tr("Error snmp.getNext(%1) #%2 (%3); address: %4, node: %5")
                .arg(o).arg(r).arg(snmp.emsg, a.toString(), identifying(false));
        DWAR() << lastMsg << endl;
    }
    if (__ex && r) EXCEPTION(ESNMP, r, lastMsg);
    if (pEMsg != nullptr) *pEMsg = lastMsg;
#else
    // Open only, by prefered IP address
    QHostAddress a = getIpAddress(q);
    if (a.isNull()) {
        lastMsg = tr("A %1 SNMP eszköznek nincs IP címe").arg(getName());
        if (__ex) EXCEPTION(EDATA, -1, lastMsg);
        DERR() << lastMsg << endl;
        if (pEMsg != nullptr) *pEMsg = lastMsg;
    }
    else {
        r = snmp.open(a.toString(), comn, ver);
    }
    return r;
#endif
#else // SNMP_IS_EXISTS
    (void)snmp;
    (void)q;
    if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, -1, snmpNotSupMsg());
    if (pEMsg != NULL) *pEMsg = snmpNotSupMsg();
    return -1;
#endif // SNMP_IS_EXISTS
}

cSnmpDevice * cSnmpDevice::refresh(QSqlQuery& q, const QString& _name, eEx __ex, QString *pEMsg)
{
    cSnmpDevice *p = new cSnmpDevice;
    if (!p->fetchByName(q, _name)) {
        delete p;
        if (__ex != EX_IGNORE) EXCEPTION(ENONAME, 0, _name);
        if (pEMsg != nullptr) msgCat(*pEMsg, tr("%1 SNMP device not found").arg(_name), _sNl);
        return nullptr;
    }
    cSnmpDevice *r = p->refresh(q);
    if (r == nullptr) delete p;
    return r;
}

cSnmpDevice * cSnmpDevice::refresh(QSqlQuery& q, qlonglong _id, eEx __ex, QString *pEMsg)
{
    cSnmpDevice *p = new cSnmpDevice;
    if (!p->fetchById(q, _id)) {
        delete p;
        if (__ex != EX_IGNORE) EXCEPTION(EOID, _id);
        if (pEMsg != nullptr) msgCat(*pEMsg, tr("SNMP device ID %1 not found").arg(_id), _sNl);
        return nullptr;
    }
    cSnmpDevice *r = p->refresh(q);
    if (r == nullptr) delete p;
    return r;
}

cSnmpDevice * cSnmpDevice::refresh(QSqlQuery& q, eEx __ex, QString *pEMsg)
{
    QString em;
    QList<QHostAddress> al = fetchAllIpAddress(q);
    foreach (QHostAddress a, al) {
        if (setBySnmp(_sNul, EX_IGNORE, &em, &a)) {
            if (pEMsg != nullptr) msgCat(*pEMsg, em, _sNl);
            if (R_ERROR == replace(q, EX_ERROR)) {
                if (pEMsg != nullptr) msgCat(*pEMsg, "Replace error." , _sNl);
                return nullptr;
            }
            return this;
        }
    }
    if (__ex != EX_IGNORE) {
        EXCEPTION(ESNMP, 0, em);
    }
    if (pEMsg != nullptr) msgCat(*pEMsg, em, _sNl);
    return nullptr;
}

int nodeObjectType(QSqlQuery& q, const cRecord& o, eEx __ex)
{
    int r;
    qlonglong toid = o.fetchTableOId(q);
    if      (toid == cPatch::     _descr_cPatch()     .tableoid()) r = NOT_PATCH;
    else if (toid == cNode::      _descr_cNode()      .tableoid()) r = NOT_NODE;
    else if (toid == cSnmpDevice::_descr_cSnmpDevice().tableoid()) r = NOT_SNMPDEVICE;
    else    {
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, 0, o.identifying());
        return NOT_INVALID;
    }
    if (typeid(o) == typeid(cRecordAny)) {
        r |= NOT_ANY;
    }
    if (toid != o.tableoid()) {
        r |= NOT_NEQ;
    }
    return r;
}

cPatch * nodeToOrigin(QSqlQuery& q, const cRecord *po, int *pt)
{
    int t = nodeObjectType(q, *po);
    if (t == NOT_INVALID) EXCEPTION(EDATA, 0, po->identifying());
    if ((t & ~NOT_TMSK) == 0) return nullptr;
    cPatch *r = nullptr;
    if (t & NOT_NEQ) {   // Is parent obj., convert to real type
        switch (t & NOT_TMSK) {
        case NOT_PATCH:
            EXCEPTION(EPROGFAIL, 0, po->identifying()); // Incredible
            break;
        case NOT_NODE:
        case NOT_SNMPDEVICE:
            r = cPatch::getNodeObjById(q, po->getId());
            break;
        }
    }
    else if (t & NOT_ANY) {   // Is cRecordAny, convert to real type
        switch (t & NOT_TMSK) {
        case NOT_PATCH:
            r = new cPatch;
            break;
        case NOT_NODE:
            r = new cNode;
            break;
        case NOT_SNMPDEVICE:
            r = new cSnmpDevice;
            break;
        }
        r->copy(*po);
    }
    if (pt != nullptr) *pt = t;
    return r;
}

/* ----------------------------------------------------------------- */

/* ----------------------------------------------------------------- */
int execState(const QString& _n, enum eEx __ex)
{
    if (0 == _n.compare(_sWait,     Qt::CaseInsensitive)) return ES_WAIT;
    if (0 == _n.compare(_sExecute,  Qt::CaseInsensitive)) return ES_EXECUTE;
    if (0 == _n.compare(_sOk,       Qt::CaseInsensitive)) return ES_OK;
    if (0 == _n.compare(_sFaile,    Qt::CaseInsensitive)) return ES_FAILE;
    if (0 == _n.compare(_sAborted,  Qt::CaseInsensitive)) return ES_ABORTED;
    if (__ex) EXCEPTION(EDATA, -1, _n);
    return ENUM_INVALID;
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

cTemplateMap::cTemplateMap(const QString& __type) : tStringMap(), type(__type)
{
    ;
}

cTemplateMap::cTemplateMap(const cTemplateMap& __o) : tStringMap(__o), type(__o.type)
{
    ;
}

const QString& cTemplateMap::get(QSqlQuery& __q, const QString& __name, eEx __ex)
{
    tStringMap::iterator i = find(__name);
    if (i != end()) return i.value();
    cImportTemplate it;
    it[_sTemplateType] = type;
    it[_sTemplateName] = __name;
    if (it.completion(__q) < 1) {
        if (__ex) EXCEPTION(EFOUND, -1, QString(QObject::tr("%1 template name %2 not found.")).arg(type, __name));
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
    if (0 == __n.compare(_sAuth,      Qt::CaseInsensitive)) return VT_AUTH;
    if (0 == __n.compare(_sUnauth,    Qt::CaseInsensitive)) return VT_UNAUTH;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return ENUM_INVALID;
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
    case VT_AUTH:       return _sAuth;
    case VT_UNAUTH:     return _sUnauth;
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
    return ENUM_INVALID;
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
        CHKENUM(_ixVlanType, vlanType)
        CHKENUM(_ixSetType, setType)
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
    if (_imp & RS_BREAK) cErrorException(_src, _lin, _func_name, eError::EBREAK, o.getId(), _memo);
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
    if (pI->pSelfNode        != nullptr) setId(_sNodeId,        pI->pSelfNode->getId());
    if (pI->pSelfHostService != nullptr) setId(_sHostServiceId, pI->pSelfHostService->getId());
    if (pI->pUser            != nullptr) setId(_sUserId,        pI->pUser->getId());
    setName(_sImportance, notifSwitch(_imp));
    setName(_sMemo, _memo);
    return *this;
}

/* -------------------------------------------- selects ---------------------------------------- */
CRECCNTR(cSelect)

const cRecStaticDescr&  cSelect::descr() const
{
    if (initPDescr<cSelect>(_sSelects)) {
        _ixChoice   = _descr_cSelect().toIndex(_sChoice);
//        _ixFeatures = _descr_cSelect().toIndex(_sFeatures);
//        CHKENUM(_ixPatternType, patternType);
    }
    return *_pRecordDescr;
}

int cSelect::_ixChoice   = NULL_IX;
//int cSelect::_ixFeatures = NULL_IX;

CRECDEFD(cSelect)

cSelect& cSelect::choice(QSqlQuery& q, const QString& _type, const QString& _val, eEx __ex)
{
    static QString sql =
            "SELECT * FROM selects "
                "WHERE :styp = select_type "
                  "AND ((pattern_type = 'equal'   AND COALESCE(:sval, '') = pattern) "
                    "OR (pattern_type = 'equali'  AND lower(:sval) = lower(pattern)) "
                    "OR (pattern_type = 'similar' AND :sval similar TO pattern) "
                    "OR (pattern_type = 'regexp'  AND :sval ~  pattern) "
                    "OR (pattern_type = 'regexpi' AND :sval ~* pattern)) "
                "ORDER BY precedence "
                "LIMIT 1;";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(":styp", _type);
    q.bindValue(":sval", _val);
    _EXECSQL(q);
    if (q.first()) set(q);
    else {
        if (__ex >= EX_WARNING) EXCEPTION(EFOUND, 0, _type + " : " + _val);
        set();
    }
    return *this;
}

cSelect& cSelect::choice(QSqlQuery &q, const QString& _type, const cMac _val, eEx __ex)
{
    static QString sql =
            "SELECT * FROM selects "
                "WHERE :styp = select_type AND pattern_type = 'oui' "
                  "AND pattern::macaddr = trunc(:mac::macaddr) "
                "ORDER BY precedence "
                "LIMIT 1;";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(":styp", _type);
    q.bindValue(":mac", _val.toString());
    _EXECSQL(q);
    if (q.first()) set(q);
    else {
        if (__ex >= EX_WARNING) EXCEPTION(EFOUND, 0, _type + " : " + _val.toString());
        set();
    }
    return *this;
}

