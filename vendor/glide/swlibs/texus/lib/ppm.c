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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "texusint.h"


FxBool 
_txReadPPMHeader( FILE *stream, FxU32 cookie, TxMip *info)
{ 
    char buffer[256];
    FxU32 state = 1;
    FxBool done = FXFALSE;
    
    if ( stream == NULL ) {
	txPanic("PPM file: Bad file handle.");
	return FXFALSE;
    }
    
    while( !done && fgets( buffer, 256, stream ) ) {
	char *token;
	
	if ( buffer[0] == '#' ) continue;
	for (token = strtok( buffer, " \t\n\r" ); token != NULL;
			token = strtok( NULL, " \t\n\r" )) {
	    switch( state ) {
	    case 1:	// Width
			info->width = atoi( token );
			state++;
			break;

	    case 2:	// height
			info->height = atoi( token );
			state++;
			break;

	    case 3:	// Color Depth
			info->format = atoi( token );
			if ( info->format != 255 ) {
			    txPanic("Unsupported PPM format: max != 255\n");
		    	    return FXFALSE;
			}
			state++;
			done = FXTRUE;
			break;

	    default:
			txPanic("PPM file: parse error\n");
			return FXFALSE;
			break;
	    }
	}
    }
    
    if ( state < 4 ) {
    	txPanic("PPM file: Read error before end of header.");
	return FXFALSE;
    }
    info->depth = 1;
    info->format = GR_TEXFMT_ARGB_8888;
    info->size = info->width * info->height * 4;
    return FXTRUE;
}

FxBool 
_txReadPPMData( FILE *stream, TxMip *info) 
{ 
    FxU32 numPixels;
    FxU32 *data32 = info->data[0];
    
    numPixels = info->width * info->height;

    if ( stream == NULL ) {
	txPanic("PPM file: Bad file handle.");
	return FXFALSE;
    }
    
    // Read in image data
    while (numPixels--) {
	int r, g, b;

	r = getc( stream );
	g = getc( stream );
	b = getc( stream );
	if ( b == EOF ) {
	    txPanic("PPM file: Unexpected End of File.");
	    return FXFALSE;
	}
	*data32++ = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
    return FXTRUE;
}
