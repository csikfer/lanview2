#ifndef CWORKSTATION_H
#define CWORKSTATION_H

#include <QWidget>
#include "lv2g.h"
#include "lv2models.h"

namespace Ui {
    class wstWidget;
}
class cDialogButtons;

class LV2GSHARED_EXPORT cWorkstation : public cOwnTab
{
    Q_OBJECT
public:
    cWorkstation(QWidget *parent = 0);
    ~cWorkstation();
private:
    const QStringList& placesInZone(const QString& sZone);
    Ui::wstWidget   *pUi;
    QSqlQuery *pq;
    /// A workstation objektum
    cNode      *pWorkstation;   ///< A munkaállomás objektum
    cRecordListModel*pModelNode;
    cInterface *pInterface;     ///< A munkaállomás elsődleges interfésze
    cIpAddress *pIpAddress;     ///< Az elsődleges interfész IP címe
    cPlace     *pPlace;         ///< A munkaállomás helye
    cNPort     *pPort2;         ///< Opcionális másodlagos portja(vagy interfésze) a munkaállomásnak
    enum eDataOkBits {
        DO_NAME = 0,
        DO_PORT_NAME,
        DO_MAC,
        DO_IP,
        DO_END
    };
    int     dataOkBits;
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
    //

protected slots:
    void nodeCurrentIndex(int);
    void nodeNameChanged(const QString& s);
    void zoneCurrentIndex(const QString& s);
    void placeCurrentIndex(const QString& s);
    void portNameChanged(const QString& s);
    void portTypeCurrentIndex(const QString& s);
    void macAddressChanged(const QString& s);
    void ipAddressChanged(const QString& s);
    void ipAddressTypeCurrentIndex(const QString& s);
    void subNetCurrentIndex(const QString& s);
    void subNetAddrCurrentIndex(const QString& s);
    void vLanCurrentIndex(const QString& s);
    void vLanIdCurrentIndex(const QString& s);

    void linkChangeLinkType(int id, bool f);
    void linkToglePlaceEqu(bool f);
    void linkZoneCurrentIndex(const QString& s);
    void linkPlaceCurrentIndex(const QString &s);
    void linkNodeCurrentIndex(int i);
    void linkPortCurrentIndex(int i);
    void linkPortShareCurrentIndex(const QString &s);

    void portNameChanged2(const QString& s);
    void portTypeCurrentIndex2(const QString& s);
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
