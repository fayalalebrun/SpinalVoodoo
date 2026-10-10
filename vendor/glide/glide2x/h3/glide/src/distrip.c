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
** 
** 16    12/03/98 11:27p Dow
** Code 'cleanup' heç
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
#define FN_NAME "grParameterData"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  GDBG_INFO_MORE(gc->myLevel, "(0x%x, 0x%x)\n", param, offset);

  gc->state.vData.vertexInfo.param = param;

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

  case GR_PARAM_A:

    gc->state.vData.aInfo.offset = offset;
    gc->state.vData.colorType = GR_FLOAT;
    gc->state.vData.aInfo.mode = mode;

    break;
  case GR_PARAM_RGB:

    gc->state.vData.rgbInfo.offset = offset;
    gc->state.vData.colorType = GR_FLOAT;
    gc->state.vData.rgbInfo.mode = mode;

    break;
  case GR_PARAM_PARGB:

    gc->state.vData.pargbInfo.offset = offset;
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

#if !GLIDE3_VERTEX_LAYOUT
  switch (param) {
  case GR_VERTEX:
    GR_CHECK_F(myName,
               !((components == GR_VERTEX_XYZ) ||
                (components == GR_VERTEX_XYZW)), 
               "Bad Component for Vertex Parameter");
    gc->state.vData.vertexInfo.components = components;

    GR_CHECK_F(myName, !(type == GR_FLOAT), "Bad Type for Vertex Parameter"); 
    gc->state.vData.vertexInfo.type = type;

    gc->state.vData.vertexInfo.offset = offset;

    GDBG_INFO(gc->myLevel, "%s:  Vertex Offset = %d\n", FN_NAME,
              gc->state.vData.vertexInfo.offset);
    
    break;

  case GR_COLOR:
    GR_CHECK_F(myName,
               !((components == GR_COLOR_RGB) || (components == GR_COLOR_RGBA)),
               "Bad Component for Color Parameter");
    gc->state.vData.colorInfo.components = components;

    GR_CHECK_F(myName, !((type == GR_FLOAT) || (type == GR_U8)), 
               "Bad Type for Color Parameter");
    gc->state.vData.colorInfo.type = type;

    gc->state.vData.colorInfo.offset = offset;
    GDBG_INFO(gc->myLevel, "%s:  Color Offset = %d\n", FN_NAME,
              gc->state.vData.colorInfo.offset);
    
    break;
    
  case GR_TEXTURE0:
    GR_CHECK_F(myName,
               !((components == GR_TEX_NONE) || (components == GR_TEX_ST) ||
                (components == GR_TEX_STW)),
               "Bad Component for Texture Parameter");
    gc->state.vData.tex0Info.components = components;

    GR_CHECK_F(myName, !(type == GR_FLOAT), "Bad Type for Texture Parameter");
    gc->state.vData.tex0Info.type == type;

    gc->state.vData.tex0Info.offset = offset;
    GDBG_INFO(gc->myLevel, "%s:  Tex0 Offset = %d\n", FN_NAME,
              gc->state.vData.tex0Info.offset);
    
    break;

  case GR_TEXTURE1:
    GR_CHECK_F(myName,
               !((components == GR_TEX_NONE) || (components == GR_TEX_ST) ||
                (components == GR_TEX_STW)),
               "Bad Component for Texture Parameter");
    gc->state.vData.tex1Info.components = components;

    GR_CHECK_F(myName, !(type == GR_FLOAT), "Bad Type for Texture Parameter");
    gc->state.vData.tex1Info.type == type;

    gc->state.vData.tex1Info.offset = offset;
    GDBG_INFO(gc->myLevel, "%s:  Tex1 Offset = %d\n", FN_NAME,
              gc->state.vData.tex1Info.offset);
    break;
    
  default:
    GR_CHECK_F(myName, 0, "Invalid Parameter");
    break;

  }
#endif /* !GLIDE3_VERTEX_LAYOUT */

  _grRebuildDataList();

  GR_END();
