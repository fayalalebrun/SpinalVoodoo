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
** Revision 1.1.1.1.8.1  2005/08/13 21:06:57  jwrdegoede
** Last needed 64 bit fixes for h5/h3, complete 64 bit support for cvg
**
** Revision 1.1.1.1  1999/12/07 21:42:38  joseph
** Initial checkin into SourceForge.
**
** 
** 1     10/08/98 11:30a Brent
** 
** 2     10/05/98 7:43p Peter
** 3DNow!(tm) happiness everywhere
** 
** 1     10/03/98 3:37p Peter
** Texture download vectors
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

void FX_CSTYLE
_grTexDownload_Default_8_1(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
			   const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                           void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_1"
  FxU32 
    t = minT;
  unsigned long
    tex_address = tmuBaseAddr + TEX_ROW_ADDR_INCR(t);
  const FxU8
    *src8  = (const FxU8*)texData;
  
  for (; t <= maxT; t++) {
    LINEAR_WRITE_BEGIN(1, kLinearWriteTex, 
                       tex_address - (unsigned long)gc->tex_ptr,
                       0x00U, 0x00U);
    LINEAR_WRITE_SET_8(tex_address, (const FxU32)*(const FxU8*)src8);
    LINEAR_WRITE_END();
    
    src8 += 1;
    tex_address += TEX_ROW_ADDR_INCR(1);
  }
#undef FN_NAME
}

void FX_CSTYLE
_grTexDownload_Default_8_2(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
                           const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                           void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_2"
  FxU32 
    t = minT;
  unsigned long
    tex_address = tmuBaseAddr + TEX_ROW_ADDR_INCR(t);
  const FxU8
    *src8  = (const FxU8*)texData;
  
  for (; t <= maxT; t++) {
    LINEAR_WRITE_BEGIN(1, kLinearWriteTex, 
                       tex_address - (unsigned long)gc->tex_ptr,
                       0x00U, 0x00U);
    LINEAR_WRITE_SET_8(tex_address, (const FxU32)*(const FxU16*)src8);
    LINEAR_WRITE_END();
    
    src8 += 2;
    tex_address += TEX_ROW_ADDR_INCR(1);
  }
#undef FN_NAME
}

void FX_CSTYLE
_grTexDownload_Default_8_4(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
			   const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                           void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_4"
  FxU32 
    t = minT;
  unsigned long
    tex_address = tmuBaseAddr + TEX_ROW_ADDR_INCR(t);
  const FxU8
    *src8  = (const FxU8*)texData;
  
  for (; t <= maxT; t++) {
    LINEAR_WRITE_BEGIN(1, kLinearWriteTex, 
                       tex_address - (unsigned long)gc->tex_ptr,
                       0x00U, 0x00U);
    LINEAR_WRITE_SET_8(tex_address, *(const FxU32*)src8);
    LINEAR_WRITE_END();
    
    src8 += 4;
    tex_address += TEX_ROW_ADDR_INCR(1);
  }
#undef FN_NAME
}

void FX_CSTYLE
_grTexDownload_Default_8_WideS(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
			       const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
			       void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_WideS"
  FxU32 
    t = minT;
  unsigned long
    tex_address;
  const FxU8
    *src8  = (const FxU8*)texData;
  
  for (; t <= maxT; t++) {
    tex_address = tmuBaseAddr + TEX_ROW_ADDR_INCR(t);
    
    LINEAR_WRITE_BEGIN(maxS, kLinearWriteTex,
                       tex_address - (unsigned long)gc->tex_ptr,
                       0x00U, 0x00U);
    {
      FxU32 s;
      
      for (s = 0; s < maxS; s += 2) {
        const FxU32 t0 = *(const FxU32*)(src8   );
        const FxU32 t1 = *(const FxU32*)(src8 + sizeof(FxU32));
        
        LINEAR_WRITE_SET_8(tex_address                , t0);
        LINEAR_WRITE_SET_8(tex_address + sizeof(FxU32), t1);
        
        src8 += 8;
        tex_address += (sizeof(FxU32) << 1);
      }
    }
    LINEAR_WRITE_END();
  }
#undef FN_NAME
}

extern void FX_CSTYLE
_grTexDownload_Default_16_1(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
			    const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                            void* texData)
{
#define FN_NAME "_grTexDownload_Default_16_1"
  FxU32 
    t = minT;
  unsigned long
    tex_address = tmuBaseAddr + TEX_ROW_ADDR_INCR(t);
  const FxU16
    *src16 = (const FxU16*)texData;

  for (; t <= maxT; t++) {
    LINEAR_WRITE_BEGIN(1, kLinearWriteTex,
                       tex_address - (unsigned long)gc->tex_ptr,
                       0x00U, 0x00U);
    LINEAR_WRITE_SET_16(tex_address, (const FxU32)*(const FxU16*)src16);
    LINEAR_WRITE_END();
    
    src16 += 1;
    tex_address += TEX_ROW_ADDR_INCR(1);
  }
#undef FN_NAME
}

extern void FX_CSTYLE
_grTexDownload_Default_16_2(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
			    const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
                            void* texData)
{
#define FN_NAME "_grTexDownload_Default_16_2"
  FxU32 
    t = minT;
  unsigned long
    tex_address = tmuBaseAddr + TEX_ROW_ADDR_INCR(t);
  const FxU16
    *src16 = (const FxU16*)texData;

  for (; t <= maxT; t++) {
    LINEAR_WRITE_BEGIN(1, kLinearWriteTex,
                       tex_address - (unsigned long)gc->tex_ptr,
                       0x00U, 0x00U);
    LINEAR_WRITE_SET_16(tex_address, *(const FxU32*)src16);
    LINEAR_WRITE_END();
    
    src16 += 2;
    tex_address += TEX_ROW_ADDR_INCR(1);
  }
#undef FN_NAME
}

extern void FX_CSTYLE
_grTexDownload_Default_16_WideS(struct GrGC_s* gc, const unsigned long tmuBaseAddr,
				const FxU32 maxS, const FxU32 minT, const FxU32 maxT,
				void* texData)
{
#define FN_NAME "_grTexDownload_Default_16_WideS"
  FxU32 
    t = minT;
  unsigned long
    tex_address;
  const FxU16
    *src16 = (const FxU16*)texData;
  
  for (; t <= maxT; t++) {
    tex_address = tmuBaseAddr + TEX_ROW_ADDR_INCR(t);
    
    /* Loop unrolled to process 2 dwords per iteration */            
    LINEAR_WRITE_BEGIN(maxS, kLinearWriteTex,
                       tex_address - (unsigned long)gc->tex_ptr,
                       0x00U, 0x00U);
    {
      FxU32 s;
      
      for (s = 0; s < maxS; s += 2) {
        const FxU32 t0 = *(const FxU32*)(src16   );
        const FxU32 t1 = *(const FxU32*)(src16 + 2);
        
        GDBG_INFO(195, "s = %d, t= %d, address = 0x%x\n",
                  s, t, (FxU32) tex_address - (FxU32) gc->tex_ptr + 0x200000);
        LINEAR_WRITE_SET_16(tex_address    , t0);
        LINEAR_WRITE_SET_16(tex_address + 4, t1);
        
        tex_address += 8;
        src16 += 4;
      }
    }
    LINEAR_WRITE_END();
  }
#undef FN_NAME
}
