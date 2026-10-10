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
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "3dfx.h"
#include "fximg.h"
#include "fxos.h"

/*
** PRIVATE GLOBAL DATA
*/

char *imgErrorString = "No error.";

/*
** PRIVATE UTILITY FUNCTIONS
*/

/* _fixStream
**
** Summary: UNPUBLISHED - change mode for input / output streams so that
**          they are are in binary mode for dos apps.
*/
#if defined( __DOS__ ) || defined( WIN32 )
/* 
** On dos-compatible platforms, iostreams must be set to binary( untranslated )
** mode
*/
#include <fcntl.h>
#include <io.h>
void _fixStream( FILE *stream )
{
	setmode( fileno( stream ), O_BINARY );
}
#else
/*
** Otherwise do nothing
*/
#define _fixStream( X ) X = X
#endif


/* _imgStrUpr
**
** Summary: UNPUBLISHED - upper case the provided string in place.
**          Provided because strupr isn't provided in all clib implementations.
*/
void _imgStrUpr( char *str )
{
    FxU32 index;
    for ( index = 0; index < strlen( str ); index++ )
    {
		if ( str[index] >= 'a' && str[index] <= 'z' )
		{
	    	str[index] -= 'a' - 'A';
		}
    }
}

/* _imgIsLittleEndian
**
** UNPUBLISHED - returns FXTRUE if the host architecture
**               stores memory in little endian format
*/
FxBool _imgIsLittleEndian( void )
{
    unsigned long x = 0x0000FFFF;
    return( *((char *)&x)?FXTRUE:FXFALSE );
}

/* _imgTxDecodeColorFormat
**
** UNPUBLISHED - returns the integer color format give in a tdf color format
**
*/
imgTxColorFormat _imgTxDecodeColorFormat( char * String )
{
  if(!strcmp(String,"true") ||
     !strcmp(String,"TRUE"))
    return txColorTrue;
  else if(!strcmp(String,"i8") ||
	  !strcmp(String,"I8"))
    return txColorI_8;
  else if(!strcmp(String,"a8") ||
	  !strcmp(String,"A8"))
    return txColorA_8;
  else if(!strcmp(String,"ai44") ||
	  !strcmp(String,"AI44"))
    return txColorAI_44;
  else if(!strcmp(String,"yiq") ||
	  !strcmp(String,"YIQ"))
    return txColorYIQ;
  else if(!strcmp(String,"rgb332") ||
	  !strcmp(String,"RGB332"))
    return txColorRGB_332;
  else if(!strcmp(String,"rgb565") ||
	  !strcmp(String,"RGB565"))
    return txColorRGB_565;
  else if(!strcmp(String,"argb8332") ||
	  !strcmp(String,"ARGB8332"))
    return txColorARGB_8332;
  else if(!strcmp(String,"argb1555") ||
	  !strcmp(String,"ARGB1555"))
    return txColorARGB_1555;
  else if(!strcmp(String,"ayiq8422") ||
	  !strcmp(String,"AYIQ8422"))
    return txColorAYIQ_8422;
  else if(!strcmp(String,"argb4444") ||
	  !strcmp(String,"ARGB4444"))
    return txColorARGB_4444;
  else if(!strcmp(String,"ai88") ||
	  !strcmp(String,"AI88"))
    return txColorAI_88;
  else if(!strcmp(String,"argb8888") ||
	  !strcmp(String,"ARGB8888"))
    return txColorARGB_8888;
  else
    return txColorUnknown;
}

/* _imgMSBReplicate
**
** Summary: UNPUBLISHED - Replicates the msb's of the input value to fill
**                        in an 8-bit color channel;
*/
#define _imgMSBReplicate( COLOR, LEFT_SHIFT, RIGHT_SHIFT ) (COLOR << LEFT_SHIFT) | (COLOR >> RIGHT_SHIFT)

/* _imgGuessType
**
** Summary: UNPUBLISHED - read the first word of an image file stream
**          determine the file type.
*/
FxBool _imgGuessType( FILE *stream, ImgType *type )
{
    FxU32 cookie = 0;
    int   c;
    
    if ( stream == NULL )
    {
		imgErrorString = "Bad File Handle";
		return FXFALSE;
    }
    
    if ( ( c = fgetc( stream ) ) == EOF )
    {
		imgErrorString = "Unexpected end of file";
		return FXFALSE;
    }
    
    cookie = (c << 8);
    
    if ( ( c = fgetc( stream ) ) == EOF )
    {
		imgErrorString = "Unexpected end of file";
		return FXFALSE;
    }
    
    cookie |= c;
    
    switch( cookie )
    {
      	case ('P' << 8) | '9': // SBI
			*type = IMG_SBI;
			break;
      	case ('P' << 8) | '6': // PPM
			*type = IMG_P6;
			break;
      	case ('3' << 8) | 'd': // 3df
      	case ('3' << 8) | 'D': // 3df
			*type = IMG_3DF;
			break;
      	case 0xDA01:
			*type = IMG_RGT;
			break;
      	case (('E' << 8) | 'L'):
	  cookie = 0;
	  if ( ( c = fgetc( stream ) ) == EOF )
	    {
	      imgErrorString = "Unexpected end of file";
	      return FXFALSE;
	    }
	  cookie = (cookie << 8) | c;
	  if ( ( c = fgetc( stream ) ) == EOF )
	    {
	      imgErrorString = "Unexpected end of file";
	      return FXFALSE;
	    }
	  cookie = (cookie << 8) | c;
	  if (cookie == (('R' << 8) | 'S'))
	    *type = IMG_SRLE;
	  break;
      	default: // Might Be TGA
			if ( ( c = fgetc( stream ) ) == EOF )
			{
	    		imgErrorString = "Unexpected end of file";
	    		return FXFALSE;
			}
			cookie <<= 8;
			cookie |= c;
	
			if ( (cookie & 0x00FFFF) == 0x0002 ) *type = IMG_TGA32;
			else *type = IMG_UNKNOWN;
			break;
    }
    return FXTRUE;
}


/* _imgReadSbiHeader
**
** Summary: UNPUBLISHED - read header and fill in header info
*/
FxBool _imgReadSbiHeader( FILE *stream, SbiInfo *info )
{
    char buffer[256];
    FxU32 state = 1;
    FxBool done = FXFALSE;
    
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
    while( !done && fgets( buffer, 256, stream ) )
    {
		char *token;
	
		if ( buffer[0] == '#' ) continue;
		for (token = strtok( buffer, " \t\n\r" );
			token != NULL;
			token = strtok( NULL, " \t\n\r" ))
		switch( state )
	    	{
	      		case 1: // YOrigin
					if ( token[1] == '+' ) info->yOrigin = 1;
					else if ( token[1] == '-' )	info->yOrigin = 0;
					else
					{
		    			imgErrorString = "Error parsing y-origin.";
		    			return FXFALSE;
					}
					state++;
					break;
	      		case 2:   // Width
					info->width = atoi( token );
					state++;
					break;
	      		case 3: // height
					info->height = atoi( token );
					state++;
					break;
	     	 	case 4: // 'R'
					if ( token[0] != 'R' )
					{
		    			imgErrorString = "Error parsing R color channel.";
		    			return FXFALSE;
					}
					state++;
					break;
	      		case 5: // Red Channel
					info->redBits = atoi( token );
					state++;
					break;
	      		case 6: // 'G'
					if ( token[0] != 'G' )
					{
		    			imgErrorString = "Error parsing G color channel.";
		    			return FXFALSE;
					}
					state++;
					break;
	      		case 7: // Green Channel
					info->greenBits = atoi( token );
					state++;
					break;
	      		case 8: // 'B'
					if ( token[0] != 'B' )
					{
		    			imgErrorString = "Error parsing B color channel.";
		    			return FXFALSE;
					}
					state++;
					break;
	      		case 9: // Blue Channel                             
					info->blueBits = atoi( token );
					done = FXTRUE;
					state++;
					break;
	      		default:
					break;
	    	}
    }
    
    if ( state < 10 )
    {
		imgErrorString = "Read error before end of header.";
		return FXFALSE;
    }
    
    info->sizeInBytes = info->width * info->height * 4;
    
    return FXTRUE;
}

