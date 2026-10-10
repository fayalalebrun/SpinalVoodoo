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
 ** Revision 1.1.2.2  2005/06/09 18:32:35  jwrdegoede
 ** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
 **
 ** Revision 1.1.2.1  2004/03/02 07:55:30  dborca
 ** Bastardised Glide3x for SST1
 **
 ** Revision 1.1.1.1  1999/12/07 21:42:33  joseph
 ** Initial checkin into SourceForge.
 **
** 
** 1     10/08/98 11:30a Brent
** 
** 14    7/24/98 1:41p Hohn
** 
** 13    6/24/98 1:47p Atai
** code clean up; rename texute line routine
** 
** 12    5/23/98 1:36p Atai
** fix GR_PARAM_Q
** 
** 11    5/12/98 7:26p Atai
** rename fog mode
** 
** 10    2/09/98 10:28a Atai
** set grSpash default to windows coords
** 
** 9     2/06/98 4:25p Atai
** remove GrTmuVertex
** 
** 8     2/05/98 2:30p Atai
** fixed bug#1222.
** 
** 7     1/28/98 6:36p Atai
** remove z,w, min_max definition
** 
** 6     1/28/98 1:13p Peter
** Stupid swap state form gratuitous splash screen
** 
** 5     1/26/98 12:32p Atai
** glide 3 header clean up
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
 * 23    1/09/98 6:48p Atai
 * grTexInfo, GR_LOD_* and GR_ASPECT_*
 * 
 * 21    1/06/98 6:47p Atai
 * undo grSplash and remove gu routines
 * 
 * 20    12/18/97 2:13p Peter
 * fogTable cataclysm
 * 
 * 19    12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 18    12/09/97 10:20a Peter
 * removed extraneous cruft
 * 
 * 17    11/14/97 5:03p Peter
 * picking up the torch... removed #!@#$% c++ comments
 * 
 * 16    10/23/97 5:30p Peter
 * packed rgb happiness
 * 
 * 15    7/31/97 11:14a Peter
 * merge w/ sst1 glide tree, hopefully
 * 
 * 13    7/07/97 3:05p Dow
 * Moved fouled clears
 * 
 * 12    3/18/97 9:07p Dow
 * Got rid of #$#%#$ // comments
 * 
 * 11    3/16/97 12:42a Jdt
 * Removed watcom warning
 * 
 * 10    3/13/97 10:53p Jdt
 * Rewrote with simple optimizations.  Changed prototype for
 * multi-resolution, and running as a better shameless plug.
 * 
 * 9     3/13/97 2:52a Jdt
 * Added arguments to splash.
 * 
 * 8     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 7     3/01/97 6:24p Jdt
 * Made splash force yorigin.
 * 
 * 6     1/02/97 1:06p Dow
 * Fixed state bug
 * 
 * 3     11/17/96 11:16p Garymct
 * Updated grSplash code to set all of the necessary state itself rather
 * than expecting the caller to do so.
 */
#include <math.h>

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxinline.h"

#ifndef GLIDE_SPLASH
GR_DIENTRY(grSplash,void,(float x,float y,float w,float h,FxU32 _frame)) {
/* nothing */
}

#else /* GLIDE_SPLASH */

/*-----------------------------
  Constants
  -----------------------------*/
#define MAX_NUM_VERTS 2556

#define FADEIN_END_PERCENT (0.3f)
#define FADEOUT_BEGIN_PERCENT (0.8f)

#define SPIN_FRAMES 25
#define SPIN_START  26

#define NO_TABLE ((GrTexTable_t)(~0))

/*-----------------------------
  Macros
  -----------------------------*/
#define SNAP_BIAS     ((float)(3<<18))
#define SNAP_COORD(X) ((X)+SNAP_BIAS)

/*-----------------------------
  Types
  -----------------------------*/
typedef struct {
  float  sow;                   /* s texture ordinate (s over w) */
  float  tow;                   /* t texture ordinate (t over w) */  
  float  oow;                   /* 1/w (used mipmapping - really 0xfff/w) */
}  GrTmuVertex;

