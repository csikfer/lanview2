#ifndef POPUPREPORT_H
#define POPUPREPORT_H

#include "lv2g.h"
#include "report.h"

_GEX QWidget * popupReportWindow(QObject* _par, const QString& _text, const QString& _title = QString());
_GEX QWidget * popupReportNode(QObject *par, QSqlQuery& q, qlonglong nid);


#endif // POPUPREPORT_H
