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
#include <assert.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

typedef struct {
  FxU32 signature;
  FxU16 width;
  FxU16 height;
  FxU8  depth;
  FxU8  type;
  void  *data;
} LFB_Img;

int hwconfig;
static const char *version;

static const char name[]    = "display";
static const char purpose[] = "display a 16 bit frame buffer (565 format) dump out";
static const char usage[]   = "-n <frames> -r <res> -s srcimage -t testimage";

static const char *sourceFormatString[] = {
    "GR_LFB_SRC_FMT_565       ",
    "GR_LFB_SRC_FMT_555       ",
    "GR_LFB_SRC_FMT_1555      ",
    "INVALID FORMAT           ",
    "GR_LFB_SRC_FMT_888       ",
    "GR_LFB_SRC_FMT_8888      ",
    "INVALID FORMAT           ",
    "INVALID FORMAT           ",
    "INVALID FORMAT           ",
    "INVALID FORMAT           ",
    "INVALID FORMAT           ",
    "INVALID FORMAT           ",
    "GR_LFB_SRC_FMT_565_DEPTH ",
    "GR_LFB_SRC_FMT_555_DEPTH ",
    "GR_LFB_SRC_FMT_1555_DEPTH",
    "GR_LFB_SRC_FMT_ZA16      "
};

static void imageConvert( void *dst,
                          void *src,
                          GrLfbSrcFmt_t format,
                          FxU32 *bpp );

int main( int argc, char **argv)
{
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;
    char srcfname[80], dstfname[80];
    FILE *fp;
    void *mem;
    FxU32 filesize;
    LFB_Img src, dst, diff;
    FxBool txtdisplay = FXTRUE;

    void *image = NULL;
    FxU32 bpp;

    GrLfbSrcFmt_t sourceFormat;

    static FxU32 imageWidth;
    static FxU32 imageHeight;

    srcfname[0] = 0;
    dstfname[0] = 0;

    /* Process Command Line Arguments */
    while ((rv = tlGetOpt(argc, argv, "nrst", &match, &remArgs)) != 0) {
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
        case 's':
            strcpy(srcfname, remArgs[0]);
            break;
        case 't':
            strcpy(dstfname, remArgs[0]);
            break;
        }
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
    
    /* Initialize Glide */
    grGlideInit();
    assert( hwconfig = tlVoodooType() );

    tlVertexLayout();
    grSstSelect( 0 );
    assert( grSstWinOpen(tlGethWnd(),
                         resolution,
                         GR_REFRESH_60Hz,
                         GR_COLORFORMAT_ABGR,
                         GR_ORIGIN_UPPER_LEFT,
                         2, 1 ) );
    
    tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 
              60, 30, 0xffffff );

    /* Set up Render State - disable dithering */
    grDitherMode( GR_DITHER_DISABLE );

    /* Load src image from disk */

    if (srcfname[0]) {
      fp = fopen(srcfname, "rb");
      if (fp == NULL)
        srcfname[0] = 0;
      else {
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        rewind(fp);

        /* readng the LFB file header */
        fread(&src.signature, 4, 1, fp);
        if (src.signature != IMAGE_SRLE) {
          printf("%s file type incorrect\n", srcfname);
          return -1;
        }
        fread(&src.width, 2, 1, fp);
        fread(&src.height, 2, 1, fp);
        fread(&src.depth, 1, 1, fp);
        fread(&src.type, 1, 1, fp);

        mem = malloc(filesize-6);
        src.data = malloc(src.width*src.height*(src.depth/8));
        /* reading the LFB data */
        fread(mem, filesize-6, 1, fp);
        /* Simple16BitDecode(src.width,src.height,mem,src.data); */
        SimpleRleDecode(src.width,src.height,2,mem,src.data);

        free(mem);
        fclose(fp);
      }
    }

    /* Load dst image from disk */

    if (dstfname[0]) {
      fp = fopen(dstfname, "rb");
      if (fp == NULL)
        dstfname[0] = 0;
      else {
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        rewind(fp);

        /* readng the LFB file header */
        fread(&dst.signature, 4, 1, fp);
        if (dst.signature != IMAGE_SRLE) {
          printf("%s file type incorrect\n", dstfname);
          return -1;
        }
        fread(&dst.width, 2, 1, fp);
        fread(&dst.height, 2, 1, fp);
        fread(&dst.depth, 1, 1, fp);
        fread(&dst.type, 1, 1, fp);

        mem = malloc(filesize-6);
        dst.data = malloc(dst.width*dst.height*(dst.depth/8));
        /* reading the LFB data */
        fread(mem, filesize-6, 1, fp);
        SimpleRleDecode(dst.width,dst.height,2,mem,dst.data);

        free(mem);
        fclose(fp);
      }
    }

    diff.data = NULL;
    diff.width = 0;
    diff.height = 0;
        
    if (srcfname[0] && dstfname[0]) {
      FxU32 count;
      FxU16 *srcptr, *dstptr, *difptr;

      if ((src.width == dst.width) && (src.height == dst.height)) {
        diff.width = src.width;
        diff.height = src.height;
        diff.depth = src.depth;
        diff.data = malloc(diff.width*diff.height*(diff.depth/8));
        count = diff.width * diff.height;
        srcptr = src.data;
        dstptr = dst.data;
        difptr = diff.data;
        while (count) {
          if (*srcptr == *dstptr)
            *difptr = 0;
          else
            *difptr = *srcptr ^ *dstptr;
          srcptr++; dstptr++; difptr++;
          count--;
        }
      }
    }



    sourceFormat = GR_LFB_SRC_FMT_565;

    if (srcfname[0]) {
      imageWidth  = src.width;
      imageHeight = src.height;
      image       = calloc( imageWidth * imageHeight, sizeof( FxU32 ) );
      imageConvert( image, src.data, sourceFormat, &bpp );
    }
    else if (dstfname[0]) {
      imageWidth  = dst.width;
      imageHeight = dst.height;
      image       = calloc( imageWidth * imageHeight, sizeof( FxU32 ) );
      imageConvert( image, dst.data, sourceFormat, &bpp );
    }
    if ( ( imageWidth > (FxU32)scrWidth ) ||
         ( imageHeight > (FxU32)scrHeight ) )
        return -1;

    while( frames-- ) {

      grBufferClear( 0x00303030, 0, 0 );


      grLfbWriteRegion( GR_BUFFER_BACKBUFFER,
                        0, 0, sourceFormat,
                        imageWidth, imageHeight, FXFALSE, imageWidth*bpp, image );

      tlConClear();
      if (txtdisplay) {
        tlConOutput("1   - lfb source format (%s)\n",
                    sourceFormatString[sourceFormat] );
        if (srcfname[0])
          tlConOutput("s   - display source image\n");
        else
          tlConOutput("no source image\n");
        if (dstfname[0])
          tlConOutput("t   - display test image\n");
        else
          tlConOutput("no test image\n");
        if (srcfname[0] && dstfname[0]) {
          if (diff.width > 0)
            tlConOutput("d   - display differences\n");
          else
            tlConOutput("source and test image size are not the same\n");
        }

        tlConOutput("space - turn on/off text display\n");

        tlConOutput("any other key to quit\n\n");
      }

      /* 
      tlConOutput( "1   - lfb source format (%s)\n"
                   "+/- - change width of source image copied\n"
                   "s   - display source image\n"
                   "t   - display test image\n"
                   "d   - display differences\n"
                   "any other key to quit\n\n",
                   sourceFormatString[sourceFormat] );
      */

      tlConRender();
      grBufferSwap( 1 );
      while( tlKbHit() ) {
        switch( tlGetCH() ) {
        case '1':
          break;
        case '+':
          break;
        case '-':
          break;
        case 's':
          if (srcfname[0]) {
            imageWidth  = src.width;
            imageHeight = src.height;
            imageConvert( image, src.data, sourceFormat, &bpp );
          }
          break;
        case 't':
          if (dstfname[0]) {
            imageWidth  = dst.width;
            imageHeight = dst.height;
            imageConvert( image, dst.data, sourceFormat, &bpp );
          }
          break;
        case 'd':
          if (diff.width > 0)
            imageConvert( image, diff.data, sourceFormat, &bpp );
          break;
        case ' ':
          txtdisplay = !txtdisplay;
          break;
        default:
          frames = 0;
          break;
        }
      }
    }
    

    grGlideShutdown();
    if (srcfname[0])
      free(src.data);
    if (dstfname[0])
      free(dst.data);
    if (srcfname[0] && dstfname[0])
      free(diff.data);
    free(image);
    return 0;
}


