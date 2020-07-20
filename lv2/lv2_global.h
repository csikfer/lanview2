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
#include <QtCore>

#define LV2_THROW 1

#if defined(LV2_LIBRARY)
#  define LV2SHARED_EXPORT Q_DECL_EXPORT
// #  warning "LIB..."
#else
#  define LV2SHARED_EXPORT Q_DECL_IMPORT
// #  warning "APP..."
#endif

#define EXT_ extern LV2SHARED_EXPORT

#ifdef Q_CC_GNU
#   define LV2_ATR_NORET_ __attribute__((noreturn))
    // C++17
#   if __cplusplus >= 201703L
#       define LV2_FALLTHROUGH [[fallthrough]];
/*
    // C++14
#   elif __cplusplus >= 201402L
#       define LV2_FALLTHROUGH [[gnu::fallthrough]];
    // C++11
#   elif __cplusplus >= 201103L
#       define LV2_FALLTHROUGH [[gnu::fallthrough]];
*/
#   else
//#     define LV2_FALLTHROUGH [[clang::fallthrough]]
#      if defined(__GNUC__) && __GNUC__ >= 7
#          define LV2_FALLTHROUGH __attribute__ ((fallthrough));
#      else
#	   define LV2_FALLTHROUGH
#      endif
#   endif
#elif  Q_CC_MSVC
#   define LV2_ATR_NORET_ __declspec(noreturn)
#   if __cplusplus >= 201703L
#       define LV2_FALLTHROUGH [[fallthrough]];
#   else
#       define LV2_FALLTHROUGH
#   endif
#else
#   define LV2_ATR_NORET_
#   define LV2_FALLTHROUGH
#endif

#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
#define swapItemsAt swap
#endif


#if defined(__cplusplus) && __cplusplus < 201703L
    // ?!
#   define nullptr NULL
#endif


/// @def NULL_ID
/// NULL ID-t reprezentáló konstans.
#define NULL_ID     LLONG_MIN
/// @def NULL_IX
/// NULL index-et-t reprezentáló konstans.
#define NULL_IX     INT_MIN


#if   defined(Q_CC_MSVC)
    // OK
#elif defined(Q_CC_GNU)
    // OK
#else
#error "Nem támogatott C++ fordító használata."
#endif

enum eEx {
    EX_IGNORE = 0,  // Hiba esetén nem dob kizárást
    EX_ERROR,       // Hiba esetén kizárást dob
    EX_WARNING,     // Bármilyen probléma esetén kizárást dob
    EX_NOOP         // Akkor is kizárást dob, ha nem történt változás
};

inline enum eEx bool2ex(bool b, eEx ex = EX_ERROR) { return b ? ex : EX_IGNORE; }

/// Nem értelmezhető numrikus enumerációs érték.
#define ENUM_INVALID    -1

enum eTristate {
    TS_NULL = -1,
    TS_FALSE=  0,
    TS_TRUE =  1
};

typedef QPair<QString, QString>     tStringPair;
/// A QPolygonF osztály a GUI része, így nem használlható
/// Mert vagy nem GUI-val fordítjuk, és akkor elszállhat (pl. a QVariant-ból való kikonvertáláskor)
/// Vagy GUI-val fordítunk, de akkor meg nem fog elindulni tisztán konzolos módban
typedef QList<QPointF>  tPolygonF;

/// Egész szám (int/index) vektor
typedef QVector<int>        tIntVector;
/// Egész szám (qlonglong/id) vektor
typedef QVector<qlonglong>  tLongLongVector;

typedef QMap<QString, QString>  tStringMap;


#endif // LV2_GLOBAL_H
