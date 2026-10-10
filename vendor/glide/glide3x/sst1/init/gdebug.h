/* 
 * THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
 * PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
 * TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
 * INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
 * DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
 * THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
 * FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
 * 
 * USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
 * RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
 * TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
 * AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
 * SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
 * THE UNITED STATES.  
 * 
 * COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
 */

#ifndef __GDEBUG_H__
#define __GDEBUG_H__

#include <stdarg.h>
#include <3dfx.h>

#define GDBG_MAX_LEVELS 512

// if debug info turned on then GDBG_INFO does something
#ifdef GDBG_INFO_ON

#define GDBG_INFO(a) gdbg_info a
#define GDBG_INFO_MORE(a) gdbg_info_more a
#define GDBG_PRINTF(a) gdbg_printf a

// otherwise GDBG_INFO does nothing
#else

#define GDBG_INFO(a)
#define GDBG_INFO_MORE(a)
#define GDBG_PRINTF(a)

#endif

#define GDBG_INIT               gdbg_init
#define GDBG_ERROR              gdbg_error
#define GDBG_GET_ERRORS         gdbg_get_errors
#define GDBG_GET_DEBUGLEVEL     gdbg_get_debuglevel
#define GDBG_SET_DEBUGLEVEL     gdbg_set_debuglevel
#define GDBG_SET_FILE           gdbg_set_file

FX_ENTRY void FX_CALL gdbg_init(void);
FX_ENTRY void FX_CALL gdbg_vprintf(const char *format, va_list);
FX_ENTRY void FX_CALL gdbg_printf(const char *format, ...);
FX_ENTRY int  FX_CALL gdbg_info(const int level, const char *format, ...);
FX_ENTRY int  FX_CALL gdbg_info_more(const int level, const char *format, ...);
FX_ENTRY void FX_CALL gdbg_error(const char *name, const char *format, ...);
FX_ENTRY int  FX_CALL gdbg_get_errors(void);
FX_ENTRY int  FX_CALL gdbg_get_debuglevel(const int level);
FX_ENTRY void FX_CALL gdbg_set_debuglevel(const int level, const int value);
FX_ENTRY int  FX_CALL gdbg_set_file(const char *name);

#endif /* !__GDEBUG_H__ */
