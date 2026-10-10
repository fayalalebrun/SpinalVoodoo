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
#include <stdarg.h>
#ifndef __linux__
#include <conio.h>
#endif
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <glide.h>

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
#include <cvg.h>
#else
#endif

#ifdef __DJGPP__
#include <crt0.h>
int crt0_startup_flags = _CRT0_FLAG_NONMOVE_SBRK;
#endif

#include "tlib.h"

static FxBool okToRender = FXTRUE;
static FxBool fullScreen = FXTRUE;


FxBool
tlOkToRender()
{
  return okToRender;
} /* tlOkToRender */

/*-------------------------------------------------------------------
  Function:tlGetOpt
  Date: 2/26
  Implementor(s): jdt
  Library: Test Library
  Description:
  Incrementally search an argument list for matches.
  Arguments:
  argc    - first argument to main
  argv    - second argument to main
  tags    - string of non-whitespace characters to be search for in the 
            argument string
  match   - pointer to storage for matched character 
  remArgs - pointer to storage for remaining arglist after any match
  Return:
   1 if matched
   0 if no more arguments
  -1 if unrecognized
  -------------------------------------------------------------------*/
int tlGetOpt( int argc, char *argv[], 
              const char *tags, char *match, 
              char **remArgs[] ){
    static int firstCall;
    static int lastArg;
    
    int rv = 0;

    if ( !firstCall ) {
        lastArg   = 1;
        firstCall = 1;
    } 

    while( (lastArg<argc) && (*argv[lastArg] != '-') ) lastArg++;

    if ( lastArg < argc ) {
        unsigned tag;
        for( tag = 0; tag < strlen( tags ); tag++ ) {
            if ( *(argv[lastArg]+1) == tags[tag] ) {
                *match = tags[tag];
                *remArgs = &argv[++lastArg];
                rv = 2;
                break;
            }
        }
        rv--;
    }
    return rv;
}

/*-------------------------------------------------------------------
  Function: tlGetResolution
  Date: 2/25
  Implementor(s): jdt
  Library: Test Library
  Description:
  Get the integer representing the provided resolution and return the
  necessary floats for screen scaling.
  Arguments:
  identifier - string identifying resolution of the form XRSxYRS
  width - pointer to float storage for width
  height - pointer to float storage for height
  Return:
  int
  -------------------------------------------------------------------*/

typedef struct {
    const char *id;
    int         res;
    float       width;
    float       height;
} ResToRes;

static ResToRes resTable[] = {
    { "320x200", GR_RESOLUTION_320x200, 320.0f, 200.0f },
    { "320x240", GR_RESOLUTION_320x240, 320.0f, 240.0f },
    { "512x256", GR_RESOLUTION_512x256, 512.0f, 256.0f },
    { "512x384", GR_RESOLUTION_512x384, 512.0f, 384.0f },
    { "640x400", GR_RESOLUTION_640x400, 640.0f, 400.0f },
    { "640x480", GR_RESOLUTION_640x480, 640.0f, 480.0f },
    { "800x600", GR_RESOLUTION_800x600, 800.0f, 600.0f },
    { "856x480", GR_RESOLUTION_856x480, 856.0f, 480.0f },
    { "960x720", GR_RESOLUTION_960x720, 960.0f, 720.0f }
#ifdef H3D
   ,{ "640x240", GR_RESOLUTION_640x240_AUTOFLIPPED, 640.0f, 240.0f },
    { "800x300", GR_RESOLUTION_800x300_AUTOFLIPPED, 800.0f, 300.0f }
#endif
};

static long resTableSize = sizeof( resTable ) / sizeof( ResToRes );

void
tlGetDimsByConst(const int resConst, float *w, float *h)
{
  int match;
  
  for( match = 0; match < resTableSize; match++ ) {
    if ( resTable[match].res == resConst ){
      *w  = resTable[match].width;
      *h = resTable[match].height;
      return;
    }
  }
#ifdef __WIN32__
  {
    extern void getWindowSize(float *width, float *height);
    getWindowSize(w, h);
  }
#endif
} /* tlGetDimsByConst */

/*-------------------------------------------------------------------
  Function: tlGetReslutionConstant
  Date: 2/28
  Implementor(s): jdt
  Library: Test Library
  Description:
  Returns the glide constant for a command line resolution argument
  Arguments:
  identifier - command line resolution arg.
  width - storage for floating point screen width
  height - storaget for floating piont screen height
  Return:
  GrResolution_t matching resolution argument
  -------------------------------------------------------------------*/
static int Res = GR_RESOLUTION_640x480;
int 
tlGetResolutionConstant( const char *identifier, float *width, 
                        float *height ) 
{
  int match;
  
  for( match = 0; match < resTableSize; match++ ) {
    if ( !strcmp( identifier, resTable[match].id ) ) {
      Res = resTable[match].res;
      *width  = resTable[match].width;
      *height = resTable[match].height;
      fullScreen = FXTRUE;
      return Res;
    }
  }
#ifdef __WIN32__
  {
    extern void getWindowSize(float *width, float *height);

    fullScreen = FXFALSE;
    getWindowSize(width, height);
  }
#endif
  return Res = GR_RESOLUTION_NONE;
}

/*-------------------------------------------------------------------
  Function: tlGetResolutionString
  Date: 2/28
  Implementor(s): jdt
  Library: Test LIbrary
  Description:
  Returns a string value for a given resolution constant
  Arguments:
  res - resolution constant
  Return:
  string representing resolution
  -------------------------------------------------------------------*/
const char *tlGetResolutionString( int res ) {
    int match;
    const char *str = "unknown";

    for( match = 0; match < resTableSize; match++ ) {
        if ( resTable[match].res == res ) {
            str = resTable[match].id;
            break;
        }
    }
    return str;
}

/*-------------------------------------------------------------------
  Function: tlGetResolutionList
  Date: 2/28
  Implementor(s): jdt
  Library: TestLibrary
  Description:
  Return a list of all supported resolutions
  Arguments:
  none
  Return:
  const char * to resolution list
  -------------------------------------------------------------------*/
const char *tlGetResolutionList( void ) {
    char *list = calloc( sizeof( char ), 256 );
    int member;
    strcat( list, resTable[0].id );
    for( member = 1; member < resTableSize; member++ ) {
        strcat( list, " | " );
        strcat( list, resTable[member].id );
    }
    return list;
}

/*-------------------------------------------------------------------
  Function: tlSetScreen
  Date: 2/28
  Implementor(s): jdt
  Library: Test Library
  Description:
  Set up screen scaling
  Arguments:
  width - width of screen
  height - height of screen
  Return:
  none
  -------------------------------------------------------------------*/
