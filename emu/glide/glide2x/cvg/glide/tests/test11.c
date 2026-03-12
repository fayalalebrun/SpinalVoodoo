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


GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "test11";
static const char purpose[] = "simple lfb read/write test";
static const char usage[]   = "-n <frames> -r <res>";

static int rRandom(int s, int e);
static unsigned int iRandom (unsigned int maxr);

int main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;

    static unsigned short colorBuf[64][64];
    static unsigned short grabBuf[64][64];
    
    int x,y;

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

    tlSetScreen( scrWidth, scrHeight );

    grGlideGetVersion( version );

    printf( "%s:\n%s\n", name, purpose );
    printf( "%s\n", version );
    printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );
    if ( frames == -1 ) {
        printf( "Press A Key To Begin Test.\n" );
        tlGetCH();
    }
    
    /* Initialize Glide */
    grGlideInit();
    assert( grSstQueryHardware( &hwconfig ) );
    grSstSelect( 0 );
    assert( grSstWinOpen( 0,
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1 ) );
    
    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );

    /* Set up Render State - disable dithering*/
    grDitherMode( GR_DITHER_DISABLE );


    /* Create Source Bitmap to be copied to framebuffer */
    for( y = 0; y < 64; y++ ) {
        for( x = 0; x < 64; x++ ) {
            FxU8 red = x << 2;
            FxU8 grn = y << 2;
            FxU8 blu = ( x + y )<<1;
            colorBuf[y][x] =  (red & 0xF8) << 8;
            colorBuf[y][x] |= (grn & 0xFC) << 3;
            colorBuf[y][x] |= (blu & 0xF8) >> 3;
        }
    }


    tlConOutput( "Press any key to quit\n" );
    while( frames-- && tlOkToRender()) {
        GrLfbInfo_t info;
        int startX, startY;

        if (hwconfig.SSTs[0].type == GR_SSTTYPE_SST96) {
          tlGetDimsByConst(resolution,
                           &scrWidth, 
                           &scrHeight );
        
          grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
        }

        grBufferClear( 0x00, 0, 0 );

        /* prepare info structure */
        info.size = sizeof( GrLfbInfo_t );
        
        /* lock back buffer */
        if ( grLfbLock( GR_LFB_WRITE_ONLY,
                        GR_BUFFER_BACKBUFFER,
                        GR_LFBWRITEMODE_565,
                        GR_ORIGIN_UPPER_LEFT,
                        FXFALSE,
                        &info )==FXFALSE) {
            tlConOutput( "Error, failed to take write lock\n" );
            break;
        } 

        if ( tlScaleX(1.0f) < 64.0 ||
             tlScaleY(1.0f) < 64.0 )
            return -1;
        
        /* generate random start position */
        startX = (int)rRandom( 64, (int)tlScaleX(1.0f) - 65 );
        startY = (int)rRandom( 64, (int)tlScaleY(1.0f) - 65 );
        
        /* render image to back buffer */
        for( y = 0; y < 64; y++ ) {
            for( x = 0; x < 64; x++ ) {
                FxU16 *pixel = 
                    (FxU16*)(((char*)info.lfbPtr) + 
                             (y+startY)*info.strideInBytes+
                             (x+startX)*2);
                *pixel = colorBuf[y][x];
            }
        }
        /* unlock the backbuffer */
        grLfbUnlock( GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER );

        /* swap to front buffer */
        grBufferSwap( 1 );
        grBufferClear( 0,0,0 );

        tlSleep( 1 );
        
        /* lock the front buffer */
        if ( grLfbLock( GR_LFB_READ_ONLY,
                        GR_BUFFER_FRONTBUFFER,
                        GR_LFBWRITEMODE_ANY,
                        GR_ORIGIN_UPPER_LEFT,
                        FXFALSE,
                        &info )==FXFALSE) {
            tlConOutput( "Error, failed to take read lock\n" );
            break;
        } 
        
        /* grab the source image out of the front buffer */
        for( y = 0; y < 64; y++ ) {
            for( x = 0; x < 64; x++ ) {
                FxU16 *pixel = 
                    (FxU16*)(((char*)info.lfbPtr) + 
                             (y+startY)*info.strideInBytes+
                             (x+startX)*2);
                grabBuf[y][x] = *pixel;
            }
        }
        /* unlock the front buffer */
        grLfbUnlock( GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER );

        tlConClear();
        
        /* compare the source image to the readback image */
        if ( memcmp( colorBuf, grabBuf, sizeof( colorBuf ) ) ) {
            tlConOutput( "Failed readback test\n" );
        } else {
            tlConOutput( "Passed readback test\n" );
        }

        grBufferSwap( 1 );
        tlConOutput( "Press any key to quit\n" );
        tlConRender();
        grBufferSwap( 1 );

        tlSleep( 1 );

        while( tlKbHit() ) {
            switch( tlGetCH() ) {
            default:
                frames = 0;
                break;
            }
        }
    }
    
    grGlideShutdown();
    return 0;
}

static unsigned long randx = 1;

static unsigned int iRandom (unsigned int maxr)
{
    unsigned int n,retval;

    if (maxr > 0xFFFFFFF) {
        do {
            retval = iRandom(0xFFFFFFF);
            retval |= iRandom(maxr>>28)<<28;
        } while (retval > maxr);
        return retval;
    }
    for (n=1; n<32; n++)
        if (((unsigned)1 << n) > maxr) break;
    do {
        randx = randx*1103515245 + 12345;
        retval = (randx & 0x7fffffff) >> (31-n);
    } while (retval > maxr);
    return retval;
}

static int rRandom(int s, int e)
{
    return s + iRandom(e-s);
}



