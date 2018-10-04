#ifndef RECORD_LINK
#define RECORD_LINK

#include "record_table.h"
#include "record_link_model.h"
#include "lv2models.h"

/// @class cRecordLink
/// Egy adatbázis link nézet tábla megjelenítését végző objektum.
class LV2GSHARED_EXPORT cRecordLink : public cRecordTable {
    Q_OBJECT
public:
    /// Konstruktor már beolvasott leíró
    /// @param _mn A tábla megjelenítését leíró objektum pointere
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = nullptr, QWidget * par = nullptr);
    /// destruktor
    ~cRecordLink();
    virtual void init();
    virtual QStringList where(QVariantList& qParams);
    virtual void buttonPressed(int id);
    /// Edit dialog
    /// @param _similar Ha igaz, akkor az aktuális rekord a minta.
    void edit(bool _similar = false, eEx __ex = EX_ERROR);
    // void lldp2phs();
    enum eLinkType {
        LT_PHISICAL, LT_LOGICAL, LT_LLDP
    }   linkType;
private slots:
    void modifyByIndex(const QModelIndex & index);
};

class phsLinkWidget;
class cPhsLink;
/// @class cLinkDialog
/// @brief Link rekord szerkesztés dialógus objektum, Csak a fizikai linkeket lehet szerkeszteni.
class LV2GSHARED_EXPORT cLinkDialog : public QDialog {
    friend class phsLinkWidget;
    Q_OBJECT
public:
    /// Konstruktor
    /// @param __parent Az szülő objektum pointere
    cLinkDialog(bool __similar, cRecordLink * __parent = nullptr);
    ~cLinkDialog();
    void get(cPhsLink& link);
    bool next();
    bool prev();
protected:
    void init();

    QLabel*         pLabelCollisions;
    QCheckBox *     pCheckBoxCollisions;
    QToolButton *   pToolButtonRefresh;
    QTextEdit *     pTextEditCollisions;
    QTextEdit *     pTextEditNote;
    QToolButton *   pToolButtonNoteNull;

    cRecordLink *   parent;
    QSqlQuery *     pq;
    cRecord *       pActRecord;
    qlonglong       parentNodeId;
    qlonglong       parentPortId;
    phsLinkWidget * pLink1;
    phsLinkWidget * pLink2;
    cDialogButtons *pButtons;
    bool            collision; /// Nincs ütközés
    bool            imperfect;  /// Hiányos
    bool            exists;     /// Létezik
    bool            noteChanged;/// Megváltoztattuk a megjegyzés mezőt
    qlonglong       linkId;     /// Ha létezik, akkor az utolsó
public slots:
    /// Változás esetén az állapot beállítása, és a riport megjelenítése.
    void changed();
private slots:
    void collisionsTogled(bool f);
    void noteNull(bool st);
    void modifyNote();
    void buttonPressed(int kid);
};

#endif // RECORD_LINK
