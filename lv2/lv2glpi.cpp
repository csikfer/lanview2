#include "lv2glpi.h"
#include "lv2data.h"

QString cRegExpConverterItem::compareAndConvert(const QString& s)
{
    QString r;
    QString c = getName(ixChoice());
    if (pattern.exactMatch(s)) {
        QString::iterator i = c.begin();
        QString::iterator e = c.end();
        for (; i < e; ++i) {
            if (*i == QChar('$')) {
                ++i;
                if (i >= e || *i == QChar('$')) r += *i;
                else {
                    int ix = i->toLatin1() - '0';
                    if (ix > 0 && ix < 10) r += pattern.cap(ix);
                }
            }
            else {
                r += *i;
            }
        }
    }
    return r;
}

cRegExpConverter::cRegExpConverter(const QString& key) : QList<cRegExpConverterItem *>()
{
    static const QString sql = "SELECT * FROM selects WHERE select_type = ? ORDER BY precedence ASC";
    QSqlQuery q = getQuery();
    if (execSql(q, sql, key)) {
        do {
            (*this) << new cRegExpConverterItem(q);
        } while (q.next());
    }
    else {
        EXCEPTION(EENODATA, 0, key);
    }
}

QString cRegExpConverter::compareAndConvert(const QString& s)
{
    QString r;
    foreach (cRegExpConverterItem * c, *this) {
        r = c->compareAndConvert(s);
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

static const QString _sPlaceFromGlpi = "place.from.glpi";
static const QString _sPlaceToGlpi   = "place.to.glpi";
// const QString cGlpiLocations::sLevelSep = " > ";

CMYRECCNTR(cGlpiLocations)
CRECDEFNCD(cGlpiLocations)

cRegExpConverter * cGlpiLocations::pConvertFromGlpi = nullptr;
cRegExpConverter * cGlpiLocations::pConvertToGlpi   = nullptr;

const cRecStaticDescr&  cGlpiLocations::descr() const {
    if (initPMyDescr<cGlpiLocations>(_sGlpiLocations)) {
        pConvertFromGlpi = new cRegExpConverter(_sPlaceFromGlpi);
        pConvertToGlpi   = new cRegExpConverter(_sPlaceToGlpi);
    }
    return *_pRecordDescr;
}

QString cGlpiLocations::nameToGlpi(const QString& __n)
{
    if (pConvertToGlpi == nullptr) EXCEPTION(EPROGFAIL);
    return pConvertToGlpi->compareAndConvert(__n);
}

QString cGlpiLocations::nameFromGlpi(const QString& __n)
{
    if (pConvertFromGlpi == nullptr) EXCEPTION(EPROGFAIL);
    return pConvertFromGlpi->compareAndConvert(__n);
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

