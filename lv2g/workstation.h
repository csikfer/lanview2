#ifndef CWORKSTATION_H
#define CWORKSTATION_H

#include <QWidget>
#include "lv2g.h"
#include "lv2models.h"

namespace Ui {
    class wstWidget;
    class phsLinkForm;
}
class cDialogButtons;

class LV2GSHARED_EXPORT cWorkstation : public cOwnTab
{
    Q_OBJECT
public:
    cWorkstation(QWidget *parent = 0);
    ~cWorkstation();
private:
    Ui::wstWidget   *pUi;
    QSqlQuery *pq;
    /// A workstation objektum
    cNode      *pWorkstation;
    cInterface *pInterface;
    cIpAddress *pIpAddress;
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
    Ui::phsLinkForm    *pLinkUi;
    QWidget            *pLinkWidget;
    cPatch             *pLinkNode;
    cNPort             *pLinkPrt;
    qlonglong           linkType;
    QString             linkShared;
    QButtonGroup *      pLinkButtonsLinkType;
    cRecordListModel   *pModelNode;
    cRecordListModel   *pModelPort;
    cRecordListModel   *pModelPortShare;
    //
    cDialogButtons     *pButtons;
protected slots:
    void linkChangeLinkType(int id, bool f);
    void linkToglePlaceEqu(bool f);
    void linkZoneCurrentIndex(int i);
    void linkPlaceCurrentIndex(int i);
    void linkNodeCurrentIndex(int i);
    void linkPortCurrentIndex(int i);
    void linkPortShareCurrentText(const QString &s);

};

#endif // CWORKSTATION_H
