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
    pUi->setupUi(this);

    pWorkstation->setId(_sNodeType, NT_WORKSTATION);
    pIpAddress->  setId(_sIpAddressType, AT_DYNAMIC);
    pInterface->addresses << pIpAddress;
    pWorkstation->ports   << pInterface;
    pPlace->setById(0);     // NAME = 'unknown'

    {   // Nem üres zónák listája
        static const QString sql =
                "SELECT place_group_name FROM place_groups"
                " WHERE place_group_type = 'zone'"
                 " AND 0 < (SELECT COUNT(*) FROM place_group_places WHERE place_group_places.place_group_id = place_groups.place_group_id)"
                " ORDER BY place_group_name ASC";
        // Az összes zóna neve (place_groups rekord, zone_type = 'zone', betürendben, de az all az első
        if (!execSql(*pq, sql)) {
            EXCEPTION(EDATA, 0, "No any zone.");
        }
        do {
            QString name = pq->value(0).toString();
            if (name != _sAll) {
                zones << name;
            }
        } while (pq->next());
        // Az "all" zónába tartozó, vagyis az összes hely listája
        cPlace pl;
        QStringList& zoneAll = mapZones[_sAll];
        // Az összes hely, helyiség, betűrendben, de unknown az első, és a root kimarad
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
    // Workstation típusú nod-ok:
    pUi->comboBoxName->setModel(pModelLinkNode);
    pModelNode->setConstFilter(quoted(_sWorkstation) + " = ANY " + parentheses(_sNodeType), FT_SQL_WHERE);
    pModelNode->nullable = true;    // Az első elem a semmi lessz
    pModelNode->sFkeyName = _sPlaceId;
    pModelNode->setFilter(0, OT_ASC, FT_FKEY_ID);   // Az unknown (ID = 0) helyen lévő gépek,mert majd az lessz az első/kiválasztott helyiség
    pUi->comboBoxName->setCurrentIndex(0);  // első elem, NULL kiválasztva

    pUi->comboBoxZone->addItems(zones);
    pUi->comboBoxZone->setCurrentIndex(0);
    pUi->comboBoxPlace->addItems(mapZones[_sAll]);
    pUi->comboBoxPlace->setCurrentIndex(0);

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
    pModelLinkNode->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott hely, a lista üres lessz

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
    pModelLinkNode2->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott hely, a lista üres lessz

    pUi->comboBoxLinkPort_2->setModel(pModelLinkPort2);
    pModelLinkPort2->setFilter(NULL_ID, OT_ASC, FT_FKEY_ID); // Nincs kiválasztott node, a lista üres

    // Workstation:
    connect(pUi->comboBoxZone,          SIGNAL(currentIndexChanged(QString)), this, SLOT(zoneCurrentIndex(QString)));
    connect(pUi->comboBoxPlace,         SIGNAL(currentIndexChanged(QString)), this, SLOT(placeCurrentIndex(QString)));

    // Link:
    connect(pLinkButtonsLinkType,       SIGNAL(buttonToggled(int,bool)),      this, SLOT(linkChangeLinkType(int, bool)));
    connect(pUi->comboBoxLinkZone,      SIGNAL(currentIndexChanged(QString)), this, SLOT(linkZoneCurrentIndex(QString)));
    connect(pUi->comboBoxLinkPlace,     SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPlaceCurrentIndex(QString)));
    connect(pUi->comboBoxLinkNode,      SIGNAL(currentIndexChanged(int)),     this, SLOT(linkNodeCurrentIndex(int)));
    connect(pUi->comboBoxLinkPort,      SIGNAL(currentIndexChanged(int)),     this, SLOT(linkPortCurrentIndex(int)));
    connect(pUi->comboBoxLinkPortShare, SIGNAL(currentIndexChanged(QString)), this, SLOT(linkPortShareCurrentIndex(QString)));
    connect(pUi->checkBoxPlaceEqu,      SIGNAL(toggled(bool)),                this, SLOT(linkToglePlaceEqu(bool)));

    dataOkBits = enum2set(DO_IP, DO_PORT_NAME); // Ezekre van alapértelmezett érték
}

cWorkstation::~cWorkstation()
{
    delete pq;
    delete pWorkstation;
    delete pLinkNode;
    delete pLinkPrt;
    delete pPlace;
}