typedef struct
{
  float x, y, z;                /* X, Y, and Z of scrn space -- Z is ignored */
  float r, g, b;                /* R, G, B, ([0..255.0]) */
  float ooz;                    /* 65535/Z (used for Z-buffering) */
  float a;                      /* Alpha [0..255.0] */
  float oow;                    /* 1/W (used for W-buffering, texturing) */
  GrTmuVertex  tmuvtx[GLIDE_NUM_TMU];
} GrVertex;
#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              1
#define GR_VERTEX_Z_OFFSET              2
#define GR_VERTEX_R_OFFSET              3
#define GR_VERTEX_G_OFFSET              4
#define GR_VERTEX_B_OFFSET              5
#define GR_VERTEX_OOZ_OFFSET            6
#define GR_VERTEX_A_OFFSET              7
#define GR_VERTEX_OOW_OFFSET            8
#define GR_VERTEX_SOW_TMU0_OFFSET       9
#define GR_VERTEX_TOW_TMU0_OFFSET       10
#define GR_VERTEX_OOW_TMU0_OFFSET       11
#define GR_VERTEX_SOW_TMU1_OFFSET       12
#define GR_VERTEX_TOW_TMU1_OFFSET       13
#define GR_VERTEX_OOW_TMU1_OFFSET       14
#if (GLIDE_NUM_TMU > 2)
#define GR_VERTEX_SOW_TMU2_OFFSET       15
#define GR_VERTEX_TOW_TMU2_OFFSET       16
#define GR_VERTEX_OOW_TMU2_OFFSET       17
#endif

typedef struct {
  float x, y, z;                /* object space coordinates */
  float nx, ny, nz;             /* object space vertex normal for lighting */
  float s, t;                   /* pre-glide-ified texture coordinates */
} Vert;

typedef struct {
  int v[3];                     /* vertex indices into array of vertes for face */
  int mat_index;                /* material index */
  int aa_edge_flags;
} Face;

typedef float Vector[3];
typedef float Matrix[16];

typedef FxU32 Palette[256];
typedef struct {
  FxU8  yRGB[16];
  FxI16 iRGB[4][3];
  FxI16 qRGB[4][3];
  FxU32 packed_data[12];
} NCCTable;
typedef union {
  Palette  palette;
  NCCTable nccTable;
} TextureTable;

/*-----------------------------
  Globals
  -----------------------------*/
/* xScale, xOffset, yScale, yOffset */
static float viewPort[4] = { 
  480.0f, 320.0f, 
  480.0f, 240.0f 
};

typedef struct {
  GrTexInfo    info;
  FxU32        addr;
  GrTexTable_t tableType;
  TextureTable tableData;
} Texture;

static int do_phong = 0;
static int pass;
static int fog;
static int useTextures;
static Vector light = { -0.57735f, -0.57735f, -0.57735f };
static Vector transformed_verts[MAX_NUM_VERTS];
static Vector transformed_norms[MAX_NUM_VERTS];

#include "splshdat.c"

static Texture textImage;
static Texture hiliteImage;
static Texture shadowImage;

static FxU32 nextFreeBase;

/*-----------------------------
  Privates
  -----------------------------*/
static void 
sourceTexture(Texture *texture) 
{
  static Texture *lastTexture;

  if (texture != lastTexture && useTextures) {
    grTexSource(GR_TMU0,
                texture->addr,
                GR_MIPMAPLEVELMASK_BOTH,
                &texture->info);
    if (texture->tableType != NO_TABLE) {
      grTexDownloadTable(texture->tableType,
                         &texture->tableData);
    }
    lastTexture = texture;
  }
}

static GrTexTable_t 
texTableType(GrTextureFormat_t format) 
{
  GrTexTable_t rv = (GrTexTable_t)NO_TABLE;

  switch(format) {
  case GR_TEXFMT_YIQ_422:
  case GR_TEXFMT_AYIQ_8422:
    rv = GR_TEXTABLE_NCC0;
    break;

  case GR_TEXFMT_P_8:
  case GR_TEXFMT_AP_88:
    rv = GR_TEXTABLE_PALETTE;
    break;
  }

  return rv;
}

static void 
downloadTexture(Texture *texture, Gu3dfInfo *info) 
{
  texture->info.data        = info->data;
  TEX_INFO(texture->info.,smallLod)    = info->header.small_lod;
  TEX_INFO(texture->info.,largeLod)    = info->header.large_lod;
  TEX_INFO(texture->info.,aspectRatio) = info->header.aspect_ratio;
  texture->info.format      = info->header.format;

  texture->addr = nextFreeBase;
  nextFreeBase += grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,
                                          &texture->info);
  grTexDownloadMipMap(GR_TMU0,
                      texture->addr,
                      GR_MIPMAPLEVELMASK_BOTH,
                      &texture->info);

  texture->tableType = texTableType(info->header.format);
  switch(texture->tableType) {
  case GR_TEXTABLE_NCC0:
  case GR_TEXTABLE_NCC1:
  case GR_TEXTABLE_PALETTE:
    texture->tableData = *(TextureTable*)(&info->table);
    break;
  default:
    break;
  }
}