static float scrXScale;
static float scrYScale;

void tlSetScreen( float width, float height ) {
    scrXScale = width;
    scrYScale = height;
    return;
}

/*-------------------------------------------------------------------
  Function: tlScaleX
  Date: 2/28
  Implementor(s): jdt
  Library: Test Lib
  Description:
  Scale X coordinates from normalized device coordinates [ 0.0, 1.0 )
  to Screen Coordinates [ 0.0, WidthOfScreenInPixels )
  Arguments:
  coord - x coordinate to scale
  Return:
  scaled x coordinate
  -------------------------------------------------------------------*/
float tlScaleX( float coord ) {
    return coord * scrXScale;
}

/*-------------------------------------------------------------------
  Function: tlScaleY
  Date: 2/28
  Implementor(s): jdt
  Library: Test Lib
  Description:
  Scale Y coordinates from normalized device coordinates [ 0.0, 1.0 )
  to Screen Coordinates [ 0.0, HeightOfScreenInPixels )
  Arguments:
  coord - y coordinate to scale
  Return:
  scaled y coordinate
  -------------------------------------------------------------------*/
float tlScaleY( float coord ) {
    return coord *scrYScale;
}


/*-------------------------------------------------------------------
  Function: tlSetConsole
  Date:
  Implementor(s):
  Library:
  Description:
  Arguments:
  Return:
  -------------------------------------------------------------------*/
static unsigned char fontTable[128][2];
static GrState state;
static GrTexInfo fontInfo;
static unsigned long fontAddress;
static const char fontString[] = "ABCDEFGHIJKLMN"
                                 "OPQRSTUVWXYZ01"
                                 "23456789.,;:*-"
                                 "+/_()<>|[]{}! ";

static const int fontWidth    = 9*2;
static const int fontHeight   = 12*2;
static const int charsPerLine = 14;

static int fontInitialized;

#if 0 /* not used */
static void grabTex( FxU32 addr, void *storage );
static void putTex( FxU32 addr, void *storage );
#endif
static void consoleScroll( void );
static void drawChar( char character, float x, float y, float w, float h );

#include "tldata.inc"

/*-------------------------------------------------------------------
  Function: tlSetConsole
  Date: 2/28
  Implementor(s): jdt
  Library: Test Library
  Description:
  Initialize Console for printing.  The console will scroll text 
  60 column text in the window described by minx, miny, maxx, maxy.
  Arguments:
  minX, minY - upper left corner of console
  maxX, maxY - lower right corner of console
  rows    - rows of text to display
  columns - columns to display before scroll
  Return:
  none
  -------------------------------------------------------------------*/
static char *consoleGrid;
static int   consoleRows;
static int   consoleColumns;
static int   consoleX;
static int   consoleY;
static int   consoleColor;
static float consoleOriginX;
static float consoleOriginY;
static float consoleCharWidth;
static float consoleCharHeight;

void tlConSet( float minX, float minY, 
               float maxX, float maxY,
               int columns, int rows,
               int color ) {
    int entry;
    char xCoord;
    char yCoord;

    fontInfo.smallLod    = GR_LOD_128;
    fontInfo.largeLod    = GR_LOD_128;
    fontInfo.aspectRatio = GR_ASPECT_2x1;
    fontInfo.format      = GR_TEXFMT_ALPHA_8;
    fontInfo.data        = &fontData[0];

    if ( getenv( "FX_GLIDE_NO_FONT" ) ) {
        fontInitialized = 0;
        return;
    }

    for( entry = 1; entry < 128; entry++ ) {
        char *hit = strchr( fontString, entry );
        if ( hit ) {
            int offset = hit - fontString;
            
            xCoord = ( offset % charsPerLine )  * fontWidth;
            yCoord = ( offset / charsPerLine )  * fontHeight; 

            fontTable[entry][0] = xCoord;
            fontTable[entry][1] = yCoord;
        }
    }
    
    if ( consoleGrid ) free( consoleGrid );

    consoleGrid = calloc( sizeof( char ), rows * columns );
    memset( consoleGrid, 32, rows*columns );
    consoleRows    = rows;
    consoleColumns = columns;
    consoleX = consoleY = 0;

    consoleColor      = color;
    consoleOriginX    = minX;
    consoleOriginY    = minY;
    consoleCharWidth  = ( (maxX - minX)/(float)columns );
    consoleCharHeight = ( (maxY - minY)/(float)rows    );

    fontAddress = grTexMaxAddress( 0 ) - 
        grTexCalcMemRequired( fontInfo.smallLod, fontInfo.largeLod,
                              fontInfo.aspectRatio, fontInfo.format );

    fontInitialized = 1;

    return; 
};

/*-------------------------------------------------------------------
  Function: tlConOutput
  Date: 2/28
  Implementor(s): jdt
  Library: Test Library
  Description:
  Output a printf style string to the console
  Arguments:
  fmt - format string
  ... - other args
  Return:
  int - number of chars printed
  -------------------------------------------------------------------*/
int tlConOutput( const char *fmt, ... ) {
    int rv = 0;
    va_list argptr;

    if( fontInitialized ) {
        static char buffer[1024];
        const char *c;
        char* temp;

        va_start( argptr, fmt );
        rv = vsprintf( buffer, fmt, argptr );
        va_end( argptr );

        temp = buffer;
        while(*temp != '\0') {
            if (*temp >= 'a' && *temp <= 'z')
                *temp -= ('a'-'A');
            temp++;
        }

        c = buffer;

        /* update console grid */

        while( *c ) {
            switch( *c ) {
            case '\n':
                consoleY++;
            case '\r':
                consoleX = 0;
                if ( consoleY >= consoleRows ) {
                    consoleY = consoleRows - 1;
                    consoleScroll();
                }
                break;
            default:
                if ( consoleX >= consoleColumns ) {
                    consoleX = 0;
                    consoleY++;
                    if ( consoleY >= consoleRows ) {
                        consoleY = consoleRows - 1;
                        consoleScroll();
                    }
                }
                consoleGrid[(consoleY*consoleColumns)+consoleX]=*c;
                consoleX++;
                break;
            }
            c++;
        }
    }

    return rv;
}

/*-------------------------------------------------------------------
  Function: tlConClear
  Date: 3/1
  Implementor(s): jdt
  Library: Test Library
  Description:
  Clear the console
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
void tlConClear() {
    memset( consoleGrid, 32, consoleRows*consoleColumns );
    consoleX = consoleY = 0;
    return;
}


/*-------------------------------------------------------------------
  Function: tlConRender
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Render the console
  Arguments:
  none
  Return:
  none
  -------------------------------------------------------------------*/
