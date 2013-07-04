#ifndef IMPORT_PARSER_H
#define IMPORT_PARSER_H

#include "lv2data.h"
#include "lv2user.h"


extern int sImportParse(QString &text);
extern int fImportParse(const QString& fn);


inline static QSqlQuery& qq(void) {  return *((lv2import *)lanView::getInstance())->pq; }

#define UNKNOWN_PLACE_ID 0LL
extern qlonglong globalPlaceId;
static inline qlonglong gPlace() { return globalPlaceId == NULL_ID ? UNKNOWN_PLACE_ID : globalPlaceId; }

/// Létrehoz az adatbázisban egy nodes és egy nports rekordot
/// A port neve és típusa is 'attach" lessz.
/// @param __n A node neve
/// @param __d A node megjegyzés/leírás
/// @return Az nports rekord id-je
extern qlonglong newAttachedNode(QSqlQuery &q, const QString &__n, const QString &__d);
/// Létrehoz az adatbázisban egy nodes és egy hozzá tartozó nports rekord sorozatot
/// A portok neve és típusa is 'attach" lessz.
/// @param __np Egy minta a node nevekhez
/// @param __dp Egy minta a node megjegyzés/leírás -okhoz
/// @param __from Futó index első érték
/// @param __to Futó index utolsó érték
extern void newAttachedNodes(QSqlQuery &q, const QString& __np, const QString& __dp, int __from, int __to);
/// Létrehoz az adatbázisban egy nodes és egy hozzá tartozó interfaces rekordot
/// A portok neve és típusa is 'ethernet" lessz.
/// @param __n Node neve
/// @param __mac MAC cím
/// @param __d megjegyzés/leírás mező értéke.
/// @return Az interfaces rekord id-je
extern qlonglong newWorkstation(QSqlQuery &q, const QString& __n, const cMac& __mac, const QString& __d);

extern bool srcOpen(QFile& f);

class c_yyFile {
public:
    c_yyFile(const c_yyFile& __o) : oldFileName(__o.oldFileName) {
        oldStream = __o.oldStream;
        newFile = __o.newFile;
        oldPos = __o.oldPos;
    }
    c_yyFile() : oldFileName() { oldStream = NULL; newFile = NULL; }
    static void inc(QString *__f);
    static void eoi();
    static int size() { return stack.size(); }
private:
    QTextStream*    oldStream;
    QFile *         newFile;
    QString         oldFileName;
    unsigned int    oldPos;
    static QStack<c_yyFile> stack;
};

class  cArpServerDefs;
extern cArpServerDefs * pArpServerDefs;

class cArpServerDef {
public:
    enum eType { UNKNOWN, SNMP, DHCPD_LOCAL, DHCPD_SSH,  PROC_LOCAL, PROC_SSH  };
    /// A lekérdezés típusa
    enum eType      type;
    /// A szerver címe
    QHostAddress    server;
    /// Fájl paraméter
    QString         file;
    /// Community név
    QString         community;
    ///
    static bool firstTime;
    cArpServerDef() : server(), file(), community() { type = UNKNOWN; }
    cArpServerDef(const cArpServerDef& __o)
        : server(__o.server), file(__o.file), community(__o.community) { type = __o.type; }
    cArpServerDef(enum eType __t, const QString& __s, const QString& __f = QString(), const QString& __c = QString())
        : server(__s), file(__f), community(__c) { type = __t; }
    void updateArpTable(QSqlQuery &q) const;
};

class cArpServerDefs : public QVector<cArpServerDef> {
public:
    cArpServerDefs() : QVector<cArpServerDef>() { ; }
    void append(QSqlQuery &q, const cArpServerDef& __asd) {
        __asd.updateArpTable(q);
        *(cArpServerDefs *)this << __asd;
    }

    void updateArpTable(QSqlQuery &q) {
        ConstIterator i, n = constEnd();
        for (i = constBegin(); i != n; ++i) { i->updateArpTable(q); }
    }
};

enum eShare {
    ES_     = 0,
    ES_A,   ES_AA, ES_AB,
    ES_B,   ES_BA, ES_BB,
    ES_C,   ES_D
};

extern qlonglong    alertServiceId;

