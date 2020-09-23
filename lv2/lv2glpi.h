#ifndef LV2GLPI_H
#define LV2GLPI_H
#include "lv2mariadb.h"
#include "lv2cont.h"
#include "lv2data.h"

/// @class cRegExpConverterItem
/// Segédosztály a cRegExpConverter -hez.
class LV2SHARED_EXPORT cRegExpConverterItem : public cSelect {
    friend class cRegExpConverter;
protected:
    cRegExpConverterItem(QSqlQuery q) : cSelect() {
        set(q);
        const QString patternType = getName(_sPatternType);
        Qt::CaseSensitivity      caseSensitivity;
        if      (patternType.compare(_sRegexp,  Qt::CaseInsensitive) == 0) caseSensitivity = Qt::CaseSensitive;
        else if (patternType.compare(_sRegexpi, Qt::CaseInsensitive) == 0) caseSensitivity = Qt::CaseInsensitive;
        else {
            EXCEPTION(EDATA);
        }
        pattern.setPattern(getName(_sPattern));
        pattern.setCaseSensitivity(caseSensitivity);

    }
    QString compareAndConvert(const QString& s);
    QRegExp     pattern;
};

/// @class cRegExpConverter
/// Az objektum RegExp kifelyezések alapján konvertál neveket, pl. két adatbázisban tárolt nevek összevetéséhez.
/// A kifelyezéseket a selects táblábol vaszi, így egy konverzió több kifelyezés alapján is történhet, a kifelyezéseket
/// a megadott sorrendben használva. A selects táblában a RegExp kifelyezést, amire a konvertálandó szövegnek illeszkednie kell a
/// pattern mező tartalmazza. A konverzió eredményét pedig a choice mező, melyben a $1, $2,... kifelyezések behelyettesítése a szokásos.
/// Az osztály által használt selects rekordokban csak a 'regexp' és 'regexpi' minta típusok támogatottak.
class LV2SHARED_EXPORT cRegExpConverter : public QList<cRegExpConverterItem *> {
public:
    /// Konstruktor. Betölti az adott kulcs alapján a mintákat.
    /// @param key A kulcs érték. A selects táblából azok a rekordok kerülnek betöltésre, melynek select_type mezőjének
    /// értéke azonos  key-el.
    cRegExpConverter(const QString& key);
    /// A konverziós függvény.
    /// @param s A konvertálandó név ill. szöveg.
    /// @return A kikonvertált szöveg. Ha nem talált olyan mintát, mely illeszkedne az s paraméterre, akkor üres striggel tér vissza.
    QString compareAndConvert(const QString& s);
};


class LV2SHARED_EXPORT cGlpiEntity : public cMyRec {
    CMYRECORD(cGlpiEntity);
public:
};

class LV2SHARED_EXPORT cGlpiLocation : public cMyRec {
    CMYRECORD(cGlpiLocation);
public:
    QString nameFromGlpi() { return nameFromGlpi(getName(_sCompletename)); }
    static QString nameToGlpi(const QString& __n);
    static QString nameFromGlpi(const QString& __n);
    static const QString sLevelSep;
private:
    static cRegExpConverter * pConvertFromGlpi;
    static cRegExpConverter * pConvertToGlpi;
};

enum eGlpiSyncResult {
    SR_UNSET,       ///< Előkészítés
    SR_EQU,         ///< Azonos rekordok
    SR_SYNCED_GLPI, ///< GLPI rekord létrehozva
    SR_SYNCED_LV2,  ///< LanView2 rekord létrehozva
    SR_ERROR_GLPI,  ///< GLPI rekord létrehozvása sikertelen
    SR_ERROR_LV2,   ///< LanView2 rekord létrehozvása sikertelen
    SR_SKIP_GLPI,   ///< GLPI rekord létrehozvésa kihagyva
    SR_SKIP_LV2     ///< LanView2 rekord létrehozvása kihagyva
};

class LV2SHARED_EXPORT cLocationsTreeItemData  {
public:
    cLocationsTreeItemData(const cMyRec& _glpiRecord);
    cLocationsTreeItemData(const cPlace& _lv2Record);
    cLocationsTreeItemData(cPlace * _pLv2Record);
    ~cLocationsTreeItemData();
    cLocationsTreeItemData *dup() const { EXCEPTION(ENOTSUPP); }    // A VC reklamál ha nincs dup()
    QString toString();
    cMyRec    * pGlpiRecord;    ///< cGlpiEntity (root) or cGlpiLocation
    cPlace    * pLv2Record;
    QString     placeName;
    QString     completename;
    QString     locationName;
    enum eGlpiSyncResult result;
};

class LV2SHARED_EXPORT cGlpiLocationsTreeItem : public tTreeItem<cLocationsTreeItemData> {
public:
    cGlpiLocationsTreeItem(const cMyRec& __d, tTreeItem * __par = nullptr)
        : tTreeItem(new cLocationsTreeItemData(__d), __par) { }
    QString toString(bool tree = false, int indent = 0);
    bool fetchGlpiTree(QString& emsg);
    bool mergeLv2Tree(QString& emsg);
    bool addLv2Tree(QString& emsg);
    void prepare(QString &emsg, eTristate _updateGlpi, eTristate _updateLv2);
    void updateLv2Record(bool _update, QString& emsg);
    void updateGlpiRecord(bool _update, QString& emsg);

    /// Betölti a GLPI glpi_locations rekordokból a hely fát, ahol lehet betölti a places rekordokat.
    /// Majd kiegésziti a fát a még fel nem dolgozott places rekordokkal.
    /// Azokat a glpi_locations reokordokat, melyek neve nem konvertálható a places rekord névvé, el lesznek dobva.
    /// A places rekordok közül amelyek neve nem konvertálható, szintén eldobásra kerülnek.
    static cGlpiLocationsTreeItem * fetchLocationAndPlaceTree(QString& emsg);

    static cGlpiLocationsTreeItem * syncing(QString& emsg, eTristate _updateGlpi, eTristate _updateLv2)
    {
        cMariaDb::init();
        cGlpiLocationsTreeItem * pTree = fetchLocationAndPlaceTree(emsg);
        cMariaDb::drop();
        if (pTree == nullptr) return nullptr;
        pTree->prepare(emsg, _updateGlpi, _updateLv2);
        return pTree;
    }
};


#endif // LV2GLPI_H
