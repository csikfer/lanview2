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
    cRegExpConverterItem(QSqlQuery& q);
    /// A megadott szövek összehasonlítása a mintával, és egyezés esetén a konvertált szöveg visszaadása.
    /// @param s A konvertálandó szöveg
    /// @param smap Opcionális behelyettesítendő paraméterek konténere (azonosító, szöveg)
    /// @return A konvertált szöveg, vagy üres, ha nincs minta egyezés.
    QString compareAndConvert(const QString& s, const QMap<QChar, QString>& smap = QMap<QChar, QString>());
    QMap<int, QChar>    substituteMap;      ///< A mintában behelyettesítendők index és azonosító
    QList<int>          reverseIndexList;   ///< A behelyettesítések indexei csökkenő sorrendben
    QString     pattern;                    ///< A keresett minta (behelyettesítés elött)
    /// Ha nincs behelyettesítés, akkor a keresett mintát írja ide a konstruktor.
    /// Ha van (substituteMap nem üres), akkor a compareAndConvert() metódus minden alkalommal
    /// létrehozza a mintát a behelyettesítések végrehajtása után, a regexp a munka objektum.
    QRegularExpression     patternRe;
    /// A behelyettesítéseknél használlt minta.
    QRegularExpression     substRe;
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
    QString compareAndConvert(const QString& s, const QMap<QChar, QString> &smap = QMap<QChar, QString>());
};


class LV2SHARED_EXPORT cGlpiEntity : public cMyRec {
    CMYRECORD(cGlpiEntity);
public:
};

class LV2SHARED_EXPORT cGlpiLocation : public cMyRec {
    CMYRECORD(cGlpiLocation);
public:
    static QString nameToGlpi(const QString& __n, const QMap<QChar, QString> &smap);
    static QString nameFromGlpi(const QString& __n, const QMap<QChar, QString> &smap);
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
    bool fetchGlpiTree();
    bool mergeLv2Tree();
    bool addLv2Tree();
    bool isRootItem();
    /// A beolvasott egyesített fa alapján feltételesen végrehalytja a szinkronizálást.
    /// Rekurzív.
    /// @param _inserGlpi Ha értéke nem TS_NULL, akkor hívja az insertGlpiRecord() metódust az aktuális
    ///  elemmel, a metódus paramétere : (bool)_inserGlpi
    /// @param _inserLv2 Ha értéke nem TS_NULL, akkor hívja az insertLv2Record() metódust az aktuális
    ///  elemmel, a metódus paramétere : (bool)_inserLv2
    /// @param _updateGlpi Ha értéke true, akkor ha mindkét rekord megvan, és a glpi_location alapján megtatlált
    ///  places rekordból viiszakonvertált completename érték nem egyezik, akkor modosítja a glpi_locations rekordot.
    void prepare(eTristate _insertGlpi, eTristate _insertLv2, bool _updateGlpi);
    void insertLv2Record(bool _do);
    void insertGlpiRecord(bool _do);
    eTristate updateGlpiRecord(bool _do);

    /// Betölti a GLPI glpi_locations rekordokból a hely fát, ahol lehet betölti a places rekordokat.
    /// Majd kiegésziti a fát a még fel nem dolgozott places rekordokkal.
    /// Azokat a glpi_locations reokordokat, melyek neve nem konvertálható a places rekord névvé, el lesznek dobva.
    /// A places rekordok közül amelyek neve nem konvertálható, szintén eldobásra kerülnek.
    static cGlpiLocationsTreeItem * fetchLocationAndPlaceTree();

    static cGlpiLocationsTreeItem * syncing(eTristate _insertGlpi, eTristate _insertLv2, bool _updateGlpi)
    {
        cMariaDb::init();
        cGlpiLocationsTreeItem * pTree = fetchLocationAndPlaceTree();
        if (pTree == nullptr) return nullptr;
        pTree->prepare(_insertGlpi, _insertLv2, _updateGlpi);
        cMariaDb::drop();
        return pTree;
    }
};


#endif // LV2GLPI_H