/* _imgReadP6Header
**
** Summary: UNPUBLISHED - read header and fill in header info
*/
FxBool _imgReadP6Header( FILE *stream,  P6Info *info )
{
    char buffer[256];
    FxU32 state = 1;
    FxBool done = FXFALSE;
    
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
    while( !done && fgets( buffer, 256, stream ) )
    {
		char *token;
	
		if ( buffer[0] == '#' ) continue;
		for (token = strtok( buffer, " \t\n\r" );
			token != NULL;
			token = strtok( NULL, " \t\n\r" ))
	    	switch( state )
	    	{
	     		case 1:   // Width
					info->width = atoi( token );
					state++;
					break;
	      		case 2: // height
					info->height = atoi( token );
					state++;
					break;
	      		case 3: // Color Depth
					info->colorChannelMax = atoi( token );
					if ( info->colorChannelMax != 255 )
					{
		    			imgErrorString = "Unsupported max color value.  Must be 255.";
		    			return FXFALSE;
					}
					state++;
					done = FXTRUE;
					break;
	      		default:
					imgErrorString = "General parse error reading header.";
					return FXFALSE;
					break;
	    	}
    }
    
    if ( state < 4 )
    {
		imgErrorString = "Read error before end of header.";
		return FXFALSE;
    }
    
    info->sizeInBytes = info->width * info->height * 4;
    
    return FXTRUE;
}

/* _imgReadTGAHeader
**
** Summary: UNPUBLISHED - read header and fill in header info
*/
FxBool _imgReadTGAHeader( FILE *stream, TgaInfo *info )
{
    struct {
		FxU8 CMapStartLo;
		FxU8 CMapStartHi;
		FxU8 CMapLengthLo;
		FxU8 CMapLengthHi;
		FxU8 CMapDepth;
		FxU8 XOffSetLo;
		FxU8 XOffSetHi;
		FxU8 YOffSetLo;
		FxU8 YOffSetHi;
		FxU8 WidthLo;
		FxU8 WidthHi;
		FxU8 HeightLo;
		FxU8 HeightHi;
		FxU8 PixelDepth;
		FxU8 ImageDescriptor;
    } partialTGAHeader;
    
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
    if ( fread( &partialTGAHeader, 1, 15, stream ) != 15 )
    {
	  	imgErrorString = "Unexpected end of file.";
		return FXFALSE;
    }
    
    info->width  = partialTGAHeader.WidthHi << 8 | partialTGAHeader.WidthLo;
    info->height = partialTGAHeader.HeightHi << 8 | partialTGAHeader.HeightLo;
    info->yOrigin = 0x01 & partialTGAHeader.ImageDescriptor;
    info->sizeInBytes = info->width * info->height * 4;
    
    return FXTRUE;
}

/* _imgWriteTGAHeader
**
** Summary: UNPUBLISHED - write header
*/
FxBool _imgWriteTGAHeader( FILE *stream, const TgaInfo *info ) {

    struct {
		FxU8 IDLength;
		FxU8 ColorMapType;
		FxU8 ImgType;
		FxU8 CMapStartLo;
		FxU8 CMapStartHi;
		FxU8 CMapLengthLo;
		FxU8 CMapLengthHi;
		FxU8 CMapDepth;
		FxU8 XOffSetLo;
		FxU8 XOffSetHi;
		FxU8 YOffSetLo;
		FxU8 YOffSetHi;
		FxU8 WidthLo;
		FxU8 WidthHi;
		FxU8 HeightLo;
		FxU8 HeightHi;
		FxU8 PixelDepth;
		FxU8 ImageDescriptor;
    } tgaHeader;
    
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
	
	tgaHeader.IDLength         = 0;
	tgaHeader.ColorMapType     = 0;
	tgaHeader.ImgType          = 0x2;
	tgaHeader.CMapStartLo      = 0;
	tgaHeader.CMapStartHi      = 0;
	tgaHeader.CMapLengthLo     = 0;
	tgaHeader.CMapLengthHi     = 0;
	tgaHeader.CMapDepth        = 0;
	tgaHeader.XOffSetLo        = 0;
	tgaHeader.XOffSetHi        = 0;
	tgaHeader.YOffSetLo        = 0;
	tgaHeader.YOffSetHi        = 0;
	tgaHeader.WidthHi          = (FxU8)(info->width >> 8) & 0xFF;
	tgaHeader.WidthLo          = (FxU8)info->width & 0xFF;
	tgaHeader.HeightHi         = (FxU8)(info->height >> 8) & 0xFF;
	tgaHeader.HeightLo         = (FxU8)info->height & 0xFF;
	tgaHeader.PixelDepth       = 32;
	tgaHeader.ImageDescriptor  = info->yOrigin ? ( 0x1 ) : ( 0x0 );	
	    

	if ( fwrite( &tgaHeader, 1, 18, stream ) != 18 ) {
		imgErrorString = "TGA Header stream write error.";
		return FXFALSE;
	}

	return FXTRUE;
}
    
/* _imgReadRGTHeader
**
** Summary: UNPUBLISHED - read header and fill in header info
*/
FxBool _imgReadRGTHeader( FILE *stream, RgtInfo *info )
{
    const FxU16 ITYPE_RLE      = 0x01;
    const FxU16 ITYPE_NCC      = 0x02;
    const FxU16 ITYPE_BGR      = 0x04;
    const FxU16 ITYPE_RGT      = 0x08;
    
    struct {
		FxU8 typeLo;
		FxU8 typeHi;
		FxU8 dimLo;
		FxU8 dimHi;
		FxU8 sizeXLo;
		FxU8 sizeXHi;
		FxU8 sizeYLo;
		FxU8 sizeYHi;
		FxU8 sizeZLo;
		FxU8 sizeZHigh;
    } partialRGTHeader;
    
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
    if ( fread( &partialRGTHeader, 1, 10, stream ) != 10 )
    {
		imgErrorString = "Unexpected end of file.";
		return FXFALSE;
    }
    
    info->width = partialRGTHeader.sizeXHi << 8 | partialRGTHeader.sizeXLo;
    info->height = partialRGTHeader.sizeYHi << 8 | partialRGTHeader.sizeYLo;
    info->sizeInBytes = info->width * info->height * 4;
    info->rle = partialRGTHeader.typeHi & ITYPE_RLE;
    info->bgr = partialRGTHeader.typeHi & ITYPE_BGR;
    info->rgb = partialRGTHeader.typeHi & ITYPE_RGT;
    info->ncc = partialRGTHeader.typeHi & ITYPE_NCC;
    
    return FXTRUE;
}

