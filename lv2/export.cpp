#include "export.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
#define swapItemsAt swap
#endif


static const QString _sDq = "\"";
static const QString _sSp = " ";
static const QString _sCo = ",";
static const QString _sEB = "}";
static const QString _sDELETE_      = "DELETE ";
static const QString _sTITLE        = "TITLE";
static const QString _sWHATS_THIS   = "WHATS THIS";
static const QString _sTOOL_TIP     = "TOOL TIP";
static const QString _sNAME         = "NAME";
static const QString _sPARAM        = "PARAM";
static const QString _sFEATURES     = "FEATURES ";
static const QString _sPLACE        = "PLACE";
static const QString _sINENTORY     = "INVENTORY NUMBER";
static const QString _sSERIAL       = "SERIAL NUMBER";
static const QString _sSET          = "SET ";
static const QString _sPORT         = "PORT ";
static const QString _sADD_PORT     = "ADD PORT ";


QStringList cExport::_exportableObjects;
QList<int>  cExport::_exportPotentials;

cExport::cExport(QObject *par) : QObject(par)
{
    sNoAnyObj = QObject::tr("No any object");
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
                else               r += "\\" + QString::number(cc, 8);
                break;
            }
        }
    }
    return r + _sDq;
}

QString cExport::escaped(const QStringList& sl)
{
    QString r;
    foreach (QString s, sl) {
        r += escaped(s) + _sCommaSp;
    }
    r.chop(_sCommaSp.size());
    return r;
}

QString cExport::head(const QString& kw, const QString& s, const QString& n)
{
    QString r = kw + _sSp + escaped(s);
    if (!n.isEmpty()) r += _sSp + escaped(n);
    return r;
}

QString cExport::str(const cRecordFieldConstRef &fr, bool sp)
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

QString cExport::str_z(const cRecordFieldConstRef& fr, bool sp, bool skipEmpty)
{
    QString r;
    bool skip;
    if (skipEmpty) skip = fr.toString().isEmpty();
    else           skip = fr.isNull();
    if (!skip) {
        r = str(fr, sp);
    }
    return r;
}

