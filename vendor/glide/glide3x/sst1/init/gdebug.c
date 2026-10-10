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

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include "gdebug.h"

#define GETENV getenv

static char *gdbg_myname = "gd";		// default library name
static char gdbg_debuglevel[GDBG_MAX_LEVELS];	// array of debuglevel controls

static long gdbg_errors = 0;

static FxBool UseDebugString;

static FILE *gdbg_msgfile = NULL; /* stdout; */	// GDBG info/error file

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

static void parse(const char *env)
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
	    if (EOF == sscanf(env,"%i%n",&level,&pos)) return;
	    if (pos==0) return;		// oops, guess not
	    if (level >= GDBG_MAX_LEVELS) level = GDBG_MAX_LEVELS-1;
	    while (level >= 0)		// enable the range [0,#]
		gdbg_debuglevel[level--] = 1;
	    env += pos;
	}
    } while (env[0] == ',');
}

FX_EXPORT void FX_CSTYLE
gdbg_init(void)
{
    static int done=0;			// only execute once
    const char *env;

    if (done) return;

    /* I can't init gdbg_msgfile to stdout since it isn't constant so
     * I do it now */
    gdbg_msgfile = stdout;

    done = 1;
    gdbg_debuglevel[0] = 1;		// always enable level 0
    env = GETENV("GDBG_FILE");
    if (env != NULL) GDBG_SET_FILE(env);
    env = GETENV("GDBG_LEVEL");
    if (env == NULL) env = "0";
    parse(env);
    gdbg_info(1,"gdbg_init(): debug level = %s\n",env);
}

FX_EXPORT void FX_CSTYLE
gdbg_shutdown(void)
{
  gdbg_info(1,"gdbg_shutdown()\n");
  if (gdbg_msgfile != stdout) {	// close any existing output file
    fclose(gdbg_msgfile);
    gdbg_msgfile = stdout;
  }
}

//----------------------------------------------------------------------
// the MAIN message display suboutine - ALL messages come thru here
//----------------------------------------------------------------------
FX_EXPORT void FX_CSTYLE
gdbg_vprintf (const char *format,va_list args)
{
  if (gdbg_msgfile != NULL) {
    if (!UseDebugString) {
      vfprintf(gdbg_msgfile,format,args);
      fflush(gdbg_msgfile);
    }
#ifdef __WIN32__
    else {
      char msgString[1024];

      vsprintf(msgString, format, args);
      OutputDebugString(msgString);
    }
#endif
  }
}

FX_EXPORT void FX_CSTYLE
gdbg_printf (const char *format, ...)
{
    va_list args;

    va_start(args, format);
    gdbg_vprintf(format,args);
    va_end(args);
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
    char newformat[4095];

    if (!gdbg_debuglevel[level>=GDBG_MAX_LEVELS ? GDBG_MAX_LEVELS-1 : level])
	return(0);
    va_start(args, format);
    sprintf(newformat, "%s.%d:\t", gdbg_myname,level);
    strcat(newformat,format);
    gdbg_vprintf(newformat,args);
    va_end(args);
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
    va_start(args, format);
    gdbg_vprintf(format,args);
    va_end(args);
    return (1);
}

//----------------------------------------------------------------------
// ALL errors must come thru here, this subroutine adds a preamble
// and then displays the message and increments the error counter
//----------------------------------------------------------------------
FX_EXPORT void FX_CSTYLE
gdbg_error (const char *kind, const char *format, ...)
{
    va_list args;

    char newformat[1024];

    va_start(args, format);
    sprintf(newformat, "%s error (%s): ", gdbg_myname,kind);
    strcat(newformat,format);		// add a preamble to message
    gdbg_vprintf(newformat,args);
    gdbg_errors++;			// increment the error counter
    va_end(args);
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
  FILE *outf;

  if (gdbg_msgfile != stdout) {	// close any existing output file
    fclose(gdbg_msgfile);
    gdbg_msgfile = stdout;
  }
#ifdef __WIN32__
  else if (!strcmp(name, "DEBUG")) {
    gdbg_msgfile = (FILE *) 1;
    UseDebugString = 1;
  }
#endif
  else {
    outf = fopen(name,"w");		// open up a new one
    if (outf) gdbg_msgfile = outf;
    return (outf != NULL);
  }
  return FXTRUE;
}
