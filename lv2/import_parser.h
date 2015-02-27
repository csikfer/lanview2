#ifndef IMPORT_PARSER_H
#define IMPORT_PARSER_H

#include "lv2data.h"
#include "lv2user.h"

extern QString importFileNm;               ///< Source file name
extern unsigned int importLineNo;          ///< Source line counter
extern QTextStream* importInputStream;     ///< Source stream
extern cError *importLastError;            ///< Last error object, or NULL pointer

/// Parse string
/// Hiba esetén a importLastError pointer a hiba okát leíró objekumra mutat
/// @param text A feldolgozandó string
/// @return Ha nem volt hiba akkor 0, egyébként a hiba kód.
extern int importParseText(QString text);
/// Parse file
/// Hiba esetén a importLastError pointer a hiba okát leíró objekumra mutat
/// @param fn A feldolgozandó fájl neve lsd.: bool importSrcOpen(QFile& f);
/// @return Ha nem volt hiba akkor 0, egyébként a hiba kód.
extern int importParseFile(const QString& fn);
///
extern int importParse();

/// Megníitja olvasásra a megadott nevű forrás fájlt.
/// Elöszőr az aktuális könyvtárban, ill. ha a fájl nem létezik, akkor a home könyvtárban próbállja megnyitni.
/// @return true, ha sikerült a fájlt megnyitni.
extern bool importSrcOpen(QFile& f);
/// Az import parse híváshoz alaphelyzezbe állítja a parser globális változóit.
/// (Nem azokat a globális pointereket, melyet a void initImportParser(); már felszabadított.)
extern void initImportParser();
/// A parser hívás után az esetleg (pl. hiba esetén) fel nem szabadított pointereket felszabadítja, ill nullázza.
extern void downImportParser();

// Ez a kód csak azért került ide, mert a qtcreator nem támogatja a .yy forrásokat, és abban nem működik a kodkiegészítés, szinezés ...
#if defined(LV2_LIBRARY)
// Mivel ez a kódrész nem teljesen idevaló, a fordító reklamál
static void insertCode(const QString& __txt);
static QSqlQuery      *piq = NULL;
static inline QSqlQuery& qq() { if (piq == NULL) EXCEPTION(EPROGFAIL); return *piq; }

class cHostServices : public tRecordList<cHostService> {
public:
    cHostServices(QSqlQuery& q, QString * ph, QString * pp, QString * ps, QString * pn) : tRecordList<cHostService>()
    {
        tRecordList<cNode> hl;
        hl.fetchByNamePattern(q, *ph, false); delete ph;
        tRecordList<cService> sl;
        sl.fetchByNamePattern(q, *ps, false); delete ps;
        for (tRecordList<cNode>::iterator i = hl.begin(); i < hl.end(); ++i) {
            cNode &h = **i;
            for (tRecordList<cService>::iterator j = sl.begin(); j < sl.end(); ++j) {
                cService &s = **j;
                cHostService *phs = new cHostService();
                phs->setId(_sNodeId,    h.getId());
                phs->setId(_sServiceId, s.getId());
                if (pn) phs->setName(_sHostServiceNote, *pn);
                if (pp) phs->setId(_sPortId, cNPort().getPortIdByName(q, *pp, h.getId()));
                *this << phs;
            }
        }
        pDelete(pn);
        pDelete(pp);
    }
    cHostServices(QSqlQuery& q, cHostService *&_p)  : tRecordList<cHostService>(_p)
    {
        _p = NULL;
        while (true) {
            cHostService *p = new cHostService;
            if (!p->next(q)) {
                delete p;
                return;
            }
            this->append(p);
        }
    }
    void setsPort(QSqlQuery& q, const QString& pn) {
        for (int i = 0; i < size(); ++i) {
            at(i)->set(_sPortId, cNPort().getPortIdByName(q, pn, at(i)->getId(_sNodeId)));
        }
    }
    void cat(cHostServices *pp) {
        cHostService *p;
        while (NULL != (p = pp->pop_front(false))) pp->append(p);
        delete pp;
    }
};

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
        if (__cont.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Üres minta."))
        (*this)[__type].set(__name, __cont);
    }
    /// Egy adott nevű template elhelyezése a konténerbe, és az adatbázisban.
    void save(const QString& __type, const QString& __name, const QString& __cont, const QString& __descr) {
        if (__cont.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Üres minta."))
        (*this)[__type].save(qq(), __name, __cont, __descr);
    }
    /// Egy adott nevű template törlése a konténerből, és az adatbázisból.
    void del(const QString& __type, const QString& __name) {
        (*this)[__type].del(qq(), __name);
    }
};


#endif

#endif // IMPORT_PARSER_H
