#include "export.h"

static const QString _sDq = "\"";
static const QString _sSp = " ";
static const QString _sSc = ";";
static const QString _sDELETE_ = "DELETE ";

cExport::cExport(QObject *par) : QObject(par)
{
    sNoAnyObj = QObject::trUtf8("No any object");
    actIndent = 0;
}

QString cExport::escaped(const QString& s)
{
    QString r = _sDq;
    foreach (QChar c, s) {
        if (c.isLetterOrNumber())   r += c;
        else if (c.isNull())        r += "\\0";
        else {
            char cc = c.toLatin1();
            switch (cc) {
            case '"':   r += "\\\"";    break;
            case '\n':  r += "\\n";     break;
            case '\r':  r += "\\r";     break;
            case '\t':  r += "\\t";     break;
            case '\\':  r += "\\\\";    break;
            case 0x1b:  r += "\\e";     break;
            default:
                if (c.isPrint())   r += c;
                else if (cc == 0)  r += "\\?";
                else               r += "\\" + QString::number((int)cc, 8);
                break;
            }
        }
    }
    return r + _sDq;
}

QString cExport::head(const QString& kw, cRecord& o)
{
    QString r = kw + _sSp + escaped(o.getName());
    QString n = o.getNote();
    if (!n.isEmpty()) r += _sSp + escaped(n);
    return r;
}

QString cExport::str(const cRecordFieldRef &fr, bool sp)
{
    QString r;
    if (fr.isNull()) {
        EXCEPTION(EENODATA, fr.index(), fr.record().identifying());
    }
    r = fr.toString();
    r = escaped(r);
    if (sp) r.prepend(_sSp);
    return r;
}

QString cExport::str_z(const cRecordFieldRef& fr, bool sp)
{
    QString r;
    if (!fr.isNull()) {
        r = str(fr, sp);
    }
    return r;
}

QString cExport::value(QSqlQuery& q, const cRecordFieldRef &fr, bool sp)
{
    QString r;
    if (fr.isNull()) {
        EXCEPTION(EENODATA, fr.index(), fr.record().identifying());
    }
    bool fk = false;
    switch (fr.descr().fKeyType) {
    case cColStaticDescr::FT_NONE:
        break;
    case cColStaticDescr::FT_PROPERTY:
    case cColStaticDescr::FT_OWNER:
    case cColStaticDescr::FT_SELF:
        fk = true;
        break;
    case cColStaticDescr::FT_TEXT_ID:
    default:
        EXCEPTION(EDATA, fr.index(), fr.record().identifying());
    }
    QVariant v = fr;
    switch (fr.descr().eColType) {
    case cColStaticDescr::FT_INTEGER:
        if (fk) r = escaped(fr.view(q));
        else    r = QString::number(v.toLongLong());
        break;
    case cColStaticDescr::FT_INTERVAL:
        r = QString::number(v.toLongLong() / 1000);
        break;
    case cColStaticDescr::FT_REAL:
        r = QString::number(v.toDouble());
        break;
    case cColStaticDescr::FT_TEXT:
    case cColStaticDescr::FT_ENUM:
        r = escaped(v.toString());
        break;
    case cColStaticDescr::FT_BOOLEAN:
        r = ((bool)fr) ? "TRUE" : "FALSE";
        break;
    case cColStaticDescr::FT_MAC:
    case cColStaticDescr::FT_INET:
    case cColStaticDescr::FT_CIDR:
        r = fr.view(q);
        break;
    case cColStaticDescr::FT_TEXT_ARRAY:
    case cColStaticDescr::FT_SET: {
        QStringList l = v.toStringList();
        foreach (QString s, l) {
            r += escaped(s) + ", ";
        }
        r.chop(2);
    }
        break;
    case cColStaticDescr::FT_INTEGER_ARRAY:
        if (fk) {
            QVariantList l = v.toList();
            foreach (QVariant vid, l) {
                qlonglong id = vid.toLongLong();
                r += escaped(fr.descr().fKeyId2name(q, id)) + ", ";
            }
            r.chop(2);
        }
        else {
            r = fr.view(q);
        }
        break;
    case cColStaticDescr::FT_REAL_ARRAY:
        r = fr.view(q);
        break;
    case cColStaticDescr::FT_TIME:
    case cColStaticDescr::FT_DATE:
    case cColStaticDescr::FT_DATE_TIME:
    case cColStaticDescr::FT_POLYGON:
    case cColStaticDescr::FT_BINARY:
    default:
        EXCEPTION(EDATA, fr.index(), fr.record().identifying());
        break;
    }
    if (sp) r.prepend(_sSp);
    return r;
}

