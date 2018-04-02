#include "export.h"

static const QString _sSp = " ";
static const QString _sSc = ";";

cExport::cExport(QObject *par) : QObject(par)
{
    sNoAnyObj = QObject::trUtf8("No any object");
    actIndent = 0;
}

QString cExport::head(const QString& kw, cRecord& o)
{
    QString r = kw + _sSp + dQuoted(o.getName());
    QString n = o.getNote();
    if (!n.isEmpty()) r += _sSp + n;
    return r;
}

QString cExport::str(QSqlQuery& q, const cRecordFieldRef &fr, bool sp)
{
    QString r;
    if (fr.isNull()) {
        EXCEPTION(EENODATA, fr.index(), fr.record().identifying());
    }
    r = str(q, fr);
    r = dQuoted(r);
    if (sp) r.prepend(_sSp);
    return r;
}

QString cExport::str_z(QSqlQuery& q, const cRecordFieldRef& fr, bool sp)
{
    QString r;
    if (!fr.isNull()) {
        r = str(q, fr, sp);
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
    switch (fr.descr().eColType) {
    case cColStaticDescr::FT_INTEGER:
        if (fk) return str(q, fr, sp);
        r = QString::number((qlonglong)fr);
        break;
    case cColStaticDescr::FT_INTERVAL:
        r = QString::number((qlonglong)fr);
        break;
    case cColStaticDescr::FT_REAL:
        r = QString::number((double)fr);
        break;
    case cColStaticDescr::FT_TEXT:
    case cColStaticDescr::FT_ENUM:
        r = dQuoted((QString)fr);
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
        QVariant v = fr;
        foreach (QString s, v.toStringList()) {
            r += dQuoted(s) + ", ";
        }
        r.chop(2);
    }
        break;
    case cColStaticDescr::FT_INTEGER_ARRAY:
        if (fk) {
            QVariantList l = ((QVariant&)fr).toList();
            foreach (QVariant vid, l) {
                qlonglong id = vid.toLongLong();
                r += dQuoted(fr.descr().fKeyId2name(q, id)) + ", ";
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
    case cColStaticDescr::FT_BINARY:
    case cColStaticDescr::FT_TIME:
    case cColStaticDescr::FT_DATE:
    case cColStaticDescr::FT_DATE_TIME:
    case cColStaticDescr::FT_POLYGON:
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
    if (!s.isEmpty()) {
        r = line("FEATURES " + dQuoted(s) + _sSc);
    }
    return r;
}

QString cExport::paramLine(QSqlQuery& q, const QString& kw, const cRecordFieldRef& fr)
{
    QString r;
    if (!fr.isNull()) {
        r = line(kw + value(q, fr) + _sSc);
    }
    return r;
}

QString cExport::paramType(QSqlQuery& q)
{
    cParamType o;
    return sympleExport(q, o, o.toIndex(_sParamTypeName));
}
QString cExport::_export(QSqlQuery& q, cParamType& o)
{
    QString r;
    r  = "PARAM" +  str(q, o[_sParamTypeName]) + str_z(q, o[_sParamTypeNote]);
    r += " TYPE" + str(q, o[_sParamTypeType]) + str_z(q, o[_sParamTypeDim]) + _sSc;
    return line(r);
}

QString cExport::sysParams(QSqlQuery& q)
{
    cSysParam o;
    return sympleExport(q, o, o.toIndex(_sSysParamName));
}


QString cExport::_export(QSqlQuery& q, cSysParam& o)
{
    QString r;
    r  = "SYS" + str(q, o[_sParamTypeId]) + " PARAM" + str(q, o[_sSysParamName]);
    r += " =" + value(q, o[_sParamValue]) + _sSc;
    return line(r);
}

QString cExport::tableShapes(QSqlQuery& q)
{
    exportedNames.clear();
    cTableShape o;
    return sympleExport(q, o, o.toIndex(_sTableShapeName));
}

QString cExport::_export(QSqlQuery &q, cTableShape& o)
{
    QString r, s, n;
    r  = "TABLE";
    s = o.getName(_sTableName);
    n = o.getName();
    if (s != n) r += " " + dQuoted(s);
    r += " SHAPE " + dQuoted(n) + str_z(q, o[_sTableShapeNote]);
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
