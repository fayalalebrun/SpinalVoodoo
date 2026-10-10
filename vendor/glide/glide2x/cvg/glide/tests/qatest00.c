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
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <glide.h>
#include "tlib.h"
#include "plib.h"
#include "qatest00.h"

/* 1BgFile+8FgFile+3Algo+8DwnlodAlgo=21*/
#define NUMTEXTURES 21
myTexture_t theTextures[NUMTEXTURES];

TexCoordFactors aspctToTxtreCrdFctrs[7] = {
  { 256.0f, 256.0f / 8.0f },  /* GR_ASPECT_8x1 */
  { 256.0f, 256.0f / 4.0f },  /* GR_ASPECT_4x1 */
  { 256.0f, 256.0f / 2.0f },  /* GR_ASPECT_2x1 */
  { 256.0f, 256.0f        },  /* GR_ASPECT_1x1 */
  { 256.0f / 2.0f, 256.0f },  /* GR_ASPECT_1x2 */
  { 256.0f / 4.0f, 256.0f },  /* GR_ASPECT_1x4 */
  { 256.0f / 8.0f, 256.0f }   /* GR_ASPECT_1x8 */
};

GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "QATEST00";
static const char purpose[] = "Texture Download";
static const char usage[]   = "-n <frames> -r <res>";

static float   scrWidth   = 640.0f;
static float   scrHeight  = 480.0f;

/* some global shit for mipmap lod && partial lod downloads */
static FxBool  g_bDoLodDwnld = FXFALSE;
static FxI32   g_nLodToDwnld;
static FxBool  g_bDoLodPrtlDwnld = FXFALSE;
static FxI32   g_nLodPrtlFstRow;
static FxI32   g_nLodPrtlLstRow;

