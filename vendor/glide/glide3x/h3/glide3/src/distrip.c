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
** Revision 1.1.1.1.6.2  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.1.1.1.6.1  2005/05/07 08:40:18  jwrdegoede
** lvalue cast fixes for gcc4
**
** Revision 1.1.1.1  1999/11/24 21:44:54  joseph
** Initial checkin for SourceForge
**
** 
** 4     4/06/99 3:54p Dow
** Alt tab again.
** 
** 30    2/18/99 3:12p Kcd
** Removed useless cast that Codewarrior barfs on.
** 
** 29    1/20/99 8:03p Peter
** fixed debug output of grVertexLayout
** 
** 28    12/09/98 5:10p Atai
** set MAXLOD = MINLOD = 8 in _grUpdateParamIndex if ST1 is not used
** 
** 27    12/09/98 2:02p Atai
** Added _grTexForceLod back. Set tLOD min = max = 8 for the 2nd TMU by
** default for Avenger to increase single texturing tri fillrate.
** 
** 26    12/07/98 7:13p Atai
** same as previous one for grDrawVertexArray
** 
** 25    12/07/98 4:54p Atai
** use trisetup routine for GR_TRIANGLES to enable software culling
** 
** 24    12/03/98 11:26p Dow
** Code 'cleanup' heheh
** 
** 23    10/12/98 9:51a Peter
** dynamic 3DNow!(tm)
** 
** 22    9/29/98 2:33p Atai
** change color type if parameter is enabled
** 
** 20    9/09/98 11:58a Atai
** fix grVertexLayout dbg msg
** 
** 19    8/30/98 10:54p Jdt
** Call validation code when vertex layout changes.
** 
** 18    8/03/98 6:38a Jdt
** moved stats into GC
** 
** 17    7/16/98 8:15a Jdt
** fxcmd.h
** 
** 16    6/11/98 6:04p Atai
** added aa case for OGL
** 
** 15    6/09/98 11:59a Atai
** 1. update glide header
** 2. fix cull mode
** 3. fix tri stats
** 
** 14    5/29/98 11:45a Atai
** 1.added _EXT for extension #defines. 
** 2. change GR_TEXBASE_* values
** 3. Remove GR_TEXCHROMA_ENABLE_SUBSTITUTE_RGB
** 
** 13    5/15/98 4:02p Atai
** fogCoord and texchroma extension for Banshee
** 
** 11    4/21/98 1:34p Atai
** make 32 bit clean
** 
** 10    4/17/98 10:59a Atai
** added grGlideGetVertexLayout and grGlideSetVertexLayout
** 
** 9     3/21/98 11:31a Atai
** added GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINUE
** 
** 8     2/09/98 6:20p Atai
** remove aa strip and fan for grDrawVertexArrayContiguous
** 
** 7     2/06/98 6:57p Atai
** rename grVertexArrayLinear to grDrawVertexArrayContiguous
** 
** 6     2/05/98 6:19p Atai
** lazy evaluate for grVertexLayout
** 
** 5     2/03/98 3:40p Atai
** remove aa strip/fan and code clean up
** 
** 4     1/28/98 10:12a Atai
** update GrState size
** 
** 3     1/26/98 11:30a Atai
** update to new glide.h
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 14    1/08/98 4:58p Atai
 * tex table broadcast, grVertexLayout enable/disable, stq, and some
 * defines
 * 
 * 13    12/12/97 10:59a Atai
 * clip space and viewport
 * 
 * 12    12/08/97 10:42a Atai
 * added grDrawVertexArrayLinear()
 * 
 * 11    11/21/97 6:05p Atai
 * use one datalist (tsuDataList) in glide3
 * 
 * 10    11/18/97 6:11p Peter
 * fixed glide3 effage
 * 
 * 9     11/18/97 3:24p Atai
 * change grParameterData to grVertexLayout
 * define GR_PARAM_*
 * 
 * 8     11/07/97 11:22a Atai
 * remove GR_*_SMOOTH. use GR_SMOOTH
 * 
 * 7     11/06/97 6:10p Atai
 * update GrState size
 * rename grDrawArray to grDrawVertexArray
 * update _grDrawPoint and _grDrawVertexList
 * 
 * 6     10/21/97 8:36p Atai
 * added gr_lines routines
 * use dword offset
 * 
 * 5     10/17/97 2:11p Atai
 * added grContinueArray. We only support non aa mode for now.
 * 
 * 4     10/14/97 4:34p Atai
 * filled out the calls to different drawarray routines
 * 
 * 3     9/29/97 1:26p Dow
 * Fixed packed color strips/fans
 * 
 * 2     9/26/97 10:24a Dow
 * Fixed state effage in Glide3 parameter data
 * 
 * 1     9/23/97 2:04p Dow
 * DI code for strips
