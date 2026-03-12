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
** Revision 1.1.1.1.6.1  2005/06/09 18:32:29  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1  1999/11/24 21:45:00  joseph
** Initial checkin for SourceForge
**
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
** 
** 5     2/18/99 5:28p Peter
** download/source for everything
** 
** 4     2/10/99 2:01p Peter
** cleanup small mip downloads w/ fewer tetsts
** 
** 3     2/02/99 4:36p Peter
** download through lfb rather than texture port
** 
** 2     12/03/98 2:56p Atai
** fixed 16_1 texture download
** 
** 1     10/09/98 6:48p Peter
** base texture download procs
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

/* Because this is uma the memory controller can either direct the
 * download indirectly via the texture port which is offset from the
 * hw address in texBaseAddr or we can try to address the bits
 * directly via 2d lfb accesses. 
 */
#define PACKET5_MODE   SSTCP_PKT5_LFB

void FX_CSTYLE
_grTexDownload_Default_8_1(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                           const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                           void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_1"
#define kTexelAlignShift 0x02UL
  const FxU8
    *src8  = (const FxU8*)texData;
  const FxI32 
    endTAligned = ((maxT + 1) & ~((0x01UL << kTexelAlignShift) - 1UL));
  FxI32 
    t = minT;
  FxU32
    tex_address = tmuBaseAddr + (t * 1UL);

  for(; t < endTAligned; t += (0x01UL << kTexelAlignShift)) {
    LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address, 0x00UL, 0x00UL);
    LINEAR_WRITE_SET(tex_address, *(const FxU32*)src8);
    LINEAR_WRITE_END();
    
    src8 += 4;
    tex_address += sizeof(FxU32);
  }

  /* Copy any remaining stuff if any. There can be at most 3 scanlines
   * each 1 texel wide, but we don't want to whack any other texture
   * data so we have to make sure that we're setting the w0 write mask 
   * along w/ the shifting for the data.
   */
  if (endTAligned < maxT + 1) {
    FxU32 
      texData = 0x00UL,
      texMask = 0x0FUL,
      texShift = 0UL;
    FxI32
      shiftTexel = 0;
    FxU32
      shiftData = 0,
      shiftMask = 0;
      
    for(; t <= maxT; t++) {
      texData |= (*src8++ << (texShift << 3UL));
      texMask ^= (0x01UL << texShift);
      
      texShift++;
    }
    GR_ASSERT(texShift <= 4);

    /* W/ really small lod levels the download address can be
     * unaligned for packet5. This is bad since the address gets
     * truncated. Do even more magic bit twiddling here to convert the
     * whole word into aligned writes w/ the right byte masks again.  
     */
    {
      const FxU32
        texAddrSlop = (tex_address & 0x03UL);
      
      if (texAddrSlop != 0x00UL) {      
        tex_address &= ~0x03UL;

        shiftTexel = (texShift - texAddrSlop);
        if (shiftTexel > 0) {
          shiftMask = (((texMask >> texAddrSlop) | (0x0FUL << shiftTexel)) & 0x0FUL);
          shiftData = (texData >> (texAddrSlop << 0x03UL));
        }

        texMask = (((texMask << texAddrSlop) | (0x0FUL >> (0x04UL - texAddrSlop))) & 0x0FUL);
        texData = (texData << (texAddrSlop << 0x03UL));
      }
    }
    
    LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address, texMask, 0x00UL);
    LINEAR_WRITE_SET(tex_address, texData);
    LINEAR_WRITE_END();

    if (shiftTexel > 0) {
      LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address + 0x04UL, shiftMask, 0x00UL);
      LINEAR_WRITE_SET(tex_address + 0x04UL, shiftData);
      LINEAR_WRITE_END();
    }
  }
#undef kTexelAlignShift
#undef FN_NAME
}

void FX_CSTYLE
_grTexDownload_Default_8_2(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                           const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                           void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_2"
#define kTexelAlignShift 0x01UL
  const FxU8
    *src8  = (const FxU8*)texData;
  const FxI32 
    endTAligned = ((maxT + 1) & ~((0x01UL << kTexelAlignShift) - 1UL));
  FxI32 
    t = minT;
  FxU32
    tex_address = tmuBaseAddr + (t << 1UL);

  for(; t < endTAligned; t += (0x01UL << kTexelAlignShift)) {
    LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address, 0x00UL, 0x00UL);
    LINEAR_WRITE_SET(tex_address, *(const FxU32*)src8);
    LINEAR_WRITE_END();
    
    src8 += 4;
    tex_address += sizeof(FxU32);
  }

  /* Copy any remaining stuff if any.  There should only be one
   * texture scanline left so we have to set the w0 write mask up so
   * that we don't trash any other data.  
   */
  if (endTAligned < maxT + 1) {
    const FxU32
      texData = ((*(src8 + 1) << 8) |
                 (*(src8 + 0) << 0));

    GR_ASSERT(t == maxT);
  
    LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address, 0x0CUL, 0x00UL);
    LINEAR_WRITE_SET(tex_address, texData);
    LINEAR_WRITE_END();
  }
#undef kTexelAlignShift
#undef FN_NAME
}

