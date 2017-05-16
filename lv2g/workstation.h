#ifndef CWORKSTATION_H
#define CWORKSTATION_H

#include <QWidget>
#include "lv2g.h"
#include "lv2models.h"
#include "lv2link.h"

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
    IS_EXTERNAL,
    IS_SOFT_COLL,   ///< Cím ütközés, dinamikus cím, törölhető
    IS_IMPERFECT = IS_INVALID,
    IS_NONEXIST = 0,
    IS_EXIST,
    IS_ENABLE,
    IS_NOT_POSSIBLE = 0,
    IS_POSSIBLE,
    IS_MUST

};

class LV2GSHARED_EXPORT cWorkstation : public cIntSubObj
{
    Q_OBJECT
public:
    cWorkstation(QMdiArea *parent = 0);
    ~cWorkstation();
private:
    /// A komboBox-okhoz legenerálja az állandó listákat.
    void _initLists();
    /// Újra építi az állandó listákat
    void _fullRefresh();
    void _refreshPlaces(QComboBox *pComboBoxZone, QComboBox *pComboBoxPlace);
    int _changeZone(QComboBox *pComboBoxPlace, const QString& sZone, const QString& _placeName = QString());
    int _checkNodeCollision(int ix, const QString& s);
    /// Beállítja a szűrési feltételeket a node-okra, a model-nem.
    /// Ha a kiválasztott node az új listában is szerepel, akkor a pComboBoxNode
    /// aktuális indexét erre állítja, ha nem akkor az első üres elemre.
    /// A pComboBoxNode widget beállításakor a slot-ok le vannak tíltva (lockSlot = true).
    /// @param placeName Az aktuális hely neve (vagy üres string, ha nincs kiválasztva egy sem)
    /// @param zoneName Az aktuális zóna neve
    /// @param pComboBoxNode A node listát megjelenító comboBox Widget pointere.
    /// @param pModel A node-ok lekérdezését végző (rekord) lista model
    /// @param _patt Egy opcionális minta string, ha megadható a névre ls egy szűrő (LIKE)
    /// @return a pComboBoxNode új aktuális indexe (0: nincs kiválasztva node)
    int _changePlace(const QString& placeNme, const QString& zoneName,
                     QComboBox *pComboBoxNode, cRecordListModel *pModel,
                     const QString &_patt = QString());
    int _changeLinkNode(const QString &nodeName, cRecordListModel *pModelPort, QComboBox * pComboBoxPort,
                        cPatch *pNode = NULL, cRecordAny *pPort = NULL,
                        ePhsLinkType  *pLinkType = NULL, ePortShare *pLinkShared = NULL);
    int _changeLinkPort(qlonglong portId,
                        QButtonGroup * pLinkType, QComboBox *pComboBoxShare, eTristate _isPatch,
                        cRecordAny *pPort = NULL, ePhsLinkType *pType = NULL, ePortShare *pShare = NULL);
    Ui::wstWidget   *pUi;
    QSqlQuery *pq;
    /// Kiválasztott workstation objektum (modosítandó eredetije vagy minta)
    cNode      *pSample;
    /// A szerkesztett workstation objektum
    cNode       node;   ///< A munkaállomás objektum
    /// A modosítandó vagy minta node objektum lista model.
    cRecordListModel*pModelNode;
    cNPort     *pPort1(eEx __ex = EX_ERROR);         ///< A munkaállomás elsődleges interfésze
    cIpAddress *pIpAddress(eEx __ex = EX_ERROR);     ///< Az elsődleges interfész IP címe
    cPlace     *pPlace;         ///< A munkaállomás helye
    cNPort     *pPort2(eEx __ex = EX_ERROR);         ///< Opcionális másodlagos portja(vagy interfésze) a munkaállomásnak
    QStringList portTypeList[2];
    QString     portType2No;
    QString     nodeListSql[2];
    struct cStates {
        unsigned    passive:1;          // bool (false: workstation, true: passive node)
        unsigned    modify:1;           // bool (false: new object, true: modify existing object)
        unsigned    nodeName:2;         // EMPTY, OK, COLLISION
        unsigned    nodePlace:1;        // EMPTY, OK
        unsigned    serialNumber:2;     // EMPTY, OK, COLLISION
        unsigned    inventoryNumber:2;  // EMPTY, OK, COLLISION
        unsigned    portName:1;         // EMPTY, OK
        unsigned    macNeed:1;          // bool
        unsigned    mac:2;              // EMPTY, OK, COLLISION, INVALID
        unsigned    ipNeed:2;           //
        unsigned    ipAddr:3;           // EMPTY, OK, COLLISION, INVALID, LOOPBACK
        unsigned    subNetNeed:1;       // bool
        unsigned    subNet:1;           // EMPTY, OK
        unsigned    port2Need:1;        // bool
        unsigned    port2Name:2;        // EMPTY, OK, COLLISION
        unsigned    mac2need:1;         // bool
        unsigned    mac2:2;             // EMPTY, OK, COLLISION, INVALID
        unsigned    linkPossible:1;     // bool
        unsigned    link:2;             // EMPTY, OK, COLLISION, IMPERFECT
        unsigned    link2Possible:1;
        unsigned    link2:2;
        unsigned    alarmPossibe:1;
        unsigned    alarmOld:2;         // NONEXIST, EXIST, ENABLE
        unsigned    alarmNew:2;
        unsigned    alarm2Possibe:1;
        unsigned    alarm2Old:2;
        unsigned    alarm2New:2;
    }   states;
    QString     addrCollisionInfo;
    QString     linkInfoMsg;
    QString     linkInfoMsg2;

