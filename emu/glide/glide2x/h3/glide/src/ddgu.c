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
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
 * 
 * 11    11/12/97 11:14a Peter
 * ganked old sli code
 * 
 * 10    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 9     7/28/97 2:41p Peter
 * turned sli code back on for cvg, but waiting for hal
 * 
 * 8     6/23/97 4:43p Peter
 * cleaned up #defines etc for a nicer tree
 * 
 * 7     6/06/97 10:47a Peter
 * texture downloading, fixed 640x480 dimension, changed cvg dep to be the
 * same as sst1
 * 
 * 6     5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 5     5/21/97 6:04a Peter
 * 
 * 4     5/02/97 2:06p Pgj
 * screen_width/height now FxU32
 * 
 * 3     3/04/97 9:08p Dow
 * 
 * 2     3/01/97 11:31a Tarolli
 * fixed flow control bug for LFB writes
 * 
 * 1     12/23/96 1:39p Dow
 * Changes for multiplatform
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/*---------------------------------------------------------------------------
**  guFbReadRegion
*/
GR_ENTRY(guFbReadRegion, void, ( const int srcX, const int srcY, const int w, const int h, const void *dst, const int strideInBytes ) )
{
  int x, y;
  FxU32 *lfbPtr, lfbReadAddr;
  FxU32 *longPtr;
  unsigned char *charPtr;
  unsigned short *shortPtr;
  
  GR_BEGIN_NOFIFOCHECK("guFbReadRegion",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d,%d, 0x%x,%d)\n",srcX,srcY,w,h,dst,strideInBytes);
  GR_CHECK_F(myName,
              (srcX < 0) || ((srcX + w) > (float) gc->state.screen_width),
              "invalid combination of x & w");
  GR_CHECK_F(myName,
              (srcY < 0) || ((srcY + h) > (float) gc->state.screen_height),
              "invalid combination of y & h");
  GR_CHECK_F(myName, strideInBytes&1, "strideInBytes not a multiple of 2" );

  grSstIdle();
  charPtr = (unsigned char *) dst;
  lfbReadAddr = (FxU32) gc->lfb_ptr;
  for(y=srcY; y<(srcY+h); y++) {
    x = srcX;
    shortPtr = (unsigned short *) charPtr;
    lfbPtr = (FxU32 *) (lfbReadAddr + (y << 11) + ((x&0x3fe) << 1));

    /* Left Edge */
    if(x & 1) {
      x++;
      *shortPtr++ = (unsigned short) (GR_GET(*lfbPtr++) >> 16);
    }

    /* Middle */
    longPtr = (FxU32 *) shortPtr;
    for( ; x < (srcX+w-1); x+=2)
      *longPtr++ = GR_GET(*lfbPtr++);

    /* Right Edge */
    if(x < (srcX+w)) {
      shortPtr = (unsigned short *) longPtr;
      *shortPtr = (unsigned short) GR_GET(*lfbPtr);
    }
    charPtr = (unsigned char *) ((FxU32) charPtr + (FxU32) strideInBytes);
  } /* Y loop */

  GR_END();
} /* guFbReadRegion */

/*---------------------------------------------------------------------------
**  guFbWriteRegion
**
**  WARNING:  GMT: SST_LFB_WRITE_SWAP16 changes pixel addressing!!!
**                 so grLfbWriteColorSwizzle() might break things
*/
GR_ENTRY(guFbWriteRegion, void, 
         ( const int dstX, const int dstY, 
           const int w, const int h, 
           const void *src, const int strideInBytes))
{
  FxU32
    lfbMode;                    /* lfbMode SST-1 register */
  int x,y=h;

  GR_BEGIN_NOFIFOCHECK("guFbWriteRegion",99);
  GDBG_INFO_MORE(gc->myLevel,"(%d,%d,%d,%d, 0x%x,%d)\n",dstX,dstY,w,h,src,strideInBytes);
  GR_CHECK_F(myName,
              (dstX < 0) || ((dstX + w) > (float) gc->state.screen_width),
              "invalid combination of x & w");
  GR_CHECK_F(myName,
              (dstY < 0) || ((dstY + h) > (float) gc->state.screen_height),
              "invalid combination of y & h");
  GR_CHECK_F(myName, strideInBytes&1, "strideInBytes not a multiple of 2" );

  /*
  **  Get the format bits out of the lfbMode shadow register to determine
  **  the size of pixels written to the frame buffer
  */
  lfbMode = gc->state.fbi_config.lfbMode;

  /*  We'll be reading an FxU32 at a time, so determine the pixel
  **  increment by dividing the pixelSize by the size of FxU32
  */
  if ((lfbMode & SST_LFB_FORMAT) < GR_LFBWRITEMODE_888) { 
    FxU16 *lfbPtr;                        /* pointer for storing to lfb */
    const FxU16 *src16 = src;

    lfbPtr = (FxU16 *) (((FxU32) gc->lfb_ptr) + (dstY << 11) + (dstX << 1));
    while (y-- > 0) {                     /* for each row         */
      /* 16-bit source pixels
       * There are 2 cases that we have, 
       * 1) src & dst are offset by one short (see code below) 
       * 2) src & dst are aligned together 
       */
      GR_SET_EXPECTED_SIZE((((dstX & 0x01) +    /* Unaligned start dest pixel */
                             (w >> 1) +         /* Aligned dest pixels */
                             (w & 0x01)) << 2), /* Unaligned end src pixel */
                           ((dstX & 0x01) + (w >> 1) + (w & 0x01)));
      {
        /* case 2) left, middle, right */
        x = 0;
        if (dstX & 1) {
          GR_SET16(lfbPtr[0], src16[0]);
          x++;
        }

        for (; x < w - 1; x += 2) GR_SET(*(FxU32*)&(lfbPtr[x]), *(const FxU32*)&(src16[x]));
        if (x < w) GR_SET16(lfbPtr[w - 1], src16[w - 1]);
      }
      GR_CHECK_SIZE();

      /* advance to next src and dst rows */
      src16 += (strideInBytes >> 1);
      lfbPtr += 1024;
    }
    
  } else {                                /* 32-bit source pixels */
    FxU32 *lfbPtr;                        /* pointer for storing to lfb */
    const FxU32 *src32 = src;

    GR_CHECK_F(myName, strideInBytes&2, "strideInBytes not a multiple of 4" );

    lfbPtr = (FxU32 *) (((FxU32) gc->lfb_ptr) + (dstY << 12) + (dstX << 2));
    while (y-- > 0) {                     /* for each row         */
      GR_SET_EXPECTED_SIZE(w << 2, w);
      {
        for (x = 0; x < w; x++) GR_SET(lfbPtr[x], src32[x]);
      }
      GR_CHECK_SIZE();

      /* advance to next src and dst rows */
      src32 += strideInBytes >> 2;
      lfbPtr += 1024;
    }
  }
  GR_END();
} /* guFbWriteRegion */