QString cExport::features(cRecord& o)
{
    QString s = o.getName(_sFeatures);
    QString r;
    if (!s.isEmpty() && s != ":") {
        r = line("FEATURES " + escaped(s) + _sSc);
    }
    return r;
}

QString cExport::paramLine(QSqlQuery& q, const QString& kw, const cRecordFieldRef& fr, const QVariant& _def)
{
    QString r;
    if (!fr.isNull()) { // is NULL, skeep
        QVariant v = fr;
        if (_def.isNull() || _def != v) {    // is default, skeep
            r = line(kw + value(q, fr) + _sSc);
        }
    }
    return r;
}

QString cExport::flag(const QString& kw, const cRecordFieldRef& fr, bool inverse)
{
    QString r;
    if (fr.record().getBool(fr.index()) != inverse) {
        r = line(kw + _sSc);
    }
    return r;
}

QString cExport::lineEndBlock(const QString& s, const QString& b)
{
    QString r = s;
    if (b.isEmpty()) {
        actIndent--;
        r.chop(2);  // drop "{\n"
        return r + _sSc + '\n';
    }
    else {
        return r + b + lineEndBlock();
    }
}

QString cExport::paramType(eEx __ex)
{
    cParamType o;
    return sympleExport(o, o.toIndex(_sParamTypeName), __ex);
}
QString cExport::_export(QSqlQuery& q, cParamType& o)
{
    (void)q;
    QString r;
    r  = "PARAM" +  str(o[_sParamTypeName]) + str_z(o[_sParamTypeNote]);
    r += " TYPE" + str(o[_sParamTypeType]) + str_z(o[_sParamTypeDim]) + _sSc;
    return line(r);
}

QString cExport::sysParams(eEx __ex)
{
    cSysParam o;
    return sympleExport(o, o.toIndex(_sSysParamName), __ex);
}


QString cExport::_export(QSqlQuery& q, cSysParam& o)
{
    QString r;
    r  = "SYS" + str(o[_sParamTypeId]) + " PARAM" + str(o[_sSysParamName]);
    r += " =" + value(q, o[_sParamValue]) + _sSc;
    return line(r);
}

QString cExport::tableShapes(eEx __ex)
{
    exportedNames.clear();
    cTableShape o;
    return sympleExport(o, o.toIndex(_sTableShapeName), __ex);
}

QString cExport::_export(QSqlQuery &q, cTableShape& o)
{
    QString r, s, n;
    r  = "TABLE";
    s = o.getName(_sTableName);
    n = o.getName();
    if (s != n) r += " " + escaped(s);
    r += " SHAPE " + escaped(n) + str_z(o[_sTableShapeNote]);
    r  = lineBeginBlock(r);
    r += paramLine(q, "TABLE TYPE",         o[_sTableShapeType]);
    r += paramLine(q, "TABLE FEATURES",     o[_sFeatures]);
    r += paramLine(q, "AUTO REFRESH",       o[_sAutoRefresh]);
    r += paramLine(q, "REFINE",             o[_sRefine]);
    r += paramLine(q, "TABLE INHERIT TYPE", o[_sTableInheritType]);
    r += paramLine(q, "INHERIT TABLE NAMES",o[_sInheritTableNames]);
    r += paramLine(q, "TABLE VIEW RIGHTS",  o[_sViewRights]);
    r += paramLine(q, "TABLE EDIT RIGHTS",  o[_sEditRights]);
    r += paramLine(q, "TABLE DELETE RIGHTS",o[_sRemoveRights]);
    r += paramLine(q, "TABLE INSERT RIGHTS",o[_sInsertRights]);
    // Right shapes
    if (!o[_sRightShapeIds]) {
        QVariantList ids = o.get(_sRightShapeIds).toList();
        if (!ids.isEmpty()) {
            QStringList names;  // jobb oldali már definiált tábla nevek
            foreach (QVariant vid, ids) {
                QString rn = cTableShape().getNameById(q, vid.toLongLong());
                if (exportedNames.contains(rn)) {   // defined?
                    names << rn;
                }
                else {
                    divert += QString("SET table_shape[\"%1\"].right_shape_ids += ID TABLE SHAPE (\"%2\")").arg(n).arg(rn);
                }
            }
            if (!names.isEmpty()) {
                r += line(QString("RIGHT SHAPE %1;").arg(quotedStringList(names)));
            }
        }
    }
    if (o.shapeFields.isEmpty()) o.fetchFields(q);
    for (int i = 0; i < o.shapeFields.size(); ++i) {
        cTableShapeField& f = *o.shapeFields.at(i);
        r += lineBeginBlock(head("ADD FIELD ", f));
        r += features(f);
        r += paramLine(q, "VIEW RIGHTS",  f[_sViewRights]);
        r += paramLine(q, "EDIT RIGHTS",  f[_sEditRights]);
        r += lineEndBlock();
    }
/*
    r += paramLine(q, "", o[_s]);
    r += paramLine(q, "", o[_s]);
    r += paramLine(q, "", o[_s]);
    r += paramLine(q, "", o[_s]);
    r += paramLine(q, "", o[_s]);
    r += paramLine(q, "", o[_s]);
    r += paramLine(q, "", o[_s]);
*/
    exportedNames << n;
    r += lineEndBlock();
    return r;
}

