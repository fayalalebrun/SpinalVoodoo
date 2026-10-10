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

#include <stdlib.h>
#include <stdio.h>

#include <fx64.h>

#ifndef NATIVE_64_SUPPORT

FxI64  i64_one  = { 0, 1 };
FxI64  i64_zero = { 0, 0 };

static FxBool fx64_cf;

FxBool __FX_ADD32( FxU32 a, FxU32 b );
#pragma aux __FX_ADD32 = \
   "mov fx64_cf, 0" \
   "add eax, edx"   \
   "adc fx64_cf, 0" \
   parm [eax] [edx] \
   value [eax];

FxI64 __FX_AND64( FxI64 a, FxI64 b )
{
   FxI64 result;

   result.hi = a.hi & b.hi;
   result.lo = a.lo & b.lo;

   return result;
}

FxI64 __FX_CREATE64( FxI32 a, FxU32 b )
{
   FxI64 result;

   result.hi = a;
   result.lo = b;

   return result;
}

/*
** __FX_MUL64
**
** The following code is NOT a full 64-bit multiply.  It only computes
** the low order 64-bits of the product since this is all our hardware
** needs.
*/
FxI64 __FX_MUL64( FxI64 a, FxI64 b )
{
   FxI64 result = { 0, 0 };

   FXUNUSED( a );
   FXUNUSED( b );

   puts( "__FX_MUL64:  not implemented" );
   exit( 1 );
   return result;
}

FxI64 __FX_OR64( FxI64 a, FxI64 b )
{
   FxI64 result;

   result.hi = a.hi | b.hi;
   result.lo = a.lo | b.lo;

   return result;
}

FxI64 __FX_COMP64( FxI64 a )
{
   FxI64 result;

   result.hi = ~a.hi;
   result.lo = ~a.lo;

   return result;
}

float __FX_64TOFLOAT( FxI64 a )
{
   double hi, lo;

   hi = a.hi * 4294967296.0;	// 2**32
   lo = ( float ) a.lo;
   return (float)(hi + lo);
}

FxI64 __FX_FLOATTO64( float f )
{
    int n;
    FxU32 data;
    FxI64 result;

    data = *(FxU32 *)&f;		// get into integer register
    n = (data>>23) & 0xFF;		// peel off exponent
    n = 150 - n;			// compute shift amount
    data |= 0x800000;			// add in the hidden bit
    result.hi = 0;
    result.lo = (data&0xFFFFFF);
    if (n > 0) {			// need to shift right, < 2**23
	if (n > 31) n = 31;
	result.lo >>= n;
    }
    else {
	n = -n;
	if (n > 63) n = 63;
        result = FX_SHL64( result, n );
    }
    // negate after shift, this rounds down instead of to zero
    if (data & 0x80000000)
      result = FX_NEG64(result);
    return result;
}


int __FX_CMP64( FxI64 a, FxI64 b )
{
   if ( a.hi < b.hi )
      return -1;
   if ( a.hi > b.hi )
      return 1;
   if ( a.lo < b.lo )
      return -1;
   else if ( a.lo > b.lo )
      return 1;
   return 0;
}

#endif