static void imageConvert( void *dst, void *src, 
                          GrLfbSrcFmt_t format, FxU32 *bpp ) {
    FxU32 x, y; 
    FxU32 *longData   = dst;
    FxU16 *shortData  = dst;
    FxU16 *srcData    = src;
    FxU32 longStride  = 640;
    FxU32 shortStride = 640;
    FxU32 longColor;
    FxU16 shortColor;

    switch( format ) {
    case GR_LFB_SRC_FMT_565:
    case GR_LFB_SRC_FMT_555:
    case GR_LFB_SRC_FMT_1555:
        *bpp = 2;
        break;
    case GR_LFB_SRC_FMT_888:
    case GR_LFB_SRC_FMT_8888:
    case GR_LFB_SRC_FMT_565_DEPTH:
    case GR_LFB_SRC_FMT_1555_DEPTH:
    case GR_LFB_SRC_FMT_555_DEPTH:
        *bpp = 4;
        break;
    }

    for( y = 0; y < 480; y++ ) {
        for( x = 0; x < 640; x++ ) {
            switch( format ) {
            case GR_LFB_SRC_FMT_565:
                shortData[y*shortStride+x] = 
                    srcData[y*640+x];
                break;
            case GR_LFB_SRC_FMT_555:
            case GR_LFB_SRC_FMT_1555:
                shortColor = srcData[y*640+x];
                shortColor = 
                    (0x8000) | // Alpha == 1
                    ((shortColor >> 1) & 0x7C00) |
                    ((shortColor >> 1) & 0x03E0) |
                    ((shortColor) & 0x1f);
                shortData[y*shortStride+x] =
                    shortColor;
                break;
            case GR_LFB_SRC_FMT_888:
            case GR_LFB_SRC_FMT_8888:
                longColor = srcData[y*640+x];
                longColor =
                    (0xFF000000)                 |
                    ((longColor<<8)&0x00F80000)  |
                    ((longColor<<5)&0x0000FC00)  |
                    ((longColor<<3)&0x000000F8);
                longData[y*longStride+x] = longColor;
                break;
            case GR_LFB_SRC_FMT_565_DEPTH:
                longColor = srcData[y*640+x];
                longData[y*longStride+x] = longColor;
                break;
            case GR_LFB_SRC_FMT_1555_DEPTH:
            case GR_LFB_SRC_FMT_555_DEPTH:
                longColor = srcData[y*640+x];
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
    }
    return;
}



