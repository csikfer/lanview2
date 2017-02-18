#include "workstation.h"
#include "ui_wstform.h"
#include "ui_phslinkform.h"
#include "lv2widgets.h"
#include "lv2link.h"
#include "record_dialog.h"

cWorkstation::cWorkstation(QWidget *parent) :
    cOwnTab(parent),
    pUi(new Ui::wstWidget),
    pq(newQuery()),
    pSample(new cNode),
    pWorkstation(new cNode),
    pModelNode(new cRecordListModel(pWorkstation->descr())),
    pInterface(new cInterface),
    pIpAddress(new cIpAddress),
    pPlace(new cPlace),
    pPort2(NULL),                                   // Ez lehet cNPort vagy cInterface objektum is.
    pLinkNode(new cPatch),                          // Elvileg nem csak patch lehet, de ez az ős
    pLinkPort(new cRecordAny("patchable_ports")),   // patchelhető portok view
    linkType(LT_INVALID),
    linkShared(ES_NC),
    pLinkButtonsLinkType(new QButtonGroup(this)),
    pModelLinkNode(new cRecordListModel(pLinkNode->descr())),
    pModelLinkPort(new cRecordListModel(pLinkPort->descr())),
    pLinkNode2(new cPatch),
    pLinkPort2(new cRecordAny(&pLinkPort->descr())),
    linkType2(LT_INVALID),
    linkShared2(ES_NC),
    pLinkButtonsLinkType2(new QButtonGroup(this)),
    pModelLinkNode2(new cRecordListModel(pLinkNode2->descr())),
    pModelLinkPort2(new cRecordListModel(pLinkPort2->descr()))
{
    withinSlot = 0;
    lockSlot = true;
    memset(&states, 0, sizeof(states)); // set all members: IS_EMPTY, false
    states.portName = IS_OK;    // A form-ban van egy alapértelmezett név: "ethernet"
    states.subNetNeed = true;   // A form-ban dinamikus cím van megadva, ahoz kell subnet;
    pUi->setupUi(this);

    pWorkstation->setId(_sNodeType, NT_WORKSTATION);
    pInterface->  setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
    pIpAddress->  setId(_sIpAddressType, AT_DYNAMIC);
    pInterface->addresses << pIpAddress;
    pWorkstation->ports   << pInterface;
    pPlace->setById(0);     // NAME = 'unknown'

    initLists();
    // Workstation típusú nod-ok:
    pUi->comboBoxNode->setModel(pModelNode);
    const static QString sncf = // Konstans szűrő a nodes táblára, SQL kifejezés:
            // Típusa 'workstation' (is)
            quoted(_sWorkstation) + " = ANY " + parentheses(_sNodeType)
            // Legfeljebb 2 portja van
            + " AND 2 >= (SELECT COUNT(*) FROM nports "                             " WHERE node_id = nodes.node_id)"
            // Legfeljebb 1 ip címe van
            + " AND 1 >= (SELECT COUNT(*) FROM ip_addresses JOIN nports USING(port_id) WHERE node_id = nodes.node_id)";
    pModelNode->setConstFilter(sncf, FT_SQL_WHERE);
    pModelNode->nullable = true;        // Az első elem a semmi lessz
    pModelNode->setFilter(_sNul, OT_ASC, FT_SQL_WHERE); // NULL string (nincs további szűrés)
    pUi->comboBoxNode->setCurrentIndex(0);  // első elem, NULL kiválasztva

    pUi->comboBoxFilterZone->addItems(zones);
    pUi->comboBoxFilterZone->setCurrentIndex(0);          // Első elem 'all' zóna
    pUi->comboBoxFilterPlace->addItems(mapZones[_sAll]);
    pUi->comboBoxFilterPlace->setCurrentIndex(0);         // Első elem <NULL>/nincs megadva hely

    pUi->comboBoxZone->addItems(zones);
    pUi->comboBoxZone->setCurrentIndex(0);          // Első elem 'all' zóna
    pUi->comboBoxPlace->addItems(mapZones[_sAll]);
    pUi->comboBoxPlace->setCurrentIndex(0);         // Első elem <NULL>/nincs megadva hely

    pUi->comboBoxVLan->addItem(_sNul);
    pUi->comboBoxVLan->addItems(vlannamelist);
    pUi->comboBoxVLanId->addItem(_sNul);
    pUi->comboBoxVLanId->addItems(vlanidlist);
    pUi->comboBoxSubNet->addItem(_sNul);
    pUi->comboBoxSubNet->addItems(subnetnamelist);
    pUi->comboBoxSubNetAddr->addItem(_sNul);
    pUi->comboBoxSubNetAddr->addItems(subnetaddrlist);

    pLinkButtonsLinkType->addButton(pUi->radioButtonLinkTerm,  LT_TERM);
    pLinkButtonsLinkType->addButton(pUi->radioButtonLinkFront, LT_FRONT);
    pLinkButtonsLinkType->addButton(pUi->radioButtonLinkBack,  LT_BACK);

    pUi->comboBoxLinkZone->addItems(zones);
    pUi->comboBoxLinkZone->setCurrentIndex(0);
    pUi->comboBoxLinkPlace->addItems(mapZones[_sAll]);
    pUi->comboBoxLinkPlace->setCurrentIndex(0);

    pModelLinkNode->nullable = true;
    pUi->comboBoxLinkNode->setModel(pModelLinkNode);
    pModelLinkNode->sFkeyName = _sPlaceId;              // Paraméter ID szerinti szűrés, a model nem tudja kitalálni
    pModelLinkNode->setFilter(_sNul, OT_ASC, FT_SQL_WHERE); // Nincs kiválasztott hely, a lista üres lessz

    // pModelLinkPort->nullable = false;
    pUi->comboBoxLinkPort->setModel(pModelLinkPort);
    pModelLinkPort->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres
    // Link 2
    pLinkButtonsLinkType2->addButton(pUi->radioButtonLinkTerm_2,  LT_TERM);
    pLinkButtonsLinkType2->addButton(pUi->radioButtonLinkFront_2, LT_FRONT);
    pLinkButtonsLinkType2->addButton(pUi->radioButtonLinkBack_2,  LT_BACK);

    pUi->comboBoxLinkZone_2->addItems(zones);
    pUi->comboBoxLinkZone_2->setCurrentIndex(0);
    pUi->comboBoxLinkPlace_2->addItems(mapZones[_sAll]);
    pUi->comboBoxLinkPlace_2->setCurrentIndex(0);

    pModelLinkNode2->nullable = true;
    pUi->comboBoxLinkNode_2->setModel(pModelLinkNode2);
    pModelLinkNode2->sFkeyName = _sPlaceId;              // Paraméter ID szerinti szűrés, a model nem tudja kitalálni
    pModelLinkNode2->setFilter(_sNul, OT_ASC, FT_SQL_WHERE); // Nincs kiválasztott hely, a lista üres lessz

    // pModelLinkPort->nullable = false;
    pUi->comboBoxLinkPort_2->setModel(pModelLinkPort2);
    pModelLinkPort2->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres

    connect(pUi->radioButtonMod,        SIGNAL(toggled(bool)),                this, SLOT(togleNewMod(bool)));
    connect(pUi->pushButtonCancel,      SIGNAL(pressed()),                    this, SLOT(endIt()));
    connect(pUi->pushButtonSave,        SIGNAL(pressed()),                    this, SLOT(save()));
    connect(pUi->pushButtonRefresh,     SIGNAL(pressed()),                    this, SLOT(refresh()));
    connect(pUi->comboBoxFilterZone,    SIGNAL(currentIndexChanged(QString)), this, SLOT(filterZoneCurrentIndex(QString)));
    connect(pUi->comboBoxFilterPlace,   SIGNAL(currentIndexChanged(QString)), this, SLOT(filterPlaceCurrentIndex(QString)));
    connect(pUi->lineEditFilterPattern, SIGNAL(textChanged(QString)),         this, SLOT(filterPatternChanged(QString)));
    // Workstation:
    connect(pUi->comboBoxNode,          SIGNAL(currentIndexChanged(int)),     this, SLOT(nodeCurrentIndex(int)));
    connect(pUi->lineEditName,          SIGNAL(textChanged(QString)),         this, SLOT(nodeNameChanged(QString)));
    connect(pUi->lineEditSerial,        SIGNAL(textChanged(QString)),         this, SLOT(serialChanged(QString)));
    connect(pUi->lineEditInventory,     SIGNAL(textChanged(QString)),         this, SLOT(inventoryChanged(QString)));
    connect(pUi->comboBoxZone,          SIGNAL(currentIndexChanged(QString)), this, SLOT(zoneCurrentIndex(QString)));
    connect(pUi->comboBoxPlace,         SIGNAL(currentIndexChanged(QString)), this, SLOT(placeCurrentIndex(QString)));
    connect(pUi->lineEditPName,         SIGNAL(textChanged(QString)),         this, SLOT(portNameChanged(QString)));
    connect(pUi->comboBoxPType,         SIGNAL(currentIndexChanged(QString)), this, SLOT(portTypeCurrentIndex(QString)));
    connect(pUi->lineEditPMAC,          SIGNAL(textChanged(QString)),         this, SLOT(macAddressChanged(QString)));
    connect(pUi->lineEditAddress,       SIGNAL(textChanged(QString)),         this, SLOT(ipAddressChanged(QString)));
    connect(pUi->comboBoxIpType,        SIGNAL(currentIndexChanged(QString)), this, SLOT(ipAddressTypeCurrentIndex(QString)));
    connect(pUi->comboBoxSubNet,        SIGNAL(currentIndexChanged(int)),     this, SLOT(subNetCurrentIndex(int)));
    connect(pUi->comboBoxSubNetAddr,    SIGNAL(currentIndexChanged(int)),     this, SLOT(subNetAddrCurrentIndex(int)));
    connect(pUi->comboBoxVLan,          SIGNAL(currentIndexChanged(int)),     this, SLOT(vLanCurrentIndex(int)));
    connect(pUi->comboBoxVLanId,        SIGNAL(currentIndexChanged(int)),     this, SLOT(vLanIdCurrentIndex(int)));
    connect(pUi->lineEditPName_2,       SIGNAL(textChanged(QString)),         this, SLOT(portNameChanged2(QString)));
    connect(pUi->comboBoxPType_2,       SIGNAL(currentIndexChanged(QString)), this, SLOT(portTypeCurrentIndex2(QString)));
    connect(pUi->lineEditPMAC_2,        SIGNAL(textChanged(QString)),         this, SLOT(macAddressChanged2(QString)));

    // Link:
    connect(pUi->checkBoxPlaceEqu,      SIGNAL(toggled(bool)),                this, SLOT(_linkToglePlaceEqu(bool)));
    connect(pUi->comboBoxLinkZone,      SIGNAL(currentIndexChanged(QString)), this, SLOT(linkZoneCurrentIndex(QString)));
    connect(pUi->comboBoxLinkPlace,     SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPlaceCurrentIndex(QString)));
    connect(pUi->comboBoxLinkNode,      SIGNAL(currentIndexChanged(int)),     this, SLOT(linkNodeCurrentIndex(int)));
    connect(pUi->comboBoxLinkPort,      SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPortCurrentIndex(QString)));
    connect(pUi->comboBoxLinkPortShare, SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPortShareCurrentIndex(QString)));
    connect(pLinkButtonsLinkType,       SIGNAL(buttonToggled(int,bool)),      this, SLOT(linkChangeLinkType(int, bool)));

    // Link2:
    connect(pUi->checkBoxPlaceEqu_2,    SIGNAL(toggled(bool)),                this, SLOT(linkToglePlaceEqu2(bool)));
    connect(pUi->comboBoxLinkZone_2,    SIGNAL(currentIndexChanged(QString)), this, SLOT(linkZoneCurrentIndex2(QString)));
    connect(pUi->comboBoxLinkPlace_2,   SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPlaceCurrentIndex2(QString)));
    connect(pUi->comboBoxLinkNode_2,    SIGNAL(currentIndexChanged(int)),     this, SLOT(linkNodeCurrentIndex2(int)));
    connect(pUi->comboBoxLinkPort_2,    SIGNAL(currentIndexChanged(int)),     this, SLOT(linkPortCurrentIndex2(int)));
    connect(pUi->comboBoxLinkPortShare_2,SIGNAL(currentIndexChanged(QString)),this, SLOT(linkPortShareCurrentIndex2(QString)));
    connect(pLinkButtonsLinkType2,      SIGNAL(buttonToggled(int,bool)),      this, SLOT(linkChangeLinkType2(int, bool)));

    lockSlot = false;
    // sync..
    withinSlot = 1;
    portTypeCurrentIndex(pUi->comboBoxPType->currentText());
    ipAddressTypeCurrentIndex(pUi->comboBoxIpType->currentText());
    portTypeCurrentIndex2(pUi->comboBoxPType_2->currentText());
    withinSlot = 0;
    setMessage();
}

