#ifndef RECORD_LINK
#define RECORD_LINK

#include "record_table.h"
#include "record_link_model.h"

/// @class cRecordLink
/// Egy adatbázis link nézet tábla megjelenítését végző objektum.
class LV2GSHARED_EXPORT cRecordLink : public cRecordTable {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _mn A tábla megjelenítését leíró rekord neve (table_shapes.table_shape_name)
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordLink(const QString& _mn, bool _isDialog, QWidget * par = NULL);
    /// Konstruktor már beolvasott leíró
    /// @param _mn A tábla megjelenítését leíró objektum pointere
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = NULL, QWidget * par = NULL);
    /// destruktor
    ~cRecordLink();
};

#endif // RECORD_LINK
