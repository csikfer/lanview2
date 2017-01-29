#include "workstation.h"
#include "ui_wstform.h"
#include "ui_phslinkform.h"
#include "lv2widgets.h"
#include "lv2link.h"
#include "record_dialog.h"

cWorkstation::cWorkstation(QWidget *parent) :
    cOwnTab(parent),
    pUi(new Ui::wstWidget)
{
    pq = newQuery();
    pWorkstation = new cNode;
    pInterface   = new cInterface;
    pIpAddress   = new cIpAddress;
    pWorkstation->setId(_sNodeType, NT_WORKSTATION);
    pIpAddress->  setId(_sIpAddressType, AT_DYNAMIC);
    pInterface->addresses << pIpAddress;
    pWorkstation->ports   << pInterface;
    pPlace = new cPlace;
    pPlace->setById(0);     // 'unknown'
    dataOkBits = enum2set(DO_IP, DO_PORT_NAME); // Ezekre van alapértelmezett érték

    {   // Nem üres zónák listája
        static const QString sql =
                "SELECT place_group_name FROM place_groups"
                " WHERE place_group_type = 'zone'"
                 " AND 0 < (SELECT COUNT(*) FROM place_group_places WHERE place_group_places.place_group_id = place_groups.place_group_id)"
                " ORDER BY place_group_name ASC";
        // Az összes zóna neve (place_groups rekord, zone_type = 'zone', betörendben
        if (!execSql(*pq, sql)) {
            EXCEPTION(EDATA, 0, "No any zone.");
        }
        do { mapZones.insert(pq->value(0).toString(), QStringList()); } while (pq->next());
        // Az "all" zónába tartozó, vagyis az összes hely listája
        cPlace pl;
        QStringList& zoneAll = mapZones[_sAll];
        // Az összes hely, helyiség, betűrendben
        if (!pl.fetch(*pq, false, QBitArray(1, false), pl.iTab(_sPlaceName))) {
            EXCEPTION(EDATA, 0, "No any place.");
        }
        do {
            zoneAll << pl.getName();
        } while (pl.next(*pq));
    }
    pUi->setupUi(this);             // GUI: A munkaállomás adatai (+ line)
    pUi->comboBoxZone->addItems(mapZones.keys());
    pUi->comboBoxZone->setCurrentText(_sAll);
    pUi->comboBoxPlace->addItems(mapZones[_sAll]);
    pUi->comboBoxPlace->setCurrentIndex(mapZones[_sAll].indexOf(_sUnknown));

    pLinkButtonsLinkType = new QButtonGroup(this);
    pLinkButtonsLinkType->addButton(pUi->radioButtonLinkTerm,  LT_TERM);
    pLinkButtonsLinkType->addButton(pUi->radioButtonLinkFront, LT_FRONT);
    pLinkButtonsLinkType->addButton(pUi->radioButtonLinkBack,  LT_BACK);

    pLinkNode  = new cPatch;

    pUi->comboBoxLinkZone->addItems(mapZones.keys());
    pUi->comboBoxLinkZone->setCurrentText(_sAll);
    pUi->comboBoxLinkPlace->addItems(mapZones[_sAll]);
    pUi->comboBoxLinkPlace->setCurrentIndex(mapZones[_sAll].indexOf(_sUnknown));
    pModelLinkNode = new cRecordListModel(pLinkNode->descr());
    pUi->comboBoxLinkNode->setModel(pModelLinkNode);
    // Az nports tábla helyett a patchable_ports view-t használjuk, hogy csak a patch-elhető portokat listázza.
    const cRecStaticDescr *ppDescr = cRecStaticDescr::get("patchable_ports");
    pModelLinkPort = new cRecordListModel(*ppDescr);
    pUi->comboBoxLinkPort->setModel(pModelLinkPort);
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
    QStringList& zones = mapZones[sZone];
    if (zones.isEmpty()) {
        const QString sql =
                "SELECT places.* FROM place_group_plaes"
                " JOIN places USING(place_id)"
                " WHERE place_group_id = (SELECT place_group_id FROM place_groups WHERE place_group_name = ?)"
                " ORDER BY place_name ASC";
        if (!execSql(*pq, sql, sZone)) EXCEPTION(EDATA, 0, trUtf8("A %1 zónához nincs egyetlen hely sem rendelve.").arg(sZone));
        cPlace pl;
        do {
            pl.set(*pq);
            zones << pl.getName();
        } while(pq->next());
    }
    return zones;
}

// Wst. slots
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

// LINK slots
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
