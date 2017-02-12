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
    pPort2(NULL),
    pLinkNode(new cPatch),
    pLinkPrt(NULL),
    pLinkButtonsLinkType(new QButtonGroup(this)),
    pModelLinkNode(new cRecordListModel(pLinkNode->descr())),
    pModelLinkPort(new cRecordListModel("patchable_ports")),
    pLinkNode2(NULL),
    pLinkPrt2(NULL),
    pLinkButtonsLinkType2(new QButtonGroup(this)),
    pModelLinkNode2(new cRecordListModel(pLinkNode->descr())),
    // Az nports tábla helyett a patchable_ports view-t használjuk, hogy csak a patch-elhető portokat listázza.
    pModelLinkPort2(new cRecordListModel("patchable_ports"))
{
    withinSlot = 0;
    lockSlot = true;
    memset(&states, 0, sizeof(states)); // IS_EMPTY, false
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

    pUi->comboBoxLinkNode->setModel(pModelLinkNode);
    pModelLinkNode->sFkeyName = _sPlaceId;              // Paraméter ID szerinti szűrés, a model nem tudja kitalálni
    pModelLinkNode->setFilter(_sNul, OT_ASC, FT_SQL_WHERE); // Nincs kiválasztott hely, a lista üres lessz

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

    pUi->comboBoxLinkNode_2->setModel(pModelLinkNode2);
    pModelLinkNode2->sFkeyName = _sPlaceId;              // Paraméter ID szerinti szűrés, a model nem tudja kitalálni
    pModelLinkNode2->setFilter(_sNul, OT_ASC, FT_SQL_WHERE); // Nincs kiválasztott hely, a lista üres lessz

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
    connect(pUi->comboBoxPType_2,       SIGNAL(currentIndexChanged(int)),     this, SLOT(portTypeCurrentIndex2(int)));
    connect(pUi->lineEditPMAC_2,        SIGNAL(textChanged(QString)),         this, SLOT(macAddressChanged2(QString)));

    // Link:
    connect(pUi->checkBoxPlaceEqu,      SIGNAL(toggled(bool)),                this, SLOT(linkToglePlaceEqu(bool)));
    connect(pUi->comboBoxLinkZone,      SIGNAL(currentIndexChanged(QString)), this, SLOT(linkZoneCurrentIndex(QString)));
    connect(pUi->comboBoxLinkPlace,     SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPlaceCurrentIndex(QString)));
    connect(pUi->comboBoxLinkNode,      SIGNAL(currentIndexChanged(int)),     this, SLOT(linkNodeCurrentIndex(int)));
    connect(pUi->comboBoxLinkPort,      SIGNAL(currentIndexChanged(int)),     this, SLOT(linkPortCurrentIndex(int)));
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
    withinSlot = 0;
    setMessage();
}

