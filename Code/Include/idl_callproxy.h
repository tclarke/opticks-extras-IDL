/*
 * idl_callproxy.h (Non-DLL version)
 *
 * THIS FILE IS GENERATED AUTOMATICALLY.
 * CHANGES MADE HERE ARE NOT PERMANENT:
 *
 *      Date:             Fri Mar 23 22:58:17 2007
 *      Operating System: MSWIN
 *      Architecture:     i386
 *      User:             idl@engbuild01
 *      Script:           mk_idl_callproxy
 */


#ifndef idl_callproxy_IDL_DEF
#define idl_callproxy_IDL_DEF

#ifdef __cplusplus
extern "C"{
#endif

/*
 * IDL_CallProxyInit() initializes the Callable IDL DLL, and must
 * be called before any calls to the Callable IDL API. Typically,
 * you will call it just before calling IDL_ToutPush() and IDL_Win32Init().
*/
int __stdcall IDL_CallProxyInit(char *path);


/*
 * The callproxy library is silent in the face of errors, leaving the
 * caller to handle them. There are two problems that the caller faces
 * in doing so:
 *
 *    (1) If a library fails to load, there is no way to know which
 *        library failed. A TRUE/FALSE return from IDL_CallProxyInit()
 *        doesn't convey much information.
 *    (2) If an attempt to locate a Callable IDL function from the IDL
 *	  DLL fails, no error is returned by the callproxy function that
 *        wraps it, because there is no way to return such information.
 *
 * By calling IDL_CallProxyDebug() before your call to IDL_CallProxyInit(),
 * you can tell the callproxy library to use the Windows MessageBox()
 * function to report errors. The flags argument is a bitmask that can
 * take any combination of the IDL_CPDEBUG_ values given here. The old
 * flags value is returned by the function, and can be used to reset the
 * original state.
*/
#define IDL_CPDEBUG_REQDLLNOLOAD  1 /* Required DLL failed to load */
#define IDL_CPDEBUG_OPTDLLNOLOAD  2 /* Optional DLL failed to load */
#define IDL_CPDEBUG_DLLLOAD       4 /* DLL successfully loaded */
#define IDL_CPDEBUG_FCNNORES      8 /* Unable to resolve function in IDL DLL */
#define IDL_CPDEBUG_FCNRES       16 /* IDL function successfully resolved */

#define IDL_CPDEBUG_ALL   	  0x1f

int __stdcall IDL_CallProxyDebug(int flags);

#ifdef __cplusplus
}
#endif


#endif /* idl_callproxy_IDL_DEF */