void FX_CSTYLE
_grTexDownload_Default_8_4(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                           const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                           void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_4"
  const FxU8
    *src8  = (const FxU8*)texData;
  FxI32 
    t = minT;

  for (; t <= maxT; t++) {
    FxU32 
      tex_address = tmuBaseAddr + (t << 2UL);
    FxI32
      s;

    LINEAR_WRITE_BEGIN(maxS, PACKET5_MODE,
                       (FxU32)tex_address, 0x00UL, 0x00UL);

    for (s = 0; s < maxS; s++ ) {
      const FxU32 t0 = *(const FxU32*)src8;
      
      GDBG_INFO(195, "s = %d, t= %d, address = 0x%x\n", s, t,
                (FxU32) tex_address - (FxU32) gc->tex_ptr + 0x200000);
      
      LINEAR_WRITE_SET_8(tex_address, t0);
      
      tex_address += 4;
      src8 += 4;
    }
    
    LINEAR_WRITE_END();
  }
#undef FN_NAME
}

void FX_CSTYLE
_grTexDownload_Default_8_WideS(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                               const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                               void* texData)
{
#define FN_NAME "_grTexDownload_Default_8_WideS"
  const FxU8
    *src8  = (const FxU8*)texData;
  FxI32 
    t = minT;

  for (; t <= maxT; t++) {
    FxU32 
      tex_address = tmuBaseAddr + (t * (maxS << 2));
    FxI32
      s;
    
    LINEAR_WRITE_BEGIN(maxS, PACKET5_MODE,
                       (FxU32)tex_address, 0x00UL, 0x00UL);
    for (s = 0; s < maxS; s += 2) {
      const FxU32 
        t0 = *(const FxU32*)(src8 + 0),
        t1 = *(const FxU32*)(src8 + 4);
      
      GDBG_INFO(195, "s = %d, t= %d, address = 0x%x\n", s, t,
                (FxU32) tex_address - (FxU32) gc->tex_ptr + 0x200000);
      
      LINEAR_WRITE_SET_8(tex_address + 0, t0);
      LINEAR_WRITE_SET_8(tex_address + 4, t1);
      
      tex_address += 8;
      src8 += 8;
    }
    
    LINEAR_WRITE_END();
  }
#undef FN_NAME
}

extern void FX_CSTYLE
_grTexDownload_Default_16_1(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                            const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                            void* texData)
{
#define FN_NAME "_grTexDownload_Default_16_1"
  const FxU16
    *src16 = (const FxU16*)texData;
  const FxI32 
    endTAligned = ((maxT + 1) & ~0x01UL); 
  FxI32 
    t = minT;
  FxU32
    tex_address = tmuBaseAddr + (t << 1UL);
  
  /* Do dword aligned writes */
  for (; t < endTAligned; t += 2) {
    LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address, 0x00UL, 0x00UL);
    LINEAR_WRITE_SET_16(tex_address, *(const FxU32*)src16);
    LINEAR_WRITE_END();
    
    src16 += 2;
    tex_address += sizeof(FxU32);
  }
  
  /* There can only be one line of slop so we deal w/ this by the bits
   * of the w0 write mask so that other data does not get whacked in
   * the process.
   */
  if (endTAligned < maxT + 1) {
    LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address, 0x0CUL, 0x00UL);
    LINEAR_WRITE_SET(tex_address, (FxU32)(*src16));
    LINEAR_WRITE_END();
  }
#undef FN_NAME
}

extern void FX_CSTYLE
_grTexDownload_Default_16_2(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                            const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                            void* texData)
{
#define FN_NAME "_grTexDownload_Default_16_2"
  const FxU16
    *src16 = (const FxU16*)texData;
  FxI32
    t = minT;
  FxU32 
    tex_address = tmuBaseAddr + (t << 1UL);

  /* Copy things that are going to be dword aligned */
  for (; t <= maxT; t++) {
    LINEAR_WRITE_BEGIN(1, PACKET5_MODE, (FxU32)tex_address, 0x00UL, 0x00UL);
    LINEAR_WRITE_SET(tex_address, *(const FxU32*)src16);
    LINEAR_WRITE_END();
    
    src16 += 2;
    tex_address += sizeof(FxU32);
  }
#undef FN_NAME
}

extern void FX_CSTYLE
_grTexDownload_Default_16_WideS(struct GrGC_s* gc, const FxU32 tmuBaseAddr,
                                const FxI32 maxS, const FxI32 minT, const FxI32 maxT,
                                void* texData)
{
#define FN_NAME "_grTexDownload_Default_16_WideS"
  const FxU16
    *src16 = (const FxU16*)texData;
  FxI32 
    t = minT;

  for (; t <= maxT; t++) {
    FxU32 
      tex_address = tmuBaseAddr + (t * (maxS << 2));
    FxI32 
      s;

    LINEAR_WRITE_BEGIN(maxS, PACKET5_MODE,
                       (FxU32)tex_address, 0x00UL, 0x00UL);
    for (s = 0; s < maxS; s += 2) {
      const FxU32 t0 = *(const FxU32*)(src16 + 0);
      const FxU32 t1 = *(const FxU32*)(src16 + 2);
      
      GDBG_INFO(195, "s = %d, t= %d, address = 0x%x\n", s, t,
                (FxU32) tex_address - (FxU32) gc->tex_ptr + 0x200000);
      
      LINEAR_WRITE_SET_16(tex_address + 0, t0);
      LINEAR_WRITE_SET_16(tex_address + 4, t1);
      
      tex_address += 8;
      src16 += 4;
    }
    
    LINEAR_WRITE_END();
  }
#undef FN_NAME
}