void tlConRender( void ) {
    if( fontInitialized ) {
        int x, y;
        
        grGlideGetState( &state );

        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                        GR_COMBINE_FACTOR_LOCAL,
                        GR_COMBINE_LOCAL_CONSTANT,
                        GR_COMBINE_OTHER_TEXTURE,
                        FXFALSE );
        grAlphaCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                        GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_NONE,
                        GR_COMBINE_OTHER_TEXTURE,
                        FXFALSE );
        grTexCombine( GR_TMU0,
                      GR_COMBINE_FUNCTION_LOCAL,
                      GR_COMBINE_FACTOR_NONE,
                      GR_COMBINE_FUNCTION_LOCAL,
                      GR_COMBINE_FACTOR_NONE,
                      FXFALSE, 
                      FXFALSE );
        grAlphaBlendFunction( GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA,
                              GR_BLEND_ONE, GR_BLEND_ZERO );
        grAlphaTestFunction( GR_CMP_ALWAYS );
        grTexFilterMode( GR_TMU0,
                         GR_TEXTUREFILTER_BILINEAR,
                         GR_TEXTUREFILTER_BILINEAR );
        grTexMipMapMode( GR_TMU0,
                         GR_MIPMAP_DISABLE,
                         FXFALSE );
        grDepthBufferFunction( GR_CMP_ALWAYS );
        grAlphaTestReferenceValue( 0x1 );
        grSstOrigin( GR_ORIGIN_UPPER_LEFT );
        grCullMode( GR_CULL_DISABLE );
        grTexDownloadMipMap( 0, fontAddress, GR_MIPMAPLEVELMASK_BOTH,
                             &fontInfo );
        grTexSource( 0, fontAddress, 
                     GR_MIPMAPLEVELMASK_BOTH, &fontInfo );
        grClipWindow( (int)tlScaleX(0.0f),(int)tlScaleY(0.0f),
                      (int)tlScaleX(1.0f),(int)tlScaleY(1.0f) );

        for( y = 0; y < consoleRows; y++ ) {
            float charX = consoleOriginX;
            float charY = consoleOriginY+(consoleCharHeight*y);
            for( x = 0; x < consoleColumns; x++ ) {
                drawChar( consoleGrid[(y*consoleColumns)+x],
                          charX, charY, 
                          consoleCharWidth, 
                          consoleCharHeight );
                charX += consoleCharWidth;
            }
        }

        grGlideSetState(&state);
    }

    return;
}

/*-------------------------------------------------------------------
  Function: tlSleep
  Date: 3/1
  Implementor(s): jdt
  Library: Test Library
  Description:
  Block for a number of seconds 
  Arguments:
  seconds - number of seconds before function returns
  Return:
  none
  -------------------------------------------------------------------*/
void tlSleep( int seconds ) {
    time_t time0 = time( 0 );
    while( (time(0)-time0) < seconds );
}

/*-------------------------------------------------------------------
  Function: tlIdentity
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description: 
  Return an identity matrix
  Arguments:
  none
  Return:
  const pointer to identity matrix
  -------------------------------------------------------------------*/
static TlMatrix currentMatrix;

#define DEGREE (.01745328f)

const float *tlIdentity( void ) {
    static TlMatrix m;
    m[0][0] = 1.0f, m[0][1] = 0.0f, m[0][2] = 0.0f, m[0][3] = 0.0f;
    m[1][0] = 0.0f, m[1][1] = 1.0f, m[1][2] = 0.0f, m[1][3] = 0.0f;
    m[2][0] = 0.0f, m[2][1] = 0.0f, m[2][2] = 1.0f, m[2][3] = 0.0f;
    m[3][0] = 0.0f, m[3][1] = 0.0f, m[3][2] = 0.0f, m[3][3] = 1.0f;
    return &m[0][0];
}

/*-------------------------------------------------------------------
  Function: tlXRotation
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  Generate a rotation about the x axis
  Arguments:
  degrees - number of degrees to rotate
  Return:
  const point to rotation matrix
  -------------------------------------------------------------------*/
const float *tlXRotation( float degrees ) {
    static TlMatrix m;
    float c = (float)cos( degrees * DEGREE );
    float s = (float)sin( degrees * DEGREE );
    m[0][0] = 1.0f, m[0][1] = 0.0f, m[0][2] = 0.0f, m[0][3] = 0.0f;
    m[1][0] = 0.0f, m[1][1] =    c, m[1][2] =    s, m[1][3] = 0.0f;
    m[2][0] = 0.0f, m[2][1] =   -s, m[2][2] =    c, m[2][3] = 0.0f;
    m[3][0] = 0.0f, m[3][1] = 0.0f, m[3][2] = 0.0f, m[3][3] = 1.0f;
    return &m[0][0];
}


/*-------------------------------------------------------------------
  Function: tlYRotation
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  Generate a rotation about the y axis
  Arguments:
  degrees - number of degrees to rotate
  Return:
  const point to rotation matrix
  -------------------------------------------------------------------*/
const float *tlYRotation( float degrees ) {
    static TlMatrix m;
    float c = (float)cos( degrees * DEGREE );
    float s = (float)sin( degrees * DEGREE );
    m[0][0] =    c, m[0][1] = 0.0f, m[0][2] =   -s, m[0][3] = 0.0f;
    m[1][0] = 0.0f, m[1][1] = 1.0f, m[1][2] = 0.0f, m[1][3] = 0.0f;
    m[2][0] =    s, m[2][1] = 0.0f, m[2][2] =    c, m[2][3] = 0.0f;
    m[3][0] = 0.0f, m[3][1] = 0.0f, m[3][2] = 0.0f, m[3][3] = 1.0f;
    return &m[0][0];
}


/*-------------------------------------------------------------------
  Function: tlZRotation
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  Generate about the z axis
  Arguments:
  degrees - number of degrees to rotate
  Return:
  const point to rotation matrix
  -------------------------------------------------------------------*/
const float *tlZRotation( float degrees ) {
    static TlMatrix m;
    float c = (float)cos( degrees * DEGREE );
    float s = (float)sin( degrees * DEGREE );
    m[0][0] =    c, m[0][1] =    s, m[0][2] = 0.0f, m[0][3] = 0.0f;
    m[1][0] =   -s, m[1][1] =    c, m[1][2] = 0.0f, m[1][3] = 0.0f;
    m[2][0] = 0.0f, m[2][1] = 0.0f, m[2][2] = 1.0f, m[2][3] = 0.0f;
    m[3][0] = 0.0f, m[3][1] = 0.0f, m[3][2] = 0.0f, m[3][3] = 1.0f;
    return &m[0][0];
}

