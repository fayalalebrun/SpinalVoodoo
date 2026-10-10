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
#include <conio.h>
#include <glide.h>
#include <texus.h>

/*
 * VIEWPPM
 * This is a simple program to show how to use the Texus library
 * to convert textures from an in memory format to a format
 * that Glide can handle for texture downloads.
 */

typedef struct
{
  unsigned char b, g, r, a;
} Color32;

/*
 * This structure contains the factors to multiply normalized [0.0, 1.0]
 * texture coordinates by in order to get the texture coordinates that
 * Voodoo Graphics expect.
 */
typedef struct
{
  float sMult;
  float tMult;
} TexCoordFactors;

/*
 * An array which is indexed by GrAspectRatio_t to convert from normalized
 * texture coordinates to Voodoo texture coordinates.
 */
TexCoordFactors aspectToTexCoordFactors[7] = {
  { 255.0f,        255.0f / 8.0f },  /* GR_ASPECT_8x1 */
  { 255.0f,        255.0f / 4.0f },  /* GR_ASPECT_4x1 */
  { 255.0f,        255.0f / 2.0f },  /* GR_ASPECT_2x1 */
  { 255.0f,        255.0f        },  /* GR_ASPECT_1x1 */
  { 255.0f / 2.0f, 255.0f        },  /* GR_ASPECT_1x2 */
  { 255.0f / 4.0f, 255.0f        },  /* GR_ASPECT_1x4 */
  { 255.0f / 8.0f, 255.0f        }   /* GR_ASPECT_1x8 */
};

/*
 * Do whatever is necessary to get Glide going.
 */
void InitGlide( void )
{
  static GrHwConfiguration hwconfig;

  grGlideInit();

  if ( !grSstQueryHardware( &hwconfig ) )
    {
      fprintf( stderr, "grQuery failed!" );
      exit( -1 );
    }

  grSstSelect( 0 );

  if ( !grSstWinOpen( 0,
                   GR_RESOLUTION_640x480,
                   GR_REFRESH_60Hz, 
                   GR_COLORFORMAT_ABGR,
                   GR_ORIGIN_UPPER_LEFT, 
                   2, 0 ) )
    {
      fprintf( stderr, "grOpen failed!" );
      exit( -1 );
    }

  grTexCombineFunction( GR_TMU0, GR_TEXTURECOMBINE_DECAL );
  guColorCombineFunction( GR_COLORCOMBINE_DECAL_TEXTURE );
  grCullMode( GR_CULL_NEGATIVE );
}

int ValidateImageDimensions( int width, int height )
{
  switch( width )
    {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
      break;
    default:
      fprintf( stderr, "Invalid image width: %d\n", width );
      return 0;
    }

  switch( height )
    {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
      break;
    default:
      fprintf( stderr, "Invalid image height: %d\n", height );
      return 0;
    }

  if( ( ( width / height ) > 8 ) || ( ( height / width ) > 8 ) )
    {
      fprintf( stderr, "Invalid aspect ratio, must one of:\n" );
      fprintf( stderr, "1:8, 1:4, 1:2, 1:1, 2:1, 4:1, 8:1\n" );
      return 0;
    }
  return 1;
}

/*
 * Get the dimensions of a PPM file.
 * returns 1 if successful, 0 otherwise.
 */
int PPMGetDimensions( const char *filename, int *width, int *height )
{
  FILE *fp;
  char buf[200];

  /*
   * Open the PPM file.
   */
  if( !( fp = fopen( filename, "rb" ) ) )
    return 0;

  /*
   * Read the PPM header.
   */

  /* Skip the P6 */
  fgets( buf, 200, fp );

  /* REad the dimensions */
  fgets( buf, 200, fp );
  sscanf( buf, "%d %d", width, height );

  fclose( fp );

  return 1;
}

/*
 * LoadPPM
 * Allocate memory for an image, load a ppm file into it.
 * Not the most efficient loader because it reads directly
 * into a 8888 instead of an 888 without allocating any
 * extra memory.
 */
Color32 *PPMLoad( const char *filename )
{
  Color32 *out;
  char buf[200];
  FILE *fp;
  int width, height;
  long i;

  /*
   * Open the PPM file.
   */
  if( !( fp = fopen( filename, "rb" ) ) )
    return NULL;

  /*
   * Read the PPM header.
   */

  /* Skip the P6 */
  fgets( buf, 200, fp );

  /* REad the dimensions */
  fgets( buf, 200, fp );
  sscanf( buf, "%d %d", &width, &height );

  /* Skip the maxval line. . . assume that it is 255. */
  fgets( buf, 200, fp );

  /*
   * Allocate memory for the output image.
   */
  out = ( Color32 * )malloc( sizeof( Color32 ) * ( long )width * ( long )height );

  if( !out )
    {
      fclose( fp );
      return NULL;
    }

  /*
   * Read in each texel. . .fill in the alpha with a totally
   * opaque value.
   */
  for( i = 0; i < ( long )width * ( long )height; i++ )
    {
      fread( &out[i].r, 1, 1, fp );
      fread( &out[i].g, 1, 1, fp );
      fread( &out[i].b, 1, 1, fp );
      out[i].a = 0xff;
    }

  fclose( fp );

  return out;
}


