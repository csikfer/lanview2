#include "lanview.h"
#include "lv2daterr.h"

/* ------------------------------ errors ------------------------------ */
DEFAULTCRECDEF(cDbErrType, _sErrors)

/// Adatbázis hiba típus: Információ; minden rendben
const QString    cDbErrType::_sOk         = "Ok";
/// Adatbázis hiba típus: Információ; Az alkalmazás elindult
const QString    cDbErrType::_sStart      = "Start";
/// Adatbázis hiba típus: Információ; Az alkalmazás leállt
const QString    cDbErrType::_sReStart    = "ReStart";
/// Adatbázis hiba típus: Információ; egyébb információ
const QString    cDbErrType::_sInfo       = "Info";
/// Adatbázis hiba típus: Figyelmeztetés; paraméter hiba
const QString    cDbErrType::_sParam      = "WParams";
/// Adatbázis hiba típus: Figyelmeztetés; A keresett adat nem található
const QString    cDbErrType::_sNotFound   = "WNotFound";
/// Adatbázis hiba típus: Figyelmeztetés; Letíltva
const QString    cDbErrType::_sDisabled   = "QDisabled";
/// Adatbázis hiba típus: Figyelmeztetés; Adat hiba, inkonzisztens adatok
const QString    cDbErrType::_sDataWarn   = "DataWarn";
/// Adatbázis hiba típus: Figyelmeztetés; Az adat eldobásra került
const QString    cDbErrType::_sDrop       = "DataDrop";
/// Futásidejű hiba
const QString    cDbErrType::_sRunTime    = "RunTime";
/* ------------------------------ db_errs ------------------------------ */
DEFAULTCRECDEF(cDbErr, _sDbErrs)

/// Egy új bejegyzés létrehozása a db_errs táblában.
/// A rekordot a paraméterek alapján tülti ki, a "trigger_op" mező értéke "EXTERNAL" lessz.
/// @param q Az adatbázis művelethez használt objektum példány.
/// @param type Az adat hiba típus neve (errors tábla egy elemét azonosítja a név mező alapján).
/// @param msg A hiba szöveges paramétere.
/// @param code A hiba numerikus paramétere
/// @param table A hibával kapcsolatbahozható tábla neve
/// @param func A hibával kapcsolatbahozható metódus neve
/// @return Ha létrejött egy új rekord, akkor 0, ha csak egy régebbi azonos rekord lett modsítva, akkor a "reapeat" mező új értéke
/// @exception Ha a művelet sikertelen.
int cDbErr::insertNew(QSqlQuery& q, const QString& type, const QString& msg, int code = -1, const QString& table = _sNil, const QString& func = _sNil)
{
    cDbErr  r;
    r.setId(_sErrorId, cDbErrType().getIdByName(q, type));
    r.setName(_sErrMsg, msg);
    r.setId(_sErrSubcode, code);
    r.setName(_sTableName, table);
    r.setName(_sFuncName, func);
    r.setName(_sTriggerOp, "EXTERNAL");
    r.insert(q);
    return r.getId(_sReapeat);
}

int cDbErr::insertNew(const QString& type, const QString& msg, int code = -1, const QString& table = _sNil, const QString& func = _sNil)
{
    QSqlQuery *pq = newQuery();
    return insertNew(*pq, type, msg, code,  table, func);
    delete pq;
}
