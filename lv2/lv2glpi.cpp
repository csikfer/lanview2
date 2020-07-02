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
    if (initPMyDescr<cGlpiEntities>("glpi_entities")) {

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

cRegExpConverter cGlpiLocations::convertFromGlpi;
cRegExpConverter cGlpiLocations::convertToGlpi;

CMYRECCNTR(cGlpiLocations)
CRECDEFNCD(cGlpiLocations)

const cRecStaticDescr&  cGlpiLocations::descr() const {
    if (initPMyDescr<cGlpiLocations>("glpi_locations")) {
        convertFromGlpi = cRegExpConverter(patternFromGlpi);
        convertToGlpi   = cRegExpConverter(patternToGlpi);
    }
    return *_pRecordDescr;
}

QString cGlpiLocations::nameToGlpi(const QString& __n)
{

}

QString cGlpiLocations::nameFromGlpi(const QString& __n)
{

}


/* ************************** locations tree ************************** */
QString  cGlpiLocationsTreeItem::entityName;
cGlpiLocationsTreeItem   cGlpiLocationsTreeItem::rootItem;

bool cGlpiLocationsTreeItem::setEntity()
{
    rootItem.clear();
    QSqlQuery q = getQuery();   // LanView2 database
    entityName = cSysParam::getTextSysParam(q, "glpi_entity");
    if (entityName.isEmpty()) return false;
    q = cMariaDb::getQuery();   // GLPI database
    rootItem.pData = new cGlpiEntities;
    int n = rootItem.pData->setName("name", entityName).completion(q);
    if (n != 1) return false;
    // ...
    return  true;
}

