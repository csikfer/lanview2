#ifndef IMPORT_PARSER_YACC_H
#define IMPORT_PARSER_YACC_H

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
extern cHub *       pHub;
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

inline static cHost& host() {
    if (pNode == NULL) EXCEPTION(EPROGFAIL);
    if (pNode->tableoid() != cHost::_descr().tableoid() && pNode->tableoid() != cSnmpDevice::_descr().tableoid()) yyerror("Invalid contex.");
    return *(cHost *)pNode;
}

inline static cSnmpDevice& snmpdev() {
    if (pNode == NULL) EXCEPTION(EPROGFAIL);
    if (pNode->tableoid() != cSnmpDevice::_descr().tableoid()) yyerror("Invalid contex.");
    return *(cSnmpDevice *)pNode;
}

inline static void chkHost()
{
    cHost& h = host();
    // Ha nincs main port név, akkor az alapértelmezett az "ethernet"
    if (h.isNull(_sPortName)) h.setName(_sPortName,_sEthernet);
}

enum {
    EP_NIL = -1,
    EP_IP  = 0,
    EP_ICMP = 1,
    EP_TCP = 6,
    EP_UDP = 17
};


#endif // IMPORT_PARSER_YACC_H
