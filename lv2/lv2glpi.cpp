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

CMYRECCNTR(cGlpiEntity)
CRECDEFNCD(cGlpiEntity)

const cRecStaticDescr&  cGlpiEntity::descr() const {
    if (initPMyDescr<cGlpiEntity>(_sGlpiEntities)) {

    }
    return *_pRecordDescr;
}

/* ************************** locations ************************** */

static const QString _sPlaceFromGlpi = "place.from.glpi";
static const QString _sPlaceToGlpi   = "place.to.glpi";

CMYRECCNTR(cGlpiLocation)
CRECDEFNCD(cGlpiLocation)

const QString cGlpiLocation::sLevelSep = " > ";
cRegExpConverter * cGlpiLocation::pConvertFromGlpi = nullptr;
cRegExpConverter * cGlpiLocation::pConvertToGlpi   = nullptr;

const cRecStaticDescr&  cGlpiLocation::descr() const {
    if (initPMyDescr<cGlpiLocation>(_sGlpiLocations)) {
        pConvertFromGlpi = new cRegExpConverter(_sPlaceFromGlpi);
        pConvertToGlpi   = new cRegExpConverter(_sPlaceToGlpi);
        QSqlQuery q = getQuery();
        static const QString spname = "glpi_level_separator";
        QString s = cSysParam::getTextSysParam(q, spname);
        if (!s.isEmpty()) {
            if (s != sLevelSep) {
                const_cast<QString&>(sLevelSep) = s;
            }
        }
        else {
            cSysParam::setTextSysParam(q, spname, sLevelSep);
        }
    }
    return *_pRecordDescr;
}

QString cGlpiLocation::nameToGlpi(const QString& __n)
{
    if (pConvertToGlpi == nullptr) EXCEPTION(EPROGFAIL);
    return pConvertToGlpi->compareAndConvert(__n);
}

QString cGlpiLocation::nameFromGlpi(const QString& __n)
{
    if (pConvertFromGlpi == nullptr) EXCEPTION(EPROGFAIL);
    return pConvertFromGlpi->compareAndConvert(__n);
}


/* ************************** locations tree ************************** */

cLocationsTreeItemData::cLocationsTreeItemData(const cMyRec& _glpiRecord)
{
    result = SR_UNSET;
    pGlpiRecord = static_cast<cMyRec *>(_glpiRecord.dup());
    pLv2Record  = nullptr;
}

cLocationsTreeItemData::cLocationsTreeItemData(const cPlace& _lv2Record)
{
    result = SR_UNSET;
    pGlpiRecord = nullptr;
    pLv2Record  = static_cast<cPlace *>(_lv2Record.dup());
}

cLocationsTreeItemData::cLocationsTreeItemData(cPlace * _pLv2Record)
{
    result = SR_UNSET;
    pGlpiRecord = nullptr;
    pLv2Record  = _pLv2Record;
}


cLocationsTreeItemData::~cLocationsTreeItemData()
{
    pDelete(pGlpiRecord);
    pDelete(pLv2Record);
}

/*
cLocationsTreeItemData *cLocationsTreeItemData::dup() const
{
    cLocationsTreeItemData * r = new cLocationsTreeItemData(*pGlpiRecord);
    r->pLv2Record  = static_cast<cPlace *>(pLv2Record->dup());
    return r;
}
*/

QString cLocationsTreeItemData::toString()
{
    QString r;
    r += QString("Item [%1] :").arg((qlonglong)this, 0, 16);
    r += QString(" Location %1 ('%2', '%3')")
            .arg(pGlpiRecord == nullptr ? _sNULL : pGlpiRecord->getName(_sCompletename))
            .arg(placeName, completename);
    r += QString(" Place : %1 (%2)")
            .arg(pLv2Record  == nullptr ? _sNULL : pLv2Record->getName())
            .arg(placeName);
    return r;
}

/* */

QString cGlpiLocationsTreeItem::toString(bool tree, int indent)
{
    QString r;
    if (indent > 0) {
        for (int i = 0; i < indent; ++i) r += " >";
        r += " ";
    }
    r += pData->toString();
    if (tree) {
        r += "\n";
        foreach (tTreeItem * p, _childList) {
            r += static_cast<cGlpiLocationsTreeItem *>(p)->toString(tree, indent +1);
        }
    }
    return r;
}

