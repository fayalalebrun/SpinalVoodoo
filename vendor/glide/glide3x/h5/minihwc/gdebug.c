#include "vxd.h"

/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONL
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGH
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DF
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com).
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR 
** FULL TEXT OF THE NON-WARRANTY PROVISIONS. 
**
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT T
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS I
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FA
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS O
** THE UNITED STATES. 
**
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVE
**
*/

#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <3dfx.h>

#if __MWERKS__
/* Dork w/ the console window */
#include <sioux.h>

#endif /* __MWERKS__ */

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <fxpci.h>
#include <gdebug.h>
#include <glidesys.h>

#if defined(__WIN32__) && !defined(KERNEL)
#include <windows.h>
#endif /* defined(__WIN32__) && !defined(KERNEL) */

#define USE_DEBUG_STRING (__DOS32__ || __WIN32__ || macintosh)
#if USE_DEBUG_STRING
static FxBool UseDebugString = 0;
#endif /* USE_DEBUG_STRING */

#ifdef KERNEL_NT
void __stdcall
EngDebugPrint(
    char * StandardPrefix,
    const char * DebugMessage,
    va_list ap
    );

#endif

static char *gdbg_myname = "gd";		// default library name
static char gdbg_debuglevel[GDBG_MAX_LEVELS];	// array of debuglevel controls

static long gdbg_errors = 0;

#if defined(HWC_EXT_INIT) || (GLIDE_PLATFORM & GLIDE_OS_DOS32) || (GLIDE_PLATFORM & GLIDE_OS_UNIX)
char *hwcGetenv (const char *a);
/*#ifdef GETENV
#undef GETENV
#endif
#define GETENV hwcGetenv*/
#endif


/*
 * For linux, we cannot statically initialize gdbg_msgfile
 * to stderr.  In linux, stderr is not constant.  The same
 * would be true of stdout for that matter.  So, we initialize
 * gdbg_msgfile to NULL and then initialize it to stderr
 * below.  There are two dfns here, one for the static dfn
 * and one for the dynamic dfn.  For linux they are different.
 * For Windows they are the same.
 */
#if defined(__linux__) || defined(__FreeBSD__)
#define	INITIAL_STATIC_GDBG_MSGFILE	NULL
#define INITIAL_GDBG_MSGFILE		stderr
#else
#ifdef NEED_MSGFILE_ASSIGN
#define	INITIAL_STATIC_GDBG_MSGFILE	NULL
#else
#define	INITIAL_STATIC_GDBG_MSGFILE	stdout
#endif
#define INITIAL_GDBG_MSGFILE		stdout
#endif

#ifdef KERNEL
// gdbgout is array to store strings for debug output
//#include <windows.h>
static char gdbgout[512];

// for setting levels interactively through the debugger while you're
// running in the KERNEL mode
void setLevel(int level, int value)
{
    if (level >= GDBG_MAX_LEVELS)
	level = GDBG_MAX_LEVELS - 1;

    gdbg_debuglevel[level] = value;
}


#ifndef KERNEL_NT
// when the simulator runs in kernal mode there is no C runtime library
// so we need to call a kernal printf.
extern int __cdecl klvfprintf(FILE        *stream,
                              const char  *format,
                              va_list      arg    ) ;
#endif

static FILE *gdbg_msgfile;	// GDBG info/error file

#else /* #ifdef KERNEL */

static FILE *gdbg_msgfile = INITIAL_STATIC_GDBG_MSGFILE;	// GDBG info/error file


//----------------------------------------------------------------------
// initialize gdbg_level from an environment variable
//----------------------------------------------------------------------
static const char *setRange(const char *buf, int val)
{
    int r0,r1,pos=0;

    if (sscanf(buf,"%i%n",&r0,&pos) >= 1)       // parse the first integer
    {
      if (buf[pos]=='-' || buf[pos]==':') {     // if there's a second
          buf += pos+1;
          if (sscanf(buf,"%i%n",&r1,&pos) < 1)  // then parse it
          {
             r1 = r0;
             pos = 0;
          }
      }
      else
          r1 = r0;

      if (r0 < 0) r0 = 0;                         // sanity checks
      if (r1 >= GDBG_MAX_LEVELS) r1 = GDBG_MAX_LEVELS-1;
      if (r1 < r0) r1 = r0;

      while (r0 <= r1)                            // now set the debuglevel levels
          gdbg_debuglevel[r0++] = val;
    }

    return buf + pos;				// and return rest of string
}

