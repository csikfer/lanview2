#ifndef LV2_GLOBAL_H
#define LV2_GLOBAL_H

/*
This file is part of LanView2.

    Foobar is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

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

enum eEx {
    EX_IGNORE = 0,  // Hiba esetén nem dob kizárást
    EX_ERROR,       // Hiba esetén kizárást dob
    EX_WARNING,     // Bármilyen probléma esetén kizárást dob
    EX_NOOP          // Akkor is kizárást dob, ha nem történt változás
};

static inline enum eEx bool2ex(bool b, eEx ex = EX_ERROR) { return b ? ex : EX_IGNORE; }

#endif // LV2_GLOBAL_H
