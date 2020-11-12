#include "lv2tooldata.h"


/* -------------------------------------------- tools ---------------------------------------- */
const QString& toolType(int e, eEx __ex)
{
    switch (e) {
    case TT_COMMAND:   return _sCommand;
    case TT_URL:       return _sUrl;
    case TT_PARSE:     return _sParse;
    }
    if (__ex) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

int toolType(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sCommand, Qt::CaseInsensitive)) return TT_COMMAND;
    if (0 == n.compare(_sUrl,     Qt::CaseInsensitive)) return TT_URL;
    if (0 == n.compare(_sParse,   Qt::CaseInsensitive)) return TT_PARSE;
    if (__ex) EXCEPTION(EENUMVAL, -1, n);
    return ENUM_INVALID;
}

CRECCNTR(cTool)

const cRecStaticDescr&  cTool::descr() const
{
    if (initPDescr<cTool>(_sTools)) {
        _ixFeatures = _descr_cTool().toIndex(_sFeatures);
        CHKENUM(_sToolType, toolType);
    }
    return *_pRecordDescr;
}

int cTool::_ixFeatures = NULL_IX;

CRECDEFD(cTool)

/* -------------------------------------------- tool_objects ---------------------------------------- */

CRECCNTR(cToolObject)

const cRecStaticDescr&  cToolObject::descr() const
{
    if (initPDescr<cToolObject>(_sToolObjects)) {
        _ixFeatures = _descr_cToolObject().toIndex(_sFeatures);
        CHKENUM(_sToolType, toolType);
    }
    return *_pRecordDescr;
}

int cToolObject::_ixFeatures = NULL_IX;

CRECDEFD(cToolObject)

QString cToolObject::substituted()
{
    QString c = tool.getName(_sToolStmt);
    if (!c.isEmpty()) {
        // Feature :
        _mergedFeatures = tool.features();
        _mergedFeatures.merge(features());
        int ofix = object.toIndex(_sFeatures, EX_IGNORE);
        if (ofix > 0) {
            cFeatures f;
            f.split(object.getName(ofix));
            _mergedFeatures.merge(f, mCat(_sTools, object.getName()));
        }
        // Command or URL :
        QSqlQuery dumy;
        c = substitute(dumy, this, c, [] (const QString& key, QSqlQuery&, void * _p) {
                    cToolObject *p = static_cast<cToolObject *>(_p);
                    return p->_mergedFeatures.value(key);
                });
    }
    return c;
}