int main( int argc, char **argv) {
  char match; 
  char **remArgs;
  int  rv;

  /* definitions */
  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  int 	frames     = -1;

  /* some texture stuff */
  /* disk based textures	*/
  const char *lpzBgTxtrFile = "miro.3df";
  const int  nNumFgTxtrFiles = 8;
  const char *lpzFgTxtrFiles[] = { "rgb332.3df", "rgb565.3df", 
                                   "argb4444.3df", "argb1555.3df",
                                   "p8.3df", "ap88.3df", 
                                   "yiq.3df", "ayiq.3df" };
  /* color stuff for algorithmic textures */
  const int nAlgFgTxtres = 4;
  FxU16 yAlgFgClrs[][9] = 
    {{ RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565 },
     { RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565 },
     { RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565 },
     { RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565, GRN_565, RED_565 }};

  /* templates for all algorithmic textures */
  GrTexInfo yAlgTxtreTmplts[][9] = 
    {{ {GR_LOD_1, GR_LOD_256, GR_ASPECT_8x1, GR_TEXFMT_RGB_565, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_4x1, GR_TEXFMT_RGB_565, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_2x1, GR_TEXFMT_RGB_565, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_1x1, GR_TEXFMT_RGB_565, NULL} }};

  const int nDwnLodTxtres = 8;
  FxU16  yDwnLodClrs[][9] = 
    {{ BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565 },
     { BLU_332, BLU_332, BLU_332, BLU_332, BLU_332, BLU_332, BLU_332, BLU_332, BLU_332 },
     { BLU_1555, BLU_1555, BLU_1555, BLU_1555, BLU_1555, BLU_1555, BLU_1555, BLU_1555, BLU_1555 },
     { BLU_4444, BLU_4444, BLU_4444, BLU_4444, BLU_4444, BLU_4444, BLU_4444, BLU_4444, BLU_4444 },
     { BLU_8332, BLU_8332, BLU_8332, BLU_8332, BLU_8332, BLU_8332, BLU_8332, BLU_8332, BLU_8332 },
     { BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565 },
     { BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565 },
     { BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565, BLU_565 }};

  GrTexInfo yDwnTxtreTmplts[][9] = 
    {{ {GR_LOD_1, GR_LOD_256, GR_ASPECT_1x1, GR_TEXFMT_RGB_565, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_1x1, GR_TEXFMT_RGB_332, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_1x1, GR_TEXFMT_ARGB_1555, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_1x1, GR_TEXFMT_ARGB_4444, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_1x1, GR_TEXFMT_RGB_565, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_8x1, GR_TEXFMT_RGB_565, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_4x1, GR_TEXFMT_RGB_565, NULL} },
     { {GR_LOD_1, GR_LOD_256, GR_ASPECT_2x1, GR_TEXFMT_RGB_565, NULL} }};

  /* texture handles */
  hTexId_t  hCurrFgTxtre,    /* current fg texture */
            hFstFgTxtre = 0, /* first fg texture */
            hLstFgTxtre,     /* last fg texture */
            hFstAlgFgTxtre,	/* first algo texture */
            hLstAlgFgTxtre = 0,	/* last algo texture */
            hFstDwnlTxtre = 0,  /* first lod download texture */
            hLstDwnlTxtre = 0,  /* last lod download texture */
            hDwlnSrcTxtre,
            hTmpTxtre;
  hTexId_t  hBgTxtre;  /* bg texture */

  GrVertex  bgVerts[4];
  GrVertex  fgVerts[4];

  const float alpha = 192.0f; /* Alpha for ,blending tringle over background */
  FxU32 fxColorValue;
  int i;

  /*---- 
    0-1
    |/|
    3-2
  -----*/
  bgVerts[0].x = 0.f, bgVerts[0].y = 0.f;
  bgVerts[0].a = 255.f, bgVerts[0].oow = 1.f;
  bgVerts[0].tmuvtx[0].sow = 0.f * bgVerts[0].oow;
  bgVerts[0].tmuvtx[0].tow = 0.f * bgVerts[0].oow;

  bgVerts[1].x = scrWidth, bgVerts[1].y = 0.f;
  bgVerts[1].a = 255.f,bgVerts[1].oow = 1.f;
  bgVerts[1].tmuvtx[0].sow = 255.f * bgVerts[1].oow;
  bgVerts[1].tmuvtx[0].tow = 0.f * bgVerts[1].oow;

  bgVerts[2].x = scrWidth, bgVerts[2].y = scrHeight;
  bgVerts[2].a = 255.f, bgVerts[2].oow = 1.f;
  bgVerts[2].tmuvtx[0].sow = 255.f * bgVerts[2].oow;
  bgVerts[2].tmuvtx[0].tow = 255.f * bgVerts[2].oow;
  
  bgVerts[3].x = 0.f, bgVerts[3].y = scrHeight;
  bgVerts[3].a = 255.f, bgVerts[3].oow = 1.f;
  bgVerts[3].tmuvtx[0].sow = 0.f * bgVerts[3].oow;
  bgVerts[3].tmuvtx[0].tow = 255.f * bgVerts[3].oow;

  fgVerts[0].x = 0.f, fgVerts[0].y = 0.f;
  fgVerts[0].a = 255.f, fgVerts[0].oow = 1.f;
  fgVerts[0].tmuvtx[0].sow = 0.f;
  fgVerts[0].tmuvtx[0].tow = 0.f;

  fgVerts[1].x = 0.f, fgVerts[1].y = 0.f;
  fgVerts[1].a = 255.f, fgVerts[1].oow = 1.f;
  fgVerts[1].tmuvtx[0].sow = 0.f;
  fgVerts[1].tmuvtx[0].tow = 0.f;

  fgVerts[2].x = 0.f, fgVerts[2].y = 0.f;
  fgVerts[2].a = 255.f, fgVerts[2].oow = 1.f;
  fgVerts[2].tmuvtx[0].sow = 0.f;
  fgVerts[2].tmuvtx[0].tow = 0.f;

  fgVerts[3].x = 0.f, fgVerts[3].y = 0.f;
  fgVerts[3].a = 255.f, fgVerts[3].oow = 1.f;
  fgVerts[3].tmuvtx[0].sow = 0.f;
  fgVerts[3].tmuvtx[0].tow = 0.f;

  /* Process Command Line Arguments */
  while ((rv = tlGetOpt(argc, argv, "nr", &match, &remArgs)) != 0)
  {
    if ( rv == -1 )
    {
      printf( "Unrecognized command line argument\n" );
      printf( "%s %s\n", name, usage );
      printf( "Available resolutions:\n%s\n", tlGetResolutionList() );
      exit(1);
    }

    switch( match )
    {
      case 'n':
        frames = atoi( remArgs[0] );
      break;

      case 'r':
        resolution = tlGetResolutionConstant( remArgs[0], &scrWidth, &scrHeight );
      break;
    }
  } /* while */

  tlSetScreen( scrWidth, scrHeight );

  grGlideGetVersion( version );

  printf( "%s:\n%s\n", name, purpose );
  printf( "%s\n", version );
  printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );

  if ( frames == -1 )
  {
    printf( "Press A Key To Begin Test.\n" );
    tlGetCH();
  }

  /* Initialize Glide */
  grGlideInit();
  assert( grSstQueryHardware( &hwconfig ) );
  grSstSelect( 0 );
  assert( grSstWinOpen( 0, resolution, GR_REFRESH_60Hz, GR_COLORFORMAT_ABGR,
                        GR_ORIGIN_UPPER_LEFT, 2, 1 ) );

  tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 60, 40, 0xffffff );
  /* done initialize Glide */

  /* Texure stuff */
  /* do the disk based texture stuff */
  for ( i = 0; i < nNumFgTxtrFiles; ++i )
    if ( i==0 )
      hFstFgTxtre = doLoadTexture( lpzFgTxtrFiles[i], NULL, NULL, FXTRUE );
    else
      hLstFgTxtre = doLoadTexture( lpzFgTxtrFiles[i], NULL, NULL, FXTRUE );
    /* do the algorithmic fg texture stuff */
  for ( i = 0; i < nAlgFgTxtres; ++i )
    if ( i==0 )
      hFstAlgFgTxtre = doLoadTexture( NULL, &yAlgTxtreTmplts[i][0], &yAlgFgClrs[i][0], FXTRUE );
    else
      hLstAlgFgTxtre = doLoadTexture( NULL, &yAlgTxtreTmplts[i][0], &yAlgFgClrs[i][0], FXTRUE );

  hLstFgTxtre = hLstAlgFgTxtre;
  /* do the algorithmic download lod texture stuff, these are not downloadable */
  for ( i = 0; i < nDwnLodTxtres; ++i )
    if ( i==0 ) 
      hFstDwnlTxtre = doLoadTexture( NULL, &yDwnTxtreTmplts[i][0], &yDwnLodClrs[i][0], FXFALSE );
    else
      hLstDwnlTxtre = doLoadTexture( NULL, &yDwnTxtreTmplts[i][0], &yDwnLodClrs[i][0], FXFALSE );

  hCurrFgTxtre = hFstFgTxtre; /* init hCurrFgTxtre */
  hBgTxtre = doLoadTexture( lpzBgTxtrFile, NULL, NULL, FXTRUE );
  plTxMnDownloadAll();  /* make plTxMn download everything */
  getFgVerts( hCurrFgTxtre, &fgVerts[0] );
  /* done loading plTxMn */