FX_EXPORT void FX_CSTYLE
gdbg_parse(const char *env)
{
    int level, pos;

    do {
	if (env[0] == ',')		// advance past commas
	    env++;
	if (env[0] == '+')		// if + then enable a range
	    env = setRange(env+1,1);
	else if (env[0] == '-')		// if - then disable a range
	    env = setRange(env+1,0);
	else {				// else just a number
	    if (sscanf(env,"%i%n",&level,&pos) <= 0) return;
	    if (pos==0) return;		// oops, guess not
	    if (level >= GDBG_MAX_LEVELS) level = GDBG_MAX_LEVELS-1;
	    while (level >= 0)		// enable the range [0,#]
		gdbg_debuglevel[level--] = 1;
	    env += pos;
	}
    } while (env[0] == ',');
}

#endif /* #ifndef KERNEL */

void gdbg_init_gdbg_msgfile(void) 
{
    static int done=0;			// only execute once
	if (done)return;
	done = 1;
#ifdef NEED_MSGFILE_ASSIGN
    gdbg_msgfile = INITIAL_GDBG_MSGFILE;
#endif
}

FX_EXPORT void FX_CSTYLE
gdbg_init(void)
{
    static int done=0;			// only execute once
    char *env;

    if (done) return;

#if __MWERKS__
	SIOUXSettings.standalone 				= false;
	SIOUXSettings.setupmenus 				= false;
	SIOUXSettings.autocloseonquit 	= true;
	SIOUXSettings.asktosaveonclose 	= false;
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__MINGW32__)
    gdbg_msgfile = INITIAL_GDBG_MSGFILE;
#endif

#ifdef KERNEL
	// put code in here to set the default level
    gdbg_debuglevel[0] = 1;		// always enable level 0
    gdbg_debuglevel[120] = 1;		// always enable level 0
    done = 1;
    env = 0;
    return;
#else /* #ifdef KERNEL */
    done = 1;
    gdbg_debuglevel[0] = 1;		// always enable level 0
    env = hwcGetenv("GDBG_FILE");
#ifdef _DEBUG
    if (env == NULL) env = "gdbg.txt";
#endif
#if _FIFODUMP
	env = "fifodump.txt";
#endif
    if (env != NULL) GDBG_SET_FILE(env);
    env = hwcGetenv("GDBG_LEVEL");
#ifdef _DEBUG
    if (env == NULL) env = "80";
#endif
    if (env == NULL) env = "0";
#if _FIFODUMP
	env = "444";
#endif
    gdbg_parse(env);
    gdbg_info(1,"gdbg_init(): debug level = %s\n",env);
#endif /* #ifndef KERNEL */
}

FX_EXPORT void FX_CSTYLE
gdbg_shutdown(void)
{
  gdbg_info(1,"gdbg_shutdown()\n");
#ifndef KERNEL
  if (gdbg_msgfile && (gdbg_msgfile != INITIAL_GDBG_MSGFILE)) { // close any existing output file
#if USE_DEBUG_STRING
    if (!UseDebugString) 
#endif /* USE_DEBUG_STRING */
      fclose(gdbg_msgfile);
    gdbg_msgfile = INITIAL_GDBG_MSGFILE;
  }
#endif /* #ifndef KERNEL */
}

#if defined(KERNEL) && !defined(KERNEL_NT)
    extern void MyPrintf();
#endif /* #ifdef KERNEL */

#ifdef KERNEL_NT
//----------------------------------------------------------------------
// NT Debug print helper routine
//----------------------------------------------------------------------
static void gdbg_NTPrint( const char *format, ... )
{
    va_list arglist;

    va_start(arglist, format);
    EngDebugPrint( "\nHAL:  ", format, arglist );
    va_end(arglist);
}
#endif  // KERNEL_NT

static GDBGKeepAliveProc keepAliveProc;

FX_EXPORT void FX_CSTYLE gdbg_set_keepalive(GDBGKeepAliveProc p)
{
   keepAliveProc = p;
}

