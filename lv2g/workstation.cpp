#include "lv2widgets.h"
#include "srvdata.h"
#include "lv2link.h"
#include "record_dialog.h"
#include "workstation.h"
#include "ui_wstform.h"
#include "ui_phslinkform.h"

static QString tableLine(const QStringList& fl, const QString& ft)
{
    QString r = "<tr>\n";
    foreach (QString f, fl) {
        r += "\t<" + ft + "> ";
        r += f;
        r += " </" + ft + ">\n";
    }
    r += "</tr>\n";
    return r;
}

/// \brief linkCollision
/// \return A link statusz (IS_OK vagy IS_COLLISION)
/// @param stat link status
/// @param pid  saját port ID
/// @param lpid Linkelt port ID
/// @param type Fizikai link típusa
/// @param sh   Megosztás
/// @param msg  Üzenet string referencia
static int _linkCollision(QSqlQuery& q, int stat, qlonglong pid, qlonglong lpid, ePhsLinkType type, ePortShare sh, QString& msg)
{
    DBGFN();
    msg.clear();
    if (stat != IS_OK && stat != IS_COLLISION) return stat;
    cPhsLink link;
    tRecordList<cPhsLink> list;
    int r;
    if (link.collisions(q, list, lpid, type, sh)
     && (list.size() != 1 && list.first()->getId(_sPortId1) != pid)) {
        cPhsLink *pL;
        cNode  n;
        cNPort p;
        msg += QObject::trUtf8("Az elsődleges porthoz megadott link a következő link(ek)el ütközik:");
        msg += "\n<table> ";
        QStringList head;
        head << QObject::trUtf8("Cél host")
             << QObject::trUtf8("Port")
             << QObject::trUtf8("Típus")
             << QObject::trUtf8("Megosztás");
        msg += tableLine(head, "th");
        while (NULL != (pL = list.pop_back(EX_IGNORE))) {
            if (pL->getId(_sPortId1) == pid) continue;
            p.setById(q, pL->getId(__sPortId2));
            QStringList col;
            col << n.getNameById(p.getId(_sNodeId))
                << p.getName()
                << pL->getName(_sPhsLinkType2)
                << pL->getName(_sPortShared);
            msg += tableLine(col, "td");
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

/// A megadott radio button állapotának a beállítása
/// @param pGroup A QButtonGroup objektum pointere
/// @param id A kiválasztott radio button ID-je a group-ban
/// @param check Az állpot (true: checked)
/// @param enable Az engedályezése (true: engedélyezve)
static void buttonCheckEnable(QButtonGroup * pGroup, int id, bool check, bool enable)
{
    QAbstractButton *pButton = pGroup->button(id);
    pButton->setEnabled(enable);
    pButton->setCheckable(enable || check);
    pButton->setChecked(check);
}

static void buttonSets(QAbstractButton *p, bool e, bool c)
{
    p->setEnabled(e);
    p->setChecked(c);
}

static inline QString warning(const QString& text) {
    return "<div><b>" + text + "</b></div>";
}
static inline QString info(const QString& text) {
    return "<div>" + text + "</div>";
}
static inline QString error(const QString& text) {
    return "<div style=\"color:red\"><b>" + text + "</b></div>";
}
static inline QString italic(const QString& text) {
    return "<i>" + text + "</i>";
}


/* ************************************************************************************************* */

cWorkstation::cWorkstation(QWidget *parent) :
    cOwnTab(parent),
    pUi(new Ui::wstWidget),
    pq(newQuery()),
    pSample(new cNode),
    node(),
    pModelNode(new cRecordListModel(node.descr())),
    pPort1(new cInterface),
    pIpAddress(new cIpAddress),
    pPlace(new cPlace),
    pPort2(NULL),                                   // Ez lehet cNPort vagy cInterface objektum is.
    pPassiveButtons(new QButtonGroup(this)),
    pModifyButtons(new QButtonGroup(this)),
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
    states.macNeed = true;      // Alapértelmezetten MAC is kel az első port (interfész)-re
    pUi->setupUi(this);
    QList<int> sizes;
    sizes << 300 << 300 << 300 << 300;
    pUi->splitterWorkstation->setSizes(sizes);
    pUi->splitterLink->setSizes(sizes);
    pUi->splitterMsg->setSizes(sizes);

    pPassiveButtons->addButton(pUi->radioButtonWorkstation, 0);
    pPassiveButtons->addButton(pUi->radioButtonPassive, 1);
    pModifyButtons->addButton(pUi->radioButtonNew, 0);
    pModifyButtons->addButton(pUi->radioButtonMod, 1);
    node.setId(_sNodeType, NT_WORKSTATION);
    pPort1->    setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
    pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
    pPort1->reconvert<cInterface>()->addresses << pIpAddress;
    node.ports   << pPort1;
    pPlace->setById(UNKNOWN_PLACE_ID);     // NAME = 'unknown'

    _initLists();
    pUi->comboBoxNode->setModel(pModelNode);
    nodeListSql[0] = // Workstation: Konstans szűrő a nodes táblára, SQL kifejezés:
            // Típusa 'workstation' (is)
            quoted(_sWorkstation) + " = ANY " + parentheses(_sNodeType)
            // Legfeljebb 2 portja van
            + " AND 2 >= (SELECT COUNT(*) FROM nports "                             " WHERE node_id = nodes.node_id)"
            // Legfeljebb 1 ip címe van
            + " AND 1 >= (SELECT COUNT(*) FROM ip_addresses JOIN nports USING(port_id) WHERE node_id = nodes.node_id)";
    nodeListSql[1] = // Passive: Konstans szűrő a nodes táblára, SQL kifejezés:
            // Típusa 'node' (is)
            quoted(_sNode) + " = ANY " + parentheses(_sNodeType)
            // Legfeljebb 2 portja van
            + " AND 2 >= (SELECT COUNT(*) FROM nports "                             " WHERE node_id = nodes.node_id)"
            // Nincs ip címe
            + " AND 0 = (SELECT COUNT(*) FROM ip_addresses JOIN nports USING(port_id) WHERE node_id = nodes.node_id)";
    pModelNode->setConstFilter(nodeListSql[0], FT_SQL_WHERE);
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

    pUi->comboBoxPType->addItems(portTypeList[0]);
    pUi->comboBoxPType->setCurrentIndex(0);
    pUi->comboBoxVLan->addItem(_sNul);
    pUi->comboBoxVLan->addItems(vlannamelist);
    pUi->comboBoxVLan->setCurrentIndex(0);
    pUi->comboBoxVLanId->addItem(_sNul);
    pUi->comboBoxVLanId->addItems(vlanidlist);
    pUi->comboBoxVLanId->setCurrentIndex(0);
    pUi->comboBoxSubNet->addItem(_sNul);
    pUi->comboBoxSubNet->addItems(subnetnamelist);
    pUi->comboBoxSubNet->setCurrentIndex(0);
    pUi->comboBoxSubNetAddr->addItem(_sNul);
    pUi->comboBoxSubNetAddr->addItems(subnetaddrlist);
    pUi->comboBoxSubNetAddr->setCurrentIndex(0);

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
    pModelLinkPort->sFkeyName = _sNodeId;
    pUi->comboBoxLinkPort->setModel(pModelLinkPort);
    pModelLinkPort->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres
    portType2No = trUtf8("Nincs másodlagos port");
    pUi->comboBoxPType_2->addItem(portType2No);
    pUi->comboBoxPType_2->addItems(portTypeList[1]);
    pUi->comboBoxPType_2->setCurrentIndex(0);
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

    // pModelLinkPort2->nullable = false;
    pModelLinkPort2->sFkeyName = _sNodeId;
    pUi->comboBoxLinkPort_2->setModel(pModelLinkPort2);
    pModelLinkPort2->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres

    connect(pModifyButtons,             SIGNAL(buttonToggled(int,bool)),      this, SLOT(togleModify(int, bool)));
    connect(pPassiveButtons,            SIGNAL(buttonToggled(int,bool)),      this, SLOT(toglePassive(int,bool)));
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
    connect(pUi->toolButtonAddPlace,    SIGNAL(pressed()),                    this, SLOT(nodeAddPlace()));
    connect(pUi->lineEditPName,         SIGNAL(textChanged(QString)),         this, SLOT(portNameChanged(QString)));
    connect(pUi->comboBoxPType,         SIGNAL(currentIndexChanged(int)),     this, SLOT(portTypeCurrentIndex(int)));
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
    connect(pUi->comboBoxLinkNode,      SIGNAL(currentIndexChanged(QString)), this, SLOT(linkNodeCurrentIndex(QString)));
    connect(pUi->comboBoxLinkPort,      SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPortCurrentIndex(QString)));
    connect(pUi->comboBoxLinkPortShare, SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPortShareCurrentIndex(QString)));
    connect(pLinkButtonsLinkType,       SIGNAL(buttonToggled(int,bool)),      this, SLOT(linkChangeLinkType(int, bool)));
    connect(pUi->pushButtonNewPatch,    SIGNAL(pressed()),                    this, SLOT(newPatch()));

    // Link2:
    connect(pUi->checkBoxPlaceEqu_2,    SIGNAL(toggled(bool)),                this, SLOT(linkToglePlaceEqu2(bool)));
    connect(pUi->comboBoxLinkZone_2,    SIGNAL(currentIndexChanged(QString)), this, SLOT(linkZoneCurrentIndex2(QString)));
    connect(pUi->comboBoxLinkPlace_2,   SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPlaceCurrentIndex2(QString)));
    connect(pUi->comboBoxLinkNode_2,    SIGNAL(currentIndexChanged(QString)), this, SLOT(linkNodeCurrentIndex2(QString)));
    connect(pUi->comboBoxLinkPort_2,    SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPortCurrentIndex2(QString)));
    connect(pUi->comboBoxLinkPortShare_2,SIGNAL(currentIndexChanged(QString)),this, SLOT(linkPortShareCurrentIndex2(QString)));
    connect(pLinkButtonsLinkType2,      SIGNAL(buttonToggled(int,bool)),      this, SLOT(linkChangeLinkType2(int, bool)));
    connect(pUi->pushButtonNewPatch_2,  SIGNAL(pressed()),                    this, SLOT(newPatch2()));

    lockSlot = false;
    // sync..
    withinSlot = 1;
    _changePortType(true, 0);
    ipAddressTypeCurrentIndex(pUi->comboBoxIpType->currentText());
    _changePortType(false, 0);
    withinSlot = 0;
    _setMessage();
}

