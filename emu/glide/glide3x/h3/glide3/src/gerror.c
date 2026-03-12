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
**
**
** Revision 1.2.6.6  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.2.6.5  2003/11/08 08:39:15  guillemj
** Unify multi-arch implementation of _grErrorDefaultCallback() function.
**
** Revision 1.2.6.4  2003/11/04 12:53:38  dborca
** Removed balanced #endif.
**
** Revision 1.2.6.3  2003/11/03 07:27:50  guillemj
** Remove unbalanced #endif.
**
** Revision 1.2.6.2  2003/07/25 07:13:41  dborca
** removed debris
**
** Revision 1.2.6.1  2003/05/05 07:12:46  dborca
** no message
**
** Revision 1.2  2000/02/15 22:35:58  joseph
** Changes to support FreeBSD (patch submitted by Doug Rabson)
**
** Revision 1.1.1.1  1999/11/24 21:44:56  joseph
** Initial checkin for SourceForge
**
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
** 
** 13    4/05/99 8:25p Dow
** Alt tab mostly happy
** 
** 12    4/04/99 8:51p Atai
** Partial check-in for alt-tab issue. set FX_GLIDE_ALT_TAB=1 to build
** glide3x with hwcQueryContext built into GR_BEGIN_NOFIFOCHECK. It works
** with DEBUG glide only. In the non-debug glide, we can still see the
** desktop corruption.
** 
** 11    3/19/99 11:26a Peter
** expose direct fifo for gl
** 
** 10    3/02/99 2:04p Peter
** made error tracing output use consistent address bases for the fifo
** output
** 
** 9     10/22/98 11:45a Atai
** be consistent with fatal error
** 
** 8     7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 7     7/16/98 8:17a Jdt
** fxcmd.h
** 
** protected some direct hardware reads from TACO_MEMORY_FIFO_HACK
** 
** 6     7/01/98 12:40p Jdt
** Changed grErrorWindowsCallback to grErrorDefault Callback,
**            this removes some conditional compilation from grSstWinOpen
** 
** 5     4/30/98 5:01p Peter
** first pass glide3 merge
** 
** 3     4/22/98 4:57p Peter
** glide2x merge
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 26    12/18/97 2:13p Peter
 * cleaned up the error code
 * 
 * 25    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 24    12/05/97 4:38p Peter
 * sli vs assertions
 * 
 * 23    12/03/97 11:34a Peter
 * dos debugging
 * 
 * 22    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 21    11/15/97 8:55p Peter
 * Removed OutputDebugString
 * 
 * 20    11/15/97 7:43p Peter
 * more comdex silliness
 * 
 * 19    11/12/97 2:27p Peter
 * 
 * 18    11/12/97 11:16a Peter
 * cleaned up assertions
 * 
 * 17    11/04/97 5:04p Peter
 * cataclysm part deux
 * 
 * 16    11/03/97 4:02p Peter
 * cataclysm fix
 * 
 * 15    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 14    10/23/97 5:28p Peter
 * sli fifo thing
 * 
 * 13    9/24/97 1:29p Peter
 * more assertion spewage
 * 
 * 12    9/05/97 5:29p Peter
 * changes for direct hw
 * 
 * 11    5/30/97 5:44p Peter
 * Version that does basic triangles/registers w/ command fifo. Does not
 * currently download textures correctly.
 * 
 * 10    5/28/97 9:05a Peter
 * Merge w/ latest glide changes
 * 
 * 9     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 8     5/21/97 6:05a Peter
 * 
 * 7     5/20/97 9:47a Pgj
 * Use OutputDebugString for non-fatal errors under windows
 * 
 * 6     5/19/97 7:35p Pgj
 * Print cogent error message if h/w not found
 * 
 * 5     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 4     12/23/96 1:37p Dow
 * chagnes for multiplatform glide
**
*/
#include <stdio.h>
#ifdef __DOS__
#  include <malloc.h>
#endif

#ifdef __WIN32__
#  include <windows.h>
#endif

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

void (*GrErrorCallback)( const char *string, FxBool fatal );