/// A fizikai linkeket definiáló blokk adatait tartalmazó, ill. a blokkon
/// belüli definíciókat rögzítő metódusok osztálya.
/// Az objektumban épül fel a rögzítendő rekord.
class cLink {
private:
    enum eSide { LEFT = 1, RIGHT = 2};
    void side(eSide __e, QSqlQuery &q, QString * __n, QString *__p, int __s);
    void side(eSide __e, QSqlQuery &q, QString * __n, int __p, int __s);
    qlonglong& nodeId(eSide __e);
    qlonglong& portId(eSide __e);
public:
    /// Konstruktor
    /// @param __t1 A bal oldali elemen értelmezett link típusa (-1,0,1)
    /// @param __n1 Az alapértelmezett bal oldali elem neve, vagy egy üres string
    /// @param __t2 A jobb oldali elemen értelmezett link típusa (-1,0,1)
    /// @param __n2 Az alapértelmezett jobb oldali elem neve, vagy egy üres string
    cLink(QSqlQuery &q, int __t1, QString *__n1, int __t2, QString *__n2);
    /// A link baloldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port név. A pointert felszabadítja.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás
    void left(QSqlQuery &q, QString * __n, QString *__p, int __s = 0)   { side(LEFT, q, __n,__p, __s); }
    /// A link jobboldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port név. A pointert felszabadítja.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás
    void right(QSqlQuery &q, QString * __n, QString *__p, int __s = 0)  { side(RIGHT, q, __n,__p, __s); }
    /// A link baloldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port index, vagy NULL_ID, ha a kötelezően megadott node egyetlen portját jelöltük ki.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás)
    void left(QSqlQuery &q, QString * __n, int __p, int __s)  { side(LEFT, q, __n, __p, __s); }
    /// A link jobboldali portjának a megadása
    /// @param q
    /// @param __n Opcionális eszköz (node/patch) név. A pointert felszabadítja.
    /// @param __p Port index, vagy NULL_ID, ha a kötelezően megadott node egyetlen portját jelöltük ki.
    /// @param __s Az esetleges megosztás típusának az indexe (0 = nincs megosztás)
    void right(QSqlQuery &q, QString * __n, int __p, int __s) { side(RIGHT, q, __n, __p, __s); }
    /// 'jobb' oldali link egy röptében felvett workstationra.
    /// @arg __n A munkaállomás neve
    /// @arg __mac A munkaáééomás MAC címe
    /// @arg __d node_descr értéke
    void workstation(QSqlQuery &q, QString * __n, cMac * __mac, QString * __d);
    void attached(QSqlQuery &q, QString * __n, QString * __d);
    void insert(QSqlQuery &q, QString * __d, QStringList * __srv);
    void insert(QSqlQuery &q, QString *__hn1, qlonglong __pi1, QString *__hn2, qlonglong __pi2, qlonglong __n);
    static const QString& i2LinkType(int __i);
    static const QString& chkShare(int __s);
    const QString&  phsLinkType1;
    qlonglong       nodeId1;
    qlonglong       portId1;
    const QString&  phsLinkType2;
    qlonglong       nodeId2;
    qlonglong       portId2;
    QString         share;
    bool            newNode;
};

// Rekord ID lekérdezése név alapján, a névre mutató pointert felszabadítja.
extern qlonglong placeId(QSqlQuery &q, const QString *__np);
extern cIfType *ifType(const QString *__np);
extern void setSuperiorHostService(cHostService * phs, QString * phn, QString * psn, QString *ppo = NULL);

extern cNPort *hostAddPort(cNode *pNode, int ix, QString *pt, QString *pn, QStringPair *ip, QVariant *mac, QString *d);
extern void    hostAddAddress(QStringPair *ip, QString *d);
extern cNPort *portAddAddress(QString *pn, QStringPair *ip, QString *d);
extern cNPort *portAddAddress(int ix, QStringPair *ip, QString *d);

template <class T> cNode *newHost(QString *name, QStringPair *ip, QString *mac, QString *d);

void setLastPort(QString * np, qlonglong ix);
void setLastPort(const QString& n, qlonglong ix);
void setLastPort(cRecord *p);

extern QStringList allNotifSwitchs;

typedef QList<QStringPair> QStringPairList;

extern void setSuperior(QStringPair *pshs, QStringPairList *pshl);

cTableShape * newTableShape(QString *pTbl, QString * pMod, const QString *pDescr);