/* _imgRead3DFHeader
**
** Summary: UNPUBLISHED - read header and fill in header info
*/
FxBool _imgRead3DFHeader( FILE *stream, TdfInfo *info ) {
	FxBool done = FXFALSE;
    char buffer[256];
	FxU32 state = 0;
	float tempFloat;

    if ( stream == NULL ) {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }

    while( !done && fgets( buffer, 256, stream ) ) {
		char *token;
	
		if ( buffer[0] == '#' ) continue;
		for (token = strtok( buffer, " \t\n\r" );
			token != NULL;
			token = strtok( NULL, " \t\n\r" ))
	    	switch( state )	{
				case  0:   // 'f'
					if ( token[0] != 'f' ) {
						imgErrorString = "Bad cookie( \"3df\" ).";
						return FXFALSE;
					}
					state++;
					break;
				case  1:   // Version
					if ( sscanf( token, 
							     "v%f", 
							     &tempFloat ) == 0 ) {	
						imgErrorString = "Couldn't determine version of 3DF file.";
						return FXFALSE;
					}
					info->version = tempFloat;
					state++;
					break;
				case  2:  // Color Format
					if ( (info->colorFormat = _imgTxDecodeColorFormat( token ))==txColorUnknown ) {
						imgErrorString = "3DF Unknown color format.";
						return FXFALSE;
					} 
					state++;
					break;
				case  3:  // "lod"
					if ( strcmp( token, "lod" ) ) {
						imgErrorString = "Bad lod range identifier.";
						return FXFALSE;
					}
					state++;
					break;
				case  4:  // "range:"
					if ( strcmp( token, "range:" ) ) {
						imgErrorString = "Bad lod range identifier.";
						return FXFALSE;
					}
					state++;
					break;
				case  5:  // lodMin
					info->lodMin = atoi( token );
					state++;
					break;
				case  6:  // lodMax
					info->lodMax = atoi( token );
					state++;
					break;
				case  7:  // "aspect"
					if ( strcmp( token , "aspect" ) ) {	
						imgErrorString = "Bad aspect ratio identifier.";
						return FXFALSE;
					}
					state++;
					break;
				case  8:  // "ratio:"
					if ( strcmp( token, "ratio:" ) ) {	
						imgErrorString = "Bad aspect ratio identifier.";
						return FXFALSE;
					}
					state++;
					break;
				case  9:  // aspectWidth
					info->aspectWidth = atoi( token );
					state++;
					break;
				case 10:  // aspectHeight
					info->aspectHeight = atoi( token );
					done = FXTRUE;
					break;
				default:
					imgErrorString = "General parse error reading header.";
					return FXFALSE;
					break;
	    	}
    }
    
    if ( state != 10 ) {
		imgErrorString = "Read error before end of header.";
		return FXFALSE;
    }

	if ( info->lodMin > info->lodMax ) {
		imgErrorString = "3DF Format Error.  lodMin must be <= lodMax.";
		return FXFALSE;
	}

	// Calculate Width and Height with All Mipmaps
	if ( info->aspectWidth < info->aspectHeight ) {
		FxU32 lod;
		info->height = info->lodMax;
		info->width = info->lodMax / info->aspectHeight;
		for ( lod = info->lodMax / 2; lod >= info->lodMin; lod /= 2 ) {
			info->width += lod > 1 ? lod / info->aspectHeight : 1;
		}
	} else {
		FxU32 lod;
		info->width = info->lodMax;
		info->height = info->lodMax / info->aspectWidth;
		for ( lod = info->lodMax / 2; lod >= info->lodMin; lod /= 2 ) {
			info->height += lod > 1 ? lod / info->aspectWidth : 1;
		}
	}

	info->sizeInBytes = info->width * info->height * 4;

    return FXTRUE;
}