cWorkstation::~cWorkstation()
{
    lockSlot = true;
    delete pq;
    delete pSample;
    delete pWorkstation;
    delete pPlace;
    delete pLinkNode;
    delete pLinkPort;
    delete pLinkNode2;
    delete pLinkPort2;
}

void cWorkstation::initLists()
{
    // Nem üres zónák listája
    static const QString sql =
            "SELECT place_group_name FROM place_groups"
            " WHERE place_group_type = 'zone'"
             " AND 0 < (SELECT COUNT(*) FROM place_group_places WHERE place_group_places.place_group_id = place_groups.place_group_id)"
            " ORDER BY place_group_name ASC";
    // Az összes zóna neve (place_groups rekord, zone_type = 'zone', betürendben, de az all az első
    if (!execSql(*pq, sql)) {
        EXCEPTION(EDATA, 0, "No any zone.");
    }
    zones << _sAll;
    do {
        QString name = pq->value(0).toString();
        if (name != _sAll) {
            zones << name;
        }
    } while (pq->next());
    // Az "all" zónába tartozó, vagyis az összes hely listája
    cPlace pl;
    QStringList& zoneAll = mapZones[_sAll];
    // Az összes hely, helyiség, betűrendben, de NULL, unknown az első, és a root kimarad
    zoneAll     << _sNul   << _sUnknown;
    placeIdList << NULL_ID << 0;
    if (!pl.fetch(*pq, false, QBitArray(1, false), pl.iTab(_sPlaceName))) {
        EXCEPTION(EDATA, 0, "No any place.");
    }
    do {
        QString name = pl.getName();
        if (name != _sUnknown && name != _sRoot) {
            zoneAll     << name;
            placeIdList << pl.getId();
        }
    } while (pl.next(*pq));
    // A többi hely listát a placesInZone() metódus tölti fel, ha szükséges
    // VLANS:
    cVLan vlan;
    if (vlan.fetch(*pq, false, QBitArray(1, false), vlan.iTab(_sVlanId))) {
        do {
            vlans        << vlan;
            vlannamelist << vlan.getName();
            vlanidlist   << QString::number(vlan.getId());
        } while (vlan.next(*pq));
    }
    else {
        EXCEPTION(EDATA, 0, trUtf8("Nincsenek vlan-ok."));
    }
    // Subnets
    cSubNet subNet;
    if (subNet.fetch(*pq, false, QBitArray(1, false), subNet.iTab(_sVlanId))) {
        do {
            subnets        << subNet;
            subnetnamelist << subNet.getName();
            subnetaddrlist << subNet.getName(_sNetAddr);
        } while (subNet.next(*pq));
    }
    else {
        EXCEPTION(EDATA, 0, trUtf8("Nincsenek subnet-ek."));
    }

}

const QStringList& cWorkstation::placesInZone(const QString& sZone)
{
    QStringList& places = mapZones[sZone];
    if (places.isEmpty()) {
        places << _sNul;   // Első elem: NULL
        const QString sql =
                "SELECT * FROM places"
                " WHERE is_group_place(place_id, ?)"
                " ORDER BY place_name ASC";
        if (!execSql(*pq, sql, sZone)) EXCEPTION(EDATA, 0, trUtf8("A %1 zónához nincs egyetlen hely sem rendelve.").arg(sZone));
        cPlace pl;
        do {
            pl.set(*pq);
            places << pl.getName();
        } while(pq->next());
    }
    return places;
}