bool cGlpiLocationsTreeItem::fetchGlpiTree(QString& emsg)
{
    QSqlQuery q = cMariaDb::getQuery();        // GLPI database (and init if success)
    // Stack
    class cStack : public QStack<cGlpiLocationsTreeItem *> {
    public:
        cGlpiLocationsTreeItem *pRootItem;
        cStack(cGlpiLocationsTreeItem *p) : QStack<cGlpiLocationsTreeItem *>() {
            pRootItem = p;
        }
        bool checkLevel(cGlpiLocation * pLocation, QString& emsg)
        {
            int parentLevel = isEmpty() ? 0 : int(top()->pData->pGlpiRecord->getId(_sLevel));
            if (parentLevel != size()) {
                QString e = QObject::tr("Level and stack size, inconsistent : parentLevel =  %1  stack size = %2; Location : %3").arg(parentLevel).arg(size()).arg(pLocation->getName(_sCompletename));
                DERR() << e << endl;
                emsg += e + _sNl;
                return false;
            }
            int level = parentLevel +1;
            int currentLevel = int(pLocation->getId(_sLevel));
            QString completename = pLocation->getName(_sCompletename);
            QString locationName = pLocation->getName();
            QString cname = locationName;  // calculated completename
            cGlpiLocationsTreeItem *pActItem = nullptr;
            PDEB(VERBOSE) << toString() << endl;
            if (level >= currentLevel) {
                while (level > currentLevel) {
                    PDEB(VERBOSE) << "POP : " << top()->pData->pGlpiRecord->getName() << endl;
                    pop();
                    level--;
                }
                pActItem = actItem();
                PDEB(VERBOSE) << pActItem->pData->pGlpiRecord->getName() <<  " << " << pLocation->getName() << " (" << pLocation->getName(_sCompletename) << ")" << endl;
                if (!isEmpty()) cname = locationName.prepend(top()->pData->pGlpiRecord->getName(_sCompletename) + cGlpiLocation::sLevelSep);
                if (cname == completename) {
                    cGlpiLocationsTreeItem& newItem = static_cast<cGlpiLocationsTreeItem&>(*pActItem << new cLocationsTreeItemData(*pLocation));
                    newItem.pData->completename = completename;
                    newItem.pData->locationName = locationName;
                    return true;
                }
                else {
                    QString e = QObject::tr("completename : %1 <> %2").arg(quoted(completename), quoted(cname));
                    PDEB(WARNING) << e << endl;
                    emsg += e + _sCommaSp + toString() + _sNl;
                    return false;
                }
            }
            else { //
                pActItem = actItem();
                if (0 == pActItem->childNumber()) {
                    QString e = QObject::tr("Parent is dropped; completename : %1").arg(quoted(completename));
                    PDEB(WARNING) << e << endl;
                    emsg += e + _sCommaSp + toString() + _sNl;
                    return false;
                }
                pActItem = static_cast<cGlpiLocationsTreeItem *>(pActItem->lastChild());
                PDEB(VERBOSE) << "PUSH : " << pActItem->pData->pGlpiRecord->getName() << endl;
                PDEB(VERBOSE) << pActItem->pData->pGlpiRecord->getName() <<  " << " << pLocation->getName() << " (" << pLocation->getName(_sCompletename) << ")" << endl;
                cname = locationName.prepend(pActItem->pData->pGlpiRecord->getName(_sCompletename) + cGlpiLocation::sLevelSep);
                if (cname == completename) {
                    push(pActItem);
                    cGlpiLocationsTreeItem& newItem = static_cast<cGlpiLocationsTreeItem&>(*pActItem << new cLocationsTreeItemData(*pLocation));
                    newItem.pData->completename = completename;
                    newItem.pData->locationName = locationName;
                    return true;
                }
                else {
                    QString e = QObject::tr("completename : %1 <> %2").arg(quoted(completename), quoted(cname));
                    PDEB(WARNING) << e << endl;
                    emsg += e + _sCommaSp + toString() + _sNl;
                    return false;
                }
            }
        }
    private:
        cGlpiLocationsTreeItem *actItem() {
            if (isEmpty()) {
                return pRootItem;
            }
            else {
                return top();
            }
        }
        QString toString() {
            QString r = "STACK:[";
            if (isEmpty()) {
                r += "empty";
            }
            else {
                for (reverse_iterator i = rbegin(); i != rend(); i++) {
                    r += quoted((*i)->pData->pGlpiRecord->getName()) + ", ";
                }
                r.chop(2);
            }
            return r + "]";
        }
    };
    cStack stack(this);
    // read tree
    cGlpiLocation  location;
    location.setId(_sEntitiesId, pData->pGlpiRecord->getId());  // root item, data: entities record
    if (location.fetch(q, false, location.mask(_sEntitiesId), location.iTab(_sCompletename))) {
        do {
            emsg += QString("Fetched glpi_location : '%1' / '%2'.\n").arg(location.getName(_sCompletename), location.getName());
            stack.checkLevel(&location, emsg);
        } while (location.next(q));
    }
    return  true;
}