extern tRecordList<cMenuItem>   menuItems;
extern QString *pMenuApp;
extern void newMenuMenu(const QString& _n, const QString& _t);
extern void delMenuItem();
extern void newMenuItem(const QString& _sn, const QString& _n, const QString& _t, const char *typ);
extern cMenuItem& actMenuItem();
extern void setToolTip(const QString& _tt);
extern void setWhatsThis(const QString& _wt);

typedef QList<qlonglong> intList;

extern QString          fileNm;
extern unsigned int     lineNo;
extern QTextStream*     yyif;       //Source file descriptor
extern int yyparse(void);
extern int yyerror(const char * em);
extern int yyerror(QString em);

extern void insertCode(const QString& __txt);

inline cPlace& rPlace(void) {  return *((lv2import *)lanView::getInstance())->pPlace; }

#define place   rPlace()

class cTemplateMapMap : public QMap<QString, cTemplateMap> {
 public:
    /// Konstruktor
    cTemplateMapMap() : QMap<QString, cTemplateMap>() { ; }
    ///
    cTemplateMap& operator[](const QString __t) {
        if (contains(__t)) {
            return  (*(QMap<QString, cTemplateMap> *)this)[__t];
        }
        return *insert(__t, cTemplateMap(__t));
    }

    /// Egy megadott nevű template lekérése, ha nincs a konténerben, akkor beolvassa az adatbázisból.
    /// Az eredményt stringgel tér vissza
    const QString& _get(const QString& __type, const QString&  __name) {
        const QString& r = (*this)[__type].get(qq(), __name);
        return r;
    }
    /// Egy megadott nevű template lekérése, ha nincs a konténerben, akkor beolvassa az adatbázisból.
    /// Az eredményt a macro bufferbe tölti
    void get(const QString& __type, const QString&  __name) {
        insertCode(_get(__type, __name));
    }
    /// Egy adott nevű template elhelyezése a konténerbe, de az adatbázisban nem.
    void set(const QString& __type, const QString& __name, const QString& __cont) {
        (*this)[__type].set(__name, __cont);
    }
    /// Egy adott nevű template elhelyezése a konténerbe, és az adatbázisban.
    void save(const QString& __type, const QString& __name, const QString& __cont, const QString& __descr) {
        (*this)[__type].save(qq(), __name, __cont, __descr);
    }
    /// Egy adott nevű template törlése a konténerből, és az adatbázisból.
    void del(const QString& __type, const QString& __name) {
        (*this)[__type].del(qq(), __name);
    }
};

extern qlonglong    actVlanId;
extern QString      actVlanName;
extern QString      actVlanDescr;
extern enum eSubNetType netType;
extern cPatch *     pPatch;
extern cUser *      pUser;
extern cGroup *     pGroup;
extern cImage *     pImage;
extern cNode *      pNode;
extern cLink      * pLink;
extern cService   * pService;
extern cHostService*pHostService;
extern qlonglong           alertServiceId;
extern QMap<QString, qlonglong>    ivars;
extern QMap<QString, QString>      svars;
// QMap<QString, duble>        rvars;
extern QString       sPortIx;   // Port index
extern QString       sPortNm;   // Port név

inline static qlonglong& vint(const QString& n){
    if (!ivars.contains(n)) yyerror(QString("Integer variable %1 not found").arg(n));
    return ivars[n];
}

inline static QString& vstr(const QString& n){
    if (!svars.contains(n)) yyerror(QString("String variable %1 not found").arg(n));
    return svars[n];
}

inline static const QString& nextNetType() {
    enum eSubNetType r = netType;
    switch (r) {
    case NT_PRIMARY:    netType = NT_SECONDARY;      break;
    case NT_SECONDARY:  break;
    case NT_PRIVATE:
    case NT_PSEUDO:     netType = NT_INVALID;        break;
    default:            yyerror("Compiler error.");
    }
    return subNetType(r);
}

inline static cSnmpDevice& snmpdev() {
    if (pNode == NULL) EXCEPTION(EPROGFAIL);
    return *pNode->reconvert<cSnmpDevice>();
}

enum {
    EP_NIL = -1,
    EP_IP  = 0,
    EP_ICMP = 1,
    EP_TCP = 6,
    EP_UDP = 17
};


#endif // IMPORT_PARSER_H
