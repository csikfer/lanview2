#ifndef DEDUCEPATCH_H
#define DEDUCEPATCH_H
#include "lv2g.h"
#include "lv2link.h"
#include "srvdata.h"
#include "lv2models.h"

class cSelectNode : public QObject {
    Q_OBJECT
public:
    cSelectNode(QComboBox *_pZone, QComboBox *_pPLace, QComboBox *_pNode, QLineEdit *_pFilt = NULL, const QString& _cFilt = QString());
private:
    QComboBox *pComboBoxZone;
    QComboBox *pComboBoxPLace;
    QComboBox *pComboBoxNode;
    QLineEdit *pLineEditFilt;
    const QString    constFilter;
    cZoneListModel     *pZoneModel;
    cPlacesInZoneModel *pPlaceModel;
    cRecordListModel   *pNodeModel;
    bool                blockSignal;
private slots:
    void zoneChanged(int ix);
    void placeChanged(int ix);
    void patternChanged(const QString& s);
    void _nodeChanged(int ix);
signals:
    void nodeChanged(const QString& name);
    void nodeChanged(qlonglong id);
};

/* *** */

#if defined(LV2G_LIBRARY)
#include "ui_deducepatch.h"
class cDeducePatch;
class LV2GSHARED_EXPORT cDPRow : public QObject {
    Q_OBJECT
public:
    /// @param par parent objektum
    /// @param _row sor index a táblában
    /// @param il Bal oldali interfész port (linkelt a bal oldali patch porttal)
    /// @param ppl Bal oldali patch port
    /// @param pll Fizikai link objektum il és ppl között.
    /// @param plr Jobb oldali patch porthoz tartozó végponti front link objektum
    cDPRow(QSqlQuery& q, cDeducePatch *par, int _row, cMacTab& mt, cNPort& il, cPPort& ppl, cPhsLink& pll, cPhsLink& plr);
    cDPRow(QSqlQuery& q, cDeducePatch *par, int _row, bool unique, cPPort& ppl, cPPort& ppr);
    cDeducePatch * parent;
    QTableWidget * pTable;
    const int      row;
    QCheckBox    * pCheckBox;
    cPhsLink       phsLink;
private slots:
    void checkBoxchange(bool f);
};


#else
namespace Ui {
    class class deducePatch;
}
class cDPRow;
#endif


class cDeducePatch : public cIntSubObj {
    friend class cDPRow;
    Q_OBJECT
public:
    cDeducePatch(QMdiArea *par);
    ~cDeducePatch();
    static const enum ePrivilegeLevel rights;
protected:
    void setButtons();
    void clearTable();
    void byLLDP(QSqlQuery &q);
    void byMAC(QSqlQuery& q);
    void byTag(QSqlQuery& q);
    bool findLink(cPhsLink& pl);
    Ui::deducePatch *pUi;
    cSelectNode *pSelNode;
    cSelectNode *pSelNode2;
    QList<cDPRow *> rows;
    enum eDeducePatchMeth { DPM_LLDP, DPM_MAC, DPM_TAG } methode;
    qlonglong   nid, nid2;
    cPatch      patch;
protected slots:
    void changeNode(qlonglong id);
    void changeNode2(qlonglong id);
    void setMethodeLLDP() { methode = DPM_LLDP; clearTable(); setButtons(); }
    void setMethodeMAC()  { methode = DPM_MAC;  clearTable(); setButtons(); }
    void setMethodeTag()  { methode = DPM_TAG;  clearTable(); setButtons(); }
    void start();
    void save();
};

#endif // DEDUCEPATCH_H