//  grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR );

  fxColorValue = (0x00FFFFFF | ( ((int) alpha) << 24 ) );
  grConstantColorValue(fxColorValue);

  /* Main processing loop */
  while( frames-- )
  {
    /* definitions */
    static ScaleAmt_t    tCurrScle    = SCALE_1;
    static MipMapMode_t  mipMapMode   = DISABLE;
    static FilterMode_t  filterMode   = BOTH_BILNEAR;
    static float         fBiasLevel   = 0.0f;
    static float         fBiasInc     = 0.25f;
    static FxBool        bVerboseMode = FXTRUE;
    static FxBool        bBgImage     = FXTRUE;

    float       fXsize, fYsize, fULsow, fLRsow, fULtow,  fLRtow;
    GrTexInfo   tmpTexInfo;
    FxBool      isTrilinear, isMultibase;

    /* code */
    grBufferClear( 0x00404040, 0, GR_ZDEPTHVALUE_FARTHEST );

    /* Output the diagnostic stuff */
    if (bVerboseMode)
    {
      tlConClear();
      plTxMnGetInfo( hCurrFgTxtre, &tmpTexInfo );
      isTrilinear = plTxMnGetTrilinear( hCurrFgTxtre );
      isMultibase = plTxMnGetMultibase( hCurrFgTxtre );
      tlConOutput("txt %d, fmt %#x, lgLod %#x, smLod %#x, aspct %#x, T:%d, M:%d\n",
                   hCurrFgTxtre, tmpTexInfo.format, tmpTexInfo.largeLod,
                   tmpTexInfo.smallLod, tmpTexInfo.aspectRatio,
                   isTrilinear, isMultibase );
      fXsize = fgVerts[1].x - fgVerts[0].x;
      fYsize = fgVerts[3].y - fgVerts[0].y;
      fULsow = fgVerts[0].tmuvtx[0].sow;
      fULtow = fgVerts[0].tmuvtx[0].tow;
      fLRsow = fgVerts[2].tmuvtx[0].sow;
      fLRtow = fgVerts[2].tmuvtx[0].tow;
      tlConOutput("Size %f by %f:\nUL s,t %f, %f: LR s,t %f, %f\n", 
                  fXsize, fYsize, fULsow, fULtow, fLRsow, fLRtow );
      switch (tCurrScle)
      {
        case SCALE_1:
          tlConOutput("Curr Scale is 1 LOD; ");
        break;

        case SCALE_2:
          tlConOutput("Curr Scale is 1/2 LOD; ");
        break;

        case SCALE_4:
          tlConOutput("Curr Scale is 1/4 LOD; ");
        break;

        case SCALE_8:
          tlConOutput("Curr Scale is 1/8 LOD; ");
        break;

        case SCALE_16:
          tlConOutput("Curr Scale is 1/16 LOD; ");
        break;

        case SCALE_32:
          tlConOutput("Curr Scale is 1/32 LOD; ");
        break;
      }
      tlConOutput("LOD Bias is %f\n", fBiasLevel);
    } /* done with the Con output stuff */
 
    /* do the Bg texture */
    if (bBgImage)
    {
      /* Setup the system for the Bg Texture */
      grTexLodBiasValue( GR_TMU0, 0.0f );
      grHints(GR_HINT_ALLOW_MIPMAP_DITHER, FXFALSE);
      grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
      grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR );
      grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                      GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, 
                       FXFALSE, FXFALSE );
      grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
      grAlphaCombine( GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_NONE,
                        FXFALSE );
      grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO, GR_BLEND_ZERO );

      /* set bgTexture as source of next rendering operations */
      plTxMnTexSource( hBgTxtre, GR_MIPMAPLEVELMASK_BOTH );

      grDrawTriangle( &bgVerts[0], &bgVerts[1], &bgVerts[3] );
      grDrawTriangle( &bgVerts[1], &bgVerts[2], &bgVerts[3] );
    } /* done with Bg txtre */

    /* Setup the system for the Fg Texture */
    switch( mipMapMode )
    {
      case DISABLE:
        grHints (GR_HINT_ALLOW_MIPMAP_DITHER, FXFALSE );
        grTexMipMapMode( GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE );
        if (bVerboseMode)
          tlConOutput( "MipMapMode: DISABLE  " );
      break;

      case NEAREST:
        grHints (GR_HINT_ALLOW_MIPMAP_DITHER, FXFALSE );
        grTexMipMapMode( GR_TMU0, GR_MIPMAP_NEAREST, FXFALSE );
        if (bVerboseMode)
          tlConOutput( "MipMapMode: NEAREST  " );
      break;

      case TRILINEAR:
        grHints (GR_HINT_ALLOW_MIPMAP_DITHER, FXFALSE );
        grTexMipMapMode( GR_TMU0, GR_MIPMAP_NEAREST, FXTRUE );
        if (bVerboseMode)
          tlConOutput( "MipMapMode: TRILINEAR  " );
      break;

      case DITHRD_NREST:
        grHints (GR_HINT_ALLOW_MIPMAP_DITHER, FXTRUE );
        grTexMipMapMode( GR_TMU0, GR_MIPMAP_NEAREST_DITHER, FXFALSE );
        if (bVerboseMode)
          tlConOutput( "MipMapMode: NEAR_DITHRD  " );
      break;

      case DITHRD_TRILIN:
        grHints (GR_HINT_ALLOW_MIPMAP_DITHER, FXTRUE );
        grTexMipMapMode( GR_TMU0, GR_MIPMAP_NEAREST_DITHER, FXTRUE );
        if (bVerboseMode)
          tlConOutput( "MipMapMode: NEAR_TRILNR  " );
      break;
    }	/* switch( mipMapMode ) */


    switch( filterMode )
    {
      case BOTH_POINT:
        grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED );
        if (bVerboseMode)
          tlConOutput( "FltrMode: Point Samp\n" );
      break;

      case MIN_BILNEAR:
        grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_POINT_SAMPLED );
        if (bVerboseMode)
          tlConOutput( "FltrMode: MinBilnr\n" );
      break;

      case MAG_BILNEAR:
        grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_BILINEAR );
        if (bVerboseMode)
          tlConOutput( "FltrMode: MagBilnr\n" );
      break;

      case BOTH_BILNEAR:
        grTexFilterMode( GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR );
        if (bVerboseMode)
          tlConOutput( "FltrMode: BothBilnr\n" );
      break;
    }	/* switch( filterMode ) */


    /* do the Fg texturing stuff */
    if ((mipMapMode != TRILINEAR) && (mipMapMode != DITHRD_TRILIN))
    {
      /* setup the Fg texture states */
      grTexLodBiasValue( GR_TMU0, fBiasLevel );
      grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                      GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, 
                       FXFALSE, FXFALSE );
      grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
      grAlphaCombine( GR_COMBINE_FUNCTION_BLEND_OTHER, GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_CONSTANT,
                        FXFALSE );
      grAlphaBlendFunction( GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
                            GR_BLEND_ONE, GR_BLEND_ZERO );

      /* set the Fg texture */
      plTxMnTexSource( hCurrFgTxtre, GR_MIPMAPLEVELMASK_BOTH );
      grDrawTriangle( &fgVerts[0], &fgVerts[1], &fgVerts[3] );
      grDrawTriangle( &fgVerts[1], &fgVerts[2], &fgVerts[3] );
    } /* if */
    else
    {
      /* setup system for the Even MipMaps */
      grTexLodBiasValue( GR_TMU0, fBiasLevel );
      grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_BLEND_LOCAL, GR_COMBINE_FACTOR_LOD_FRACTION,
                      GR_COMBINE_FUNCTION_BLEND_LOCAL,GR_COMBINE_FACTOR_LOD_FRACTION,
                      FXFALSE, FXFALSE );
      grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
      grAlphaCombine( GR_COMBINE_FUNCTION_BLEND_OTHER, GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_CONSTANT,
                        FXFALSE );
      grAlphaBlendFunction( GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
                              GR_BLEND_ONE, GR_BLEND_ZERO );

      /* render with the Even MipMaps */
      plTxMnTexSource( hCurrFgTxtre, GR_MIPMAPLEVELMASK_EVEN );
      grDrawTriangle( &fgVerts[0], &fgVerts[1], &fgVerts[3] );
      grDrawTriangle( &fgVerts[1], &fgVerts[2], &fgVerts[3] );

      /* setup system for the Odd MipMaps */
      grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_BLEND_LOCAL, GR_COMBINE_FACTOR_LOD_FRACTION,
                      GR_COMBINE_FUNCTION_BLEND_LOCAL,GR_COMBINE_FACTOR_LOD_FRACTION,
                      FXFALSE, FXFALSE );
      grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
      grAlphaCombine( GR_COMBINE_FUNCTION_BLEND_OTHER, GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_CONSTANT,
                        FXFALSE );
      grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO );

      /* render with the Odd MipMaps */
      plTxMnTexSource( hCurrFgTxtre, GR_MIPMAPLEVELMASK_ODD );
      grDrawTriangle( &fgVerts[0], &fgVerts[1], &fgVerts[3] );
      grDrawTriangle( &fgVerts[1], &fgVerts[2], &fgVerts[3] );
    } /* else */

    /* do the swap */
    tlConRender();
    grBufferSwap( 1 );

    /* dynamic resizing */
    if (hwconfig.SSTs[0].type == GR_SSTTYPE_SST96) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );

      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }

    /* handle any keyboard input */
    while( tlKbHit() )
    {
      switch( tlGetCH() )
      {
        static int           nCurrAlpha = -1;
        GrTexInfo            tFgTexInfo;

        case '+': 
          doScaleScreenSurf( hCurrFgTxtre, SCALE_OUT, tCurrScle );
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case '-':
          doScaleScreenSurf( hCurrFgTxtre, SCALE_IN, tCurrScle );
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 'a':
          tlConClear();
          bVerboseMode = FXFALSE;
          tlConOutput("Verbose mode Off\n");
          nCurrAlpha = (nCurrAlpha == 0) ? 0 : 
                       (nCurrAlpha < 0) ? (int) alpha-1: nCurrAlpha-1;
          fxColorValue = ( 0x00FFFFFF | (nCurrAlpha << 24) );
          grConstantColorValue(fxColorValue);
          tlConOutput("Current fxColorValue is %-#8x\n", fxColorValue);
        break;

        case 'A':
          tlConClear();
          bVerboseMode = FXFALSE;
          tlConOutput("Verbose mode Off\n");
          nCurrAlpha = (nCurrAlpha == 255) ? 255 : 
                       (nCurrAlpha < 0) ? (int) alpha+1: nCurrAlpha+1;
          fxColorValue = ( 0x00FFFFFF | (nCurrAlpha << 24) );
          grConstantColorValue(fxColorValue);
          tlConOutput("Current fxColorValue is %-#8x\n", fxColorValue);
        break;

        case 'b':
          mipMapMode++;
          mipMapMode%=5;
          /* set or unset the trilinear bit on all the fg textures */
          if ((mipMapMode == TRILINEAR) || (mipMapMode == DITHRD_TRILIN))
            for (hTmpTxtre = hFstFgTxtre; hTmpTxtre <= hLstFgTxtre; ++hTmpTxtre )
              plTxMnSetTrilinear( hTmpTxtre, FXTRUE );
          else
            for (hTmpTxtre = hFstFgTxtre; hTmpTxtre <= hLstFgTxtre; ++hTmpTxtre )
              plTxMnSetTrilinear( hTmpTxtre, FXFALSE );

          plTxMnDownloadAll();	/* remember to reset the TMU mem */
        break;

        case 'B':
          filterMode++;
          filterMode%=4;
        break;

        case 'c':
        case 'C':
          tlConClear();
        break;

        case 'd':
          tlConClear();
          bVerboseMode = FXFALSE;
          tlConOutput("Verbose mode Off\n");
          plTxMnSetMultiOffsetFix( plTxMnGetMultiOffsetFix() ? FXFALSE : FXTRUE );
          tlConOutput( plTxMnGetMultiOffsetFix() ? "Multibase offset fix in\n" :
                                                   "Multibase offset fix NOT\n");
          plTxMnDownloadAll(); /* reset the tmu mem */
        break;

        case 'D':
          tlConClear();
          bVerboseMode = FXFALSE;
          tlConOutput("Verbose mode Off\n");
          plTxMnSetMultiRevOrder( plTxMnGetMultiRevOrder() ? FXFALSE : FXTRUE );
          tlConOutput( plTxMnGetMultiRevOrder() ? "Multibase in rev order\n" :
                                                  "Multibase in linear order\n");
          plTxMnDownloadAll(); /* reset the tmu mem */
        break;

        case 'e':
          if (fBiasLevel > -8.00f)
            fBiasLevel -= fBiasInc;

          grTexLodBiasValue( GR_TMU0, fBiasLevel );
        break;

        case 'E':
          if (fBiasLevel < 7.750f)
            fBiasLevel += fBiasInc;

          grTexLodBiasValue( GR_TMU0, fBiasLevel );
        break;

        case 'h':
        case 'H':
          doHelp();
        break;

        case 'l':
        case 'L':
          getLodToDwnld();
          if (g_bDoLodDwnld)
          {
            hDwlnSrcTxtre = H_NULL;
            plTxMnGetInfo( hCurrFgTxtre, &tFgTexInfo );

            for ( hTmpTxtre = hFstDwnlTxtre; hTmpTxtre <= hLstDwnlTxtre; ++hTmpTxtre)
            {
              plTxMnGetInfo( hTmpTxtre, &tmpTexInfo );
              if ((tFgTexInfo.format == tmpTexInfo.format) &&
                  (tFgTexInfo.aspectRatio == tmpTexInfo.aspectRatio))
              { /* if they are compatible */
                hDwlnSrcTxtre = hTmpTxtre;
                hTmpTxtre = hLstDwnlTxtre;  /* get first occurence, short circut */
              }
            }

            if (hDwlnSrcTxtre)
              plTxMnDwnldMipMapLod( hCurrFgTxtre, hDwlnSrcTxtre, g_nLodToDwnld );
            else  /* else still H_NULL */
              tlConOutput("No cmptble txtre format avlable\n");
          }
        break;

        case 'm':
          if ( plTxMnGetMultibase(hCurrFgTxtre) )
            plTxMnSetMultibase( hCurrFgTxtre, FXFALSE );
          else
            plTxMnSetMultibase( hCurrFgTxtre, FXTRUE );

          plTxMnDownloadAll(); /* reset the tmu mem */
        break;

        case 'M':
          bBgImage = bBgImage ? FXFALSE : FXTRUE;
        break;

        case 'o':
          plPopLod( hCurrFgTxtre, btmOfMipMap ); 
          plTxMnDownloadAll(); /* reset the tmu mem */
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 'O':
          plPopLod( hCurrFgTxtre, topOfMipMap ); 
          plTxMnDownloadAll(); /* reset the tmu mem */
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 'p':
        case 'P':
          getLodPrtlToDwnld();
          if (g_bDoLodPrtlDwnld)
          {
            hDwlnSrcTxtre = H_NULL;
            plTxMnGetInfo( hCurrFgTxtre, &tFgTexInfo );

            for ( hTmpTxtre = hFstDwnlTxtre; hTmpTxtre <= hLstDwnlTxtre; ++hTmpTxtre)
            {
              plTxMnGetInfo( hTmpTxtre, &tmpTexInfo );
              if ((tFgTexInfo.format == tmpTexInfo.format) &&
                  (tFgTexInfo.aspectRatio == tmpTexInfo.aspectRatio))
              { /* if they are compatible */
                hDwlnSrcTxtre = hTmpTxtre;
                hTmpTxtre = hLstDwnlTxtre;  /* get first occurence, short circut */
              }
            }

            if (hDwlnSrcTxtre)
              plTxMnDwnldMipMapLodPrtl( hCurrFgTxtre, hDwlnSrcTxtre, g_nLodToDwnld,
                                        g_nLodPrtlFstRow, g_nLodPrtlLstRow );
            else  /* else still H_NULL */
              tlConOutput("No cmptble txtre format avlable\n");
          }
        break;

        case 'r':
        case 'R':
          for (hTmpTxtre = hFstFgTxtre; hTmpTxtre <= hLstFgTxtre; ++hTmpTxtre )
          {
            plTxMnReset( hTmpTxtre );
            doResetFgVerts( hTmpTxtre );
          }  /* reset d'em puppies */

          mipMapMode = DISABLE;
          plTxMnDownloadAll(); /* reset the tmu mem */
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 't':
          if (hCurrFgTxtre == hFstFgTxtre)
            hCurrFgTxtre = hLstFgTxtre;
          else
            --hCurrFgTxtre;
 
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 'T':
          if (hCurrFgTxtre == hLstFgTxtre)
            hCurrFgTxtre = hFstFgTxtre;
          else
            ++hCurrFgTxtre;

          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 'u':
          plPushLod( hCurrFgTxtre, btmOfMipMap ); 
          plTxMnDownloadAll(); /* reset the tmu mem */
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 'U':
          plPushLod( hCurrFgTxtre, topOfMipMap ); 
          plTxMnDownloadAll(); /* reset the tmu mem */
          getFgVerts( hCurrFgTxtre, &fgVerts[0] );
        break;

        case 'v':
        case 'V':
          bVerboseMode = bVerboseMode ? FXFALSE : FXTRUE;
        break;

        case 'w':
          if (tCurrScle != SCALE_1)
            --tCurrScle;
        break;

        case 'W':
          if (tCurrScle != SCALE_32)
            ++tCurrScle;
        break;

        default:
          frames = 0;
        break;
      } /* switch( tlGetCH() ) */
    } /* while( tlKbHit() ) */
  }  /* while (frames--) */

  /* get the funk out */
  doUnloadTextures();
  grGlideShutdown();

  exit(1);
}	/* main */



