#ifndef LV2GLPI_H
#define LV2GLPI_H
#include "lv2mariadb.h"
#include "lv2cont.h"
#include "lv2data.h"

class LV2SHARED_EXPORT cRegExpConverterItem : public cSelect {
public:
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

class LV2SHARED_EXPORT cRegExpConverter : public QList<cRegExpConverterItem *> {
public:
    cRegExpConverter(const QString& key);
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
