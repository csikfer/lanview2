/* Ha nincsen definiálva modul név, akkor nem tudja behúzni a modul hiba kódokat */
#ifndef __MODUL_NAME__
#error Undefined __MODUL_NAME__ macro.
#endif

// Ha ERRCODES_H_DEFINE definiált, akkor már harmadszor illeszti be az állományt
#ifdef ERRCODES_H_DEFINE
 #error Az ercodes.h hibás használata. ERRCODES_H_DEFINE már definiálva van. Töbszörös beillesztés?
#endif

// Ha megváltorott a modul név, akkor 'reset'
#ifdef ERRCODES_H_DECLARE
  #if ERCODES_H_DECLARE_MOD != __MODUL_NAME__
    #undef ERRCODES_H_DECLARE_MOD
  #endif
#endif

#ifdef ERRCOD
  #undef ERRCOD
#endif

#ifdef ERRCODE
  #undef ERRCODE
#endif


#ifndef ERCODES_H_DECLARE
  #define ERCODES_H_DECLARE_MOD __MODUL_NAME__
  #define ERRCOD(id, title)       id,
  #define ERRCODE(id, title, i)   id = i,

  namespace eError {
      enum {
#else
  #define ERRCOD(id, title)       cError::addErrorString(eError::id, QObject::tr(title));
  #define ERRCODE(id, title, i)   cError::addErrorString(eError::id, QObject::tr(title));
#endif

#define     _ADD_QUOTE(s)               #s
#define     ADD_QUOTE(s)                _ADD_QUOTE(s)
#define     _CONCATENATE(a,b)           a##b
#define     CONCATENATE(a,b)            _CONCATENATE(a,b)
#define     INCLUDE_FILE_NAME           CONCATENATE(__MODUL_NAME__, _errcodes.h)
#define     QUOTED_INCLUDE_FILE_NAME    ADD_QUOTE(INCLUDE_FILE_NAME)

#include    QUOTED_INCLUDE_FILE_NAME

#undef      QUOTED_INCLUDE_FILE_NAME
#undef      INCLUDE_FILE_NAME
#undef      ADD_QUOTE
#undef      _ADD_QUOTE
#undef      CONCATENATE
#undef      _CONCATENATE

#if !defined(ERCODES_H_DECLARE)

    };  // end enum
  };  // end namespace

 #define ERCODES_H_DECLARE

#endif

#undef ERRCOD
#undef ERRCODE