#undef FN_NAME
} /* grParameterData */

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
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawPoints(GR_VTX_PTR_ARRAY, Count, pointers);
    else
      _grDrawPoints(GR_VTX_PTR_ARRAY, Count, pointers);
    break;
  case GR_LINE_STRIP:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINE_STRIP, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINE_STRIP, Count, pointers);
    break;
  case GR_LINES:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR_ARRAY, GR_LINES, Count, pointers);
    break;
  case GR_POLYGON:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawVertexList(kSetupFan, GR_VTX_PTR_ARRAY, Count, pointers);
    else
      _grDrawVertexList(kSetupFan, GR_VTX_PTR_ARRAY, Count, pointers);
    break;
    
  case GR_TRIANGLE_STRIP:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawVertexList(kSetupStrip, GR_VTX_PTR_ARRAY, Count, pointers);
    else
      _grDrawVertexList(kSetupStrip, GR_VTX_PTR_ARRAY, Count, pointers);
    break;

  case GR_TRIANGLE_FAN:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawVertexList(kSetupFan, GR_VTX_PTR_ARRAY, Count, pointers);
    else
      _grDrawVertexList(kSetupFan, GR_VTX_PTR_ARRAY, Count, pointers);
    break;
      
  case GR_TRIANGLES:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        _grAADrawTriangles(GR_VTX_PTR_ARRAY, GR_TRIANGLES, Count, pointers);
      else
        _grAAVpDrawTriangles(GR_VTX_PTR_ARRAY, GR_TRIANGLES, Count, pointers);
    else 
      _grDrawTriangles(GR_VTX_PTR_ARRAY, Count, pointers);
    break;

  }
#undef FN_NAME
} /* grDrawVertexArray */


/*-------------------------------------------------------------------
  Function: grDrawVertexArrayLinear
  Date: 04-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
        mode:   GR_POINTS, GR_LINE_STRIP, GR_POLYGON, GR_TRIANLGE_STRIP,
                GR_TRIANGLE_FAN, GR_TRIANGLES
  
  Return:
        Nothing ever
  -------------------------------------------------------------------*/
GR_DIENTRY(grDrawVertexArrayLinear, void , (FxU32 mode, FxU32 Count, void *pointers, FxU32 stride) )
{
#define FN_NAME "grDrawVertexArrayLinear"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE(gc->myLevel, "(0x%x, 0x%x, 0x%x)\n",
                 mode, Count, pointers);

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
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawPoints(GR_VTX_PTR, Count, pointers);
    else
      _grDrawPoints(GR_VTX_PTR, Count, pointers);
    break;
  case GR_LINE_STRIP:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawLineStrip(GR_VTX_PTR, GR_LINE_STRIP, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR, GR_LINE_STRIP, Count, pointers);
    break;
  case GR_LINES:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawLineStrip(GR_VTX_PTR, GR_LINES, Count, pointers);
    else
      _grDrawLineStrip(GR_VTX_PTR, GR_LINES, Count, pointers);
    break;
  case GR_POLYGON:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawVertexList(kSetupFan, GR_VTX_PTR, Count, pointers);
    else
      _grDrawVertexList(kSetupFan, GR_VTX_PTR, Count, pointers);
    break;
    
  case GR_TRIANGLE_STRIP:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawVertexList(kSetupStrip, GR_VTX_PTR, Count, pointers);
    else
      _grDrawVertexList(kSetupStrip, GR_VTX_PTR, Count, pointers);
    break;

  case GR_TRIANGLE_FAN:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      _grAADrawVertexList(kSetupFan, GR_VTX_PTR, Count, pointers);
    else
      _grDrawVertexList(kSetupFan, GR_VTX_PTR, Count, pointers);
    break;
      
  case GR_TRIANGLES:
    if (gc->state.grEnableArgs.primitive_smooth_mode)
      if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS)
        _grAADrawTriangles(GR_VTX_PTR, GR_TRIANGLES, Count, pointers);
      else
        _grAAVpDrawTriangles(GR_VTX_PTR, GR_TRIANGLES, Count, pointers);
    else 
      _grDrawTriangles(GR_VTX_PTR, Count, pointers);
    break;

  }
#undef FN_NAME
} /* grDrawVertexArrayLinear */

#endif /* GLIDE3 */  