QString cExport::int_z(const cRecordFieldConstRef &fr, bool sp)
{
    QString r;
    if (!fr.isNull()) {
        r = fr.toString();
    }
    if (sp) r.prepend(_sSp);
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
        r = bool(fr) ? "TRUE" : "FALSE";
        break;
    case cColStaticDescr::FT_MAC:
    case cColStaticDescr::FT_INET:
    case cColStaticDescr::FT_CIDR:
        r = fr.view(q);
        break;
    case cColStaticDescr::FT_TEXT_ARRAY:
    case cColStaticDescr::FT_SET:
        r = escaped(v.toStringList());
        break;
    case cColStaticDescr::FT_INTEGER_ARRAY:
        if (fk) {
            QVariantList l = v.toList();
            foreach (QVariant vid, l) {
                qlonglong id = vid.toLongLong();
                r += escaped(fr.descr().fKeyId2name(q, id)) + _sCommaSp;
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

QString cExport::langComment()
{
    QSqlQuery q = getQuery();
    static const QString sql =
            "SELECT 'default : ' || "
            " COALESCE((SELECT language_name FROM languages WHERE language_id = get_int_sys_param('default_language' )), '?')"
            "  || ' ; failower : ' || "
            " COALESCE((SELECT language_name FROM languages WHERE language_id = get_int_sys_param('failower_language')), '?')";
    execSql(q, sql);
    cLanguage *pAct = lanView::getInstance()->pLanguage;
    if (pAct == nullptr) EXCEPTION(EPROGFAIL);
    QString r;
    r  = "// Act Language : " + pAct->getName();
    r += "; ( " + q.value(0).toString() + ")\n";
    return r;
}

QString cExport::lineText(const QString& kw, const cRecord& o, int _tix)
{
    QString r;
    QString t = o.getText(_tix);
    if (!t.isEmpty()) r = line(kw + _sSp + quotedString(t) + _sSemicolon);
    return r;
}

QString cExport::lineTitles(const QString& kw, const cRecord& o, int _fr, int _to)
{
    QString r;
    static const QString _sCommaNULL = ",NULL";
    int i;
    QString last = o.getText(_fr);
    r += kw + _sSp + quotedString(last);
    for (i = _fr +1; i <= _to; ++i) {
        QString t = o.getText(i);
        if (t == last) {
            if (t.isNull()) r += _sCommaNULL;
            else            r += ",@";
        }
        else {
            last = t;
            r += _sCo + quotedString(last);
        }
    }
    while (r.endsWith(_sCommaNULL)) r = r.left(r.size() - _sCommaNULL.size()); // All NULLs are dropped from the end.
    r = line(r + _sSemicolon);
    return r;
}

QString cExport::features(const cRecord& o)
{
    QString s = o.getName(_sFeatures);
    QString r;
    if (!s.isEmpty() && s != ":") {
        cFeatures features;
        if (!features.split(s, EX_IGNORE)) {
            r  = line(QString("// ") + tr("Invalid features:"));
            r += line("//" + _sFEATURES + escaped(s) + _sSemicolon);
            return r;
        }
        if (s.size() < 32) {
            return line(_sFEATURES + escaped(s) + _sSemicolon); // Small, print raw string
        }
        r = lineBeginBlock(_sFEATURES);
        QString b;
        QRegExp isMap("\\[\\w+\\s*=\\s*\\w+");
        QStringList keys = features.keys();
        foreach (QString key, keys) {
            QString v = features[key];
            if (v.isEmpty()) {
                b += line(escaped(key) + _sSemicolon);
            }
            else {
                tStringMap map = cFeatures::value2map(v);
                if (map.isEmpty()) {
                    b += line(escaped(key) + " = " + escaped(v) + _sSemicolon);
                }
                else {
                    b += lineBeginBlock(escaped(key) + " = ");
                    QString bb;
                    QStringList subKeys = map.keys();
                    foreach (QString subKey, subKeys) {
                        v = map[subKey];
                        bb += line(escaped(subKey) + " = " + escaped(v) + _sSemicolon);
                    }
                    b = lineEndBlock(b, bb);
                }
            }
        }
        r = lineEndBlock(r, b);
    }
    return r;
}

QString cExport::paramLine(QSqlQuery& q, const QString& kw, const cRecordFieldRef& fr, const QVariant& _def)
{
    QString r;
    if (!fr.isNull()) { // is NULL, skeep
        QVariant v = fr;
        if (_def.isNull() || _def != v) {    // is default, skeep
            r = line(kw + value(q, fr) + _sSemicolon);
        }
    }
    return r;
}

QString cExport::flag(const QString& kw, const cRecordFieldRef& fr, bool inverse)
{
    QString r;
    if (fr.record().getBool(fr.index()) != inverse) {
        r = line(kw + _sSemicolon);
    }
    return r;
}

QString cExport::lineEndBlock()
{
    actIndent--; return line(_sEB);
}

QString cExport::lineEndBlock(const QString& s, const QString& b)
{
    QString r = s;
    if (b.isEmpty()) {
        actIndent--;
        r.chop(2);  // drop "{\n"
        return r + _sSemicolon + '\n';
    }
    else {
        return r + b + lineEndBlock();
    }
}

QString cExport::block(const QString& head, const QString& bb)
{
    QString b;
    if (!bb.isEmpty()) {
        b  = lineBeginBlock(head);
        b += bb;
        b += lineEndBlock();
    }
    return b;
}

/* ======================================================================================== */


const QStringList& cExport::exportableObjects()
{
    if (_exportableObjects.isEmpty()) {
#define X(e, f)    _exportableObjects << _s##e##s; _exportPotentials << f;
        X_EXPORTABLE_OBJECTS
#undef X
    }
    return _exportableObjects;
}

QStringList cExport::exportable(int msk)
{
    QStringList sl;
#define X(e, f)    if (f & msk) { sl << _s##e##s; }
        X_EXPORTABLE_OBJECTS
#undef X
    return sl;
}

QString cExport::exportTable(const QString& name, eEx __ex)
{
    int ix = exportableObjects().indexOf(name);
    QString r;
    if (ix < 0) {
        if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, ix, tr("Invalid or unsupported object name %1").arg(name));
    }
    else {
        r = exportTable(ix, __ex);
    }
    return r;
}

QString cExport::exportTable(int ix, eEx __ex)
{
    QString r;
    switch (ix) {
#define X(e, f)    case EO_##e:   r = e##s(__ex);    break;
    X_EXPORTABLE_OBJECTS
#undef X
    default:
        if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, 0, tr("Invalid or unsupported object index %1").arg(ix));
    }
    return r;
}

QString cExport::exportObject(QSqlQuery& q, cRecord &o, eEx __ex)
{
    QString r;
    QString tableName = o.tableName();
    int ix = exportableObjects().indexOf(tableName);
    if (ix < 0) {
        if (__ex != EX_IGNORE) EXCEPTION(ENOTSUPP, 0, tr("Invalid or unsupported object index %1").arg(ix));
        return r;
    }
    cExport x;
    if (typeid (o) == typeid (cRecordAny)) {
        switch (ix) {
    #define X(e, f)    case EO_##e: { c##e oo; oo.copy(o); r = x._export(q, oo); } break;
        X_EXPORTABLE_OBJECTS
    #undef X
        default:
            EXCEPTION(EPROGFAIL);
        }
    }
    else {
        switch (ix) {
    #define X(e, f)    case EO_##e: r = x._export(q, *o.reconvert<c##e>()); break;
        X_EXPORTABLE_OBJECTS
    #undef X
        default:
            EXCEPTION(EPROGFAIL);
        }
    }
    return r;
}

/* ======================================================================================== */

QString cExport::ParamTypes(eEx __ex)
{
    cParamType o;
    return sympleExport(o, o.toIndex(_sParamTypeName), __ex);
}
QString cExport::_export(QSqlQuery& q, cParamType& o)
{
    (void)q;
    QString r;
    r  = _sPARAM +  str(o.cref(_sParamTypeName)) + str_z(o.cref(_sParamTypeNote));
    r += " TYPE" + str(o.cref(_sParamTypeType)) + str_z(o.cref(_sParamTypeDim)) + _sSemicolon;
    return line(r);
}

QString cExport::SysParams(eEx __ex)
{
    cSysParam o;
    return sympleExport(o, o.toIndex(_sSysParamName), __ex);
}


QString cExport::_export(QSqlQuery& q, cSysParam& o)
{
    QString r;
    r  = "SYS" + value(q, o[_sParamTypeId]) + _sSp + _sPARAM + str(o.cref(_sSysParamName));
    r += " =" + value(q, o[_sParamValue]) + _sSemicolon;
    return line(r);
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::IfTypes(eEx __ex)
{
    cIfType o;
    return sympleExport(o, o.toIndex(_sIfTypeIanaId), __ex);
}
QString cExport::_export(QSqlQuery &q, cIfType& o)
{
    (void)q;
    QString r;
    static const QString kw = "IFTYPE";
    r  = head(kw, o) + " IANA " + o.getName(_sIfTypeIanaId);
    if (o.isNull(_sIanaIdLink)) {
        r += _sSp + o.getName(_sIfTypeObjType) + _sSp + o.getName(_sIfTypeLinkType);
        if (o.getBool(_sPreferred)) r += " PREFERED";
    }
    else {
        r += " TO " + o.getName(_sIanaIdLink);
    }
    r += _sSemicolon;
    r = line(r);
    if (!o.isNull(_sIfNamePrefix)) {
        QString prefix = o.getName(_sIfNamePrefix);
        if (!prefix.isEmpty()) {
            r += line("SET " + head(kw, o) + " NAME PREFIX " + quotedString(prefix) + _sSemicolon);
        }
    }
    return r;
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::TableShapes(eEx __ex)
{
    exportedNames.clear();
    divert.clear();
    cTableShape o;
    QString r;
    r  = langComment();
    r += sympleExport(o, o.toIndex(_sTableShapeName), __ex);
    r += divert;
    divert.clear();
    return r;
}

QString cExport::_export(QSqlQuery &q, cTableShape& o)
{
    QString r, s, n;
    o.fetchText(q);
    r  = "TABLE";
    s = o.getName(_sTableName);
    n = o.getName();
    if (s != n) r += " " + escaped(s);
    r += " SHAPE " + escaped(n) + str_z(o[_sTableShapeNote]);
    r  = lineBeginBlock(r);
    r += lineTitles(_sTITLE, o, cTableShape::LTX_TABLE_TITLE, cTableShape::LTX_NOT_MEMBER_TITLE);
    r += paramLine(q, "TYPE",               o[_sTableShapeType],    QVariant(QStringList()));
    r += features(o);
    r += paramLine(q, "AUTO REFRESH",       o[_sAutoRefresh],       QVariant(0LL));
    r += paramLine(q, "REFINE",             o[_sRefine]);
    r += paramLine(q, "INHERIT TYPE",       o[_sTableInheritType]);
    r += paramLine(q, "INHERIT TABLE NAMES",o[_sInheritTableNames]);
    r += paramLine(q, "STYLE SHEET ",       o[_sStyleSheet]);
    r += paramLine(q, "VIEW RIGHTS",        o[_sViewRights]);
    r += paramLine(q, "EDIT RIGHTS",        o[_sEditRights]);
    r += paramLine(q, "DELETE RIGHTS",      o[_sRemoveRights]);
    r += paramLine(q, "INSERT RIGHTS",      o[_sInsertRights]);
    // Right shapes
    if (!o.isNull(_sRightShapeIds)) {
        QVariantList ids = o.get(_sRightShapeIds).toList();
        if (!ids.isEmpty()) {
            QStringList names;  // jobb oldali már definiált tábla nevek
            QStringList undef;  // jobb oldali még nem definiált tábla nevek
            foreach (QVariant vid, ids) {
                QString rn = cTableShape().getNameById(q, vid.toLongLong());
                if (exportedNames.contains(rn)) {   // defined?
                    names << rn;
                }
                else {
                    undef << rn;
                    divert += QString("SET table_shapes[\"%1\"].right_shape_ids += ID TABLE SHAPE (\"%2\")").arg(n).arg(rn) + _sSemicolonNl;
                }
            }
            if (!names.isEmpty()) {
                r += line(QString("RIGHT SHAPE %1;").arg(quotedStringList(names)));
            }
            if (!undef.isEmpty()) {
                r += line(QString("// RIGHT SHAPE %1;").arg(quotedStringList(undef)));
            }
        }
    }
    if (o.shapeFields.isEmpty()) o.fetchFields(q, true);    // raw = true
    QMap<int, QStringList>      ordFields;  ///< Sequence Number - field name
    QMap<QString, QStringList>  ordTypes;   ///< Types           - field list
    for (int i = 0; i < o.shapeFields.size(); ++i) {
        cTableShapeField& f = *o.shapeFields.at(i);
        f.fetchText(q);
        QString qfn = quotedString(f.getName());
        QString qtn = quotedString(f.getName(_sTableFieldName));
        QString h = "ADD FIELD " + qtn;
        if (qtn != qfn) h += " AS " + qfn;
        if (!f.getNote().isEmpty()) h += _sSp + quotedString(f.getNote());
        r += lineBeginBlock(h);
        QString b;
        b  = lineTitles(_sTITLE, f, cTableShapeField::LTX_TABLE_TITLE, cTableShapeField::LTX_DIALOG_TITLE);
        b += lineText(_sTOOL_TIP,   f, cTableShapeField::LTX_TOOL_TIP);
        b += lineText(_sWHATS_THIS, f, cTableShapeField::LTX_WHATS_THIS);
        b += features(f);
        b += paramLine(q, "FLAG",         f[_sFieldFlags], QVariant(QStringList()));
        b += paramLine(q, "DEFAULT VALUE",f[_sDefaultValue]);
        b += paramLine(q, "VIEW RIGHTS",  f[_sViewRights]);
        b += paramLine(q, "EDIT RIGHTS",  f[_sEditRights]);
        r  = lineEndBlock(r, b);
        qlonglong   ords = f.getId(_sOrdTypes);
        if (ords != 0 && ords != ENUM2SET(OT_NO)) {
            eOrderType dtype = eOrderType(f.getId(_sOrdInitType));
            QString types = orderType(dtype);
            ords &= ~ENUM2SET(dtype);
            qlonglong ord;
            int e;
            for (e = 0, ord = 1; ords >= ord; ++e, ord <<= 1) {
                if (ords & ord) {
                    types += _sCommaSp + orderType(e);
                }
            }
            ordTypes[types] << qfn;
            ordFields[int(f.getId(_sOrdInitSequenceNumber))] << qfn;
        }
    }
    if (!ordTypes.isEmpty()) {
        foreach (QString types, ordTypes.keys()) {
            r += line("FIELD " + ordTypes[types].join(_sCommaSp) + " ORD " + types + _sSemicolon);
        }
        QList<int> sn = ordFields.keys();
        std::sort(sn.begin(), sn.end());
        QString s;
        foreach (int n, sn) {
            if (!s.isEmpty()) s += _sCommaSp;
            s += ordFields[n].join(_sCommaSp);
        }
        r += line("FIELD ORD SEQUENCE " + s + _sSemicolon);

    }
    exportedNames << n;
    r += lineEndBlock();
    return r;
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::MenuItems(eEx __ex)
{
    static const QString sql = "SELECT DISTINCT(app_name) FROM menu_items ORDER BY app_name ASC";
    QSqlQuery q = getQuery();
    QString r;
    r  = langComment();
    if (execSql(q, sql)) {
        do {
            QString sAppName = q.value(0).toString();
            r += line(QString("DELETE GUI %1 MENU;").arg(quotedString(sAppName)));
            r += lineBeginBlock("GUI " + quotedString(sAppName));
            r += MenuItems(sAppName, NULL_ID, __ex);
            r += lineEndBlock();
        } while (q.next());
    }
    else {
        if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
    }
    return r;
}

QString cExport::MenuItems(const QString& _app, qlonglong upperMenuItemId, eEx __ex)
{
    QSqlQuery q = getQuery();
    QSqlQuery q2 = getQuery();
    QString r;
    cMenuItem o;
    if (o.fetchFirstItem(q, _app, upperMenuItemId)) do {
        if (_app != o.getName(_sAppName)) APPMEMO(q2, QObject::tr("App name is %1 : menu item %2, invalid app name : %3").arg(_app, o.identifying(false), o.getName(_sAppName)), RS_CRITICAL);
        r += _export(q2, o);
    } while (o.next(q));
    else {
        if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
    }
    return r;

}

QString cExport::_export(QSqlQuery &q, cMenuItem &o)
{
    (void)q;
    QString r;
    o.fetchText(q);
    int t = int(o.getId(_sMenuItemType));
    switch (t) {
    case MT_SHAPE:
        r = head("SHAPE", o);
        break;
    case MT_OWN:
        r = head("OWN", o);
        break;
    case MT_EXEC:
        r = head("EXEC", o);
        break;
    case MT_MENU:
        r = head("MENU", o);
        break;
    default:
        EXCEPTION(EDATA, t, o.identifying(false));
    }
    r = lineBeginBlock(r);
    QString n = o.getName();
    if (t != MT_MENU) {
        QString p = o.getParam();
        if (p != n) r += line(_sPARAM + _sSp + quotedString(p) + _sSemicolon);
    }
    r += features(o);
    r += lineTitles(_sTITLE, o, cMenuItem::LTX_MENU_TITLE, cMenuItem::LTX_TAB_TITLE);
    r += lineText(_sTOOL_TIP, o, cMenuItem::LTX_TOOL_TIP);
    r += lineText(_sWHATS_THIS, o, cMenuItem::LTX_WHATS_THIS);
    if (t == MT_MENU) {
        r += MenuItems(o.getName(_sAppName), o.getId());
    }
    r += lineEndBlock();
    return r;
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::EnumVals(eEx __ex)
{
    cEnumVal o;
    QString r;
    QSqlQuery q = getQuery();
    r  = langComment();
    return r + sympleExport(o, o.ixEnumTypeName(), __ex, o.mask(o.ixEnumValName()));
}

QString cExport::_export(QSqlQuery &q, cEnumVal &o)
{
    QString r;
    bool isType = o.isNull(o.ixEnumValName());
    if (isType) {
        r = "ENUM" + str(o[o.ixEnumTypeName()]) + str_z(o[_sEnumValNote]);
    }
    else {
        r = str(o[o.ixEnumValName()]) + str_z(o[_sEnumValNote]);
    }
    o.fetchText(q);
    r = lineBeginBlock(r);
        QString b;
        b  = lineTitles("VIEW", o, cEnumVal::LTX_VIEW_LONG, cEnumVal::LTX_VIEW_SHORT);
        b += paramLine(q, "ICON",             o[_sIcon]);
        b += paramLine(q, "BACKGROUND COLOR", o[_sBgColor]);
        b += paramLine(q, "FOREGROUND COLOR", o[_sFgColor]);
        b += paramLine(q, "FONT FAMILY",      o[_sFontFamily]);
        b += paramLine(q, "FONT ATTR",        o[_sFontAttr],    QVariant(QStringList()));
        b += lineText(_sTOOL_TIP, o, cEnumVal::LTX_TOOL_TIP);
        if (isType) {
            static tIntVector ord;
            static QBitArray  fm;
            if (ord.isEmpty()) {
                ord << o.ixEnumValName();
                fm = o.mask(o.ixEnumTypeName());
            }
            static const QString where = "enum_val_name IS NOT NULL AND enum_type_name = ?";
            b += exportWhere(o, ord, where, EX_ERROR, fm);
        }
    return lineEndBlock(r, b);
}

/* ---------------------------------------------------------------------------------------- */
QString cExport::ServiceTypes(eEx __ex)
{
    cServiceType o;
    QString r;
    r  =  langComment();
    r += sympleExport(o, o.toIndex(_sServiceTypeName), __ex);
    return r;
}

QString cExport::_export(QSqlQuery &q, cServiceType& o)
{
    static const QString sServiceType = "SERVICE TYPE ";
    QString r = line(head(sServiceType, o) + _sSemicolon);
    cAlarmMsg am;
    am.setId(_sServiceTypeId, o.getId());
    bool f = am.fetch(q, false, am.mask(_sServiceTypeId), am.iTab(_sStatus));
    if (f) {
        tRecordList<cAlarmMsg>  aml;
        do {
            aml << am;
        } while (am.next(q));
        actIndent++;
        for (int i = 0; i < aml.size(); ++i) {
            cAlarmMsg *pam = aml.at(i);
            pam->fetchText(q);
            QString l = "MESSAGE "
                    + escaped(pam->getText(cAlarmMsg::LTX_SHORT_MSG))
                    + _sSp
                    + escaped(pam->getText(cAlarmMsg::LTX_MESSAGE))
                    + _sSp
                    + sServiceType
                    + escaped(o.getName())
                    + _sSp
                    + escaped(pam->getName(_sStatus))
                    + _sSemicolon;
            r += line(l);
        }
        actIndent--;
    }
    return r;
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::Services(eEx __ex)
{
    cService o;
    return sympleExport(o, o.toIndex(_sServiceName), __ex);
}

QString cExport::_export(QSqlQuery &q, cService& o)
{
    QString r;
    static qlonglong      flapIval   = NULL_ID;
    static qlonglong      flapMax    = NULL_ID;
    if (flapIval == NULL_ID) {
        flapIval = getColDefaultInterval(_sServices, _sFlappingInterval);
        flapMax  = getColDefaultInteger(_sServices, _sFlappingMaxChange);
    }
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

QString cExport::hostService(QSqlQuery& q, qlonglong _id)
{
    cHostService hs;
    hs.setById(q, _id);
    QString r, s;
    qlonglong id, id2;
    id = hs.getId(_sNodeId);
    s  = cNode().getNameById(q, id);
    r += s;
    id2 = hs.getId(_sPortId);
    if (id2 != NULL_ID) {
        cNPort p;
        if (p.fetchById(q, id2)) {
            if (p.getId(_sNodeId) != id) {
                r+= tr("/* invalid port : %1 (#%2) */").arg(p.getFullName(q)).arg(id2);
            }
            else {
                r += ":" + s;
            }
        }
        else {
            r+= tr("/* invalid port_id : %1").arg(id2);
        }
    }
    id = hs.getId(_sServiceId);
    s  = cService::service(q, id)->getName();
    r += "." + escaped(s);
    id = hs.getId(_sProtoServiceId);
    id2= hs.getId(_sPrimeServiceId);
    if (id != NIL_SERVICE_ID || id2 != NIL_SERVICE_ID) {
        r += "(";
        if (id != NIL_SERVICE_ID) {
            s = cService::service(q, id)->getName();
            r += escaped(s);
        }
        r += ":";
        if (id != NIL_SERVICE_ID) {
            s = cService::service(q, id2)->getName();
            r += escaped(s);
        }
        r += ")";
    }
    return r;
}

/* ---------------------------------------------------------------------------------------- */
// A servoce_rrd_vars -t nem kezeli, a parser sem!!!

QString cExport::ServiceVars(eEx __ex)
{
    (void)__ex;
    cServiceVar o;
    static const QString sql =
            "SELECT * FROM service_vars ORDER BY host_service_id2name(host_service_id) ASC, service_var_name ASC";
    QSqlQuery q = getQuery();
    QSqlQuery q2 = getQuery();
    QString r;
    if (execSql(q, sql)) {
        do {
            o.set(q);
            r += _export(q2, o);
        } while (q.next());
    }
    return r;
}

QString cExport::_export(QSqlQuery &q, cServiceVar& o)
{
    (void)q;
    QString r, s, ss;
    qlonglong id;
    id = o.getId(_sHostServiceId);
    s  = "SERVICE VAR " + hostService(q, id) + _sSp;
    s += escaped(o.getName());
    ss = o.getNote();
    if (!ss.isEmpty()) s += _sSp + ss;
    ss += " TYPE " + o.varType(q)->getName();
    r = lineBeginBlock(s);
        QString b;
        b  = paramLine(q, "DELEGATE SERVICE STATE", o[_sDelegateServiceState], false);
        b += paramLine(q, "DELEGATE PORT STATE",    o[_sDelegatePortState],    false);
        b += paramLine(q, "RAREFACTION",            o[_sRarefaction],          1);
        b += features(o);
    r = lineEndBlock(r, b);
    return r;
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::ServiceVarTypes(eEx __ex)
{
    cServiceVarType o;
    return sympleExport(o, o.toIndex(_sServiceVarTypeName), __ex);
}

static QString varFilter(const QString& _kv, qlonglong _t, bool _i, const QVariant& _p1, const QVariant& _p2)
{
    QString r;
    if (_t != NULL_ID) {
        r  = _kv + _sSp;
        if (_i) r += "INVERSE ";
        r += cExport::escaped(filterType(int(_t)));
        if (!_p1.isNull()) {
            r += _sSp + cExport::escaped(_p1.toString());
            if (!_p2.isNull()) {
                r += _sCommaSp + cExport::escaped(_p2.toString());
            }
        }
        r += _sSemicolon;
    }
    return r;
}

QString cExport::_export(QSqlQuery &q, cServiceVarType &o)
{
    (void)q;
    QString r;
    r = lineBeginBlock(head("SERVICE VAR TYPE", o));
        QString b;
        QString s;
        QString valType = cParamType::paramType(o.getId(_sParamTypeId)).getName();
        QString rawType = cParamType::paramType(o.getId(_sRawParamTypeId)).getName();
        QString type;
        type = escaped(valType);
        if (valType != rawType) type += _sCommaSp + escaped(rawType);
        if (!o.isNull(_sServiceVarType)) type += _sSp + escaped(o.getName(_sServiceVarType));
        b  = line("TYPE " + type + _sSemicolon);
        s = varFilter("PLAUSIBILITY", o.getId(_sPlausibilityType), o.getBool(_sPlausibilityInverse), o.get(_sPlausibilityParam1), o.get(_sPlausibilityParam2));
        if (!s.isEmpty()) b += line(s);
        s = varFilter("CRITICAL", o.getId(_sCriticalType), o.getBool(_sCriticalInverse), o.get(_sCriticalParam1), o.get(_sCriticalParam2));
        if (!s.isEmpty()) b += line(s);
        s = varFilter("WARNING", o.getId(_sWarningType), o.getBool(_sWarningInverse), o.get(_sWarningParam1), o.get(_sWarningParam2));
        if (!s.isEmpty()) b += line(s);
        if (!o.isNull(_sRawToRrd)) {
            s = "RAW TO RRD;";
            if (!o.getBool(_sRawToRrd)) s.prepend("NO ");
            b += line(s);
        }
        b += features(o);
    r  = lineEndBlock(r, b);
    return r;
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::QueryParsers(eEx __ex)
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
                r += line(_sDELETE_ + _sQUERY_PARSER_ + servicName + _sSemicolon);
                r += lineBeginBlock(_sQUERY_PARSER_ + servicName);
            }
            int pt = int(o.getId(_sParseType));
            QString l;
            static const qlonglong defAttr = ENUM2SET(RA_EXACTMATCH);
            switch (pt) {
            case PT_PREP:   l = "PREP ";    break;
            case PT_POST:   l = "POST ";    break;
            case PT_PARSE:
                l = "TEMPLATE ";
                if (defAttr != o.getId(_sRegexpAttr)) {
                    l += "ATTR(" + o.getName(_sRegexpAttr) + ") ";
                }
                l += str(o[_sRegularExpression]) + _sSp;
                break;
            default:
                EXCEPTION(EPROGFAIL);
            }
            r += line(l + str(o[_sImportExpression]) + str_z(o[_sQueryParserNote]) + _sSemicolon);
        } while (o.next(q));
        r += lineEndBlock();
    }
    else {
        if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
    }
    return r;
}

/// Not supported
QString cExport::_export(QSqlQuery&, cQueryParser&)
{
    EXCEPTION(ENOTSUPP);
    return QString();
}
/* ---------------------------------------------------------------------------------------- */

QString cExport::Patchs(eEx __ex, bool only)
{
    cPatch o;
    QSqlQuery q = getQuery();
    QSqlQuery q2 = getQuery();
    QString r;
    if (o.fetch(q, only, QBitArray(1, false), o.iTab(_sNodeName))) {
        do {
            r += _export(q2, o, only);
        } while (o.next(q));
    }
    else {
        if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
    }
    return r;
}

template <class O, class P> QString cExport::oParam(tOwnRecords<P, O>& list)
{
    QString r;
    foreach (P *p, list.list()) {
        r += line(_sPARAM + str((*p)[p->nameIndex()]) + " =" + str((*p)[_sParamValue]) + "::" + escaped(p->typeName()) + _sSemicolon);
    }
    return r;
}

QString cExport::_export(QSqlQuery& q, cPatch& o, bool only)
{
    if (!only) {
        qlonglong table_oid = o.fetchTableOId(q);
        if (o.tableoid() != table_oid) {
            cNode no;
            if (no.tableoid() == table_oid) {
                no.setId(o.getId());
                if (!no.completion(q)) EXCEPTION(EDATA, o.getId(), o.identifying());
                return _export(q, no, true);
            }
            else {
                cSnmpDevice so;
                if (so.tableoid() == table_oid) {
                    so.setId(o.getId());
                    if (!so.completion(q)) EXCEPTION(EDATA, o.getId(), o.identifying());
                    return _export(q, so);
                }
                else {
                    EXCEPTION(EDATA, table_oid, o.identifying());
                }
            }
        }
    }
    o.fetchPorts(q);
    o.fetchParams(q);
    QString r, b, s;

    r  = lineBeginBlock(head("PATCH", o));
        b  = paramLine(q, _sPLACE,    o[_sPlaceId]);
        b += paramLine(q, _sINENTORY, o[_sInventoryNumber]);
        b += paramLine(q, _sSERIAL,   o[_sSerialNumber]);
        b += oParam(o.params);
        QList<cNPort *> pl = o.ports.list();
        QStringList nc; // Not connected port indexes
        QMap<qlonglong, tStringPair> shared;
        foreach (cNPort *pp, pl) {
            cPPort& port = *(pp->reconvert<cPPort>());
            QString six = port.getName(_sPortIndex);
            ePortShare sh = ePortShare(port.getId(_sSharedCable));
            // Not connected
            if      (sh == ES_NC) nc << six;
            // Shares? Main cable
            else if (sh != ES_ && port.isNull(__sSharedPortId)) shared[port.getId()] = tStringPair(six, portShare(sh)) ;
            if (!six.isEmpty()) six += _sSp;
            s = _sADD_PORT + six + name(port) + note(port);
            if (!port.isNull(_sPortTag)) s += " TAG " + str(port[_sPortTag], false);
            b += line (s + _sSemicolon);
            b += oParam(pp->params);
        }
        // Not connected list
        if (!nc.isEmpty()) b += line(_sPORT + nc.join(_sCommaSp) + " NC;");
        // Shares
        if (!shared.isEmpty()) {
            // Short ports by share type
            std::sort(pl.begin(), pl.end(),
                      [](cNPort *pa, cNPort *pb) { return pa->getId(_sSharedCable) < pb->getId(_sSharedCable); }
                     );
            while (!pl.isEmpty() && pl.first()->getId(_sSharedCable) == ES_) pl.pop_front();    // drop unshared ports from list
            foreach (qlonglong id, shared.keys()) {
                QStringList ports;
                QStringList shs;
                ports << shared[id].first;
                shs   << shared[id].second;
                foreach (cNPort *pp, pl) {
                    if (id == pp->getId(_sSharedPortId)) {
                        ports << pp->getName(_sPortIndex);
                        shs   << pp->getName(_sSharedCable);
                    }
                }
                s.clear();
                if (shs.size() < 2 || shs.size() > 4) s = "// Invalid share : ";
                s += "PORTS " + shs.join(_sCommaSp) + " SHARED " + ports.join(_sCommaSp) + _sSemicolon;
                b += line(s);
            }
        }
    r = lineEndBlock(r, b);
    return r;
}

/* ---------------------------------------------------------------------------------------- */

QString cExport::Nodes(eEx __ex, bool only)
{
    cNode o;
    QSqlQuery q = getQuery();
    QSqlQuery q2 = getQuery();
    QString r;
    if (o.fetch(q, only, QBitArray(1, false), o.iTab(_sNodeName))) {
        do {
            r += _export(q2, o, only);
        } while (o.next(q));
    }
    else {
        if (__ex >= EX_NOOP) EXCEPTION(EDATA, 0, sNoAnyObj);
    }
    return r;

}

static inline QString _address(cIpAddress *a)
{
    return a->address().toString() + "/" + a->getName(_sIpAddressType);
}

QString cExport::oAddress(tOwnRecords<cIpAddress, cInterface>& as, int first)
{
    QString r;
    QList<cIpAddress *>& list = as.list();
    foreach (cIpAddress *a, list.mid(first)) {
        r += line("ADD ADDRESS " + _address(a) + _sSemicolon);
    }
    return r;
}

QString cExport::nodeCommon(QSqlQuery& q, cNode &o)
{
    QString b;
    b += paramLine(q, _sPLACE,    o[_sPlaceId], UNKNOWN_PLACE_ID);
    b += paramLine(q, _sINENTORY, o[_sInventoryNumber]);
    b += paramLine(q, _sSERIAL,   o[_sSerialNumber]);
    if (!o.isNull(_sOsName) || !o.isNull(_sOsVersion)) {
        QString s = "OS " + str_z(o[_sOsName], false);
        if (!o.isNull(_sOsVersion)) s += " VERSION " + str(o[_sOsVersion], false);
        b += line(s + _sSemicolon);
    }
    b += oParam(o.params);
    return  b;
}

QString cExport::_export(QSqlQuery& q, cNode& o, bool only)
{
    static const QString sLastPort = _sPORT + "#@";
    if (!only) {
        qlonglong table_oid = o.fetchTableOId(q);
        if (o.tableoid() != table_oid) {
            cSnmpDevice so;
            if (so.tableoid() == table_oid) {
                so.setId(o.getId());
                if (!so.completion(q)) EXCEPTION(EDATA, o.getId(), o.identifying());
                return _export(q, so);
            }
            else {
                EXCEPTION(EDATA, table_oid, o.identifying());
            }
        }
    }

    o.fetchPorts(q);
    o.fetchParams(q);
    QString r, b, bb, s;

    qlonglong type = o.getId(_sNodeType);
    bool isHost = type & ENUM2SET(NT_HOST) && !o.ports.isEmpty();
    if (o.ports.size() == 1) {  // Only one port
        cNPort *pp = o.ports.first();
        if (isHost) {
            if (type & ENUM2SET(NT_WORKSTATION)
             && pp->ifType().getName() == _sEthernet    // type is ethernet
             && pp->getMac(_sHwAddress).isValid()       // valid MAC
             && pp->reconvert<cInterface>()->addresses.size() == 1) {   // Only one IP address
                // WORKSTATION ...
                s = "WORKSTATION" + name(o);
                cIpAddress *a = pp->reconvert<cInterface>()->addresses.first();
                if (a->address().isNull()) s += " DYNAMIC";
                else                       s += _sSp + _address(a);
                s += _sSp + pp->getMac(_sHwAddress).toString();
                r = lineBeginBlock(s + note(o));
                    if (type != ENUM2SET2(NT_HOST, NT_WORKSTATION)) {
                        type = type & ~ENUM2SET2(NT_HOST, NT_WORKSTATION);
                        QStringList ant = o.descr()[_sNodeType].enumType().set2lst(type);
                        if (ant.isEmpty()) {
                            b += line (tr("// Warning : Invalid node type : %1").arg(o.getName(_sNodeType)));
                        }
                        else {
                            b += line("TYPE + " + escaped(ant));
                        }
                    }
                    b += nodeCommon(q, o);
                    ++actIndent;
                        bb += paramLine(q, _sNAME, (*pp)[_sPortName], _sEthernet);
                        if (pp->getId(_sPortIndex) != 1) bb += line("INDEX " + int_z((*pp)[_sPortIndex]) + _sSemicolon);
                        bb += paramLine(q, "TAG", (*pp)[_sPortTag]);
                        bb += oParam(pp->params);
                    --actIndent;
                    b += block(sLastPort, bb);
                r = lineEndBlock(r, b);
                return r;
            }
        }
        else {
            if (pp->ifType().getName() == _sAttach) {   // Only one attach type port : ATTACH
                // ATTACH ...
                r = lineBeginBlock("ATTACHED" + name(o) + note(o));
                    b += nodeCommon(q, o);
                    ++actIndent;
                        bb += paramLine(q, _sNAME,  (*pp)[_sPortName], _sAttach);
                        bb += paramLine(q, "INDEX ",(*pp)[_sPortIndex], 1LL);
                        bb += paramLine(q, "TAG",   (*pp)[_sPortTag]);
                        bb += oParam(pp->params);
                    --actIndent;
                    b += block(sLastPort, bb);
                r = lineEndBlock(r, b);
                return r;
            }
        }
    }

    s = "NODE ";
    if (0 != (type & ~ENUM2SET(NT_NODE))) s += "(" + escaped(o.getStringList(_sNodeType)) + ") ";
    s += name(o) + _sSp;
    tRecordList<cNPort>::const_iterator pi;
    QHostAddress a = o.getIpAddress(&pi);
    QStringList wMsgs;
    QList<cNPort *> pl = o.ports.list();
    if (isHost) {
        if (!a.isNull()) {
            int plix = pi - o.ports.constBegin();
            if (plix != 0) pl.swapItemsAt(0, plix);   // set first port
            tOwnRecords<cIpAddress, cInterface>& addresses = pl.first()->reconvert<cInterface>()->addresses;
            if (addresses.isEmpty()) EXCEPTION(EPROGFAIL);  // Imposible
            if (a != addresses.first()->address()) {    // Find address
                int i, n = addresses.size();
                for (i = 1; i < n; ++i) {
                    if (addresses.at(i)->address() == a) {
                        addresses.swapItemsAt(0, i);           // Set first address in first port
                        break;
                    }
                }
                if (i >= n) EXCEPTION(EPROGFAIL);   // Address is not found. Imposible
            }
            s += a.toString() + "/" + addresses.first()->getName(_sIpAddressType) + _sSp;
        }
        else {
            wMsgs << tr("// Warning :  Missing IP address.");
            // The first port must be an interface
            if (pl.first()->chkObjType<cInterface>(EX_IGNORE) < 0) {   // First port object type is not interface
                for (pi = pl.begin() + 1; pi < pl.end(); ++pi) {
                    if ((*pi)->chkObjType<cInterface>(EX_IGNORE) == 0) {
                        pl.swapItemsAt(0, pi - pl.begin());
                        break;
                    }
                }
                if (pi >= pl.end()) {  // There is no interface type port
                    wMsgs << tr("It's a host, but there's no interface type port.");
                    isHost = false;
                }
            }
            if (isHost) s += "DYNAMIC ";
        }
        if (isHost) {
            cMac mac = pl.first()->getMac(_sHwAddress);
            if (mac.isValid()) s += mac.toString() + _sSp;
            else               s += "NULL ";
        }
    }
    s += note(o);
    r  = lineBeginBlock(s);
        foreach (QString wmsg, wMsgs) {
            b += line(wmsg);
        }
        cNPort *pp;
        cInterface *pif;
        if (isHost) {
            // First port (in header)
            pp = pl.first();
            pif = pp->reconvert<cInterface>();
            // If the first port is different from the default
            actIndent++;
                bb  = paramLine(q, "IFTYPE", (*pp)[pp->ixIfTypeId()], cIfType::ifTypeId(_sEthernet));
                bb += paramLine(q, _sNAME,   (*pp)[pp->nameIndex()], _sEthernet);
                bb += paramLine(q, "INDEX",  (*pp)[_sPortIndex], 1LL);
                bb += paramLine(q, "TAG",    (*pp)[_sPortTag]);
                bb += oParam(pp->params);             // parameters, if any
                bb += oAddress(pif->addresses, 1);    // more ip addresses, if any
            actIndent--;
            b += block(sLastPort, bb);
            pl.pop_front(); // Drop first port (exported all data)
        }
        b += nodeCommon(q, o);
        // Ports (Sort by index or name if index is NULL.
        std::sort(pl.begin(), pl.end(),
                  [](cNPort *pa, cNPort *pb)
                    {
                        if (pa->isNull(pa->ixPortIndex())) {
                            if (!pb->isNull(pb->ixPortIndex())) return false;
                            return pa->getName() < pb->getName();
                        }
                        return pa->getId(pa->ixPortIndex()) < pb->getId(pb->ixPortIndex());
                    }
                  );
        foreach (pp, pl) {
            bool isInterface = 0 == pp->chkObjType<cInterface>(EX_IGNORE);
            pif = isInterface ? pp->reconvert<cInterface>() : nullptr;
            QString six = pp->getName(_sPortIndex);
            if (!six.isEmpty()) six += _sSp;
            // Port header
            s = _sADD_PORT + six + escaped(pp->ifType().getName()) + name(*pp) + _sSp;
            if (isInterface) {
                if (pif->addresses.isEmpty()) s += _sNULL;
                else {
                    s += _address(pif->addresses.first());
                    pif->addresses.pop_front();
                }
                cMac mac = pif->getMac(_sHwAddress);
                s += _sSp + (mac.isValid() ? mac.toString() : _sNULL);
            }
            s += _sSp + note(*pp);
            b += lineBeginBlock(s);
                bb.clear();
                if (!pp->isNull(_sPortTag)) bb += " TAG " + str((*pp)[_sPortTag], false) + _sSemicolon;
                bb += oParam(pp->params);
                if (isInterface) bb += oAddress(pif->addresses);
            b  = lineEndBlock(b, bb);
        }
    r = lineEndBlock(r, b);
    return r;
}

/// Not supported
QString cExport::SnmpDevices(eEx __ex, bool only)
{
    if (only) {
        cSnmpDevice o;
        return sympleExport(o, o.toIndex(_sNodeName), __ex);
    }
    else {
        return Nodes(__ex, false);
    }
}

QString cExport::_export(QSqlQuery& q, cSnmpDevice& o)
{
    o.fetchPorts(q);
    o.fetchParams(q);
    QString r  = _export(q, static_cast<cNode&>(o));
    QString s;
    actIndent++;
    s  = paramLine(q, "READ COMMUNITY", o[_sCommunityRd]);
    s += paramLine(q, "WRITE COMMUNITY", o[_sCommunityWr]);
    s += fieldSetLine(q, o, _sSysDescr);
    s += fieldSetLine(q, o, _sSysObjectId);
    s += fieldSetLine(q, o, _sSysUpTime);
    s += fieldSetLine(q, o, _sSysContact);
    s += fieldSetLine(q, o, _sSysName);
    s += fieldSetLine(q, o, _sSysLocation);
    s += fieldSetLine(q, o, _sSysServices);
    if (s.isEmpty()) {
        actIndent--;
    }
    else {
        r.chop(1);  // drop last newline
        if (r.endsWith(QChar(';'))) {
            r.chop(1);
            r = lineBeginBlock(r);
            r = lineEndBlock(r, s);
        }
        else {
            int lix = r.lastIndexOf(QChar('\n'), -1);
            r = r.mid(0, lix + 1);  // drop last line
            r += s;
            r += lineEndBlock();
        }
    }
    return r;
}
