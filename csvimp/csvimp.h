#ifndef CSVIMP
#define CSVIMP

#include <Qt>
#include <QPoint>
#include "lanview.h"
#include "ping.h"
#include "guidata.h"

#define APPNAME "import"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2csvimp : public lanView {
    Q_OBJECT
   public:
    lv2csvimp();
    ~lv2csvimp();
    QSqlQuery   *pq;
    QString     fileNm;
    QFile       in;
    void readcsv();
    void csvrow(QStringList &fields);
    QString floor;              ///< Emelet neve
    QString _room;              ///< Hely helyiség (szoba) neve
    QString side;               ///< Több fakk esetán a nevet kiegészíitó jel
    QString room;               ///< Hely helyiség (szoba) neve + fakk
    QString wport;              ///< A port felirata a szobában
    QString share;              ///< megosztás
    QString swname;             ///< a port "túloldalán" észlelt switch neve
    QString swport;             ///< a port "túloldalán" észlelt switch port neve
    QString category;           ///< a helyiség kategória
    int     need;               ///< szükséges port szám a helyiségben (kiegészíti a kategóriát, ha van)

    cPlace      placeFloor;     ///< Emelet (a helyíség parent-je)
    cPlace      placeRoom;      ///< Helyiség
    cPlace      placePatch;     ///< A rendező helye
    cNode       nodeSwitch;     ///< A switch objektum (rendezőben)
    cInterface  intSwitch;      ///< A switch portja (rendezőben)
    cPPort      pport;          ///< Patch port (rendezőben)
    cPlaceGroup plgrCat;        ///< Helyiség katgória/csoport
    cPatch      roomSocket;     ///< Hipotetikus fali csatlakozó, minden szobához 1db
};


#endif // CSVIMP

