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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <glide.h>

#ifdef __unix__
#include <limits.h>
#define _MAX_PATH PATH_MAX
unsigned long hWndMain;
#endif

#ifdef __MWERKS__
#include <sioux.h>
#include <Types.h>
#include <QuickDraw.h>
#include <Windows.h>
#endif

#ifdef __WATCOMC__
#include <conio.h>
#endif
#ifdef __DJGPP__
#include <conio.h>
#include <pc.h>
#include <crt0.h>
int crt0_startup_flags = _CRT0_FLAG_NONMOVE_SBRK;
#endif

#include "tlib.h"

#ifndef __TXS_LOAD_HEADER__
#define __TXS_LOAD_HEADER__

#define TXS_COOKIE             "TXSF"
#define TXS_COOKIE_SIZE        5
#define TXS_VERSION            1.0f
#define TXS_READ_NUM_FIELDS    7
#define TXS_WRITE_OFFSET_SIZE  9

#ifndef GR_TEXFMT_P_8_6666
#define GR_TEXFMT_P_8_6666 GR_TEXFMT_RSVD0
#elif GR_TEXFMT_P_8_6666 != GR_TEXFMT_RSVD0
#       warning GR_TEXFMT_P_8_6666 is not defined as expected
#endif

/* TXS file header
   
   New fields can be added at any time, but they should ALWAYS be added after the data_offset
   field.  This keeps backwards compatibility in place if the new fields aren't mandatory.
   
   Field Definitions:
   
   cookie                       - used to identify a true TXS file
   version                      - TXS version for the file.  For backwards compatible changes (i.e. the file
                                  can still be loaded and used by earlier versions of TXS/Texus ), only the
                                  minor revision number should be changed.  For non-backwards compatible
                                  changes, the major revision number must be changed.
   format                       - texture format of the data
   width                        - width of the texture
   height                       - height of the texture
   mipmap_levels        - number of mipmap levels contained in the data
   data_offset          - offset in bytes from the start of the file to the actual texture data
*/

typedef struct
{
  char  cookie[TXS_COOKIE_SIZE];
  float version;
  FxU16 format;
  FxU16 width;
  FxU16 height;
  FxU16 mipmap_levels;
  FxU32 data_offset;    /* in bytes */
} TXSHeader;

typedef struct
{
  float version;
  FxU16 format;
  FxU16 width;
  FxU16 height;
  FxU16 mipmap_levels;
  FxU32 data_size;
  void  *table;
  void  *data;
} TXSInfo;

#endif /* #ifndef __TXS_LOAD_HEADER__ */


static struct fsContextEntry {
  GrContext_t context;
  FxU32 hWnd;
  GrScreenResolution_t res;
  GrScreenRefresh_t refresh;
  GrColorFormat_t colorFmt;
  GrOriginLocation_t origin;
  int nColBuf, nAuxBuf;
} fsContextList[kMaxGlideContext];
static FxBool fullScreen = FXTRUE;
static FxBool okToRender = FXTRUE;
static void *state = NULL;
static void *vlstate = NULL;

FxBool tlOkToRender(void)
{
  return okToRender;
} /* tlOkToRender */

GrContext_t*
tlCheckRenderContext(FxU32 glideDeviceNum)
{
  return ((fsContextList[glideDeviceNum].context == 0x00UL) 
          ? NULL
          : &fsContextList[glideDeviceNum].context);
} /* tlCheckRenderContext */

