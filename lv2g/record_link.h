#ifndef RECORD_LINK
#define RECORD_LINK

#include "record_table.h"
#include "record_link_model.h"
#include "lv2models.h"
#include "ui_add_phs_link.h"

/// @class cRecordLink
/// Egy adatbázis link nézet tábla megjelenítését végző objektum.
class LV2GSHARED_EXPORT cRecordLink : public cRecordTable {
    Q_OBJECT
public:
    /// Konstruktor már beolvasott leíró
    /// @param _mn A tábla megjelenítését leíró objektum pointere
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = NULL, QWidget * par = NULL);
    /// destruktor
    ~cRecordLink();
    virtual QStringList where(QVariantList& qParams);
    virtual void insert();
    virtual void modify(enum eEx __ex = EX_ERROR);
    virtual void remove();


};

/// @class cLinkDialog
/// @brief Link rekord szerkesztés dialógus objektum, Csak a fizikai linkeket lehet szerkeszteni.
class LV2GSHARED_EXPORT cLinkDialog : public QDialog {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param parent Az szülő objektum pointere
    cLinkDialog(cRecordLink * __parent = NULL);
    ~cLinkDialog();
private:
    void init();
    QString nodeFilter(const cPlace& place);
    QString portFilter(const cPatch& node);

    cRecordLink * parent;
    QSqlQuery *   pq;
    cRecord *     pActRecord;
    qlonglong     parentOwnerId;
    cPatch        node1, node2;
    cNPort *      pPrt1, *pPrt2;
    cPlaceGroup   pgrp1, pgrp2;
    cPlace        plac1, plac2;
    Ui_DialogAddPhsLink *pUi;
    bool                utter1;
    bool                utter2;
    QButtonGroup       *pButtonsLink1Type;
    QButtonGroup       *pButtonsLink2Type;
    cRecordListModel   *pModelZone1;
    cRecordListModel   *pModelZone2;
    cRecordListModel   *pModelPlace1;
    cRecordListModel   *pModelPlace2;
    cRecordListModel   *pModelNode1;
    cRecordListModel   *pModelNode2;
    cRecordListModel   *pModelPort1;
    cRecordListModel   *pModelPort2;
    cRecordListModel   *pModelPort1Share;
private slots:
    void zone1CurrentIndex(int i);
    void zone2CurrentIndex(int i);
    void place1CurrentIndex(int i);
    void place2CurrentIndex(int i);
    void node1CurrentIndex(int i);
    void node2CurrentIndex(int i);
    void port1CurrentIndex(int i);
    void pore2CurrentIndex(int i);
    void port1ShareCurrentIndex(int i);

};

#endif // RECORD_LINK
