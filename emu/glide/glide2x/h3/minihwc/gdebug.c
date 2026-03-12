#include "vxd.h"

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

#undef FX_DLL_ENABLE /* so that we don't dllexport the symbols */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <3dfx.h>

#if __MWERKS__
/* Dork w/ the console window */
#include <sioux.h>

/* So the debug level comes from the right place */
#include <cvg.h>
#include <sst1init.h>
#endif /* __MWERKS__ */

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <fxpci.h>
#include <gdebug.h>

#if defined(__WIN32__) && !defined(KERNEL)
#include <windows.h>
#endif /* defined(__WIN32__) && !defined(KERNEL) */

#define USE_DEBUG_STRING (__DOS32__ || __WIN32__)
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
// we need to call a kernal printf.
extern int __cdecl klvfprintf(FILE        *stream,
                              const char  *format,
                              va_list      arg    ) ;
#endif

static FILE *gdbg_msgfile;	// GDBG info/error file
#else /* #ifdef KERNEL */

#if !defined(__linux__) && !defined(__MINGW32__)
static FILE *gdbg_msgfile = stdout;	// GDBG info/error file
#else
static FILE *gdbg_msgfile;	// GDBG info/error file
#endif

//----------------------------------------------------------------------
// initialize gdbg_level from an environment variable
//----------------------------------------------------------------------
static const char *setRange(const char *buf, int val)
{
    int r0,r1,pos;

    sscanf(buf,"%i%n",&r0,&pos);		// parse the first integer
    if (buf[pos]=='-' || buf[pos]==':') {	// if there's a second
	buf += pos+1;
	sscanf(buf,"%i%n",&r1,&pos);		// then parse it
    }
    else
	r1 = r0;

    if (r0 < 0) r0 = 0;				// sanity checks
    if (r1 >= GDBG_MAX_LEVELS) r1 = GDBG_MAX_LEVELS-1;
    if (r1 < r0) r1 = r0;

    while (r0 <= r1)				// now set the debuglevel levels
	gdbg_debuglevel[r0++] = val;

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

#ifdef __linux__
    gdbg_msgfile = stderr;	// GDBG info/error file
#elif defined(__MINGW32__)
    gdbg_msgfile = stdout;
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
    env = GETENV("GDBG_FILE");
    if (env != NULL) GDBG_SET_FILE(env);
    env = GETENV("GDBG_LEVEL");
    if (env == NULL) env = "0";
    gdbg_parse(env);
    gdbg_info(1,"gdbg_init(): debug level = %s\n",env);
#endif /* #ifndef KERNEL */
}

FX_EXPORT void FX_CSTYLE
gdbg_shutdown(void)
{
  gdbg_info(1,"gdbg_shutdown()\n");
#ifndef KERNEL
  if (gdbg_msgfile != stdout) {	// close any existing output file
#if USE_DEBUG_STRING
    if (!UseDebugString) 
#endif /* USE_DEBUG_STRING */
      fclose(gdbg_msgfile);
    gdbg_msgfile = stdout;
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
}

FX_EXPORT void FX_CSTYLE
gdbg_printf (const char *format, ...)
{
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

    if (!gdbg_debuglevel[level>=GDBG_MAX_LEVELS ? GDBG_MAX_LEVELS-1 : level])
	return(0);
#ifndef KERNEL
    va_start(args, format);
    sprintf(newformat, "%s.%d:\t", gdbg_myname,level);
    strcat(newformat,format);
    gdbg_vprintf(newformat,args);
    va_end(args);
#elif defined( KERNEL_NT )
    gdbg_NTPrint( "%s.%d:\t", gdbg_myname, level );
    va_start(args, format);
    EngDebugPrint( "", format, args );
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

  if (gdbg_msgfile != stdout) {	// close any existing output file
    fclose(gdbg_msgfile);
    gdbg_msgfile = stdout;
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