static void 
createTextures(void) 
{
  Gu3dfInfo *info;

  /* Download the texture for the "3D" part of the model. */
  info       = (Gu3dfInfo*)&text_3dfinfo;
  info->data = (void *)text_3dfinfo_image;

  downloadTexture(&textImage, info);

  /* Download the texture for the specular highlight. */
  info       = (Gu3dfInfo *)&hilite_3dfinfo;
  info->data = (void *)hilite_3dfinfo_image;
  downloadTexture(&hiliteImage, info);

  /* Download the texture for the shadow. */
  info       = (Gu3dfInfo *)&shadow_3dfinfo;
  info->data = (void *)shadow_3dfinfo_image;
  downloadTexture(&shadowImage, info);
}

static void 
vecMatMult(float *dstVec, float *srcVec, float *matrix) 
{
  dstVec[0] = (srcVec[0] * matrix[0] +
               srcVec[1] * matrix[4] +
               srcVec[2] * matrix[8] +
               matrix[12]);
    
  dstVec[1] = (srcVec[0] * matrix[1] +
               srcVec[1] * matrix[5] +
               srcVec[2] * matrix[9] +
               matrix[13]);
    
  dstVec[2] = (srcVec[0] * matrix[2] +
               srcVec[1] * matrix[6] +
               srcVec[2] * matrix[10] +
               matrix[14]);
}

static void 
normMatMult(float *dstVec, float *srcVec, float *matrix) 
{
  dstVec[0] = (srcVec[0] * matrix[0] +
               srcVec[1] * matrix[4] +
               srcVec[2] * matrix[8]);
    
  dstVec[1] = (srcVec[0] * matrix[1] +
               srcVec[1] * matrix[5] +
               srcVec[2] * matrix[9]);
    
  dstVec[2] = (srcVec[0] * matrix[2] +
               srcVec[1] * matrix[6] +
               srcVec[2] * matrix[10]);
}

static void 
xfAndProj(int frame, int obj) 
{
  int    vertex;
  float *matrix;

  matrix = &mat[frame][obj][0];

  for(vertex = 0; vertex < num_verts[obj]; vertex++) {
    float *srcVec;
    float *dstVec;
    float oow;

    /* transform point */
    srcVec = (float*)&(vert[obj][vertex].x);
    dstVec = (float*)transformed_verts[vertex];
    vecMatMult(dstVec, srcVec, matrix);

    /* project point */
    oow = 1.0f / dstVec[2];
    dstVec[0] = dstVec[0] * oow * viewPort[0] + viewPort[1] + SNAP_BIAS;
    dstVec[1] = dstVec[1] * oow * viewPort[2] + viewPort[3] + SNAP_BIAS;

    /* transform normal */
    srcVec = (float*)&(vert[obj][vertex].nx);
    dstVec = (float*)transformed_norms[vertex];
    normMatMult(dstVec, srcVec, matrix);
  }
}

static void 
setupMaterial(int material_index) 
{
  switch(material_index) {
  case 0:                       /* 3d */
    if(pass == 1) {
      sourceTexture(&textImage);
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_LOCAL,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
      do_phong = 1;
    } else if (pass == 0xbeef) {
      /* Pantone 320C - 3D Green */
      grConstantColorValue(0x00989100);
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_LOCAL,
                     GR_COMBINE_LOCAL_CONSTANT,
                     GR_COMBINE_OTHER_ITERATED,
                     FXFALSE);
    } else {
      sourceTexture(&hiliteImage);
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_NONE,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
    } 
    break;
  case 1:                       /* fx */
    if (pass == 0xbeef) {
      /* Black - fx */
      grConstantColorValue(0x00);
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_LOCAL,
                     GR_COMBINE_LOCAL_CONSTANT,
                     GR_COMBINE_OTHER_ITERATED,
                     FXFALSE);
      do_phong = 1;
    } else {
      sourceTexture(&hiliteImage);
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_NONE,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
      do_phong = 1;
    }
    break;
  case 2:
  case 3:
  case 4:
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                   GR_COMBINE_FACTOR_NONE,
                   GR_COMBINE_LOCAL_ITERATED,
                   GR_COMBINE_OTHER_NONE,
                   FXFALSE);
    do_phong = 0;
    break;
  }
}

