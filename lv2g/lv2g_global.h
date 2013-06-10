#ifndef LV2G_GLOBAL_H
#define LV2G_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LV2G_LIBRARY)
#  define LV2GSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LV2GSHARED_EXPORT Q_DECL_IMPORT
#endif

#define _GEX extern LV2GSHARED_EXPORT

#endif // LV2G_GLOBAL_H