int main( int argc, char **argv )
{
  Color32 *raw_image;
  size_t tex_mem_required;
  Gu3dfInfo info;
  int ppm_width, ppm_height;
  int target_width, target_height;
  GrMipMapId_t mmid;
  GrVertex vtx[4];
  float smult, tmult;

  if( argc < 2 )
    {
      fprintf( stderr, "Usage: %s in.ppm [display_width display_height] \n", argv[0] );
      exit( -1 );
    }

  InitGlide();

  if( !PPMGetDimensions( argv[1], &ppm_width, &ppm_height ) )
    {
      fprintf( stderr, "Error getting PPM dimensions\n" );
      exit( -1 );
    }

  /*
   * If there are args to resize the image, then remember them.
   */
  if( argc == 4 )
    {
      target_width = atoi( argv[2] );
      target_height = atoi( argv[3] );
    }
  else
    {
      target_width = ppm_width;
      target_height = ppm_height;
    }

#if 0
  /*
   * Validate the geometry of the output image.
   */
  if( !ValidateImageDimensions( target_width, target_height ) )
    {
      fprintf( stderr, "Invalid image dimensions\n" );
      exit( -1 );
    }
#endif

  tex_mem_required = txInit3dfInfo( &info, GR_TEXFMT_P_8,
                                    &target_width, &target_height,
                                    -1, TX_AUTORESIZE_GROW );

  /*
   * Allocate system memory for the texture.
   */
  info.data = malloc( tex_mem_required );
  if( !info.data )
    {
      fprintf( stderr, "Out of memory allocating system memory textures.\n" );
      exit( -1 );
    }

  /*
   * Make sure txInit3dfInfo didn't fail.
   */
  if( tex_mem_required == 0 )
    {
      fprintf( stderr, "Problem with txInit3dfInfo\n" );
      exit( -1 );
    }

  /*
   * Read the PPM image.
   */
  if( !( raw_image = PPMLoad( argv[1] ) ) )
    {
      fprintf( stderr, "Not able to load PPM file.\n" );
      exit( -1 );
    }

  /*
   * Convert to a texture that can be downloaded to the hardware.
   */
  txConvert( &info, GR_TEXFMT_ARGB_8888,
             ppm_width, ppm_height,
             raw_image, TX_DITHER_NONE, NULL );
  
  {
    FILE *fp;

    fprintf( stderr, "Writing blah.3df\n" );
    if( !( fp = fopen( "blah.3df", "wb" ) ) )
      {
        fprintf( stderr, "Not able to open output file.\n" );
        exit( -1 );
      }

    txWrite( &info, fp, TX_WRITE_3DF );
    fclose( fp );

    grTexDownloadTable( GR_TMU0,
                        GR_TEXTABLE_PALETTE,
                        &info.table.palette.data );
  }

  /*
   * Call Glide to allocate memory for the converted texture.
   */
  mmid = guTexAllocateMemory( 0, GR_MIPMAPLEVELMASK_BOTH,
                              info.header.width, info.header.height,
                              info.header.format,
                              GR_MIPMAP_NEAREST,
                              info.header.small_lod, info.header.large_lod,
                              info.header.aspect_ratio,
                              GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                              GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR,
                              0.0F,
                              FXFALSE );

  /*
   * Download the texture and set it as the current texture.
   */
  guTexDownloadMipMap( mmid, info.data, &info.table.nccTable );
  guTexSource( mmid );

  /*
   * Figure out what the factor is to get from normalized texture
   * coords to something that the hardware can deal with.
   */
  smult = aspectToTexCoordFactors[info.header.aspect_ratio].sMult;
  tmult = aspectToTexCoordFactors[info.header.aspect_ratio].tMult;

  /*
   * Set up some verts to draw the texture on a couple of triangles
   * with.
   */
  vtx[0].x = 0.0f;
  vtx[0].y = 0.0f;
  vtx[0].oow = 1.0f;
  vtx[0].tmuvtx[0].sow = 0.0f;
  vtx[0].tmuvtx[0].tow = 0.0f;

  vtx[1].x = ( float )target_width - 1.0f;
  vtx[1].y = 0.0f;
  vtx[1].oow = 1.0f;
  vtx[1].tmuvtx[0].sow = 1.0f * smult;
  vtx[1].tmuvtx[0].tow = 0.0f;

  vtx[2].x = ( float )target_width - 1.0f;
  vtx[2].y = ( float )target_height - 1.0f;
  vtx[2].oow = 1.0f;
  vtx[2].tmuvtx[0].sow = 1.0f * smult;
  vtx[2].tmuvtx[0].tow = 1.0f * tmult;

  vtx[3].x = 0.0f;
  vtx[3].y = ( float )target_height - 1.0f;
  vtx[3].oow = 1.0f;
  vtx[3].tmuvtx[0].sow = 0.0f;
  vtx[3].tmuvtx[0].tow = 1.0f * tmult;

  printf( "Hit a key to exit\n" );
  fflush( stdout );

  while( !kbhit() )
    {
      grDrawTriangle( &vtx[0], &vtx[1], &vtx[2] );
      grDrawTriangle( &vtx[0], &vtx[2], &vtx[3] );
      grBufferSwap( 1 );
    }

  /*
   * Shutdown Glide (and switch the passthru back to the 
   * main display.
   */
  grGlideShutdown();

  /*
   * Clean up.
   */
  free( raw_image );
  free( info.data );

  return 0;
}