/*-------------------------------------------------------------------
  Function: tlTranslation
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  Generate a translation matrix
  Arguments:
  x, y, z - offsets to translate origin
  Return:
  const point to translation matrix
  -------------------------------------------------------------------*/
const float *tlTranslation( float x, float y, float z ) {
    static TlMatrix m;
    m[0][0] = 1.0f, m[0][1] = 0.0f, m[0][2] = 0.0f, m[0][3] = 0.0f;
    m[1][0] = 0.0f, m[1][1] = 1.0f, m[1][2] = 0.0f, m[1][3] = 0.0f;
    m[2][0] = 0.0f, m[2][1] = 0.0f, m[2][2] = 1.0f, m[2][3] = 0.0f;
    m[3][0] =    x, m[3][1] =    y, m[3][2] =    z, m[3][3] = 1.0f;
    return &m[0][0];
}

/*-------------------------------------------------------------------
  Function: tlSetMatrix
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  Set the current matrix.  This matrix translates the object into
  View space from local coordiantes during calls to transformVertices
  All spaces are considered to by -1.0->1.0 normalized.
  Arguments:
  m - pointer to matrix
  Return:
  none
  -------------------------------------------------------------------*/
void tlSetMatrix( const float *m ) {
    memcpy( currentMatrix, m, sizeof( TlMatrix ) );
    return;
}

/*-------------------------------------------------------------------
  Function: tlMultMatrix
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  Multiply the current matrix by the provided matrix
  Arguments:
  matrix to post-cat to the current matrix
  Return:
  none
  -------------------------------------------------------------------*/
void tlMultMatrix( const float *m ) {
    TlMatrix result;
    TlMatrix mat;
    int i, j;

    memcpy( mat, m, sizeof( TlMatrix ) );

    for( j = 0; j < 4; j++ ) {
        for( i = 0; i < 4; i++ ) {
            result[j][i] = 
                currentMatrix[j][0] * mat[0][i] +
                currentMatrix[j][1] * mat[1][i] +
                currentMatrix[j][2] * mat[2][i] +
                currentMatrix[j][3] * mat[3][i];
        }
    }
    memcpy( currentMatrix, result, sizeof( TlMatrix ) );
    
}

/*-------------------------------------------------------------------
  Function: tlTransformVertices
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  Transform a list of vertices from model space into view space
  Arguments:
  dstVerts - memory to store transformed vertices
  srcVerts - array of vertices to be transformed
  length - number of vertices to transform
  Return:
  none
  -------------------------------------------------------------------*/
void tlTransformVertices( TlVertex3D *dstVerts, TlVertex3D *srcVerts, unsigned length ) {
    TlVertex3D tmp, v;
    TlMatrix m;
    unsigned i;

    memcpy( m, currentMatrix, sizeof( TlMatrix ) );
    for( i = 0; i < length; i++ ) {
        v = srcVerts[i];
        tmp = v;
        tmp.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + v.w * m[3][0];
        tmp.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + v.w * m[3][1];
        tmp.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + v.w * m[3][2];
        tmp.w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + v.w * m[3][3];
        dstVerts[i] = tmp;
    }
    return;
}

/*-------------------------------------------------------------------
  Function: tlProjectVertices
  Date: 3/3
  Implementor(s): jdt
  Library: Test Library
  Description:
  perspective project a set of vertices into normalized 2D space (0,1)
  Arguments:
  dstVerts - memory to store projected vertices
  srcVerts - array of vertices to be transformed
  length - number of vertices to transform
  Return:
  none
  -------------------------------------------------------------------*/
#define VP_OFFSET 1.0f
#define VP_SCALE  0.5f

void tlProjectVertices( TlVertex3D *dstVerts, TlVertex3D *srcVerts, unsigned length ) {
    TlVertex3D tmp, v;
    TlMatrix m;
    unsigned i;

    /* simplified perspective proj matrix assume unit clip volume */
    m[0][0] = 1.0f, m[0][1] = 0.0f, m[0][2] = 0.0f, m[0][3] = 0.0f;
    m[1][0] = 0.0f, m[1][1] = 1.0f, m[1][2] = 0.0f, m[1][3] = 0.0f;
    m[2][0] = 0.0f, m[2][1] = 0.0f, m[2][2] = 1.0f, m[2][3] = 1.0f;
    m[3][0] = 0.0f, m[3][1] = 0.0f, m[3][2] = 0.0f, m[3][3] = 0.0f;

    for( i = 0; i < length; i++ ) {
        v = srcVerts[i];
        tmp = v;
        tmp.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + v.w * m[3][0];
        tmp.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + v.w * m[3][1];
        tmp.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + v.w * m[3][2];
        tmp.w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + v.w * m[3][3];
        tmp.x /= tmp.w, tmp.y /= tmp.w, tmp.z /= tmp.w;
        tmp.x += VP_OFFSET, tmp.x *= VP_SCALE;
        tmp.y += VP_OFFSET, tmp.y *= VP_SCALE;
        dstVerts[i] = tmp;
    }
}


/*-------------------------------------------------------------------
  Function: tlLoadTexture
  Date: 3/3
  Implementor(s): jdt
  Library: Test Libarary
  Description:
  Load Texture
  
  This example loads textures from a .3df file.  3DF files
  containe pre-computed mipmaps data along with any
  necessary supplementary information, for example 
  palettes, ncc-tables, level-of-detail description,
  aspect ratio or format 
  
  The gu3dfGetInfo and gu3dfLoad APIs load A 3DF file
  into Gu3DfInfo structure from a file on disk.  Data
  can then be extracted from the gu3DfInfo structure 
  to initialize a GrTexInfo structure used in the 
  glide texturing routines.  Also note that texture table
  ( either NCC or Palette ) management is left up to the
  application programmer.  
  Arguments:
  filename - name of .3df file on disk
  info - Pointer to GrTexInfo
  tableType - pointer to tabletype
  table - pointer to table data
  Return:
  0 - fail
  1 - pass
  -------------------------------------------------------------------*/
static GrTexTable_t texTableType( GrTextureFormat_t format );