**
*/
#ifdef GLIDE3

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

/*-------------------------------------------------------------------
  Function: grVertexLayout
  Date: 17-Sep-97
  Implementor(s): dow
  Library: Init Code for 
  Description:
    This routine defines the format for vertex arrays.

  Arguments:
    param       -  Type of date-i.e. vertex, color, or texture info:
                   GR_COLOR, GR_VERTEX, GR_TEXTURE0, GR_TEXTURE1
    components  -  Which components are specified
                   Valid Components:


        So, this table summarizes the legal combinations:
        Param           Type            Size    Description 
        =======================================================================================================
        GR_PARAM_XY     FxFloat         8       X and Y coordinates.  Offset must be zero.
        GR_PARAM_Z      FxFloat         4       Z coordinate.
        GR_PARAM_A      FxFloat         4       Alpha value.
        GR_PARAM_RGB    FxFloat         12      RGB triplet.
        GR_PARAM_PARGB  FxU32           4       Packed ARGB.  High-order byte is A, followed by R, G, and B.
        GR_PARAM_STn    FxFloat         8       S and T coordinates for TMU , where n is in the range [0, TBD]
        GR_PARAM_Wn     FxFloat         4       

  Return:
  Nothing ever.
  -------------------------------------------------------------------*/
GR_DIENTRY(grVertexLayout, void , (FxU32 param, FxI32 offset, FxU32 mode) )
{
#define FN_NAME "grVertexLayout"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  GDBG_INFO_MORE(gc->myLevel, 
                 "(0x%x, 0x%x, %s)\n", 
                 param, offset, (mode == GR_PARAM_ENABLE) ? "Enable" : "Disable");

  switch (param) {
  case GR_PARAM_XY:
    GR_CHECK_F(myName,
               (offset != 0), 
               "Offset must be zero.");

    gc->state.vData.vertexInfo.offset = offset;
    gc->state.vData.vertexInfo.mode = mode;
    break;

  case GR_PARAM_Z:

    gc->state.vData.zInfo.offset = offset;
    gc->state.vData.zInfo.mode = mode;

    break;

  case GR_PARAM_W:

    gc->state.vData.wInfo.offset = offset;
    gc->state.vData.wInfo.mode = mode;

    break;

  case GR_PARAM_FOG_EXT:
    /*
    ** Fog coordinate is an extension in Glide3. It is supported in V2 and VB.
    ** If z-buffering, we use w iterator for fog coordinate.
    ** If w-buffering, we move the w iterator to floating point z and use w iterator for fog.
    */
    gc->state.vData.fogInfo.offset = offset;
    gc->state.vData.fogInfo.mode = mode;

    break;

  case GR_PARAM_A:

    gc->state.vData.aInfo.offset = offset;
    if (mode == GR_PARAM_ENABLE)
      gc->state.vData.colorType = GR_FLOAT;
    gc->state.vData.aInfo.mode = mode;

    break;
  case GR_PARAM_RGB:

    gc->state.vData.rgbInfo.offset = offset;
    if (mode == GR_PARAM_ENABLE)
      gc->state.vData.colorType = GR_FLOAT;
    gc->state.vData.rgbInfo.mode = mode;

    break;
  case GR_PARAM_PARGB:

    gc->state.vData.pargbInfo.offset = offset;
    if (mode == GR_PARAM_ENABLE)
      gc->state.vData.colorType = GR_U8;
    gc->state.vData.pargbInfo.mode = mode;

    break;
  case GR_PARAM_ST0:

    gc->state.vData.st0Info.offset = offset;
    gc->state.vData.st0Info.mode = mode;

    break;
  case GR_PARAM_ST1:

    gc->state.vData.st1Info.offset = offset;
    gc->state.vData.st1Info.mode = mode;

    break;
  case GR_PARAM_Q:

    gc->state.vData.qInfo.offset = offset;
    gc->state.vData.qInfo.mode = mode;

    break;
  case GR_PARAM_Q0:

    gc->state.vData.q0Info.offset = offset;
    gc->state.vData.q0Info.mode = mode;

    break;
  case GR_PARAM_Q1:

    gc->state.vData.q1Info.offset = offset;
    gc->state.vData.q1Info.mode = mode;

    break;
  }

  INVALIDATE( vtxlayout );

  GR_END();
