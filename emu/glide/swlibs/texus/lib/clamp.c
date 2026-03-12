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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "texusint.h"

static void _txImgClamp( FxU32 *out, int ox, int oy,
                         const FxU32 *in, int ix, int iy )
{
  int x, y;

  if( txVerbose )
    {
      printf( "clamping from %dx%d to %dx%d\n", 
              ix, iy, ox, oy );
    }

  for( y = 0; y < oy; y++ )
    {
      for( x = 0; x < ox; x++ )
        {
          out[y*ox+x] = in[ ( ( y < iy  )? y : ( iy - 1 ) ) * ix + ( ( x < ix ) ? x : ( ix - 1 ) ) ];
        }
    }
}

void txMipClamp( TxMip *dstMip, TxMip *srcMip )
{
  int i, sw, sh, dw, dh;

  if( dstMip->format != srcMip->format )
    {
      txPanic( "Image formats must be the same in txMipClamp." );
    }

  if( dstMip->format != GR_TEXFMT_ARGB_8888 )
    {
      txPanic( "txMipClamp only works on GR_TEXFMT_ARGB_8888 images." );
    }

  if( ( dstMip->width == srcMip->width ) && ( dstMip->height == srcMip->height ) &&
      ( dstMip->data[0] == srcMip->data[0] ) )
    {
      if( txVerbose )
        {
          printf("No Clamping necessary.\n");
        }
      return;
    }

  if ((srcMip->data[0] == NULL) || (dstMip->data[0] == NULL))
    txPanic("txImageClamp: Null buffer\n");
  
  sw = srcMip->width;
  sh = srcMip->height;
  dw = dstMip->width;
  dh = dstMip->height;

  for( i = 0; i < srcMip->depth; i++ ) 
    {
      if( !dstMip->data[i] )
        txPanic("txImageResize: no miplevel present\n");
      _txImgClamp( dstMip->data[i], dw, dh,
                   srcMip->data[i], sw, sh );
      if( txVerbose )
        {
          printf(" %dx%d", sw, sh); fflush(stdout);
        }

      if (sw > 1) sw >>= 1;
      if (sh > 1) sh >>= 1;
      if (dw > 1) dw >>= 1;
      if (dh > 1) dh >>= 1;
    }
  if( txVerbose )
    {
      printf(".\n");
    }
}
