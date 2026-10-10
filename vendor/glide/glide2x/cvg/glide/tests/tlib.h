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

int tlGetOpt( int argc, char *argv[], const char *tags, char *match, char **remArgs[] );

int tlGetResolutionConstant( const char *identifier, float *width, float *height );
void tlGetDimsByConst(const int res, float *w, float *h);
const char *tlGetResolutionString( int res );
const char *tlGetResolutionList( void );

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
                          TlVertex3D *srcList, 
                          unsigned length );
void tlProjectVertices( TlVertex3D *dstList,
                        TlVertex3D *srcList, 
                        unsigned length );

FxBool tlOkToRender(void);

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

#ifdef __cplusplus
}
#endif
#endif
