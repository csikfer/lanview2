#ifndef INDALARMIF2_GL_H
#define INDALARMIF2_GL_H

#ifndef INDALARMIF1_GL_H
#include "../IndAlarmIf1/indalarmif1_gl.h"
#endif

#define IAIF2PORTS  16
#define IAIF2SWS     8


/// Státusz lekérdező üzenet szerkezete ( 13 Byte)
struct qMsg2 {
    char                    beginMark[2];   // 0, 1: Marker '??'
    char                    tMaster[2];     // 2, 3: Target master address (hexa)
    char                    clrErrSt;       // 4:    Hiba státusz törlése (T/F)
    char                    clrKontSt[IAIF2PORTS/4];   // 5- 8: Érzékelők hiba állapot törlése bitvektor (hexa)
    char                    clrSwSt[IAIF2SWS/4];       // 9-10: Kapcsolók hiba állapot törlése bitvektor (hexa)
    char                    endMark[2];     //11,12: Marker CRLF
};

/// Státusz lekérdező üzenetre küldött válasz üzenet szerkezete (38 Byte)
struct aMsg2 {
    char                    beginMark[2];   	       //  0, 1: Marker '..'
    char                    sMaster[2];     	       //  2, 3: Source master address (hexa)
    char                    kontakt[IAIF2PORTS];       //  4-19: Ld.: enum eKStat
    char                    swstat[IAIF2SWS];          // 20-27: Ld.: enum eKStat
    char                    errstat[4];     	       // 28-31: bit vektor hexa
    char                    counter[4];     	       // 32-35: bit vektor hexa
    char                    endMark[2];     	       // 36,37: Marker CRLF
};

/// Parancs üzenet szerkezete (18 Byte)
struct cMsg2 {
    char                    beginMark[2];   	       //  0, 1: Marker '!!'
    char                    tMaster[2];     	       //  2, 3: Target address (hexa)
    char                    disabled[IAIF2PORTS/4];    //  4- 7: bit vektor hexa (ha "!!" az a RESET)
                                                       //        Reverz nincs, azt kezeli a kontroler
    char                    dis_sw[IAIF2SWS/4];        //  8, 9: Kapcsolók engedélyezése
    char                    rev_sw[IAIF2SWS/4];        // 10,11: Riasztás polaritása
    char                    light[4];                  // 12-15: Érzékelők fényereje: piros/zöld; csatik LED fényereje piros/zöld
    char                    endMark[2];     	       // 16,17: Marker CRLF
};

/// Hőmérséklet lekérdezése (23 Byte)
struct tMsg {
    char                    beginMark[2];   	       //  0, 1: Marker '@@'
    char                    tMaster[2];     	       //  2, 3: Target address (hexa)
    char                    cmd;                       //  4:    Command
    char                    rom[16];                   //  5,20: 64bit ROM code
    char                    endMark[2];     	       // 21,22: Marker CRLF
};
/// Hömérséklet válasz: (23 Byte)
struct tAns {
    char                    beginMark[2];   	       //  0, 1: Marker '$$'
    char                    tMaster[2];     	       //  2, 3: Target address (hexa)
    char                    stat;                      //  4:
    char                    data[16];                  //  5,20: 64bit Data code
    char                    endMark[2];     	       // 21,22: Marker CRLF
};

#endif // INDALARMIF2_GL_H