static void 
doHelp( void )
{
  grBufferClear( 0, 0, 0 );
  tlConClear();
  tlConOutput("Keymap: (case sensitive - second letter lower case)\n");
  tlConOutput("        b (lc):   cycles mipmap modes\n");
  tlConOutput("        B (uc):   cycles filter modes\n");
  tlConOutput("        T xor t:  cycles through fg texture formats\n");
  tlConOutput("        m (lc):   toggles multibase for fg texture\n");
  tlConOutput("        M (uc):   toggles the bg image\n");
  tlConOutput("        d (lc):   toggles the mltbse offset fix\n");
  tlConOutput("        D (uc):   toggles the order of the mltbse segs\n");
  tlConOutput("        L or l:   LOD download menu\n");
  tlConOutput("        P or p:   partial LOD download menu\n");
  tlConOutput("        R or r:   resets all fg textures states\n");
  tlConOutput("        O xor o:  pop lod off TOP or btm of stack\n");
  tlConOutput("        U xor u:  push lod back on TOP or btm of stack\n");
  tlConOutput("        V or v:   toggle verbose mode\n");
  tlConOutput("        + xor -:  change size of fg obj by LOD frac\n");
  tlConOutput("        W xor w:  change LOD frac\n");
  tlConOutput("        E xor e:  INC or dec LOD Bias\n");
  tlConOutput("        A xor a:  INC or dec alpha const\n");
  tlConOutput("        C or c:   clears console\n");
  tlConOutput("        H or h:   Help\n");
  tlConOutput("        Q or q:   Quit\n");
  tlConOutput("Press a key to continue...\n");
  tlConRender();
  grBufferSwap( 1 );
  tlGetCH();

  tlConClear();
}