#undef FN_NAME
} /* grParameterData */

/*-------------------------------------------------------------------
  Function: grGlideGetVertexLayout
  Date: 16-Apr-98
  Implementor(s): atai
  Library:  
  Description:
    This routine gets the vertex layout

  Arguments:
    layout  -  vertexlayout
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grGlideGetVertexLayout, void , (void *layout) )
{
#define FN_NAME "grGlideGetVertexLayout"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",layout);
  GR_ASSERT(layout != NULL);

  *((GrVertexLayout *)layout) = gc->state.vData;

  GR_END();
#undef FN_NAME
} /* grGlideGetVertexLayout */

/*-------------------------------------------------------------------
  Function: grGlideSetVertexLayout
  Date: 16-Apr-98
  Implementor(s): atai
  Library:  
  Description:
    This routine sets the vertex layout

  Arguments:
    layout  -  vertexlayout
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grGlideSetVertexLayout, void , (const void *layout) )
{
#define FN_NAME "grGlideSetVertexLayout"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);
  GDBG_INFO_MORE(gc->myLevel,"(0x%x)\n",layout);
  GR_ASSERT(layout != NULL);

  gc->state.vData = *((GrVertexLayout *)layout);

  INVALIDATE( vtxlayout );

  GR_END();
#undef FN_NAME
} /* grGlideSetVertexLayout */

/*-------------------------------------------------------------------
  Function: grDrawVertexArray
  Date: 18-Sep-97
  Implementor(s): dow
  Description:
  
  Arguments:
        mode:   GR_POINTS, GR_LINE_STRIP, GR_POLYGON, GR_TRIANLGE_STRIP,
                GR_TRIANGLE_FAN, GR_TRIANGLES
  
  Return:
        Nothing ever
  -------------------------------------------------------------------*/
GR_DIENTRY(grDrawVertexArray, void , (FxU32 mode, FxU32 Count, void *pointers) )
{
#define FN_NAME "grDrawVertexArray"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE(gc->myLevel, "(0x%x, 0x%x, 0x%x)\n",
                 mode, Count, pointers);

  GR_FLUSH_STATE();

#ifdef GDBG_INFO_ON
  {
    FxU32 i;
    for (i = 0; i < Count; i++)
      GDBG_INFO(110, "%s:  pointers[%d] = 0x%x\n",
                FN_NAME, i, ((float **)pointers)[i]);
  }
#endif

  switch (mode) {
  case GR_POINTS:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_POINTS_MASK)
      _grAADrawPoints(GR_VTX_PTR_ARRAY, Count, pointers);
    else
      _grDrawPoints(GR_VTX_PTR_ARRAY, Count, pointers);
    break;
  case GR_LINE_STRIP:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK)
      _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINE_STRIP, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINE_STRIP, Count, pointers);
    break;
  case GR_LINES:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK)
      _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, Count, pointers);
    break;
    
    /*
    ** anti-alias does not apply to strip and fan
    */
  case GR_TRIANGLE_STRIP:
    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_BDDDDD, kSetupStrip, GR_VTX_PTR_ARRAY, Count, pointers);
    gc->stats.trisProcessed+=(Count-2);
    break;

  case GR_POLYGON:
  case GR_TRIANGLE_FAN:
    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_BDDDDD, kSetupFan, GR_VTX_PTR_ARRAY, Count, pointers); 
    gc->stats.trisProcessed+=(Count-2);
    break;

  case GR_TRIANGLE_STRIP_CONTINUE:

    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_DDDDDD, kSetupStrip, GR_VTX_PTR_ARRAY, Count, pointers);
    gc->stats.trisProcessed+=Count;
    break;

  case GR_TRIANGLE_FAN_CONTINUE:
    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_DDDDDD, kSetupFan, GR_VTX_PTR_ARRAY, Count, pointers);
    gc->stats.trisProcessed+=Count;
    break;
      
  case GR_TRIANGLES:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_TRIANGLES_MASK)
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        _grAADrawTriangles(GR_VTX_PTR_ARRAY, GR_TRIANGLES, Count, pointers);
      else
        _grAAVpDrawTriangles(GR_VTX_PTR_ARRAY, GR_TRIANGLES, Count, pointers);
    else {
      while ((int)Count >= 3) {
        grDrawTriangle(*(float **)pointers, *((float **)pointers+1), *((float **)pointers+2));
        pointers = (float *)pointers + 3;
        Count -= 3;
      }
    }
    break;

  }