int tlLoadTexture( const char *filename, 
                   GrTexInfo *info, 
                   GrTexTable_t *tableType,
                   void *table ) {
    Gu3dfInfo tdfInfo;
    int rv = 0;

    assert( filename );
    assert( info );
    assert( tableType );
    assert( table );
    if ( gu3dfGetInfo( filename, &tdfInfo ) ) {
        tdfInfo.data = malloc( tdfInfo.mem_required );
        assert( tdfInfo.data );
        if ( gu3dfLoad( filename, &tdfInfo ) ) {
            info->smallLod    = tdfInfo.header.small_lod;
            info->largeLod    = tdfInfo.header.large_lod;
            info->aspectRatio = tdfInfo.header.aspect_ratio;
            info->format      = tdfInfo.header.format;
            info->data        = tdfInfo.data;
            *tableType = texTableType( info->format );
            switch( *tableType ) {
            case GR_TEXTABLE_NCC0:
            case GR_TEXTABLE_NCC1:
            case GR_TEXTABLE_PALETTE:
                memcpy( table, &(tdfInfo.table), sizeof( TlTextureTable ) );
                break;
            default:
                break;
            }
            rv = 1;
        }
    }
    return rv;
}



/*--------------------------------------------------------------------
  Static Helpers
  --------------------------------------------------------------------*/

static void consoleScroll( void ) {
    memmove( consoleGrid, 
             consoleGrid + consoleColumns,
             (consoleRows-1)*consoleColumns );
    memset( consoleGrid+(consoleRows-1)*consoleColumns,
            32, 
            consoleColumns );
}

static void drawChar( char character, float x, float y, float w, float h ) {
    GrVertex a, b, c, d;
    /* a---b
       |\  |    
       | \ |
       |  \|
       c---d */

    if ( character == 32 ) return;

    a.oow = b.oow = c.oow = d.oow = 1.0f;

    a.x = c.x = tlScaleX(x);
    a.y = b.y = tlScaleY(y);
    d.x = b.x = tlScaleX(x+w);
    d.y = c.y = tlScaleY(y+h);

    grConstantColorValue( consoleColor );

    a.tmuvtx[0].sow = c.tmuvtx[0].sow = 
        (float)fontTable[(unsigned char)character][0];
    a.tmuvtx[0].tow = b.tmuvtx[0].tow = 
        (float)fontTable[(unsigned char)character][1];
    d.tmuvtx[0].sow = b.tmuvtx[0].sow = 
        a.tmuvtx[0].sow + (float)fontWidth;
    d.tmuvtx[0].tow = c.tmuvtx[0].tow = 
        a.tmuvtx[0].tow + (float)fontHeight;

    grDrawTriangle( &a, &d, &c );
    grDrawTriangle( &a, &b, &d );
    return;
}




#if 0 /* not used */
static void readRegion( void *data, 
                        int x, int y,
                        int w, int h );
static void writeRegion( void *data, 
                         int x, int y,
                         int w, int h );


static void putTex( FxU32 addr, void *storage ) {
    GrTexInfo texInfo;

    texInfo.smallLod    = GR_LOD_256;
    texInfo.largeLod    = GR_LOD_256;
    texInfo.aspectRatio = GR_ASPECT_1x1;
    texInfo.format      = GR_TEXFMT_RGB_565;
    texInfo.data        = storage;

    grTexDownloadMipMap( 0, addr, GR_MIPMAPLEVELMASK_BOTH, &fontInfo );
}


static void grabTex( FxU32 addr, void *storage ) {
    static FxU16 tmpSpace[256][256];
    GrTexInfo   texInfo;
    GrVertex    a, b, c, d;

    grGlideGetState( &state );
    grDitherMode( GR_DITHER_DISABLE );
    grColorMask( FXTRUE, FXFALSE );
    grSstOrigin( GR_ORIGIN_UPPER_LEFT );
    grCullMode( GR_CULL_DISABLE );

    /* Grab Upper Left 256*256 of frame buffer */
    readRegion( tmpSpace, 0, 0, 256, 256 );

    /* Grab First 256x256 MM in Texture Ram */
    texInfo.smallLod    = GR_LOD_256;
    texInfo.largeLod    = GR_LOD_256;
    texInfo.aspectRatio = GR_ASPECT_1x1;
    texInfo.format      = GR_TEXFMT_RGB_565;
    texInfo.data        = 0;
    grTexMipMapMode( 0, GR_MIPMAP_DISABLE, FXFALSE );
    grTexFilterMode( 0, 
                     GR_TEXTUREFILTER_POINT_SAMPLED, 
                     GR_TEXTUREFILTER_POINT_SAMPLED );
    grTexCombine( 0, 
                  GR_COMBINE_FUNCTION_LOCAL, 
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_LOCAL, 
                  GR_COMBINE_FACTOR_NONE,
                  FXFALSE,
                  FXFALSE );
    grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER,
                    GR_COMBINE_FACTOR_ONE,
                    GR_COMBINE_LOCAL_NONE,
                    GR_COMBINE_OTHER_TEXTURE,
                    FXFALSE );
    grTexSource( 0, addr, GR_MIPMAPLEVELMASK_BOTH, &texInfo );
    grAlphaBlendFunction( GR_BLEND_ONE, GR_BLEND_ZERO,
                          GR_BLEND_ONE, GR_BLEND_ZERO);
    grDepthBufferFunction( GR_DEPTHBUFFER_DISABLE );
    grAlphaTestFunction( GR_CMP_ALWAYS );    
    grFogMode( GR_FOG_DISABLE );
    grCullMode( GR_CULL_DISABLE );
    grChromakeyMode( GR_CHROMAKEY_DISABLE );
    /*-------------------
      A---B
      | \ |
      C---D
      -------------------*/
    a.oow = a.tmuvtx[0].oow = 1.0f;
    b = c = d = a;
    a.x = c.x = a.y = b.y = 0.5f;
    b.x = d.x = c.y = d.y = 255.6f;
    a.tmuvtx[0].sow = c.tmuvtx[0].sow = a.tmuvtx[0].tow = b.tmuvtx[0].tow =
        0.5f;
    b.tmuvtx[0].sow = d.tmuvtx[0].sow = c.tmuvtx[0].tow = d.tmuvtx[0].tow =
        0.5f;
    grDrawTriangle( &a, &d, &c );
    grDrawTriangle( &a, &b, &d );
    readRegion( storage, 0, 0, 256, 256 );
    
    /* Restore The Upper Left Hand of Frame Buffer */
    writeRegion( tmpSpace, 0, 0, 256, 256 );
    grGlideSetState( &state );
    return;
}