/* _imgRead3DFData
**
** Summary: UNPUBLISHED - read data from a 3DF file.
**          when the texuslib has an owner, and it
**          is reliably maintained, this will be
**          converted over to calls to the library.
*/
/* Read in data from a 3DF file. */
FxBool _imgRead3DFData( FILE *stream, const TdfInfo *info, ImgData *data )
{
	FxBool doVerticalDisplay;
	ImgData *destPtr, *destOrigin;
	FxU32 xBound, yBound;
	FxU32 lod;

	struct { 
		FxU8  yRGB[16];
		FxI16 iRGB[4][3];
		FxI16 qRGB[4][3];
	} imgYIQTable;

	if ( stream == NULL ) {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
	}  

  	/* Read in the YIQ decompression table, if necessary. */
  	if( (info->colorFormat == txColorYIQ) || (info->colorFormat == txColorAYIQ_8422) ) {
		FxU32 index;
		for( index=0; index < 16; index++ ) {
            getc( stream );
			imgYIQTable.yRGB[index] = (FxU8) getc( stream );
		}
		for( index=0; index < 4;index++ ) {	
			FxI16 msb = (FxI16) ( getc( stream ) & 0xFF ) << 8;
			FxI16 lsb = (FxI16) ( getc( stream ) & 0xFF );
			imgYIQTable.iRGB[index][0] = msb | lsb;
			msb = (FxI16) ( getc( stream ) & 0xFF ) << 8;
			lsb = (FxI16) ( getc( stream ) & 0xFF );
			imgYIQTable.iRGB[index][1] = msb | lsb;
			msb = (FxI16) ( getc( stream ) & 0xFF ) << 8;
			lsb = (FxI16) ( getc( stream ) & 0xFF );
			imgYIQTable.iRGB[index][2] = msb | lsb;
		}
		for( index=0; index < 4;index++ ) {	
			FxI16 msb = (FxI16) ( getc( stream ) & 0xFF ) << 8;
			FxI16 lsb = (FxI16) ( getc( stream ) & 0xFF );
			imgYIQTable.qRGB[index][0] = msb | lsb;
			msb = (FxI16) ( getc( stream ) & 0xFF ) << 8;
			lsb = (FxI16) ( getc( stream ) & 0xFF );
			imgYIQTable.qRGB[index][1] = msb | lsb;
			msb = (FxI16) ( getc( stream ) & 0xFF ) << 8;
			lsb = (FxI16) ( getc( stream ) & 0xFF );
			imgYIQTable.qRGB[index][2] = msb | lsb;
		}
		if ( feof( stream ) ) {
			imgErrorString = "Unexpected end of file reading YIQ Table.";
			return FXFALSE;
		}
	}

	/* Read in the Image One mipmap level at a time */

	doVerticalDisplay = (info->aspectHeight <= info->aspectWidth ) ? FXTRUE : FXFALSE;

	destPtr = destOrigin = data;
	if ( doVerticalDisplay ) {
		yBound = info->lodMax / info->aspectWidth;
		xBound = info->lodMax;	
	} else { 
		yBound = info->lodMax;
		xBound = info->lodMax / info->aspectHeight;
	}

	for ( lod = info->lodMax; lod >= info->lodMin; lod /= 2 )	{
		FxU32 x, y;
		switch( info->colorFormat )	{
			case txColorTrue:        /* True color, 24-bit color (RGB) + 8-bit alpha.               */
			case txColorARGB_8888:   /* 8-bits of Alpha, Red, Green, and Blue.  Not yet supported.  */
				for ( y = 0; y < yBound; y++ ) {
					for( x = 0; x < xBound; x++ ) {
						FxU8 r, g, b, a;
						a = getc( stream );
						r = getc( stream );
						g = getc( stream );
						b = getc( stream );
						*destPtr++ = b;
						*destPtr++ = g;
						*destPtr++ = r;
						*destPtr++ = a;
					}
					destPtr += (info->width - xBound) * 4;
				}
				break;
			case txColorI_8:         /* 8-bit intensity.                                            */
	        		                 /* For this, an intensity is calculated by averaging the       */
	                		         /* RG and B channels, and placed back into all three channels  */
			                         /* so that outputs of TGA's and PPM's will look okay.          */
				for ( y = 0; y < yBound; y++ ) {
					for ( x = 0; x < xBound; x++ ) {
						FxU8 intensity;
						intensity = getc( stream );
						*destPtr++ = intensity;
						*destPtr++ = intensity;
						*destPtr++ = intensity;
						*destPtr++ = intensity;
					}
					destPtr += (info->width - xBound) * 4;
				}
				break;
			case txColorA_8: /* 8-bit alpha. */
				for ( y = 0; y < yBound; y++ ) {
					for ( x = 0; x < xBound; x++ ) {
						FxU8 alpha;
						alpha = getc( stream );
						*destPtr++ = alpha;
						*destPtr++ = alpha;
						*destPtr++ = alpha;
						*destPtr++ = alpha;
					}
					destPtr += (info->width - xBound) * 4;
				}
				break;
			case txColorARGB_1555:
				for ( y = 0; y < yBound; y++ ) {
					for ( x = 0; x < xBound; x++ ) {
						FxU16 color;
						FxU8  a,r,g,b;
						color = ( 0xFF & getc( stream ) ) << 8;
						color |= ( 0xFF & getc( stream ) );
						a = ( color >> 15 ) & 0x01;
						r = ( color >> 10 ) & 0x1F;
						g = ( color >> 5  ) & 0x1F;
						b =   color         & 0x1F;

						*destPtr++ = _imgMSBReplicate( b, 3, 2);
						*destPtr++ = _imgMSBReplicate( g, 3, 2);
  						*destPtr++ = _imgMSBReplicate( r, 3, 2);
						*destPtr++ = a ? 0x0 : 0xFF;
					}
					destPtr += (info->width - xBound) * 4;
				}
				break;
			case txColorARGB_4444:
				for ( y = 0; y < yBound; y++ ) {
					for ( x = 0; x < xBound; x++ ) {
						FxU16 color;
						FxU8  a,r,g,b;
						color = ( 0xFF & getc( stream ) ) << 8;
						color |= ( 0xFF & getc( stream ) );
						a = ( color >> 12 ) & 0x0F;
						r = ( color >> 8  ) & 0x0F;
						g = ( color >> 4  ) & 0x0F;
						b = ( color >> 0  ) & 0x0F;

						*destPtr++ = _imgMSBReplicate( b, 4, 0 );
						*destPtr++ = _imgMSBReplicate( g, 4, 0 );
  						*destPtr++ = _imgMSBReplicate( r, 4, 0 );
						*destPtr++ = _imgMSBReplicate( a, 4, 0 );
					}
					destPtr += (info->width - xBound) * 4;
				}
				break;
			case txColorRGB_565:
				for ( y = 0; y < yBound; y++ ) {
					for ( x = 0; x < xBound; x++ ) {
						FxU16 color;
						FxU8  r,g,b;
						color = ( 0xFF & getc( stream ) ) << 8;
						color |= ( 0xFF & getc( stream ) );
						r = ( color >> 11 ) & 0x1F;
						g = ( color >> 5 ) &  0x3F;
						b = ( color >> 0 ) &  0x1F;

						*destPtr++ = _imgMSBReplicate( b, 3, 2 );
						*destPtr++ = _imgMSBReplicate( g, 2, 4);
  						*destPtr++ = _imgMSBReplicate( r, 3, 2);
						*destPtr++ = 0x0; // Alpha
					}
					destPtr += (info->width - xBound) * 4;
				}
				break;
			case txColorAI_44:
			default:
				imgErrorString = "Unhandled 3df color format.";
				return FXFALSE;	
		}
		if ( doVerticalDisplay ) { 
			destPtr = destOrigin + ( yBound * info->width * 4 );
		} else {
			destPtr = destOrigin + (xBound * 4);
		}
		destOrigin = destPtr;
		if ( yBound > 1 ) yBound /= 2;
		if ( xBound > 1 ) xBound /= 2;
	}
	
#if 0
	case txColorAI_44:       /* 4-bit alpha, 4-bit intensity.                               */
	                         /* For this conversion, the alpha is shredded to four bits,    */
                                 /* the intensities are averaged and then shredded into four    */
                                 /* bits, and then the intensity is placed into all three       */
                                 /* color channels.                                             */
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Read in both alpha and intensity. */
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor = tmpChar;
	      
	      /* Extract the alpha portion. */
	      Image->AlphaChannel[index] = (tmpColor & 0xf0);

	      /* Extract the color portion. */
	      Image->RedChannel[index] = 
	      Image->GreenChannel[index] = 
	      Image->BlueChannel[index] = ((tmpColor & 0x0f)<<4);
	    }
	  
	  break;

	case txColorYIQ:         /* Narrow-channel, compressed color.                           */
	  /* Get a copy of the YIQ table so that it doesn't have to be dereferenced
	     every single time it's used. */
	  YIQTable = Image->YIQTable;
	  
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Get the YIQ pixel into a temporary value. */
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor = (tmpChar & 0xff);
	      
	      /* Rip out YIQ from that pixel. */
	      y = tmpColor >> 4;
	      i = (tmpColor >> 2) & 0x3;
	      q = (tmpColor >> 0) & 0x3;

	      /* Convert the colors back out. */
	      r = YIQTable.yRGB[y] + YIQTable.iRGB[i][0] + YIQTable.qRGB[q][0];
	      g = YIQTable.yRGB[y] + YIQTable.iRGB[i][1] + YIQTable.qRGB[q][1];
	      b = YIQTable.yRGB[y] + YIQTable.iRGB[i][2] + YIQTable.qRGB[q][2];

	      /* Clamp the values. */
	      if( r < 0 ) r = 0; else if ( r > 0xff ) r = 0xff;
	      if( g < 0 ) g = 0; else if ( g > 0xff ) g = 0xff;
	      if( b < 0 ) b = 0; else if ( b > 0xff ) b = 0xff;

	      /* Stick the values into the image structure. */
	      Image->RedChannel[index] = (unsigned char)r;
	      Image->GreenChannel[index] = (unsigned char)g;
	      Image->BlueChannel[index] = (unsigned char)b;
	      
	      /* Make up something for alpha. */
	      Image->AlphaChannel[index] = 0xff;
	    }

	  break;

	case txColorRGB_332:     /* 3-bit red, 3-bit green, 2-bit blue.                         */
                                 /* Shred bits off of the color channels, and tip the alpha     */
                                 /* channel up to 0xff.                                         */
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Combine the whole color into one byte. */
	      fread(&tmpChar,1,1,ImageFile);

	      /* Rip the byte apart into colors. */
	      Image->RedChannel[index] = (tmpChar & 0xe0);
	      Image->GreenChannel[index] = (tmpChar & 0x1c)<<3;
	      Image->BlueChannel[index] = (tmpChar & 0x03)<<6;

	      /* Make up something for alpha. */
	      Image->AlphaChannel[index] = 0xff;
	    }
	  
	  break;
	case txColorRGB_565:     /* 5-bit red, 6-bit green, 5-bit blue.                         */
                                 /* Shred bits off of the color channels, and tip the alpha     */
                                 /* channel up to 0xff.                                         */
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Read both bytes into a tmp color. */
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor = (tmpChar & 0xff)<<8;
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor |= (tmpChar & 0xff);
	  
	      /* Rip the short apart into colors. */
	      Image->RedChannel[index] = (tmpColor & 0xf800)>>8;
	      Image->GreenChannel[index] = (tmpColor & 0x07e0)>>3;
	      Image->BlueChannel[index] = (tmpColor & 0x001f)<<3;
	      
	      /* Make up something for alpha. */
	      Image->AlphaChannel[index] = 0xff;
	    }
	  
	  break;
	case txColorARGB_8332:   /* 8-bit alpha, 3-bit red, 3-bit green, 2-bit blue.            */
                                 /* Nothing but shredding.                                      */
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Read both bytes into a tmp color. */
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor = (tmpChar & 0xff)<<8;
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor |= (tmpChar & 0xff);
	      
	      /* Read out the alpha. */
	      Image->AlphaChannel[index] = ((tmpColor & 0xff00)>>8);

	      /* Rip the byte apart into colors. */
	      Image->RedChannel[index] = (tmpColor & 0xe0);
	      Image->GreenChannel[index] = (tmpColor & 0x1c)<<3;
	      Image->BlueChannel[index] = (tmpColor & 0x03)<<6;
	    }
	  
	  break;
	case txColorARGB_1555:   /* 1-bit alpha, 5-bit red, 5-bit green, 5-bit blue.            */
	                         /* Nothing but shredding.                                      */
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Read both bytes into a tmp color. */
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor = (tmpChar & 0xff)<<8;
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor |= (tmpChar & 0xff);

	      /* Rip the color apart. */
	      Image->AlphaChannel[index] = (tmpColor & 0x8000)>>8;
	      Image->RedChannel[index] = (tmpColor & 0x7c00)>>7;
	      Image->GreenChannel[index] = (tmpColor & 0x03e0)>>2;
	      Image->BlueChannel[index] = (tmpColor & 0x001f)<<3;
	    }
	  
	  break;

	case txColorAYIQ_8422:   /* 8-bit alpha + narrow-channel compressed color.              */
	  /* Get a copy of the YIQ table so that it doesn't have to be dereferenced
	     every single time it's used. */
	  YIQTable = Image->YIQTable;
	  
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Get alpha from the first byte. */
	      fread(&tmpChar,1,1,ImageFile);
	      (Image->AlphaChannel[index]) = (unsigned char)tmpChar;
	      
	      /* Get the YIQ pixel into a temporary value. */
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor = (tmpChar & 0xff);

	      /* Rip out YIQ from that pixel. */
	      y = tmpColor >> 4;
	      i = (tmpColor >> 2) & 0x3;
	      q = (tmpColor >> 0) & 0x3;

	      /* Convert the colors back out. */
	      r = YIQTable.yRGB[y] + YIQTable.iRGB[i][0] + YIQTable.qRGB[q][0];
	      g = YIQTable.yRGB[y] + YIQTable.iRGB[i][1] + YIQTable.qRGB[q][1];
	      b = YIQTable.yRGB[y] + YIQTable.iRGB[i][2] + YIQTable.qRGB[q][2];

	      /* Clamp the values. */
	      if( r < 0 ) r = 0; else if ( r > 0xff ) r = 0xff;
	      if( g < 0 ) g = 0; else if ( g > 0xff ) g = 0xff;
	      if( b < 0 ) b = 0; else if ( b > 0xff ) b = 0xff;

	      /* Stick the values into the image structure. */
	      Image->RedChannel[index] = (unsigned char)r;
	      Image->GreenChannel[index] = (unsigned char)g;
	      Image->BlueChannel[index] = (unsigned char)b;
	      
	      /* Make up something for alpha. */
	      Image->AlphaChannel[index] = 0xff;
	    }

	  break;
	case txColorARGB_4444:   /* 4-bit alpha, 4-bit red, 4-bit green, 4-bit blue.            */
	                         /* Nothing but shredding.                                      */
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Read both bytes into a tmp color. */
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor = (tmpChar & 0xff)<<8;
	      fread(&tmpChar,1,1,ImageFile);
	      tmpColor |= (tmpChar & 0xff);

	      /* Rip that color apart. */
	      Image->AlphaChannel[index] = (tmpColor & 0xf000)>>8;
	      Image->RedChannel[index] = (tmpColor & 0x0f00)>>4;
	      Image->GreenChannel[index] = (tmpColor & 0x00f0);
	      Image->BlueChannel[index] = (tmpColor & 0x000f)<<4;
	    }
	  
	  break;
	case txColorAI_88:       /* 8-bit alpha, 8-bit itensity.                                */
	                         /* Same as txColorAI_44, minus the shredding.                  */
	  /* Loop through the entire image. */
	  for(index=0;index<((Image->Width)*(Image->Height));index++)
	    {
	      /* Grab alpha from the first byte. */
	      fread(&tmpChar,1,1,ImageFile);
	      (Image->AlphaChannel[index]) = (tmpChar & 0xff);
	      
	      /* And grab the intensity from the second byte. */
	      fread(&tmpChar,1,1,ImageFile);
	      Image->RedChannel[index] =
	      Image->GreenChannel[index] =
	      Image->BlueChannel[index] = (tmpChar & 0xff);
	    }
	  
	  break;
	}
    }
  
  /* Close the file. */
  fclose(ImageFile);