static hTexId_t
doLoadTexture( const char *lpzFileName, GrTexInfo *tEtTexInfo,
                 FxU16 *yColors, FxBool isDwnldble )
{
  /* definitions */
  static FxU8  lstTexIndx = 0;  /* last available texture index */
  GrTexInfo    *lpTmpTexInfo;  	
  surfInfo_t   *lpSurfInfo;
  myTexture_t  *lpTmpMyTxtre; /* for ease of read */
  double       dNonFrac, ret;

  /* assumptions */
  assert( lpzFileName || tEtTexInfo );
  assert( lstTexIndx < NUMTEXTURES );

  /* initializations */
  lpTmpMyTxtre = &theTextures[lstTexIndx];
  lpTmpMyTxtre->lpzTexFileName = lpzFileName;


  /* code */
  lpTmpMyTxtre->lpTlTexture = (TlTexture*) malloc(sizeof(TlTexture));
  assert( lpTmpMyTxtre->lpTlTexture );
  lpTmpTexInfo = &lpTmpMyTxtre->lpTlTexture->info;

  if (lpzFileName != NULL)
  { /* load from disk */
    assert(tlLoadTexture( lpTmpMyTxtre->lpzTexFileName,
                            &lpTmpMyTxtre->lpTlTexture->info,
                            &lpTmpMyTxtre->lpTlTexture->tableType,
                            &lpTmpMyTxtre->lpTlTexture->tableData));
    lpTmpTexInfo = &lpTmpMyTxtre->lpTlTexture->info;
  }
  else
  { /* else create in RAM from template && lod color array */
    *lpTmpTexInfo = *tEtTexInfo;
    lpTmpTexInfo->data = malloc(grTexCalcMemRequired(lpTmpTexInfo->smallLod, 
                                lpTmpTexInfo->largeLod,
                                lpTmpTexInfo->aspectRatio,
                                lpTmpTexInfo->format));
    assert(lpTmpTexInfo->data);
    plCreateTlTexture(lpTmpMyTxtre->lpTlTexture, yColors);
  }

  /* save off the starting surface screen coords for this texture */
  lpSurfInfo = &lpTmpMyTxtre->surfInfo;
  lpSurfInfo->currLod = lpTmpTexInfo->largeLod;


  lpSurfInfo->fStartX = 
      ((scrWidth - (float) getLodWidth(lpSurfInfo->currLod,
                                       lpTmpTexInfo->aspectRatio) )/2.f);
  ret = modf((double) lpSurfInfo->fStartX, &dNonFrac);
  lpSurfInfo->rStartX = (float) dNonFrac;

  lpSurfInfo->fEndX = 
      (lpSurfInfo->fStartX + (float) getLodWidth(lpSurfInfo->currLod, 
                                                 lpTmpTexInfo->aspectRatio));
  ret = modf((double) lpSurfInfo->fEndX, &dNonFrac);
  lpSurfInfo->rEndX = (float) dNonFrac;

  lpSurfInfo->fStartY = 
      ((scrHeight - (float) getLodHeight(lpSurfInfo->currLod,
                                         lpTmpTexInfo->aspectRatio) )/2.f);
  ret = modf((double) lpSurfInfo->fStartY, &dNonFrac);
  lpSurfInfo->rStartY = (float) dNonFrac;

  lpSurfInfo->fEndY = 
      (lpSurfInfo->fStartY + (float) getLodHeight(lpSurfInfo->currLod, 
                                                  lpTmpTexInfo->aspectRatio));
  ret = modf((double) lpSurfInfo->fEndY, &dNonFrac);
  lpSurfInfo->rEndY = (float) dNonFrac;
  /* done with surface coords */

  /* give it to plTxMn */
  lpTmpMyTxtre->hTexture = plTxMnLoadTxMngr( lpTmpMyTxtre->lpTlTexture );
  plTxMnSetDownload( lpTmpMyTxtre->hTexture, isDwnldble );

  ++lstTexIndx;

  return( lpTmpMyTxtre->hTexture );
}