static void readRegion( void *data, 
                        int sx, int sy,
                        int w, int h ) {
    int x; int y;
    GrLfbInfo_t info;
    
    info.size = sizeof( info );

    assert( grLfbLock( GR_LFB_READ_ONLY,
                       GR_BUFFER_BACKBUFFER,
                       GR_LFBWRITEMODE_ANY,
                       GR_ORIGIN_UPPER_LEFT,
                       FXFALSE,
                       &info ) );

    for( y = 0; y < h; y++ ) {
        unsigned short *dst = ((unsigned short *)data+
                                (w*y));
        unsigned short *src = (unsigned short*)(((char*)info.lfbPtr)
                                                +(info.strideInBytes*(sy+y))
                                                +(sx<<1));
        for( x = 0; x < w; x++ ) {
            *dst++ = *src++;
        }
    }

    
    assert( grLfbUnlock( GR_LFB_READ_ONLY, GR_BUFFER_BACKBUFFER ) );
    return;
}
static void writeRegion( void *data, 
                         int sx, int sy,
                         int w, int h ) {
    int x; int y;
    GrLfbInfo_t info;

    info.size = sizeof( info );

    assert( grLfbLock( GR_LFB_WRITE_ONLY,
                       GR_BUFFER_BACKBUFFER,
                       GR_LFBWRITEMODE_ANY,
                       GR_ORIGIN_UPPER_LEFT,
                       FXFALSE,
                       &info ) );

    for( y = 0; y < h; y++ ) {
        unsigned short *src = ((unsigned short *)data+
                                (w*y));
        unsigned short *dst = (unsigned short*)(((char*)info.lfbPtr)
                                                +(info.strideInBytes*(sy+y))
                                                +(sx<<1));
        for( x = 0; x < w; x++ ) {
            *dst++ = *src++;
        }
    }

    assert( grLfbUnlock( GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER ) );
    return;
}
#endif


