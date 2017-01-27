#include "workstation.h"
#include "ui_wstform.h"
#include "ui_phslinkform.h"
#include "lv2widgets.h"
#include "lv2link.h"
#include "record_dialog.h"

cWorkstation::cWorkstation(QWidget *parent) :
    cOwnTab(parent),
    pUi(new Ui::wstWidget),
    pLinkUi(new Ui::phsLinkForm)
{
    pWorkstation = NULL;
    pLinkNode    = NULL;
    pLinkPrt     = NULL;

    pq = newQuery();
    pWorkstation = new cNode;
    pInterface   = new cInterface;
    pIpAddress   = new cIpAddress;
    pWorkstation->setId(_sNodeType, NT_WORKSTATION);
    pIpAddress->  setId(_sIpAddressType, AT_DYNAMIC);
    pInterface->addresses << pIpAddress;
    pWorkstation->ports   << pInterface;
    dataOkBits = enum2set(DO_IP, DO_PORT_NAME);

    {
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
    }
    {
        cPlace pl;
        QStringList& zoneAll = mapZones[_sAll];
        // Az összes hely, helyiség, betörendben
        if (!pl.fetch(*pq, false, QBitArray(1, false), pl.iTab(_sPlaceName))) {
            EXCEPTION(EDATA, 0, "No any place.");
        }
        do { zoneAll << pl.getName(); } while (pl.next(*pq));
    }
    pUi->setupUi(this);             // GUI: A munkaállomás adatai (+ line)
    pUi->comboBoxZone->addItems(mapZones.keys());
    pUi->comboBoxZone->setCurrentText(_sAll);
    pUi->comboBoxPlace->addItems(mapZones[_sAll]);
    pUi->comboBoxPlace->setCurrentText(_sUnknown);
    pUi->comboBoxIpType->addItem(_sDynamic);
    pUi->comboBoxIpType->addItem(_sFixIp);
    pUi->comboBoxIpType->setCurrentIndex(0);

    pLinkWidget = new QWidget;      // GUI: Link
    pUi->verticalLayout->addWidget(pLinkWidget);
    pLinkUi->setupUi(pLinkWidget);
    pUi->verticalLayout->addWidget(line());

    pUi->verticalLayout->addStretch(0);

    tIntVector buttons;
    buttons << DBT_SPACER << DBT_SAVE << DBT_SPACER << DBT_CANCEL;
    pButtons = new cDialogButtons(buttons);
    pUi->verticalLayout->addWidget(pButtons->pWidget());


    pLinkButtonsLinkType = new QButtonGroup(this);
    pLinkButtonsLinkType->addButton(pLinkUi->radioButtonLinkTerm,  LT_TERM);
    pLinkButtonsLinkType->addButton(pLinkUi->radioButtonLinkFront, LT_FRONT);
    pLinkButtonsLinkType->addButton(pLinkUi->radioButtonLinkBack,  LT_BACK);

    pLinkNode  = new cPatch;

    pLinkUi->comboBoxZone->addItems(mapZones.keys());
    pLinkUi->comboBoxZone->setCurrentText(_sAll);
    pLinkUi->comboBoxPlace->addItems(mapZones[_sAll]);
    pLinkUi->comboBoxPlace->setCurrentText(_sUnknown);
    pModelNode = new cRecordListModel(pLinkNode->descr());
    pLinkUi->comboBoxNode->setModel(pModelNode);
    // Az nports tábla helyett a patchable_ports view-t használjuk, hogy csak a patch-elhető portokat listázza.
    const cRecStaticDescr *ppDescr = cRecStaticDescr::get("patchable_ports");
    pModelPort = new cRecordListModel(*ppDescr);
    pLinkUi->comboBoxPort->setModel(pModelPort);
    pLinkUi->comboBoxPortShare->addItem(_sNul);
    pLinkUi->comboBoxPortShare->addItem(_sA);
    pLinkUi->comboBoxPortShare->addItem(_sB);
    pLinkUi->comboBoxPortShare->setCurrentIndex(0);
    // Workstation:
    // Link:
    connect(pLinkButtonsLinkType,       SIGNAL(buttonToggled(int,bool)),    this, SLOT(linkChangeLinkType(int, bool)));
    connect(pLinkUi->comboBoxZone,      SIGNAL(currentIndexChanged(int)),   this, SLOT(linkZoneCurrentIndex(int)));
    connect(pLinkUi->comboBoxPlace,     SIGNAL(currentIndexChanged(int)),   this, SLOT(linkPlaceCurrentIndex(int)));
    connect(pLinkUi->comboBoxNode,      SIGNAL(currentIndexChanged(int)),   this, SLOT(linkNodeCurrentIndex(int)));
    connect(pLinkUi->comboBoxPort,      SIGNAL(currentIndexChanged(int)),   this, SLOT(linkPortCurrentIndex(int)));
    connect(pLinkUi->comboBoxPortShare, SIGNAL(currentTextChanged(QString)),this, SLOT(linkPortShareCurrentText(QString)));
    connect(pLinkUi->checkBoxPlaceEqu,  SIGNAL(toggled(bool)),              this, SLOT(linkToglePlaceEqu(bool)));




}

cWorkstation::~cWorkstation()
{
    delete pq;
    delete pWorkstation;
}

// LINK slots
void cWorkstation::linkChangeLinkType(int id, bool f)
{

}

void cWorkstation::linkToglePlaceEqu(bool f)
{

}

void cWorkstation::linkZoneCurrentIndex(int i)
{

}

void cWorkstation::linkPlaceCurrentIndex(int i)
{

}

void cWorkstation::linkNodeCurrentIndex(int i)
{

}

void cWorkstation::linkPortCurrentIndex(int i)
{

}

void cWorkstation::linkPortShareCurrentText(const QString &s)
{

}