    QButtonGroup               *pPassiveButtons;
    QButtonGroup               *pModifyButtons;
    cZoneListModel             *pFilterZoneModel;
    cPlacesInZoneModel         *pFilterPlaceModel;
    cZoneListModel             *pZoneModel;
    cPlacesInZoneModel         *pPlaceModel;
    tRecordList<cSubNet>        subnets;
    QStringList                 subnetnamelist;
    QStringList                 subnetaddrlist;
    tRecordList<cVLan>          vlans;
    QStringList                 vlannamelist;
    QStringList                 vlanidlist;

    // A (fizikai) link:
    cPatch             *pLinkNode;
    cRecordAny         *pLinkPort;
    ePhsLinkType        linkType;
    ePortShare          linkShared;
    QButtonGroup *      pLinkButtonsLinkType;
    cZoneListModel     *pLinkZoneModel;
    cPlacesInZoneModel *pLinkPlaceModel;
    cRecordListModel   *pModelLinkNode;
    cRecordListModel   *pModelLinkPort;
    // A (fizikai) link2:
    cPatch             *pLinkNode2;
    cRecordAny         *pLinkPort2;
    ePhsLinkType        linkType2;
    ePortShare          linkShared2;
    QButtonGroup *      pLinkButtonsLinkType2;
    cZoneListModel     *pLinkZoneModel2;
    cPlacesInZoneModel *pLinkPlaceModel2;
    cRecordListModel   *pModelLinkNode2;
    cRecordListModel   *pModelLinkPort2;

    int                 withinSlot;
    bool                lockSlot;
    //
    void _linkToglePlaceEqu(bool primary, bool f) { _linkToglePlaceEqu(primary, f ? TS_TRUE : TS_FALSE); }
    void _linkToglePlaceEqu(bool primary, eTristate __f = TS_NULL);
    void _setLinkType(bool primary);
    void _setMessage();
    void _completionObject();
    void _clearNode();
    void _parseObject();
    void _addressChanged(const QString& sType, const QString& sAddr = QString());
    void _subNetVLan(int sni, int vli);
    bool _changePortType(bool primary, int cix);
    int _macStat(const QString& s);
    // void _changeLink(bool primary);
protected slots:
    void toglePassive(int id, bool f);
    void togleModify(int id, bool f);
    void save();
    void refresh();

    void filterZoneCurrentIndex(const QString& s);
    void filterPlaceCurrentIndex(int i);
    void filterPatternChanged(const QString& s);

    void nodeCurrentIndex(int i);
    void nodeNameChanged(const QString& s);
    void serialChanged(const QString& s);
    void inventoryChanged(const QString& s);
    void zoneCurrentIndex(const QString& s);
    void placeCurrentIndex(int i);
    void nodeAddPlace();
    void portNameChanged(const QString& s);
    void portTypeCurrentIndex(int i);
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
    void linkPlaceCurrentIndex(int i);
    void linkNodeCurrentIndex(int i);
    void linkPortCurrentIndex(int i);
    void linkPortShareCurrentIndex(const QString &s);
    void newPatch();

    void portNameChanged2(const QString &s);
    void portTypeCurrentIndex2(int i);
    void macAddressChanged2(const QString& s);

    void linkChangeLinkType2(int id, bool f);
    void linkToglePlaceEqu2(bool f);
    void linkZoneCurrentIndex2(const QString& s);
    void linkPlaceCurrentIndex2(int i);
    void linkNodeCurrentIndex2(int i);
    void linkPortCurrentIndex2(int i);
    void linkPortShareCurrentIndex2(const QString &s);
    void newPatch2();

};

#endif // CWORKSTATION_H