#endif

  return FXTRUE;
}



/* _imgReadTGAData
**
** Summary: UNPUBLISHED - read data from a TGA file
** TGA is BGRA
*/
FxBool _imgReadTGAData( FILE *stream, const TgaInfo *info, ImgData *data )
{
	FxI32 y, y0, y1, dy, scanlineLength;
	const int bytesPerPixel = 4;

    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }

	if ( info->height == 0 ) return FXTRUE;

	if ( info->yOrigin == 0 )
	{
		dy = -1;
		y0 = info->height - 1;
		y1 = -1;
	}
	else
	{
		dy = 1;
		y0 = 0;
		y1 = info->height;
	}

	scanlineLength = info->width * bytesPerPixel;

	for ( y = y0; y != y1; y += dy )
	{
		if ( fread( data + scanlineLength * y, 1, scanlineLength, stream ) !=
			 (FxU32) scanlineLength )
		{
			imgErrorString = "Unexpected end of file";
			return FXFALSE;
		}
	}
    
    return FXTRUE;
}



/* _imgWriteTGAData
**
** Summary: UNPUBLISHED - write data out to a tga
** TGA is BGRA
*/
FxBool _imgWriteTGAData( FILE *stream, const TgaInfo *info, const ImgData *data )
{
	FxI32 y, scanlineLength;
	const int bytesPerPixel = 4;

    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }

	scanlineLength = info->width * bytesPerPixel;

	for ( y = info->height - 1; y >= 0; y-- )
	{
		if ( fwrite( data + scanlineLength * y, 1, scanlineLength, stream ) !=
			 (FxU32) scanlineLength )
		{
			imgErrorString = "TGA stream write error.";
			return FXFALSE;
		}
	}
    
    return FXTRUE;
}

/* _imgReadRGTData
**
** Summary: UNPUBLISHED - read data from an RGT file ( rgt's are bottom up )
** RGT is RGBA in memory (low byte to high byte), or ABGR in a register
*/
FxBool _imgReadRGTData( FILE *stream, const RgtInfo *info, ImgData *data )
{
	FxU32 scanlineSize = info->width * 4;
	FxU32 x,y;
    
    if ( stream == NULL ) {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }

	if ( info->ncc != 0 ) {	
		imgErrorString = "Compressed RGT's not yet supported.";
		return FXFALSE;
	}

	// load rgt, rgt's are bottom up
	for ( y = 0; y < info->height; y++ ) {
		ImgData *scanlineBuffer = data + scanlineSize * ( (info->height-1) - y );
		if ( fread( scanlineBuffer, 1, scanlineSize, stream ) != scanlineSize )	{
			imgErrorString = "Unexpected end of file.";
			return FXFALSE;
		}
	}

	if ( info->rgb ) {
		fprintf(stderr," (RGB->BGR)"); fflush(stderr);
		for ( y = 0; y < info->height; y++ ) {
			for ( x = 0; x < info->width; x++ ) {
				ImgData swap;
				swap = data[2];
				data[2] = data[0];
				data[0] = swap;
				data += 4;
			}
		}

	}

    return FXTRUE;
}

