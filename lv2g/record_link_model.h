#ifndef RECORD_LINK_MODEL
#define RECORD_LINK_MODEL

#include "record_table_model.h"

class cRecordLink;

/// @class cRecordLinkModel
/// Model osztály adatatbázis tábla táblázatos megjelenítéséhez
class cRecordLinkModel : public cRecordTableModel {
    Q_OBJECT
public:
    cRecordLinkModel(cRecordLink &_rt);
    ~cRecordLinkModel();
    /// Törli az index listában megjelült rekordokat.
    /// Elötte egy dialógusban megerősítést kér, ha ebből Cancel-el lépünk ki, akkor nincs törlés.
    /// A sorokat/rekordokat a removeRec() metódus hívásával törli.
    virtual void removeRecords(const QModelIndexList& mil);
    /// Eltávolítja az adatbázisból a megadott sorban megjelenített rekordot
    /// Majd ha ez sikerült, akkor törli a megjelenített sort is a widgeten is a removeRow() hívásával
    virtual bool removeRec(const QModelIndex & mi);
};

#endif // RECORD_LINK_MODEL