const QStringList& cWorkstation::placesInZone(const QString& sZone)
{
    QStringList& places = mapZones[sZone];
    if (places.isEmpty()) {
        const QString sql =
                "SELECT places.* FROM place_group_places"
                " JOIN places USING(place_id)"
                " WHERE place_group_id = (SELECT place_group_id FROM place_groups WHERE place_group_name = ?)"
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

// Workstation (node) slots
void cWorkstation::nodeCurrentIndex(int)
{

}

void cWorkstation::nodeNameChanged(const QString& s)
{

}

void cWorkstation::zoneCurrentIndex(const QString& s)
{
    pUi->comboBoxPlace->clear();
    const QStringList& pl = placesInZone(s);
    pUi->comboBoxPlace->addItems(pl);
    int i = pl.indexOf(pPlace->getName());
    if (i >= 0) {
        pUi->comboBoxPlace->setCurrentIndex(i);
    }
    else {
        pUi->comboBoxPlace->setCurrentIndex(0);
        placeCurrentIndex(pl.first());
    }
}

void cWorkstation::placeCurrentIndex(const QString& s)
{
    if (pPlace->getName() != s) {
        pPlace->setByName(*pq, s);
        pWorkstation->setId(_sPlaceId, pPlace->getId());
        if (pUi->checkBoxPlaceEqu->isChecked()) {
            linkToglePlaceEqu(true);
        }
    }
}
// Workstation ports slots
void cWorkstation::portNameChanged(const QString& s)
{

}

void cWorkstation::portTypeCurrentIndex(const QString& s)
{

}

void cWorkstation::macAddressChanged(const QString& s)
{

}

// Workstation address slots
void cWorkstation::ipAddressChanged(const QString& s)
{

}

void cWorkstation::ipAddressTypeCurrentIndex(const QString& s)
{

}

void cWorkstation::subNetCurrentIndex(const QString& s)
{

}

void cWorkstation::subNetAddrCurrentIndex(const QString& s)
{

}

void cWorkstation::vLanCurrentIndex(const QString& s)
{

}

void cWorkstation::vLanIdCurrentIndex(const QString& s)
{

}

// LINK 1 slots
void cWorkstation::linkChangeLinkType(int id, bool f)
{

}

void cWorkstation::linkToglePlaceEqu(bool f)
{
    if (f) {
        QString s = pPlace->getName();
        if (s != pUi->comboBoxLinkPlace->currentText()) {
            pUi->comboBoxLinkZone->setCurrentIndex(pUi->comboBoxZone->currentIndex());
            pUi->comboBoxLinkPlace->clear();
            pUi->comboBoxLinkPlace->addItems(mapZones[s]);
            pUi->comboBoxLinkPlace->setCurrentIndex(pUi->comboBoxPlace->currentIndex());
            linkPlaceCurrentIndex(s);
        }
    }
    pUi->comboBoxLinkZone->setDisabled(f);
    pUi->comboBoxLinkPlace->setDisabled(f);
}

void cWorkstation::linkZoneCurrentIndex(const QString &s)
{

}

void cWorkstation::linkPlaceCurrentIndex(const QString& s)
{

}

void cWorkstation::linkNodeCurrentIndex(int i)
{

}

void cWorkstation::linkPortCurrentIndex(int i)
{

}

void cWorkstation::linkPortShareCurrentIndex(const QString &s)
{

}

// Workstation port 2 slots
void cWorkstation::portNameChanged2(const QString& s)
{

}

void cWorkstation::portTypeCurrentIndex2(const QString& s)
{

}

void cWorkstation::macAddressChanged2(const QString& s)
{

}

// LINK 2 slots
void cWorkstation::linkChangeLinkType2(int id, bool f)
{

}

void cWorkstation::linkToglePlaceEqu2(bool f)
{
    if (f) {
        QString s = pPlace->getName();
        if (s != pUi->comboBoxLinkPlace_2->currentText()) {
            pUi->comboBoxLinkZone_2->setCurrentIndex(pUi->comboBoxZone->currentIndex());
            pUi->comboBoxLinkPlace_2->clear();
            pUi->comboBoxLinkPlace_2->addItems(mapZones[s]);
            pUi->comboBoxLinkPlace_2->setCurrentIndex(pUi->comboBoxPlace->currentIndex());
            linkPlaceCurrentIndex2(s);
        }
    }
    pUi->comboBoxLinkZone_2->setDisabled(f);
    pUi->comboBoxLinkPlace_2->setDisabled(f);
}

void cWorkstation::linkZoneCurrentIndex2(const QString &s)
{

}

void cWorkstation::linkPlaceCurrentIndex2(const QString& s)
{

}

void cWorkstation::linkNodeCurrentIndex2(int i)
{

}

void cWorkstation::linkPortCurrentIndex2(int i)
{

}

void cWorkstation::linkPortShareCurrentIndex2(const QString &s)
{

}