static void 
calculateIntensity(int material_index, 
                   Vector intensity_factor, 
                   int frame) 
{
  switch(material_index) {
  case 0:
    /* 3d */
    intensity_factor[0] = 1.0f;
    intensity_factor[1] = 1.0f;
    intensity_factor[2] = 1.0f;
    break;
  case 1:
    /* fx */
    intensity_factor[0] = 0.125f;
    intensity_factor[1] = 0.125f;
    intensity_factor[2] = 0.125f;
    break;
  case 2:
    /* cyan */
    intensity_factor[0] = (10.0f / 255.0f);
    intensity_factor[1] = (75.0f / 255.0f);
    intensity_factor[2] = (120.0f / 255.0f);
    break;
  case 3:
    /* white */
    intensity_factor[0] = 1.0f;
    intensity_factor[1] = 1.0f;
    intensity_factor[2] = 1.0f;
    break;
  case 4:
    /* yellow */
    intensity_factor[0] = (248.0f / 255.0f);
    intensity_factor[1] = (204.0f / 255.0f);
    intensity_factor[2] = 0.0f;
    break;
  }
}

static void 
drawFaces(int frame, int objnum) 
{
  int facenum;
  int material_index;
  float intensity_factor[3] = { 0.0f, 0.0f, 0.0f };
  GrVertex gvert[3];
  static int prev_mat_index = 0xffff;
  int i;
  FxBool aa_a, aa_b, aa_c;
    
  for(facenum = 0; facenum < num_faces[objnum]; facenum++)  {
    material_index = face[objnum][facenum].mat_index;
    if(material_index != prev_mat_index)  {
      setupMaterial(material_index);
      calculateIntensity(material_index,
                         intensity_factor, frame);
      prev_mat_index = material_index;
    }
        
    if((material_index != 0) && (pass == 2))
      continue;
          
    aa_a = aa_b = aa_c = FXFALSE;
    if(face[objnum][facenum].aa_edge_flags & 4)
      aa_a = FXTRUE;
    if(face[objnum][facenum].aa_edge_flags & 2)
      aa_b = FXTRUE;
    if(face[objnum][facenum].aa_edge_flags & 1)
      aa_c = FXTRUE;
        
    for(i = 0; i < 3; i++) {
      float *transformed_vert, *transformed_norm;
      Vert *v;
      int vertnum;
      float factor;
            
      vertnum = face[objnum][facenum].v[i];
      transformed_vert = transformed_verts[vertnum];
      transformed_norm = transformed_norms[vertnum];
      v = &vert[objnum][vertnum];
            
      gvert[i].x = transformed_vert[0];
      gvert[i].y = transformed_vert[1];
      gvert[i].oow = 1.0f / transformed_vert[2];
      gvert[i].tmuvtx[0].oow = gvert[i].oow;
      gvert[i].tmuvtx[0].sow = v->s * gvert[i].oow;
      gvert[i].tmuvtx[0].tow = v->t * gvert[i].oow;
            
      factor = ((light[0] * transformed_norm[0] + 
                 light[1] * transformed_norm[1] + 
                 light[2] * transformed_norm[2])+ 1.0f) * 127.5f;

      gvert[i].r = factor * intensity_factor[0];
      gvert[i].g = factor * intensity_factor[1];
      gvert[i].b = factor * intensity_factor[2];
      gvert[i].a = 255.0f;
    }
        
    if(pass == 2) {
      for(i = 0; i < 3; i++) {
        float *transformed_norm;
                
        transformed_norm = 
          transformed_norms[face[objnum][facenum].v[i]];
                
        gvert[i].tmuvtx[0].sow = gvert[i].oow * 
          (128.0f + transformed_norm[0] * 128.0f);
        gvert[i].tmuvtx[0].tow = gvert[i].oow * 
          (128.0f + transformed_norm[1] * 128.0f);
                
        gvert[i].r = intensity_factor[0] * 255.0f;
        gvert[i].g = intensity_factor[1] * 255.0f;
        gvert[i].b = intensity_factor[2] * 255.0f;
      }                 
            
      grDrawTriangle(&gvert[0], &gvert[1], &gvert[2]);
            
      continue;
    }
        
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, 
                         GR_BLEND_ONE, GR_BLEND_ZERO);
    /* Do this if 3D. */
    if(material_index == 0) {
      /*
       * Draw the textured 3D without specular. 
       */
      sourceTexture(&textImage);
#if 0
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_LOCAL,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
#endif
      grAADrawTriangle(&gvert[0], &gvert[1], &gvert[2], 
                       aa_a, aa_b, aa_c);
    } else if(material_index != 1) {
      /*
       * Just go ahead and draw for things that don't
       * have specular highlights.
       */
      grAADrawTriangle(&gvert[0], &gvert[1], &gvert[2],
                       aa_a, aa_b, aa_c);
    }
        
    if(do_phong && (material_index != 0)) {
      /* 
       * figure out texture coords in [0..255.0f] based on the normal
       * the specular highlight.
       */
      for(i = 0; i < 3; i++) {
        float *transformed_norm;
                
        transformed_norm = 
          transformed_norms[face[objnum][facenum].v[i]];
                
        gvert[i].tmuvtx[0].sow = gvert[i].oow * 
          (128.0f + transformed_norm[0] * 128.0f);
        gvert[i].tmuvtx[0].tow = gvert[i].oow * 
          (128.0f + transformed_norm[1] * 128.0f);
                
        gvert[i].r = intensity_factor[0] * 255.0f;
        gvert[i].g = intensity_factor[1] * 255.0f;
        gvert[i].b = intensity_factor[2] * 255.0f;
      }                 
            
      grAADrawTriangle(&gvert[0], &gvert[1], &gvert[2], 
                       aa_a, aa_b, aa_c);
    }
  }
}