static void 
getFgVerts( hTexId_t deTexId, GrVertex *deFgVerts )
{
  /* definitions */
  surfInfo_t  *lpSurfInfo;
  int         i;
  GrTexInfo   tTexInfo;
  float       fSmulti, fTmulti;

  /* assumptions */
  assert( deTexId < NUMTEXTURES );
  assert( deFgVerts );

  /* parse the texture array for the right one */
  for ( i = 0; (i < NUMTEXTURES) && (theTextures[i].hTexture != deTexId) ; ++i );
  if (i == NUMTEXTURES)
    return;  /* texture not found */

  /* initializations */
  lpSurfInfo  = &theTextures[i].surfInfo;
  tTexInfo    = theTextures[i].lpTlTexture->info;
  fSmulti     = aspctToTxtreCrdFctrs[tTexInfo.aspectRatio].sMult;
  fTmulti     = aspctToTxtreCrdFctrs[tTexInfo.aspectRatio].tMult;

  /*---- 
    0-1
    |/|
    3-2
  -----*/
  
  deFgVerts[0].x = lpSurfInfo->rStartX+0.5f;
  deFgVerts[0].y = lpSurfInfo->rStartY+0.5f;
  deFgVerts[0].tmuvtx[0].sow = 0.f * (fSmulti) * deFgVerts[0].oow;
  deFgVerts[0].tmuvtx[0].tow = 0.f * (fTmulti) * deFgVerts[0].oow;

  deFgVerts[1].x = lpSurfInfo->rEndX+0.5f;
  deFgVerts[1].y = lpSurfInfo->rStartY+0.5f;
  deFgVerts[1].tmuvtx[0].sow = 1.f * (fSmulti) * deFgVerts[1].oow;
  deFgVerts[1].tmuvtx[0].tow = 0.f * (fTmulti) * deFgVerts[1].oow;

  deFgVerts[2].x = lpSurfInfo->rEndX+0.5f;
  deFgVerts[2].y = lpSurfInfo->rEndY+0.5f;
  deFgVerts[2].tmuvtx[0].sow = 1.f * (fSmulti) * deFgVerts[2].oow;
  deFgVerts[2].tmuvtx[0].tow = 1.f * (fTmulti) * deFgVerts[2].oow;

  deFgVerts[3].x = lpSurfInfo->rStartX+0.5f;
  deFgVerts[3].y = lpSurfInfo->rEndY+0.5f;
  deFgVerts[3].tmuvtx[0].sow = 0.f * (fSmulti) * deFgVerts[3].oow;
  deFgVerts[3].tmuvtx[0].tow = 1.f * (fTmulti) * deFgVerts[3].oow;
}


