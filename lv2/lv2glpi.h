#ifndef LV2GLPI_H
#define LV2GLPI_H
#include "lv2mariadb.h"
#include "lv2cont.h"

class LV2SHARED_EXPORT cRegExpConverterItem : public QPair<QRegExp, QString> {
public:
    cRegExpConverterItem(const QString& r, const QString& s) : QPair<QRegExp, QString>(QRegExp(r), s) {}
    QString compareAndConvert(const QString& s);
};

class LV2SHARED_EXPORT cRegExpConverter : public QList<cRegExpConverterItem> {
public:
    cRegExpConverter() : QList<cRegExpConverterItem>() {};
    cRegExpConverter(const QStringList& l);
    QString compareAndConvert(const QString& s);
};

class LV2SHARED_EXPORT cGlpiEntities : public cMyRec {
public:
    CMYRECORD(cGlpiEntities);
};

class LV2SHARED_EXPORT cGlpiLocations : public cMyRec {
public:
    CMYRECORD(cGlpiLocations);
    static QString nameToGlpi(const QString& __n);
    static QString nameFromGlpi(const QString& __n);
private:
    static cRegExpConverter convertFromGlpi;
    static cRegExpConverter convertToGlpi;
};

class LV2SHARED_EXPORT cGlpiLocationsTreeItem : public tTreeItem<cMyRec> {
public:
    cGlpiLocationsTreeItem(cMyRec * __d = nullptr, tTreeItem * __par = nullptr) : tTreeItem(__d, __par) {}
    static bool setEntity();
    static QString  entityName;
    static cGlpiLocationsTreeItem   rootItem;
};

#endif // LV2GLPI_H
