
ERRCODE( ELOGON,    "Login incorrect.", __LAST_ERROR_CODE__ +1)

#undef  __LAST_ERROR_CODE__
#define __LAST_ERROR_CODE__ ELOGON
