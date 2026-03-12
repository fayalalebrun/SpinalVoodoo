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

#ifndef _TLIB_H_
#define _TLIB_H_
#ifdef __cplusplus
extern "C" {
#endif

/* If not debugging, change the meaning of the ANSI assert
 * so that it is a harmless wrapper rather than ((void)0) as in <assert.h>
 */
#ifndef DEBUG
#ifdef assert
#undef assert
#endif
#define assert(exp) (void) (exp)
#endif

/* The two most commonly defined macros in the known universe */
#ifndef MIN
#define MIN(__x, __y) (((__x) < (__y)) ? (__x) : (__y))
#endif

#ifndef MAX
#define MAX(__x, __y) (((__x) < (__y)) ? (__y) : (__x))
#endif

#define TL_BACKSPACE 128
#define TL_PAGEUP    129
#define TL_PAGEDOWN  130
#define TL_END       131
#define TL_HOME      132
#define TL_LEFT      133
#define TL_UP        134
#define TL_RIGHT     135
#define TL_DOWN      136
#define TL_INSERT    137
#define TL_DELETE    138

/*
** move the vertex layout defintion to application
*/
typedef struct {
  float  sow;                   /* s texture ordinate (s over w) */
  float  tow;                   /* t texture ordinate (t over w) */  
  float  oow;                   /* 1/w (used mipmapping - really 0xfff/w) */
}  GrTmuVertex;

typedef struct
{
  float x, y;         /* X and Y in screen space */
  float ooz;          /* 65535/Z (used for Z-buffering) */
  float oow;          /* 1/W (used for W-buffering, texturing) */
  float r, g, b, a;   /* R, G, B, A [0..255.0] */
  float z;            /* Z is ignored */
  GrTmuVertex  tmuvtx[GLIDE_NUM_TMU];
} GrVertex;

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              1
#define GR_VERTEX_OOZ_OFFSET            2
#define GR_VERTEX_OOW_OFFSET            3
#define GR_VERTEX_R_OFFSET              4
#define GR_VERTEX_G_OFFSET              5
#define GR_VERTEX_B_OFFSET              6
#define GR_VERTEX_A_OFFSET              7
#define GR_VERTEX_Z_OFFSET              8
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

/*
** define voodoo types
*/
#define TL_VOODOO_UNDEF         1
#define TL_VOODOO         1
#define TL_VOODOORUSH     2
#define TL_VOODOO2        3
#define TL_VOODOOBANSHEE  4

/*
 * Some texture formats are available only on certain pieces of hardware.
 * Until all of the #defines for texture formats are in a standard location
 * we will have to conditionally compile texture-related code.
 */
#define NAPALM_TEXTURES ((defined FX_GLIDE_NAPALM) || (defined FX_GLIDE_RAMPAGE))
#define RAMPAGE_TEXTURES (defined FX_GLIDE_RAMPAGE)

#ifdef FX_GLIDE_NAPALM
#include <g3ext.h>
#endif

#ifdef FX_GLIDE_RAMPAGE
#include <sst2ext.h>
#endif


typedef struct {
  FxBool    pixext;                       /* PIXEXT */
  GrProc    grSstWinOpen;
  GrProc    grColorMask;
  GrProc    grStencilFunc;
  GrProc    grStencilMask;
  GrProc    grStencilOp;
  GrProc    grBufferClearExt;
  GrProc    grLfbConstantStencil;
  GrProc    grTBufferWriteMask;
  FxBool    combineext;                   /* COMBINE */
  GrProc    grColorCombineExt;
  GrProc    grAlphaCombineExt;
  GrProc    grTexColorCombineExt;
  GrProc    grTexAlphaCombineExt;
  FxBool    canDo32BitTexture;            /* TEXFMT */
  FxBool    canDoFXT1Texture;             /* TEXFMT */
  FxBool    canDo2kTexture;               /* TEXFMT */
  FxBool    umaExt;                       /* TEXUMA */
} tlGlideExtension;

void tlInitGlideExt ( tlGlideExtension *gExt);
int tlGetPixelFormat( const char *pf );

int tlGetOpt( int argc, char *argv[], const char *tags, char *match, char **remArgs[] );

int tlGetResolutionConstant( const char *identifier, float *width, float *height );
void tlGetDimsByConst(const int res, float *w, float *h);
const char *tlGetResolutionString( int res );
const char *tlGetResolutionList( void );

void  tlVertexLayout( void );
int tlVoodooType( void );
void  tlSetScreen( float width, float height );
float tlScaleX( float coord );
float tlScaleY( float coord );

void tlConSet( float minX, float minY, 
               float maxX, float maxY,
               int columns, int rows,
               int color );
int  tlConOutput( const char *fmt, ... );
void tlConClear();
void tlConRender();

int  tlKbHit( void );
char tlGetCH( void );
FxU32 tlGethWnd( void );

void tlSleep( int seconds );

#define LFB_DATA_LITERAL       0
#define LFB_DATA_RLE           1
#define IMAGE_SRLE             (((FxU32)'S' << 24) | ((FxU32)'R' << 16) | ((FxU32)'L' << 8) | ((FxU32)'E'))

FxBool SimpleRleDecode(FxU16 width,FxU16 height,FxU8 pixelsize, FxU8 *mem,FxU8 *buff);
void tlGrabRect(void *memory, FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy); 
FxBool tlScreenDump(const char *filename, FxU16 width,  FxU16 height);

typedef struct {
    float x;
    float y;
    float z;
    float w;
    float s;
    float t;
    float r;
    float g;
    float b;
    float a;
} TlVertex3D;

typedef float TlMatrix[4][4];

const float *tlIdentity( void );
const float *tlZRotation( float degrees );
const float *tlYRotation( float degrees );
const float *tlXRotation( float degrees );
const float *tlTranslation( float x, float y, float z );
void tlSetMatrix( const float *m );
void tlMultMatrix( const float *m );
void tlTransformVertices( TlVertex3D *dstList, 
                          const TlVertex3D *srcList, 
                          unsigned length );
void tlProjectVertices( TlVertex3D *dstList,
                        const TlVertex3D *srcList, 
                        unsigned length );
void tlCProjectVertices( TlVertex3D *dstList,
                         const TlVertex3D *srcList, 
                         unsigned length );


FxBool tlOkToRender(void);

#define kMaxGlideContext 10
GrContext_t* tlCheckRenderContext(FxU32 glideDeviceNum);
GrContext_t* tlGetRenderContext(FxU32 glideDeviceNum, FxU32 hWnd, 
                                GrScreenResolution_t res, 
                                GrScreenRefresh_t refresh,
                                GrColorFormat_t colorFmt,
                                GrOriginLocation_t origin,
                                int nColBuf, int nAuxBuf);

FxBool
tlErrorMessage(const char *err);

typedef FxU32 TlPalette[256];
typedef struct {
  FxU8  yRGB[16];
  FxI16 iRGB[4][3];
  FxI16 qRGB[4][3];
  FxU32 packed_data[12];
} TlNCCTable;

typedef union {
    TlPalette  palette;
    TlNCCTable nccTable;
} TlTextureTable;

typedef struct {
    GrTexInfo      info;
    GrTexTable_t   tableType;
    TlTextureTable tableData;
} TlTexture;

#define NO_TABLE ((GrTexTable_t)(~0))

int tlLoadTexture( const char *filename, 
                   GrTexInfo *info, 
                   GrTexTable_t *tableType,
                   void *table );

/* We are making function calls from within assert() */
#ifdef NDEBUG
#undef NDEBUG
#endif

/* Don't do this muckage if this is a DOS build since 
 * it is already a 'console' app.
 */
#if !__DOS32__ && !macintosh && !__unix__ && !defined(__linux__)
/* redefine main proc to something else */
#define main mainT
#endif /* !__DOS32__ && !macintosh && !__unix__ && !defined(__linux__) */

#if macintosh
extern char *strdup(const char *src);
#endif

#ifdef __cplusplus
}
#endif
#endif
