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
 ** Revision 1.2  2003/06/28 19:10:36  guillemj
 ** Fixed preprocessor invalid tokens in newer gcc 3.x versions.
 **
 ** Revision 1.1.1.1  1999/12/07 21:49:28  joseph
 ** Initial checkin into SourceForge.
 **
** 
** 3     3/17/99 6:16p Dow
** Phantom Menace and other fixes.
** 
** 30    2/19/99 5:54p Peter
** new splash screen
** 
** 29    12/03/98 11:27p Dow
** Code 'cleanup' heç
** 
** 28    12/03/98 4:45p Atai
** exit grSplash if aux buffer equals zero
** 
** 27    8/01/98 3:34p Jeske
** made a special grSplashCb() for Banshee diags, only avail with
** -DDIAG_BUILD
** 
** 26    1/30/98 2:07p Peter
** fixed effage
** 
** 25    1/28/98 1:13p Peter
** Stupid swap state form gratuitous splash screen
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

#if defined( DIAG_BUILD ) || defined( __linux__ )

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
#if defined(GLIDE3) && defined(GLIDE3_ALPHA)
      grTexDownloadTable(texture->tableType,
                         &texture->tableData);
#else
      grTexDownloadTable(GR_TMU0,
                         texture->tableType,
                         &texture->tableData);
#endif
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
  TEX_INFO(texture->info,smallLod)    = info->header.small_lod;
  TEX_INFO(texture->info,largeLod)    = info->header.large_lod;
  TEX_INFO(texture->info,aspectRatio) = info->header.aspect_ratio;
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
  float intensity_factor[3];
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

#ifdef __linux__
FX_ENTRY void FX_CALL
grSplashCb(float x, float y, float width, float height, FxU32 frame,
	void (*fn)(int));
#endif


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
   grSplashCb(x,y,w,h,_frame,NULL);
}

GR_DIENTRY(grSplashCb,void,(float x,float y,float w,float h,FxU32 _frame,
                void (*callback)(int frame)))
{
  GR_BEGIN_NOFIFOCHECK("grSplash", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%f,%f,%f,%f,%d)\n",
                 x, y, w, h, _frame);

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
    GrFog_t fogTable[kInternalFogTableEntryCount];
    int fadeInFrames, fadeOutFrames;

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

    if (gc->grAuxBuf == 0) return;

    if (!(gc->state.fbi_config.fbzMode & SST_YORIGIN)) {
      y = ((gc->state.screen_height - 1.0f) - (h-1.0f)) - y;
    }
        
    viewPort[0] = w *  (480.0f / 640.0f);
    viewPort[1] = x + (w / 2.0f);
    viewPort[2] = h;
    viewPort[3] = y + (h / 2.0f);

    grGlideGetState(&oldState);

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

    if (_frame == 0) {          /* Render Whole Animation */
      for(frame = 1; frame < total_num_frames; frame++) {
        int i;
            
        /* Set Fog Value For This Frame */
        if(frame < fadeInFrames)  {
          unsigned char fval = (((unsigned char)0xFF) - 
                                ((unsigned char)(255.0f * (float)(frame + 1) / (float)fadeInFrames)));
          for(i = 0; i < kInternalFogTableEntryCount; i++) fogTable[i] = fval;
          grFogMode(GR_FOG_WITH_TABLE);
          grFogTable(fogTable);
          fog = 1;
        } else if(frame > total_num_frames-fadeOutFrames) {
          unsigned char fval = (((unsigned char)0xFF) - 
                                (unsigned char)(255.0f* ((float)(total_num_frames - frame))/ 
                                                ((float)fadeOutFrames)));
          for(i = 0; i < kInternalFogTableEntryCount; i++) 
            fogTable[i] = fval;
          grFogMode(GR_FOG_WITH_TABLE);
          grFogTable(fogTable);
          fog = 1;
        } else  {
          grFogMode(GR_FOG_DISABLE);
          fog = 0;
        }
        
        grBufferClear(0x00000000, 0, GR_WDEPTHVALUE_FARTHEST);
        
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
        if (fog) grFogMode(GR_FOG_WITH_TABLE); /* hack around mp fog */
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
        if (callback) callback(frame);
      }
    } else {                      /* Render One Frame */
      frame = ((_frame>>1) % SPIN_FRAMES)+SPIN_START;
        
      grColorMask(FXFALSE, FXFALSE);
      grBufferClear(0x00000000, 0, GR_WDEPTHVALUE_FARTHEST);
      grColorMask(FXTRUE, FXFALSE);
        
      pass = 0xbeef;
        
      /* 3Dfx logo */
      xfAndProj(frame, 1);
      drawFaces(frame, 1);
    }

    /* Get rid of the remnants on teh screen just in case
     * the app doesn't do it itself yet
     * assumes that the screen is clear.
     */
    if (_frame == 0) {
      int i;
    
      for(i = 0; i < gc->grColBuf; i++) {
        grBufferClear(0x00000000, 0, GR_WDEPTHVALUE_FARTHEST);
        grBufferSwap(0);
      }
    }
    
    /*
     * Clean up after yourself!
     */
    grGlideSetState(&oldState);
  }

  GR_END();
}
#else /* !DIAG_BUILD */
GR_DIENTRY(grSplash,void,(float x, float y,
                          float w, float h,
                          FxU32 frameNumber))
{
  GR_BEGIN_NOFIFOCHECK("grSplash", 85);
  GDBG_INFO_MORE(gc->myLevel,"(%f,%f,%f,%f,%d)\n",
                 x, y, w, h, frameNumber);

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  if (gc->pluginInfo.splashProc != NULL) (*gc->pluginInfo.splashProc)(x, y,
                                                                      w, h, 
                                                                      frameNumber);
#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */

  GR_END();
}
#endif /* !DIAG_BUILD */