static void 
intersectLineWithZPlane(Vector result, Vector p1, 
                        Vector p2, float z) 
{
  float t;
  
  t = (z - p1[2]) / (p2[2] - p1[2]);
  result[0] = p1[0] + (p2[0] - p1[0]) * t;
  result[1] = p1[1] + (p2[1] - p1[1]) * t;
  result[2] = z;
}

static float 
vectorMag(float *v) 
{
  return (float)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static void 
drawShadow(int frame, int shadow_object_index, 
           int receiver_object_index, Vector light_position) 
{
  float *shadow_object_matrix;
  float *receiver_object_matrix;
  Vector view_verts[4];
  Vector projected_view_verts[4];
  int i;
  
  Vector local_verts[4] = {
    { -280.0f, 0.0f, -160.0f },
    { -280.0f, 0.0f,  150.0f },
    {  280.0f, 0.0f,  150.0f },
    {  280.0f, 0.0f, -160.0f } 
  };
  
  float texcoords[4][2] = {
    {  10.5f, 127.5f },
    {  10.5f,   0.5f },
    { 255.0f,   0.5f },
    { 255.0f, 127.5f }
  };
  
  GrVertex gvert[4];
  GrVertex projected_gvert[4];
  GrVertex light_gvert;
  
  /*
   * The point relative to the back part of the shield that 
   * the shadow is going to be projected onto.
   */
  Vector shadow_light;
  
  shadow_light[0] = light_position[0];
  shadow_light[1] = light_position[1];
  shadow_light[2] = light_position[2];
  
  shadow_object_matrix   = &mat[frame][shadow_object_index][0];
  receiver_object_matrix = &mat[frame][receiver_object_index][0];
  
  /*
   * Offset the light by the Z position of 
   * the backplane that we are projected
   * onto.
   */
  shadow_light[2] += receiver_object_matrix[14];
  
  for(i = 0; i < 4; i++) {
    vecMatMult(view_verts[i], local_verts[i], shadow_object_matrix);
        
    /*
     * project . . . 
     */
    gvert[i].oow = 1.0f / view_verts[i][2];
    gvert[i].x = 
      view_verts[i][0] * gvert[i].oow * viewPort[0] + 
        viewPort[1] + SNAP_BIAS;
    gvert[i].y = 
      view_verts[i][1] * gvert[i].oow * viewPort[2] + 
        viewPort[3] + SNAP_BIAS;
        
    /*
     * Set up texture coordinates.
     */
    gvert[i].tmuvtx[0].sow = texcoords[i][0] * gvert[i].oow;
    gvert[i].tmuvtx[0].tow = texcoords[i][1] * gvert[i].oow;
  }
  
  
  /*
   * Intersect each line formed by the light source and a
   * particular corner of the shadow object with the 
   * plane which the texture is to be projected onto.
   */
  for(i = 0; i < 4; i++) {
    Vector tmpvect;
    float q;

#if GLIDE_PACKED_RGB
    /* dpc - 22 oct 1997 - FixMe!
     * These are left uninitialized, yet they are being sent to the
     * hw, probably because some other state is not being correctly
     * setup. This doesn't seem to affect the 'correctness' of the
     * splash screen so I'm not investigating it any more.  
     */
    projected_gvert[i].r = 
    projected_gvert[i].g = 
    projected_gvert[i].b = 
    projected_gvert[i].a = 0.0f;
#endif /* GLIDE_PACKED_RGB */
        
    intersectLineWithZPlane(projected_view_verts[i], 
                            shadow_light, 
                            view_verts[i], 
                            receiver_object_matrix[14] - 26.0f);
    projected_gvert[i].oow = 1.0f / projected_view_verts[i][2];
    projected_gvert[i].x = (projected_view_verts[i][0] * projected_gvert[i].oow * viewPort[0] +
                            viewPort[1] + SNAP_BIAS);
    projected_gvert[i].y = (projected_view_verts[i][1] * projected_gvert[i].oow * viewPort[2] +
                            viewPort[3] + SNAP_BIAS);
        
    tmpvect[0] = projected_view_verts[i][0] - shadow_light[0];
    tmpvect[1] = projected_view_verts[i][1] - shadow_light[1];
    tmpvect[2] = projected_view_verts[i][2] - shadow_light[2];
    q = vectorMag(tmpvect);
        
    /*
     * Set up texture coordinates.
     */
    projected_gvert[i].tmuvtx[0].oow = projected_gvert[i].oow * q;
    projected_gvert[i].tmuvtx[0].sow = texcoords[i][0] * projected_gvert[i].oow;
    projected_gvert[i].tmuvtx[0].tow = texcoords[i][1] * projected_gvert[i].oow;
  }
  
  light_gvert.oow = 1.0f / shadow_light[2];
  light_gvert.x = shadow_light[0] * light_gvert.oow * viewPort[0] + viewPort[1] + SNAP_BIAS;
  light_gvert.y = shadow_light[1] * light_gvert.oow * viewPort[2] + viewPort[3] + SNAP_BIAS;
  
  /*
   * Draw a segment between the light and the point which hits
   * the surface that the light is being projected onto.
   */
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                 GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE,
                 GR_COMBINE_OTHER_TEXTURE,
                 FXFALSE);
  sourceTexture(&shadowImage);
  
  grAlphaBlendFunction(GR_BLEND_DST_COLOR, GR_BLEND_ZERO,
                        GR_BLEND_ONE, GR_BLEND_ZERO);

  grDrawTriangle(&projected_gvert[0], 
                 &projected_gvert[1], 
                 &projected_gvert[2]);
  grDrawTriangle(&projected_gvert[0], 
                 &projected_gvert[2], 
                 &projected_gvert[3]);
  grDrawTriangle(&projected_gvert[0], 
                 &projected_gvert[2], 
                 &projected_gvert[1]);
  grDrawTriangle(&projected_gvert[0], 
                 &projected_gvert[3], 
                 &projected_gvert[2]);
  
  grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
                       GR_BLEND_ONE, GR_BLEND_ZERO);
  
}


