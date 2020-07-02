#include "lv2glpi.h"
#include "lv2data.h"

QString cRegExpConverterItem::compareAndConvert(const QString& s)
{
    QString r;
    if (first.exactMatch(s)) {
        QString::iterator i = second.begin();
        QString::iterator e = second.end();
        for (; i < e; ++i) {
            if (*i == QChar('$')) {
                ++i;
                if (i >= e || *i == QChar('$')) r += *i;
                else {
                    int ix = i->toLatin1() - '0';
                    if (ix > 0 && ix < 10) r += first.cap(ix);
                }
            }
            else {
                r += *i;
            }
        }
    }
    return r;
}

cRegExpConverter::cRegExpConverter(const QStringList& l) : QList<cRegExpConverterItem>()
{
    for (int i = 0; i + 1 < l.size(); i += 2) {
        (*this) << cRegExpConverterItem(l.at(i), l.at(i + 1));
    }
}

QString cRegExpConverter::compareAndConvert(const QString& s)
{
    QString r;
    foreach (cRegExpConverterItem c, *this) {
        r = c.compareAndConvert(s);
        if (!r.isEmpty()) break;
    }
    return r;
}

/* ************************** entityes ************************** */

CMYRECCNTR(cGlpiEntities)
CRECDEFNCD(cGlpiEntities)

const cRecStaticDescr&  cGlpiEntities::descr() const {
    if (initPMyDescr<cGlpiEntities>(_sGlpiEntities)) {

    }
    return *_pRecordDescr;
}

/* ************************** locations ************************** */

static const QStringList patternFromGlpi = {
    "(\\S+)\\s+épület",                                                         "$1",
    "(\\S+)\\s+épület\\s*>\\s*földszint",                                       "$1F",
    "(\\S+)\\s+épület\\s*>\\s*(\\d)\\.?\\s*emelet",                             "$1$2",
    "(\\S+)\\s+épület\\s*>\\s*földszint\\s*>\\s*(\\d{2})\\.\\s*szoba",          "$1F$2",
    "(\\S+)\\s+épület\\s*>\\s*(\\d)\\.?\\s*emelet\\s*>\\s*(\\d{2})\\.\\s*szoba","$1$2$3"
};

static const QStringList patternToGlpi = {
    "(\\w)",                                        "$1 épület",
    "(\\w)F",                                       "$1 épület > Földszint",
    "(\\w)(\\d)",                                   "$1 épület > $2. emelet",
    "(\\w)F(\\d{2})",                               "$1 épület > Földszint > $2. szoba",
    "(\\w)(\\d)(\\d{2})",                           "$1 épület > $2. emelet > $3. szoba",
};

const QString cGlpiLocations::sLevelSep = " > ";
cRegExpConverter cGlpiLocations::convertFromGlpi;
cRegExpConverter cGlpiLocations::convertToGlpi;

CMYRECCNTR(cGlpiLocations)
CRECDEFNCD(cGlpiLocations)

const cRecStaticDescr&  cGlpiLocations::descr() const {
    if (initPMyDescr<cGlpiLocations>(_sGlpiLocations)) {
        convertFromGlpi = cRegExpConverter(patternFromGlpi);
        convertToGlpi   = cRegExpConverter(patternToGlpi);
    }
    return *_pRecordDescr;
}

QString cGlpiLocations::nameToGlpi(const QString& __n)
{
    return convertToGlpi.compareAndConvert(__n);
}

QString cGlpiLocations::nameFromGlpi(const QString& __n)
{
    return convertFromGlpi.compareAndConvert(__n);
}


/* ************************** locations tree ************************** */

bool cGlpiLocationsTreeItem::setEntity()
{
    clear();
    QSqlQuery q = getQuery();   // LanView2 database
    entityName = cSysParam::getTextSysParam(q, "glpi_entity");
    if (entityName.isEmpty()) return false;
    q = cMariaDb::getQuery();   // GLPI database
    // set root item (entity)
    pData = new cGlpiEntities;
    int n = pData->setName("name", entityName).completion(q);
    if (n != 1) return false;
    // Stack
    class cStack : public QStack<cGlpiLocationsTreeItem *> {
    public:
        cGlpiLocationsTreeItem *par;
        cStack(cGlpiLocationsTreeItem *p) : QStack<cGlpiLocationsTreeItem *>() {
            par = p;
        }
        bool checkLevel(cGlpiLocations * pLocation)
        {
            int parentLevel = isEmpty() ? -1 : int(top()->pData->getId(_sLevel));
            int actLevel = parentLevel +1;
            int level = int(pLocation->getId(_sLevel));
            QString basename;
            if (!isEmpty()) basename = top()->pData->getName(_sCompletename);
            QString completename = pLocation->getName(_sCompletename);
            if (actLevel == level) {
                QString cname = basename + cGlpiLocations::sLevelSep + pLocation->getName(_sName);
                if (cname == completename) {
                    *(isEmpty() ? par : top()) << pLocation;
                    return true;
                }
                else {
                    return false;
                }
            }
            else if (actLevel > level) {

            }
            else { // actlevel < level

            }

        }
    };
    cStack stack(this);
    // read tree
    cGlpiLocations  location;
    location.setId(_sEntitiesId, entitiesId());
    if (location.fetch(q, false, location.mask(_sEntitiesId), location.iTab(_sCompletename))) {

    }

    return  true;
}