static GrTexTable_t texTableType( GrTextureFormat_t format ) {
    GrTexTable_t rv = (GrTexTable_t)NO_TABLE;
    switch( format ) {
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

/*
 * SimpleRleDecode
 *  simple rle decoder
 * Arguments:
 *  width : width of the image
 *  height : height of the image
 *  pixelsize : 1-4
 *  *mem : compressed data
 *  *buff : uncompressed data
 * Return:
 *  TRUE if decode sucessful. otherwise FALSE
 * The simple rle file file format
 *   width: 2 bytes
 *   height: 2 bytes
 *   compression type: 1 byte (0: literal data, 1: rle)
 *   depth: 1 byte (16 for sst1)
 *   image data
 *      1st byte: control byte. msb = 1 indicate a run, msb = 0 indicate a literal string
 *                bit 7-0 is counter. it is zero based.
 *      next depth/8 bytes: pixel data
 */
FxBool
SimpleRleDecode
(
 FxU16 width,
 FxU16 height,
 FxU8  pixelsize,
 FxU8  *mem,
 FxU8  *buff
)
{
  FxU32 count = width * height;
  FxU8 run, lit;
  
  while (count) {
    if (*mem & 0x80) {
      run = *mem & 0x7f;
      run++;
      mem++;
      if (count < run)
        return FXFALSE;
      count -= run;
      while (run) {
        memcpy(buff, mem, pixelsize);
        run--;
        buff+=pixelsize;
      }
      mem+=pixelsize;
    }
    else {
      lit = *mem;
      lit++;
      mem++;
      if (count < lit)
        return FXFALSE;
      count -= lit;
      while (lit) {
        memcpy(buff, mem, pixelsize);
        lit--;
        buff+=pixelsize;
        mem+=pixelsize;
      }
    }
  }
  return FXTRUE;
}

/*
 * WritePixel
 *  write rle run/literal strings
 * Arguments:
 *  flag: TRUE if it is a run. otherwise it is a string
 *  count: number of run/literal (0 based)
 *  *buff: output area
 *  *tmp: source area
 *  pixelsize: pixel size (1-4)
 * Return:
 *  none
 */
static void 
WritePixel
(
 FxBool flag,
 FxU8 count,
 FxU8 *buff,
 FxU8 *tmp,
 FxU8 pixelsize
)
{
  FxU8 val;
  FxU32 i;

  if (flag) {
    val = 0x80 | count;
    count = 0;
  }
  else
    val = count;

  *buff = val;
  buff++;
  for (i = 0; i <= count; i++) {
    memcpy(buff, tmp, pixelsize);
    buff+=pixelsize;
    tmp+=4;
  }
}

/*
 * SimpleRleEncode
 *  simple rle encoder
 * Arguments:
 *  pixelcount : number of pixels
 *  pixelsize : 1-4
 *  *mem : source image
 *  *buff : compressed data
 * Return:
 *  none
 */
static FxU32 
SimpleRleEncode
(
 FxU32 pixelcount,   /* number of pixels */
 FxU8  pixelsize,    /* size of pixel (in bytes) 1-4 */
 FxU8  *mem,         /* src image */
 FxU8  *buff         /* compressed data */
)
{  
  FxU8    *src = buff;
  FxU8    run = 0, lit = 0;
  FxU32   tmp[130];
  FxU32   pval = 0, cval = 0;
  FxU32   i;
  FxU32   tt;
  FxBool  flag = FXFALSE;

  /* determine run or literal */
  mem+=pixelsize;

  for (i = 1; i < pixelcount; i++) {
    if (i > 300000)
      tt = 1;
    memcpy(&pval, mem-pixelsize, pixelsize);
    memcpy(&cval, mem, pixelsize);
    if (cval == pval) {
      flag = FXFALSE;
      if (lit) {
        WritePixel(FXFALSE, (FxU8)(lit - 1), buff, (FxU8 *)&tmp[0], pixelsize);
        buff += (1 + pixelsize * lit);
        lit = 0;
      }
      run++;
      if (run == 128) {
        WritePixel(FXTRUE, (FxU8)(run - 1), buff, (FxU8 *)&tmp[0], pixelsize);
        buff += (1 + pixelsize);
        run -= 128;
      }
      memcpy(&tmp[0], mem, pixelsize);
    }
    else {
      flag = FXTRUE;
      if (run) {
        WritePixel(FXTRUE, run, buff, (FxU8 *)&tmp[0], pixelsize);
        buff += (1 + pixelsize);
        run = 0;
      }
      else {
        if (lit == 128) {
          WritePixel(FXFALSE, (FxU8)(lit - 1), buff, (FxU8 *)&tmp[0], pixelsize);
          buff += (1 + pixelsize * lit);
          lit -= 128;
        }
        memcpy(&tmp[lit], mem-pixelsize, pixelsize);
        lit++;
      }
    }
    mem+=pixelsize;
  }
  /* last pixel */
  if (flag) {
    if (lit == 128) {
      WritePixel(FXFALSE, (FxU8)(lit - 1), buff, (FxU8 *)&tmp[0], pixelsize);
      buff += (1 + pixelsize * lit);
      lit -= 128;
    }
    memcpy(&tmp[lit], mem-pixelsize, pixelsize);
    WritePixel(FXFALSE, (FxU8)lit, buff, (FxU8 *)&tmp[0], pixelsize);
    lit++;
    buff += (1 + pixelsize * lit);
    lit = 0;
  }
  else if (run) {
    WritePixel(FXTRUE, run, buff, (FxU8 *)&tmp[0], pixelsize);
    buff += (1 + pixelsize);
    run = 0;
  }
  
  return( (FxU32) (buff - src));
}

/*-------------------------------------------------------------------
  Function: tlScreenDump
  Date: 6/6/97
  Implementor(s):
  Library: test library
  Description:
   dump the lfb data
  Arguments:
   filename - filename
   width    - width for frame buffer
   height   - height for frame buffer
  Return:
   none
  -------------------------------------------------------------------*/
FxBool
tlScreenDump
(
 const char *filename,
 FxU16 width, 
 FxU16 height
)
{
  FILE *fp;
  FxU16 *pixel, *region;
  FxU8  *buff;
  FxU32 count, signature;
  FxU8  type, depth;

  fp = fopen(filename, "wb");
  if (fp == NULL)
    return(FXFALSE);

  region = malloc(width * height * sizeof(FxU16));
  buff = malloc(width * height * sizeof(FxU16) * 2);
  grLfbReadRegion( GR_BUFFER_FRONTBUFFER,
                   0, 0, width, height,
                   width*2, region );

  pixel = (FxU16 *)region;

  count = SimpleRleEncode( (FxU32)width * (FxU32)height, 2, (FxU8 *)region, (FxU8 *)buff);

  /* header of the file */
  type = LFB_DATA_RLE;
  depth = 16;
  signature = IMAGE_SRLE;
  fwrite(&signature, sizeof(FxU32), 1, fp);
  fwrite(&width, sizeof(FxU16), 1, fp);
  fwrite(&height, sizeof(FxU16), 1, fp);
  fwrite(&depth, sizeof(FxU8), 1, fp);
  fwrite(&type, sizeof(FxU8), 1, fp);

  /* LFB data */
  fwrite(buff, count, 1, fp);

  free(buff);
  free(region);
  fclose(fp);

  return FXTRUE;
}

#ifdef __linux__
#include <linutil.h>

/*-------------------------------------------------------------------
  Function: tlKbHit
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Returns true if there are pending characters in the input queue
  Arguments:
  none
  Return:
  nonzero if keys in queue
  -------------------------------------------------------------------*/
int  tlKbHit( void ) {
  return lin_kbhit();
}

/*-------------------------------------------------------------------
  Function: tlGetCH
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Returns character from top of input fifo, blocks if fifo is empty
  Arguments:
  none
  Return:
  character
  -------------------------------------------------------------------*/
char tlGetCH( void ) {
  return lin_getch();
}

FxBool
tlErrorMessage(const char *err) {
  fprintf(stderr, "%s", err);
  return FXFALSE;
} /* tlErrorMessage */

#else
#ifdef  __DOS32__
/*-------------------------------------------------------------------
  Function: tlKbHit
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Returns true if there are pending characters in the input queue
  Arguments:
  none
  Return:
  nonzero if keys in queue
  -------------------------------------------------------------------*/
int  tlKbHit( void ) {
    return kbhit();
}

/*-------------------------------------------------------------------
  Function: tlGetCH
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Returns character from top of input fifo, blocks if fifo is empty
  Arguments:
  none
  Return:
  character
  -------------------------------------------------------------------*/
char tlGetCH( void ) {
    return getch();
}

FxBool
tlErrorMessage(const char *err) {
  fprintf(stderr, "%s", err);
  return FXFALSE;
} /* tlErrorMessage */

#else   /* __WIN32__ */


/* This segment simulates main() for Windows, creates a window, etc. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Forward declarations */
HWND hWndMain;
char ** commandLineToArgv(LPSTR lpCmdLine, int *pArgc);

/*
 * MainWndproc
 *
 * Callback for all Windows messages
 */
static int qhead = 0;
static int qtail = 0;
static int queue[256] = {0};

long FAR PASCAL 
MainWndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  PAINTSTRUCT ps;
  HDC         hdc;
  
  extern GrHwConfiguration hwconfig;

  switch( message )
    {
    case WM_SETCURSOR:
      if (Res != GR_RESOLUTION_NONE) {
        SetCursor(NULL);
        return 0;
      }
      break;

    case WM_CREATE:
      break;

    case WM_PAINT:
      hdc = BeginPaint( hWnd, &ps );
      EndPaint( hWnd, &ps );
      return 1;

    case WM_CLOSE:
      queue[qhead++] = 'q'; qhead &= 255;
      break;

    case WM_DESTROY:
      break;

    case WM_MOVE:
      if (!grSstControl(GR_CONTROL_MOVE)) {
        PostMessage( hWndMain, WM_CLOSE, 0, 0 );
        return 0;
      }
      break;

    case WM_ACTIVATE:
      if (hwconfig.SSTs[0].type == GR_SSTTYPE_VOODOO) {
        if (wParam & WA_INACTIVE) 
          grSstControl(GR_CONTROL_DEACTIVATE);
        else {
          grSstControl(GR_CONTROL_ACTIVATE);
#if 0
          grHints(GR_HINT_H3DENABLE,1);
#endif
        }
      } else
        return 0;      
      break;

    case WM_DISPLAYCHANGE:
    case WM_SIZE:
      {
        extern void getWindowSize(float *width, float *height);
        float  width, height;
          
        getWindowSize(&width, &height);
#ifndef H3D
        tlSetScreen( width, height);
#endif
      }
      if (!grSstControl(GR_CONTROL_RESIZE)) {
        MessageBox( hWnd, "Resize failed due to lack of sufficient buffer memory.\n", "Allocation Failure", MB_OK | MB_APPLMODAL );
        PostMessage( hWndMain, WM_CLOSE, 0, 0 );
        okToRender = FXFALSE; 
        return 0;
      } else {
        okToRender = FXTRUE;
      }
      break;

    case WM_CHAR:
      if (!isascii(wParam)) break;
#if 0
      printf("Posting keystroke %.02x\n", wParam);
      fflush(stdout);
#endif
      /* queue[qhead++] = tolower(wParam);  */
      queue[qhead++] = wParam;
      qhead &= 255;
      break;

    default:
      break;
    }
  return DefWindowProc(hWnd, message, wParam, lParam);
  
} /* MainWndproc */

/*
 * initApplication
 *
 * Do that Windows initialization stuff...
 */
static FxBool
initApplication( HANDLE hInstance, int nCmdShow )
{
  WNDCLASS    wc;
  FxBool        rc;
  
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = MainWndproc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon( NULL, IDI_APPLICATION);    /* generic icon */
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hbrBackground = GetStockObject( BLACK_BRUSH );
  wc.lpszMenuName =  NULL;
  wc.lpszClassName = "WinGlideClass";
  rc = RegisterClass( &wc );
  if( !rc ) {
    return FALSE;
  }
  
  hWndMain =
    CreateWindowEx(
#if 0                         
                   WS_EX_APPWINDOW  gives you regular borders?
                   WS_EX_TOPMOST    Works as advertised.
#endif
                   WS_EX_APPWINDOW,
                   "WinGlideClass",
                   "Glide Test",
                   WS_OVERLAPPED |     
                   WS_CAPTION  |     
                   WS_THICKFRAME | 
                   WS_MAXIMIZEBOX | 
                   WS_MINIMIZEBOX | 
                   WS_VISIBLE | /* so we don't have to call ShowWindow */
                   WS_POPUP | /* non-app window */
                   WS_SYSMENU, /* so we get an icon in the tray */
                   CW_USEDEFAULT, 
                   CW_USEDEFAULT,
                   0x110, /* GetSystemMetrics(SM_CXSCREEN), */
                   0x120, /* GetSystemMetrics(SM_CYSCREEN), */
                   NULL,
                   NULL,
                   hInstance,
                   NULL
                   );
  
  if( !hWndMain ) {
    return FALSE;
  }
  
  SetCursor(NULL);
  
  ShowWindow( hWndMain, SW_NORMAL);
  UpdateWindow( hWndMain );
  
  return TRUE;
  
} /* initApplication */

/*
 * WinMain
 */
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, int nCmdShow )
{
  if( !initApplication(hInstance, nCmdShow) )
    return FALSE;
  
  {
    /* 
     * Since printfs go into the bit bucket on Win32,
     * put up a message in the window.
     */
    HDC hDC = GetDC(hWndMain);
    char *message = "Press any Key to continue!";
    RECT rect;

    GetClientRect(hWndMain, &rect);
    SetTextColor(hDC, RGB(0, 255, 255));
    SetBkColor(hDC, RGB(0, 0, 0));
    SetTextAlign(hDC, TA_CENTER);
    ExtTextOut(hDC, rect.right/2, rect.bottom/2, ETO_OPAQUE, &rect, 
               message, strlen(message), NULL);
    ReleaseDC(hWndMain, hDC);
    GdiFlush();
  }
  
  {
    int     argc;
    char    **argv;
    extern int main(int argc, char **argv);

    argv = commandLineToArgv(lpCmdLine, &argc);
    main(argc, argv);
  }
  
  printf("Exiting winMain()\n");
  fflush(stdout);
  
  DestroyWindow(hWndMain);
  return 0;
  
} /* WinMain */