//----------------------------------------------------------------------
// the MAIN message display suboutine - ALL messages come thru here
//----------------------------------------------------------------------
FX_EXPORT void FX_CSTYLE
gdbg_vprintf (const char *format,va_list args)
{
#ifndef CEASE_ALL_GDBG

  if (gdbg_msgfile != NULL) {
#ifdef KERNEL
    // shouldn't get here now
    //commented out for now -KMW
    //nwvsprintf(gdbgout,format,args);
    //OutputDebugString("\n");
    //OutputDebugString("HAL:  ");
    //OutputDebugString(gdbgout);
#else
#if USE_DEBUG_STRING
    if (UseDebugString) {
      static char msgBuf[1024];

      vsprintf(msgBuf, format, args);

#if __DOS32__
      pciOutputDebugString(msgBuf);
#elif macintosh
      vdprintf(format, args);
#else
      OutputDebugString(msgBuf);
#endif /* !__DOS32__ */
    } else
#endif /* USE_DEBUG_STRING */
    {
      vfprintf(gdbg_msgfile,format,args);
      // if there is a keepAlive callback, then call it
      fflush(gdbg_msgfile);
    }

    if (keepAliveProc) keepAliveProc(100);
#endif /* !KERNEL */
  }

#endif /* !CEASE_ALL_GDBG */
}

FX_EXPORT void FX_CSTYLE
gdbg_printf (const char *format, ...)
{
#ifdef _FIFODUMP
	/* do nothing */
#else
#ifndef KERNEL
    va_list args;

    va_start(args, format);
    gdbg_vprintf(format,args);
    va_end(args);
#elif defined( KERNEL_NT )
    va_list args;

    va_start(args, format);
    EngDebugPrint( "\nHAL:  ", format, args );
    va_end(args);
#else
    __asm lea   eax, (format+4);
    __asm mov   ebx, format;
    MyPrintf();
#endif /* #ifndef KERNEL */
#endif /* _FIFODUMP */

}

//----------------------------------------------------------------------
// INFO message subroutines
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// display an INFO message if level <= debug level and return whether
// debug level high enough to allow display
//----------------------------------------------------------------------
FX_EXPORT int FX_CSTYLE
gdbg_info (const int level, const char *format, ...)
{
    va_list args;
#ifndef KERNEL_NT
    char newformat[4095];
#endif

#ifdef _FIFODUMP
	if (level!=444) return(0);
#endif

    if (!gdbg_debuglevel[level>=GDBG_MAX_LEVELS ? GDBG_MAX_LEVELS-1 : level])
	return(0);

#if defined( KERNEL_NT )
#ifndef _FIFODUMP
    gdbg_NTPrint( "%s.%d:\t", gdbg_myname, level );
#endif
    va_start(args, format);
    EngDebugPrint( "", format, args );
    va_end(args);
#elif !defined( KERNEL )
    va_start(args, format);
#ifdef _FIFODUMP
	strcpy(newformat,format);
#else
    sprintf(newformat, "%s.%d:\t", gdbg_myname,level);
    strcat(newformat,format);
#endif
    gdbg_vprintf(newformat,args);
    va_end(args);
#else /* #ifndef KERNEL */
    Debug_Printf("%s.%d:\t", gdbg_myname, level);
    __asm lea   eax, (format+4);
    __asm mov   ebx, format;
    MyPrintf();
    FXUNUSED(args);
    FXUNUSED(newformat[0]);
#endif /* #ifndef KERNEL */

    return (1);
}


//----------------------------------------------------------------------
// same as gdbg_info but does not display INFO header
//----------------------------------------------------------------------
FX_EXPORT int FX_CSTYLE
gdbg_info_more (const int level, const char *format, ...)
{
    va_list args;

#ifdef _FIFODUMP
	if(level!=444) return(0);
#endif

    if (!gdbg_debuglevel[level>=GDBG_MAX_LEVELS ? GDBG_MAX_LEVELS-1 : level])
	return(0);
#ifndef KERNEL
    va_start(args, format);
    gdbg_vprintf(format,args);
    va_end(args);
#elif defined( KERNEL_NT )
    va_start(args, format);
    EngDebugPrint( "\nHAL:  ", format, args );
    va_end(args);
#else
    __asm lea   eax, (format+4);
    __asm mov   ebx, format;
    MyPrintf();
    FXUNUSED(args);
#endif /* #ifndef KERNEL */
    return (1);
}