cWorkstation::~cWorkstation()
{
    lockSlot = true;
    delete pq;
    delete pSample;
    delete pPlace;
    delete pLinkNode;
    delete pLinkPort;
    delete pLinkNode2;
    delete pLinkPort2;
}

#define _LOCKSLOTS()    _lockSlotSave_ = lockSlot; lockSlot = true
#define LOCKSLOTS()     bool _LOCKSLOTS()
#define UNLOCKSLOTS()   lockSlot = _lockSlotSave_
#define WENABLE(wn, f) pUi->wn->setEnabled(f);

void cWorkstation::_initLists()
{
    // Port típusok:
    portTypeList[0].clear();
    portTypeList[1].clear();
    portTypeList[0] << _sEthernet << _sWireless << _sADSL << _sVirtual;         // Workstation first interface
    portTypeList[1] << _sAttach << _sEthernet << _sUSB << _sRS232 << _sRS485;   // Passive node first interface, Workstation second interface
    zones = cPlaceGroup::getAllZones(*pq);
    // Az "all" zónába tartozó, vagyis az összes hely listája
    cPlace pl;
    mapZones.clear();
    placeIdList.clear();
    _placesInZone(_sAll);
    // VLANS:
    vlans.clear();
    vlannamelist.clear();
    vlanidlist.clear();
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
    subnets.clear();
    subnetnamelist.clear();
    subnetaddrlist.clear();
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

void cWorkstation::_fullRefresh()
{
    _initLists();   // A memóriában lévő listák frissítése
    _refreshPlaces(pUi->comboBoxFilterZone, pUi->comboBoxFilterPlace);
    pUi->comboBoxNode->setCurrentIndex(0);
    pModelNode->setFilter();
    _refreshPlaces(pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace);
    _refreshPlaces(pUi->comboBoxLinkZone_2, pUi->comboBoxLinkPlace_2);
    _refreshPlaces(pUi->comboBoxZone, pUi->comboBoxPlace);
    _linkToglePlaceEqu(true);
    _linkToglePlaceEqu(false);
    LOCKSLOTS();
    pUi->comboBoxPType->addItems(portTypeList[0]);
    pUi->comboBoxPType->setCurrentIndex(0);
    pUi->comboBoxVLan->addItem(_sNul);
    pUi->comboBoxVLan->addItems(vlannamelist);
    pUi->comboBoxVLan->setCurrentIndex(0);
    pUi->comboBoxVLanId->addItem(_sNul);
    pUi->comboBoxVLanId->addItems(vlanidlist);
    pUi->comboBoxVLanId->setCurrentIndex(0);
    pUi->comboBoxSubNet->addItem(_sNul);
    pUi->comboBoxSubNet->addItems(subnetnamelist);
    pUi->comboBoxSubNet->setCurrentIndex(0);
    pUi->comboBoxSubNetAddr->addItem(_sNul);
    pUi->comboBoxSubNetAddr->addItems(subnetaddrlist);
    pUi->comboBoxSubNetAddr->setCurrentIndex(0);
    UNLOCKSLOTS();

    QString sSampleName = pSample->getName();
    int ix = _setCurrentIndex(sSampleName, pUi->comboBoxNode, EX_IGNORE);
    if (ix == 0) _clearNode();
    else {
        node.clear();   // Biztos külömbözzön az ID !!
        nodeCurrentIndex(ix);
    }
}

void cWorkstation::_refreshPlaces(QComboBox *pComboBoxZone, QComboBox *pComboBoxPlace)
{
    QString sZone  = pComboBoxZone->currentText();
    QString sPlace = pComboBoxPlace->currentText();
    LOCKSLOTS();
    pComboBoxZone->clear();
    pComboBoxZone->addItems(zones);
    _setCurrentIndex(sZone, pComboBoxZone, EX_IGNORE);
    sZone = pComboBoxZone->currentText();

    pComboBoxPlace->clear();
    pComboBoxPlace->addItems(_placesInZone(sZone));
    _setCurrentIndex(sPlace, pComboBoxPlace, EX_IGNORE);
    UNLOCKSLOTS();
}

const QStringList& cWorkstation::_placesInZone(const QString& sZone)
{
    QStringList& places = mapZones[sZone];
    if (places.isEmpty()) {
        if (sZone == _sAll) {
            placeIdList.clear();
            // Az összes hely, helyiség, betűrendben, de NULL, unknown az első, és a root kimarad
            places     << _sNul;
            placeIdList << UNKNOWN_PLACE_ID;
            cPlace pl;
            if (!pl.fetch(*pq, false, QBitArray(1, false), pl.iTab(_sPlaceName))) {
                EXCEPTION(EDATA, 0, "No any place.");
            }
            do {
                QString name = pl.getName();
                if (name != _sUnknown && name != _sRoot) {
                    places      << name;
                    placeIdList << pl.getId();
                }
            } while (pl.next(*pq));
        }
        else {
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
    }
    return places;
}

int cWorkstation::_checkNodeCollision(int ix, const QString& s)
{
    if (s.isEmpty()) return IS_EMPTY;
    if (s == pSample->getName(ix)) {
        return states.modify ? IS_OK : IS_COLLISION;
    }
    cPatch *pO;
    bool r;
    if (node.nameIndex() == ix) pO = new cPatch;   // A név mezőnel az őssel sem ütközhet
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

QString cWorkstation::_changeZone(QComboBox *pComboBoxPlace, const QString& sZone, const QString &_placeName)
{
    const QStringList& pl = _placesInZone(sZone);    // Helyek a zónában
    QString placeName = _placeName.isNull() ? pComboBoxPlace->currentText() : _placeName;
    int i = pl.indexOf(placeName);      // Az aktuális hely indexe az új listában
    if (i < 0) i = 0;                   // Ha nincs az aktuális helyiség a kiválasztott zónában
    LOCKSLOTS();
    pComboBoxPlace->clear();
    pComboBoxPlace->addItems(pl);
    pComboBoxPlace->setCurrentIndex(i);
    UNLOCKSLOTS();
    return pl.at(i);
}

void cWorkstation::_setPlace(const QString& placeName, QComboBox *pComboBoxPlace, QComboBox *pComboBoxZone)
{
    if (placeName.isEmpty()) {
        pComboBoxPlace->setCurrentIndex(0);
    }
    else {
        int ix = _setCurrentIndex(placeName, pComboBoxPlace, EX_IGNORE);
        if (ix < 1) {
            _setCurrentIndex(_sAll, pComboBoxZone);
            QString s = _changeZone(pComboBoxPlace, _sAll, placeName);
            if (placeName != s) EXCEPTION(EDATA);
        }
    }
}

int cWorkstation::_setCurrentIndex(const QString& name, QComboBox * pComboBox, eEx __ex)
{
    int ix = pComboBox->findText(name);
    if (ix < 0) {
        if (EX_IGNORE != __ex) EXCEPTION(EDATA, 0, name);
        ix = 0;
    }
    pComboBox->setCurrentIndex(ix);
    return ix;
}

int cWorkstation::_changePlace(const QString& placeNme, const QString& zoneName,
                               QComboBox *pComboBoxNode, cRecordListModel *pModel,
                               const QString& _patt)
{
    QString expr;
    QString actNode = pComboBoxNode->currentText();     // Az aktuálisan kiválasztott node neve
    if (placeNme.isEmpty()) {          // Nincs kiválasztva hely ?
        // Ha nincs kiválasztott hely, akkor az aktuális zóna szerint szűrünk
        if (zoneName == _sAll) {
            expr = _sNul;   // ALL zóna: nincs további szűrés
        }
        else {
            // Csak a zónában lévő node-ok
            qlonglong zid = cPlaceGroup().getIdByName(*pq, zoneName);
            // Azok a node-ok, melyeknek helye, vagy a hely valamelyik parentje benne van az aktuális zónában
            expr = QString("is_group_place(place_id, %1)").arg(zid);
        }
    }
    else {
        qlonglong pid = cPlace().getIdByName(*pq, placeNme);
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
    LOCKSLOTS();
    pModel->setFilter(expr);
    int ix = _setCurrentIndex(actNode, pComboBoxNode, EX_IGNORE);
    UNLOCKSLOTS();
    return ix;
}

int cWorkstation::_changeLinkNode(QString nodeName, cRecordListModel *pModelPort, QComboBox * pComboBoxPort,
                                        cPatch *pNode, cRecordAny *pPort,
                                        ePhsLinkType  *pLinkType, ePortShare *pLinkShared)
{
    int r;
    LOCKSLOTS();
    if (nodeName.isEmpty()) {               // Nincs kiválasztva node
        if (pNode       != NULL) pNode->clear();
        if (pPort       != NULL) pPort->clear();
        if (pLinkShared != NULL) *pLinkShared = ES_NC;
        if (pLinkType   != NULL) *pLinkType = LT_INVALID;
        pModelPort->setFilter(QVariant());   // Nincs node, nincs port
        pComboBoxPort->setDisabled(true);
        r = IS_EMPTY;
    }
    else {
        if (pNode == NULL || pNode->getName() != nodeName) {
            if (pNode != NULL) pNode->setByName(*pq, nodeName);
            pModelPort->setFilter(pLinkNode->getId());
            pComboBoxPort->setEnabled(true);
            pComboBoxPort->setCurrentIndex(0);
            if (pPort != NULL) pPort->clear();
            if (pLinkShared != NULL) *pLinkShared = ES_NC;
            if (pLinkType   != NULL) *pLinkType = LT_INVALID;
        }
        r =  IS_IMPERFECT;
    }
    UNLOCKSLOTS();
    return r;
}

int cWorkstation::_changeLinkPort(const QString& s,
                                        QButtonGroup * pLinkType, QComboBox *pComboBoxShare, eTristate _isPatch,
                                        cRecordAny *pPort, ePhsLinkType *pType, ePortShare *pShare)
{
    int r = IS_OK;
    LOCKSLOTS();    // !!
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
    UNLOCKSLOTS();
    return r;
}

void cWorkstation::_linkToglePlaceEqu(bool primary, eTristate __f)
{
    bool f;
    switch (__f) {
    case TS_TRUE:   f = true;   break;
    case TS_FALSE:  f = false;  break;
    case TS_NULL:
        if (primary) {
            if (linkType != LT_INVALID) return;
            f = pUi->checkBoxPlaceEqu->isChecked();
        }
        else {
            if (linkType2 != LT_INVALID) return;
            f = pUi->checkBoxPlaceEqu_2->isChecked();
        }
        break;
    }
    withinSlot++;
    LOCKSLOTS();
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
        QString sPlace  = pPlace->isValid() ? pPlace->getName() : _sNul; // És az aktuálisan megadott hely neve, vagy üres string
        int     ixPlace = mapZones[sZone].indexOf(sPlace);  // Az aktuális hely indexe az aktuális zónában (az üres a nulladik)
        if (ixPlace < 0 || ixZone < 0) EXCEPTION(EPROGFAIL);// Ha nincs találat akkor elkutyultunk valamit
        if (sZone  != pComboBoxZone->currentText()
         || sPlace != pComboBoxPlace->currentText()) {  // Ha nincs változás, akkor nem kell tenni semmit
            pComboBoxZone->setCurrentIndex(ixZone);     // A zónát csak ki kell választani
            pComboBoxPlace->clear();                    // Frissítjük a listát
            pComboBoxPlace->addItems(_placesInZone(sZone));
            pComboBoxPlace->setCurrentIndex(ixPlace);   // Béállítjuk a helyet is
            int ix = _changePlace(sPlace, sZone, pComboBoxNode, pModelNode);   // SLOT tiltva volt
            QString s = pComboBoxNode->itemText(ix);
            int st = _changeLinkNode(s, pModelPort, pComboBoxPort, pNode, pPort, pLinkType, pShare);
            if (primary) states.link  = st;
            else         states.link2 = st;
        }
    }
    // Ha nem engedélyezhető, akkor a checkbox is tiltva, ha más hívja f mindíg true!
    pComboBoxZone->setDisabled(f);
    pComboBoxPlace->setDisabled(f);
    UNLOCKSLOTS();
    withinSlot--;
    _setMessage();
}

void cWorkstation::_setLinkType(bool primary)
{
    int        type;
    QButtonGroup *      pButtons;
    QComboBox *         pComboBoxShare;
    ePortShare        * pLinkShared;
    int                 stat;
    if (primary) {
        pButtons = pLinkButtonsLinkType;
        pComboBoxShare = pUi->comboBoxLinkPortShare;
        pLinkShared=&linkShared;
        type   = linkType;
        stat   = states.link;
    }
    else {
        pButtons = pLinkButtonsLinkType2;
        pComboBoxShare = pUi->comboBoxLinkPortShare_2;
        pLinkShared=&linkShared2;
        type   = linkType2;
        stat   = states.link2;
    }
    switch (type) {
    case LT_TERM:
        buttonCheckEnable(pButtons, LT_FRONT, false, false);
        buttonCheckEnable(pButtons, LT_BACK,  false, false);
        buttonCheckEnable(pButtons, LT_TERM,  true,  true);
        *pLinkShared = ES_;
        pComboBoxShare->setDisabled(true);
        break;
    case LT_BACK:
        buttonCheckEnable(pButtons, LT_TERM,  false, false);
        buttonCheckEnable(pButtons, LT_FRONT, false, true);
        buttonCheckEnable(pButtons, LT_BACK,  true,  true);
        pComboBoxShare->setEnabled(true);
        break;
    case LT_FRONT:
        buttonCheckEnable(pButtons, LT_TERM,  false, false);
        buttonCheckEnable(pButtons, LT_BACK,  false, true);
        buttonCheckEnable(pButtons, LT_FRONT, true,  true);
        pComboBoxShare->setEnabled(true);
        break;
    default:
        buttonCheckEnable(pButtons, LT_TERM,  false, false);
        buttonCheckEnable(pButtons, LT_FRONT, false, false);
        buttonCheckEnable(pButtons, LT_BACK,  false, false);
        pComboBoxShare->setEnabled(false);
        stat = IS_EMPTY;
        break;
    }
    if (stat == IS_EMPTY) {
        if (primary) linkCollisionsMsg.clear();
        else         linkCollisionsMsg2.clear();
    }
    else {
        if (primary) {
            states.link = _linkCollision(*pq, stat, pPort1->getId(), pLinkPort->getId(), linkType, linkShared, linkCollisionsMsg);
        }
        else {
            states.link2 = _linkCollision(*pq, stat, pPort2->getId(), pLinkPort2->getId(), linkType2, linkShared2, linkCollisionsMsg2);
        }
    }
}

void cWorkstation::_setAlarmState(bool primary)
{
    bool alarmPosible = false;
    if (primary && states.link != IS_OK) {
        states.alarmPossibe = alarmPosible = false;
    }
    else if (!primary && states.link2 != IS_OK) {
        states.alarm2Possibe = alarmPosible = false;
    }
    else {
        cRecord *pLPort = primary ? pLinkPort : pLinkPort2;
        qlonglong pid = pLPort->getId();
        qlonglong nid = pLPort->getId(_sNodeId);
        qlonglong hsid = NULL_ID;
        if (primary) {
            qlonglong sid = cService::name2id(*pq, "pstat");
            cHostService hs;
            if (hs.fetchByIds(*pq, nid, sid, EX_IGNORE)) {
                hsid = hs.getId();
            }
        }
        else {
            QString nn = cNode().getNameById(*pq, nid);
            cHostService hs;
            if (1 == hs.fetchByNames(*pq, nn, "indalarmif%", EX_IGNORE)) {
                hsid = hs.getId();
            }
        }

    }




    if (!alarmPosible) {
        if (primary) {
            states.alarmNew = states.alarmOld = IS_NONEXIST;
            buttonSets(pUi->checkBoxSrvIns, false, false);
            buttonSets(pUi->checkBoxSrvDel, false, false);
            buttonSets(pUi->checkBoxSrvEna, false, false);
            pUi->lineEditSrvNote->setEnabled(false);
        }
        else {
            states.alarm2New = states.alarm2Old = IS_NONEXIST;
            buttonSets(pUi->checkBoxSrvIns_2, false, false);
            buttonSets(pUi->checkBoxSrvDel_2, false, false);
            buttonSets(pUi->checkBoxSrvEna_2, false, false);
            pUi->lineEditSrvNote_2->setEnabled(false);
        }
        return;
    }

}

void cWorkstation::_setMessage()
{
    _DBGFN() << VDEB(withinSlot) << endl;
    if (withinSlot != 0) return;
    QString infText = addrCollisionInfo + linkCollisionsMsg + linkCollisionsMsg2; // ...
    QString errText;
    bool ok = true;
    switch (states.nodeName) {
    case IS_EMPTY:
        ok = false;
        errText += error(trUtf8("Nincs megadva a munkaállomás neve."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += error(trUtf8("A megadott munkaállomás név ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.serialNumber) {
    case IS_EMPTY:
        errText += info(trUtf8("Nincs megadva a munkaállomás széria száma."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += error(trUtf8("A megadott munkaállomás széria szám ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.inventoryNumber) {
    case IS_EMPTY:
        errText += info(trUtf8("Nincs megadva a munkaállomás leltári száma."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += error(trUtf8("A megadott munkaállomás leltári szám ütközik egy másikkal."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.nodePlace == IS_EMPTY) {
        errText += info(trUtf8("Nincs megadva a munkaállomás helye."));
    }
    if (states.portName == IS_EMPTY) {
        ok = false;
        errText += error(trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a neve."));
    }
    switch (states.mac) {
    case IS_EMPTY:
        if (states.mac2need) {
            errText += info(trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a MAC-je."));
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += error(trUtf8("A megadott munkaállomás elsődleges interfész MAC ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += error(trUtf8("A megadott munkaállomás elsődleges interfész MAC hibás."));
        break;
    }
    switch (states.ipAddr) {
    case IS_EMPTY:
        if (states.ipNeed) {
            errText += info(trUtf8("Nincs megadva az IP cím."));
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += error(trUtf8("A megadott IP cím ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += error(trUtf8("A megadott IP cím hibás vagy hányos."));
        break;
    case IS_LOOPBACK:
        ok = false;
        errText += error(trUtf8("A megadott IP cím egy loopback cím."));
        break;
    case IS_EXTERNAL:
        ok = false;
        errText += error(trUtf8("Csak belső cím adható meg."));
        break;
    case IS_SOFT_COLL:
        errText += warning(trUtf8("A megadott IP cím ütközik egy másik dinamikus címmel."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.subNetNeed && states.subNet == IS_EMPTY) {
        errText += info(trUtf8("Nincs megadva IP tartomány illetve VLAN."));
    }
    switch (states.port2Name) {
    case IS_EMPTY:
        if (states.port2Need) {
            ok = false;
            errText += error(trUtf8("Nincs megadva a másodlagos port neve."));
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += error(trUtf8("A megadott a másodlagos port név nem lehet azonos az elsődlegessel."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.mac2) {
    case IS_EMPTY:
        if (states.mac2need) {
            errText += info(trUtf8("Nincs megadva a munkaállomás második interfészének a MAC-je."));
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += error(trUtf8("A munkaállomás második interfészének a MAC-je ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += error(trUtf8("A munkaállomás második interfészének a MAC-je hibás."));
        break;
    }
    if (states.linkPossible) switch (states.link) {
    case IS_EMPTY:
        errText += info(trUtf8("Nincs megadva link (patch)."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        break;
    case IS_IMPERFECT:
        errText += error(trUtf8("Nincs megadva a linkelt (patch-elt) port."));
        ok = false;
        break;
    }
    if (!ok) {
        errText += warning(trUtf8("Hiba miatt a mentés nem lehetséges."));
    }
    pUi->textEditErr->setText(errText);
    pUi->pushButtonSave->setEnabled(ok);
    pUi->textEditMsg->setText(infText);
}

void cWorkstation::_completionObject()
{
    qlonglong pid = node.getId(_sPlaceId);
    if (pid > ROOT_PLACE_ID) pPlace->fetchById(*pq, pid);// Valid
    else                     pPlace->clear();
    QString pt1 = portTypeList[states.passive][0];  // Első port alapértelmezett típusa (iftypes.iftype_name)
    switch (node.ports.size()) {
    case 0:   // Nincs port
        pPort1 = node.addPort(pt1, pt1, _sNul, NULL_IX);
        if (0 == pPort1->chkObjType<cInterface>(EX_IGNORE)) {   // Interfész ?
            pIpAddress = &(pPort1->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC));
        }
        break;
    case 1: // Egy portunk van
        if (states.passive) {   // Passive node
            pPort1 = node.ports.first();
        }
        else {                  // Aktív node : Workstation
            if (0 == node.ports.first()->chkObjType<cInterface>(EX_IGNORE)) {   // A port egy interfész ?
                pPort1 = node.ports.first();
                if (pPort1->reconvert<cInterface>()->addresses.isEmpty()) {     // Ha nincs címe, kap egy NULL, dinamikust
                    pIpAddress = &(pPort1->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC));
                }
                else {
                    pIpAddress = pPort1->reconvert<cInterface>()->addresses.first();
                }
            }
            else {                                                              // Nem interfész
                // Aktív eszköznek kell egy aktiv port (interfész), meg cím, a passzív lessz a második port
                pPort2 = node.ports.first();
                pPort1 = node.addPort(pt1, pt1, _sNul, NULL_IX);
                pIpAddress = &(pPort1->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC));
                // Swap
                node.ports[0] = pPort1;
                node.ports[1] = pPort2;
            }
        }
        break;
    case 2: // Két port van
        if (states.passive) EXCEPTION(EPROGFAIL);   // A szűrés alapján ez nem lehetséges
        // Első nem interfész, de a második igen
        if (node.ports.at(0)->tableoid() != cInterface::tableoid_interfaces()
         && node.ports.at(1)->tableoid() == cInterface::tableoid_interfaces()) {
            // Akkor felcsréljük
            pPort1 = node.ports.at(1)->reconvert<cInterface>();
            pPort2     = node.ports.first();
            node.ports[0] = pPort1;
            node.ports[1] = pPort2;
        }
        // Az első port egy interfész
        if (node.ports.first()->tableoid() == cInterface::tableoid_interfaces()) {
            pPort1 = node.ports.first()->reconvert<cInterface>();
            pPort2     = node.ports.at(1);
            if (pPort1->reconvert<cInterface>()->addresses.isEmpty()) {  // Nincs IP cím az első porton (sorrend véletlen szerű!!)
                if (pPort2->tableoid() == cInterface::tableoid_interfaces()    // A második is interfész ?
                 && !pPort2->reconvert<cInterface>()->addresses.isEmpty()) {   // neki van címe ?
                    // Akkor felcsréljük
                    pPort1 = node.ports.at(1)->reconvert<cInterface>();
                    pPort2     = node.ports.first();
                    node.ports[0] = pPort1;
                    node.ports[1] = pPort2;
                    pIpAddress = pPort1->reconvert<cInterface>()->addresses.first();
                }
                else {  // Nincs cím, csinálünk egy üreset:
                    pPort1->reconvert<cInterface>()->addresses << (pIpAddress = new cIpAddress);
                    pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
                }
            }
            else {  // Első porton van IP cím (csak egy IP cím lehet)
                pIpAddress = pPort1->reconvert<cInterface>()->addresses.first();
            }
        }
        else {  // Egyik sem interfész, ez elsőt megkamuzzuk mint interfész
            pPort1 = new cInterface();
            pPort1->copy(*node.ports.first());
            delete node.ports.first();
            node.ports.first() = pPort1;
            pPort1->setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
            node.ports.clear() << pPort1 ;
            pPort1->reconvert<cInterface>()->addresses << (pIpAddress = new cIpAddress);
            pIpAddress->setId(_sIpAddressType, AT_DYNAMIC);
        }
        break;
    default:    // A konstatns szűrési feltétel alapján ez lehetetlen
        EXCEPTION(EPROGFAIL);
        break;
    }

    // *** Link ***
    cPhsLink link;
    // Link port 1:
    if (link.isExist(*pq, pPort1->getId(), LT_TERM)) {
        pLinkPort->setById(*pq, link.getId(_sPortId2));
        pLinkNode->setById(*pq, pLinkPort->getId(_sNodeId));
        linkType   = (ePhsLinkType)link.getId(_sPhsLinkType2);
        linkShared = (ePortShare)  link.getId(_sPortShared);
    }
    else {
        pLinkNode->clear();
        pLinkPort->clear();
        linkType = LT_INVALID;
        linkShared = ES_NC;
    }
    // Link port 2:
    if (pPort2 != NULL && link.isExist(*pq, pPort2->getId(), LT_TERM)) {
        pLinkPort2->setById(*pq, link.getId(_sPortId2));
        pLinkNode2->setById(*pq, pLinkPort2->getId(_sNodeId));
        linkType2   = (ePhsLinkType)link.getId(_sPhsLinkType2);
        linkShared2 = (ePortShare)  link.getId(_sPortShared);
    }
    else {
        pLinkNode2->clear();
        pLinkPort2->clear();
        linkType2 = LT_INVALID;
        linkShared2 = ES_NC;
    }
}

void cWorkstation::_clearNode()
{
    pPort1     = NULL;
    pIpAddress = NULL;
    pPort2     = NULL;
    node.clear();
    _completionObject();
    _parseObject();
}

void cWorkstation::_parseObject()
{
    DBGFN();
    LOCKSLOTS();
    withinSlot++;       // Majd a végén lessznek üzenetek
    pUi->radioButtonMod->setEnabled(true);  // Van minta, elvileg lehet modosítani.
    // Port(ok) típusa: interfész/passzív port
    QString s;
    int ix;
    // Node name
    s = node.getName();
    pUi->lineEditName->setText(s);
    states.nodeName = _checkNodeCollision(node.nameIndex(), s);
    // Node Note
    pUi->lineEditNote->setText(node.getNote());
    // Node Place
    if (pPlace->isValid()) {
        s = pPlace->getName();
        states.nodePlace = IS_OK;
    }
    else {
        s = _sNul;
        states.nodePlace = IS_EMPTY;
    }
    _setPlace(s, pUi->comboBoxPlace, pUi->comboBoxZone);
    _linkToglePlaceEqu(true);
    _linkToglePlaceEqu(false);
    // Serial number
    ix = node.toIndex(_sSerialNumber);
    s  = node.getName(ix);
    pUi->lineEditSerial->setText(s);
    states.serialNumber = _checkNodeCollision(ix, s);
    // Inventory number
    ix = node.toIndex(_sInventoryNumber);
    s = node.getName(ix);
    pUi->lineEditInventory->setText(s);
    states.inventoryNumber = _checkNodeCollision(ix, s);
    // *** PORT 1 ***
    // Port 1 name
    s = pPort1->getName();
    pUi->lineEditPName->setText(s);
    if (s.isEmpty()) EXCEPTION(EDATA);
    states.portName = IS_OK;
    // Port 1 note, tag
    s = pPort1->getNote();
    pUi->lineEditPNote->setText(s);
    s = pPort1->getName(_sPortTag);
    pUi->lineEditPTag->setText(s);
    // Port type
    s  = cIfType::ifTypeName(pPort1->getId(_sIfTypeId));
    ix = pUi->comboBoxPType->findText(s);
    pUi->comboBoxPType->setCurrentIndex((ix >= 0) ? ix : 0);    // Nem a listában lávű típus, legyen a defíult (jobb mint kiakadni)
    _changePortType(true, ix);  // MAC and IP ADDRESS
    // *** PORT 2 ***
    if (pPort2 == NULL) {   // Nincs második port
        _changePortType(false, -1);  // MAC
    }
    else {
        // Port 2 name
        s = pPort2->getName();
        pUi->lineEditPName_2->setText(s);
        if (s.isEmpty()) EXCEPTION(EDATA);
        states.port2Name = IS_OK;
        // Port 2 note, tag
        s = pPort2->getNote();
        pUi->lineEditPNote_2->setText(s);
        s = pPort2->getName(_sPortTag);
        pUi->lineEditPTag_2->setText(s);
        // Port type
        s  = cIfType::ifTypeName(pPort2->getId(_sIfTypeId));
        ix = pUi->comboBoxPType_2->findText(s);
        pUi->comboBoxPType_2->setCurrentIndex((ix >= 1) ? ix : 1);    // Nem a listában lávű típus, legyen a defíult (jobb mint kiakadni)
        _changePortType(false, ix);  // MAC
    }

    // *** Link ***
    // Link port 1:
    if (linkType  != LT_INVALID) {      // Van linkje
        qlonglong id = pLinkNode->getId(_sPlaceId);
        if (id < UNKNOWN_PLACE_ID) {  // unknown place_id
            s = _sNul;
        }
        else {
            s = cPlace().getNameById(*pq, id);
        }
        _setPlace(s, pUi->comboBoxLinkPlace, pUi->comboBoxLinkZone);
        s = pLinkNode->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkNode);
        _changeLinkNode(s, pModelLinkPort, pUi->comboBoxLinkPort);
        pUi->lineEditLinkNodeType->setText(pLinkNode->getName(_sNodeType));
        s = pLinkPort->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkPort);

        cIfType iftype = cIfType::ifType(pLinkPort->getId(_sIfTypeId));
        eTristate isPatch = iftype.getName() == _sPatch ? TS_TRUE : TS_FALSE;
        states.link = _changeLinkPort(s, pLinkButtonsLinkType, pUi->comboBoxLinkPortShare, isPatch);
        if (isPatch == TS_TRUE) {
            _setCurrentIndex(portShare(linkShared2), pUi->comboBoxLinkPortShare);
            pUi->comboBoxLinkPortShare->setEnabled(true);
            if (linkType == LT_BACK) {  // Defaultként front lett beállítva
                pUi->radioButtonLinkBack->setChecked(true);
            }
        }
        _setLinkType(true);
    }
    else {
        _changeLinkNode(_sNul, pModelLinkPort, pUi->comboBoxLinkPort);
        pUi->lineEditLinkNodeType->setText(_sNul);
        states.link = _changeLinkPort(_sNul, pLinkButtonsLinkType, pUi->comboBoxLinkPortShare, TS_NULL);
    }
    // Link port 2:
    if (linkType2 != LT_INVALID) {      // Van linkje
        qlonglong id = pLinkNode2->getId(_sPlaceId);
        if (id <= UNKNOWN_PLACE_ID) {  // unknown place_id
            s = _sNul;
        }
        else {
            s = cPlace().getNameById(*pq, id);
        }
        _setPlace(s, pUi->comboBoxLinkPlace_2, pUi->comboBoxLinkZone_2);
        s = pLinkNode2->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkNode_2);
        _changeLinkNode(s, pModelLinkPort2, pUi->comboBoxLinkPort_2);
        pUi->lineEditLinkNodeType_2->setText(pLinkNode2->getName(_sNodeType));
        s = pLinkPort2->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkPort_2);

        cIfType iftype = cIfType::ifType(pLinkPort2->getId(_sIfTypeId));
        eTristate isPatch = iftype.getName() == _sPatch ? TS_TRUE : TS_FALSE;
        states.link = _changeLinkPort(s, pLinkButtonsLinkType2, pUi->comboBoxLinkPortShare_2, isPatch);
        if (isPatch == TS_TRUE) {
           _setCurrentIndex(portShare(linkShared), pUi->comboBoxLinkPortShare_2);
            pUi->comboBoxLinkPortShare_2->setEnabled(true);
            if (linkType == LT_BACK) {  // Defaultként front lett beállítva
                pUi->radioButtonLinkBack_2->setChecked(true);
            }
        }
        _setLinkType(false);
    }
    else {
        _changeLinkNode(_sNul, pModelLinkPort2, pUi->comboBoxLinkPort_2);
        pUi->lineEditLinkNodeType_2->setText(_sNul);
        states.link2 = _changeLinkPort(_sNul, pLinkButtonsLinkType2, pUi->comboBoxLinkPortShare_2, TS_NULL);
    }

    withinSlot--;
    _setMessage();
    UNLOCKSLOTS();
    DBGFN();
}

void cWorkstation::_addressChanged(const QString& sType, const QString& sAddr)
{
    addrCollisionInfo.clear();
    if (sType.isEmpty()) {
        pIpAddress = NULL;
        if (pPort1->chkObjType<cInterface>(EX_IGNORE) == 0) {
            pPort1->reconvert<cInterface>()->addresses.clear();
        }
        states.ipNeed = false;
        states.ipAddr = IS_EMPTY;
        pUi->lineEditAddress->setDisabled(true);
        pUi->lineEditIpNote->setDisabled(true);
        pUi->comboBoxIpType->setDisabled(true);
        pUi->comboBoxSubNet->setDisabled(true);
        pUi->comboBoxSubNetAddr->setDisabled(true);
        pUi->comboBoxVLan->setDisabled(true);
        pUi->comboBoxVLanId->setDisabled(true);
        return;
    }
    if (pIpAddress == NULL) {
        pIpAddress = &(pPort1->reconvert<cInterface>()->addIpAddress(QHostAddress(), sType));
    }
    else {
        pIpAddress->setName(cIpAddress::ixIpAddressType(), sType);
    }
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
    if (sAddr.isEmpty()) {
        pIpAddress->clear(_sAddress);
        states.ipAddr = IS_EMPTY;
    }
    else {
        QHostAddress a(sAddr);
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
                    ok = pattern.exactMatch(sAddr);
                    break;
                case QAbstractSocket::IPv6Protocol:
                    ok = sAddr != "::"; // Szerintem ez nem egy jó cím
                    break;
                default:
                    ok = false;
                    break;
                }
            }
            if (ok) {
                cNode no;
                states.ipAddr = IS_OK;
                if (no.fetchByIp(*pq, a, EX_IGNORE) && !(states.modify && pSample->getId() == no.getId())) {
                    tRecordList<cIpAddress> aa;
                    int n = no.fetchAllAddress(*pq, aa);    // kéne a konkrét rekord
                    cIpAddress *pA = NULL;
                    for (int i = 0; i < n; ++i) {
                        if (aa[i]->address() == a) {
                            pA = aa[i];
                            break;
                        }
                    }
                    if (pA == NULL) EXCEPTION(EDATA); // ??!!
                    int at = pA->getId(_sIpAddressType);
                    if (at == AT_FIXIP || at == AT_PSEUDO) {
                        states.ipAddr = IS_COLLISION;
                        ok = false;
                    }
                    else if (at == AT_DYNAMIC) {
                        states.ipAddr = IS_SOFT_COLL;
                    }
                    if (states.ipAddr != IS_OK) {
                        addrCollisionInfo = info(trUtf8("A %1 cím a %2 nevű eszköz címével ötközik").arg(a.toString(), no.getName()));
                    }
                }
                cSubNet sn;
                int ix = sn.getByAddress(*pq, a);
                switch (ix) {  // Hány subnet-re illeszkedik a cím
                case 0: // Nincs hozzá subnet
                    if (states.ipAddr == IS_OK) states.ipAddr = IS_EXTERNAL;
                    break;
                case 1: // 1 subnet, OK, belső cím
                    ix = pUi->comboBoxSubNet->findText(sn.getName());
                    if (ix < 0) EXCEPTION(EDATA);
                    pIpAddress->setAddress(a);
                    _subNetVLan(ix, -1);
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
            else {
                states.ipAddr = IS_INVALID;
            }
        }
    }
    pUi->lineEditAddress->setEnabled(true);
    pUi->lineEditIpNote->setEnabled(true);
    pUi->comboBoxIpType->setEnabled(true);
    bool f = !sAddr.isEmpty();
    pUi->comboBoxSubNet->setDisabled(f);
    pUi->comboBoxSubNetAddr->setDisabled(f);
    pUi->comboBoxVLan->setDisabled(f);
    pUi->comboBoxVLanId->setDisabled(f);
}



void cWorkstation::_subNetVLan(int sni, int vli)
{
    if (pIpAddress == NULL) return;
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
        if (pPort1->reconvert<cInterface>()->vlans.isEmpty()) {
            pPort1->reconvert<cInterface>()->vlans << new cPortVlan();
        }
        cPortVlan& pv = *pPort1->reconvert<cInterface>()->vlans.first();
        pv.setId(_sVlanId, pvl->getId());
        pv.setId(_sVlanType, VT_HARD);
        pv.setId(_sSetType, ST_MANUAL);
    }
    else {
        pPort1->reconvert<cInterface>()->vlans.clear();
    }
    LOCKSLOTS();
    pUi->comboBoxSubNet->setCurrentIndex(sni);
    pUi->comboBoxSubNetAddr->setCurrentIndex(sni);
    pUi->comboBoxVLan->setCurrentIndex(vli);
    pUi->comboBoxVLanId->setCurrentIndex(vli);
    UNLOCKSLOTS();
}

void cWorkstation::_changePortType(bool primary, int cix)
{
    if (cix < 0 || (!primary && cix == 0)) {  // Nincs port
        if (primary) EXCEPTION(EDATA);  // Elsődlegesnek kell lennie
        if (node.ports.size() > 1) delete node.ports.pop_back();    // Ha volt töröljük
        if (node.ports.size() != 1) EXCEPTION(EPROGFAIL);           // Pont egy portnak kell lennie
        pPort2 = NULL;
        linkType = LT_INVALID;  pLinkNode2->clear();    pLinkPort2->clear();
        LOCKSLOTS();
        pModelLinkPort2->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres
        _setLinkType(false);
        UNLOCKSLOTS();
        states.port2Need     = false;
        states.link2Possible = false;
        states.mac2need      = false;
        pUi->lineEditPMAC_2->setEnabled(false);
        pUi->comboBoxLinkZone_2->setEnabled(false);
        pUi->comboBoxLinkPlace_2->setEnabled(false);
        pUi->comboBoxLinkNode_2->setEnabled(false);
        pUi->comboBoxLinkPort_2->setEnabled(false);
        pUi->comboBoxLinkPortShare_2->setEnabled(false);
        pUi->lineEditLinkNodeType_2->setEnabled(false);
        pUi->lineEditLinkNote_2->setEnabled(false);
        return;
    }
    cNPort *pPort = primary ? pPort1 : pPort2;
    int ix = primary ? 0 : 1;   // port indexe a porst konténerben
    QString sType = (primary ? pUi->comboBoxPType : pUi->comboBoxPType_2)->itemText(cix);
    const cIfType& iftype = cIfType::ifType(sType);
    const cIfType& oldiftype = cIfType::ifType(pPort->getId(_sIfTypeId));
    if (iftype.getId(_sIfTypeObjType) != oldiftype.getId(_sIfTypeObjType)) {    // Megváltozott az objektum típusa!!
        cNPort *pPortNew = cNPort::newPortObj(iftype);
        node.ports[ix] = pPortNew; // csare
        pPortNew->set(*pPort);      // Amit lehet átmásoljuk
        delete pPort;
        pPort = pPortNew;
    }
    pPort->setId(_sIfTypeId, iftype.getId());  // Nem irtuk még ide, vagy elrontottuk
    bool isIface = iftype.getId(_sIfTypeObjType) == PO_INTERFACE;
    if (primary) {
        states.macNeed = isIface;
        pUi->lineEditAddress->setEnabled(isIface);
        if (isIface) {
            QString sType = _sDynamic;
            QString sAddr = _sNul;
            qlonglong six = 0;
            if (pIpAddress != NULL) {
                sType = pIpAddress->getName(_sIpAddressType);
                sAddr = pIpAddress->getName(_sAddress);
                qlonglong sid = pIpAddress->getId(_sSubNetId);
                if (sid != NULL_ID) {
                    QString subn  = cSubNet().getNameById(*pq, sid);
                    six = pUi->comboBoxSubNet->findText(subn);
                }
            }
            pUi->lineEditAddress->setText(sAddr);
            _addressChanged(sType, sAddr);
            _subNetVLan(six, -1);
            pUi->lineEditPMAC->setText(pPort1->getName(_sHwAddress));
        }
        else {
            _addressChanged(_sNul);
            _subNetVLan(0, 0);
        }
        pUi->lineEditPMAC->setEnabled(isIface);
    }
    else {
        states.mac2need = isIface;
        if (isIface) {
            pUi->lineEditPMAC_2->setText(pPort1->getName(_sHwAddress));
        }
        pUi->lineEditPMAC_2->setEnabled(isIface);
    }
    int lt = iftype.getId(_sIfTypeLinkType);
    bool linkPossible = lt == LT_PTP || lt == LT_BUS;
    if (primary) {
        states.linkPossible = linkPossible;
        WENABLE(comboBoxLinkZone, linkPossible);
        WENABLE(comboBoxLinkPlace, linkPossible);
        WENABLE(checkBoxPlaceEqu, linkPossible);
        WENABLE(comboBoxLinkNode, linkPossible);
        WENABLE(comboBoxLinkPort, linkPossible);
        WENABLE(comboBoxLinkPortShare, linkPossible);
        WENABLE(pushButtonNewPatch, linkPossible);
        WENABLE(pushButtonLinkOther, linkPossible);
        WENABLE(lineEditLinkNote, linkPossible);
        WENABLE(pushButtonNewPatch, linkPossible);
    }
    else {
        states.link2Possible = linkPossible;
        WENABLE(comboBoxLinkZone_2, linkPossible);
        WENABLE(comboBoxLinkPlace_2, linkPossible);
        WENABLE(checkBoxPlaceEqu_2, linkPossible);
        WENABLE(comboBoxLinkNode_2, linkPossible);
        WENABLE(comboBoxLinkPort_2, linkPossible);
        WENABLE(comboBoxLinkPortShare_2, linkPossible);
        WENABLE(pushButtonNewPatch_2, linkPossible);
        WENABLE(pushButtonLinkOther_2, linkPossible);
        WENABLE(lineEditLinkNote_2, linkPossible);
        WENABLE(pushButtonNewPatch_2, linkPossible);
    }

/*    pUi->checkBoxSrvIns->setEnabled(states.linkPossible);   // !!!
    pUi->checkBoxSrvDel->setEnabled(states.linkPossible);
    pUi->checkBoxSrvEna->setEnabled(states.linkPossible);
    pUi->lineEditSrvNote->setEnabled(states.linkPossible);*/
}

/**********************   SLOTS   ************************/

/// Debug üzenet a slot belépési pontján egy plussz paraméterrel
/// Ha lockSlot értéke treu, akkor az üzenet feltételes kiírása után visszatér.
#define BEGINSLOT(s) \
    _DBGFN() << (lockSlot ? " Skip (" : " (") << s << ")" << endl; \
    if (lockSlot) return; \
    int __i_temp__ = withinSlot; \
    ++withinSlot;

#define ENDSLOT() \
    --withinSlot; \
    if (__i_temp__ != withinSlot) EXCEPTION(EPROGFAIL); \
    DBGFNL()

#define ENDSLOTM() \
    --withinSlot; \
    if (__i_temp__ != withinSlot) EXCEPTION(EPROGFAIL); \
    _setMessage(); \
    DBGFNL()

void cWorkstation::toglePassive(int id, bool f)
{
    if (!f) return;
    BEGINSLOT(QString::number(id) + ", " + DBOOL(f));
    states.passive = id;
    states.ipNeed  = false;
    states.ipAddr  = IS_EMPTY;
    states.subNetNeed = false;
    node.ports.clear();
    QString t = portTypeList[id].first();       // Az alapértelmezett port típus neve
    pPort1 = node.addPort(t, t, _sNul, NULL_IX);// Hozzáadjuk a portot (alapértelmezett név a típus)
    pIpAddress = NULL;                          // Cím objektum =még) nincs
    switch (cIfType::ifType(t).getId(_sIfTypeObjType)) {
    case PO_INTERFACE:                          // Kell cím objektum
        pIpAddress = &(pPort1->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC));
        break;
    case PO_NPORT:                              // Nem kell cím objektum
        break;
    case PO_PPORT:                              // Ez nem lehetséges
        EXCEPTION(EDATA);
    }
    pPort2 = NULL;                              // Másodlagos port alapértelmezetten nincs
    pModelNode->setConstFilter(nodeListSql[id], FT_SQL_WHERE);  // A node típustol függő szűrés
    pModelNode->setFilter();                    // Frissítés. A konstans szűrő hívása nem frissít
    bool fip = pIpAddress != NULL;              // Ha ninbcs IP, a kapcolódó widgetek tiltása, ha van engedélyezése
    states.macNeed = fip;                       // Ha nincs IP (nem interface) nincs MAC sem
    lockSlot = true;                            // Slottok tiltása, majd a végén hívjuk, ami kell.
    pUi->comboBoxPType->clear();
    pUi->comboBoxPType->addItems(portTypeList[id]);
    pUi->comboBoxPType->setCurrentIndex(0);
    pUi->comboBoxPType_2->clear();
    if (id == 0) {                              // Csak workstation esetén (id == 0) lehet második port
        pUi->comboBoxPType_2->addItems(portTypeList[1]);
        pUi->comboBoxPType_2->setCurrentIndex(0);
    }
    lockSlot = false;
    portTypeCurrentIndex(0);
    pUi->comboBoxPType_2->setEnabled(id == 0);
    portTypeCurrentIndex2(0);
    pUi->lineEditPName->setText(t);

    pUi->lineEditAddress->setEnabled(fip);
    pUi->comboBoxIpType->setEnabled(fip);
    pUi->lineEditIpNote->setEnabled(fip);
    pUi->comboBoxSubNet->setEnabled(fip);
    pUi->comboBoxSubNetAddr->setEnabled(fip);
    pUi->comboBoxVLan->setEnabled(fip);
    pUi->comboBoxVLanId->setEnabled(fip);

    pUi->lineEditAddress->setText(_sNul);       // Alapértelmezett értékek
    pUi->comboBoxIpType->setCurrentIndex(0);
    pUi->comboBoxSubNet->setCurrentIndex(0);
    pUi->comboBoxSubNetAddr->setCurrentIndex(0);
    pUi->comboBoxVLan->setCurrentIndex(0);
    pUi->comboBoxVLanId->setCurrentIndex(0);
    ENDSLOTM();
}


void cWorkstation::togleModify(int id, bool f)
{
    if (!f) return;
    BEGINSLOT(QString::number(id) + ", " + DBOOL(f));
    states.modify = id;
    if (!f && pSample->isNull()) {  // Meg kellett adni, mit is modosítunk, anélkül nem OK
        EXCEPTION(EPROGFAIL);
    }
    pUi->radioButtonWorkstation->setDisabled(f);
    pUi->radioButtonPassive->setDisabled(f);
    // Pár státuszt újra kell számolni! (Ami függ a states.modify értékétől)
    nodeNameChanged(node.getName());
    serialChanged(node.getName(_sSerialNumber));
    inventoryChanged(node.getName(_sInventoryNumber));
    if (pPort1->isIndex(_sHwAddress)) macAddressChanged(pPort1->getName(_sHwAddress));
    if (pIpAddress != NULL) ipAddressChanged(pIpAddress->getName(cIpAddress::ixAddress()));
    ENDSLOTM();
}

void cWorkstation::save()
{
    DBGFN();
    // Néhány érték nem íródik automatikusan az objektumba
    node.setNote(pUi->lineEditNote->text());
    pPort1->setNote(pUi->lineEditPNote->text());
    pPort1->setName(_sPortTag, pUi->lineEditPTag->text());
    if (pIpAddress != NULL) {
        pIpAddress->setNote(pUi->lineEditIpNote->text());
    }
    if (pPort2 != NULL) {
        pPort2->setNote(pUi->lineEditPNote_2->text());
    }
    qlonglong id = node.getId();   // Hiba esetén vissza kell írni
    cError *pe = NULL;
    const static QString tkey = "Workstation_save";
    try {
        sqlBegin(*pq, tkey);
        if (states.modify) {
            node.rewriteById(*pq);
        }
        else {
            node.clear(node.idIndex());
            node.insert(*pq);
        }
        sqlEnd(*pq, tkey);
    } CATCHS(pe);
    if (pe != NULL) {
        sqlRollback(*pq, tkey);
        cErrorMessageBox::messageBox(pe, this);
        node.setId(id);    // Vissza az ID
    }
    _parseObject();
    DBGFNL();
}

void cWorkstation::refresh()
{
    _fullRefresh();
}

void cWorkstation::filterZoneCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    QString placeName = _changeZone(pUi->comboBoxFilterPlace, s);
    filterPlaceCurrentIndex(placeName);
    ENDSLOT();
}

void cWorkstation::filterPlaceCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    QString patt       = pUi->lineEditFilterPattern->text();
    QString zone_name  = pUi->comboBoxFilterZone->currentText();
    _changePlace(s, zone_name, pUi->comboBoxNode, pModelNode, patt);
    ENDSLOT();
}

void cWorkstation::filterPatternChanged(const QString& s)
{
    _DBGFN() << " (" << s << ")" << endl;
    QString place_name = pUi->comboBoxFilterPlace->currentText();
    QString zone_name  = pUi->comboBoxFilterZone->currentText();
    _changePlace(place_name, zone_name, pUi->comboBoxNode, pModelNode, s);
    DBGFNL();
}

// Workstation (node) slots
void cWorkstation::nodeCurrentIndex(int i)
{
    if (i == 0) return; // Első elem, semmi (nincs minta)
    qlonglong id = pModelNode->atId(i); // node_id
    if (pSample->getId() == id) return; // Nincs változás
    BEGINSLOT(i);
    pSample->setById(*pq, id);
    pSample->fetchPorts(*pq);
    pPort1 = NULL;  // A klonozásnál fel lesznek szabadítva!!!
    pIpAddress = NULL;
    pPort2     = NULL;
    node.clone(*pSample);
    _completionObject();
    _parseObject();
    ENDSLOTM();
}

void cWorkstation::nodeNameChanged(const QString& s)
{
    BEGINSLOT(s);
    node.setName(s);
    states.nodeName = _checkNodeCollision(node.nameIndex(), s);
    ENDSLOTM();
}

void cWorkstation::serialChanged(const QString& s)
{
    BEGINSLOT(s);
    int ix = node.toIndex(_sSerialNumber);
    node.setName(ix, s);
    states.serialNumber = _checkNodeCollision(ix, s);
    ENDSLOTM();
}

void cWorkstation::inventoryChanged(const QString& s)
{
    BEGINSLOT(s);
    int ix = node.toIndex(_sInventoryNumber);
    node.setName(ix, s);
    states.inventoryNumber = _checkNodeCollision(ix, s);
    ENDSLOTM();
}

void cWorkstation::zoneCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    QString sPlace = pPlace->isValid() ? pPlace->getName() : _sNul;
    sPlace = _changeZone(pUi->comboBoxPlace, s, sPlace);
    placeCurrentIndex(sPlace);
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
    node.setId(_sPlaceId, pPlace->getId());
    if (pUi->checkBoxPlaceEqu->isChecked())   _linkToglePlaceEqu(true, TS_TRUE);    // Elsődleges
    if (pUi->checkBoxPlaceEqu_2->isChecked()) _linkToglePlaceEqu(false, TS_TRUE);   // Mésodlagos
    ENDSLOTM();
}

void cWorkstation::nodeAddPlace()
{
    cRecord *pRec = insertRecordDialog(*pq, _sPlaces, this);
    if (pRec == NULL) return;
    QString sZone  = pUi->comboBoxZone->currentText();
    QString sPlace = pRec->getName();
    delete pRec;
    LOCKSLOTS();
    _setCurrentIndex(sZone, pUi->comboBoxZone);
    mapZones.clear();
    _placesInZone(_sAll);
    pUi->comboBoxPlace->clear();
    pUi->comboBoxPlace->addItems(_placesInZone(sZone));
    int ix = _setCurrentIndex(sPlace, pUi->comboBoxPlace, EX_IGNORE);
    if (ix < 0) {
        QString s = _changeZone(pUi->comboBoxPlace, _sAll, sPlace);
        if (sPlace != s) EXCEPTION(EDATA);
    }
    UNLOCKSLOTS();
    placeCurrentIndex(sPlace);
}

// Workstation ports slots
void cWorkstation::portNameChanged(const QString& s)
{
    BEGINSLOT(s);
    pPort1->setName(s);
    states.portName = s.isEmpty() ? IS_EMPTY : IS_OK;
    ENDSLOTM();
}

void cWorkstation::portTypeCurrentIndex(int i)
{
    BEGINSLOT(i);
    _changePortType(true, i);
    ENDSLOTM();
}

void cWorkstation::macAddressChanged(const QString& s)
{
    BEGINSLOT(s);
    if (s.isEmpty()) {
        pPort1->clear(_sHwAddress);
        states.mac = IS_EMPTY;
    }
    else {
        cMac mac(s);
        if (mac.isValid()) {
            pPort1->setMac(_sHwAddress, mac);
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
    ENDSLOTM();
}

// Workstation address slots
void cWorkstation::ipAddressChanged(const QString& s)
{
    if (pIpAddress == NULL) return;
    BEGINSLOT(s);
    QString sType = pIpAddress->getName(_sIpAddressType);
    _addressChanged(sType, s);
    ENDSLOTM();
}

void cWorkstation::ipAddressTypeCurrentIndex(const QString& s)
{
    if (pIpAddress == NULL) return;
    BEGINSLOT(s);
    QString sAddr = pIpAddress->getName(_sAddress);
    _addressChanged(s, sAddr);
    ENDSLOTM();
}

void cWorkstation::subNetCurrentIndex(int i)
{
    BEGINSLOT(i);
    _subNetVLan(i, -1);
    ENDSLOTM();
}

void cWorkstation::subNetAddrCurrentIndex(int i)
{
    BEGINSLOT(i);
    _subNetVLan(i, -1);
    ENDSLOTM();
}

void cWorkstation::vLanCurrentIndex(int i)
{
    BEGINSLOT(i);
    _subNetVLan(-1, i);
    ENDSLOTM();
}

void cWorkstation::vLanIdCurrentIndex(int i)
{
    BEGINSLOT(i);
    _subNetVLan(-1, i);
    ENDSLOTM();
}

// LINK 1 slots

void cWorkstation::linkChangeLinkType(int id, bool f)
{
    BEGINSLOT((QString::number(id) + ", " + DBOOL(f)));
    if (f && id != linkType) {
        linkType = (ePhsLinkType)id;
        states.link = _linkCollision(*pq, states.link, pPort1->getId(), pLinkPort->getId(), linkType, linkShared, linkCollisionsMsg);
    }
    ENDSLOTM();
}

void cWorkstation::linkToglePlaceEqu(bool f)
{
    BEGINSLOT(DBOOL(f));
    _linkToglePlaceEqu(true, f);
    ENDSLOT();
}

void cWorkstation::linkZoneCurrentIndex(const QString &s)
{
    BEGINSLOT(s);
    QString sPlace = _changeZone(pUi->comboBoxLinkPlace, s);
    linkPlaceCurrentIndex(sPlace);
    ENDSLOTM();
}

void cWorkstation::linkPlaceCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    int ix = _changePlace(s, pUi->comboBoxLinkZone->currentText(), pUi->comboBoxLinkNode, pModelLinkNode);
    pUi->comboBoxLinkNode->setCurrentIndex(ix);
    ENDSLOTM();
}

void cWorkstation::linkNodeCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    _changeLinkNode(s, pModelLinkPort, pUi->comboBoxLinkPort, pLinkNode, pLinkPort, &linkType, &linkShared);
    if (s.isEmpty()) {
        pUi->lineEditLinkNodeType->setText(_sNul);
    }
    else {
         pUi->lineEditLinkNodeType->setText(pLinkNode->getName(_sNodeType));
    }
    ENDSLOTM();
}


void cWorkstation::linkPortCurrentIndex(const QString& s)
{
    BEGINSLOT(s);
    states.link = _changeLinkPort(s, pLinkButtonsLinkType, pUi->comboBoxLinkPortShare, TS_NULL, pLinkPort, &linkType, &linkShared);
    states.link = _linkCollision(*pq, states.link, pPort1->getId(), pLinkPort->getId(), linkType, linkShared, linkCollisionsMsg);
    ENDSLOTM();
}

void cWorkstation::linkPortShareCurrentIndex(const QString &s)
{
    BEGINSLOT(s);
    linkShared = (ePortShare)portShare(s);
    states.link = _linkCollision(*pq, states.link, pPort1->getId(), pLinkPort->getId(), linkType, linkShared, linkCollisionsMsg);
    ENDSLOTM();
}

void cWorkstation::newPatch()
{
    cPatch *p = insertPatchDialog(*pq, this);

    delete p;
}


// Workstation port 2 slots
void cWorkstation::portNameChanged2(const QString& s)
{
    BEGINSLOT(s);
    ;
    ENDSLOTM();
}

void cWorkstation::portTypeCurrentIndex2(int i)
{
    BEGINSLOT(i);
    _changePortType(false, i);
    ENDSLOTM();
}

void cWorkstation::macAddressChanged2(const QString& s)
{
    BEGINSLOT(s);
    ;
    ENDSLOTM();
}

// LINK 2 slots
void cWorkstation::linkChangeLinkType2(int id, bool f)
{
    BEGINSLOT(QString::number(id) + ", " + DBOOL(f));
    ;
    ENDSLOTM();
}

void cWorkstation::linkToglePlaceEqu2(bool f)
{
    BEGINSLOT(DBOOL(f));
    _linkToglePlaceEqu(true, f);
    ENDSLOT();
}

void cWorkstation::linkZoneCurrentIndex2(const QString &s)
{
    BEGINSLOT(s);
    QString sPlace = _changeZone(pUi->comboBoxLinkPlace_2, s);
    linkPlaceCurrentIndex2(sPlace);
    ENDSLOTM();
}

void cWorkstation::linkPlaceCurrentIndex2(const QString& s)
{
    BEGINSLOT(s);
    ;
    ENDSLOTM();
}

void cWorkstation::linkNodeCurrentIndex2(const QString& s)
{
    BEGINSLOT(s);
    _changeLinkNode(s, pModelLinkPort2, pUi->comboBoxLinkPort_2, pLinkNode2, pLinkPort2, &linkType2, &linkShared2);
    if (s.isEmpty()) {
        pUi->lineEditLinkNodeType_2->setText(_sNul);
    }
    else {
        pUi->lineEditLinkNodeType_2->setText(pLinkNode2->getName(_sNodeType));
    }
    ENDSLOTM();
}

void cWorkstation::linkPortCurrentIndex2(const QString& s)
{
    BEGINSLOT(s);
    ;
    ENDSLOTM();
}

void cWorkstation::linkPortShareCurrentIndex2(const QString &s)
{
    BEGINSLOT(s);
    ;
    ENDSLOTM();
}

void cWorkstation::newPatch2()
{

}