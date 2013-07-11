#ifndef LV2G_H
#define LV2G_H

#include "lv2g_global.h"
#include "guidata.h"
#include <QtWidgets/QLayout>
#include <QtWidgets/QLabel>

/// A kiszelektált sorok listájával tér vissza
_GEX QList<int> selection2rows(const QItemSelection& _sel);
_GEX QList<int>& modSelectedRows(QList<int>& rows, const QItemSelection& _on, const QItemSelection& _off);

template <class T, class V> static inline bool fromEdit(cRecord& __o, const T& __i, const V&  __v)
{
    if (!__o.isIndex(__i)) {
        if (__v.isEmpty()) return true;    // Nincs ilyen mező, és nem rakunk bele semmit, OK
        else               return false;   // Nincs ilyen mező, de raknánk bele valamit az már gáz
    }
    if (__v.isEmpty() && __o.isNullable(__i)) __o.clear(__i);   // Ures string esetén, ha lehet NULL, akkor NULL-ra állítjuk
    else                                      __o[__i] = __v;   // Egyébként megy bele az érték
    return true;
}

typedef QList<QHBoxLayout *> hBoxLayoutList;
typedef QList<QLabel *>     labelList;

static inline QWidget *newFrame(int _st, QWidget * p = NULL)
{
    QFrame *pFrame = new QFrame(p);
    pFrame->setFrameStyle(_st);
    return pFrame;
}
static inline QWidget *newHLine(QWidget * p = NULL) { return newFrame(QFrame::HLine, p); }
static inline QWidget *newVLine(QWidget * p = NULL) { return newFrame(QFrame::VLine, p); }

#endif // LV2G_H
