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
    cNode      *pWorkstation;
    cInterface *pInterface;
    cIpAddress *pIpAddress;
    cPlace     *pPlace;
    enum eDataOkBits {
        DO_NAME = 0,
        DO_PORT_NAME,
        DO_MAC,
        DO_IP,
        DO_END
    };
    int     dataOkBits;
    QMap<QString, QStringList>  mapZones;

    // A (fizikai) link:
    cPatch             *pLinkNode;
    cNPort             *pLinkPrt;
    qlonglong           linkType;
    QString             linkShared;
    QButtonGroup *      pLinkButtonsLinkType;
    cRecordListModel   *pModelLinkNode;
    cRecordListModel   *pModelLinkPort;
    cRecordListModel   *pModelPortShare;
    //
    cDialogButtons     *pButtons;
protected slots:
    void zoneCurrentIndex(const QString& s);
    void placeCurrentIndex(const QString& s);

    void linkChangeLinkType(int id, bool f);
    void linkToglePlaceEqu(bool f);
    void linkZoneCurrentIndex(const QString& s);
    void linkPlaceCurrentIndex(const QString &s);
    void linkNodeCurrentIndex(int i);
    void linkPortCurrentIndex(int i);
    void linkPortShareCurrentIndex(const QString &s);

};

#endif // CWORKSTATION_H
