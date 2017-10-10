#ifndef INDALARMIF1_GL_H
#define INDALARMIF1_GL_H

#define IAIF1PORTS  8

/// Kontakt érzékelők állapota
enum eKStat {
    KOK   = 'O',    ///< OK
    KNULL = 'N',    ///< Nincs mért érték
    KDISA = 'D',    ///< Tíltva, nincs érzékelő
    KALERT= 'A',    ///< Riasztás
    KINV  = 'I',    ///< Fordított bekötés + Riasztás
    KSHORT= 'S',    ///< Rövidzár, szabotázs ?
    KCUT  = 'C',    ///< Szakadás, szabotázs ?
    KANY  = 'U',    ///< A mért érték nem értelmezhető
    KOLD  = 'a' - 'A', ///< A riasztási állapotnak vége, de nincs nyugtázva
    KERR  = 'E'     ///< program hiba
};

enum eKConf {
    KENA  = 'E',    ///< Engedélyezve
//  KDISA = 'D',    ///< Tiltva
    KREV  = 'R'     ///< Engedélyezve. Az érzékelőpár fordítva van bekötve (párból bármelyiknél jelezve)
};

/// Eldönti a megadott kontakt érzékelő állapotról, hogy az riasztás-e vagy sem
/// @param e A kontakt érzékelő állapota
/// @return Ha a megadott állapot riasztás, akkor 1 (true), ha nem akkor 0 (false)
static inline uint8_t isAlert(enum eKStat s)
{
    return (s == 0 || s == KOK || s == KNULL || s == KDISA) ? 0 : 1;
}

/// Státusz lekérdező üzenet szerkezete (11 Byte)
struct qMsg {
    char                    beginMark[2];   // 0, 1: Marker '??'
    char                    tMaster[2];     // 2, 3: Target master address (hexa)
    char                    tSlave[2];      // 4, 5: Target slave address (hexa)
    char                    clrErrSt;       // 6:    Hiba státusz törlése (T/F)
    char                    clrKontSt[2];   // 7, 8: Érzékelők hiba állapot törlése bitvektor (hexa)
    char                    endMark[2];     // 9,10: Marker CRLF
};

/// Státusz lekérdező üzenetre küldött válasz üzenet szerkezete (24 Byte)
struct aMsg {
    char                    beginMark[2];   //  0, 1: Marker '..'
    char                    sMaster[2];     //  2, 3: Source master address (hexa)
    char                    sSlave[2];      //  4, 5: Source slave address (hexa)
    char                    kontakt[8];     //  6-13: Ld.: enum eKStat
    char                    errstat[4];     // 14-17:
    char                    counter[4];     // 18-21:
    char                    endMark[2];     // 22,23: Marker CRLF
};

/// Parancs üzenet szerkezete (18 Byte)
struct cMsg {
    char                    beginMark[2];   //  0, 1: Marker '!!'
    char                    tMaster[2];     //  2, 3: Target master address (hexa)
    char                    tSlave[2];      //  4, 5: Target slave address (hexa)
    char                    kontakt[8];     //  6-13: Ld.: enum eKConf
    char                    light[2];       // 14-17: Érzékelők fényereje: piros/zöld
    char                    swdet[2];       // 18,19: k1,k2 kapcsoló 'E' enable, 'D' disable
    char                    endMark[2];     // 20,21: Marker CRLF
};

#endif // INDALARMIF1_GL_H