static GDBGErrorProc errorProcList[3];

FX_EXPORT int FX_CSTYLE gdbg_error_set_callback(GDBGErrorProc p)
{
   int i;
   const int count = sizeof(errorProcList) / sizeof(errorProcList[0]);

   for(i = 0; i < count; i++) {
      if (errorProcList[i] == p) {
         break;
      } else if (errorProcList[i] == NULL) {
         errorProcList[i] = p;
         break;
      }
   }

   return (i < count);
}

FX_EXPORT void FX_CSTYLE gdbg_error_clear_callback(GDBGErrorProc p)
{
   int i;
   const int count = sizeof(errorProcList) / sizeof(errorProcList[0]);

   for(i = 0; i < count; i++) {
      if (errorProcList[i] == p) {
         errorProcList[i] = NULL;
         break;
      }
   }
}

//----------------------------------------------------------------------
// ALL errors must come thru here, this subroutine adds a preamble
// and then displays the message and increments the error counter
//----------------------------------------------------------------------
FX_EXPORT void FX_CSTYLE
gdbg_error (const char *kind, const char *format, ...)
{
#ifndef KERNEL
    va_list args;

    char newformat[1024];

    va_start(args, format);
    sprintf(newformat, "%s error (%s): ", gdbg_myname,kind);
    strcat(newformat,format);		// add a preamble to message
    gdbg_vprintf(newformat,args);
    gdbg_errors++;			// increment the error counter
    va_end(args);
    
    {
       int i;
       const int count = sizeof(errorProcList) / sizeof(errorProcList[0]);
       
       for(i = 0; i < count; i++) {
          if (errorProcList[i] != NULL) {
             va_start(args, format);
             (*errorProcList[i])(kind, newformat, args);
             va_end(args);
          }
       }
    }
#elif defined( KERNEL_NT )
    va_list args;

    gdbg_NTPrint( "%s error (%s): ", gdbg_myname, kind);
    va_start(args, format);
    EngDebugPrint( "", format, args );
    va_end(args);
#else
    Debug_Printf("%s error (%s): ", gdbg_myname, kind);
    __asm lea   eax, (format+4);
    __asm mov   ebx, format;
    MyPrintf();
#endif /* #ifndef KERNEL */
}

// return the error counter
FX_EXPORT int FX_CSTYLE
gdbg_get_errors(void)
{
    return gdbg_errors;
}

// return a debuglevel level
FX_EXPORT int FX_CSTYLE
gdbg_get_debuglevel(const int level)
{
    return gdbg_debuglevel[level>=GDBG_MAX_LEVELS ? GDBG_MAX_LEVELS-1 : level];
}

// set a debuglevel level
FX_EXPORT void FX_CSTYLE
gdbg_set_debuglevel(const int level, const int value)
{
    gdbg_debuglevel[level>=GDBG_MAX_LEVELS ? GDBG_MAX_LEVELS-1 : level] = value;
}

// open up a new output file
FX_EXPORT int FX_CSTYLE
gdbg_set_file(const char *name)
{
#ifndef KERNEL
  FILE *outf;

  //
  // For Linux, fclose fails if its parameter is NULL.
  //
  if (gdbg_msgfile && (gdbg_msgfile != INITIAL_GDBG_MSGFILE)) { // close any existing output file
    fclose(gdbg_msgfile);
    gdbg_msgfile = INITIAL_GDBG_MSGFILE;
  }

#if USE_DEBUG_STRING
  if (!strcmp(name, "DEBUG")) {
    gdbg_msgfile = (FILE *) 1;
    UseDebugString = 1;
  } else 
#endif /* USE_DEBUG_STRING */
  {
    outf = fopen(name,"w");		// open up a new one
    if (outf) gdbg_msgfile = outf;
    return (outf != NULL);
  }

  return 1;
#else /* #ifndef KERNEL */
  return 0;
#endif /* #ifndef KERNEL */
}