FxBool
tlErrorMessage(const char *err)
{
  /* make the cursor visible */
  SetCursor(LoadCursor( NULL, IDC_ARROW ));
  
  /*
   ** warn user if there is one 
   */
  printf("Error %s..\n", err);
  fflush(stdout);
  
  MessageBox( hWndMain, err, "ERROR", MB_OK );
  return FXFALSE;
} /* tlErrorMessage */

/*
 * Converts lpCmdLine to WinMain into argc, argv
 */
static char    *fakeName = "WinTest";
static char    *argvbuf[32];
static char    cmdLineBuffer[1024];
char **
commandLineToArgv(LPSTR lpCmdLine, int *pArgc)
{
  char    *p, *pEnd;
  int     argc = 0;
  
  argvbuf[argc++] = fakeName;
  
  if (lpCmdLine == NULL) {
    *pArgc = argc;
    return argvbuf;
  }
  
  strcpy(cmdLineBuffer, lpCmdLine);
  p = cmdLineBuffer;
  pEnd = p + strlen(cmdLineBuffer);
  if (pEnd >= &cmdLineBuffer[1022]) pEnd = &cmdLineBuffer[1022];
  
  fflush(stdout);
  
  while (1) {
    /* skip over white space */
    fflush(stdout);

    while (*p == ' ') p++;
    if (p >= pEnd) break;

    argvbuf[argc++] = p;
    if (argc >= 32) break;

    /* skip till there's a 0 or a white space */
    while (*p && (*p != ' ')) p++;

    if (*p == ' ') *p++ = 0;
  }
  
  *pArgc = argc;
  return argvbuf;
}

/*-------------------------------------------------------------------
  Function: tlKbHit
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Returns true if there are pending characters in the input queue
  Arguments:
  none
  Return:
  nonzero if keys in queue
  -------------------------------------------------------------------*/
int
tlKbHit( void ) 
{
  MSG msg;
  
  if (qhead != qtail) {
    return 1;
  }
  
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);      /* this might change qhead */
    if (qhead != qtail) {
      return 1;
    }
  }
  return 0;
}

/*-------------------------------------------------------------------
  Function: tlGetCH
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Returns character from top of input fifo, blocks if fifo is empty
  Arguments:
  none
  Return:
  character
  -------------------------------------------------------------------*/
char
tlGetCH( void ) 
{
  MSG     msg;
  char    rv;
  
  if (qtail != qhead) {
    rv = queue[qtail++];
    qtail &= 255;
    return rv;
  }
  
  while (GetMessage( &msg, NULL, 0, 0 )) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    if (qtail != qhead) {
      rv = queue[qtail++];
      qtail &= 255;
      return rv;
    }
  }
  
  /* Should never get here!! */
  /* printf("Bad exit..\n"); */
  /* fflush(stdout); */
}

void
tlExit()
{
  PostMessage( hWndMain, WM_CLOSE, 0, 0 );
}


void 
getWindowSize(float *width, float *height)
{
  RECT    rect;
  
  if (fullScreen) {
    GetWindowRect(hWndMain, &rect);
    *width = (float) (rect.right-rect.left);
    *height = (float) (rect.bottom-rect.top);
  }
  else {
    GetClientRect(hWndMain, &rect);
    *width = (float) (rect.right-rect.left);
    *height = (float) (rect.bottom-rect.top);
  }
}


#endif  /* __DOS32__ */
#endif /* __linux__ */