int cWorkstation::checkWrkCollision(int ix, const QString& s)
{
    if (s.isEmpty()) return IS_EMPTY;
    if (s == pSample->getName(ix)) {
        return states.modify ? IS_OK : IS_COLLISION;
    }
    cPatch *pO;
    bool r;
    if (pWorkstation->nameIndex() == ix) pO = new cPatch;   // A név mezőnel az őssel sem ütközhet
    else                                 pO = new cNode;
    pO->setName(ix, s);
    if (pO->fetchQuery(*pq, false, _bit(ix))) {
        r = states.modify || pO->getId() != pSample->getId();
    }
    else {
        r = false;
    }
    delete pO;
    return r ? IS_COLLISION : IS_OK;
}

int cWorkstation::_placeCurrentIndex(const QString& sPlace, const QString& sZone,
                                          QComboBox *pComboBoxNode, cRecordListModel *pModel,
                                          const QString& _patt)
{
    bool oldLockSlot = lockSlot;
    QString expr;
    QString actNode = pComboBoxNode->currentText();     // Az aktuálisan kiválasztott node neve
    if (sPlace.isEmpty()) {          // Nincs kiválasztva hely ?
        // Ha nincs kiválasztott hely, akkor az aktuális zóna szerint szűrünk
        if (sZone == _sAll) {
            expr = _sNul;   // ALL zóna: nincs további szűrés
        }
        else {
            // Csak a zónában lévő node-ok
            qlonglong zid = cPlaceGroup().getIdByName(*pq, sZone);
            // Azok a node-ok, melyeknek helye, vagy a hely valamelyik parentje benne van az aktuális zónában
            expr = QString("is_group_place(place_id, %1)").arg(zid);
        }
    }
    else {
        qlonglong pid = cPlace().getIdByName(*pq, sPlace);
        // Azok a node-ok, melyeknek helye, vagy a hely valamelyik parentje azonos az aktuális hellyel.
        expr = QString("is_parent_place(place_id, %1)").arg(pid);
    }
    // Ha van pettern a node névre
    if (!_patt.isEmpty()) {
        QString patt = _patt;
        if (!patt.contains(QChar('%'))) patt += QChar('%'); // ha nincs joker, akkor rakunk a végére
        if (!expr.isEmpty()) expr += " AND ";
        expr += "node_name LIKE " + quoted(patt);
    }
    lockSlot = true;
    pModel->setFilter(expr);
    int ix = pComboBoxNode->findText(actNode);
    if (ix < 0) ix = 0;
    pComboBoxNode->setCurrentIndex(ix);
    lockSlot = oldLockSlot;
    return ix;
}

int cWorkstation::_linkNodeCurrentIndex(int i, cRecordListModel *pModelPort, QComboBox * pComboBoxPort,
                                        cPatch *pNode, cRecordAny *pPort,
                                        ePhsLinkType  *pLinkType, ePortShare *pLinkShared)
{
    int r;
    bool oldLockSlot = lockSlot;
    lockSlot = true;
    if (i == 0) {               // Nincs kiválasztva node
        if (pNode       != NULL) pNode->clear();
        if (pPort       != NULL) pPort->clear();
        if (pLinkShared != NULL) *pLinkShared = ES_NC;
        if (pLinkType   != NULL) *pLinkType = LT_INVALID;
        pModelPort->setFilter(QVariant());   // Nincs node, nincs port
        pComboBoxPort->setDisabled(true);
        r = IS_EMPTY;
    }
    else {
        QString s = pUi->comboBoxLinkNode->currentText();
        if (pNode == NULL || pNode->getName() != s) {
            if (pNode != NULL) pNode->setByName(*pq, s);
            pModelPort->setFilter(pLinkNode->getId());
            pComboBoxPort->setEnabled(true);
            pComboBoxPort->setCurrentIndex(0);
            if (pPort != NULL) pPort->clear();
            if (pLinkShared != NULL) *pLinkShared = ES_NC;
            if (pLinkType   != NULL) *pLinkType = LT_INVALID;
        }
        r =  IS_IMPERFECT;
    }
    lockSlot = oldLockSlot;
    return r;
}

/// A megadott radio button állapotának a beállítása
/// @param pGroup A QButtonGroup objektum pointere
/// @param id A kiválasztott radio button ID-je a group-ban
/// @param check Az állpot (true: checked)
/// @param enable Az engedályezése (true: engedélyezve)
static void buttonCheckEnable(QButtonGroup * pGroup, int id, bool check, bool enable)
{
    QAbstractButton *pButton = pGroup->button(id);
    pButton->setEnabled(enable);
    pButton->setChecked(check);
}

int cWorkstation::_linkPortCurrentIndex(const QString& s,
                                        QButtonGroup * pLinkType, QComboBox *pComboBoxShare, eTristate _isPatch,
                                        cRecordAny *pPort, ePhsLinkType *pType, ePortShare *pShare)
{
    int r = IS_OK;
    bool oldLockSlot = lockSlot;
    lockSlot = true;    // !!
    if (s.isEmpty()) {
        if (pPort  != NULL) pPort->clear();
        if (pType  != NULL) *pType = LT_INVALID;
        if (pShare != NULL) *pShare = ES_NC;
        r = IS_IMPERFECT;
        pComboBoxShare->setCurrentIndex(0);
        pComboBoxShare->setEditable(false);
        buttonCheckEnable(pLinkType, LT_BACK,  false, false);
        buttonCheckEnable(pLinkType, LT_FRONT, false, false);
        buttonCheckEnable(pLinkType, LT_TERM,  false, false);
    }
    else {
        eTristate isPatch = _isPatch;
        if (pPort != NULL) {
            pPort->setByName(*pq, s);
            isPatch = pPort->getId(_sIfTypeId) == cIfType::ifTypeId(_sPatch) ? TS_TRUE : TS_FALSE;
        }
        if (pShare != NULL) *pShare = ES_;
        if (isPatch == TS_NULL) EXCEPTION(EDATA);
        pComboBoxShare->setCurrentIndex(0);
        if (isPatch == TS_TRUE) {
            if (pType != NULL) *pType = LT_FRONT;
            pComboBoxShare->setEnabled(true);
            buttonCheckEnable(pLinkType, LT_BACK,  false, true);
            buttonCheckEnable(pLinkType, LT_FRONT, true,  true);
            buttonCheckEnable(pLinkType, LT_TERM,  false, false);
        }
        else {
            if (pType != NULL) *pType = LT_TERM;
            pComboBoxShare->setEnabled(false);
            buttonCheckEnable(pLinkType, LT_BACK,  false, false);
            buttonCheckEnable(pLinkType, LT_FRONT, false, false);
            buttonCheckEnable(pLinkType, LT_TERM,  true,  false);
        }
        // collisions!:
    }
    lockSlot = oldLockSlot;
    return r;
}


