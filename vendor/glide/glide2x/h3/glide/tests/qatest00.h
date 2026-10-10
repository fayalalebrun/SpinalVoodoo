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

#ifndef _QATEST00_H_
#define _QATEST00_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RED_565   0xF800
#define GRN_565   0x07E0
#define BLU_565   0x001F
#define RED_1555  0x7C00
#define GRN_1555  0x03E0
#define BLU_1555  0x001F
#define RED_4444  0x0F00
#define GRN_4444  0x00F0
#define BLU_4444  0x000F
#define RED_332   0x00E0
#define GRN_332   0x001C
#define BLU_332   0x0003
#define RED_8332  0x00E0
#define GRN_8332  0x001C
#define BLU_8332  0x0003

#define RED   0x000000ff
#define BLUE  0x00ff0000

typedef enum { DISABLE, NEAREST, TRILINEAR, DITHRD_NREST, DITHRD_TRILIN } MipMapMode_t;
typedef enum { SCALE_IN=1, SCALE_OUT=-1 } ScaleDir_t; //sign
typedef enum { SCALE_1=0, SCALE_2, SCALE_4, SCALE_8, SCALE_16, SCALE_32 } ScaleAmt_t;//bitwise shift factor
typedef enum { BOTH_POINT, MIN_BILNEAR, MAG_BILNEAR, BOTH_BILNEAR } FilterMode_t;

typedef struct {
  float    fStartX, fEndX;
  float    rStartX, rEndX;
  float    fStartY, fEndY;
  float    rStartY, rEndY;
  GrLOD_t  currLod;
} surfInfo_t;

typedef struct {
  const char	*lpzTexFileName;
  TlTexture   *lpTlTexture;
  hTexId_t    hTexture;
  surfInfo_t 	surfInfo;
}  myTexture_t;

typedef struct
{
  float sMult;
  float tMult;
} TexCoordFactors;


static void doHelp( void );
static hTexId_t doLoadTexture( const char *lpzFileName, GrTexInfo *tEtTexInfo, 
															 															FxU16 *yColors, FxBool isDwnldble );
static void doUnloadTextures( );
static void doScaleScreenSurf( hTexId_t deTexId, ScaleDir_t deDir, ScaleAmt_t deAmt );
static void getFgVerts( hTexId_t deTexId, GrVertex *deFgVerts );
static int getLodWidth( GrLOD_t lod, GrAspectRatio_t aspect );
static int getLodHeight( GrLOD_t lod, GrAspectRatio_t aspect );
static void getLodToDwnld( );
static void getLodPrtlToDwnld( );
static void doResetFgVerts( hTexId_t deTexId );

#ifdef NDEBUG
#undef NDEBUG
#endif

#ifdef __cplusplus
}
#endif
#endif