/* _imgReadSbiData
**
** Summary: UNPUBLISHED - read data from an SBI file
*/
FxBool _imgReadSbiData( FILE *stream, const SbiInfo *info, ImgData *data )
{
    FxI32 y, y0, y1, dy;
    FxU32 x;
    FxU32 rShift, gShift, bShift, rMask, gMask, bMask;
	FxU32 rRightShift, rLeftShift;
	FxU32 gRightShift, gLeftShift;
	FxU32 bRightShift, bLeftShift;
	FxU32 scanlineWidth;
	ImgData *baseOfData;
    
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
    bShift = 0;
    gShift = info->blueBits;
    rShift = gShift + info->greenBits;

    rMask = (FxU32) ~0;
    rMask >>= sizeof( FxU32 ) * 8 - info->redBits;
	rLeftShift = 8 - info->redBits;
	rRightShift = info->redBits - rLeftShift;
    
    gMask = (FxU32) ~0;
    gMask >>= sizeof( FxU32 ) * 8 - info->greenBits;
	gLeftShift = 8 - info->greenBits;
	gRightShift = info->greenBits - gLeftShift;
    
    bMask = (FxU32) ~0;
    bMask >>= sizeof( FxU32 ) * 8 - info->blueBits;
	bLeftShift = 8 - info->blueBits;
	bRightShift = info->blueBits - bLeftShift;

	scanlineWidth = info->width * 4;
	baseOfData = data;

    if ( info->yOrigin == 0 )
    {
		y0 = info->height - 1;
		y1 = -1;
		dy = -1;
    }
    else
    {
		y0 = 0;
		y1 = info->height;
		dy = 1;
    }
    
    for( y = y0; y != y1; y += dy )
    {
		//point data at apropriate scanline
		data = baseOfData + y * scanlineWidth;

		for ( x = 0; x < info->width; x++ )
		{
	    	int r, g, b;
	    	int inchar;
	    	FxU32 color;
	    
	    	color = getc( stream );
	    	if ( (inchar = getc( stream ))==EOF )
	    	{
				imgErrorString = "Unexpected end of file.";
				return FXFALSE;
	    	}
	    	color |= (inchar << 8);
	    
	    	// Get sig bits
	    	r = (color >> rShift ) & rMask;
	    	g = (color >> gShift ) & gMask;
	    	b = (color >> bShift ) & bMask;
	    
	    	// Expand to 8 bits/color
			// Current implementation is only meaningful if 
			// using more than 4Bits/Channel color precision.....
			// Needs to be fixed, but this is much, much faster....
	    	*data++ = _imgMSBReplicate( b, bLeftShift, bRightShift );
	    	*data++ = _imgMSBReplicate( g, gLeftShift, gRightShift );
	    	*data++ = _imgMSBReplicate( r, rLeftShift, rRightShift );
	    	data++; // Blank A
		}
    }
    return FXTRUE;
}

/* _imgReadP6Data
**
** Summary: UNPUBLISHED - read data from a PPM
** PPM is RGB
** Memory format is B - G - R - A
*/
FxBool _imgReadP6Data( FILE *stream, const P6Info *info, ImgData *data )
{
    FxU32 numPixels;
    
    numPixels = info->width * info->height;

    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
	// Read in image data
    for( ;numPixels;numPixels-- )
    {
		int r, g, b;
		r = getc( stream );
		g = getc( stream );
		b = getc( stream );
		if ( b == EOF )
		{
	    	imgErrorString = "Unexpected end of file.";
	    	return FXFALSE;
		}
		*data++ = (ImgData)b;
		*data++ = (ImgData)g;
		*data++ = (ImgData)r;
		data++; // A
    }

    return FXTRUE;
}

/* _imgReadSRLEHeader
**
** Summary: UNPUBLISHED - read header and fill in header info
** The simple rle format is intended for glide test programs image comparison
** The file format is as follows:
** 4 bytes file signature
** 2 bytes image width
** 2 bytes image height
** 1 byte  image depth
** 1 byte  compression method (1 for rle compression)
** image data
**
** The image data started with a control byte and followed by pixel data
** Each control byte has two parts. Bit 7 (msb) equals to 1 indicates a run,
** 0 indicates a literal string. Bit 6 - 0 id the counter. It is zero based.
*/
FxBool _imgReadSRLEHeader( FILE *stream,  SrleInfo *info )
{
    FxU16 tmp16;
    FxU8  tmp8;
    
    if ( stream == NULL )
    {
      imgErrorString = "Bad file handle.";
      return FXFALSE;
    }
    
    fread(&tmp16, sizeof(FxU16), 1, stream);
    info->width = tmp16;
    fread(&tmp16, sizeof(FxU16), 1, stream);
    info->height = tmp16;
    fread(&tmp8, sizeof(FxU8), 1, stream);
    info->pixelsize = tmp8 / 8;
    fread(&tmp8 , sizeof(FxU8), 1, stream);
    info->c_type = tmp8;
    
    info->sizeInBytes = info->width * info->height * 4;
    
    return FXTRUE;
}

/* _imgReadSRLE
**
** Summary: UNPUBLISHED - read data from a simple rle format
*/
FxBool _imgReadSRLEData( FILE *stream, const SrleInfo *info, ImgData *data )
{
    int   numPixels;
    FxU16 color;
    FxU8  r = 0, g = 0, b = 0, a = 0;
    FxU8  ctl, repeat, run;
    FxBool flag;
    
    numPixels = info->width * info->height;

    if ( stream == NULL )
    {
      imgErrorString = "Bad file handle.";
      return FXFALSE;
    }

    while (numPixels) {
      ctl = getc( stream );
      flag = ctl & 0x80;
      run = repeat = (ctl & 0x7f) + 1;
      numPixels -= repeat;
      while (repeat) {
	switch (info->pixelsize) 
	  {
	  case 2:
	    if (((flag) && (repeat == run)) || (!flag))
	      fread(&color,2,1,stream);
	    r = ( color >> 11 ) & 0x1F;
	    g = ( color >> 5 ) &  0x3F;
	    b = ( color >> 0 ) &  0x1F;

	    *data++ = _imgMSBReplicate( b, 3, 2 );
	    *data++ = _imgMSBReplicate( g, 2, 4);
	    *data++ = _imgMSBReplicate( r, 3, 2);
	    *data++ = 0x0;
	    break;
	  case 4:
	    if (((flag) && (repeat == run)) || (!flag)) {
	      a = getc( stream );
	      r = getc( stream );
	      g = getc( stream );
	      b = getc( stream );
	    }
	    *data++ = b;
	    *data++ = g;
	    *data++ = r;
	    *data++ = a;
	    break;
	  }
	repeat--;
      }
      if (numPixels < 0)
	return FXFALSE;
    }
    return FXTRUE;
}

/* _imgWriteP6Header
**
** Summary: UNPUBLISHED - write out formatted header for a P6
*/
FxBool _imgWriteP6Header( FILE *stream, const P6Info *info )
{
	imgErrorString = "Image write error.";
	if ( 0 > fprintf( stream, "P6\n" ) ) return FXFALSE;
	if ( 0 > fprintf( stream, "# PPM Comment\n" ) ) return FXFALSE;
	if ( 0 > fprintf( stream, "%u ", info->width ) ) return FXFALSE;
	if ( 0 > fprintf( stream, "%u\n", info->height ) ) return FXFALSE;
	if ( 0 > fprintf( stream, "255\n" ) ) return FXFALSE;
	imgErrorString = "No error.";
	return FXTRUE;
}