void _doGrErrorCallback( const char *name, const char *msg, FxBool fatal )
{
  char buf[1024];

#if GDBG_INFO_ON
  /* Neuter any fifo checking from the failing
   * call. Otherwise entries into the shutdown
   * calls cause spurious crap.
   */
  if (fatal) {
    GR_DCL_GC;

    gc->checkCounter     =
    gc->expected_counter = 0;

    gc->checkCounter =
    gc->checkPtr     = 0UL;
  }
#endif /* GDBG_INFO_ON */

  gdbg_printf("%s: %s.\n", name, msg);
  sprintf(buf,"%s: %s.\n", name, msg);
  GrErrorCallback(buf, fatal);

  if (fatal) exit(1);
}

GR_DIENTRY(grErrorSetCallback, void,
           ( void (*function) ( const char *string, FxBool fatal ) ))
{
  GDBG_INFO(80,"grErrorSetCallback(0x%x)",function);
  GrErrorCallback = function;
}

void
_grErrorDefaultCallback( const char *s, FxBool fatal )
{
  if ( fatal ) {
    GDBG_ERROR("glide", s);
    grGlideShutdown();

#ifdef __WIN32__
    MessageBox(NULL, s, NULL, MB_OK);
    exit(1);
#elif (GLIDE_PLATFORM & GLIDE_OS_MACOS)
    {
      Str255 errBuf;

      errBuf[0] = sprintf((char*)(errBuf + 1), "%s", s);
      DebugStr(errBuf);
    }
#endif /* (GLIDE_PLATFORM * GLIDE_OS_MACOS) */
  }
}

/*
#if !defined(__linux__) && !defined(__FreeBSD__)
void
i3(void) 
{
  __asm int 3;
} 
#endif
*/

void
_grAssert(char *exp, char *fileName, int lineNo)
{
  static int depth;

  return;
  if (depth)
    return;

  depth++;

#if GLIDE_INIT_HAL

#else /* !GLIDE_INIT_HAL */
  /* initRestoreVideo(); */
#endif /* !GLIDE_INIT_HAL */

  gdbg_printf("ASSERTION FAILED:\n");
  gdbg_printf("\tExpression:   %s\n", exp);
  gdbg_printf("\tFile:         %s\n", fileName);
  gdbg_printf("\tLine:         %d\n", lineNo);

#if USE_PACKET_FIFO
  /* Spew about the state of the fifo since that's what most of the
   * assertions are about anyway.  
   */
  {
    GR_DCL_GC;
    GR_DCL_HW;
    
    gdbg_printf("Command Fifo:\n");
    gdbg_printf("\tSoftware:\n");
    gdbg_printf("\t\tfifoPtr:           0x%X\n", (unsigned long)gc->cmdTransportInfo.fifoPtr - (unsigned long) gc->rawLfb);
    gdbg_printf("\t\tfifoOffset:        0x%X\n", gc->cmdTransportInfo.fifoOffset); 
    gdbg_printf("\t\tfifoEnd:           0x%X\n", gc->cmdTransportInfo.fifoEnd - gc->rawLfb);
    gdbg_printf("\t\tfifoSize:          0x%X\n", gc->cmdTransportInfo.fifoSize); 
    gdbg_printf("\t\tfifoRoom:          0x%X\n", gc->cmdTransportInfo.fifoRoom);
    gdbg_printf("\t\troomToReadPtr:     0x%X\n", gc->cmdTransportInfo.roomToReadPtr);
    gdbg_printf("\t\troomToEnd:         0x%X\n", gc->cmdTransportInfo.roomToEnd);

    if ( !gc->windowed ) {
      gdbg_printf("\tHardware:\n");
      gdbg_printf("\t\treadPtrL:          0x%X\n", HW_FIFO_PTR(FXTRUE) - (unsigned long)gc->rawLfb);
      gdbg_printf("\t\tdepth:             0x%X\n", GR_CAGP_GET(depth));
      gdbg_printf("\t\tholeCount:         0x%X\n", GR_CAGP_GET(holeCount));
      gdbg_printf("\t\tbaseAddrL:         0x%X\n", GR_CAGP_GET(baseAddrL));
      gdbg_printf("\t\taMin:              0x%X\n", GR_CAGP_GET(aMin));
      gdbg_printf("\t\taMax:              0x%X\n", GR_CAGP_GET(aMax));
      gdbg_printf("\t\tStatus:            0x%X\n", GR_GET(hw->status));
    }
  }
#endif /* (GLIDE_PLATFORM & GLIDE_HW_CVG) && USE_PACKET_FIFO */

  gdbg_printf("ABNORMAL TERMINATION\n");

  grGlideShutdown();

  depth--;

  exit(-1);

} /* _grAssert */
