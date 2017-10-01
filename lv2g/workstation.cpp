#include "lv2widgets.h"
#include "srvdata.h"
#include "lv2user.h"
#include "lv2link.h"
#include "record_dialog.h"
#include "object_dialog.h"
#include "report.h"
#include "workstation.h"
#include "ui_wstform.h"
#include "ui_phslinkform.h"
#include "report.h"

/// \brief linkText
/// \return A link statusz (IS_OK vagy IS_COLLISION)
/// @param stat link status
/// @param modify modify status
/// @param pid  saját port ID (ha nem mentett port, akkor NULL_ID)
/// @param lpid Linkelt port ID
/// @param type Fizikai link típusa
/// @param sh   Megosztás
/// @param msg  Üzenet string referencia
static int _linkTest(QSqlQuery& q, int stat, bool modify, qlonglong _pid, qlonglong lpid, ePhsLinkType type, ePortShare _sh, QString& msg)
{
    DBGFN();
    msg.clear();
    if (stat != IS_OK && stat != IS_COLLISION) return stat;
    cPhsLink link;
    tRecordList<cPhsLink> list;
    int r;
    if (link.collisions(q, list, lpid, type, _sh)) {
        if (modify) {   // Modosítás esetén saját magunkkal nem ütközünk
            int ix = -1;
            if (_pid != NULL_ID) ix = list.indexOf(_sPortId2, _pid);
            if (ix >= 0) delete list.takeAt(ix);
        }
    }
    if (list.size() > 0) {
        msg += htmlInfo(QObject::trUtf8("A megadott link a következő link(ek)el ütközik:"));
        msg += linksHtmlTable(q, list);
        r = IS_COLLISION;
    }
    else {
        r = IS_OK;
    }
    if (type != LT_TERM) {  // További linkek
        static const QString sql = "SELECT * from next_phs_link(NULL,?,?,?)";
        list.clear();
        link.clear();
        link.setId(_sPortId2,      lpid);
        link.setId(_sPhsLinkType2, type);
        QString sh = portShare(_sh);
        while (execSql(q, sql, link.get(_sPortId2), link.get(_sPhsLinkType2), sh)) {
            if (q.value(0).isNull()) break; // vége (beolvassa a NULL rekordot is, mert függvényt hívunk)
            link.set(q);
            list << link;
            sh = link.getName(_sPortShared);
            if (list.size() > 10) break;
            if (link.getId(_sPhsLinkType2) == LT_TERM) break;
        }
        if (list.size() > 0) {
            if (link.getId(_sPhsLinkType2) == LT_TERM) {
                msg += htmlInfo(QObject::trUtf8("A linkel lánca, végponttól - végpontig:"));
            }
            else {
                msg += htmlInfo(QObject::trUtf8("A linkek lánca, csonka, nem ér el a másik végpontig :"));
            }
            msg += linksHtmlTable(q, list);
            if (link.getId(_sPhsLinkType2) != LT_TERM && list.size() > 10) msg += htmlError("...");
        }
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

/*static void buttonSets(QAbstractButton *p, bool e, bool c)
{
    p->setEnabled(e);
    p->setChecked(c);
}*/

/* ************************************************************************************************* */

const enum ePrivilegeLevel cWorkstation::rights = PL_OPERATOR;

#if 1
cWorkstation::cWorkstation(QMdiArea *parent) :
    cIntSubObj(parent),
    pUi(new Ui::wstWidget),
    pq(newQuery())
{
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
    pModifyButtons = new QButtonGroup(this);
    pModifyButtons->addButton(pUi->radioButtonNew, 0);
    pModifyButtons->addButton(pUi->radioButtonMod, 1);
    // Csak az egy portos eszközöket keressük, és persze nem patch paneleket
    static const QString sNodeConstFilt = "(NOT 'patch'::nodetype = ANY (node_type) AND 1 = (SELECT count(*) FROM nports AS p WHERE patchs.node_id = p.node_id))";
    pSelNode      = new cSelectNode(pUi->comboBoxFilterZone, pUi->comboBoxFilterPlace, pUi->comboBoxNode,
                                    pUi->lineEditFilterPlace, pUi->lineEditName, _sNul, sNodeConstFilt, this);
    pSelPlace     = new cSelectPlace(pUi->comboBoxZone, pUi->comboBoxPlace, pUi->lineEditPlacePat, _sNul, this);
    pSelPlaceLink = new cSelectPlace(pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace, pUi->lineEditLinkPlacePat, _sNul, this);

}

cWorkstation::~cWorkstation()
{
    ;
}

void cWorkstation::node2Gui()
{

}

#else
cWorkstation::cWorkstation(QMdiArea *parent) :
    cIntSubObj(parent),
    pUi(new Ui::wstWidget),
    pq(newQuery()),
    pSample(new cNode),
    node(),
    pModelNode(new cRecordListModel(node.descr())),
    pPlace(new cPlace),
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
    node.setId(_sNodeType, ENUM2SET(NT_WORKSTATION));
    cInterface *pi = new cInterface;
    pi->  setName(_sEthernet);      // FORM: wstform !!! Azonos alap értékek kellenek!
    pi->    setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
    cIpAddress *pa = new cIpAddress;
    pa->setId(_sIpAddressType, AT_DYNAMIC);
    pi->addresses << pa;
    node.ports   << pi;
    pPlace->setById(UNKNOWN_PLACE_ID);     // NAME = 'unknown'

    _initLists();
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
    _setRecordListModel(pUi->comboBoxNode, pModelNode);
    pUi->comboBoxNode->setCurrentIndex(0);  // első elem, NULL kiválasztva

    pFilterZoneModel = new cZoneListModel;
    pUi->comboBoxFilterZone->setModel(pFilterZoneModel);
    pUi->comboBoxFilterZone->setCurrentIndex(0);          // Első elem 'all' zóna
    pFilterPlaceModel = new cPlacesInZoneModel;
    pUi->comboBoxFilterPlace->setModel(pFilterPlaceModel);
    pUi->comboBoxFilterPlace->setCurrentIndex(0);         // Első elem <NULL>/nincs megadva hely

    pZoneModel = new cZoneListModel;
    pUi->comboBoxZone->setModel(pZoneModel);
    pUi->comboBoxZone->setCurrentIndex(0);          // Első elem 'all' zóna
    pPlaceModel = new cPlacesInZoneModel;
    _setRecordListModel(pUi->comboBoxPlace, pPlaceModel);
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

    pLinkZoneModel = new cZoneListModel;
    pUi->comboBoxLinkZone->setModel(pLinkZoneModel);
    pUi->comboBoxLinkZone->setCurrentIndex(0);
    pLinkPlaceModel = new cPlacesInZoneModel;
    pUi->comboBoxLinkPlace->setModel(pLinkPlaceModel);
    pUi->comboBoxLinkPlace->setCurrentIndex(0);

    pModelLinkNode->nullable    = true;
    _setRecordListModel(pUi->comboBoxLinkNode, pModelLinkNode);
    pModelLinkNode->setFilter(_sFALSE, OT_ASC, FT_SQL_WHERE); // Nincs kiválasztott hely, a lista üres lessz

    pModelLinkPort->nullable    = true;
    pModelLinkPort->sFkeyName   = _sNodeId;
    static const QString sViewLinkPort = "concat(port_name, ' (\"', port_tag, '\", #', port_index, ')')";
    pModelLinkPort->setViewExpr(sViewLinkPort);
    pModelLinkPort->setFilter(QVariant(), OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres
    _setRecordListModel(pUi->comboBoxLinkPort, pModelLinkPort);
    portType2No = trUtf8("Nincs másodlagos port");
    pUi->comboBoxPType_2->addItem(portType2No);
    pUi->comboBoxPType_2->addItems(portTypeList[1]);
    pUi->comboBoxPType_2->setCurrentIndex(0);
    // Link 2
    pLinkButtonsLinkType2->addButton(pUi->radioButtonLinkTerm_2,  LT_TERM);
    pLinkButtonsLinkType2->addButton(pUi->radioButtonLinkFront_2, LT_FRONT);
    pLinkButtonsLinkType2->addButton(pUi->radioButtonLinkBack_2,  LT_BACK);

    pLinkZoneModel2 = new cZoneListModel;
    pUi->comboBoxLinkZone_2->setModel(pLinkZoneModel2);
    pUi->comboBoxLinkZone_2->setCurrentIndex(0);
    pLinkPlaceModel2 = new cPlacesInZoneModel;
    pUi->comboBoxLinkPlace_2->setModel(pLinkPlaceModel2);
    pUi->comboBoxLinkPlace_2->setCurrentIndex(0);

    pModelLinkNode2->nullable    = true;
    pUi->comboBoxLinkNode_2->setModel(pModelLinkNode2);
    pModelLinkNode2->setFilter(_sFALSE, OT_ASC, FT_SQL_WHERE); // Nincs kiválasztott hely, a lista üres lessz

    pModelLinkPort2->nullable    = true;
    pModelLinkPort2->sFkeyName   = _sNodeId;
    pModelLinkPort2->setViewExpr(sViewLinkPort);
    pUi->comboBoxLinkPort_2->setModel(pModelLinkPort2);
    pModelLinkPort2->setFilter(QVariant(), OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres

    connect(pModifyButtons,             SIGNAL(buttonToggled(int,bool)),      this, SLOT(togleModify(int, bool)));
    connect(pPassiveButtons,            SIGNAL(buttonToggled(int,bool)),      this, SLOT(toglePassive(int,bool)));
    connect(pUi->pushButtonCancel,      SIGNAL(pressed()),                    this, SLOT(endIt()));
    connect(pUi->pushButtonSave,        SIGNAL(pressed()),                    this, SLOT(save()));
    connect(pUi->pushButtonRefresh,     SIGNAL(pressed()),                    this, SLOT(refresh()));
    connect(pUi->comboBoxFilterZone,    SIGNAL(currentIndexChanged(QString)), this, SLOT(filterZoneCurrentIndex(QString)));
    connect(pUi->comboBoxFilterPlace,   SIGNAL(currentIndexChanged(int)),     this, SLOT(filterPlaceCurrentIndex(int)));
    connect(pUi->lineEditFilterPattern, SIGNAL(textChanged(QString)),         this, SLOT(filterPatternChanged(QString)));
    // Workstation:
    connect(pUi->comboBoxNode,          SIGNAL(currentIndexChanged(int)),     this, SLOT(nodeCurrentIndex(int)));
    connect(pUi->lineEditName,          SIGNAL(textChanged(QString)),         this, SLOT(nodeNameChanged(QString)));
    connect(pUi->lineEditSerial,        SIGNAL(textChanged(QString)),         this, SLOT(serialChanged(QString)));
    connect(pUi->lineEditInventory,     SIGNAL(textChanged(QString)),         this, SLOT(inventoryChanged(QString)));
    connect(pUi->comboBoxZone,          SIGNAL(currentIndexChanged(QString)), this, SLOT(zoneCurrentIndex(QString)));
    connect(pUi->comboBoxPlace,         SIGNAL(currentIndexChanged(int)),     this, SLOT(placeCurrentIndex(int)));
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
    connect(pUi->comboBoxLinkPlace,     SIGNAL(currentIndexChanged(int)),     this, SLOT(linkPlaceCurrentIndex(int)));
    connect(pUi->comboBoxLinkNode,      SIGNAL(currentIndexChanged(int)),     this, SLOT(linkNodeCurrentIndex(int)));
    connect(pUi->comboBoxLinkPort,      SIGNAL(currentIndexChanged(int)),     this, SLOT(linkPortCurrentIndex(int)));
    connect(pUi->comboBoxLinkPortShare, SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPortShareCurrentIndex(QString)));
    connect(pLinkButtonsLinkType,       SIGNAL(buttonToggled(int,bool)),      this, SLOT(linkChangeLinkType(int, bool)));
    connect(pUi->pushButtonNewPatch,    SIGNAL(pressed()),                    this, SLOT(newPatch()));

    // Link2:
    connect(pUi->checkBoxPlaceEqu_2,    SIGNAL(toggled(bool)),                this, SLOT(linkToglePlaceEqu2(bool)));
    connect(pUi->comboBoxLinkZone_2,    SIGNAL(currentIndexChanged(QString)), this, SLOT(linkZoneCurrentIndex2(QString)));
    connect(pUi->comboBoxLinkPlace_2,   SIGNAL(currentIndexChanged(int)),     this, SLOT(linkPlaceCurrentIndex2(int)));
    connect(pUi->comboBoxLinkNode_2,    SIGNAL(currentIndexChanged(int)),     this, SLOT(linkNodeCurrentIndex2(int)));
    connect(pUi->comboBoxLinkPort_2,    SIGNAL(currentIndexChanged(int)),     this, SLOT(linkPortCurrentIndex2(int)));
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
        node.clearId();         // Biztos külömbözzön az ID !!
        nodeCurrentIndex(ix);
    }
}

void cWorkstation::_refreshPlaces(QComboBox *pComboBoxZone, QComboBox *pComboBoxPlace)
{
    QString sZone  = pComboBoxZone->currentText();
    QString sPlace = pPlaceModel->at(pComboBoxPlace->currentIndex());
    LOCKSLOTS();
    ((cZoneListModel *)pComboBoxZone->model())->setFilter();
    _setCurrentIndex(sZone, pComboBoxZone, EX_IGNORE);
    sZone = pComboBoxZone->currentText();
    ((cPlacesInZoneModel *)pComboBoxPlace->model())->setZone(sZone);
    _setCurrentIndex(sPlace, pComboBoxPlace, EX_IGNORE);
    UNLOCKSLOTS();
}

int cWorkstation::_checkNodeCollision(int ix, const QString& s)
{
    if (s.isEmpty()) return IS_EMPTY;
    if (s == pSample->getName(ix)) {
        return states.modify ? IS_OK : IS_COLLISION;
    }
    cPatch *pO;
    bool r;
    pO = new cPatch;   // A név mezőnel az őssel sem ütközhet
    pO->setName(ix, s);
    if (pO->fetch(*pq, false, _bit(ix))) {
        r = states.modify || pO->getId() != pSample->getId();
    }
    else {
        r = false;
    }
    delete pO;
    return r ? IS_COLLISION : IS_OK;
}

int cWorkstation::_changeZone(QComboBox *pComboBoxPlace, const QString& sZone, const QString &_placeName)
{
    QString placeName;
    if (_placeName.isNull()) {
        cPlacesInZoneModel *pModel = dynamic_cast<cPlacesInZoneModel *>(pComboBoxPlace->model());
        if (pModel == NULL) EXCEPTION(EPROGFAIL);
        placeName = pModel->at(pComboBoxPlace->currentIndex());
    }
    else {
        placeName = _placeName;
    }
    LOCKSLOTS();
    cPlacesInZoneModel *pModel = (cPlacesInZoneModel *)pComboBoxPlace->model();
    pModel->setZone(sZone);
    int i = _setCurrentIndex(placeName, pComboBoxPlace, EX_IGNORE);
    UNLOCKSLOTS();
    return i;
}

int cWorkstation::_changePlace(const QString& placeNme, const QString& zoneName,
                               QComboBox *pComboBoxNode, cRecordListModel *pModel,
                               const QString& _patt)
{
    QString expr;
    QString actNode = pModel->at(pComboBoxNode->currentIndex());     // Az aktuálisan kiválasztott node neve
    if (placeNme.isEmpty()) {          // Nincs kiválasztva hely ?
        // Ha nincs kiválasztott hely, akkor az aktuális zóna szerint szűrünk
        if (zoneName.isEmpty() || zoneName == _sAll) {
            expr = _sNul;   // ALL zóna: nincs további szűrés
        }
        else {
            // Csak a zónában lévő node-ok
            qlonglong zid = cPlaceGroup().getIdByName(*pq, zoneName);
            // Azok a node-ok, melyeknek helye, vagy a hely valamelyik parentje benne van az aktuális zónában
            expr = QString("is_place_in_zone(place_id, %1)").arg(zid);
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
    int ix = _setCurrentIndex(actNode, pComboBoxNode, pModel, EX_IGNORE);
    UNLOCKSLOTS();
    return ix;
}

int cWorkstation::_changeLinkNode(const QString& nodeName, cRecordListModel *pModelPort, QComboBox * pComboBoxPort,
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
            qlonglong nid;
            if (pNode != NULL) {
                pNode->setByName(*pq, nodeName);
                nid = pNode->getId();
            }
            else {
                nid = cPatch().getIdByName(nodeName);
            }
            pModelPort->setFilter(nid);
            pComboBoxPort->setEnabled(true);
            pComboBoxPort->setCurrentIndex(0);
            if (pPort       != NULL) pPort->clear();
            if (pLinkShared != NULL) *pLinkShared = ES_NC;
            if (pLinkType   != NULL) *pLinkType = LT_INVALID;
        }
        r =  IS_IMPERFECT;
    }
    UNLOCKSLOTS();
    return r;
}

int cWorkstation::_changeLinkPort(qlonglong portId,
                                        QButtonGroup * pLinkType, QComboBox *pComboBoxShare, eTristate _isPatch,
                                        cRecordAny *pPort, ePhsLinkType *pType, ePortShare *pShare)
{
    int r = IS_OK;
    LOCKSLOTS();    // !!
    if (portId == NULL_ID) {
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
            pPort->setById(*pq, portId);
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
    bool f = false;
    switch (__f) {
    case TS_TRUE:   f = true;   break;      // Azonosság lett beállítva (slot hívta)
    case TS_FALSE:  f = false;  break;      // Nem azonos lett beállítva (slot hívta)
    case TS_NULL:                           // A checkBox állapotának lekérdezése
        if (primary) {
            f = pUi->checkBoxPlaceEqu->isChecked();
        }
        else {
            f = pUi->checkBoxPlaceEqu_2->isChecked();
        }
        break;
    }
    withinSlot++;
    LOCKSLOTS();
    QComboBox *pComboBoxZone;
    QComboBox *pComboBoxPlace;
    cPlacesInZoneModel *pLPlaceModel;
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
        pLPlaceModel   = pLinkPlaceModel;
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
        pLPlaceModel   = pLinkPlaceModel2;
        pComboBoxNode  = pUi->comboBoxLinkNode_2;
        pModelNode     = pModelLinkNode2;
        pComboBoxPort  = pUi->comboBoxLinkPort_2;
        pModelPort     = pModelLinkPort2;
        pNode          = pLinkNode2;
        pPort          = pLinkPort2;
        pLinkType      = &linkType2;
        pShare         = &linkShared;
    }
    if (f) {
        int     ixZone  = pUi->comboBoxZone->currentIndex();
        int     ixPlace = pUi->comboBoxPlace->currentIndex();   // Az aktuális hely indexe az aktuális zónában (az üres a nulladik)
        if (ixPlace < 0 || ixZone < 0) EXCEPTION(EPROGFAIL);    // Ha nincs találat akkor elkutyultunk valamit
        QString sZone   = pZoneModel->at(ixZone);           // Az aktuális zóna (ahol a munkaállomás van)
        QString sPlace  = pPlaceModel->at(ixPlace);         // És az aktuálisan megadott hely neve, vagy üres string
        QString sLinkPlace  = pLPlaceModel->at(pComboBoxPlace->currentIndex());
        if (sZone  != pComboBoxZone->currentText()
         || sPlace != sLinkPlace) {  // Ha nincs változás, akkor nem kell tenni semmit
            pComboBoxZone->setCurrentIndex(ixZone);     // A zónát csak ki kell választani
            pLPlaceModel->setFilter(sZone);  // Frissítjük a listát
            _setCurrentIndex(sPlace, pComboBoxPlace, pLPlaceModel);   // Béállítjuk a helyet is
            int ix = _changePlace(sPlace, sZone, pComboBoxNode, pModelNode);    // SLOT tiltva volt
            QString s = pModelNode->at(ix); // A név, nem ami megjelenik
            if (__f != TS_NULL) {
                int st = _changeLinkNode(s, pModelPort, pComboBoxPort, pNode, pPort, pLinkType, pShare);
                if (primary) states.link  = st;
                else         states.link2 = st;
            }
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
        if (primary) linkInfoMsg.clear();
        else         linkInfoMsg2.clear();
    }
    else {
        if (primary) {
            states.link = _linkTest(*pq, stat, states.modify, pPort1()->getId(), pLinkPort->getId(), linkType, linkShared, linkInfoMsg);
        }
        else {
            states.link2 = _linkTest(*pq, stat, states.modify, pPort2()->getId(), pLinkPort2->getId(), linkType2, linkShared2, linkInfoMsg2);
        }
    }
}

void cWorkstation::_setMessage()
{
    _DBGFN() << VDEB(withinSlot) << endl;
    if (withinSlot != 0) return;
    QString infText;
    if (!linkInfoMsg.isEmpty() || !addrCollisionInfo.isEmpty()) {
        infText += htmlInfo(QObject::trUtf8("Első port :"));
        if (!addrCollisionInfo.isEmpty()) infText += htmlWarning(addrCollisionInfo);
        infText += linkInfoMsg;

    }
    if (states.macNeed && states.mac == IS_OK) {
        QString sMac = pPort1()->getName(_sHwAddress);
        infText += htmlWarning(sMac);
        infText += reportByMac(*pq, sMac);
    }
    if (!linkInfoMsg2.isEmpty()) {
        infText += htmlInfo(QObject::trUtf8("Második port :"));
        infText += linkInfoMsg2;
    }
    if (states.mac2need && states.mac2 == IS_OK) {
        QString sMac = pPort2()->getName(_sHwAddress);
        infText += htmlWarning(sMac);
        infText += reportByMac(*pq, sMac);
    }
    QString errText;
    bool ok = true;
    switch (states.nodeName) {
    case IS_EMPTY:
        ok = false;
        errText += htmlError(trUtf8("Nincs megadva a munkaállomás neve."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += htmlError(trUtf8("A megadott munkaállomás név ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.serialNumber) {
    case IS_EMPTY:
        errText += htmlInfo(trUtf8("Nincs megadva a munkaállomás széria száma."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += htmlError(trUtf8("A megadott munkaállomás széria szám ütközik egy másikkal."));
        break;
    case IS_INVALID:
        EXCEPTION(EPROGFAIL);
        break;
    }
    switch (states.inventoryNumber) {
    case IS_EMPTY:
        errText += htmlInfo(trUtf8("Nincs megadva a munkaállomás leltári száma."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += htmlError(trUtf8("A megadott munkaállomás leltári szám ütközik egy másikkal."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.nodePlace == IS_EMPTY) {
        errText += htmlInfo(trUtf8("Nincs megadva a munkaállomás helye."));
    }
    if (states.portName == IS_EMPTY) {
        ok = false;
        errText += htmlError(trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a neve."));
    }
    switch (states.mac) {
    case IS_EMPTY:
        if (states.macNeed) {
            errText += htmlInfo(trUtf8("Nincs megadva a munkaállomás elsődleges interfészének a MAC-je."));
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += htmlError(trUtf8("A megadott munkaállomás elsődleges interfész MAC ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += htmlError(trUtf8("A megadott munkaállomás elsődleges interfész MAC hibás."));
        break;
    }
    switch (states.ipAddr) {
    case IS_EMPTY:
        switch (states.ipNeed) {
        case IS_NOT_POSSIBLE:   break;
        case IS_POSSIBLE:       errText += htmlInfo(trUtf8("Nincs megadva az IP cím."));        break;
        case IS_MUST:           errText += htmlError(trUtf8("Nincs megadva a fix IP cím."));    ok = false; break;
        default:                EXCEPTION(EDATA);
        }
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        ok = false;
        errText += htmlError(trUtf8("A megadott IP cím ütközik egy másikkal."));
        break;
    case IS_INVALID:
        ok = false;
        errText += htmlError(trUtf8("A megadott IP cím hibás vagy hányos."));
        break;
    case IS_LOOPBACK:
        ok = false;
        errText += htmlError(trUtf8("A megadott IP cím egy loopback cím."));
        break;
    case IS_EXTERNAL:
        ok = false;
        errText += htmlError(trUtf8("Csak belső cím adható meg."));
        break;
    case IS_SOFT_COLL:
        errText += htmlWarning(trUtf8("A megadott IP cím ütközik egy másik dinamikus címmel."));
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    if (states.subNetNeed && states.subNet == IS_EMPTY) {
        errText += htmlInfo(trUtf8("Nincs megadva IP tartomány illetve VLAN."));
    }
    if (states.port2Need) {
        switch (states.port2Name) {
        case IS_EMPTY:
            errText += htmlError(trUtf8("Nincs megadva a másodlagos port neve."));
            break;
        case IS_OK:
            break;
        case IS_COLLISION:
            ok = false;
            errText += htmlError(trUtf8("A megadott a másodlagos port név nem lehet azonos az elsődlegessel."));
            break;
        default:
            EXCEPTION(EPROGFAIL);
            break;
        }
        switch (states.mac2) {
        case IS_EMPTY:
            if (states.mac2need) {
                errText += htmlInfo(trUtf8("Nincs megadva a munkaállomás második interfészének a MAC-je."));
            }
            break;
        case IS_OK:
            break;
        case IS_COLLISION:
            ok = false;
            errText += htmlError(trUtf8("A munkaállomás második interfészének a MAC-je ütközik egy másikkal."));
            break;
        case IS_INVALID:
            ok = false;
            errText += htmlError(trUtf8("A munkaállomás második interfészének a MAC-je hibás."));
            break;
        }
    }
    if (states.linkPossible) switch (states.link) {
    case IS_EMPTY:
        errText += htmlInfo(trUtf8("Nincs megadva link (patch)."));
        break;
    case IS_OK:
        break;
    case IS_COLLISION:
        break;
    case IS_IMPERFECT:
        errText += htmlError(trUtf8("Nincs megadva a linkelt (patch-elt) port."));
        ok = false;
        break;
    }
    if (!ok) {
        errText += htmlWarning(trUtf8("Hiba miatt a mentés nem lehetséges."));
    }
    pUi->textEditErr->setText(errText);
    pUi->pushButtonSave->setEnabled(ok);
    pUi->textEditMsg->setHtml(infText);
}

void cWorkstation::_completionObject()
{
    qlonglong pid = node.getId(_sPlaceId);
    if (pid > ROOT_PLACE_ID) pPlace->fetchById(*pq, pid);// Valid
    else                     pPlace->clear();
    QString pt1 = portTypeList[states.passive][0];  // Első port alapértelmezett típusa (iftypes.iftype_name)
    switch (node.ports.size()) {
    case 0:   // Nincs port
        node.addPort(pt1, pt1, _sNul, NULL_IX);
        if (0 == pPort1()->chkObjType<cInterface>(EX_IGNORE)) {   // Interfész ?
            pPort1()->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC);
        }
        break;
    case 1: // Egy portunk van
        if (states.passive) {   // Passive node
            ;
        }
        else {                  // Aktív node : Workstation
            if (0 == node.ports.first()->chkObjType<cInterface>(EX_IGNORE)) {   // A port egy interfész ?
                if (pPort1()->reconvert<cInterface>()->addresses.isEmpty()) {     // Ha nincs címe, kap egy NULL, dinamikust
                    pPort1()->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC);
                }
            }
            else {                                                              // Nem interfész
                // Aktív eszköznek kell egy aktiv port (interfész), meg cím, a passzív lessz a második port
                cNPort *p = node.ports.takeFirst();
                node.addPort(pt1, pt1, _sNul, NULL_IX);
                pPort1()->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC);
                node.ports << p;
            }
        }
        break;
    case 2: // Két port van
        if (states.passive) EXCEPTION(EPROGFAIL);   // A szűrés alapján ez nem lehetséges
        // Első nem interfész, de a második igen
        if (node.ports.at(0)->tableoid() != cInterface::tableoid_interfaces()
         && node.ports.at(1)->tableoid() == cInterface::tableoid_interfaces()) {
            // Akkor felcsréljük
            cNPort *p = node.ports.takeFirst();
            node.ports << p;
        }
        // Az első port egy interfész
        if (node.ports.first()->tableoid() == cInterface::tableoid_interfaces()) {
            if (pPort1()->reconvert<cInterface>()->addresses.isEmpty()) {  // Nincs IP cím az első porton (sorrend véletlen szerű!!)
                if (pPort2()->tableoid() == cInterface::tableoid_interfaces()    // A második is interfész ?
                 && !pPort2()->reconvert<cInterface>()->addresses.isEmpty()) {   // neki van címe ?
                    // Akkor felcsréljük
                    cNPort *p = node.ports.takeFirst();
                    node.ports << p;
                }
                else {  // Nincs cím, csinálunk egy üreset:
                    pPort1()->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC);
                }
            }
        }
        else {  // Egyik sem interfész, ez elsőt megkamuzzuk mint interfész
            cNPort    *pn = node.ports.pop_front();
            cInterface*pi = new cInterface();
            pi->copy(*pn);
            delete pn;
            pi->setId(_sIfTypeId, cIfType::ifTypeId(_sEthernet));
            pi->addIpAddress(QHostAddress(), AT_DYNAMIC);
            node.ports.push_front(pi);
        }
        break;
    default:    // A konstatns szűrési feltétel alapján ez lehetetlen
        EXCEPTION(EPROGFAIL);
        break;
    }
    if ((pPort1()->chkObjType<cInterface>(EX_IGNORE) < 0) == (pIpAddress(EX_IGNORE) != NULL)) EXCEPTION(EPROGFAIL);

    // *** Link ***
    cPhsLink link;
    // Link port 1:
    if (link.isExist(*pq, pPort1()->getId(), LT_TERM)) {
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
    if (pPort2(EX_IGNORE) != NULL && link.isExist(*pq, pPort2()->getId(), LT_TERM)) {
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
    QString s;
    qlonglong id;
    int ix;
    // Node name
    s = node.getName();
    pUi->lineEditName->setText(s);
    states.nodeName = _checkNodeCollision(node.nameIndex(), s);
    // Node Note
    pUi->lineEditNote->setText(node.getNote());
    // Node Place
    if (pPlace->isValid()) {
        id = pPlace->getId();
        states.nodePlace = IS_OK;
    }
    else {
        id = UNKNOWN_PLACE_ID;
        states.nodePlace = IS_EMPTY;
    }
    _setPlaceComboBoxs(id, pUi->comboBoxZone, pUi->comboBoxPlace, true);
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
    s = pPort1()->getName();
    pUi->lineEditPName->setText(s);
    if (s.isEmpty()) EXCEPTION(EDATA);
    states.portName = IS_OK;
    // Port 1 note, tag
    s = pPort1()->getNote();
    pUi->lineEditPNote->setText(s);
    s = pPort1()->getName(_sPortTag);
    pUi->lineEditPTag->setText(s);
    // Port type
    s  = cIfType::ifTypeName(pPort1()->getId(_sIfTypeId));
    ix = pUi->comboBoxPType->findText(s);
    pUi->comboBoxPType->setCurrentIndex((ix >= 0) ? ix : 0);    // Nem a listában lávű típus, legyen a defíult (jobb mint kiakadni)
    _changePortType(true, ix);  // MAC and IP ADDRESS
    // *** PORT 2 ***
    if (pPort2(EX_IGNORE) == NULL) {   // Nincs második port
        _changePortType(false, -1);  // MAC
    }
    else {
        // Port 2 name
        s = pPort2()->getName();
        pUi->lineEditPName_2->setText(s);
        if (s.isEmpty()) EXCEPTION(EDATA);
        states.port2Name = IS_OK;
        // Port 2 note, tag
        s = pPort2()->getNote();
        pUi->lineEditPNote_2->setText(s);
        s = pPort2()->getName(_sPortTag);
        pUi->lineEditPTag_2->setText(s);
        // Port type
        s  = cIfType::ifTypeName(pPort2()->getId(_sIfTypeId));
        ix = pUi->comboBoxPType_2->findText(s);
        pUi->comboBoxPType_2->setCurrentIndex((ix >= 1) ? ix : 1);    // Nem a listában lávű típus, legyen a defíult (jobb mint kiakadni)
        _changePortType(false, ix);  // MAC
    }

    // *** Link ***
    // Link port 1:
    if (linkType  != LT_INVALID) {      // Van linkje
        id = pLinkNode->getId(_sPlaceId);
        _setPlaceComboBoxs(id, pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace, true);
        QString sPlace = pLinkPlaceModel->at(pUi->comboBoxLinkPlace->currentIndex());
        _changePlace(sPlace, pUi->comboBoxLinkZone->currentText(), pUi->comboBoxLinkNode, pModelLinkNode);
        s = pLinkNode->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkNode);
        _changeLinkNode(s, pModelLinkPort, pUi->comboBoxLinkPort);
        pUi->lineEditLinkNodeType->setText(pLinkNode->getName(_sNodeType));
        id = pLinkPort->getId();
        s  = pLinkPort->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkPort, pModelLinkPort);

        cIfType iftype = cIfType::ifType(pLinkPort->getId(_sIfTypeId));
        eTristate isPatch = iftype.getName() == _sPatch ? TS_TRUE : TS_FALSE;
        states.link = _changeLinkPort(id, pLinkButtonsLinkType, pUi->comboBoxLinkPortShare, isPatch);
        if (isPatch == TS_TRUE) {
            _setCurrentIndex(portShare(linkShared), pUi->comboBoxLinkPortShare);
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
        states.link = _changeLinkPort(NULL_ID, pLinkButtonsLinkType, pUi->comboBoxLinkPortShare, TS_NULL);
    }
    // Link port 2:
    if (linkType2 != LT_INVALID) {      // Van linkje
        id = pLinkNode2->getId(_sPlaceId);
        _setPlaceComboBoxs(id, pUi->comboBoxLinkZone_2, pUi->comboBoxLinkPlace_2, true);
        QString sPlace = pLinkPlaceModel2->at(pUi->comboBoxLinkPlace_2->currentIndex());
        _changePlace(sPlace, pUi->comboBoxLinkZone_2->currentText(), pUi->comboBoxLinkNode_2, pModelLinkNode2);
        s = pLinkNode2->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkNode_2);
        _changeLinkNode(s, pModelLinkPort2, pUi->comboBoxLinkPort_2);
        pUi->lineEditLinkNodeType_2->setText(pLinkNode2->getName(_sNodeType));
        id = pLinkPort2->getId();
        s  = pLinkPort2->getName();
        _setCurrentIndex(s, pUi->comboBoxLinkPort_2, pModelLinkPort2);

        cIfType iftype = cIfType::ifType(pLinkPort2->getId(_sIfTypeId));
        eTristate isPatch = iftype.getName() == _sPatch ? TS_TRUE : TS_FALSE;
        states.link2 = _changeLinkPort(id, pLinkButtonsLinkType2, pUi->comboBoxLinkPortShare_2, isPatch);
        if (isPatch == TS_TRUE) {
           _setCurrentIndex(portShare(linkShared2), pUi->comboBoxLinkPortShare_2);
            pUi->comboBoxLinkPortShare_2->setEnabled(true);
            if (linkType2 == LT_BACK) {  // Defaultként front lett beállítva
                pUi->radioButtonLinkBack_2->setChecked(true);
            }
        }
        _setLinkType(false);
    }
    else {
        _changeLinkNode(_sNul, pModelLinkPort2, pUi->comboBoxLinkPort_2);
        pUi->lineEditLinkNodeType_2->setText(_sNul);
        states.link2 = _changeLinkPort(NULL_ID, pLinkButtonsLinkType2, pUi->comboBoxLinkPortShare_2, TS_NULL);
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
        if (pPort1()->chkObjType<cInterface>(EX_IGNORE) == 0) {
            pPort1()->reconvert<cInterface>()->addresses.clear();
        }
        states.ipNeed = IS_NOT_POSSIBLE;
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
    if (pIpAddress(EX_IGNORE) == NULL) {
        pPort1()->reconvert<cInterface>()->addIpAddress(QHostAddress(), sType);
    }
    else {
        pIpAddress()->setName(cIpAddress::ixIpAddressType(), sType);
    }
    switch (pIpAddress()->getId(cIpAddress::ixIpAddressType())) {
    case AT_FIXIP:
        states.ipNeed = IS_MUST;
        break;
    case AT_DYNAMIC:
        states.ipNeed = IS_POSSIBLE;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    if (sAddr.isEmpty()) {
        pIpAddress()->clear(_sAddress);
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
                        addrCollisionInfo = htmlInfo(trUtf8("A %1 cím a %2 nevű eszköz címével ötközik").arg(a.toString(), no.getName()));
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
                    pIpAddress()->setAddress(a);
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
    if (pIpAddress(EX_IGNORE) == NULL) return;
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
            sni = ix +1;
        }
    }
    if (psn != NULL) {
        pIpAddress()->setId(_sSubNetId, psn->getId());
    }
    else {
        pIpAddress()->setId(_sSubNetId, NULL_ID);
    }
    if (pvl != NULL) {
        if (pPort1()->reconvert<cInterface>()->vlans.isEmpty()) {
            pPort1()->reconvert<cInterface>()->vlans << new cPortVlan();
        }
        cPortVlan& pv = *pPort1()->reconvert<cInterface>()->vlans.first();
        pv.setId(_sVlanId, pvl->getId());
        pv.setId(_sVlanType, VT_HARD);
        pv.setId(_sSetType, ST_MANUAL);
    }
    else {
        pPort1()->reconvert<cInterface>()->vlans.clear();
    }
    LOCKSLOTS();
    pUi->comboBoxSubNet->setCurrentIndex(sni);
    pUi->comboBoxSubNetAddr->setCurrentIndex(sni);
    pUi->comboBoxVLan->setCurrentIndex(vli);
    pUi->comboBoxVLanId->setCurrentIndex(vli);
    UNLOCKSLOTS();
}

bool cWorkstation::_changePortType(bool primary, int cix)
{
    if (cix < 0 || (!primary && cix == 0)) {  // Nincs port
        if (primary) EXCEPTION(EDATA);  // Elsődlegesnek kell lennie
        if (node.ports.size() > 1) delete node.ports.pop_back();    // Ha volt töröljük
        if (node.ports.size() != 1) EXCEPTION(EPROGFAIL);           // Pont egy portnak kell lennie
        linkType2 = LT_INVALID;  pLinkNode2->clear();    pLinkPort2->clear();
        LOCKSLOTS();
        pModelLinkPort2->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres
        _setLinkType(false);
        UNLOCKSLOTS();
        states.port2Need     = false;
        states.link2Possible = false;
        states.mac2need      = false;
        // Minden másodlagos port bozbaszt tiltani kell
        WENABLE(lineEditPName_2, false);
        WENABLE(lineEditPNote_2, false);
        WENABLE(lineEditPTag_2, false);
        WENABLE(lineEditPMAC_2, false);
        WENABLE(comboBoxLinkZone_2, false);
        WENABLE(comboBoxLinkPlace_2, false);
        WENABLE(checkBoxPlaceEqu_2, false);
        WENABLE(comboBoxLinkNode_2, false);
        WENABLE(comboBoxLinkPort_2, false);
        WENABLE(pushButtonNewPatch_2, false);
        WENABLE(lineEditLinkNodeType_2, false);
        WENABLE(comboBoxLinkPortShare_2, false);
        WENABLE(pushButtonLinkOther_2, false);
        WENABLE(lineEditLinkNote_2, false);
        return false;
    }
    cNPort *pPort = primary ? pPort1() : pPort2(EX_IGNORE);
    int ix = primary ? 0 : 1;   // port indexe a port konténerben
    QString sType = (primary ? pUi->comboBoxPType : pUi->comboBoxPType_2)->itemText(cix);
    const cIfType& iftype = cIfType::ifType(sType);
    if (pPort == NULL || iftype.getId() != pPort->getId(_sIfTypeId)) {    // Megváltozott az objektum típusa!!
        cNPort *pPortNew = cNPort::newPortObj(iftype);
        if (pPort != NULL) {
            node.ports[ix] = pPortNew;  // csere
            pPortNew->set(*pPort);      // Amit lehet átmásoljuk
            delete pPort;
        }
        else {  // Ha NULL, akkor az a port2 / primary == false
            if (ix != 1 && node.ports.size() != 1) EXCEPTION(EDATA);
            pPortNew->setName(sType);
            node.ports << pPortNew;
            pUi->lineEditPName_2->setText(sType);
            states.port2Name = sType == pPort1()->getName() ? IS_COLLISION : IS_OK;
        }
        pPort = pPortNew;
    }
    pPort->setId(_sIfTypeId, iftype.getId());  // Nem irtuk még ide, vagy elrontottuk
    bool isIface = iftype.getId(_sIfTypeObjType) == PO_INTERFACE;
    if (primary) {
        states.macNeed = isIface;
        if (isIface) {
            QString sType = _sDynamic;
            QString sAddr = _sNul;
            qlonglong six = 0;
            if (pIpAddress(EX_IGNORE) != NULL) {
                sType = pIpAddress()->getName(_sIpAddressType);
                sAddr = pIpAddress()->getName(_sAddress);
                qlonglong sid = pIpAddress()->getId(_sSubNetId);
                if (sid != NULL_ID) {
                    QString subn  = cSubNet().getNameById(*pq, sid);
                    six = pUi->comboBoxSubNet->findText(subn);
                }
            }
            pUi->lineEditAddress->setText(sAddr);
            _addressChanged(sType, sAddr);
            _subNetVLan(six, -1);
            QString s = pPort1()->getName(_sHwAddress);
            pUi->lineEditPMAC->setText(s);
            states.mac = _macStat(s);
        }
        else {
            _addressChanged(_sNul);
            _subNetVLan(0, 0);
        }
        WENABLE(lineEditPMAC, isIface);
        WENABLE(lineEditAddress, isIface);
    }
    else {
        states.port2Need = true;
        states.mac2need  = isIface;
        if (isIface) {
            QString s = pPort1()->getName(_sHwAddress);
            pUi->lineEditPMAC_2->setText(s);
            states.mac2 = _macStat(s);
        }
        WENABLE(lineEditPName_2, true);
        WENABLE(lineEditPNote_2, true);
        WENABLE(lineEditPTag_2, true);
        WENABLE(lineEditPMAC_2, isIface);
    }
    int lt = iftype.getId(_sIfTypeLinkType);
    bool linkPossible = lt == LT_PTP || lt == LT_BUS;
    bool linkPlacePsb;
    if (primary) {
        states.linkPossible = linkPossible;
        linkPlacePsb = linkPossible && !(pUi->checkBoxPlaceEqu->isChecked());
        WENABLE(comboBoxLinkZone, linkPlacePsb);
        WENABLE(comboBoxLinkPlace, linkPlacePsb);
        WENABLE(checkBoxPlaceEqu, linkPossible);
        WENABLE(comboBoxLinkNode, linkPossible);
        WENABLE(comboBoxLinkPort, linkPossible);
        WENABLE(pushButtonNewPatch, linkPossible);
        WENABLE(lineEditLinkNodeType, linkPossible);
        WENABLE(comboBoxLinkPortShare, false);
        WENABLE(pushButtonLinkOther, linkPossible);
        WENABLE(lineEditLinkNote, linkPossible);
    }
    else {
        states.link2Possible = linkPossible;
        linkPlacePsb = linkPossible && !(pUi->checkBoxPlaceEqu_2->isChecked());
        WENABLE(comboBoxLinkZone_2, linkPlacePsb);
        WENABLE(comboBoxLinkPlace_2, linkPlacePsb);
        WENABLE(checkBoxPlaceEqu_2, linkPossible);
        WENABLE(comboBoxLinkNode_2, linkPossible);
        WENABLE(comboBoxLinkPort_2, linkPossible);
        WENABLE(pushButtonNewPatch_2, linkPossible);
        WENABLE(lineEditLinkNodeType_2, linkPossible);
        WENABLE(comboBoxLinkPortShare_2, false);
        WENABLE(pushButtonLinkOther_2, linkPossible);
        WENABLE(lineEditLinkNote_2, linkPossible);
    }

/*    pUi->checkBoxSrvIns->setEnabled(states.linkPossible);   // !!!
    pUi->checkBoxSrvDel->setEnabled(states.linkPossible);
    pUi->checkBoxSrvEna->setEnabled(states.linkPossible);
    pUi->lineEditSrvNote->setEnabled(states.linkPossible);*/
    return linkPossible;
}

/*
void cWorkstation::_changeLink(bool primary)
{
    qlonglong pid, lpid;
    if (primary) {
        pid = pPort1->getId();
    }
}
*/
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
    states.ipNeed  = IS_NOT_POSSIBLE;
    states.ipAddr  = IS_EMPTY;
    states.subNetNeed = false;
    node.ports.clear();
    QString t = portTypeList[id].first();       // Az alapértelmezett port típus neve
    node.addPort(t, t, _sNul, NULL_IX);// Hozzáadjuk a portot (alapértelmezett név a típus)
    switch (cIfType::ifType(t).getId(_sIfTypeObjType)) {
    case PO_INTERFACE:                          // Kell cím objektum
        pPort1()->reconvert<cInterface>()->addIpAddress(QHostAddress(), AT_DYNAMIC);
        break;
    case PO_NPORT:                              // Nem kell cím objektum
        break;
    case PO_PPORT:                              // Ez nem lehetséges
        EXCEPTION(EDATA);
    }
    pModelNode->setConstFilter(nodeListSql[id], FT_SQL_WHERE);  // A node típustol függő szűrés
    pModelNode->setFilter();                    // Frissítés. A konstans szűrő hívása nem frissít
    bool fip = pIpAddress(EX_IGNORE) != NULL;   // Ha ninbcs IP, a kapcolódó widgetek tiltása, ha van engedélyezése
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
    if (pPort1()->isIndex(_sHwAddress)) macAddressChanged(pPort1()->getName(_sHwAddress));
    if (pIpAddress(EX_IGNORE) != NULL) ipAddressChanged(pIpAddress()->getName(cIpAddress::ixAddress()));
    states.link = _linkTest(*pq, states.link, states.modify, pPort1()->getId(), pLinkPort->getId(), linkType, linkShared, linkInfoMsg);
    if (pPort2(EX_IGNORE) != NULL) {
        states.link2 = _linkTest(*pq, states.link2, states.modify, pPort2()->getId(), pLinkPort2->getId(), linkType2, linkShared2, linkInfoMsg2);
    }
    ENDSLOTM();
}

void cWorkstation::save()
{
    DBGFN();
    // Néhány érték nem íródik automatikusan az objektumba
    node.setNote(pUi->lineEditNote->text());
    pPort1()->setNote(pUi->lineEditPNote->text());
    pPort1()->setName(_sPortTag, pUi->lineEditPTag->text());
    if (pIpAddress(EX_IGNORE) != NULL) {
        pIpAddress()->setNote(pUi->lineEditIpNote->text());
    }
    if (pPort2(EX_IGNORE) != NULL) {
        pPort2()->setNote(pUi->lineEditPNote_2->text());
        pPort1()->setName(_sPortTag, pUi->lineEditPTag_2->text());
    }
    cNode *pSave = node.dup()->reconvert<cNode>();// Hiba esetén vissza kell állítani az eredetit
    cError *pe = NULL;
    const static QString tkey = "Workstation_save";
    node.setId(_sNodeType, ENUM2SET2(NT_HOST, NT_WORKSTATION));
    QString p1name = pPort1()->getName();
    int on = node.ports.size();
    if (on != 1 && on != 2) EXCEPTION(EPROGFAIL, on);
    try {
        PDEB(VERBOSE) << trUtf8("Start try (transaction %1)...").arg(tkey) << endl;
        // Az írási műveletnél, a ports konténer törlődik, újraolvasáskor pedig más lehet a sorrend!
        sqlBegin(*pq, tkey);
        if (states.modify) {
            PDEB(VERBOSE) << trUtf8("Modify (rewriteById) : ") << node.identifying() << endl;
            node.rewriteById(*pq);
        }
        else {
            node.clearIdsOnly(); // Az ID-ket törölni kell
            PDEB(VERBOSE) << trUtf8("Insert : ") << node.identifying() << endl;
            node.insert(*pq);
        }
        node.fetchPorts(*pq);                   // A portokat újra visszaolvassuk
        static const QString  cm = trUtf8("A munkaállomás adatainak a kiírása után, a visszaolvasott adatok hibásak : ");
        int n = node.ports.size();
        if (n != on) {
            EXCEPTION(EDATA, 2, cm + trUtf8("Kiírtunk %1 db. portot, de %2 portot olvastubk vissza.").arg(on).arg(n));
        }
        if (pPort1()->getName() != p1name) {    // Ha megváltozott a sorrend ?
            if (pPort2(EX_IGNORE) == NULL) {
                EXCEPTION(EDATA, 2, cm + trUtf8("Csak egy port van, de annak nem egyezik a neve a kiírttal."));
            }
            if (pPort2()->getName() != p1name) {
                EXCEPTION(EDATA, 2, cm + trUtf8("Két port van, de egyik neve sem egyezik a kiírt elő port nevével."));
            }
            // Felcseréljük
            cNPort *p = node.ports.takeFirst();
            node.ports << p;
        }
        if (linkType != LT_INVALID) {
            cPhsLink link;
            link.setId(_sPortId1,       pPort1()->getId());
            link.setId(_sPhsLinkType1,  LT_TERM);
            link.setId(_sPortId2,       pLinkPort->getId());
            link.setId(_sPhsLinkType2,  linkType);
            link.setId(_sPortShared,    linkShared);
            QString s = pUi->lineEditLinkNote->text();
            if (!s.isEmpty()) link.setNote(s);
            link.setId(_sCreateUserId,  lanView::user().getId());
            PDEB(VERBOSE) << trUtf8("Save link #1 : ") << link.toString() << endl;
            link.replace(*pq);
        }
        if (linkType2 != LT_INVALID) {
            cPhsLink link;
            link.setId(_sPortId1,       pPort2()->getId());
            link.setId(_sPhsLinkType1,  LT_TERM);
            link.setId(_sPortId2,       pLinkPort2->getId());
            link.setId(_sPhsLinkType2,  linkType2);
            link.setId(_sPortShared,    linkShared2);
            QString s = pUi->lineEditLinkNote_2->text();
            if (!s.isEmpty()) link.setNote(s);
            link.setId(_sCreateUserId,  lanView::user().getId());
            PDEB(VERBOSE) << trUtf8("Save link #1 : ") << link.toString() << endl;
            link.replace(*pq);
        }
        PDEB(VERBOSE) << trUtf8("End transaction : ") << tkey << endl;
        sqlCommit(*pq, tkey);
    } CATCHS(pe);
    if (pe != NULL) {
        DERR() << trUtf8("Error exception : ") << pe->msg() << endl;
        sqlRollback(*pq, tkey);
        node.clone(*pSave);  // Visszaállítjuk
        cErrorMessageBox::condMsgBox(pe, this);
    }
    delete pSave;
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
    int pix = _changeZone(pUi->comboBoxFilterPlace, s);
    filterPlaceCurrentIndex(pix);
    ENDSLOT();
}

void cWorkstation::filterPlaceCurrentIndex(int i)
{
    BEGINSLOT(i);
    QString patt       = pUi->lineEditFilterPattern->text();
    QString zone_name  = pUi->comboBoxFilterZone->currentText();
    QString place_name;
    if (i > 0) {
        place_name = pUi->comboBoxFilterPlace->currentText();
    }
    _changePlace(place_name, zone_name, pUi->comboBoxNode, pModelNode, patt);
    ENDSLOT();
}

void cWorkstation::filterPatternChanged(const QString& s)
{
    _DBGFN() << " (" << s << ")" << endl;
    QString place_name;
    QString zone_name;
    if (pUi->comboBoxFilterZone->currentIndex() > 0) {  // NOT ALL
        zone_name = pUi->comboBoxFilterZone->currentText();
    }
    if (pUi->comboBoxFilterPlace->currentIndex() > 0) { // NOT NULL
        place_name = pUi->comboBoxFilterPlace->currentText();
    }
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
    int pix = _changeZone(pUi->comboBoxPlace, s, sPlace);
    placeCurrentIndex(pix);
    ENDSLOTM();
}

void cWorkstation::placeCurrentIndex(int i)
{
    BEGINSLOT(i);
    if (i == 0) {   // Nincs kiválasztva hely ?
        pPlace->clear();
        states.nodePlace = IS_EMPTY;
    }
    else {
        QString s = pUi->comboBoxPlace->currentText();
        pPlace->setByName(*pq, s);
        states.nodePlace = IS_OK;
    }
    node.setId(_sPlaceId, pPlace->getId());
    if (pUi->checkBoxPlaceEqu->isChecked())   _linkToglePlaceEqu(true,  TS_TRUE);   // Elsődleges
    if (pUi->checkBoxPlaceEqu_2->isChecked()) _linkToglePlaceEqu(false, TS_TRUE);   // Mésodlagos
    ENDSLOTM();
}

void cWorkstation::nodeAddPlace()
{
    cRecord *pRec = recordDialog(*pq, _sPlaces, this);
    if (pRec == NULL) return;
    qlonglong pid = pRec->getId(_sPlaceId);
    LOCKSLOTS();
    int pix = _setPlaceComboBoxs(pid, pUi->comboBoxZone, pUi->comboBoxPlace, true);
    UNLOCKSLOTS();
    delete pRec;
    placeCurrentIndex(pix);
}

// Workstation ports slots
void cWorkstation::portNameChanged(const QString& s)
{
    BEGINSLOT(s);
    pPort1()->setName(s);
    states.portName = s.isEmpty() ? IS_EMPTY : IS_OK;
    ENDSLOTM();
}

void cWorkstation::portTypeCurrentIndex(int i)
{
    BEGINSLOT(i);
    if (_changePortType(true, i)) { // Ha lehet link
        linkNodeCurrentIndex(pUi->comboBoxLinkNode->currentIndex());
    }
    ENDSLOTM();
}

int cWorkstation::_macStat(const QString& s)
{
    if (s.isEmpty()) {
        return IS_EMPTY;
    }
    else {
        cMac mac(s);
        if (mac.isValid()) {
            cNode no;
            if (no.fetchByMac(*pq, mac) && !(states.modify && pSample->getId() == no.getId())) {
                return IS_COLLISION;
            }
            else {
                return IS_OK;
            }
        }
        else {
            return IS_INVALID;
        }
    }
}

void cWorkstation::macAddressChanged(const QString& s)
{
    BEGINSLOT(s);
    int i = _macStat(s);
    states.mac = i;
    switch (i) {
    case IS_EMPTY:
    case IS_COLLISION:
    case IS_INVALID:
        pPort1()->clear(_sHwAddress);
        break;
    case IS_OK:
        pPort1()->setName(_sHwAddress, s);
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    ENDSLOTM();
}

// Workstation address slots
void cWorkstation::ipAddressChanged(const QString& s)
{
    if (pIpAddress(EX_IGNORE) == NULL) return;
    BEGINSLOT(s);
    QString sType = pIpAddress()->getName(_sIpAddressType);
    _addressChanged(sType, s);
    ENDSLOTM();
}

void cWorkstation::ipAddressTypeCurrentIndex(const QString& s)
{
    if (pIpAddress(EX_IGNORE) == NULL) return;
    BEGINSLOT(s);
    QString sAddr = pIpAddress()->getName(_sAddress);
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
        states.link = _linkTest(*pq, states.link, states.modify, pPort1()->getId(), pLinkPort->getId(), linkType, linkShared, linkInfoMsg);
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
    int pix = _changeZone(pUi->comboBoxLinkPlace, s);
    linkPlaceCurrentIndex(pix);
    ENDSLOTM();
}

void cWorkstation::linkPlaceCurrentIndex(int i)
{
    BEGINSLOT(i);
    QString sPlace = pLinkPlaceModel->at(i);
    int ix = _changePlace(sPlace, pUi->comboBoxLinkZone->currentText(), pUi->comboBoxLinkNode, pModelLinkNode);
    pUi->comboBoxLinkNode->setCurrentIndex(ix);
    ENDSLOTM();
}

void cWorkstation::linkNodeCurrentIndex(int i)
{
    BEGINSLOT(i);
    if (!states.linkPossible && i != 0) {
        pUi->comboBoxLinkNode->setCurrentIndex(0);
        return;
    }
    QString s = pModelLinkNode->at(i);
    states.link = _changeLinkNode(s, pModelLinkPort, pUi->comboBoxLinkPort, pLinkNode, pLinkPort, &linkType, &linkShared);
    if (i == 0) {
        pUi->lineEditLinkNodeType->setText(_sNul);
    }
    else {
        pUi->lineEditLinkNodeType->setText(pLinkNode->getName(_sNodeType));
    }
    ENDSLOTM();
}


void cWorkstation::linkPortCurrentIndex(int i)
{
    BEGINSLOT(i);
    qlonglong id = pModelLinkPort->atId(i);
    states.link = _changeLinkPort(id, pLinkButtonsLinkType, pUi->comboBoxLinkPortShare, TS_NULL, pLinkPort, &linkType, &linkShared);
    states.link = _linkTest(*pq, states.link, states.modify, pPort1()->getId(), pLinkPort->getId(), linkType, linkShared, linkInfoMsg);
    ENDSLOTM();
}

void cWorkstation::linkPortShareCurrentIndex(const QString &s)
{
    BEGINSLOT(s);
    linkShared = (ePortShare)portShare(s);
    states.link = _linkTest(*pq, states.link, states.modify, pPort1()->getId(), pLinkPort->getId(), linkType, linkShared, linkInfoMsg);
    ENDSLOTM();
}

void cWorkstation::newPatch()
{
    cPatch *pSample = NULL;
    int ix = pUi->comboBoxLinkPlace->currentIndex();
    if (ix > 0) {
        qlonglong pid = pLinkPlaceModel->atId(ix);
        pSample = new cPatch;
        pSample->setId(_sPlaceId, pid);
    }
    cPatch *p = patchDialog(*pq, this, pSample);
    pDelete(pSample);
    if (p == NULL) return;
    LOCKSLOTS();
    qlonglong pid = p->getId(_sPlaceId);
    _setPlaceComboBoxs(pid, pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace, true);
    QString placeName = cPlace().getNameById(*pq, pid);
    QString zoneName  = pUi->comboBoxLinkZone->currentText();
    _changePlace(placeName, zoneName, pUi->comboBoxLinkNode, pModelLinkNode);
    UNLOCKSLOTS();
    if (pUi->comboBoxPlace->currentText() != pUi->comboBoxLinkPlace->currentText()) {
        pUi->checkBoxPlaceEqu->setChecked(false);
    }
    _setCurrentIndex(p->getName(), pUi->comboBoxLinkNode);
    delete p;
}


// Workstation port 2 slots
void cWorkstation::portNameChanged2(const QString& s)
{
    BEGINSLOT(s);
    pPort2()->setName(s);
    if      (s.isEmpty())               states.port2Name = IS_EMPTY;
    else if (s == pPort1()->getName())  states.port2Name = IS_COLLISION;
    else                                states.port2Name = IS_OK;
    ENDSLOTM();
}

void cWorkstation::portTypeCurrentIndex2(int i)
{
    BEGINSLOT(i);
    if (_changePortType(false, i)) {
        linkNodeCurrentIndex2(pUi->comboBoxLinkNode_2->currentIndex());
    }
    ENDSLOTM();
}

void cWorkstation::macAddressChanged2(const QString& s)
{
    BEGINSLOT(s);
    int i = _macStat(s);
    states.mac = i;
    switch (i) {
    case IS_EMPTY:
    case IS_COLLISION:
    case IS_INVALID:
        pPort2()->clear(_sHwAddress);
        break;
    case IS_OK:
        pPort2()->setName(_sHwAddress, s);
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    ENDSLOTM();
}

// LINK 2 slots
void cWorkstation::linkChangeLinkType2(int id, bool f)
{
    BEGINSLOT(QString::number(id) + ", " + DBOOL(f));
    if (f && id != linkType2) {
        linkType2 = (ePhsLinkType)id;
        states.link2 = _linkTest(*pq, states.link2, states.modify, pPort2()->getId(), pLinkPort2->getId(), linkType2, linkShared2, linkInfoMsg2);
    }
    ENDSLOTM();
}

void cWorkstation::linkToglePlaceEqu2(bool f)
{
    BEGINSLOT(DBOOL(f));
    _linkToglePlaceEqu(false, f);
    ENDSLOT();
}

void cWorkstation::linkZoneCurrentIndex2(const QString &s)
{
    BEGINSLOT(s);
    int pix = _changeZone(pUi->comboBoxLinkPlace_2, s);
    linkPlaceCurrentIndex2(pix);
    ENDSLOTM();
}

void cWorkstation::linkPlaceCurrentIndex2(int i)
{
    BEGINSLOT(i);
    QString sPlace = pLinkPlaceModel2->at(i);
    int ix = _changePlace(sPlace, pUi->comboBoxLinkZone_2->currentText(), pUi->comboBoxLinkNode_2, pModelLinkNode2);
    pUi->comboBoxLinkNode_2->setCurrentIndex(ix);
    ENDSLOTM();
}

void cWorkstation::linkNodeCurrentIndex2(int i)
{
    BEGINSLOT(i);
    if (!states.link2Possible && i != 0) {
        pUi->comboBoxLinkNode_2->setCurrentIndex(0);
        return;
    }
    QString s = pModelLinkNode2->at(i);
    states.link2 = _changeLinkNode(s, pModelLinkPort2, pUi->comboBoxLinkPort_2, pLinkNode2, pLinkPort2, &linkType2, &linkShared2);
    if (i == 0) {
        pUi->lineEditLinkNodeType_2->setText(_sNul);
    }
    else {
        pUi->lineEditLinkNodeType_2->setText(pLinkNode2->getName(_sNodeType));
    }
    ENDSLOTM();
}

void cWorkstation::linkPortCurrentIndex2(int i)
{
    BEGINSLOT(i);
    qlonglong id = pModelLinkPort2->atId(i);
    states.link2 = _changeLinkPort(id, pLinkButtonsLinkType2, pUi->comboBoxLinkPortShare_2, TS_NULL, pLinkPort2, &linkType2, &linkShared2);
    states.link2 = _linkTest(*pq, states.link2, states.modify, pPort2()->getId(), pLinkPort2->getId(), linkType2, linkShared2, linkInfoMsg2);
    ENDSLOTM();
}

void cWorkstation::linkPortShareCurrentIndex2(const QString &s)
{
    BEGINSLOT(s);
    linkShared2 = (ePortShare)portShare(s);
    states.link2 = _linkTest(*pq, states.link2, states.modify, pPort2()->getId(), pLinkPort2->getId(), linkType2, linkShared2, linkInfoMsg2);
    ENDSLOTM();
}

void cWorkstation::newPatch2()
{
    cPatch *pSample = NULL;
    int ix = pUi->comboBoxLinkPlace_2->currentIndex();
    if (ix > 0) {
        qlonglong pid = pLinkPlaceModel2->atId(ix);
        pSample = new cPatch;
        pSample->setId(_sPlaceId, pid);
    }
    cPatch *p = patchDialog(*pq, this, pSample);
    pDelete(pSample);
    if (p == NULL) return;
    LOCKSLOTS();
    qlonglong pid = p->getId(_sPlaceId);
    _setPlaceComboBoxs(pid, pUi->comboBoxLinkZone_2, pUi->comboBoxLinkPlace_2, true);
    QString placeName = p->getName();
    QString zoneName  = pUi->comboBoxLinkZone_2->currentText();
    _changePlace(placeName, zoneName, pUi->comboBoxLinkNode_2, pModelLinkNode2);
    UNLOCKSLOTS();
    if (pUi->comboBoxPlace->currentText() != pUi->comboBoxLinkPlace_2->currentText()) {
        pUi->checkBoxPlaceEqu_2->setChecked(false);
    }
    _setCurrentIndex(p->getName(), pUi->comboBoxLinkNode_2);
    delete p;
}

cNPort *cWorkstation::pPort1(eEx __ex)
{
    if (node.ports.size() >= 1) return  node.ports.at(0);
    if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
    return NULL;
}

cIpAddress *cWorkstation::pIpAddress(eEx __ex)
{
    cNPort *p = pPort1(EX_IGNORE);
    if (p == NULL) {
        if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
        return NULL;
    }
    cInterface *pi = p->reconvert<cInterface>();
    if (pi->addresses.size() < 1) {
        if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
        return NULL;
    }
    return pi->addresses.at(0);
}

cNPort     *cWorkstation::pPort2(eEx __ex)
{
    if (node.ports.size() >= 2) return  node.ports.at(1);
    if (__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
    return NULL;
}
#endif

void cWorkstation::on_comboBoxZone_currentIndexChanged(int index)
{
    if (pUi->checkBoxPlaceEqu->isCheckable()) pUi->comboBoxLinkZone->setCurrentIndex(index);
}

void cWorkstation::on_lineEditPlacePat_textChanged(const QString &arg1)
{
    if (pUi->checkBoxPlaceEqu->isCheckable()) pUi->lineEditLinkPlacePat->setText(arg1);
}


void cWorkstation::on_comboBoxPlace_currentIndexChanged(int index)
{
    if (pUi->checkBoxPlaceEqu->isCheckable()) pUi->comboBoxLinkPlace->setCurrentIndex(index);
}

void cWorkstation::on_checkBoxPlaceEqu_toggled(bool checked)
{
    if (checked) {
        pSelPlaceLink->copyCurrents(*pSelPlace);
        pSelPlaceLink->setDisabled();
    }
    else {
        pSelPlaceLink->setEnabled();
    }
}

void cWorkstation::on_comboBoxNode_currentIndexChanged(int index)
{
    if (index <= 0) {   // NULL
        pSample->clear();
    }
    else {
        qlonglong nid = pSelNode->currentNodeId();
        pSample->setById(*pq, nid);
        node.copy(*pSample);
        node.fetchPorts(*pq, CV_PORTS_ADDRESSES);
    }
}