void cWorkstation::_linkToglePlaceEqu(bool f, bool primary)
{
    if (lockSlot) return;
    withinSlot++;
    bool oldLockSlot = lockSlot;
    lockSlot = true;
    QComboBox *pComboBoxZone;
    QComboBox *pComboBoxPlace;
    QComboBox *pComboBoxNode;
    cRecordListModel *pModelNode;
    QComboBox *pComboBoxPort;
    cRecordListModel *pModelPort;
    cPatch            *pNode;
    cRecordAny        *pPort;
    ePhsLinkType      *pLinkType;
    ePortShare        *pShare;

    if (primary) {
        pComboBoxZone  = pUi->comboBoxLinkZone;
        pComboBoxPlace = pUi->comboBoxLinkPlace;
        pComboBoxNode  = pUi->comboBoxLinkNode;
        pModelNode     = pModelLinkNode;
        pComboBoxPort  = pUi->comboBoxLinkPort;
        pModelPort     = pModelLinkPort;
        pNode          = pLinkNode;
        pPort          = pLinkPort;
        pLinkType      = &linkType;
        pShare         = &linkShared;
    }
    else {
        pComboBoxZone  = pUi->comboBoxLinkZone_2;
        pComboBoxPlace = pUi->comboBoxLinkPlace_2;
        pComboBoxNode  = pUi->comboBoxLinkNode_2;
        pModelNode     = pModelLinkNode2;
        pComboBoxPort  = pUi->comboBoxLinkPort_2;
        pModelPort     = pModelLinkPort2;
        pNode          = pLinkNode2;
        pPort          = pLinkPort2;
        pLinkType      = &linkType;
        pShare         = &linkShared;
    }
    if (f) {
        int     ixZone  = pUi->comboBoxZone->currentIndex();
        QString sZone   = zones.at(ixZone);         // Az aktuális zóna (ahol a munkaállomás van)
        QString sPlace  = pPlace->isNull() ? _sNul : pPlace->getName(); // És az aktuálisan megadott hely neve, vagy üres string
        int     ixPlace = mapZones[sZone].indexOf(sPlace);  // Az aktuális hely indexe az aktuális zónában (az üres a nulladik)
        if (ixPlace < 0 || ixZone < 0) EXCEPTION(EPROGFAIL);// Ha nincs találat akkor elkutyultunk valamit
        if (sZone  != pComboBoxZone->currentText()
         || sPlace != pComboBoxPlace->currentText()) {  // Ha nincs változás, akkor nem kell tenni semmit
            pComboBoxZone->setCurrentIndex(ixZone);     // A zónát csak ki kell választani
            pComboBoxPlace->clear();                    // Frissítjük a listát
            pComboBoxPlace->addItems(placesInZone(sZone));
            pComboBoxPlace->setCurrentIndex(ixPlace);   // Béállítjuk a helyet is
            int ix = _placeCurrentIndex(sPlace, sZone, pComboBoxNode, pModelNode);   // SLOT tiltva volt
            int st = _linkNodeCurrentIndex(ix, pModelPort, pComboBoxPort, pNode, pPort, pLinkType, pShare);
            if (primary) states.link  = st;
            else         states.link2 = st;
        }
    }
    // Ha nem engedélyezhető, akkor a checkbox is tiltva, ha más hívja f mindíg true!
    pComboBoxZone->setDisabled(f);
    pComboBoxPlace->setDisabled(f);
    lockSlot = oldLockSlot;
    withinSlot--;
    setMessage();
}

static const QString br     = "<br>";
static inline QString red(const QString& text) {
    return "<div style=\"color:red\">" + text + "</div>";
}
static inline QString bold(const QString& text) {
    return "<b>" + text + "</b>";
}
static inline QString redBold(const QString& text) {
    return red(bold(text));
}
static inline QString italic(const QString& text) {
    return "<i>" + text + "</i>";
}