static void 
doScaleScreenSurf( hTexId_t deTexId, ScaleDir_t deScleDir, ScaleAmt_t deScleAmt )
{
  /* definitions */
  myTexture_t	     *lpTmpMyTxtre; 
  surfInfo_t       *lpSurfInfo;
  GrTexInfo        *lpTexInfo;
  int              i;
  int              nNumScles;
  float            fScleAmt;
  GrLOD_t          tLodToScle;
  GrAspectRatio_t  tCurrAspct;
  double           dNonFrac, ret;
  const float      fScleLCD = 32.f; //least common scale denominator

  /* assumptions */
  assert( deTexId < NUMTEXTURES );

  /* parse the texture array for the right one */
  for ( i = 0; (i < NUMTEXTURES) && (theTextures[i].hTexture != deTexId) ; ++i );
  if (i == NUMTEXTURES)
    return;

  /* initializations */
  lpTmpMyTxtre  = &theTextures[i];
  lpSurfInfo    = &lpTmpMyTxtre->surfInfo;
  lpTexInfo     = &lpTmpMyTxtre->lpTlTexture->info;

  if ((lpSurfInfo->currLod == GR_LOD_1) && (deScleDir == SCALE_IN))
    return;

  if ((lpSurfInfo->currLod >= GR_LOD_32) && (deScleAmt > SCALE_8))
    deScleAmt = SCALE_8;

  /* do everything as multiples of (1/8)LOD jmps, (1)LOD jmp becomes 8*1/8 */
  nNumScles = ( (int) fScleLCD) >> deScleAmt;

  /* scale by a frac of the next LOD your are scaling to */
  tLodToScle = lpSurfInfo->currLod +1;

  tCurrAspct = lpTexInfo->aspectRatio; /* ease of read */
  fScleAmt = 0.0f; /* init then go */
  for ( i = 0; i < nNumScles; ++i)
  {
    /* check to see if i am in another LOD, scale change */
    if (deScleDir == SCALE_IN)
    {
      if ( (lpSurfInfo->rEndX - lpSurfInfo->rStartX == 
             (float) getLodWidth(tLodToScle, tCurrAspct)) &&
           (lpSurfInfo->rEndY - lpSurfInfo->rStartY ==
             (float) getLodHeight(tLodToScle, tCurrAspct)) )
      {
        ++lpSurfInfo->currLod;
        tLodToScle = lpSurfInfo->currLod +1;
      }
    }
    else
    {
      if ( (lpSurfInfo->rEndX - lpSurfInfo->rStartX == 
             (float) getLodWidth(lpSurfInfo->currLod, tCurrAspct)) &&
           (lpSurfInfo->rEndY - lpSurfInfo->rStartY ==
             (float) getLodHeight(lpSurfInfo->currLod, tCurrAspct)) )
      {
        if (lpSurfInfo->currLod != GR_LOD_256)
          --lpSurfInfo->currLod;
          tLodToScle = lpSurfInfo->currLod +1;// == GR_LOD_256 ? GR_LOD_128 : lpSurfInfo->currLod;
      }
    }

    if ( !((deScleDir == SCALE_OUT) && /* is there somtin' to do */
        (getLodWidth(tLodToScle, tCurrAspct) == getLodWidth(tLodToScle -1, tCurrAspct))))
    {
      fScleAmt = ((float) getLodWidth(tLodToScle, tCurrAspct)) / fScleLCD / 2.f;
      lpSurfInfo->fStartX += deScleDir * fScleAmt;
      ret = modf((double) lpSurfInfo->fStartX, &dNonFrac);
      lpSurfInfo->rStartX = (float) dNonFrac +
          (ret > 0.5f ? 1.0f : ret == 0.0f ? 0.0f : 0.5f);

      lpSurfInfo->fEndX -= deScleDir * fScleAmt;
      if ((lpSurfInfo->fStartX) == (lpSurfInfo->fEndX))
         lpSurfInfo->fEndX += 1.0f;
      ret = modf((double) lpSurfInfo->fEndX, &dNonFrac);
      lpSurfInfo->rEndX = (float) dNonFrac +
          (ret > 0.5f ? 1.0f : ret == 0.0f ? 0.0f : 0.5f);
    }

    if ( !((deScleDir == SCALE_OUT) && /* is there somtin' to do */
         (getLodHeight(tLodToScle, tCurrAspct) == getLodHeight(tLodToScle -1, tCurrAspct))))
    {
      fScleAmt = ((float) getLodHeight(tLodToScle, tCurrAspct)) / fScleLCD / 2.f;

      lpSurfInfo->fStartY += deScleDir * fScleAmt;
      ret = modf((double) lpSurfInfo->fStartY, &dNonFrac);
      lpSurfInfo->rStartY = (float) dNonFrac +
          (ret > 0.5f ? 1.0f : ret == 0.0f ? 0.0f : 0.5f);

      lpSurfInfo->fEndY -= deScleDir * fScleAmt;
      if ((lpSurfInfo->fStartY) == (lpSurfInfo->fEndY))
        lpSurfInfo->fEndY += 1.0f;
      ret = modf((double) lpSurfInfo->fEndY, &dNonFrac);
      lpSurfInfo->rEndY = (float) dNonFrac +
          (ret > 0.5f ? 1.0f : ret == 0.0f ? 0.0f : 0.5f);
    }
  }
} /* doScaleScreenSurf */


static int 
getLodWidth( GrLOD_t lod, GrAspectRatio_t aspect )
{
  int  yAspctWdvdr[] = {0, 0, 0, 0, 1, 2, 3};
  int  yLodFctr[]    = {8, 7, 6, 5, 4, 3, 2, 1, 0};
  int  val;

  val = yLodFctr[lod] > yAspctWdvdr[aspect] ? 
          1 << yLodFctr[lod] >> yAspctWdvdr[aspect] : 1;

  return( val );
}