bool cGlpiLocationsTreeItem::mergeLv2Tree(QString& emsg)
{
    if (pData == nullptr || pData->pGlpiRecord == nullptr) {
        EXCEPTION(EPROGFAIL);
    }
    QSqlQuery q = getQuery();
    cPlace *pPlace = new cPlace;
    pPlace->_toReadBack = RB_YES;   // Read back updated record
    if (pData->pGlpiRecord->tableName() == _sGlpiEntities) {    // root item
        pPlace->mark(q);   // marked (flag = true) all places record
        pPlace->setId(UNKNOWN_PLACE_ID);
        pPlace->mark(q, QBitArray(), false);
        pPlace->setId(ROOT_PLACE_ID);
        if (0 == pPlace->update(q, false, pPlace->mask(_sFlag), QBitArray(), EX_ERROR)) { // Set flag to false, and read root record (by ID)
            emsg += QObject::tr("Not found root place record in LanView2 database\n");
            delete pPlace;
            return false;    // fatal
        }
    }
    else {                                                      // child item
        pData->placeName = pData->pGlpiRecord->reconvertMy<cGlpiLocation>()->nameFromGlpi();
        if (pData->placeName.isEmpty()) {
            emsg += QObject::tr("The GLPI locations.completename = '%1', not converted to LanView2 place name.\n").arg(pData->pGlpiRecord->getName(_sCompletename));
            delete pPlace;
            return true;    // non fatal
        }
        pPlace->setName(pData->placeName);  // WHERE ..
        pPlace->setBool(_sFlag, false);     // SET ..
        if (0 == pPlace->update(q, false, pPlace->mask(_sFlag), pPlace->mask(pPlace->nameIndex()), EX_ERROR)) { // Set flag to false, and read record
            emsg += QObject::tr("The GLPI locations.completename = '%1',  converted to LanView2 place name = '%2'. Place is not found in LanView2 database.\n").arg(pData->pGlpiRecord->getName(_sCompletename), pData->placeName);
            delete pPlace;
            return true;
        }
    }
    pData->pLv2Record = pPlace;
    pData->placeName  = pPlace->getName();
    if (pData->completename.isEmpty() && pData->pGlpiRecord->tableName() != _sGlpiEntities) {
        EXCEPTION(ECONTEXT);
    }
    foreach (tTreeItem *p, _childList) {
        if (!static_cast<cGlpiLocationsTreeItem *>(p)->mergeLv2Tree(emsg)) {
            return false;
        }
    }
    return addLv2Tree(emsg);
}