GrContext_t*
tlGetRenderContext(FxU32 glideDeviceNum, 
                   FxU32 hWnd, 
                   GrScreenResolution_t res, GrScreenRefresh_t refresh,
                   GrColorFormat_t colorFmt,
                   GrOriginLocation_t origin,
                   int nColBuf, int nAuxBuf)
{
  GrContext_t
    *retVal = NULL,
    glideContext;

  grSstSelect(glideDeviceNum);
  glideContext = grSstWinOpen(hWnd,
                              res, refresh,
                              colorFmt,
                              origin,
                              nColBuf, nAuxBuf);
  if (glideContext != 0x00UL) {
    struct fsContextEntry*
      curContext = fsContextList + glideDeviceNum;

    curContext->hWnd     = hWnd;
    curContext->res      = res;
    curContext->refresh  = refresh;
    curContext->colorFmt = colorFmt;
    curContext->origin   = origin;
    curContext->nColBuf  = nColBuf;
    curContext->nAuxBuf  = nAuxBuf;

    curContext->context  = glideContext;
    retVal = &curContext->context;
  }

  return retVal;
} /* tlSetRenderContext */

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
              char **remArgs[] )
{
    static int firstCall;
    static int lastArg;
    
    int rv = 0;

    if ( !firstCall ) {
      lastArg   = 1;
      firstCall = 1;
      

#if __MWERKS__
      SIOUXSettings.standalone       = true;
      SIOUXSettings.setupmenus       = false;
      SIOUXSettings.autocloseonquit  = true;
      SIOUXSettings.asktosaveonclose = false;
#endif        
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
  { "320x200",   GR_RESOLUTION_320x200,    320.f,   200.f }, /*  0x0 */
  { "320x240",   GR_RESOLUTION_320x240,    320.f,   240.f }, /*  0x1 */
  { "400x256",   GR_RESOLUTION_400x256,    400.f,   256.f }, /*  0x2 */
  { "512x384",   GR_RESOLUTION_512x384,    512.f,   384.f }, /*  0x3 */
  { "640x200",   GR_RESOLUTION_640x200,    640.f,   200.f }, /*  0x4 */
  { "640x350",   GR_RESOLUTION_640x350,    640.f,   350.f }, /*  0x5 */
  { "640x400",   GR_RESOLUTION_640x400,    640.f,   400.f }, /*  0x6 */
  { "640x480",   GR_RESOLUTION_640x480,    640.f,   480.f }, /*  0x7 */
  { "800x600",   GR_RESOLUTION_800x600,    800.f,   600.f }, /*  0x8 */
  { "960x720",   GR_RESOLUTION_960x720,    960.f,   720.f }, /*  0x9 */
  { "856x480",   GR_RESOLUTION_856x480,    856.f,   480.f }, /*  0xA */
  { "512x256",   GR_RESOLUTION_512x256,    512.f,   256.f }, /*  0xB */
  { "1024x768",  GR_RESOLUTION_1024x768,  1024.f,   768.f }, /*  0xC */
  { "1280x1024", GR_RESOLUTION_1280x1024, 1280.f,  1024.f }, /*  0xD */
  { "1600x1200", GR_RESOLUTION_1600x1200, 1600.f,  1200.f }, /*  0xE */
  { "400x300",   GR_RESOLUTION_400x300,    400.f,   300.f }, /*  0xF */
  { "1152x864",  GR_RESOLUTION_1152x864,  1152.f,   864.f }, /* 0x10 */
  { "1280x960",  GR_RESOLUTION_1280x960,  1280.f,   960.f }, /* 0x11 */
  { "1600x1024", GR_RESOLUTION_1600x1024, 1600.f,  1024.f }, /* 0x12 */
  { "1792x1344", GR_RESOLUTION_1792x1344, 1792.f,  1344.f }, /* 0x13 */
  { "1856x1392", GR_RESOLUTION_1856x1392, 1856.f,  1392.f }, /* 0x14 */
  { "1920x1440", GR_RESOLUTION_1920x1440, 1920.f,  1440.f }, /* 0x15 */
  { "2048x1536", GR_RESOLUTION_2048x1536, 2048.f,  1536.f }, /* 0x16 */
  { "2048x2048", GR_RESOLUTION_2048x2048, 2048.f,  204.f }   /* 0x17 */
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
const char *tlGetResolutionString( int res )
{
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
const char *tlGetResolutionList( void )
{
    char *list = calloc( sizeof( char ), 256 );
    int member;
    strcat( list, resTable[0].id );
    for( member = 1; member < resTableSize; member++ ) {
        strcat( list, " | " );
        strcat( list, resTable[member].id );
    }
    return list;
}

typedef struct {
    const char *name;
    int         type;
} tlPixelFormat;

/* FIX ME!!! Need to update pixel format from glide.h */
static tlPixelFormat pfTable[] = {
  { "i8",          0x01 },
  { "ai88",        0x02 },
  { "rgb565",      0x03 },
  { "argb1555",    0x04 },
  { "argb8888",    0x05 },
  { "aa2rgb565",   0x06 },
  { "aa2argb1555", 0x07 },
  { "aa2argb8888", 0x08 },
  { "aa4rgb565",   0x09 },
  { "aa4argb1555", 0x0a },
  { "aa4argb8888", 0x0b }
};

/*-------------------------------------------------------------------
  Function: tlGetPixelFormat
  Date: 1/26/98
  Implementor(s): atai
  Library: Test Library
  Description:
  Arguments:
  Return:
  none
  -------------------------------------------------------------------*/
int tlGetPixelFormat( const char *pf )
{
  int i;
  for (i = 0; i < (int)(sizeof(pfTable)/sizeof(tlPixelFormat)); i++) {
    if ( !strcmp( pf, pfTable[i].name ) ) {
      return pfTable[i].type;
    }
  }
  /* FIX ME!!! Need to update pixel format from glide.h */
  return 0x03;
}

/*-------------------------------------------------------------------
  Function: tlVertexLayout
  Date: 1/26/98
  Implementor(s): atai
  Library: Test Library
  Description:
  Set up default tlVertexLayout
  Arguments:
  Return:
  none
  -------------------------------------------------------------------*/
void tlVertexLayout( void )
{
  grVertexLayout(GR_PARAM_XY,  GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_A,   GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Z,   GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q0,  GR_VERTEX_OOW_TMU0_OFFSET << 2, GR_PARAM_DISABLE);
  grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);
  grVertexLayout(GR_PARAM_Q1,  GR_VERTEX_OOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);
  return;
}

/*-------------------------------------------------------------------
  Function: tlVoodooType
  Date: 6/02/98
  Implementor(s): atai
  Library: Test Library
  Description:
  return platform type
  Arguments:
  Return:
  none
  -------------------------------------------------------------------*/
int tlVoodooType( void )
{
  const char *vstr;
  int vtype = TL_VOODOO_UNDEF;

  vstr = grGetString(GR_HARDWARE);
  if (!strcmp(vstr, "Voodoo2"))
    vtype = TL_VOODOO2;
  else if (!strcmp(vstr, "Voodoo Banshee"))
    vtype = TL_VOODOOBANSHEE;
  else if (!strcmp(vstr, "Voodoo Graphics"))
    vtype = TL_VOODOO;
  else if (!strcmp(vstr, "Voodoo Rush"))
    vtype = TL_VOODOORUSH;
  return vtype;
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

void tlSetScreen( float width, float height )
{
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
float tlScaleX( float coord )
{
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
float tlScaleY( float coord )
{
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
static GrTexInfo fontInfo;
static unsigned long fontAddress;
static FxBool usePointSampling = FXFALSE;

#ifdef TLIB_SMALL_FONTS
/* smaller sized tlib font with all printable characters */
static const char fontString[] = " !\"#$%&'()*+,-./"
                                 "0123456789:;<=>?"
                                 "@ABCDEFGHIJKLMNO"
                                 "PQRSTUVWXYZ[\\]^_"
                                 "`abcdefghijklmno"
                                 "pqrstuvwxyz{|}~\177";

static const int fontWidth    = 7;
static const int fontWStride  = 8;
static const int fontHeight   = 10;
static const int fontHStride  = 10;
static const int charsPerLine = 16;

#include "tlsmdata.inc"
#else
/* normal tlib font */
static const char fontString[] = "ABCDEFGHIJKLMN"
                                 "OPQRSTUVWXYZ01"
                                 "23456789.,;:*-"
                                 "+/_()<>|[]{}! ";

static const int fontWidth    = 9;
static const int fontWStride  = 9;
static const int fontHeight   = 12;
static const int fontHStride  = 12;
static const int charsPerLine = 14;
#include "tldata.inc"
#endif

static int fontInitialized;

#if 0 /* not used */
static void grabTex( FxU32 addr, void *storage );
static void putTex( FxU32 addr, void *storage );
#endif
static void consoleScroll( void );
static int drawChar( char character,
                     float x, float y,
                     float xOffset, float yOffset,
                     float w, float h );

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
               int color )
{
    int entry;
    char xCoord;
    char yCoord;

    if (state == NULL) {
      FxI32 state_size;
      grGet(GR_GLIDE_STATE_SIZE, 4, &state_size);
      state = malloc(state_size);        
    }
    if (vlstate == NULL) {
      FxI32 state_size;
      grGet(GR_GLIDE_VERTEXLAYOUT_SIZE, 4, &state_size);
      vlstate = malloc(state_size);        
    }
    
    fontInfo.smallLodLog2    = GR_LOD_LOG2_128;
    fontInfo.largeLodLog2    = GR_LOD_LOG2_128;
    fontInfo.aspectRatioLog2 = GR_ASPECT_LOG2_2x1;
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
            
            /* we need to multiply times 2 since Glide wants texture
             * coords in the range of 0..255 and our font texture is
             * only 128 wide */
            xCoord = ( offset % charsPerLine )  * 2*fontWStride;
            yCoord = ( offset / charsPerLine )  * 2*fontHStride; 

            fontTable[entry][0] = xCoord;
            fontTable[entry][1] = yCoord;
        }
    }
    
    /* use maximum number of cols/rows possible */
    if (columns <= 0)
      columns = (int)((maxX - minX) * scrXScale / fontWidth);
    if (rows <= 0)
      rows = (int)((maxY - minY) * scrYScale / fontHeight);

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
#ifdef TLIB_SMALL_FONTS
    if ((consoleCharWidth < 1.1f * fontWidth / scrXScale) &&
        (consoleCharHeight < 1.1f * fontHeight / scrYScale)) {
      usePointSampling = FXTRUE;
      consoleCharWidth = fontWidth / scrXScale;
      consoleCharHeight = fontHeight / scrYScale;
    }
#endif

    fontAddress = grTexMaxAddress( 0 ) - 
      grTexCalcMemRequired( fontInfo.smallLodLog2, fontInfo.largeLodLog2,
                            fontInfo.aspectRatioLog2, fontInfo.format );

    fontInitialized = 1;

    return; 
}

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
int tlConOutput( const char *fmt, ... )
{
    int rv = 0;
    va_list argptr;

    if( fontInitialized ) {
        static char buffer[1024];
        const char *c;

        va_start( argptr, fmt );
        rv = vsprintf( buffer, fmt, argptr );
        va_end( argptr );

#ifndef TLIB_SMALL_FONTS
        {
          char* temp = buffer;

          while(*temp != '\0') {
            if (*temp >= 'a' && *temp <= 'z')
                *temp -= ('a'-'A');
            temp++;
          }
        }
#endif

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
void tlConClear()
{
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
void tlConRender( void )
{
    int retDrawChar;

    if( fontInitialized ) {
        int x, y;
        
        grGlideGetState( state );
        grGlideGetVertexLayout( vlstate );

        grVertexLayout(GR_PARAM_XY,  0, GR_PARAM_ENABLE);
        grVertexLayout(GR_PARAM_Q,   GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
        grVertexLayout(GR_PARAM_Q0,  GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
        grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);

        grCoordinateSpace(GR_WINDOW_COORDS);
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
        if (usePointSampling) {
          grTexFilterMode( GR_TMU0,
                           GR_TEXTUREFILTER_POINT_SAMPLED,
                           GR_TEXTUREFILTER_POINT_SAMPLED);
        } else {
          grTexFilterMode( GR_TMU0,
                           GR_TEXTUREFILTER_BILINEAR,
                           GR_TEXTUREFILTER_BILINEAR );
        }
        grTexMipMapMode( GR_TMU0,
                         GR_MIPMAP_DISABLE,
                         FXFALSE );
        grTexMultibase(GR_TMU0, FXFALSE);
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

#ifdef TLIB_SMALL_FONTS
        grConstantColorValue( 0 );
        for( y = 0; y < consoleRows; y++ ) {
            float charX = consoleOriginX;
            float charY = consoleOriginY+(consoleCharHeight*y);
            for( x = 0; x < consoleColumns; x++ ) {
                retDrawChar = drawChar( consoleGrid[(y*consoleColumns)+x],
                                        charX, charY, 1.0f, 0.0f,
                                        consoleCharWidth, 
                                        consoleCharHeight );
                if (retDrawChar == 1) {
                  charX += consoleCharWidth;
                } else if (retDrawChar == -1) {
                  charX -= consoleCharWidth;
                  if (charX < 0.0f)
                    charX = 0.0f;
                }
            }
        }
#endif

        grConstantColorValue( consoleColor );
        for( y = 0; y < consoleRows; y++ ) {
            float charX = consoleOriginX;
            float charY = consoleOriginY+(consoleCharHeight*y);
            for( x = 0; x < consoleColumns; x++ ) {
                retDrawChar = drawChar( consoleGrid[(y*consoleColumns)+x],
                                        charX, charY, 0.0f, 0.0f,
                                        consoleCharWidth, 
                                        consoleCharHeight );
                if (retDrawChar == 1) {
                  charX += consoleCharWidth;
                } else if (retDrawChar == -1) {
                  charX -= consoleCharWidth;
                  if (charX < 0.0f)
                    charX = 0.0f;
                }
            }
        }

        grGlideSetState(state);
        grGlideSetVertexLayout( vlstate );
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
void tlSleep( int seconds )
{
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

const float *tlIdentity( void )
{
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
const float *tlXRotation( float degrees )
{
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
const float *tlYRotation( float degrees )
{
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
const float *tlZRotation( float degrees )
{
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
const float *tlTranslation( float x, float y, float z )
{
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
void tlSetMatrix( const float *m )
{
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
void tlMultMatrix( const float *m )
{
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
void tlTransformVertices( TlVertex3D *dstVerts, const TlVertex3D *srcVerts, unsigned length )
{
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

void tlProjectVertices( TlVertex3D *dstVerts, const TlVertex3D *srcVerts, unsigned length )
{
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
  Function: tlCProjectVertices
  Date: 06-Feb-98
  Implementor(s): atai
  Library: Test Library
  Description:
  perspective project a set of vertices only.
  Arguments:
  dstVerts - memory to store projected vertices
  srcVerts - array of vertices to be transformed
  length - number of vertices to transform
  Return:
  none
  -------------------------------------------------------------------*/
void tlCProjectVertices( TlVertex3D *dstVerts, const TlVertex3D *srcVerts, unsigned length )
{
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

int _tlLoad3df( const char *filename, 
                GrTexInfo *info, 
                GrTexTable_t *tableType,
                void *table )
{
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
            info->smallLodLog2    = tdfInfo.header.small_lod;
            info->largeLodLog2    = tdfInfo.header.large_lod;
            info->aspectRatioLog2 = tdfInfo.header.aspect_ratio;
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


/*
 * TXS utility functions
 * These read functions will be a handy abstraction when we
 * put the endian flag into the file format.
 * Putting in the flag will require revisions to texus2 and
 * the Photoshop TXS plugin.
 */
FxBool _Read16 (FILE *stream, FxU16* data)
{
  if (fread (data, 2, 1, stream) != 1){
    return FXFALSE;
  }
  return FXTRUE;
}

/* Read long word, msb first */
FxBool _Read32 (FILE *stream, FxU32* data)
{
  if (fread (data, 4, 1, stream) != 1){
    return FXFALSE;
  }
  return FXTRUE;
}

int _tlLoadTXS( const char   *filename, 
                GrTexInfo    *grinfo, 
                GrTexTable_t *tableType,
                void         *table )
{
  FILE    *stream;
  TXSInfo info;
  FxU32   data_offset;
  char    cookie[TXS_COOKIE_SIZE];

  if(!filename || !grinfo || ! tableType || !table)
    return FXFALSE;

  stream = fopen(filename, "rb");

  if(stream==NULL) return FXFALSE;

  /* Read the full header */
  if ( fscanf ( stream, "%4s %f %hu %hu %hu %hu %8x", 
                cookie, 
                &info.version,
                &info.format, 
                &info.width, 
                &info.height, 
                &info.mipmap_levels,
                &data_offset ) 
       != TXS_READ_NUM_FIELDS )
    return FXFALSE;

#if DEBUG
  printf("TXSLOAD: cookie:%s version:%f format:%d\nw:%d h:%d m:%d o:%8x\n",
         cookie, 
         info.version,
         info.format, 
         info.width, 
         info.height, 
         info.mipmap_levels,
         data_offset);
#endif

  /* If the cookie doesn't match, then reject the file */
  if ( strcmp (cookie, TXS_COOKIE) ) return FXFALSE;

  /* If the major version has increased, 
   * then we can't be sure we can load this
   * file so reject it.
   */
  if ((int) info.version > (int)TXS_VERSION)  return FXFALSE;

  /* Advance to the texture data in the file */
  if ( fseek( stream, data_offset, SEEK_SET ) ) return FXFALSE;

  /* Calculate the size of the texture data  */
  {
    int   i, w, h, bpp;

    w=info.width;
    h=info.height;
    info.data_size = 0;

    switch ( info.format ) {
#if NAPALM_TEXTURES
    case GR_TEXFMT_ARGB_CMP_FXT1:
    case GR_TEXFMT_ARGB_CMP_DXT1:        
      bpp=4; break;
#endif

    case GR_TEXFMT_RGB_332:
    case GR_TEXFMT_YIQ_422:
    case GR_TEXFMT_ALPHA_8:
    case GR_TEXFMT_INTENSITY_8:
    case GR_TEXFMT_ALPHA_INTENSITY_44:
    case GR_TEXFMT_P_8:
    case GR_TEXFMT_P_8_6666:
#if NAPALM_TEXTURES
    case GR_TEXFMT_ARGB_CMP_DXT2:
    case GR_TEXFMT_ARGB_CMP_DXT3:
    case GR_TEXFMT_ARGB_CMP_DXT4:
    case GR_TEXFMT_ARGB_CMP_DXT5:       
#endif
      bpp=8; break;

    case GR_TEXFMT_ARGB_8332:
    case GR_TEXFMT_AYIQ_8422:
    case GR_TEXFMT_RGB_565:
    case GR_TEXFMT_ARGB_1555:
    case GR_TEXFMT_ARGB_4444:
    case GR_TEXFMT_ALPHA_INTENSITY_88:
    case GR_TEXFMT_AP_88:
#if RAMPAGE_TEXTURES
    case GR_TEXFMT_YUYV_422:
    case GR_TEXFMT_UYVY_422:   
#endif
      bpp=16; break;

#if NAPALM_TEXTURES
    case GR_TEXFMT_ARGB_8888:
#if RAMPAGE_TEXTURES
    case GR_TEXFMT_AYUV_444:   
#endif /* RAMPAGE */
      bpp=32; break;
#endif /* NAPALM */

    default:
      /* invalid texel format */
#if DEBUG
      printf("invalid texel format\n");
#endif
      return FXFALSE;
    }

    for(i=0;i<info.mipmap_levels;i++) {
      switch ( info.format ) {
#if NAPALM_TEXTURES
      case GR_TEXFMT_ARGB_CMP_FXT1:
        w = (w + 0x7 ) & ~0x7;
        h = (h + 0x3 ) & ~0x3;
        break;
      case GR_TEXFMT_ARGB_CMP_DXT1:
      case GR_TEXFMT_ARGB_CMP_DXT2:
      case GR_TEXFMT_ARGB_CMP_DXT3:
      case GR_TEXFMT_ARGB_CMP_DXT4:
      case GR_TEXFMT_ARGB_CMP_DXT5:
        w = (w + 0x3 ) & ~0x3;
        h = (h + 0x3 ) & ~0x3;
        break;
#endif
#if RAMPAGE_TEXTURES
      case GR_TEXFMT_YUYV_422:
      case GR_TEXFMT_UYVY_422:
        w = (w + 0x1 ) & ~0x1;
        break;
#endif
      default:
        break;
      }
      
      info.data_size+=(w*h*bpp)>>3;
      
      if(w>1)w>>=1;
      if(h>1)h>>=1;
    }
  }

  /* malloc needed memory */
  info.data=malloc(info.data_size);
  if(!info.data) return FXFALSE;

  /* read the data */

  /* First read NCC/palette tables 
   * straight into the user-provided table
   */
  if ((info.format == GR_TEXFMT_YIQ_422) ||
      (info.format == GR_TEXFMT_AYIQ_8422))
    {
      int i;
      for (i = 0; i < (int) sizeof(GuNccTable) >> 2; i++){
        if (!_Read32 (stream, &((FxU32 *)table)[i]))
          {
#if DEBUG
            printf("Unable to read NCC Table\n");
#endif      
            return FXFALSE;
          }
      }
    }
  else if ((info.format == GR_TEXFMT_P_8)  ||
           (info.format == GR_TEXFMT_AP_88)||
           (info.format == GR_TEXFMT_P_8_6666))
    {  
      FxU32 i;
      for (i = 0; i < 256; i ++) 
        {
          if (FXFALSE == _Read32 (stream, &((FxU32 *)table)[i]))
            {
#if DEBUG
              printf("Unable to read palette\n");
#endif
              return FXFALSE;
            }
        }
    }

  /* Next, read mipmap image data */
  switch (info.format)
    {
#if NAPALM_TEXTURES
      /* 4 bit, but treat as 8 bit for now */
    case GR_TEXFMT_ARGB_CMP_FXT1:
    case GR_TEXFMT_ARGB_CMP_DXT1:
#endif

    case GR_TEXFMT_RGB_332:
    case GR_TEXFMT_YIQ_422:
    case GR_TEXFMT_ALPHA_8:
    case GR_TEXFMT_INTENSITY_8:
    case GR_TEXFMT_ALPHA_INTENSITY_44:
    case GR_TEXFMT_P_8:
    case GR_TEXFMT_P_8_6666:
      /* 8 bit */
      if ((FxU32) info.data_size != 
          fread (info.data, 1, (FxU32) info.data_size, stream))
        {
#if DEBUG
          printf("Failed on 8 bit mipread\n");
#endif
          return FXFALSE;
        }
      break;

    case GR_TEXFMT_ARGB_8332:
    case GR_TEXFMT_AYIQ_8422:
    case GR_TEXFMT_RGB_565:
    case GR_TEXFMT_ARGB_1555:
    case GR_TEXFMT_ARGB_4444:
    case GR_TEXFMT_ALPHA_INTENSITY_88:
    case GR_TEXFMT_AP_88:
      /* 16 bit */
      {
        FxU32 i;
        FxU16* data = (FxU16 *) info.data;
        
        for (i = 0; i < (info.data_size >> 1); i++, data++)
          {
            if (FXFALSE == _Read16 (stream, data))
              {
#if DEBUG
                printf("Failed on 16 bit mipread\n");
#endif
                return FXFALSE;
              }
          }
      }
      break;

#if NAPALM_TEXTURES
    case GR_TEXFMT_ARGB_8888:
      /* 32 bit */
      {
        FxU32 i;
        FxU32* data = (FxU32 *) info.data;
        
        for (i = 0; i < (info.data_size >> 2); i++, data++)
          {
            if (FXFALSE == _Read32 (stream, data))
              {
#if DEBUG
                printf("Failed on 32 bit mipread\n");
#endif
                return FXFALSE;
              }
          }
      }
      break;
#endif /* NAPALM */

    default:
#if DEBUG
      printf("Failed on mipread\n");
#endif
      return FXFALSE;
    }

  /* fill in the GrTexInfo structure 
     Note that this assumes that the texture is a power of two --
     fails for NPOT.
   */
  {
    int i;
    FxU32 w2=info.width;
    FxU32 h2=info.height;
    FxU32 max;

    /* convert to log2 */
    i=0;
    while(w2>1){i++;w2>>=1;}
    w2=i;
    i=0;
    while(h2>1){i++;h2>>=1;}
    h2=i;

    max=(w2>h2)?w2:h2;

    grinfo->smallLodLog2=max-(info.mipmap_levels-1);
    grinfo->largeLodLog2=max;

    grinfo->aspectRatioLog2=w2-h2;
  }

  /* special case for GR_TEXFMT_P_8_6666 */
  if(info.format == GR_TEXFMT_P_8_6666)
    {
      grinfo->format=GR_TEXFMT_P_8;
      *tableType = GR_TEXTABLE_PALETTE_6666_EXT;
    }
  else
    {
      grinfo->format=info.format;
      *tableType = texTableType( info.format );
    }

  grinfo->data=info.data;

  return FXTRUE;
}

int tlLoadTexture( const char *filename, 
                   GrTexInfo *info, 
                   GrTexTable_t *tableType,
                   void *table )
{
  int i=0;
  char c=' ';

  /* find the end of the string (NULL==0) */
  while(c && i<_MAX_PATH) c=filename[i++];

  /* find the extension, 3df or txs */
  if(c==0)
    {
      i-=4;
      if(i<0) return 0;

      if((filename[i  ]=='T' || filename[i  ]=='t') &&
         (filename[i+1]=='X' || filename[i+1]=='x') &&
         (filename[i+2]=='S' || filename[i+2]=='s'))
        return _tlLoadTXS(filename,info,tableType, table);

      else if((filename[i  ]=='3') &&
              (filename[i+1]=='D' || filename[i+1]=='d') &&
              (filename[i+2]=='F' || filename[i+2]=='f'))
        return _tlLoad3df(filename,info,tableType, table);

      else
        return 0;
    }
  return 0;
}



/*--------------------------------------------------------------------
  Static Helpers
  --------------------------------------------------------------------*/

static void consoleScroll( void )
{
    memmove( consoleGrid, 
             consoleGrid + consoleColumns,
             (consoleRows-1)*consoleColumns );
    memset( consoleGrid+(consoleRows-1)*consoleColumns,
            32, 
            consoleColumns );
}

static int drawChar( char character,
                      float x, float y,
                      float xOffset, float yOffset,
                      float w, float h )
{
    GrVertex a, b, c, d;
    /* a---b
       |\  |    
       | \ |
       |  \|
       c---d */

    if (character == 32) /* space */
      return 1;
    if (character == 8)  /* backspace */
      return -1;
    if (character < 32)
      return 0;

    a.oow = b.oow = c.oow = d.oow = 1.0f;

    a.x = c.x = tlScaleX(x)+xOffset;
    a.y = b.y = tlScaleY(y)+yOffset;
    d.x = b.x = tlScaleX(x+w)+xOffset;
    d.y = c.y = tlScaleY(y+h)+yOffset;

#define TEXTURE_EPS 0.1f

    a.tmuvtx[0].sow = c.tmuvtx[0].sow = 
        (float)fontTable[(unsigned char)character][0] + TEXTURE_EPS;
    a.tmuvtx[0].tow = b.tmuvtx[0].tow = 
        (float)fontTable[(unsigned char)character][1] + TEXTURE_EPS;

    /* we need to multiply times 2 since Glide wants texture coords in
     * the range of 0..255 and our font texture is only 128 wide */
    d.tmuvtx[0].sow = b.tmuvtx[0].sow = 
        a.tmuvtx[0].sow + (float)(2*fontWidth) - 2.0f*TEXTURE_EPS;
    d.tmuvtx[0].tow = c.tmuvtx[0].tow = 
        a.tmuvtx[0].tow + (float)(2*fontHeight) - 2.0f*TEXTURE_EPS;

    if (character == '_') {
      /* remove gaps between adjacent underscores */
      a.tmuvtx[0].sow += 2.0f*1.0f;
      b.tmuvtx[0].sow -= 2.0f*1.0f;
      c.tmuvtx[0].sow += 2.0f*1.0f;
      d.tmuvtx[0].sow -= 2.0f*1.0f;
    }

    grDrawTriangle( &a, &d, &c );
    grDrawTriangle( &a, &b, &d );

    return 1;
}




#if 0 /* not used */
static void readRegion( void *data, 
                        int x, int y,
                        int w, int h );
static void writeRegion( void *data, 
                         int x, int y,
                         int w, int h );


static void putTex( FxU32 addr, void *storage )
{
    GrTexInfo texInfo;

    texInfo.smallLodLog2    = GR_LOD_LOG2_256;
    texInfo.largeLodLog2    = GR_LOD_LOG2_256;
    texInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
    texInfo.format      = GR_TEXFMT_RGB_565;
    texInfo.data        = storage;

    grTexDownloadMipMap( 0, addr, GR_MIPMAPLEVELMASK_BOTH, &fontInfo );
}


static void grabTex( FxU32 addr, void *storage )
{
    static FxU16 tmpSpace[256][256];
    GrTexInfo   texInfo;
    GrVertex    a, b, c, d;

    grGlideGetState( state );
    grDitherMode( GR_DITHER_DISABLE );
    grColorMask( FXTRUE, FXFALSE );
    grSstOrigin( GR_ORIGIN_UPPER_LEFT );
    grCullMode( GR_CULL_DISABLE );

    /* Grab Upper Left 256*256 of frame buffer */
    readRegion( tmpSpace, 0, 0, 256, 256 );

    /* Grab First 256x256 MM in Texture Ram */
    texInfo.smallLodLog2    = GR_LOD_LOG2_256;
    texInfo.largeLodLog2    = GR_LOD_LOG2_256;
    texInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
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
    grGlideSetState( state );
    return;
}

static void readRegion( void *data, 
                        int sx, int sy,
                        int w, int h )
{
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
                         int w, int h )
{
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


static GrTexTable_t texTableType( GrTextureFormat_t format )
{
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
FxBool
tlErrorMessage(const char *err) {
  fprintf(stderr, "%s", err);
  return FXFALSE;
} /* tlErrorMessage */

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

FxU32
tlGethWnd(void)
{
  return (FxU32)hWndMain;
}

#endif

#if defined(__DOS32__) || defined(macintosh)
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
int  tlKbHit( void )
{
#if macintosh
    EventRecord tempEvent;
    return EventAvail(keyDownMask,&tempEvent);
#else
    return kbhit();
#endif
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
char tlGetCH( void )
{
#if macintosh
    char retVal = 0;        
    EventRecord tempEvent;

    while(retVal==0)
      if (GetNextEvent(everyEvent, &tempEvent)) {
        switch(tempEvent.what) {
          case keyDown:
          case autoKey:
            retVal = (char)(tempEvent.message & 0xFF);
            break;                                    
          default:
            SIOUXHandleOneEvent(&tempEvent);
            break;
        }
      }
    return retVal;
#else
  return getch();
#endif
}

FxBool
tlErrorMessage(const char *err)
{
  fprintf(stderr, "%s", err);
  return FXFALSE;
} /* tlErrorMessage */

FxU32 tlGethWnd( void ) {
#ifdef __DOS32__
        return 1;
#else
        return (FxU32)FrontWindow();
#endif
}

#if macintosh
char *strdup(const char *src)
{
  char *dst = (char *)malloc(strlen(src)+1);
  if(dst) {
    strcpy(dst,src);
  }
  return dst;
}
#endif

#elif __WIN32__


/* This segment simulates main() for Windows, creates a window, etc. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Forward declarations */
HWND hWndMain;

/*
 * MainWndproc
 *
 * Callback for all Windows messages
 */
static int qhead = 0;
static int qtail = 0;
static int queue[256] = {0};

LRESULT WINAPI
MainWndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  PAINTSTRUCT ps;
  HDC         hdc;
  FxU32 i;
  
  switch( message ) {
    case WM_SETCURSOR:
      if (Res != GR_RESOLUTION_NONE) {
        SetCursor(NULL);
        return 0;
      }
      break;

  case WM_ACTIVATE:
    for(i = 0; i < kMaxGlideContext; i++) {
      if (fsContextList[i].hWnd == (FxU32)hWnd) {
        struct fsContextEntry*
          fsContext = fsContextList + i;

        grSelectContext(fsContext->context);
        if (wParam == WA_ACTIVE) {
          grSstSelect(i);
          fsContext->context = grSstWinOpen((FxU32)hWnd,
                                            fsContext->res, fsContext->refresh,
                                            fsContext->colorFmt, 
                                            fsContext->origin,
                                            fsContext->nColBuf, fsContext->nAuxBuf);
        } else if (wParam == WA_INACTIVE) {
          if (grSelectContext(fsContext->context)) {
            
          } else {
            fsContext->context = 0x00UL;
          }
        }
        break;
      }
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
      break;

    case WM_DISPLAYCHANGE:
    case WM_SIZE:
      break;

    case WM_KEYDOWN:
      switch(wParam) {
        case VK_BACK:   queue[qhead++] = TL_BACKSPACE; break;
        case VK_PRIOR:  queue[qhead++] = TL_PAGEUP;    break;
        case VK_NEXT:   queue[qhead++] = TL_PAGEDOWN;  break;
        case VK_END:    queue[qhead++] = TL_END;       break;
        case VK_HOME:   queue[qhead++] = TL_HOME;      break;
        case VK_LEFT:   queue[qhead++] = TL_LEFT;      break;
        case VK_UP:     queue[qhead++] = TL_UP;        break;
        case VK_RIGHT:  queue[qhead++] = TL_RIGHT;     break;
        case VK_DOWN:   queue[qhead++] = TL_DOWN;      break;
        case VK_INSERT: queue[qhead++] = TL_INSERT;    break;
        case VK_DELETE: queue[qhead++] = TL_DELETE;    break;
      }
      qhead &= 255;
      break;

    case WM_CHAR:
      if ((int)wParam & ~0x7f) break;/* not ascii */
#if 0
      printf("Posting keystroke %.02x\n", wParam);
      fflush(stdout);
#endif
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
static BOOL
initApplication( HANDLE hInstance )
{
  WNDCLASS    wc;
  BOOL        rc;
  
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
 * main
 */
#undef main

int
main( int argc, char **argv)
{
  extern int mainT(int argc, char **argv);
   
  if( !initApplication( GetModuleHandle(NULL)) )
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
  /* call the renamed test main proc */
  mainT(argc,argv);
  printf("Exiting main()\n");
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
  /* return here to shut up compiler */
  return 0;
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

FxU32
tlGethWnd(void)
{
  return (FxU32)hWndMain;
}

#endif  /* __WIN32__ */

void tlInitGlideExt ( tlGlideExtension *gExt)
{
  const char *extension;
  char       *extstr;

  memset(gExt, 0, sizeof(tlGlideExtension));

  extension = grGetString(GR_EXTENSION);
  if ((extstr = strstr(extension, "PIXEXT")) != NULL) {
    if (!strncmp(extstr, "PIXEXT", 6)) {
      gExt->grSstWinOpen = grGetProcAddress("grSstWinOpenExt");
      gExt->grColorMask = grGetProcAddress("grColorMaskExt");
      gExt->grStencilFunc = grGetProcAddress("grStencilFuncExt");
      gExt->grStencilMask = grGetProcAddress("grStencilMaskExt");
      gExt->grStencilOp = grGetProcAddress("grStencilOpExt");
      gExt->grBufferClearExt = grGetProcAddress("grBufferClearExt");
      gExt->grLfbConstantStencil = grGetProcAddress("grLfbConstantStencilExt");
      gExt->grTBufferWriteMask = grGetProcAddress("grTBufferWriteMaskExt");
      if ((gExt->grSstWinOpen) && (gExt->grColorMask) &&
          (gExt->grStencilFunc) && (gExt->grStencilMask) &&
          (gExt->grStencilOp) && (gExt->grBufferClearExt) && 
          (gExt->grLfbConstantStencil) && (gExt->grTBufferWriteMask)) {
        gExt->pixext = FXTRUE;
      }
    }
  }
  if ((extstr = strstr(extension, "COMBINE")) != NULL) {
    if (!strncmp(extstr, "COMBINE", 7)) {
      gExt->grColorCombineExt = grGetProcAddress("grColorCombineExt");
      gExt->grAlphaCombineExt = grGetProcAddress("grAlphaCombineExt");
      gExt->grTexColorCombineExt = grGetProcAddress("grTexColorCombineExt");
      gExt->grTexAlphaCombineExt = grGetProcAddress("grTexAlphaCombineExt");
      if ((gExt->grColorCombineExt) && (gExt->grAlphaCombineExt) &&
          (gExt->grTexColorCombineExt) && (gExt->grTexAlphaCombineExt)) {
        gExt->combineext = FXTRUE;
      }
    }
  }  
  if ((extstr = strstr(extension, "TEXFMT")) != NULL) {
    if (!strncmp(extstr, "TEXFMT", 6)) {
      gExt->canDo32BitTexture = FXTRUE;
      gExt->canDoFXT1Texture = FXTRUE;
      gExt->canDo2kTexture = FXTRUE;
    }
  }
  if ((extstr = strstr(extension, "TEXUMA")) != NULL) {
    if (!strncmp(extstr, "TEXUMA", 6)) {
      gExt->umaExt = FXTRUE;
    }
  }
}
