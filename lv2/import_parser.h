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

#if defined(LV2_LIBRARY)


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

#endif

#endif // IMPORT_PARSER_H