/*-------------------------------------------------------------------
  Function: grSplash
  Date: 3/13
  Implementor(s): GaryMcT
  Library: Glide
  Description:
  Render the opening splash screen animation, or render
  a single frame of the splash screen.
  Arguments:
  x, y - upper left hand coord of window
  w, h - width and height of window to render
  _frame - frame number to render (~0 to render all frames)
  Return:
  none
  -------------------------------------------------------------------*/
GR_DIENTRY(grSplash,void,(float x,float y,float w,float h,FxU32 _frame))
{
  GrState oldState;
  int frame;
  Vector lights[] = {
    { 5.0f, 300.0f, -1500.0f },
    { 5.0f, 150.0f, -1000.0f },
    { -30.0f, 150.0f, -1000.0f },
    { -30.0f, 100.0f, -1000.0f },
    { 30.0f, 70.0f, -1000.0f },
  };
  GrFog_t *fogTable=0;
  FxI32 fogTableSize;
  int fadeInFrames, fadeOutFrames;
  FxI32 wrange[2];

  GR_BEGIN_NOFIFOCHECK("grSplash", 85);
  GDBG_INFO_MORE((gc->myLevel,"(%f,%f,%f,%f,%d)\n",
                  x, y, w, h, _frame));

  /* Check Screen Dimensions and Check Avail of Depth Buffer */
  if ((x > gc->state.screen_width) || 
      ((x+w) > gc->state.screen_width) ||
      (y > gc->state.screen_height) ||
      ((y+h) > gc->state.screen_height)) 
    return;

  if (gc->state.screen_height == 640) {
    if (gc->fbuf_size == 1) {
      return;
    }
  } else if (gc->state.screen_width == 800) {
    if ((gc->fbuf_size == 1) ||
        (gc->fbuf_size == 2)) {
      return;
    }
  }

  if (!(gc->state.fbi_config.fbzMode & SST_YORIGIN)) {
    y = ((gc->state.screen_height - 1.0f) - (h-1.0f)) - y;
  }
        
  viewPort[0] = w *  (480.0f / 640.0f);
  viewPort[1] = x + (w / 2.0f);
  viewPort[2] = h;
  viewPort[3] = y + (h / 2.0f);

  grGlideGetState(&oldState);

  grCoordinateSpace(GR_WINDOW_COORDS);
  /*
  ** default splash screen still use glide 2 vertex layout
  */
  grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_A,   GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Z,   GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q0,  GR_VERTEX_OOW_TMU0_OFFSET << 2, GR_PARAM_DISABLE);
  grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);
  grVertexLayout(GR_PARAM_Q1,  GR_VERTEX_OOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);

  grSstOrigin(GR_ORIGIN_LOWER_LEFT);

  if (_frame == 0) {
    createTextures();
    useTextures = 1;
  } else {
    useTextures = 0;
  }

  grAlphaTestFunction(GR_CMP_ALWAYS);
  grChromakeyMode(GR_CHROMAKEY_DISABLE);
  grConstantColorValue(0xffffffff);
  grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
  grDepthMask(FXTRUE);
  grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
                 GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_LOCAL_ITERATED,
                 GR_COMBINE_OTHER_NONE,
                 FXFALSE);
  grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
                       GR_BLEND_ONE, GR_BLEND_ZERO);
  if (useTextures)
    grTexCombine(GR_TMU0, 
                 GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_NONE, GR_COMBINE_FACTOR_NONE,
                 FXFALSE, FXFALSE);
  else
    grTexCombine(GR_TMU0,
                 GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                 GR_COMBINE_FUNCTION_NONE, GR_COMBINE_FACTOR_NONE,
                 FXTRUE, FXFALSE);
  grTexMipMapMode(GR_TMU0,
                  GR_MIPMAP_NEAREST,
                  FXFALSE);
  grTexFilterMode(GR_TMU0,
                  GR_TEXTUREFILTER_BILINEAR,
                  GR_TEXTUREFILTER_BILINEAR);
  grDepthBufferFunction(GR_CMP_LEQUAL);
  grCullMode(GR_CULL_NEGATIVE);
  /* grClipWindow((int)x, (int)y, (int)(x+w), (int)(y+h)); */
  grFogColorValue(0x0);
  
  fadeInFrames  = (int)(((float)total_num_frames) * FADEIN_END_PERCENT);
  fadeOutFrames = (int)(((float)total_num_frames) * 
                        (1.0f - FADEOUT_BEGIN_PERCENT));

  grGet(GR_FOG_TABLE_ENTRIES, 4, &fogTableSize);
  fogTable=(GrFog_t*)malloc(fogTableSize*sizeof(GrFog_t));
  if (_frame == 0) {          /* Render Whole Animation */
    for(frame = 1; frame < total_num_frames; frame++) {
      int i;
            
      /* Set Fog Value For This Frame */
      if(frame < fadeInFrames)  {
        unsigned char fval = (((unsigned char)0xFF) - 
                              ((unsigned char)(255.0f * (float)(frame + 1) / (float)fadeInFrames)));
        for(i = 0; i < fogTableSize; i++) fogTable[i] = fval;
        grFogMode(GR_FOG_WITH_TABLE_ON_W);
        grFogTable(fogTable);
        fog = 1;
      } else if(frame > total_num_frames-fadeOutFrames) {
        unsigned char fval = (((unsigned char)0xFF) - 
                              (unsigned char)(255.0f* ((float)(total_num_frames - frame))/ 
                                              ((float)fadeOutFrames)));
        for(i = 0; i < fogTableSize; i++) 
          fogTable[i] = fval;
        grFogMode(GR_FOG_WITH_TABLE_ON_W);
        grFogTable(fogTable);
        fog = 1;
      } else  {
        grFogMode(GR_FOG_DISABLE);
        fog = 0;
      }
        
      grGet(GR_WDEPTH_MIN_MAX, 8, wrange);
      grBufferClear(0x00000000, 0, (FxU32)wrange[1]);
        
      pass = 1;

      /*
       * Avoid Z-aliasing between the shield and the 3dfx by
       * not writing Z for the shield.
       */

      grDepthMask(FXFALSE);
      /* cyan part of shield */
      xfAndProj(frame, 2);
      drawFaces(frame, 2);
      
      /* yellow and white part of shield. */
      xfAndProj(frame, 0);
      drawFaces(frame, 0);

      /*
       * Reanable writes to the depth-buffer.
       */
      grDepthMask(FXTRUE);

      /*
       * Draw the shadow projected from the 3Dfx logo onto
       * the rest of the powershield.
       */
      grDepthBufferFunction(GR_CMP_ALWAYS);
      grFogMode(GR_FOG_DISABLE);
      drawShadow(frame, 1, 0, lights[0]);
      if (fog) grFogMode(GR_FOG_WITH_TABLE_ON_W); /* hack around mp fog */
      grDepthBufferFunction(GR_CMP_LEQUAL);

      /* 3Dfx logo */
      xfAndProj(frame, 1);
      drawFaces(frame, 1);
      grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ONE,
                           GR_BLEND_ONE, GR_BLEND_ZERO);

      pass = 2;
      drawFaces(frame, 1);
      pass = 1;
      grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
                           GR_BLEND_ONE, GR_BLEND_ZERO);

      grBufferSwap(2);
    }
  } else {                      /* Render One Frame */
    frame = ((_frame>>1) % SPIN_FRAMES)+SPIN_START;
        
    grColorMask(FXFALSE, FXFALSE);
    grGet(GR_WDEPTH_MIN_MAX, 8, wrange);
    grBufferClear(0x00000000, 0, (FxU32)wrange[1]);
    grColorMask(FXTRUE, FXFALSE);
        
    pass = 0xbeef;
        
    /* 3Dfx logo */
    xfAndProj(frame, 1);
    drawFaces(frame, 1);
  }
  free(fogTable);

  /* Get rid of the remnants on teh screen just in case
   * the app (like PowerRender) doesn't do it itself yet
   * assumes that the screen is clear.
   */
  if (frame == 0) {
    int i;

    for(i = 0; i < gc->state.num_buffers; i++) {
      grGet(GR_WDEPTH_MIN_MAX, 8, wrange);
      grBufferClear(0x00000000, 0, (FxU32)wrange[1]);
      grBufferSwap(0);
    }
  }
    
  /*
   * Clean up after yourself!
   */
  grGlideSetState(&oldState);

  /*
  ** reset vertexlayout after calling grSplash
  */
  grVertexLayout(GR_PARAM_XY,  gc->state.vData.vertexInfo.offset , 
                 gc->state.vData.vertexInfo.mode);
  if (gc->state.vData.colorType == GR_FLOAT) {
    grVertexLayout(GR_PARAM_RGB, gc->state.vData.rgbInfo.offset , 
                   gc->state.vData.rgbInfo.mode);
    grVertexLayout(GR_PARAM_A,   gc->state.vData.aInfo.offset , 
                   gc->state.vData.aInfo.mode);
  }
  else
    grVertexLayout(GR_PARAM_PARGB,   gc->state.vData.pargbInfo.offset , 
                   gc->state.vData.pargbInfo.mode);
  grVertexLayout(GR_PARAM_Z,   gc->state.vData.zInfo.offset , 
                 gc->state.vData.zInfo.mode);
  grVertexLayout(GR_PARAM_Q,   gc->state.vData.qInfo.offset , 
                 gc->state.vData.qInfo.mode);
  grVertexLayout(GR_PARAM_ST0, gc->state.vData.st0Info.offset , 
                 gc->state.vData.st0Info.mode);
  grVertexLayout(GR_PARAM_Q0,  gc->state.vData.q0Info.offset , 
                 gc->state.vData.q0Info.mode);
  grVertexLayout(GR_PARAM_ST1, gc->state.vData.st1Info.offset , 
                 gc->state.vData.st1Info.mode);
  grVertexLayout(GR_PARAM_Q1,  gc->state.vData.q1Info.offset , 
                 gc->state.vData.q1Info.mode);
}
#endif /* GLIDE_SPLASH */
