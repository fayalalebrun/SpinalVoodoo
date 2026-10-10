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

/* NOTE: This file is compiled to naught if we aren't
   running under Win32 */

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

#if defined( __WIN32__ )

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#include <windows.h>
#endif

static CRITICAL_SECTION criticalSectionObject;
static DWORD            tlsIndex;

static FxBool threadInit;
static FxBool criticalSectionInit;

void
initThreadStorage( void )
{
  if ( !threadInit ) {
    threadInit = 1;
    _GlideRoot.tlsIndex = TlsAlloc();

    if (_GlideRoot.tlsIndex == TLS_OUT_OF_INDEXES)
      GrErrorCallback( "initThreadStorage:  Failed to allocate TLS index.", FXTRUE );

    /* pray the index is lower than 64 -- TLS_MINIMUM_AVAILABLE = 64 */
    if (_GlideRoot.tlsIndex >= TLS_MINIMUM_AVAILABLE)
      GrErrorCallback( "initThreadStorage:  TLS index higher than 64.", FXTRUE );
#if !USE_STANDARD_TLS_FUNC
    if (hwcIsOSWin9x()) {
      _GlideRoot.tlsOffset = W95_TLS_INDEX_TO_OFFSET(_GlideRoot.tlsIndex);
    }
    else {
      _GlideRoot.tlsOffset = WNT_TLS_INDEX_TO_OFFSET(_GlideRoot.tlsIndex);
    }
#endif
  }
} /* initThreadStorage */

void setThreadValue( unsigned long value ) {
    GR_CHECK_F( "setThreadValue", !threadInit, "Thread storage not initialized\n" );
    TlsSetValue( _GlideRoot.tlsIndex, (void*)value );
}

unsigned long getThreadValueSLOW( void )
{
    GR_CHECK_F( "getThreadValue", !threadInit, "Thread storage not initialized\n" );

    return getThreadValueFast();
}

#if USE_STANDARD_TLS_FUNC
/* used in gdraw.c (grDrawTriangle) and xos.inc */
unsigned long getThreadValue(void)
{
  return (unsigned long)TlsGetValue(_GlideRoot.tlsIndex);
}
#endif

void freeThreadStorage( void )
{
  if( threadInit ) {
    TlsFree(_GlideRoot.tlsIndex);
    threadInit = 0;
  }
}

void initCriticalSection( void ) {
    if ( !criticalSectionInit ) {
        criticalSectionInit = 1;
        InitializeCriticalSection( &criticalSectionObject );
    }
}

void deleteCriticalSection( void ) {
  if ( criticalSectionInit ) {
    DeleteCriticalSection( &criticalSectionObject );
    criticalSectionInit = 0;
  }
}

void beginCriticalSection( void ) {
    GR_CHECK_F( "beginCriticalSection", !criticalSectionInit, "Critical section not initialized\n" );
    EnterCriticalSection( &criticalSectionObject );
}

void endCriticalSection( void ) {
    GR_CHECK_F( "endCriticalSection", !criticalSectionInit, "Critical section not initialized\n" );
    LeaveCriticalSection( &criticalSectionObject );
}

#elif defined(macintosh)

unsigned long _threadValueMacOS;

void initThreadStorage(void)
{
}

void setThreadValue( unsigned long value )
{
	_threadValueMacOS = value;
}

unsigned long getThreadValueSLOW( void )
{
	return _threadValueMacOS;
}

void freeThreadStorage( void )
{
}

void initCriticalSection(void)
{
}

void deleteCriticalSection( void )
{
}

void beginCriticalSection(void)
{
}

void endCriticalSection(void)
{
}

#elif (GLIDE_PLATFORM & GLIDE_OS_UNIX)

unsigned long threadValueLinux;

void initThreadStorage(void)
{
}

void setThreadValue( unsigned long value )
{
	threadValueLinux = value;
}

unsigned long getThreadValueSLOW( void )
{
	return threadValueLinux;
}

void initCriticalSection(void)
{
}

void deleteCriticalSection( void )
{
}

void beginCriticalSection(void)
{
}

void endCriticalSection(void)
{
}

#elif (GLIDE_PLATFORM & GLIDE_OS_DOS32)

unsigned long GR_CDECL threadValueDJGPP;

void initThreadStorage(void)
{
}

void setThreadValue( unsigned long value )
{
	threadValueDJGPP = value;
}

unsigned long getThreadValueSLOW( void )
{
	return threadValueDJGPP;
}

void initCriticalSection(void)
{
}

void deleteCriticalSection( void )
{
}

void beginCriticalSection(void)
{
}

void endCriticalSection(void)
{
}

#else
#  error "No thread synchronization/storage functions defined for this OS"
#endif
