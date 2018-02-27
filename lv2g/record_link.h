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
    cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = NULL, QWidget * par = NULL);
    /// destruktor
    ~cRecordLink();
    virtual void init();
    virtual QStringList where(QVariantList& qParams);
    virtual void buttonPressed(int id);
    virtual void insert(bool _similar = false);
    virtual void modify(enum eEx __ex = EX_ERROR);
    void lldp2phs();
    enum eLinkType {
        LT_PHISICAL, LT_LOGICAL, LT_LLDP
    }   linkType;
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
    /// @param parent Az szülő objektum pointere
    cLinkDialog(bool isInsert, cRecordLink * __parent = NULL);
    ~cLinkDialog();
    bool get(cPhsLink& link);
protected:
    void init();

    QLabel*         pLabelCollisions;
    QCheckBox *     pCheckBoxCollisions;
    QTextEdit *     pTextEditCollisions;
    QTextEdit *     pTextEditNote;
    QPushButton *   pPushButtonNote;

    cRecordLink *   parent;
    QSqlQuery *     pq;
    cRecord *       pActRecord;
    qlonglong       parentOwnerId;
    phsLinkWidget * pLink1;
    phsLinkWidget * pLink2;
    cDialogButtons *pButtons;
    bool            insertOnly; /// Nincs ütközés
    bool            imperfect;  /// Hiányos
    bool            exists;     /// Létezik
    qlonglong       linkId;     /// Ha létezik, akkor az utolsó
public slots:
    void changed();
private slots:
    void collisionsTogled(bool f);
    void saveNote();
    void modifyNote();
};

#endif // RECORD_LINK
