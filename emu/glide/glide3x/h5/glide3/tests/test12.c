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
#include <assert.h>

#include <glide.h>
#include "tlib.h"


int hwconfig;
static const char *version;

static const char name[]    = "test12";
static const char purpose[] = "lfb write modes, render buffer, pixpipe, yorigin test";
static const char usage[]   = "-n <frames> -r <res>";


static const char *originString[] = {
  "GR_ORIGIN_UPPER_LEFT ",
  "GR_ORIGIN_LOWER_LEFT"
};

static const char *renderBufferString[] = {
  "GR_BUFFER_FRONTBUFFER",
  "GR_BUFFER_BACKBUFFER "
};

static const char *writeModeString[] = {
  "GR_LFBWRITEMODE_565       ",
  "GR_LFBWRITEMODE_555       ",
  "GR_LFBWRITEMODE_1555      ",
  "GR_LFBWRITEMODE_RESERVED1 ",
  "GR_LFBWRITEMODE_888       ",
  "GR_LFBWRITEMODE_8888      ",
  "GR_LFBWRITEMODE_RESERVED2 ",
  "GR_LFBWRITEMODE_RESERVED3 ",
  "GR_LFBWRITEMODE_RESERVED4 ",
  "GR_LFBWRITEMODE_RESERVED5 ",
  "GR_LFBWRITEMODE_RESERVED6 ",
  "GR_LFBWRITEMODE_RESERVED7 ",
  "GR_LFBWRITEMODE_565_DEPTH ",
  "GR_LFBWRITEMODE_555_DEPTH ",
  "GR_LFBWRITEMODE_1555_DEPTH",
  "GR_LFBWRITEMODE_ZA16      "
};

static const char *pixPipeString[] = {
  "PIXELPIPE DISABLED",
  "PIXELPIPE ENABLED "
};

