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
** 3     3/17/99 6:14p Dow
** Fixes Phantom Menace, OGL, and others.
** 
** 4     10/08/98 10:14a Dow
** Fixes 512x384 sometimes
** 
** 3     9/02/98 1:34p Peter
** watcom warnings
** 
** 2     6/25/98 7:40p Dow
** Made it compile
** 
*/

#include <string.h>

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#ifdef __DJGPP__
#include <dos.h>
#else
#include <i86.h>
#endif
#include <gdebug.h>

#include <h3cinit.h>

static FxU16 oldVidMode;

typedef struct {
    int xres;
    int yres;
    int mode;
} ResTableEntry;

static ResTableEntry _table[] = {
  {  512, 384, 0x188 },
  {  640, 480, 0x111 },
  {  800, 600, 0x114 },
  { 1024, 768, 0x117 },
  { 0, 0, 0 }
};

static unsigned long _tableSize = sizeof( _table ) / sizeof( ResTableEntry );

FxBool 
setVideoMode( unsigned long dummy, int xres, int yres, int refresh, void *hmon ) 
{
  union REGS r, rOut;
  int i;
  int mode;

  dummy = dummy;

  r.w.ax = 0x4f03;
  int386(0x10, &r, &rOut);
    
  oldVidMode = rOut.w.bx;

  mode = 0;
  for( i = 0; i < _tableSize; i++ ) {
    if ( ( _table[i].xres == xres ) &&
      ( _table[i].yres == yres )  ) {
      mode = _table[i].mode;
    }
  }

  if ( mode == 0 ) {
    GDBG_INFO(80, "Setmode failed --  unimplemented resolution\n" );
    return FXFALSE;
  }

  r.w.ax = 0x4f02;
  r.w.bx = mode;

  GDBG_INFO(80, "Setting mode 0x%x, 0x%x\n", r.w.ax, r.w.bx);

  /* Do VGA Magic */
  int386(0x10, &r, &rOut);

  /* XXXTACO!! - We should check the return value */

  /* [dBorca] ... and so we do! */
  return (rOut.w.ax==0x004f);
} /* setVideoMode */

void 
resetVideo( void ) 
{
  union REGS r;

  memset(&r, 0, sizeof(r));

  r.w.ax = 0x4f02;
  r.w.bx = oldVidMode;
  GDBG_INFO(80, "resetVideo(): Setting mode 0x%x, 0x%x\n", r.w.ax, r.w.bx);
  int386( 0x10, &r, &r );
} /* resetVideo */