/* _imgWriteSbiHeader
**
** Summary: UNPUBLISHED - write out formatted header for an SBI
*/
FxBool _imgWriteSbiHeader( FILE *stream, const SbiInfo *info )
{
	imgErrorString = "Image write error.";
	if ( 0 > fprintf( stream, "P9\n" ) ) return FXFALSE;
	if ( 0 > !fprintf( stream, "Y%c\n", info->yOrigin? '+' : '-' ) ) return FXFALSE;
	if ( 0 > !fprintf( stream, "%u ", info->width ) ) return FXFALSE;
	if ( 0 > !fprintf( stream, "%u\n", info->height ) ) return FXFALSE;
	if ( 0 > !fprintf( stream, "R %u ", info->redBits ) ) return FXFALSE;
	if ( 0 > !fprintf( stream, "G %u ", info->greenBits ) ) return FXFALSE;
	if ( 0 > !fprintf( stream, "B %u\n", info->blueBits ) ) return FXFALSE;
	imgErrorString = "No Error.";
	return FXTRUE;
}

/* _imgWriteP6Data
**
** Summary: UNPUBLISHED - write P6 image data
*/
FxBool _imgWriteP6Data( FILE *stream, const P6Info *info, const ImgData *data )
{
	FxU32 dataSize = info->width * info->height * 4;
	const ImgData *dataEnd  = data + dataSize;

	while( data < dataEnd )
	{
		ImgData r, g, b;
		b = *data++;
		g = *data++;
		r = *data++;
		data++;

		putc( r, stream );
		putc( g, stream );
		if ( putc( b, stream ) == EOF )
		{
			imgErrorString = "Image write error.";
			return FXFALSE;
		}
	}
	return FXTRUE;
}

/* _imgWriteSbiData
**
** Summary: UNPUBLISHED - write sbi image data
*/
FxBool _imgWriteSbiData( FILE *stream, const SbiInfo *info, const ImgData *data )
{
	// convert each pixel, then write it out as two byte-writes
	FxU32 x;
	FxU32 y;
	FxU32 sbiPixel;

	FxI32 scanlineIncrement;

	if ( info->yOrigin == 0 ) // Lower
	{
		scanlineIncrement = (4 * (-(long)info->width)) + 1;
		// point at beginning of last SL
		data = data + (( info->height - 2 ) * info->width * 4);
	}
	else // upper
	{
		scanlineIncrement = 0;
	}

	imgErrorString = "Image write error.";
	for( y = 0; y < info->height; y++ )
	{
		for( x = 0; x < info->width; x++ )
		{
			FxU32 r, g, b;
			b =  *data++ & 0xF8;
			g =  *data++ & 0xFC;
			r =  *data++ & 0xF8;
			data++;
			sbiPixel = (r << 8) | (g << 3) | (b >> 3);
			
			if ( putc( sbiPixel, stream ) == EOF ) return FXFALSE;
			if ( putc( sbiPixel >> 8, stream ) == EOF ) return FXFALSE;
		}
		data += scanlineIncrement;
	}
	imgErrorString = "No error.";
	return FXTRUE;
}


/* _imgWriteSbiDataWide
**
** Summary: UNPUBLISHED - write sbi image data
*/
FxBool _imgWriteSbiDataWide( FILE *stream, const SbiInfo *info, const ImgData *data, FxU32 slWidth )
{
	// convert each pixel, then write it out as two byte-writes
	FxU32 x;
	FxU32 y;
	FxU32 sbiPixel;

	FxI32 scanlineIncrement;

	if ( info->yOrigin == 0 ) // Lower
	{
		scanlineIncrement = (4 * slWidth) + 1;
		// point at beginning of last SL
		data = data + (( info->height - 2 ) * info->width * 4);
	}														 
	else // upper
	{
		scanlineIncrement = (slWidth * 4) - (4*info->width);
	}

	imgErrorString = "Image write error.";
	for( y = 0; y < info->height; y++ )
	{
		for( x = 0; x < info->width; x++ )
		{
			FxU32 r, g, b;
			b =  *data++ & 0xF8;
			g =  *data++ & 0xFC;
			r =  *data++ & 0xF8;
			data++;

			sbiPixel = (r << 8) | (g << 3) | (b >> 3);
			
			if ( putc( sbiPixel, stream ) == EOF ) return FXFALSE;
			if ( putc( sbiPixel >> 8, stream ) == EOF ) return FXFALSE;
		}
		data += scanlineIncrement;
	}
	imgErrorString = "No error.";
	return FXTRUE;
}


/*
** PUBLIC INTERFACE
*/

/*
** PUBLIC DATA
*/

const char *imgTypeString[] = 
{ 
	"UNKNOWN", 
	"SBI", 
	"Binary PPM", 
	"3DF", 
	"RGT", 
	"TGA32" 
};

const char *imgTxColorFormatNames[] =
{
  "unknown",
  "true",
  "i8",
  "a8",
  "ai44",
  "yiq",
  "rgb332",
  "rgb565",
  "argb8332",
  "argb1555",
  "ayiq8422",
  "argb4444",
  "ai88",
  "argb8888"
};


/* imgGetErrorString
**
** Summary: Get a pointer to the buffer that contains a string describing
**          the most recent error in the  Image Library
**
** Returns: Point to an error description string.  Probably a bad idea to write
**         to this address as it will likely be a static string.
*/
const char *imgGetErrorString( void )
{
    return imgErrorString;
}

/* imgReadInfo
**
** Summary: Fill the Info union and return the proper image type
**
** Parameters: info - pointer to an ImgInfo structure including type
**             stream - pointer file stream
**
** Returns: FXTRUE if header read in correctly.
**          FXFALSE if header not read, error described
**                  in string obtainable by sbiGetErrorString();
*/
FxBool imgReadInfo( FILE *stream, ImgInfo *info )
{
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
	_fixStream( stream );
	info->any.data = NULL;

    // Determine the Type of File and Fill The Appropriate Header Structure
    if ( _imgGuessType( stream, &info->any.type ) == FXFALSE ) return FXFALSE;

    switch( info->any.type )
    {
    	case IMG_SBI:
			return _imgReadSbiHeader( stream, &(info->sbiInfo) );
      	case IMG_RGT:
			return _imgReadRGTHeader( stream, &(info->rgtInfo) );
      	case IMG_TGA32:
			return _imgReadTGAHeader( stream, &(info->tgaInfo) );
    	case IMG_P6:
			return _imgReadP6Header( stream, &(info->p6Info) );
      	case IMG_3DF:
			return _imgRead3DFHeader( stream, &(info->tdfInfo) );
      	case IMG_SRLE:
			return _imgReadSRLEHeader( stream, &(info->srleInfo) );
      	case IMG_UNKNOWN:
      	default:
			imgErrorString = "Unknown image file format.";
			break;
    }
	return FXFALSE;
}

/* imgReadData
**
** Summary: Read all of the image data from an image stream given
**          that the header has already been read by imgReadInfo.
**
** Parameters: stream - a readable file stream
**             info - pointer to filled info structure (except data)
**					- if data is NULL then we malloc data large enough
** Returns: FXTRUE if data was read successfully
**          FXFALSE if an error occured, reason may be
**          obtained with imgGetErrorString().
*/
FxBool imgReadData( FILE *stream, ImgInfo *info )
{
    if ( stream == NULL )
    {
		imgErrorString = "Bad file handle.";
		return FXFALSE;
    }
    
	_fixStream( stream );

	// if data pointer is NULL then malloc something big enough
    if ( info->any.data == NULL ) {
		info->any.data = (ImgData*) malloc( info->any.sizeInBytes );
	    if ( info->any.data == NULL ) {
			imgErrorString = "Malloc failed.";
			return FXFALSE;
		}
    }

    switch( info->any.type )
    {
    	case IMG_TGA32:
			return _imgReadTGAData( stream, &(info->tgaInfo), info->any.data );
    	case IMG_RGT:
			return _imgReadRGTData( stream, &(info->rgtInfo), info->any.data );
    	case IMG_3DF:
			return _imgRead3DFData( stream, &(info->tdfInfo), info->any.data );
    	case IMG_SBI:
			return _imgReadSbiData( stream, &(info->sbiInfo), info->any.data );
    	case IMG_P6:
			return _imgReadP6Data( stream, &(info->p6Info), info->any.data );
    	case IMG_SRLE:
			return _imgReadSRLEData( stream, &(info->srleInfo), info->any.data );
    	case IMG_UNKNOWN:
      default:
		imgErrorString = "Unknown file type.";
      break;
    }
	return FXFALSE;
}