QString cExport::_export(QSqlQuery &q, cTableShapeField& o)
{
    // Incomplette
    (void)q;
    (void)o;
    return QString();
}


QString cExport::services(eEx __ex)
{
    cService o;
    return sympleExport(o, o.toIndex(_sServiceName), __ex);
}

QString cExport::_export(QSqlQuery &q, cService& o)
{
    QString r;
    static const qlonglong      flapIval   = 30 * 60 * 1000;   // 30 min
    static const qlonglong      flapMax    = 15;
    qlonglong id = o.getId();
    if (id == TICKET_SERVICE_ID || id == NIL_SERVICE_ID) {
        // Fix id, nem lehet exportálni, ill. inmportálni.
        return r;
    }
    r = lineBeginBlock(head("SERVICE", o));
        QString b;
        b  = paramLine(q, "SUPERIOR SERVICE MASK", o[_sSuperiorServiceMask]);
        b += paramLine(q, "PORT",                  o[_sPort]);
        b += paramLine(q, "COMMAND",               o[_sCheckCmd]);
        b += features(o);
        b += paramLine(q, "MAX CHECK ATTEMPTS",    o[_sMaxCheckAttempts]);
        b += paramLine(q, "NORMAL CHECK INTERVAL", o[_sNormalCheckInterval]);
        b += paramLine(q, "HEARTBEAT TIME",        o[_sHeartbeatTime]);
        b += paramLine(q, "RETRY CHECK INTERVAL",  o[_sRetryCheckInterval]);
        b += paramLine(q, "FLAPPING INTERVAL",     o[_sFlappingInterval],   flapIval);
        b += paramLine(q, "FLAPPING MAX CHANGE",   o[_sFlappingMaxChange],  flapMax);
        b += paramLine(q, "TYPE",                  o[_sServiceTypeId],      UNMARKED_SERVICE_TYPE_ID);
        b += paramLine(q, "ON LINE GROUPS",        o[_sOnLineGroupIds],     cColStaticDescrArray::emptyVariantList);
        b += paramLine(q, "OFF LINE GROUPS",       o[_sOffLineGroupIds],    cColStaticDescrArray::emptyVariantList);
        b += paramLine(q, "TIME PERIODS",          o[_sTimePeriodId],       ALWAYS_TIMEPERIOD_ID);
        b += flag(        "DISABLE",               o[_sDisabled]);
    r  = lineEndBlock(r, b);
    return r;
}

QString cExport::queryParser(eEx __ex)
{
    const static QString _sQUERY_PARSER_ = "QUERY PARSER ";
    cQueryParser o;
    tIntVector ord;
    ord << o.toIndex(_sServiceId) << o.toIndex(_sItemSequenceNumber);
    QSqlQuery q = getQuery();
    QSqlQuery q2 = getQuery();
    QString r;
    qlonglong actServiceId = NULL_ID;
    if (o.fetch(q, false, QBitArray(1, false), ord)) {
        do {
            qlonglong serviceId = o.getId(_sServiceId);
            if (serviceId != actServiceId) {
                if (actServiceId != NULL_ID) {  // Close prev. ?
                    r += lineEndBlock();
                }
                QString servicName = escaped(cService().getNameById(q2, serviceId));
                actServiceId = serviceId;
                r += line(_sDELETE_ + _sQUERY_PARSER_ + servicName + _sSc);
                r += lineBeginBlock(_sQUERY_PARSER_ + servicName);
            }
            QString pt = o.getName(_sParseType);
            QString l;
            if      (pt == _sPrep)                  l = "PREP ";
            else if (pt == _sPost)                  l = "POST ";
            else if (o.getBigInt(_sCaseSensitive))  l = "CASE " + str(o[_sRegularExpression]);
            else                                    l =           str(o[_sRegularExpression]);
            r += line(l + str(o[_sImportExpression]) + str_z(o[_sQueryParserNote]) + _sSc);
        } while (o.next(q));
        r += lineEndBlock();
    }
    else {
        if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
    }
    return r;
}

