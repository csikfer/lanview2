#ifndef CWORKSTATION_H
#define CWORKSTATION_H

#include <QWidget>
#include "lv2g.h"
#include "lv2models.h"

namespace Ui {
    class wstWidget;
}
class cDialogButtons;

enum {
    IS_EMPTY = 0,   ///< Üres, nincs megadva
    IS_OK,          ///< OK
    IS_COLLISION,   ///< Más értékkel ütközik
    IS_INVALID,     ///< Nem megengedett érték (értéktattományon kívül)
    IS_LOOPBACK,    ///< Egy loopback cím lett megadva.
    IS_EXTERNAL
};

class LV2GSHARED_EXPORT cWorkstation : public cOwnTab
{
    Q_OBJECT
public:
    cWorkstation(QWidget *parent = 0);
    ~cWorkstation();
private:
    /// A komboBox-okhoz legenerálja az állandó listákat.
    void initLists();
    /// A zónához tartozó helyek listáját adja vissza.
    /// A mapZones tömb tartalmazza a listákat, ha a lista üres, akkor lekérdezi.
    /// @param sZone A zóna neve
    const QStringList& placesInZone(const QString& sZone);
    int checkWrkCollision(int ix, const QString& s);
    qlonglong placeCurrentIndex(const QString& sPlace, QComboBox *pComboBoxZone,
                                QComboBox *pComboBoxNode, cRecordListModel *pModel,
                                const QString &_patt = QString());
    Ui::wstWidget   *pUi;
    QSqlQuery *pq;
    /// Kiválasztott workstation objektum (modosítandó eredetije vagy minta)
    cNode      *pSample;
    /// A szerkesztett workstation objektum
    cNode      *pWorkstation;   ///< A munkaállomás objektum
    /// A modosítandó vagy minta node objektum lista model.
    cRecordListModel*pModelNode;
    cInterface *pInterface;     ///< A munkaállomás elsődleges interfésze
    cIpAddress *pIpAddress;     ///< Az elsődleges interfész IP címe
    cPlace     *pPlace;         ///< A munkaállomás helye
    cNPort     *pPort2;         ///< Opcionális másodlagos portja(vagy interfésze) a munkaállomásnak
    struct cStates {
        unsigned    nodeName:2;         // EMPTY, OK, COLLISION
        unsigned    nodePlace:1;        // EMPTY, OK
        unsigned    serialNumber:2;     // EMPTY, OK, COLLISION
        unsigned    inventoraNumber:2;  // EMPTY, OK, COLLISION
        unsigned    portName:1;         // EMPTY, OK
        unsigned    mac:2;              // EMPTY, OK, COLLISION, INVALID
        unsigned    ipNeed:1;           // bool
        unsigned    ipAddr:3;           // EMPTY, OK, COLLISION, INVALID, LOOPBACK
        unsigned    subNetNeed:1;       // bool
        unsigned    subNet:1;           // EMPTY, OK
        unsigned    port2Need:1;        // bool
        unsigned    port2Name:2;        // EMPTY, OK, COLLISION
        unsigned    mac2need:1;         // bool
        unsigned    mac2:2;             // EMPTY, OK, COLLISION, INVALID
        unsigned    modify:1;           // bool (false: new object, true: modify existing object)
        unsigned    linkPossible:1;
        unsigned    link:2;
        unsigned    link2Possible:1;
        unsigned    link2:2;
    }   states;

    QMap<QString, QStringList>  mapZones;   ///< A zonákhoz tartozó hely nevek listái.
    QStringList                 zones;      ///< A zónák listája sorrendben
    tRecordList<cSubNet>        subnets;
    QStringList                 subnetnamelist;
    QStringList                 subnetaddrlist;
    tRecordList<cVLan>          vlans;
    QStringList                 vlannamelist;
    QStringList                 vlanidlist;

    // A (fizikai) link:
    cPatch             *pLinkNode;
    cNPort             *pLinkPrt;
    qlonglong           linkType;
    QString             linkShared;
    QButtonGroup *      pLinkButtonsLinkType;
    cRecordListModel   *pModelLinkNode;
    cRecordListModel   *pModelLinkPort;
    // A (fizikai) link2:
    cPatch             *pLinkNode2;
    cNPort             *pLinkPrt2;
    qlonglong           linkType2;
    QString             linkShared2;
    QButtonGroup *      pLinkButtonsLinkType2;
    cRecordListModel   *pModelLinkNode2;
    cRecordListModel   *pModelLinkPort2;

    int                 withinSlot;
    bool                lockSlot;
    //
    void linkToglePlaceEqu(bool f, bool primary);
    void setMessage();
    void parseObject();

protected slots:
    void togleNewMod(bool f);
    void save();
    void refresh();

    void filterZoneCurrentIndex(const QString& s);
    void filterPlaceCurrentIndex(const QString& s);
    void filterPatternChanged(const QString& s);

    void nodeCurrentIndex(int i);
    void nodeNameChanged(const QString& s);
    void serialChanged(const QString& s);
    void inventoryChanged(const QString& s);
    void zoneCurrentIndex(const QString& s);
    void placeCurrentIndex(const QString& s);
    void portNameChanged(const QString& s);
    void portTypeCurrentIndex(const QString& s);
    void macAddressChanged(const QString& s);
    void ipAddressChanged(const QString& s);
    void ipAddressTypeCurrentIndex(const QString& s);
    void subNetCurrentIndex(int i);
    void subNetAddrCurrentIndex(int i);
    void vLanCurrentIndex(int i);
    void vLanIdCurrentIndex(int i);

    void linkChangeLinkType(int id, bool f);
    void linkToglePlaceEqu(bool f);
    void linkZoneCurrentIndex(const QString& s);
    void linkPlaceCurrentIndex(const QString &s);
    void linkNodeCurrentIndex(int i);
    void linkPortCurrentIndex(int i);
    void linkPortShareCurrentIndex(const QString &s);

    void portNameChanged2(const QString& s);
    void portTypeCurrentIndex2(int i);
    void macAddressChanged2(const QString& s);

    void linkChangeLinkType2(int id, bool f);
    void linkToglePlaceEqu2(bool f);
    void linkZoneCurrentIndex2(const QString& s);
    void linkPlaceCurrentIndex2(const QString &s);
    void linkNodeCurrentIndex2(int i);
    void linkPortCurrentIndex2(int i);
    void linkPortShareCurrentIndex2(const QString &s);

};

#endif // CWORKSTATION_H