#undef FN_NAME
} /* grDrawVertexArray */


/*-------------------------------------------------------------------
  Function: grDrawVertexArrayContiguous
  Date: 04-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
        mode:   GR_POINTS, GR_LINE_STRIP, GR_POLYGON, GR_TRIANLGE_STRIP,
                GR_TRIANGLE_FAN, GR_TRIANGLES
  
  Return:
        Nothing ever
  -------------------------------------------------------------------*/
GR_DIENTRY(grDrawVertexArrayContiguous, void , (FxU32 mode, FxU32 Count, void *pointers, FxU32 stride) )
{
#define FN_NAME "grDrawVertexArrayContiguous"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE(gc->myLevel, "(0x%x, 0x%x, 0x%x)\n",
                 mode, Count, pointers);

  GR_FLUSH_STATE();

#ifdef GDBG_INFO_ON
  {
    FxU32 i;
    for (i = 0; i < Count; i++)
      GDBG_INFO(110, "%s:  pointers[%d] = 0x%x\n",
                FN_NAME, i, (int)pointers + gc->state.vData.vStride * i);
  }
#endif

  gc->state.vData.vStride = stride >> 2;
  switch (mode) {
  case GR_POINTS:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_POINTS_MASK)
      _grAADrawPoints(GR_VTX_PTR, Count, pointers);
    else
      _grDrawPoints(GR_VTX_PTR, Count, pointers);
    break;
  case GR_LINE_STRIP:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK)
      _grAADrawLineStrip(GR_VTX_PTR, GR_LINE_STRIP, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR, GR_LINE_STRIP, Count, pointers);
    break;
  case GR_LINES:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK)
      _grAADrawLineStrip(GR_VTX_PTR, GR_LINES, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR, GR_LINES, Count, pointers);
    break;
    
  case GR_TRIANGLE_STRIP:
    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_BDDDDD, kSetupStrip, GR_VTX_PTR, Count, pointers);
    gc->stats.trisProcessed+=(Count-2);
    break;

  case GR_POLYGON:
  case GR_TRIANGLE_FAN:
    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_BDDDDD, kSetupFan, GR_VTX_PTR, Count, pointers);
    gc->stats.trisProcessed+=(Count-2);
    break;
      
  case GR_TRIANGLE_STRIP_CONTINUE:
    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_DDDDDD, kSetupStrip, GR_VTX_PTR, Count, pointers);
    gc->stats.trisProcessed+=Count;
    break;

  case GR_TRIANGLE_FAN_CONTINUE:
    (*gc->archDispatchProcs.drawVertexList)(SSTCP_PKT3_DDDDDD, kSetupFan, GR_VTX_PTR, Count, pointers);
    gc->stats.trisProcessed+=Count;
    break;
      
  case GR_TRIANGLES:
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_TRIANGLES_MASK)
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        _grAADrawTriangles(GR_VTX_PTR, GR_TRIANGLES, Count, pointers);
      else
        _grAAVpDrawTriangles(GR_VTX_PTR, GR_TRIANGLES, Count, pointers);
    else {
      void *b_ptr, *c_ptr;
      while ((int)Count >= 3) {
        b_ptr = (void *)((unsigned long)pointers + stride);
        c_ptr = (void *)((unsigned long)pointers + stride*2);
        TRISETUP(pointers, b_ptr, c_ptr);
        pointers = (void *)((unsigned long)c_ptr + stride);
        Count -= 3;
      }
    }
    break;

  }
#undef FN_NAME
} /* grDrawVertexArrayContiguous */

#endif /* GLIDE3 */  
