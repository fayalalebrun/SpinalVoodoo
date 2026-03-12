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

int hwconfig;

static const char name[]    = "test36";
static const char purpose[] = "screen resolutions";
static const char usage[]   = "-p show passed, -n no progress";

static const char status[]  = "-\\|/";

int main( int argc, char **argv) {
        FxBool bShowPassed = FXFALSE;
        FxBool bNoProgress = FXFALSE;
        GrResolution resTemplate;
        GrResolution *presSupported;
        FxI32 size, i, 
                iFailures = 0, iStatusAt = 0,
                viewport[4];
        GrContext_t context;

        printf ( "Test36 - Glide resolution checker.  All output goes to the console.\n\n" );
        tlGetCH ();
        for ( i=1; i < argc; i++ ) {
                if ( !strcmp ( argv[i], "-p" ) )
                        bShowPassed = FXTRUE;
                if ( !strcmp ( argv[i], "-n" ) )
                        bNoProgress = FXTRUE;
        }

        grGlideInit ();
        resTemplate.resolution = GR_QUERY_ANY;
        resTemplate.refresh = GR_QUERY_ANY;
        resTemplate.numColorBuffers = GR_QUERY_ANY;
        resTemplate.numAuxBuffers = GR_QUERY_ANY;

        size = grQueryResolutions ( &resTemplate, NULL );
        presSupported = ( GrResolution * ) malloc ( size );
        
        size = size / sizeof ( GrResolution );
        grQueryResolutions ( &resTemplate, presSupported );

        grGlideShutdown ();

        // Now, start the assault...  Glide Init, mode set, Glide Shutdown...

        for ( i = 0; i < size; i++ ) {
                grGlideInit ();

                context = grSstWinOpen ( tlGethWnd(), 
                        presSupported[i].resolution, 
                        presSupported[i].refresh, 
                        GR_COLORFORMAT_ABGR, 
                        GR_ORIGIN_UPPER_LEFT, 
                        presSupported[i].numColorBuffers, 
                        presSupported[i].numAuxBuffers );
                
                if ( context ) {
                        grGet ( GR_VIEWPORT, sizeof ( FxI32 ) * 4, viewport );
                        if ( bShowPassed ) {
                                printf ( "Res #%d ( %d, %d, %d, %d ) - Refresh %d, nCol %d, nAux %d - PASSED!\n", 
                                presSupported[i].resolution, viewport[0], viewport[1], viewport[2], viewport[3], 
                                presSupported[i].refresh, presSupported[i].numColorBuffers, presSupported[i].numAuxBuffers );
                        } else if ( !bNoProgress ) {
                                printf ( "Working...  %c\r", status[iStatusAt] );
                                iStatusAt = ( iStatusAt + 1 ) & 3;
                        }
                        grSstWinClose ( context ) ;
                } else {
                        printf ( "Res #%d - Refresh %d, nCol %d, nAux %d - FAILED!\n", 
                                presSupported[i].resolution, presSupported[i].refresh, 
                                presSupported[i].numColorBuffers, presSupported[i].numAuxBuffers );
                        iFailures++;
                }                       

                grGlideShutdown ();
        }

        free ( presSupported );

        if ( iFailures ) 
                printf ( "This test failed with %d failures.  ", iFailures );
        else
            printf ( "This test passed.  " );

        printf ( "Press any key.\n" );
        tlGetCH ();
        return 0;
}