bool cGlpiLocationsTreeItem::addLv2Tree(QString &emsg)
{
    if (pData == nullptr && pData->pLv2Record == nullptr) {
        EXCEPTION(EPROGFAIL);
    }
    PDEB(VERBOSE) << toString() << endl;
    QSqlQuery q = getQuery();
    qlonglong parentPlaceId = NULL_ID;
    QString parentPlaceName;
    if (pData->pLv2Record != nullptr) {
        parentPlaceId = pData->pLv2Record->getId();
        parentPlaceName = pData->pLv2Record->getName();
    }
    if (pData->pLv2Record == nullptr || parentPlaceId == NULL_ID || parentPlaceName.isEmpty()) {
        EXCEPTION(EDATA, 0, QObject::tr("Place is NULL, or empty/invalid.") + toString());
    }
    if (pData->pGlpiRecord == nullptr) {
        cPlace place;
        place.setId(_sParentId, parentPlaceId);
        place.setBool(_sFlag, false);
        if (place.completion(q)) {
            emsg += QObject::tr("Database error: If there is no GLPI parent, there can be no processed record among the children, parent_id = %1, first invalid record name : %2\n").arg(parentPlaceId).arg(place.getName());
            return false;
        }
    }
    static const QString sql =
            "UPDATE places"
            " SET flag = false"
            " WHERE flag AND parent_id = ?"
            " RETURNING *";
    if (execSql(q, sql, parentPlaceId)) {
        do {
            cPlace *pPlace = new cPlace();
            pPlace->set(q);
            QString placeName = pPlace->getName();
            QString completename = cGlpiLocation::nameToGlpi(placeName);
            if (completename.isEmpty()) {
                emsg += QObject::tr("The places place_name = %1, not converted to GLPI locations completename name.\n").arg(placeName);
                delete pPlace;
                continue;    // non fatal, DROP RECORD
            }
            QString parentCompletename = pData->completename;
            QString locationName;
            if (parentCompletename.isEmpty()) {
                locationName = completename;
            }
            else if (completename.startsWith(parentCompletename)) {
                locationName = completename.mid(parentCompletename.size());
                if (locationName.startsWith(cGlpiLocation::sLevelSep)) {
                    locationName = locationName.mid(cGlpiLocation::sLevelSep.size());
                }
                else {
                    locationName.clear();   // ERROR
                }
            }
            if (locationName.isEmpty()) {
                emsg += QObject::tr("The GLPI locations completename name is invalid : parent completename = '%1', completename = '%2' (separator = '%3'), place_name source = %4, parent = %5\n").arg(parentCompletename, completename, cGlpiLocation::sLevelSep, placeName, parentPlaceName);
//              emsg += "Tree : \n" + static_cast<cGlpiLocationsTreeItem *>(root())->toString(true, 0);
                delete pPlace;
                continue;    // non fatal, DROP RECORD
            }
            PDEB(VERBOSE) << "Create item by place " << VDEB(placeName) << VDEB(completename) << endl;
            cLocationsTreeItemData *pd = new cLocationsTreeItemData(pPlace);
            pd->placeName = placeName;
            pd->completename = completename;
            pd->locationName = locationName;
            cGlpiLocationsTreeItem& newItem = static_cast<cGlpiLocationsTreeItem&>(*this << pd);
            PDEB(VERBOSE) << "NEW " << newItem.toString() << endl;
            if (!newItem.addLv2Tree(emsg)) {
                return false;
            }
        } while (q.next());
    }
    return true;
}

cGlpiLocationsTreeItem *cGlpiLocationsTreeItem::fetchLocationAndPlaceTree(QString& emsg)
{
    QSqlQuery q = getQuery();   // LanView2 database
    QString entityName = cSysParam::getTextSysParam(q, "glpi_entity");  // GLPI Entity name get from glpi_entity system parameter.
    if (entityName.isEmpty()) {                     // glpi_entity is not set, unknown entity
        emsg += QObject::tr("GLPI entity name not set.\n");
        return nullptr;
    }

    q = cMariaDb::getQuery();                          // GLPI database (and init if success)
    cGlpiEntity  entity;
    // set root item (entity)
    int n = entity.setName(_sName, entityName).completion(q);
    if (n != 1) {
        emsg += QObject::tr("Entity '%1' is not found (or unclear).\n").arg(entityName);
        return nullptr;
    }
    cGlpiLocationsTreeItem * pRootItem = new cGlpiLocationsTreeItem(entity);
    // fetch GLPI locations to tree
    if (!pRootItem->fetchGlpiTree(emsg)) {
        delete pRootItem;
        return nullptr;
    }
    // Merge LanView2 places to tree
    if (!pRootItem->mergeLv2Tree(emsg)) {
        delete pRootItem;
        return nullptr;
    }
    return pRootItem;
}

void cGlpiLocationsTreeItem::prepare(QString& emsg, eTristate _updateGlpi, eTristate _updateLv2)
{
    foreach (tTreeItem * pti, _childList) {
        cGlpiLocationsTreeItem *pItem = static_cast<cGlpiLocationsTreeItem *>(pti);
        cLocationsTreeItemData *p = pItem->pData;
        if      (_updateLv2 != TS_NULL && p->pLv2Record  == nullptr) {
            pItem->updateLv2Record(toBool(_updateLv2), emsg);
        }
        else if (_updateGlpi != TS_NULL && p->pGlpiRecord == nullptr) {
            pItem->updateGlpiRecord(toBool(_updateGlpi), emsg);
        }
        else if (p->pLv2Record != nullptr && p->pGlpiRecord != nullptr) {
            pItem->pData->result = SR_EQU;
            emsg += QObject::tr("Place '%1' == Location %2 : '%3'\n").arg(p->pLv2Record->getName(), p->pGlpiRecord->getName(_sName), p->pGlpiRecord->getName(_sCompletename));
        }
        pItem->prepare(emsg, _updateGlpi, _updateLv2);
    }
}