void cWorkstation::setMessage()
{
    if (withinSlot != 0) return;
    QString infText = linkCollisionsMsg + linkCollisionsMsg2; // ...
    QString errText;
    bool ok = true;
    switch (states.nodeName) {
    case IS_EMPTY:
        ok = false;
        errText += redBold(trUtf8("Nincs megadva a munkaállomás neve."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += redBold(trUtf8("A megadott munkaállomás név ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.serialNumber) {
    case IS_EMPTY:
        errText += trUtf8("Nincs megadva a munkaállomás széria száma.") + br;
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += redBold(trUtf8("A megadott munkaállomás széria szám ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.inventoraNumber) {
    case IS_EMPTY:
        errText += trUtf8("Nincs megadva a munkaállomás leltári száma.") + br;
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += redBold(trUtf8("A megadott munkaállomás leltári szám ütközik egy másikkal."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.nodePlace == IS_EMPTY) {
        errText += trUtf8("Nincs megadva a munkaállomás helye.") + br;
    }
    if (states.portName == IS_EMPTY) {
        ok = false;
        errText += redBold(trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a neve."));
    }
    switch (states.mac) {
    case IS_EMPTY:
        errText += trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a MAC-je.") + br;
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += redBold(trUtf8("A megadott munkaállomás elsődleges interfész MAC ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += redBold(trUtf8("A megadott munkaállomás elsődleges interfész MAC hibás."));
        break;
    }
    switch (states.ipAddr) {
    case IS_EMPTY:
        if (states.ipNeed) {
            errText += trUtf8("Nincs megadva az IP cím.") + br;
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += redBold(trUtf8("A megadott IP cím ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += redBold(trUtf8("A megadott IP cím hibás vagy hányos."));
        break;
    case IS_LOOPBACK:
        ok = false;
        errText += redBold(trUtf8("A megadott IP cím egy loopback cím."));
        break;
    case IS_EXTERNAL:
        ok = false;
        errText += redBold(trUtf8("Csak belső cím adható meg."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.subNetNeed && states.subNet == IS_EMPTY) {
        errText += trUtf8("Nincs megadva IP tartomány illetve VLAN.") + br;
    }
    switch (states.port2Name) {
    case IS_EMPTY:
        if (states.port2Need) {
            ok = false;
            errText += redBold(trUtf8("Nincs megadva a másodlagos port neve."));
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += redBold(trUtf8("A megadott a másodlagos port név nem lehet azonos az elsődlegessel."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.mac2) {
    case IS_EMPTY:
        if (states.mac2need) {
            errText += trUtf8("Nincs megadva a munkaállomás második interfészének a MAC-je.") + br;
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += redBold(trUtf8("A munkaállomás második interfészének a MAC-je ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += redBold(trUtf8("A munkaállomás második interfészének a MAC-je hibás."));
        break;
    }
    if (states.linkPossible) switch (states.link) {
    case IS_EMPTY:
        errText += trUtf8("Nincs megadva link (patch).");
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        break;
    case IS_IMPERFECT:
        errText += redBold(trUtf8("Nincs megadva a linkelt (patch-elt) port."));
        ok = false;
        break;
    }
    if (!ok) {
        errText += trUtf8("Hiba miatt a mentés nem lehetséges.");
    }
    pUi->textEditErr->setText(errText);
    pUi->pushButtonSave->setEnabled(ok);
    pUi->textEditMsg->setText(infText);
}

void cWorkstation::parseObject()
{
    DBGFN();
    withinSlot++;
    pUi->radioButtonMod->setEnabled(true);  // Van minta, elvileg lehet modosítani.
    switch (pWorkstation->ports.size()) {
    case 0:   // Nincs port
        pWorkstation->ports   << (pInterface = new cInterface);
        pInterface->setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
        pInterface->addresses << (pIpAddress = new cIpAddress);
        pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
        break;
    case 1: // Egy portunk van
        if (pWorkstation->ports.first()->tableoid() == cInterface::tableoid_interfaces()) {
            pInterface = pWorkstation->ports.first()->reconvert<cInterface>();
            if (pInterface->addresses.isEmpty()) {
                pInterface->addresses << (pIpAddress = new cIpAddress);
            }
            else {
                pIpAddress = pInterface->addresses.first();
            }
        }
        else {
            pInterface = new cInterface();
            pInterface->copy(*pWorkstation->ports.first());
            pInterface->setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
            pWorkstation->ports.clear() << pInterface ;
            pInterface->addresses << (pIpAddress = new cIpAddress);
            pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
        }
        break;
    case 2: // Két port van
        // Első nem interfész, de a második igen
        if (pWorkstation->ports.at(0)->tableoid() != cInterface::tableoid_interfaces()
         && pWorkstation->ports.at(1)->tableoid() == cInterface::tableoid_interfaces()) {
            // Akkor felcsréljük
            pInterface = pWorkstation->ports.at(1)->reconvert<cInterface>();
            pPort2     = pWorkstation->ports.first();
            pWorkstation->ports[0] = pInterface;
            pWorkstation->ports[1] = pPort2;
        }
        // Az első port egy interfész
        if (pWorkstation->ports.first()->tableoid() == cInterface::tableoid_interfaces()) {
            pInterface = pWorkstation->ports.first()->reconvert<cInterface>();
            pPort2     = pWorkstation->ports.at(1);
            if (pInterface->addresses.isEmpty()) {  // Nincs IP cím az első porton (sorrend véletlen szerű!!)
                if (pPort2->tableoid() == cInterface::tableoid_interfaces()    // A második is interfész ?
                 && !pPort2->reconvert<cInterface>()->addresses.isEmpty()) {   // neki van címe ?
                    // Akkor felcsréljük
                    pInterface = pWorkstation->ports.at(1)->reconvert<cInterface>();
                    pPort2     = pWorkstation->ports.first();
                    pWorkstation->ports[0] = pInterface;
                    pWorkstation->ports[1] = pPort2;
                    pIpAddress = pInterface->addresses.first();
                }
                else {  // Nincs cím, csinálünk egy üreset:
                    pInterface->addresses << (pIpAddress = new cIpAddress);
                    pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
                }
            }
            else {  // Első porton van IP cím (csak egy IP cím lehet)
                pIpAddress = pInterface->addresses.first();
            }
        }
        else {  // Egyik sem interfész, ez elsőt megkamuzzuk mint interfész
            pInterface = new cInterface();
            pInterface->copy(*pWorkstation->ports.first());
            delete pWorkstation->ports.first();
            pWorkstation->ports.first() = pInterface;
            pInterface->setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
            pWorkstation->ports.clear() << pInterface ;
            pInterface->addresses << (pIpAddress = new cIpAddress);
            pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
        }
        break;
    default:    // A konstatns szűrési feltétel alapján ez lehetetlen
        EXCEPTION(EPROGFAIL);
        break;
    }
    QString s;
    int ix;
    pUi->lineEditName->setText(pWorkstation->getName());    // emit nodeNameChanged();
    pUi->lineEditNote->setText(pWorkstation->getNote());
    ix = pUi->comboBoxPlace->findText(pPlace->getName());
    if (ix < 0) EXCEPTION(EPROGFAIL);
    pUi->comboBoxPlace->setCurrentIndex(ix);
    pUi->lineEditSerial->setText(pWorkstation->getName(_sSerialNumber));
    pUi->lineEditInventory->setText(pWorkstation->getName(_sInventoryNumber));

    s = pInterface->getName();
    pUi->lineEditPName->setText(s);     // emit portNameChanged();
    s = pInterface->getNote();
    pUi->lineEditPNote->setText(s);
    s = pInterface->getName(_sPortTag);
    pUi->lineEditPTag->setText(s);
    s = cIfType::ifTypeName(pInterface->getId(_sIfTypeId));
    ix = pUi->comboBoxPType->findText(s);
    pUi->comboBoxPType->setCurrentIndex((ix >= 0) ? ix : 0);  // emit portTypeCurrentIndex();
    cMac mac = pInterface->getMac(_sHwAddress);
    if (mac.isValid()) s = mac.toString();
    else               s = _sNul;
    pUi->lineEditPMAC->setText(s);                          // emit macAddressChanged(s);

    s = pIpAddress->getName(_sIpAddressType);
    ix = pUi->comboBoxIpType->findText(s);
    pUi->comboBoxIpType->setCurrentIndex((ix >= 0) ? ix : 0); // emit ipAddressTypeCurrentIndex(s);
    if (pIpAddress->isNull(_sAddress)) s = _sNul;
    else                               s = pIpAddress->getName(_sAddress);
    pUi->lineEditAddress->setText(s);                       // emit ipAddressChanged(s);
    if (s.isEmpty()) {  // NULL cím esetén nics VLAN állítás!
        qlonglong snid = pIpAddress->getId(_sSubNetId);
        ix = 0;
        if (snid != NULL_ID) {
            ix = subnets.indexOf(snid);
            if (ix < 0) EXCEPTION(EDATA,snid,trUtf8("Missing subnet ID"));
            ix++;
        }
        subNetVLan(ix, -1);
    }
    pUi->lineEditIpNote->setText(pIpAddress->getNote());
    // Link port 1:
    cPhsLink link;
    if (link.isExist(*pq, pInterface->getId(), LT_TERM)) {
        pLinkPort->setById(*pq, link.getId(_sPortId2));
        pLinkNode->setById(*pq, pLinkPort->getId(_sNodeId));
        int aix = placeIdList.indexOf(pLinkNode->getId(_sPlaceId)); // Indexe az összes hely listájában
        if (aix < 0) EXCEPTION(EDATA);
        QString placeName = mapZones[_sAll][ix];
        ix = pUi->comboBoxLinkPlace->findText(placeName);   // Index az aktuális hely listában
        {   /* lockSlot */
            lockSlot = true;
            if (ix < 0) {                                       // Nincs, átváltunk az "all" zónára
                pUi->checkBoxPlaceEqu->setChecked(false);
                pUi->comboBoxLinkZone->setCurrentIndex(0);
                pUi->comboBoxLinkPlace->clear();
                pUi->comboBoxLinkPlace->addItems(mapZones[_sAll]);
                ix = aix;                                       // Az all zonában ez a hely index
            }
            pUi->comboBoxLinkPlace->setCurrentIndex(aix);
            s = pUi->comboBoxLinkPlace->currentText();
            _placeCurrentIndex(s, pUi->comboBoxLinkZone->currentText(), pUi->comboBoxLinkNode, pModelLinkNode);
            s = pLinkNode->getName();
            ix = pUi->comboBoxLinkNode->findText(s);
            if (ix < 0) EXCEPTION(EDATA);
            _linkNodeCurrentIndex(ix, pModelLinkPort, pUi->comboBoxLinkPort, pLinkNode, pLinkPort, &linkType, &linkShared);
            pUi->comboBoxLinkNode->setCurrentIndex(ix);
            s = pLinkPort->getName();
            ix = pUi->comboBoxLinkPort->findText(s);
            if (ix < 0) EXCEPTION(EDATA);
            pUi->comboBoxLinkPort->setCurrentIndex(ix);
            linkType   = (ePhsLinkType)link.getId(_sPhsLinkType2);
            linkShared = (ePortShare)  link.getId(_sPortShared);
            switch (linkType) {
            case LT_FRONT:
                pUi->radioButtonLinkBack->setChecked(false);
                pUi->radioButtonLinkFront->setChecked(true);
                pUi->radioButtonLinkTerm->setChecked(false);
                pUi->radioButtonLinkBack->setEnabled(true);
                pUi->radioButtonLinkFront->setEnabled(true);
                pUi->radioButtonLinkTerm->setEnabled(false);
                break;
            case LT_BACK:
                pUi->radioButtonLinkBack->setChecked(true);
                pUi->radioButtonLinkFront->setChecked(false);
                pUi->radioButtonLinkTerm->setChecked(false);
                pUi->radioButtonLinkBack->setEnabled(true);
                pUi->radioButtonLinkFront->setEnabled(true);
                pUi->radioButtonLinkTerm->setEnabled(false);
                break;
            case LT_TERM:
                pUi->radioButtonLinkBack->setChecked(false);
                pUi->radioButtonLinkFront->setChecked(false);
                pUi->radioButtonLinkTerm->setChecked(true);
                pUi->radioButtonLinkBack->setEnabled(false);
                pUi->radioButtonLinkFront->setEnabled(false);
                pUi->radioButtonLinkTerm->setEnabled(false);
                break;
            default:
                EXCEPTION(EPROGFAIL);
                break;
            }
            ix = pUi->comboBoxLinkPortShare->findText(portShare(linkShared));
            if (ix < 0) EXCEPTION(EDATA);
            pUi->comboBoxLinkPortShare->setCurrentIndex(ix);
            lockSlot = false;
        }   /* lockSlot */

    }
    else {
        pLinkNode->clear();
        pLinkPort->clear();
        linkType = LT_INVALID;
        linkShared = ES_NC;
        lockSlot = true;
        /* lockSlot */ pUi->comboBoxLinkNode->setCurrentIndex(0);
        /* lockSlot */ pUi->comboBoxLinkPort->setCurrentIndex(0);
        /* lockSlot */ pUi->radioButtonLinkBack->setChecked(false);
        /* lockSlot */ pUi->radioButtonLinkFront->setChecked(false);
        /* lockSlot */ pUi->radioButtonLinkTerm->setChecked(false);
        lockSlot = false;
        pUi->radioButtonLinkBack->setEnabled(false);
        pUi->radioButtonLinkFront->setEnabled(false);
        pUi->radioButtonLinkTerm->setEnabled(false);
    }

/*    if (pPort2 == NULL) {
        pUi->comboBoxPType_2->setCurrentIndex(0);   // nincs port
        pDelete(pLinkPrt2);
        pDelete(pLinkNode2);
        linkType = link.getId(_sPhsLinkType2);
    }
    else {
        s = pPort2->ifType().getName();
        ix = pUi->comboBoxPType_2->findText(s);
        if (ix < 0) ix = 1;   // a 0. a nincs, az második (ix=1) az alapértelmezett!!
        pUi->comboBoxPType_2->setCurrentIndex(ix);
        s = pUi->comboBoxPType_2->currentText();
        pPort2->setId(cIfType::ifTypeId(s));
    }
    portTypeCurrentIndex2();
*/
    withinSlot--;
    setMessage();
    DBGFN();
}

void cWorkstation::subNetVLan(int sni, int vli)
{
    int ix;
    cSubNet   *psn = NULL;
    cVLan     *pvl = NULL;
    qlonglong vlid = NULL_ID;
    if (vli < 0) {                          // sni: SubNet index
        if (sni < 0) EXCEPTION(EPROGFAIL);
        if (sni == 0) {
            states.subNet =  IS_EMPTY;
            vli = 0;
        }
        else {
            states.subNet = IS_OK;
            ix = sni - 1;     // Első elem a comboBox-ban egy üres string
            psn = subnets[ix];
            vlid = psn->getId(_sVlanId);
            if (vlid != NULL_ID) {
                ix = vlans.indexOf(vlid);
                if (ix >= 0) pvl = vlans[ix];
                vli = ix +1;
            }
            else {
                vli = 0;
            }
        }

    }
    else {                                  // lvi: VLan index
        if (vli == 0) {
            states.subNet =  IS_EMPTY;
            sni = 0;
        }
        else {
            states.subNet = IS_OK;
            ix = vli - 1;     // Első elem a comboBox-ban egy üres string
            pvl = vlans[ix];
            qlonglong vlid = pvl->getId();
            ix = subnets.indexOf(_sVlanId, vlid); // Elvileg több találat is lehet !!! Egyelőre nem foglakozunk vele...
            if (ix >= 0) psn = subnets[ix];
            vli = ix +1;
        }
    }
    if (psn != NULL) {
        pIpAddress->setId(_sSubNetId, psn->getId());
    }
    else {
        pIpAddress->setId(_sSubNetId, NULL_ID);
    }
    if (pvl != NULL) {
        if (pInterface->vlans.isEmpty()) {
            pInterface->vlans << new cPortVlan();
        }
        cPortVlan& pv = *pInterface->vlans.first();
        pv.setId(_sVlanId, pvl->getId());
        pv.setId(_sVlanType, VT_HARD);
        pv.setId(_sSetType, ST_MANUAL);
    }
    else {
        pInterface->vlans.clear();
    }
    lockSlot = true;
    /* lockSlot */ pUi->comboBoxSubNet->setCurrentIndex(sni);
    /* lockSlot */ pUi->comboBoxSubNetAddr->setCurrentIndex(sni);
    /* lockSlot */ pUi->comboBoxVLan->setCurrentIndex(vli);
    /* lockSlot */ pUi->comboBoxVLanId->setCurrentIndex(vli);
    lockSlot = false;
    setMessage();
}

/**********************   SLOTS   ************************/

#define _BEGINSLOT(s) \
    _DBGFN() << (lockSlot ? " Skip (" : " (") << s << ")" << endl; \
    if (lockSlot) return;

#define BEGINSLOT(s) \
    _DBGFN() << (lockSlot ? " Skip (" : " (") << s << ")" << endl; \
    if (lockSlot) return; \
    ++withinSlot;

#define ENDSLOT() \
    --withinSlot; \
    DBGFNL()

#define ENDSLOTM() \
    --withinSlot; \
    setMessage(); \
    DBGFNL()


void cWorkstation::togleNewMod(bool f)
{
    BEGINSLOT(DBOOL(f));
    states.modify = f;
    if (!f && pSample->isNull()) {  // Meg kell adni, mit is modosítunk, anélkül nem OK
        EXCEPTION(EPROGFAIL);
    }
    // Pár státuszt újra kell számolni! (Ami függ a states.modify értékétől)
    nodeNameChanged(pWorkstation->getName());
    serialChanged(pWorkstation->getName(_sSerialNumber));
    inventoryChanged(pWorkstation->getName(_sInventoryNumber));
    macAddressChanged(pInterface->getName(_sHwAddress));
    ipAddressChanged(pIpAddress->getName(cIpAddress::ixAddress()));
    ENDSLOTM();
}

void cWorkstation::save()
{
    DBGFN();
    // Néhány érték nem íródik automatikusan az objektumba
    pWorkstation->setNote(pUi->lineEditNote->text());
    pInterface->setNote(pUi->lineEditPNote->text());
    pInterface->setName(_sPortTag, pUi->lineEditPTag->text());
    pIpAddress->setNote(pUi->lineEditIpNote->text());
    if (pPort2 != NULL) {
        pPort2->setNote(pUi->lineEditPNote_2->text());
    }
    qlonglong id = pWorkstation->getId();   // Hiba esetén vissza kell írni
    cError *pe = NULL;
    const static QString tkey = "Workstation_save";
    try {
        sqlBegin(*pq, tkey);
        if (states.modify) {
            pWorkstation->rewriteById(*pq);
        }
        else {
            pWorkstation->clear(pWorkstation->idIndex());
            pWorkstation->insert(*pq);
        }
        sqlEnd(*pq, tkey);
    } CATCHS(pe);
    if (pe != NULL) {
        sqlRollback(*pq, tkey);
        cErrorMessageBox::messageBox(pe, this);
        pWorkstation->setId(id);    // Vissza az ID
    }
    parseObject();
    DBGFNL();
}

void cWorkstation::refresh()
{
    // MAJD
}

void cWorkstation::filterZoneCurrentIndex(const QString& s)
{
    _DBGFN() << " (" << s << ")" << endl;
    const QStringList& pl = placesInZone(s);    // Helyek a zónában
    QString placeName = pUi->comboBoxFilterPlace->currentText();
    int i = pl.indexOf(placeName);      // Az aktuális hely indexe az új listában
    if (i < 0) i = 0;       // Ha nincs az aktuális helyiség a kiválasztott zónában
    pUi->comboBoxFilterPlace->clear();
    pUi->comboBoxFilterPlace->addItems(pl);
    pUi->comboBoxFilterPlace->setCurrentIndex(i);
    filterPlaceCurrentIndex(pl.at(i));
    DBGFNL();
}

void cWorkstation::filterPlaceCurrentIndex(const QString& s)
{
    _DBGFN() << " (" << s << ")" << endl;
    QString patt       = pUi->lineEditFilterPattern->text();
    QString zone_name  = pUi->comboBoxFilterZone->currentText();
    _placeCurrentIndex(s, zone_name, pUi->comboBoxNode, pModelNode, patt);
    DBGFNL();
}

void cWorkstation::filterPatternChanged(const QString& s)
{
    _DBGFN() << " (" << s << ")" << endl;
    QString place_name = pUi->comboBoxFilterPlace->currentText();
    QString zone_name  = pUi->comboBoxFilterZone->currentText();
    _placeCurrentIndex(place_name, zone_name, pUi->comboBoxNode, pModelNode, s);
    DBGFNL();
}

// Workstation (node) slots
void cWorkstation::nodeCurrentIndex(int i)
{
    _BEGINSLOT(i);
    if (i == 0) return; // Első elem, semmi (nincs minta)
    if (withinSlot) EXCEPTION(EPROGFAIL);   // Nem hívja senki, csak nulla lehet
    qlonglong id = pModelNode->atId(i); // node_id
    if (pSample->getId() == id) return; // Nincs változás
    pSample->setById(*pq, id);
    pSample->fetchPorts(*pq);
    pInterface = NULL;  // A klonozásnál fel lesznek szabadítva!!!
    pIpAddress = NULL;
    pPort2     = NULL;
    pWorkstation->clone(*pSample);
    pPlace->fetchById(*pq, pWorkstation->getId(_sPlaceId));
    parseObject();
    DBGFNL();
}

void cWorkstation::nodeNameChanged(const QString& s)
{
    _BEGINSLOT(s);
    // withinSlot++;
    pWorkstation->setName(s);
    states.nodeName = checkWrkCollision(pWorkstation->nameIndex(), s);
    // withinSlot--;
    setMessage();
    DBGFNL();
}

void cWorkstation::serialChanged(const QString& s)
{
    _BEGINSLOT(s);
    // withinSlot++;
    int ix = pWorkstation->toIndex(_sSerialNumber);
    pWorkstation->setName(ix, s);
    states.serialNumber = checkWrkCollision(ix, s);
    // withinSlot--;
    setMessage();
    DBGFNL();
}

void cWorkstation::inventoryChanged(const QString& s)
{
    _BEGINSLOT(s);
    // withinSlot++;
    int ix = pWorkstation->toIndex(_sInventoryNumber);
    pWorkstation->setName(ix, s);
    states.inventoraNumber = checkWrkCollision(ix, s);
    // withinSlot--;
    setMessage();
    DBGFNL();
}

void cWorkstation::zoneCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    const QStringList& pl = placesInZone(s);    // Helyek a zónában
    int i = pl.indexOf(pPlace->getName());      // Az aktuális hely indexe az új listában
    if (i < 0) i = 0;       // Ha nincs az aktuális helyiség a kiválasztott zónában
    lockSlot = true;        // A slot-ot majd a végén meghívjuk, ha megvan a végleges index
    /* lockSlot */ pUi->comboBoxPlace->clear();
    /* lockSlot */ pUi->comboBoxPlace->addItems(pl);
    /* lockSlot */ pUi->comboBoxPlace->setCurrentIndex(i);
    lockSlot = false;
    placeCurrentIndex(pl.at(i));
    ENDSLOTM();
}

void cWorkstation::placeCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    if (s.isEmpty()) {   // Nincs kiválasztva hely ?
        pPlace->clear();
        states.nodePlace = IS_EMPTY;
    }
    else {
        pPlace->setByName(*pq, s);
        states.nodePlace = IS_OK;
    }
    pWorkstation->setId(_sPlaceId, pPlace->getId());
    if (pUi->checkBoxPlaceEqu->isChecked())   _linkToglePlaceEqu(true, true);    // Elsődleges
    if (pUi->checkBoxPlaceEqu_2->isChecked()) _linkToglePlaceEqu(true, false);   // Mésodlagos
    ENDSLOTM();
}
// Workstation ports slots
void cWorkstation::portNameChanged(const QString& s)
{
    BEGINSLOT(s);
    pInterface->setName(s);
    states.portName = s.isEmpty() ? IS_EMPTY : IS_OK;
    withinSlot--;
    ENDSLOTM();
}

void cWorkstation::portTypeCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    const cIfType& iftype = cIfType::ifType(s);
    pInterface->setId(_sIfTypeId, iftype.getId());
    int lt = iftype.getId(_sIfTypeLinkType);
    states.linkPossible = lt == LT_PTP || lt == LT_BUS;
    pUi->comboBoxLinkZone->setEnabled(states.linkPossible);
    pUi->comboBoxLinkPlace->setEnabled(states.linkPossible);
    pUi->checkBoxPlaceEqu->setEnabled(states.linkPossible);
    pUi->comboBoxLinkNode->setEnabled(states.linkPossible);
    pUi->comboBoxLinkPort->setEnabled(states.linkPossible);
    pUi->comboBoxLinkPortShare->setEnabled(states.linkPossible);
    pUi->pushButtonLinkOther->setEnabled(states.linkPossible && states.link == IS_OK);
    pUi->radioButtonLinkBack->setEnabled(states.linkPossible);
    pUi->radioButtonLinkFront->setEnabled(states.linkPossible);
    pUi->radioButtonLinkTerm->setEnabled(states.linkPossible);
    pUi->lineEditLinkNote->setEnabled(states.linkPossible);

    pUi->checkBoxSrvIns->setEnabled(states.linkPossible);   // !!!
    pUi->checkBoxSrvDel->setEnabled(states.linkPossible);
    pUi->checkBoxSrvEna->setEnabled(states.linkPossible);
    pUi->lineEditSrvNote->setEnabled(states.linkPossible);
    ENDSLOTM();
}

void cWorkstation::macAddressChanged(const QString& s)
{
    _BEGINSLOT(s);
    // withinSlot++;
    if (s.isEmpty()) {
        pInterface->clear(_sHwAddress);
        states.mac = IS_EMPTY;
    }
    else {
        cMac mac(s);
        if (mac.isValid()) {
            pInterface->setMac(_sHwAddress, mac);
            cNode no;
            if (no.fetchByMac(*pq, mac) && !(states.modify && pSample->getId() == no.getId())) {
                states.mac = IS_COLLISION;
            }
            else {
                states.mac = IS_OK;
            }
        }
        else {
            states.mac = IS_INVALID;
        }
    }
    // withinSlot--;
    setMessage();
    DBGFNL();
}

// Workstation address slots
void cWorkstation::ipAddressChanged(const QString& s)
{
    _BEGINSLOT(s);
    // withinSlot++;
    if (s.isEmpty()) {
        pIpAddress->clear(_sAddress);
        states.ipAddr = IS_EMPTY;
    }
    else {
        QHostAddress a(s);
        if (a.isLoopback()) {
            states.ipAddr = IS_LOOPBACK;
        }
        else {
            bool ok = !a.isNull();
            // PDEB(VERBOSE) << s << " -> " << a.toString() << " / " << DBOOL(a.isNull()) << ", " << DBOOL(loopback) << endl;
            if (ok) {
                static const QRegExp pattern("\\d+\\.\\d+\\.\\d+\\.\\d+");
                QAbstractSocket::NetworkLayerProtocol prot = a.protocol();
                switch (prot) {
                case QAbstractSocket::IPv4Protocol: // A Qt engedékenyebb a cím megadásnál mint szeretném
                    ok = pattern.exactMatch(s);
                    break;
                case QAbstractSocket::IPv6Protocol:
                    ok = s != "::"; // Szerintem ez nem egy jó cím
                    break;
                default:
                    ok = false;
                    break;
                }
            }
            if (ok) {
                cNode no;
                if (no.fetchByIp(*pq, a, EX_IGNORE) && !(states.modify && pSample->getId() == no.getId())) {
                    states.ipAddr = IS_COLLISION;
                    ok = false;
                    cSubNet sn;
                    int ix = sn.getByAddress(*pq, a);
                    switch (ix) {  // Hány subnet-re illeszkedik a cím
                    case 0: // Nincs hozzá subnet
                        if (ok) states.ipAddr = IS_EXTERNAL;
                        break;
                    case 1: // 1 subnet, OK, belső cím
                        ix = pUi->comboBoxSubNet->findText(sn.getName());
                        if (ix < 0) EXCEPTION(EDATA);
                        if (ok) states.ipAddr = IS_OK;
                        pIpAddress->setAddress(a);
                        subNetVLan(ix, -1);
                        break;
                    default:
                        if (ix > 0) {
                            EXCEPTION(ENOTSUPP);    // Egyelőre nem támogatjuk az átlapolódó subnet-eket ITT!
                        }
                        else {
                            EXCEPTION(EPROGFAIL);
                        }
                        break;
                    }
                }
            }
            else {
                states.ipAddr = IS_INVALID;
            }
        }
    }
    bool f = !s.isEmpty();
    pUi->comboBoxSubNet->setDisabled(f);
    pUi->comboBoxSubNetAddr->setDisabled(f);
    pUi->comboBoxVLan->setDisabled(f);
    pUi->comboBoxVLanId->setDisabled(f);
    // withinSlot--;
    setMessage();
    DBGFNL();
}

void cWorkstation::ipAddressTypeCurrentIndex(const QString& s)
{
    _BEGINSLOT(s);
    // withinSlot++;
    pIpAddress->setName(cIpAddress::ixIpAddressType(), s);
    switch (pIpAddress->getId(cIpAddress::ixIpAddressType())) {
    case AT_FIXIP:
        states.ipNeed = true;
        break;
    case AT_DYNAMIC:
        states.ipNeed = false;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    // withinSlot--;
    setMessage();
    DBGFNL();
}

void cWorkstation::subNetCurrentIndex(int i)
{
    _BEGINSLOT(i);
    subNetVLan(i, -1);
    DBGFNL();
}

void cWorkstation::subNetAddrCurrentIndex(int i)
{
    _BEGINSLOT(i);
    subNetVLan(i, -1);
    DBGFNL();
}

void cWorkstation::vLanCurrentIndex(int i)
{
    _BEGINSLOT(i);
    subNetVLan(-1, i);
    DBGFNL();
}

void cWorkstation::vLanIdCurrentIndex(int i)
{
    _BEGINSLOT(i);
    subNetVLan(-1, i);
    DBGFNL();
}

// LINK 1 slots

/// \brief linkCollision
/// \return A link statusz (IS_OK vagy IS_COLLISION)
/// @param pid  saját port ID
/// @param lpid Linkelt port ID
/// @param type Fizikai link típusa
/// @param sh   Megosztás
/// @param msg  Üzenet string referencia
static int linkCollision(QSqlQuery& q, qlonglong pid, qlonglong lpid, ePhsLinkType type, ePortShare sh, QString& msg)
{
    DBGFN();
    msg.clear();
    cPhsLink link;
    tRecordList<cPhsLink> list;
    int r;
    if (link.collisions(q, list, lpid, type, sh)
     && (list.size() != 1 && list.first()->getId(_sPortId1) != pid)) {
        cPhsLink *pL;
        cNode  n;
        cNPort p;
        msg += QObject::trUtf8("Az elsődleges porthoz megadott link a következő link(ek)el ütközik:")
                + "<table> <tr> <th> Cél host </tr><tr> Port </tr><tr> Típus </tr><tr> Megosztás </tr></tr>\n";
        while (NULL != (pL = list.pop_back(EX_IGNORE))) {
            if (pL->getId(_sPortId1) == pid) continue;
            p.setById(q, pL->getId(__sPortId2));
            QString nn = n.getNameById(p.getId(_sNodeId));
            msg += "<tr><td>";
            msg += nn;
            msg += "</td\n><td>";
            msg += p.getName();
            msg += "</td\n><td>";
            msg += pL->getName(_sPhsLinkType2);
            msg += "</td\n><td>";
            msg += pL->getName(_sPortShared);
            msg += "</td\n></tr>\n";
        }
        msg += "</table>\n";
        r = IS_COLLISION;
    }
    else {
        r = IS_OK;
    }
    _DBGFNL() << r << endl;
    return r;
}

void cWorkstation::linkChangeLinkType(int id, bool f)
{
    _BEGINSLOT((QString::number(id) + ", " + DBOOL(f)));
    if (f && id != linkType) {
        linkType = (ePhsLinkType)id;
        if (states.linkPossible && (states.link == IS_OK || states.link == IS_COLLISION)) {
            states.link = linkCollision(*pq, pInterface->getId(), pLinkPort->getId(), linkType, linkShared, linkCollisionsMsg);
        }
    }
    setMessage();
    DBGFNL();
}

void cWorkstation::_linkToglePlaceEqu(bool f)
{
    _DBGFN() << " (" << DBOOL(f) << ")" << endl;
    _linkToglePlaceEqu(f, true);
    DBGFNL();
}

void cWorkstation::linkZoneCurrentIndex(const QString &s)
{
    BEGINSLOT(s);
    const QStringList& pl = placesInZone(s);    // Helyek a zónában
    int i = pl.indexOf(pPlace->getName());      // Az aktuális hely indexe az új listában
    if (i < 0) i = 0;       // Ha nincs az aktuális helyiség a kiválasztott zónában
    lockSlot = true;        // A slot-ot majd a végén meghívjuk, ha megvan a végleges index
    /* lockSlot */ pUi->comboBoxLinkPlace->clear();
    /* lockSlot */ pUi->comboBoxLinkPlace->addItems(pl);
    /* lockSlot */ pUi->comboBoxLinkPlace->setCurrentIndex(i);
    lockSlot = false;
    withinSlot--;
    linkPlaceCurrentIndex(pl.at(i));
    DBGFNL();
}

void cWorkstation::linkPlaceCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    int ix = _placeCurrentIndex(s, pUi->comboBoxLinkZone->currentText(), pUi->comboBoxLinkNode, pModelLinkNode);
    pUi->comboBoxLinkNode->setCurrentIndex(ix);
    ENDSLOTM();
}

void cWorkstation::linkNodeCurrentIndex(int i)
{
    _BEGINSLOT(i);
    _linkNodeCurrentIndex(i, pModelLinkPort, pUi->comboBoxLinkPort, pLinkNode, pLinkPort, &linkType, &linkShared);
    setMessage();
    DBGFNL();
}


void cWorkstation::linkPortCurrentIndex(const QString& s)
{
    _BEGINSLOT(s);
    linkCollisionsMsg.clear();
    states.link = _linkPortCurrentIndex(s, pLinkButtonsLinkType, pUi->comboBoxLinkPortShare, TS_NULL, pLinkPort, &linkType, &linkShared);
    if (states.link == IS_OK) {
        states.link = linkCollision(*pq, pInterface->getId(), pLinkPort->getId(), linkType, linkShared, linkCollisionsMsg);
    }
    setMessage();
    DBGFNL();
}

void cWorkstation::linkPortShareCurrentIndex(const QString &s)
{
    _BEGINSLOT(s);
    linkShared = (ePortShare)portShare(s);
    states.link = linkCollision(*pq, pInterface->getId(), pLinkPort->getId(), linkType, linkShared, linkCollisionsMsg);
    setMessage();
    DBGFNL();
}

// Workstation port 2 slots
void cWorkstation::portNameChanged2(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::portTypeCurrentIndex2(const QString& s)
{
    BEGINSLOT(s);
    if (lockSlot) return;
    withinSlot++;
    int ix =pUi->comboBoxLinkPort_2->currentIndex();
    if (0 >= ix) {   // Nincs 2. port
        if (pWorkstation->ports.size() > 1) delete pWorkstation->ports.pop_back();
        states.port2Need     = false;
        states.link2Possible = false;
        states.mac2need      = false;
        pPort2 = NULL;
    }
    else {
        states.port2Need     = true;
        const cIfType& iftype = cIfType::ifType(s);
        if (pWorkstation->ports.size() > 1) {
            pPort2 = pWorkstation->ports.at(1);
            if (pPort2->getId(_sIfTypeId) != iftype.getId()) {
                delete pWorkstation->ports.pop_back(); // Nemjó eldobjuk
                pPort2 = cNPort::newPortObj(iftype);
                pWorkstation->ports << pPort2;
            }
        }
        else {
            pPort2 = cNPort::newPortObj(iftype);
            pWorkstation->ports << pPort2;
        }
        states.link2Possible = false;
        states.mac2need = pPort2->isIndex(_sHwAddress);
        int lt = iftype.getId(_sIfTypeLinkType);
        states.linkPossible = lt == LT_PTP || lt == LT_BUS;
    }

    ENDSLOTM();
}

void cWorkstation::macAddressChanged2(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

// LINK 2 slots
void cWorkstation::linkChangeLinkType2(int id, bool f)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkToglePlaceEqu2(bool f)
{
    _linkToglePlaceEqu(f, true);
}

void cWorkstation::linkZoneCurrentIndex2(const QString &s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkPlaceCurrentIndex2(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkNodeCurrentIndex2(int i)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkPortCurrentIndex2(int i)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkPortShareCurrentIndex2(const QString &s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}
