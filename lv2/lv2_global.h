#ifndef LV2_GLOBAL_H
#define LV2_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LV2_LIBRARY)
#  define LV2SHARED_EXPORT Q_DECL_EXPORT
// #  warning "LIB..."
#else
#  define LV2SHARED_EXPORT Q_DECL_IMPORT
// #  warning "APP..."
#endif

#define EXT_ extern LV2SHARED_EXPORT

/// @def NULL_ID
/// NULL ID-t reprezentáló konstans.
#define NULL_ID     LLONG_MIN
/// @def NULL_IX
/// NULL index-et-t reprezentáló konstans.
#define NULL_IX     INT_MIN


#if   defined(Q_CC_MSVC)

#elif defined(Q_CC_GNU)

#else
#error "Nem támogatott C++ fordító használata."
#endif

#endif // LV2_GLOBAL_H