void cGlpiLocationsTreeItem::updateLv2Record(bool _update, QString& emsg)
{
    cPlace *pPlace = new cPlace;
    pData->pLv2Record = pPlace;
    QString placeName = pData->placeName;
    cPlace *pParent = parent()->pData->pLv2Record;
    if (pParent == nullptr) {
        emsg += QObject::tr("Data error: pParent (place parent object) is NULL. Place name = '%1', completename = '%2'\n")
                            .arg(pData->placeName, pData->completename);
        return;
    }
    if (placeName.isEmpty()) {
        emsg += QObject::tr("Data error: placeName is empty. parent : '%1' / '%2'; completename = '%3'\n")
                            .arg(pParent->getName(), parent()->pData->completename, pData->completename);
        return;
    }
    pPlace->setName(pData->placeName);
    QString note = QObject::tr("Synced from GLPI.");
    msgAppend(&note, pData->pGlpiRecord->getName("comment"));
    pPlace->setNote(note);
    pPlace->setId(_sParentId, pParent->getId());
    if (_update) {
        QSqlQuery q = getQuery();
        cError *pe = pPlace->tryInsert(q);
        if (pe != nullptr) {
            emsg += QObject::tr("Insert place %1, error : ").arg(placeName) + pe->msg();
            delete pe;
            pData->result = SR_ERROR_LV2;
        }
        else {
            emsg += QObject::tr("Insert place %1 (%2)\n").arg(placeName, pData->completename);
            pData->result = SR_SYNCED_LV2;
        }
    }
    else {
        emsg += QObject::tr("Skip insert place %1 (%2)\n").arg(placeName, pData->completename);
        pData->result = SR_SKIP_LV2;
    }
}

void cGlpiLocationsTreeItem::updateGlpiRecord(bool _update, QString& emsg)
{
    cGlpiLocation *pLocation = new cGlpiLocation;
    pLocation->_toReadBack = RB_ID; // Need the ID after the insert
    pData->pGlpiRecord = pLocation;
    pLocation->setName(_sName,         pData->locationName);
    pLocation->setName(_sCompletename, pData->completename);
    pLocation->setId(_sEntitiesId, root()->pData->pGlpiRecord->getId());
    qlonglong locationId;   // parent
    int level;
    if (parent()->pData->pGlpiRecord->tableName() == _sGlpiEntities) {
        locationId = 0;
        level = 1;
    }
    else {
        locationId = parent()->pData->pGlpiRecord->getId();
        if (locationId == 0) EXCEPTION(EDATA);
        level = int(parent()->pData->pGlpiRecord->getId(_sLevel)) + 1;
    }
    pLocation->setId(__sLocationsId, locationId);
    pLocation->setId(_sLevel, level);
    pLocation->setId("is_recursive", 0);
    QString note = QObject::tr("Synced from LanView2.");
    msgAppend(&note, pData->pLv2Record->getNote());
    pLocation->setName("comment", note);
    QVariant now = QDateTime::currentDateTime();
    pLocation->set("date_mod", now);
    pLocation->set("date_creation", now);
    if (_update) {
        QSqlQuery q = cMariaDb::getQuery();
        cError *pe = pLocation->tryInsert(q);
        if (pe != nullptr) {
            emsg += QObject::tr("Insert location %1, error : ").arg(pData->completename) + pe->msg();
            delete pe;
            pData->result = SR_ERROR_GLPI;
        }
        else {
            emsg += QObject::tr("Insert location %1 (%2)\n").arg(pData->completename, pData->placeName);
            pData->result = SR_SYNCED_GLPI;
        }
    }
    else {
        pData->result = SR_SKIP_GLPI;
        emsg += QObject::tr("Skip insert location %1 (%2)\n").arg(pData->completename, pData->placeName);
    }
}