/* imgWriteImage
**
** Summary: Write out an image file in the format specified by *type.
**
** Parameters: stream - a writeable file stream
** 		       info - pointer to initialized image info struct appropriate
**                       to type described in *type
** 		       type - desired image type to write
** 		       data - pointer to 32BPP xRGB data in linear memory storage
**
** Returns: FXTRUE if data was written sucessfully
**          FXFALSE if an error occured, reason may be
**          obtained with imgGetErrorString().
*/


FxBool imgWriteImage( FILE *stream, const ImgInfo *info, const ImgType type, const ImgData *data )
{
	// Try to catch obvious bugs.
	if ( stream == NULL )
	{
		imgErrorString = "Bad file handle.";
		return FXFALSE;
	}

	_fixStream( stream );

	if ( data == NULL )
	{
		imgErrorString = "Bad data pointer.";
		return FXFALSE;
	}

	switch( type )
	{
		case IMG_SBI:
			if ( !_imgWriteSbiHeader( stream, &(info->sbiInfo) ) )
			{
				imgErrorString = "Couldn't write SBI info.";
				return FXFALSE;
			}
			return _imgWriteSbiData( stream, &(info->sbiInfo), data );
		case IMG_P6:
			if ( !_imgWriteP6Header( stream, &(info->p6Info)) )
			{
				imgErrorString = "Couldn't write P6 info.";
				return FXFALSE;
			}
			return _imgWriteP6Data( stream, &(info->p6Info), data );
		case IMG_RGT:
			imgErrorString = "RGT writes unimplemented.";
			break;
		case IMG_TGA32:
			if ( !_imgWriteTGAHeader( stream, &(info->tgaInfo)) )
			{
				imgErrorString = "Couldn't write tga info.";
				return FXFALSE;
			}
			return _imgWriteTGAData( stream, &(info->tgaInfo), data );
			break;
		case IMG_UNKNOWN:
		default:
			imgErrorString = "Can't write unknown format.";
			break;
	}
	
	return FXFALSE;
}

/* imgWriteImageFromWideBuffer
**
** Summary: Write out an image file in the format specified by *type give a pointer
**          to an oversized buffer and a scanline width.  This is in here strictly for
**          the c-simulator, and isn't published in the header file.
**
** Parameters: stream - a writeable file stream
** 		       type - pointer to image desired image type to write
** 		       info - pointer to initialized image info struct appropriate
**                       to type described in *type
** 		       data - pointer to 32BPP xRGB data in linear memory storage
**             scanlineWidth - width of scanline in long-words.
**
** Returns: FXTRUE if data was written sucessfully
**          FXFALSE if an error occured, reason may be
**          obtained with imgGetErrorString().
*/


FxBool imgWriteImageFromWideBuffer( FILE *stream, const ImgInfo *info, 
									const ImgData *data, FxU32 scanlineWidth )
{
	// Try to catch obvious bugs.
	if ( stream == NULL )
	{
		imgErrorString = "Bad file handle.";
		return FXFALSE;
	}

	_fixStream( stream );

	if ( data == NULL )
	{
		imgErrorString = "Bad data pointer.";
		return FXFALSE;
	}

	if ( scanlineWidth < info->any.width )
	{
		imgErrorString = "Output buffer is too small.";
		return FXFALSE;
	}

	switch( info->any.type )
	{
		case IMG_SBI:
			if ( !_imgWriteSbiHeader( stream, &(info->sbiInfo) ) )
			{
				imgErrorString = "Couldn't write SBI info.";
				return FXFALSE;
			}
			return _imgWriteSbiDataWide( stream, &(info->sbiInfo), data, scanlineWidth );
		case IMG_P6:
			imgErrorString = "P6 writes unimplemented.";
			break;
		case IMG_RGT:
			imgErrorString = "RGT writes unimplemented.";
			break;
		case IMG_TGA32:
			imgErrorString = "TGA32 writes unimplemented.";
			break;
		case IMG_UNKNOWN:
		default:
			imgErrorString = "Can't write unknown format.";
			break;
	}
	
	return FXFALSE;
}

// returns a 3-letter abbreviation for the image type
const char *imgTypeName( const ImgInfo *info )
{
	switch( info->any.type ) {
	    case IMG_UNKNOWN:	return "???";
	    case IMG_P6:		return "ppm";
	    case IMG_SBI:		return "sbi";
	    case IMG_3DF:		return "3df";
	    case IMG_TGA32:		return "tga";
	    case IMG_SRLE:		return "srle";
	    case IMG_RGT:
	       	if (info->rgtInfo.rle) return "rle";
	    	if (info->rgtInfo.bgr) return "bgr";
	    	if (info->rgtInfo.rgb) return "rgb";
	    	if (info->rgtInfo.ncc) return "ncc";

		default:			return "ERR";
    }
}

//----------------------------------------------------------------------
// high level routine: reads an image file thru a search path
// exits upon failure, prints out trace messages
//----------------------------------------------------------------------
FxBool imgReadFile(const char *filename, ImgInfo *info)
{
	const char *prefix;
	FILE *file;

	file = fxFopenPath(filename,READ_ATTRIBS, getenv(IMAGE_PATH), &prefix);
	if (file == NULL) {
		fprintf(stderr,"Error: can't open input file %s\n",filename);
		exit(2);
	}
	if ( imgReadInfo( file, info ) == FXFALSE ) {
		fprintf(stderr,"Error: reading info for %s, %s\n",filename,imgGetErrorString());
		exit(2);
	}
	fprintf(stderr,"Loading %s image file ", imgTypeName(info));
	if (prefix) {					// if there's a path prefix
	    char buf[1024], *p;
	    strcpy(buf,prefix);			// copy and replace semicolon
	    if ((p = strchr(buf,';')) != NULL) *p = '\0';
	    fprintf(stderr,buf);
	    fprintf(stderr,"/");
	}
	fprintf (stderr,"%s (%ux%u) ...", filename, info->any.width,info->any.height);
	fflush(stderr);

	if ( imgReadData( file, info ) == FXFALSE ) {
		fprintf(stderr, "\nError: reading data for %s, %s\n",filename,imgGetErrorString());
		exit(4);
	}
	fclose(file);
	fprintf(stderr," done.\n");
	fflush(stderr);
	return FXTRUE;
}

//----------------------------------------------------------------------
// high level routine: writes an image file
// exits upon failure, prints out trace messages
//----------------------------------------------------------------------
FxBool imgWriteFile(const char *filename, const ImgInfo *info, const ImgType type, const ImgData *data)
{
    FxBool retval;
    ImgInfo tempInfo = *info;		// make a local copy
    FILE *file;

    file = fopen(filename,WRITE_ATTRIBS);
    if (file == NULL) {
	fprintf(stderr,"Error: can't open output file %s\n",filename);
	exit(2);
    }
    tempInfo.any.type = type;		// set the new type
    fprintf(stderr,"Storing %s image file %s (%ux%u) ...",
		imgTypeName(&tempInfo), filename, info->any.width,info->any.height);
    fflush(stderr);

    retval = imgWriteImage( file, info, type, data );
    fclose(file);
    fprintf(stderr,retval ? " done.\n" : " aborted.\n");
    fflush(stderr);
    return retval;
}