int
main( int argc, char **argv)
{
  char match; 
  char **remArgs;
  int  rv;

  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  int frames                      = -1;

  TlTexture texture;

  GrOriginLocation_t lfbOrigin;
  GrOriginLocation_t sstOrigin;
  GrLfbWriteMode_t   writeMode;
  GrBuffer_t         lfbBuffer;
  GrBuffer_t         sstBuffer;
  GrBuffer_t         curBuffer;
  GrLfbInfo_t        info;
  FxBool             pixPipe;
  GrContext_t        gc = 0;

  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );

  /* Process Command Line Arguments */
  while ((rv = tlGetOpt(argc, argv, "nr", &match, &remArgs)) != 0) {
    if ( rv == -1 ) {
      printf( "Unrecognized command line argument\n" );
      printf( "%s %s\n", name, usage );
      printf( "Available resolutions:\n%s\n",
             tlGetResolutionList() );
      return -1;
    }
    switch( match ) {
    case 'n':
      frames = atoi( remArgs[0] );
      break;
    case 'r':
      resolution = tlGetResolutionConstant( remArgs[0], 
                                           &scrWidth, 
                                           &scrHeight );
      break;
    }
  }

  if ( resolution == GR_RESOLUTION_NONE ) {
    tlErrorMessage( "Error!: Frontbuffer rendering not supported in a window\n" );
    return -1;
  }

  tlSetScreen( scrWidth, scrHeight );

  version = grGetString( GR_VERSION );

  printf( "%s:\n%s\n", name, purpose );
  printf( "%s\n", version );
  printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );
  if ( frames == -1 ) {
    printf( "Press A Key To Begin Test.\n" );
    tlGetCH();
  }
    
  grSstSelect( 0 );
  gc = grSstWinOpen(tlGethWnd(),
                    resolution,
                    GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ABGR,
                    GR_ORIGIN_UPPER_LEFT,
                    2, 1 );
  if (!gc) {
    printf("Could not allocate glide fullscreen context.\n");
    goto __errExit;
  }
  
  tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
           60, 15, 0xffffff );


  /* Load source bitmap from a .3df file */

  assert( tlLoadTexture( "decal1.3df", 
                        &texture.info, 
                        &texture.tableType, 
                        &texture.tableData ) );
    
  lfbOrigin = GR_ORIGIN_UPPER_LEFT;
  sstOrigin = GR_ORIGIN_UPPER_LEFT;
  lfbBuffer = GR_BUFFER_BACKBUFFER;
  sstBuffer = GR_BUFFER_BACKBUFFER;
  curBuffer = GR_BUFFER_BACKBUFFER;
  pixPipe   = FXFALSE;
  writeMode = GR_LFBWRITEMODE_565;

  info.size = sizeof( info );

  while( frames-- && tlOkToRender()) {

    static const char *console = {
      "Current Buffer: %s\n"
        "1 - lock yOrigin        (%s)\n"
          "2 - glide yOrigin       (%s)\n"
            "3 - lfb render buffer   (%s)\n"
              "4 - glide render buffer (%s)\n"
                "5 - pixpipe enable      (%s)\n"
                  "6 - lfb write mode      (%s)\n"
                    "Press any other key to quit\n"
    };

    if (hwconfig == TL_VOODOORUSH) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );
      
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    }

    grRenderBuffer( GR_BUFFER_BACKBUFFER );
    grBufferClear( 0, 0, 0 );
    grRenderBuffer( GR_BUFFER_FRONTBUFFER );
    grBufferClear( 0, 0, 0 );
    grRenderBuffer( GR_BUFFER_BACKBUFFER );


    /* set Glide State */
    grRenderBuffer( sstBuffer );
    grSstOrigin( sstOrigin );
        
    /* Attempt Lock */
    if ( grLfbLock( GR_LFB_WRITE_ONLY, 
                   (writeMode == GR_LFB_SRC_FMT_ZA16) ? GR_BUFFER_AUXBUFFER : lfbBuffer,
                   writeMode,
                   lfbOrigin,
                   pixPipe,
                   &info ) ) {
      int x, y;
      FxU32 *longData   = info.lfbPtr;
      FxU16 *shortData  = info.lfbPtr;
      FxU16 *srcData    = texture.info.data;
      FxU32 longStride  = info.strideInBytes >> 2;
      FxU32 shortStride = info.strideInBytes >> 1;
      FxU32 longColor;
      FxU16 shortColor;
      for( y = 0; y < 256; y++ )
        for( x = 0; x < 256; x++ ) {
          switch( writeMode ) {
          case GR_LFBWRITEMODE_565:
            shortData[y*shortStride+x] = 
              srcData[y*256+x];
            break;
          case GR_LFBWRITEMODE_555:
          case GR_LFBWRITEMODE_1555:
            shortColor = srcData[y*256+x];
            shortColor = 
              (0x8000) | // Alpha == 1
                ((shortColor >> 1) & 0x7C00) |
                  ((shortColor >> 1) & 0x03E0) |
                    ((shortColor) & 0x1f);
            shortData[y*shortStride+x] =
              shortColor;
            break;
          case GR_LFBWRITEMODE_888:
          case GR_LFBWRITEMODE_8888:
            longColor = srcData[y*256+x];
            longColor =
              (0xFF000000)                 |
                ((longColor<<8)&0x00F80000)  |
                  ((longColor<<5)&0x0000FC00)  |
                    ((longColor<<3)&0x000000F8);
            longData[y*longStride+x] = longColor;
            break;
          case GR_LFBWRITEMODE_565_DEPTH:
            longColor = srcData[y*256+x];
            longData[y*longStride+x] = longColor;
            break;
          case GR_LFBWRITEMODE_1555_DEPTH:
          case GR_LFBWRITEMODE_555_DEPTH:
            longColor = srcData[y*256+x];
            longColor =
              (0x00008000)                  |
                ((longColor>>1) & 0x00007C00) |
                  ((longColor>>1) & 0x000003E0) |
                    ((longColor   ) & 0x0000001f);
            longData[y*longStride+x] = longColor;
          default:
            break;
          }

        }

      grLfbUnlock( GR_LFB_WRITE_ONLY,
                   (writeMode == GR_LFB_SRC_FMT_ZA16) ? GR_BUFFER_AUXBUFFER : lfbBuffer);
      
      grRenderBuffer( GR_BUFFER_BACKBUFFER );
            
      tlConClear();
      curBuffer = GR_BUFFER_BACKBUFFER;
      tlConOutput( console,
                  renderBufferString[curBuffer],
                  originString[lfbOrigin],
                  originString[sstOrigin],
                  renderBufferString[lfbBuffer],
                  renderBufferString[sstBuffer],
                  pixPipeString[pixPipe],
                  writeModeString[writeMode] );
      tlConRender();
      grBufferSwap( 1 );
      tlSleep( 1 );
      tlConClear();
      curBuffer = GR_BUFFER_FRONTBUFFER;
      tlConOutput( console,
                  renderBufferString[curBuffer],
                  originString[lfbOrigin],
                  originString[sstOrigin],
                  renderBufferString[lfbBuffer],
                  renderBufferString[sstBuffer],
                  pixPipeString[pixPipe],
                  writeModeString[writeMode] );
      tlConRender();
      grBufferSwap( 1 );
      tlSleep( 1 );
    } else {
      grRenderBuffer( GR_BUFFER_BACKBUFFER );
      tlConClear();
      tlConOutput( console,
                  renderBufferString[curBuffer],
                  originString[lfbOrigin],
                  originString[sstOrigin],
                  renderBufferString[lfbBuffer],
                  renderBufferString[sstBuffer],
                  pixPipeString[pixPipe],
                  writeModeString[writeMode] );
      tlConOutput( "\nLock Failed....no output\n" );
      tlConRender();
      grBufferSwap( 1 );
      tlSleep( 1 );
    }
        
        
    while( tlKbHit() ) {
      switch( tlGetCH() ) {
      case '1':
        lfbOrigin = !lfbOrigin;
        break;
      case '2':
        sstOrigin = !sstOrigin;
        break;
      case '3':
        lfbBuffer = !lfbBuffer;
        break;
      case '4':
        sstBuffer = !sstBuffer;
        break;
      case '5':
        pixPipe = !pixPipe;
        break;
      case '6':
        writeMode++;
        if ( writeMode > GR_LFBWRITEMODE_ZA16 ) 
          writeMode = GR_LFBWRITEMODE_565;
        break;
      default:
        frames = 0;
        break;
      }
    }
  }
    
 __errExit:    
  grGlideShutdown();
  return 0;
} /* main */