cWorkstation::~cWorkstation()
{
    lockSlot = true;
    delete pq;
    delete pSample;
    delete pWorkstation;
    delete pLinkNode;
    delete pLinkPrt;
    delete pPlace;
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
    zoneAll << _sNul;
    zoneAll << _sUnknown;
    if (!pl.fetch(*pq, false, QBitArray(1, false), pl.iTab(_sPlaceName))) {
        EXCEPTION(EDATA, 0, "No any place.");
    }
    do {
        QString name = pl.getName();
        if (name != _sUnknown && name != _sRoot) {
            zoneAll << name;
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

qlonglong cWorkstation::placeCurrentIndex(const QString& sPlace, QComboBox *pComboBoxZone,
                                          QComboBox *pComboBoxNode, cRecordListModel *pModel,
                                          const QString& _patt)
{
    qlonglong pid = NULL_ID;
    QString expr;
    if (sPlace.isEmpty()) {          // Nincs kiválasztva hely ?
        // Ha nincs kiválasztott hely, akkor az aktuális zóna szerint szűrünk
        QString sZone = pComboBoxZone->currentText();
        if (sZone == _sAll) {
            expr = _sNul;   // ALL zóna: nincs további szűrés
        }
        else {
            // Csak a zónában lévő node-ok
            qlonglong zid = cPlaceGroup().getIdByName(*pq, sZone);
            expr = QString("is_group_place(place_id, %1)").arg(zid);
        }
    }
    else {
        pid = cPlace().getIdByName(*pq, sPlace);
        expr = QString("is_parent_place(place_id, %1)").arg(pid);
    }
    if (!_patt.isEmpty()) {
        QString patt = _patt;
        if (!patt.contains(QChar('%'))) patt += QChar('%');
        if (!expr.isEmpty()) expr += " AND ";
        expr += "node_name LIKE " + quoted(patt);
    }
    lockSlot = true;
    /* lockSlot */ pModel->setFilter(expr);
    /* lockSlot */ int ix = pComboBoxNode->findText(pSample->getName());
    /* lockSlot */ if (ix < 0) ix = 1;
    /* lockSlot */ pComboBoxNode->setCurrentIndex(ix);
    lockSlot = false;
    return pid;
}

void cWorkstation::linkToglePlaceEqu(bool f, bool primary)
{
    if (lockSlot) return;
    withinSlot++;
    setMessage();
    QComboBox *pComboBoxZone;
    QComboBox *pComboBoxPlace;
    if (primary) {
        pComboBoxZone  = pUi->comboBoxLinkZone;
        pComboBoxPlace = pUi->comboBoxLinkPlace;
    }
    else {
        pComboBoxZone  = pUi->comboBoxLinkZone_2;
        pComboBoxPlace = pUi->comboBoxLinkPlace_2;
    }
    if (f) {
        int     ixZone  = pUi->comboBoxZone->currentIndex();
        QString sZone   = zones.at(ixZone);
        QString sPlace  = pPlace->isNull() ? _sNul : pPlace->getName();
        int     ixPlace = mapZones[sZone].indexOf(sPlace);
        if (ixPlace < 0 || ixZone < 0) EXCEPTION(EPROGFAIL);
        if (sZone  != pComboBoxZone->currentText()
         || sPlace != pComboBoxPlace->currentText()) {
            pComboBoxZone->setCurrentIndex(ixZone);
            lockSlot = true;
            /* lockSlot */ pComboBoxPlace->clear();
            /* lockSlot */ pComboBoxPlace->addItems(placesInZone(sZone));
            lockSlot = false;
            pComboBoxPlace->setCurrentIndex(ixPlace);
        }
    }
    // Ha nem engedélyezhető, akkor a checkbox is tiltva, ha más hívja f mindíg true!
    pComboBoxZone->setDisabled(f);
    pComboBoxPlace->setDisabled(f);
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
    QString text;
    bool ok = true;
    switch (states.nodeName) {
    case IS_EMPTY:
        ok = false;
        text += redBold(trUtf8("Nincs megadva a munkaállomás neve."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        text += redBold(trUtf8("A megadott munkaállomás név ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.serialNumber) {
    case IS_EMPTY:
        text += trUtf8("Nincs megadva a munkaállomás széria száma.") + br;
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        text += redBold(trUtf8("A megadott munkaállomás széria szám ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.inventoraNumber) {
    case IS_EMPTY:
        text += trUtf8("Nincs megadva a munkaállomás leltári száma.") + br;
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        text += redBold(trUtf8("A megadott munkaállomás leltári szám ütközik egy másikkal."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.nodePlace == IS_EMPTY) {
        text += trUtf8("Nincs megadva a munkaállomás helye.") + br;
    }
    if (states.portName == IS_EMPTY) {
        ok = false;
        text += redBold(trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a neve."));
    }
    switch (states.mac) {
    case IS_EMPTY:
        text += trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a MAC-je.") + br;
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        text += redBold(trUtf8("A megadott munkaállomás elsődleges interfész MAC ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        text += redBold(trUtf8("A megadott munkaállomás elsődleges interfész MAC hibás."));
        break;
    }
    switch (states.ipAddr) {
    case IS_EMPTY:
        if (states.ipNeed) {
            text += trUtf8("Nincs megadva az IP cím.") + br;
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        text += redBold(trUtf8("A megadott IP cím ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        text += redBold(trUtf8("A megadott IP cím hibás vagy hányos."));
        break;
    case IS_LOOPBACK:
        ok = false;
        text += redBold(trUtf8("A megadott IP cím egy loopback cím."));
        break;
    case IS_EXTERNAL:
        ok = false;
        text += redBold(trUtf8("Csak belső cím adható meg."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.subNetNeed && states.subNet == IS_EMPTY) {
        text += trUtf8("Nincs megadva IP tartomány illetve VLAN.") + br;
    }
    switch (states.port2Name) {
    case IS_EMPTY:
        if (states.port2Need) {
            ok = false;
            text += redBold(trUtf8("Nincs megadva a másodlagos port neve."));
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        text += redBold(trUtf8("A megadott a másodlagos port név nem lehet azonos az elsődlegessel."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.mac2) {
    case IS_EMPTY:
        if (states.mac2need) {
            text += trUtf8("Nincs megadva a munkaállomás második interfészének a MAC-je.") + br;
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        text += redBold(trUtf8("A munkaállomás második interfészének a MAC-je ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        text += redBold(trUtf8("A munkaállomás második interfészének a MAC-je hibás."));
        break;
    }
    pUi->textEditMsg->setText(text);
    pUi->pushButtonSave->setEnabled(ok);
}

void cWorkstation::parseObject()
{
    withinSlot++;
    pUi->radioButtonMod->setEnabled(true);  // Van minta, elvileg lehet modosítani.
    switch (pWorkstation->ports.size()) {
    case 0:   // Nincs port
        pWorkstation->ports   << (pInterface = new cInterface);
        pInterface->setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
        pInterface->addresses << (pIpAddress = new cIpAddress);
        pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
        break;
    case 1:
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
    case 2:
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

    if (pPort2 == NULL) {
        pUi->comboBoxPType_2->setCurrentIndex(0);   // nincs port
    }
    else {
        s = pPort2->ifType().getName();
        ix = pUi->comboBoxPType_2->findText(s);
        if (ix < 0) ix = 1;   // a 0. a nincs, az második (ix=1) az alapértelmezett!!
        pUi->comboBoxPType_2->setCurrentIndex(ix);
        s = pUi->comboBoxPType_2->currentText();
        pPort2->setId(cIfType::ifTypeId(s));
    }
    portTypeCurrentIndex2(0);

    withinSlot--;
    setMessage();
}

/**********************   SLOTS   ************************/
void cWorkstation::togleNewMod(bool f)
{
    if (lockSlot) return;
    states.modify = f;
    if (!f && pSample->isNull()) {  // Meg kell adni, mit is modosítunk, anélkül nem OK
        EXCEPTION(EPROGFAIL);
    }
    // Pár státuszt újra kell számolni! (Ami függ a states.modify értékétől)
    ++withinSlot;
    nodeNameChanged(pWorkstation->getName());
    serialChanged(pWorkstation->getName(_sSerialNumber));
    inventoryChanged(pWorkstation->getName(_sInventoryNumber));
    macAddressChanged(pInterface->getName(_sHwAddress));
    ipAddressChanged(pIpAddress->getName(cIpAddress::ixAddress()));
    --withinSlot;
    setMessage();
}

void cWorkstation::save()
{
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
}

void cWorkstation::refresh()
{
    // MAJD
}

void cWorkstation::filterZoneCurrentIndex(const QString& s)
{
    const QStringList& pl = placesInZone(s);    // Helyek a zónában
    QString placeName = pUi->comboBoxFilterPlace->currentText();
    int i = pl.indexOf(placeName);      // Az aktuális hely indexe az új listában
    if (i < 0) i = 0;       // Ha nincs az aktuális helyiség a kiválasztott zónában
    pUi->comboBoxFilterPlace->clear();
    pUi->comboBoxFilterPlace->addItems(pl);
    pUi->comboBoxFilterPlace->setCurrentIndex(i);
    filterPlaceCurrentIndex(pl.at(i));
}

void cWorkstation::filterPlaceCurrentIndex(const QString& s)
{
    QString patt = pUi->lineEditFilterPattern->text();
    placeCurrentIndex(s, pUi->comboBoxFilterZone, pUi->comboBoxNode, pModelNode, patt);
}

void cWorkstation::filterPatternChanged(const QString& s)
{
    QString place_name = pUi->comboBoxFilterPlace->currentText();
    placeCurrentIndex(place_name, pUi->comboBoxFilterZone, pUi->comboBoxNode, pModelNode, s);
}

// Workstation (node) slots
void cWorkstation::nodeCurrentIndex(int i)
{
    if (lockSlot) return;
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
    pPlace->setById(pWorkstation->getId(_sPlaceId));
    parseObject();
}

void cWorkstation::nodeNameChanged(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    pWorkstation->setName(s);
    states.nodeName = checkWrkCollision(pWorkstation->nameIndex(), s);
    // withinSlot--;
    setMessage();
}

void cWorkstation::serialChanged(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    int ix = pWorkstation->toIndex(_sSerialNumber);
    pWorkstation->setName(ix, s);
    states.serialNumber = checkWrkCollision(ix, s);
    // withinSlot--;
    setMessage();
}

void cWorkstation::inventoryChanged(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    int ix = pWorkstation->toIndex(_sInventoryNumber);
    pWorkstation->setName(ix, s);
    states.inventoraNumber = checkWrkCollision(ix, s);
    // withinSlot--;
    setMessage();
}

void cWorkstation::zoneCurrentIndex(const QString& s)
{
    if (lockSlot) return;
    withinSlot++;
    const QStringList& pl = placesInZone(s);    // Helyek a zónában
    int i = pl.indexOf(pPlace->getName());      // Az aktuális hely indexe az új listában
    if (i < 0) i = 0;       // Ha nincs az aktuális helyiség a kiválasztott zónában
    lockSlot = true;        // A slot-ot majd a végén meghívjuk, ha megvan a végleges index
    /* lockSlot */ pUi->comboBoxPlace->clear();
    /* lockSlot */ pUi->comboBoxPlace->addItems(pl);
    /* lockSlot */ pUi->comboBoxPlace->setCurrentIndex(i);
    lockSlot = false;
    placeCurrentIndex(pl.at(i));
    withinSlot--;
    setMessage();
}

void cWorkstation::placeCurrentIndex(const QString& s)
{
    if (lockSlot) return;
    withinSlot++;
    if (s.isEmpty()) {   // Nincs kiválasztva hely ?
        pPlace->clear();
        states.nodePlace = IS_EMPTY;
    }
    else {
        pPlace->setByName(*pq, s);
        states.nodePlace = IS_OK;
    }
    pWorkstation->setId(_sPlaceId, pPlace->getId());
    if (pUi->checkBoxPlaceEqu->isChecked())   linkToglePlaceEqu(true, true);    // Elsődleges
    if (pUi->checkBoxPlaceEqu_2->isChecked()) linkToglePlaceEqu(true, false);   // Mésodlagos
    withinSlot--;
    setMessage();
}
// Workstation ports slots
void cWorkstation::portNameChanged(const QString& s)
{
    if (lockSlot) return;
    withinSlot++;
    pInterface->setName(s);
    states.portName = s.isEmpty() ? IS_EMPTY : IS_OK;
    withinSlot--;
    setMessage();
}

void cWorkstation::portTypeCurrentIndex(const QString& s)
{
    if (lockSlot) return;
    withinSlot++;
    const cIfType& iftype = cIfType::ifType(s);
    pInterface->setId(_sIfTypeId, iftype.getId());
    states.linkPossible = iftype.getName(_sIfTypeLinkType) == _sPTP;
    pUi->comboBoxLinkZone->setEnabled(states.linkPossible);
    pUi->comboBoxLinkPlace->setEnabled(states.linkPossible);
    pUi->checkBoxPlaceEqu->setEnabled(states.linkPossible);
    pUi->comboBoxLinkNode->setEnabled(states.linkPossible);
    pUi->comboBoxLinkPort->setEnabled(states.linkPossible);
    pUi->lineEditLinkNodeType->setEnabled(states.linkPossible);
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
    withinSlot--;
    setMessage();
}

void cWorkstation::macAddressChanged(const QString& s)
{
    if (lockSlot) return;
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
}

// Workstation address slots
void cWorkstation::ipAddressChanged(const QString& s)
{
    if (lockSlot) return;
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
                QAbstractSocket::NetworkLayerProtocol prot = a.protocol();
                switch (prot) {
                case QAbstractSocket::IPv4Protocol: // A Qt engedékenyebb a cím megadásnál mint szeretném
                    static const QRegExp pattern("\\d+\\.\\d+\\.\\d+\\.\\d+");
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
                }
                else {
                    cSubNet sn;
                    int ix = sn.getByAddress(*pq, a);
                    switch (ix) {  // Hány subnet-re illeszkedik a cím
                    case 0: // Nincs hozzá subnet
                        states.ipAddr = IS_EXTERNAL;
                        break;
                    case 1: // 1 subnet, OK, belső cím
                        ix = pUi->comboBoxSubNet->findText(sn.getName());
                        if (ix < 0) EXCEPTION(EDATA);
                        states.ipAddr = IS_OK;
                        pIpAddress->setAddress(a);
                        pUi->comboBoxSubNet->setCurrentIndex(ix);   // emit subNetCurrentIndex();
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
    bool f = states.ipAddr == IS_OK;    // Ha van IP cím, nem változtatható a subnet és a VLAN (ITT!! feltételezzük, hogy nincs átfedés a véan-ok között)
    pUi->comboBoxSubNet->setDisabled(f);
    pUi->comboBoxSubNetAddr->setDisabled(f);
    pUi->comboBoxVLan->setDisabled(f);
    pUi->comboBoxVLanId->setDisabled(f);
    // withinSlot--;
    setMessage();
}

void cWorkstation::ipAddressTypeCurrentIndex(const QString& s)
{
    if (lockSlot) return;
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
}

void cWorkstation::subNetCurrentIndex(int i)
{
    if (lockSlot) return;
    // withinSlot++;
    lockSlot = true;
    /* lockSlot */ pUi->comboBoxSubNetAddr->setCurrentIndex(i);
    lockSlot = false;
    int ix = 0;
    if ((i == 0)) {
        states.subNet =  IS_EMPTY;
        pIpAddress->clear(_sSubNetId);
    }
    else {
        states.subNet = IS_OK;
        ix = i - 1;     // Első elem a comboBox-ban egy öres string
        cSubNet& sn = *subnets[ix];
        qlonglong vlid = sn.getId(_sVlanId);
        if (vlid != NULL_ID) {
            cVLan vl;
            vl.setById(*pq, vlid);
            ix = pUi->comboBoxVLan->findText(vl.getName());
            if (ix < 0) EXCEPTION(EDATA);
        }
        pIpAddress->setId(_sSubNetId, sn.getId());
    }
    // withinSlot++;
    lockSlot = true;
    /* lockSlot */ pUi->comboBoxVLan->setCurrentIndex(ix);
    /* lockSlot */ pUi->comboBoxVLanId->setCurrentIndex(ix);
    lockSlot = false;
    // withinSlot--;
    setMessage();
}

void cWorkstation::subNetAddrCurrentIndex(int i)
{
    if (lockSlot) return;
    // withinSlot++;
    pUi->comboBoxSubNet->setCurrentIndex(i);
    // withinSlot--;
}

void cWorkstation::vLanCurrentIndex(int i)
{
    if (lockSlot) return;
    withinSlot++;
    int ix = 0;
    if (i != 0) {
        ix = i - 1;
        cVLan&   vl = *vlans[ix];
        cSubNet sn;
        ix = sn.toIndex(_sVlanId);
        sn.setId(ix, vl.getId());
        if (sn.fetch(*pq, false, _bit(ix))) {   // Elvileg lehet tobb találat, de most nem foglakozunk vele!!!!
            ix = pUi->comboBoxSubNet->findText(sn.getName());
            if (ix < 0) EXCEPTION(EDATA);
        }
        else {
            ix = 0;
        }
    }
    pUi->comboBoxSubNet->setCurrentIndex(ix);
    withinSlot--;
    setMessage();
}

void cWorkstation::vLanIdCurrentIndex(int i)
{
    if (lockSlot) return;
    // withinSlot++;
    pUi->comboBoxVLan->setCurrentIndex(i);
    // withinSlot--;
}

// LINK 1 slots
void cWorkstation::linkChangeLinkType(int id, bool f)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkToglePlaceEqu(bool f)
{
    linkToglePlaceEqu(f, true);
}

void cWorkstation::linkZoneCurrentIndex(const QString &s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkPlaceCurrentIndex(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkNodeCurrentIndex(int i)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkPortCurrentIndex(int i)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::linkPortShareCurrentIndex(const QString &s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

// Workstation port 2 slots
void cWorkstation::portNameChanged2(const QString& s)
{
    if (lockSlot) return;
    // withinSlot++;
    // withinSlot--;
    setMessage();
}

void cWorkstation::portTypeCurrentIndex2(int i)
{
    if (lockSlot) return;
    withinSlot++;
    setMessage();
    if (i == 0) {   // Nincs második port (vagy interface)
        pUi->lineEditPName_2->setDisabled(true);
        pUi->lineEditPNote_2->setDisabled(true);
        pUi->lineEditPTag_2->setDisabled(true);
        pUi->lineEditPMAC_2->setDisabled(true);

        linkToglePlaceEqu(true, false);

        pUi->comboBoxLinkNode_2->setDisabled(true);
        pUi->lineEditLinkNodeType_2->setDisabled(true);
        pUi->comboBoxLinkPort_2->setDisabled(true);
        pUi->comboBoxLinkPortShare_2->setDisabled(true);
        pUi->lineEditLinkNote_2->setDisabled(true);
        pUi->radioButtonLinkBack_2->setDisabled(true);
        pUi->radioButtonLinkFront_2->setDisabled(true);
        pUi->radioButtonLinkTerm_2->setDisabled(true);
        pUi->pushButtonLinkOther_2->setDisabled(true);

        pUi->checkBoxSrvEna_2->setDisabled(true);
        pUi->lineEditSrvNote_2->setDisabled(true);
        pUi->checkBoxSrvDel_2->setDisabled(true);

        pUi->checkBoxSrvIns_2->setDisabled(true);
        switch (pWorkstation->ports.size()) {
        case 1: break;
        case 2: delete pWorkstation->ports.pop_back(); break;
        default:    EXCEPTION(EPROGFAIL);
        }
        pPort2 = NULL;
    }
    else {
        // Interfész típus név
        QString sType = pUi->comboBoxPType_2->currentText();
        const cIfType& ifType = cIfType::ifType(sType); // típus rekord
        const QString portobjtype = ifType.getName(_sIfTypeObjType);
        if (pPort2 == NULL) {
            pPort2 = cNPort::newPortObj(ifType);        // létre kell hozni a másodlagos portot
            pPort2->setName(ifType.getName());
        }
        else {  // volt másodlagos port
            // típus ellenörzés
            qlonglong toid = pPort2->tableoid();
            if      (portobjtype == _sNPort     && toid == cNPort::tableoid_nports()) {
                ;   // Azonost típus (nports) OK
            }
            else if (portobjtype == _sNPort     && toid == cNPort::tableoid_interfaces()) {
                cNPort *p = new cNPort();
                p->copy(*pPort2);
                p->setId(_sIfTypeId, ifType.getId());
                pPort2 = p;
            }
            else if (portobjtype == _sInterface && toid == cNPort::tableoid_interfaces()) {
                 ;  // Azonosz típus (interfész) OK
            }
            else if (portobjtype == _sInterface && toid == cNPort::tableoid_nports()) {
                cInterface *p = new cInterface();
                p->copy(*pPort2);
                p->setId(_sIfTypeId, ifType.getId());
                pPort2 = p;
            }
            else EXCEPTION(EDATA);  // ez nem lehetséges
            switch (pWorkstation->ports.size()) {
            case 2:
                if (pPort2 != pWorkstation->ports.at(1)) {
                    delete  pWorkstation->ports.pop_back();
                    pWorkstation->ports << pPort2;
                }
                break;
            case 1:
                pWorkstation->ports << pPort2;
                break;
            default:
                EXCEPTION(EPROGFAIL);
            }
        }
        pUi->lineEditPName_2->setDisabled(false);
        pUi->lineEditPNote_2->setDisabled(false);
        pUi->lineEditPTag_2->setDisabled(false);

        pUi->comboBoxLinkZone_2->setDisabled(false);
        pUi->comboBoxLinkZone_2->setDisabled(false);

        pUi->comboBoxLinkNode_2->setDisabled(false);
        pUi->lineEditLinkNodeType_2->setDisabled(false);
        pUi->comboBoxLinkPort_2->setDisabled(false);
        pUi->comboBoxLinkPortShare_2->setDisabled(false);
        pUi->lineEditLinkNote_2->setDisabled(false);
        pUi->radioButtonLinkBack_2->setDisabled(false);
        pUi->radioButtonLinkFront_2->setDisabled(false);
        pUi->radioButtonLinkTerm_2->setDisabled(false);
        pUi->pushButtonLinkOther_2->setDisabled(false);

        pUi->checkBoxSrvEna_2->setDisabled(false);
        pUi->lineEditSrvNote_2->setDisabled(false);
        pUi->checkBoxSrvDel_2->setDisabled(false);

        pUi->checkBoxSrvIns_2->setDisabled(false);

        QString s;

        s = pPort2->getName();
        pUi->lineEditPName_2->setText(s);
        portNameChanged2(s);
        pUi->lineEditPNote_2->setText(pPort2->getNote());
        pUi->lineEditPTag_2->setText(pPort2->getName(_sPortTag));
        if (portobjtype == _sInterface) {
            s = pPort2->getMac(_sHwAddress).isValid() ? pPort2->getName(_sHwAddress) : _sNul;
            pUi->lineEditPMAC_2->setDisabled(false);
        }
        else {
            s = _sNul;
            pUi->lineEditPMAC_2->setDisabled(true);
        }
        pUi->lineEditPMAC_2->setText(s);
        macAddressChanged2(s);
    }
    withinSlot--;
    setMessage();
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
    linkToglePlaceEqu(f, true);
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
