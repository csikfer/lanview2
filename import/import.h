#ifndef IMPORT_H
#define IMPORT_H

#include <QtCore>
#include <QPoint>
#include "lanview.h"
#include "ping.h"
#include "guidata.h"

#define APPNAME "import"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  IMPORT

class lv2import : public lanView {
    Q_OBJECT
   public:
    lv2import();
    ~lv2import();
    void abortOldRecords();
    QSqlQuery   *pq;
    cPlace      *pPlace;
    QString     fileNm;
    QFile       in;
    bool        daemonMode;
private slots:
    void dbNotif(QString __s);
};

inline static QSqlQuery& qq(void) {  return *((lv2import *)lanView::getInstance())->pq; }

extern qlonglong globalPlaceId;
#define UNKNOWN_PLACE_ID 0LL
static inline qlonglong gPlace() { return globalPlaceId == NULL_ID ? UNKNOWN_PLACE_ID : globalPlaceId; }

extern cNode newAttachedNode(const QString& __n, const QString&  __d);
extern void newAttachedNodes(const QString& __np, const QString& __dp, int __from, int __to);
extern cHost newWorkstation(const QString& __n, const cMac& __mac, const QString& __d);

extern cArpTable *pArpTable;
class cArpServerDefs;
extern cArpServerDefs * pArpServerDefs;

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

// typedef QPair<QString,QString>  QStringPair;

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
    cArpServerDef() : server(), file(), community() { type = UNKNOWN; }
    cArpServerDef(const cArpServerDef& __o)
        : server(__o.server), file(__o.file), community(__o.community) { type = __o.type; }
    cArpServerDef(enum eType __t, const QString& __s, const QString& __f = QString(), const QString& __c = QString())
        : server(__s), file(__f), community(__c) { type = __t; }
    void updateArpTable(cArpTable& __at) const;
    void reUpdateArpTable(cArpTable& __at) const;
};

class cArpServerDefs : public QVector<cArpServerDef> {
public:
    cArpServerDefs() : QVector<cArpServerDef>() { ; }
    void append(const cArpServerDef& __asd, cArpTable& __at) {
        __asd.updateArpTable(__at);
        *(cArpServerDefs *)this << __asd;
    }

    void updateArpTable(cArpTable& __at) {
        ConstIterator i, n = constEnd();
        for (i = constBegin(); i != n; ++i) { i->updateArpTable(__at); }
    }
    void reUpdateArpTable(cArpTable& __at) {
        ConstIterator i, n = constEnd();
        for (i = constBegin(); i != n; ++i) { i->reUpdateArpTable(__at); }
    }
};

enum eShare {
    ES_     = 0,
    ES_A,   ES_AA, ES_AB,
    ES_B,   ES_BA, ES_BB,
    ES_C,   ES_D
};

extern qlonglong    alertServiceId;

class cLink {
public:
    cLink(int __t1, QString *__n1, int __t2, QString *__n2);
    void left(QString * __n, QString *__p, int __s);
    void right(QString * __n, QString *__p, int __s);
    void left(QString * __n, int __p, int __s);
    void right(QString * __n, int __p, int __s);
    /// 'jobb' oldali link egy röptében felvett workstationra.
    /// @arg __n A munkaállomás neve
    /// @arg __mac A munkaáééomás MAC címe
    /// @arg __d node_descr értéke
    void workstation(QString * __n, cMac * __mac, QString * __d);
    void attached(QString * __n, QString * __d);
    void insert(QString * __d, QStringList * __srv);
    void insert(QString *__hn1, qlonglong __pi1, QString *__hn2, qlonglong __pi2, qlonglong __n);
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
extern qlonglong placeId(const QString *__np);
extern cIfType *ifType(const QString *__np);
extern void setSuperiorHostService(cHostService * phs, QString * phn, QString * psn, QString *ppo = NULL);

extern cNPort *hostAddPort(int ix, QString *pt, QString *pn, QStringPair *ip, QVariant *mac, QString *d);
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

#endif // IMPORT_H
