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


class LV2SHARED_EXPORT cGlpiEntities : public cMyRec {
    CMYRECORD(cGlpiEntities);
public:
};

class LV2SHARED_EXPORT cGlpiLocations : public cMyRec {
    CMYRECORD(cGlpiLocations);
public:
    static QString nameToGlpi(const QString& __n);
    static QString nameFromGlpi(const QString& __n);
    static const QString sLevelSep;
private:
    static cRegExpConverter * pConvertFromGlpi;
    static cRegExpConverter * pConvertToGlpi;
};

class LV2SHARED_EXPORT cGlpiLocationsTreeItem : public tTreeItem<cMyRec> {
public:
    cGlpiLocationsTreeItem(cMyRec * __d = nullptr, tTreeItem * __par = nullptr) : tTreeItem(__d, __par) {}
    bool setEntity();
    qlonglong entitiesId() { return pData->getId(); }
    QString  entityName;
};

#endif // LV2GLPI_H