static int 
getLodHeight( GrLOD_t lod, GrAspectRatio_t aspect )
{
  int  yAspctHdvdr[] = {3, 2, 1, 0, 0, 0, 0};
  int  yLodFctr[]    = {8, 7, 6, 5, 4, 3, 2, 1, 0};
  int  val;

  val = yLodFctr[lod] > yAspctHdvdr[aspect] ? 
          1 << yLodFctr[lod] >> yAspctHdvdr[aspect] : 1;

  return( val );
}


static void 
doUnloadTextures( )
{
  int i;

  for ( i = 0; i < NUMTEXTURES; ++i )
  { /* free the textures, if they love you they'll come back */
    free(theTextures[i].lpTlTexture->info.data);
    free(theTextures[i].lpTlTexture);
  }
}


static void
getLodToDwnld( )
{
  /* definitions */
  char ch;
  
  /* initializations */
  g_bDoLodDwnld = FXFALSE;

  /* code */
  tlConClear();
  grBufferClear( 0, 0, 0 );
  grBufferSwap( 1 );
  grBufferClear( 0, 0, 0 );
  tlConOutput("LOD Download menu\n");
  tlConOutput("Choose LOD 0-8 ");
  tlConRender();
  grBufferSwap( 1 );
  ch = tlGetCH();
  g_nLodToDwnld = atoi(&ch);
  
  if ((g_nLodToDwnld >= GR_LOD_256) && (g_nLodToDwnld <= GR_LOD_1))
  {
    tlConOutput("%d\n", g_nLodToDwnld);
    tlConRender();
    grBufferSwap( 1 );
    g_bDoLodDwnld = FXTRUE;
  }
  else
    tlConOutput("invalid LOD\n");

  tlConRender();
  grBufferSwap( 1 );
}


static void
getLodPrtlToDwnld( )
{
  /* definitions */
  char ch;
  int i;

  /* initializations */
  g_nLodToDwnld = 0x0;
  g_bDoLodPrtlDwnld = FXFALSE;
  g_nLodPrtlFstRow  = 0x0;
  g_nLodPrtlLstRow  = 0x0;

  /* code */
  tlConClear();
  grBufferClear( 0, 0, 0 );
  grBufferSwap( 1 );
  grBufferClear( 0, 0, 0 );
  tlConOutput("LOD Prtl Dwnld menu\n");
  tlConOutput("Choose LOD 0-8 ");
  tlConRender();
  grBufferSwap( 1 );
  ch = tlGetCH();
  g_nLodToDwnld = atoi(&ch);
  
  if ((g_nLodToDwnld >= GR_LOD_256) && (g_nLodToDwnld <= GR_LOD_1))
  {
    tlConOutput("%d\n", g_nLodToDwnld);
    tlConRender();
    grBufferSwap( 1 );

    tlConOutput("Entr Fst Row 000-255 ");
    tlConRender();
    grBufferSwap( 1 );
    for ( i = 0; i < 3; ++i )
    {
      ch = tlGetCH();
      tlConOutput("%c", ch);
      tlConRender();
      grBufferSwap( 1 );
      g_nLodPrtlFstRow = (g_nLodPrtlFstRow * 0xA) + atoi(&ch);
    }
    tlConOutput("  %d\n", g_nLodPrtlFstRow);
    tlConRender();
    grBufferSwap( 1 );

    tlConOutput("Entr Lst Row 000-255 ");
    tlConRender();
    grBufferSwap( 1 );
    for ( i = 0; i < 3; ++i )
    {
      ch = tlGetCH();
      tlConOutput("%c", ch);
      tlConRender();
      grBufferSwap( 1 );
      g_nLodPrtlLstRow = (g_nLodPrtlLstRow * 0xA) + atoi(&ch);
    }
    tlConOutput("  %d\n", g_nLodPrtlLstRow);
    tlConRender();
    grBufferSwap( 1 );

    if (((g_nLodPrtlFstRow >= 0x0) && (g_nLodPrtlFstRow < 0x100)) &&
        ((g_nLodPrtlLstRow >= 0x0) && (g_nLodPrtlLstRow < 0x100)))
      g_bDoLodPrtlDwnld = FXTRUE;
    else
      tlConOutput("invalid Row(s)\n");
  }
  else
    tlConOutput("invalid LOD\n");

  tlConRender();
  grBufferSwap( 1 );
}


static void 
doResetFgVerts( hTexId_t deTexId )
{
  /* definitions */
  surfInfo_t  *lpSurfInfo;
  GrTexInfo   *lpTexInfo;
  int         i;
  double      ret, dNonFrac;

  /* code */

  /* parse the texture array for the right one */
  for ( i = 0; (i < NUMTEXTURES) && (theTextures[i].hTexture != deTexId) ; ++i );
  if (i == NUMTEXTURES)
    return;

  /* initializations */
  lpSurfInfo          = &theTextures[i].surfInfo;
  lpSurfInfo->currLod = theTextures[i].lpTlTexture->info.largeLod;
  lpTexInfo           = &theTextures[i].lpTlTexture->info;

  /* reset the surf info */
  lpSurfInfo->fStartX = 
      ((scrWidth - (float) getLodWidth(lpSurfInfo->currLod,
                                       lpTexInfo->aspectRatio) )/2.f);
  ret = modf((double) lpSurfInfo->fStartX, &dNonFrac);
  lpSurfInfo->rStartX = (float) dNonFrac;

  lpSurfInfo->fEndX = 
      (lpSurfInfo->fStartX + (float) getLodWidth(lpSurfInfo->currLod, 
                                                 lpTexInfo->aspectRatio));
  ret = modf((double) lpSurfInfo->fEndX, &dNonFrac);
  lpSurfInfo->rEndX = (float) dNonFrac;

  lpSurfInfo->fStartY = 
      ((scrHeight - (float) getLodHeight(lpSurfInfo->currLod,
                                         lpTexInfo->aspectRatio) )/2.f);
  ret = modf((double) lpSurfInfo->fStartY, &dNonFrac);
  lpSurfInfo->rStartY = (float) dNonFrac;

  lpSurfInfo->fEndY = 
      (lpSurfInfo->fStartY + (float) getLodHeight(lpSurfInfo->currLod, 
                                                  lpTexInfo->aspectRatio));
  ret = modf((double) lpSurfInfo->fEndY, &dNonFrac);
  lpSurfInfo->rEndY = (float) dNonFrac;
  /* done with surface coords */
}

