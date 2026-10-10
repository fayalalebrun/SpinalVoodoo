/*-*-c++-*-*/
#ifndef __GDEBUG_H__
#define __GDEBUG_H__

/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
*/

#include <stdarg.h>

#if defined(FX_DLL_ENABLE)
#define FX_DLL_DEFINITION

#endif
#include <fxdll.h>
#include <minihwc.h>

#define GDBG_MAX_LEVELS 512

#ifndef GETENV
#ifndef __linux__
#define GETENV(a) getenv(a)
#else
#define GETENV hwcGetenv
#endif
#endif

// if debug info turned on then GDBG_INFO does something
#ifdef GDBG_INFO_ON

// Standard C provides no clean way to do this, but the GNU C tools do..
#ifdef __GNUC__

#define GDBG_INFO(level, format, args...)	\
    gdbg_info(level, format , ## args)
#define GDBG_INFO_MORE(level, format, args...)	\
    gdbg_info_more(level, format , ## args)
#define GDBG_PRINTF(format, args...)		\
    gdbg_printf(format , ## args)

#else

#define GDBG_INFO gdbg_info
#define GDBG_INFO_MORE gdbg_info_more
#define GDBG_PRINTF gdbg_printf

#endif

#define GDBG_ERROR_SET_CALLBACK   gdbg_error_set_callback
#define GDBG_ERROR_CLEAR_CALLBACK gdbg_error_clear_callback

#define GDBG_GET_DEBUGLEVEL	gdbg_get_debuglevel
#define GDBG_SET_DEBUGLEVEL	gdbg_set_debuglevel

// otherwise GDBG_INFO does nothing
#else

#if defined(__WATCOMC__) || defined(__WATCOM_CPLUSPLUS__)
/* Turn off the dead code warnings. Also changed the macro definitions
 * to use an 'if' rather than the ternary operator because the
 * type of the result sub-expressions must match.
 *
 * w111: Meaningless use of an expression
 * w201: Unreachable code
 * w302: Expression only useful for side-effects.
 */
#pragma disable_message (111, 201, 302)
#endif /* defined(__WATCOMC__) || defined(__WATCOM_CPLUSPLUS__) */

// Standard C provides no clean way to do this, but the GNU C tools do..
#ifdef __GNUC__

#define GDBG_INFO(level, format, args...)
#define GDBG_INFO_MORE(level, format, args...)
#define GDBG_PRINTF(format, args...)

#else

#define GDBG_INFO      0 && (unsigned long)
#define GDBG_INFO_MORE 0 && (unsigned long)
#define GDBG_PRINTF    0 && (unsigned long)

#define GDBG_ERROR_SET_CALLBACK   0 && (unsigned long)
#define GDBG_ERROR_CLEAR_CALLBACK 0 && (unsigned long)

#endif

#define GDBG_GET_DEBUGLEVEL(x) 0
#define GDBG_SET_DEBUGLEVEL(a,b)


#endif

#define GDBG_INIT		gdbg_init
#define GDBG_SHUTDOWN		gdbg_shutdown
#define GDBG_ERROR		gdbg_error
#define GDBG_GET_ERRORS		gdbg_get_errors
#define GDBG_SET_FILE		gdbg_set_file

FX_ENTRY void	FX_CALL gdbg_init(void);
FX_ENTRY void	FX_CALL gdbg_parse(const char *env);
FX_ENTRY void	FX_CALL gdbg_shutdown(void);
FX_ENTRY void	FX_CALL gdbg_vprintf(const char *format, va_list);
FX_ENTRY void	FX_CALL gdbg_printf(const char *format, ...);
FX_ENTRY int	FX_CALL gdbg_info(const int level, const char *format, ...);
FX_ENTRY int	FX_CALL gdbg_info_more(const int level, const char *format, ...);
FX_ENTRY void	FX_CALL gdbg_error(const char *name, const char *format, ...);
FX_ENTRY int	FX_CALL gdbg_get_errors(void);
FX_ENTRY int	FX_CALL gdbg_set_file(const char *name);
FX_ENTRY int	FX_CALL gdbg_get_debuglevel(const int level);
FX_ENTRY void	FX_CALL gdbg_set_debuglevel(const int level, const int value);

// these routines allow for a library (like Glide) to get called back
typedef void (*GDBGErrorProc)(const char* const procName,
                              const char* const format,
                              va_list     args);
FX_ENTRY int FX_CALL gdbg_error_set_callback(GDBGErrorProc p);
FX_ENTRY void FX_CALL gdbg_error_clear_callback(GDBGErrorProc p);

// these routines allow for some GUI code to get called once in a while
// so that it can keep the UI alive by reading the message queue
typedef void (*GDBGKeepAliveProc)(int adjust);
FX_ENTRY void FX_CALL gdbg_set_keepalive(GDBGKeepAliveProc p);

#endif /* !__GDEBUG_H__ */
