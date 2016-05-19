#ifndef RECORD_LINK
#define RECORD_LINK

#include "record_table.h"
#include "record_link_model.h"

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
    virtual void modify();
    virtual void remove();


};

/*
/// @class cLinkDialog
/// @brief Link rekord szerkesztés dialógus objektum, Csak a fizikai linkeket lehet szerkeszteni.
/// Tartalmazza (felépíti) a rekord mezőinek a megjelenítését, ill. a szerkesztést megvalósító widget-eket is.
class LV2GSHARED_EXPORT cLinkDialog : public cRecordDialogBase {
public:
    /// Konstruktor
    /// @param rec Az editálandó adat objktum referenciája
    /// @param __tm A rekord/tábla megjelenítését ill szerkesztését vezérlő leíró
    /// @param _buttons A megjelenítendő nyomógombok bit maszkja
    /// @param dialog Ha a dialóus ablakot QDialog-ból kell létrehozni, akkor true, ha fals, akkor QWidget-ből.
    /// @param parent Az szülő widget opcionális parent pointere
    cLinkDialog(cPhsLink rec, const cTableShape &__tm, bool dialog = true, QWidget * parent = NULL);
    /// A rekord adattag tartalmának a megjelenítése/megjelenítés visszaállítása
    virtual void restore(cRecord *_pRec = NULL);
    /// A megjelenített értékek kiolvasása
    virtual bool accept();
    virtual cFieldEditBase * operator[](const QString& __fn);
    static const int buttons;
};
*/
#endif // RECORD_LINK
