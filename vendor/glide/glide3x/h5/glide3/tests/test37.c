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

#define CORRECT_GR_BITS_DEPTH 16
#define CORRECT_GR_BITS_RGBA_R 5
#define CORRECT_GR_BITS_RGBA_G 6
#define CORRECT_GR_BITS_RGBA_B 5
#define CORRECT_GR_BITS_RGBA_A 0
#define CORRECT_GR_BITS_GAMMA 8 
#define CORRECT_GR_FOG_TABLE_ENTRIES 64
#define CORRECT_GR_GAMMA_TABLE_ENTRIES 32
#define CORRECT_GR_MAX_TEXTURE_SIZE 256
#define CORRECT_GR_MAX_TEXTURE_ASPECT_RATIO 3
#define CORRECT_GR_NUM_FB 2
#define CORRECT_GR_NUM_TMU 1
#define CORRECT_GR_TEXTURE_ALIGN 8
#define CORRECT_GR_VIEWPORT_X 0
#define CORRECT_GR_VIEWPORT_Y 0
#define CORRECT_GR_VIEWPORT_WIDTH 640
#define CORRECT_GR_VIEWPORT_HEIGHT 480

void TestGet(char * getvalstr, int getval, int memcount , FxI32 * memval, int * failures);
void TestGetString(char * getvalstr, int getval, int * failures);

int hwconfig;

int 
main( int argc, char **argv) 
{
  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  FxI32 val4[4];
  FxI32 * histbuffer;
  FxI32 histsize;
  const char * str;
  int ret;
  int i;
  int failures = 0;
  int warnings = 0;
  GrProc func;
  GrContext_t          gc = 0;
  
  /* Initialize Glide */
  grGlideInit();
  assert( hwconfig = tlVoodooType() );
  
  /* Process Command Line Arguments */
  tlSetScreen( scrWidth, scrHeight );
  
  str = grGetString( GR_VERSION );
  
  printf("Test 37 - grGet() Stress Test. All output goes to the Console.\n\n");
  tlGetCH ();
  
  /* Test the Num Boards Call before we do the win open*/
  TestGet("GR_NUM_BOARDS", GR_NUM_BOARDS, 1 , &val4[0], &failures);
  
  grSstSelect( 0 );
  gc = grSstWinOpen( tlGethWnd(),
                     resolution,
                     GR_REFRESH_60Hz,
                     GR_COLORFORMAT_ABGR,
                     GR_ORIGIN_UPPER_LEFT,
                     2, 1 );
  if (!gc) {
    printf("Could not allocate glide fullscreen context.\n");
    goto __errExit;
  }
  
  tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 
           60, 30, 0xffffff );
  
  /*do the 4 byte grGet()'s*/
  TestGet("GR_BITS_DEPTH", GR_BITS_DEPTH, 1 , &val4[0], &failures);
  if(val4[0]!=CORRECT_GR_BITS_DEPTH) {
    printf("WARNING : Value returned was not not equal to %d\n",CORRECT_GR_BITS_DEPTH);
    warnings++;
  }
  
  TestGet("GR_BITS_GAMMA", GR_BITS_GAMMA, 1 , &val4[0], &failures);
  if(val4[0]<CORRECT_GR_BITS_GAMMA) {
    printf("WARNING : Value returned was less than expected\n");
    warnings++;
  }
  
  TestGet("GR_FOG_TABLE_ENTRIES", GR_FOG_TABLE_ENTRIES, 1 , &val4[0], &failures);
  if(val4[0]<CORRECT_GR_FOG_TABLE_ENTRIES) {
    printf("WARNING : Value returned was less than expected\n");
    warnings++;
  }
  
  TestGet("GR_GAMMA_TABLE_ENTRIES", GR_GAMMA_TABLE_ENTRIES, 1 , &val4[0], &failures);
  if(val4[0]<CORRECT_GR_GAMMA_TABLE_ENTRIES) {
    printf("WARNING : Value returned was less than expected\n");
    warnings++;
  }
  
  TestGet("GR_IS_BUSY", GR_IS_BUSY, 1 , &val4[0], &failures);
  if(val4[0]!=FXTRUE && val4[0]!=FXFALSE) {
    printf("WARNING : Value returned was not FXTRUE or FXFALSE\n");
    warnings++;
  }
  
  TestGet("GR_LFB_PIXEL_PIPE", GR_LFB_PIXEL_PIPE, 1 , &val4[0], &failures);
  if(val4[0]!=FXTRUE && val4[0]!=FXFALSE) {
    printf("WARNING : Value returned was not FXTRUE or FXFALSE\n");
    warnings++;
  }
  
  TestGet("GR_MAX_TEXTURE_SIZE", GR_MAX_TEXTURE_SIZE, 1 , &val4[0], &failures);
  if(val4[0]<CORRECT_GR_MAX_TEXTURE_SIZE) {
    printf("WARNING : Value returned was less than expected\n");
    warnings++;
  }
  
  TestGet("GR_MAX_TEXTURE_ASPECT_RATIO", GR_MAX_TEXTURE_ASPECT_RATIO, 1 , &val4[0], &failures);
  if(val4[0]<CORRECT_GR_MAX_TEXTURE_ASPECT_RATIO) {
    printf("WARNING : Value returned was less than expected\n");
    warnings++;
  }
  
  TestGet("GR_NON_POWER_OF_TWO_TEXTURES", GR_NON_POWER_OF_TWO_TEXTURES, 1 , &val4[0], &failures);
  if(val4[0]!=FXTRUE && val4[0]!=FXFALSE) {
    printf("WARNING : Value returned was not FXTRUE or FXFALSE\n");
    warnings++;
  }
  
  TestGet("GR_NUM_FB", GR_NUM_FB, 1 , &val4[0], &failures);
  if(val4[0]>CORRECT_GR_NUM_FB) {
    printf("WARNING : Value returned was greater than %d\n",CORRECT_GR_NUM_FB);
    warnings++;
  }
  
  TestGet("GR_NUM_TMU", GR_NUM_TMU, 1 , &val4[0], &failures);
  if(val4[0]<CORRECT_GR_NUM_TMU) {
    printf("WARNING : Value returned was less than %d\n",CORRECT_GR_NUM_TMU);
    warnings++;
  }
  
  TestGet("GR_SUPPORTS_PASSTHRU", GR_SUPPORTS_PASSTHRU, 1 , &val4[0], &failures);
  if(val4[0]!=FXTRUE && val4[0]!=FXFALSE) {
    printf("WARNING : Value returned was not FXTRUE or FXFALSE\n");
    warnings++;
  }
  
  TestGet("GR_TEXTURE_ALIGN", GR_TEXTURE_ALIGN, 1 , &val4[0], &failures);
  if(val4[0]<CORRECT_GR_TEXTURE_ALIGN) {
    printf("WARNING : Value returned was less than expected\n");
    warnings++;
  }
  
  /*Now do the ones we cant verify*/
  TestGet("GR_GLIDE_STATE_SIZE", GR_GLIDE_STATE_SIZE, 1 , &val4[0], &failures);
  TestGet("GR_GLIDE_VERTEXLAYOUT_SIZE", GR_GLIDE_VERTEXLAYOUT_SIZE, 1 , &val4[0], &failures);
  TestGet("GR_MEMORY_FB", GR_MEMORY_FB, 1 , &val4[0], &failures);
  TestGet("GR_MEMORY_TMU", GR_MEMORY_TMU, 1 , &val4[0], &failures);
  TestGet("GR_MEMORY_UMA", GR_MEMORY_UMA, 1 , &val4[0], &failures);
  TestGet("GR_NUM_SWAP_HISTORY_BUFFER", GR_NUM_SWAP_HISTORY_BUFFER, 1 , &val4[0], &failures);
  TestGet("GR_PENDING_BUFFERSWAPS", GR_PENDING_BUFFERSWAPS, 1 , &val4[0], &failures);
  TestGet("GR_REVISION_FB", GR_REVISION_FB, 1 , &val4[0], &failures);
  TestGet("GR_REVISION_TMU", GR_REVISION_TMU, 1 , &val4[0], &failures);
  TestGet("GR_STATS_LINES", GR_STATS_LINES, 1 , &val4[0], &failures);
  TestGet("GR_STATS_PIXELS_AFUNC_FAIL", GR_STATS_PIXELS_AFUNC_FAIL, 1 , &val4[0], &failures);
  TestGet("GR_STATS_PIXELS_CHROMA_FAIL", GR_STATS_PIXELS_CHROMA_FAIL, 1 , &val4[0], &failures);
  TestGet("GR_STATS_PIXELS_DEPTHFUNC_FAIL", GR_STATS_PIXELS_DEPTHFUNC_FAIL, 1 , &val4[0], &failures);
  TestGet("GR_STATS_PIXELS_IN", GR_STATS_PIXELS_IN, 1 , &val4[0], &failures);
  TestGet("GR_STATS_PIXELS_OUT", GR_STATS_PIXELS_OUT, 1 , &val4[0], &failures);
  TestGet("GR_STATS_POINTS", GR_STATS_POINTS, 1 , &val4[0], &failures);
  TestGet("GR_STATS_TRIANGLES_IN", GR_STATS_TRIANGLES_IN, 1 , &val4[0], &failures);
  TestGet("GR_STATS_TRIANGLES_OUT", GR_STATS_TRIANGLES_OUT, 1 , &val4[0], &failures);
  
  /*Now do 8 byte grGet()'s*/
  TestGet("GR_WDEPTH_MIN_MAX", GR_WDEPTH_MIN_MAX, 2 , &val4[0], &failures);
  if(val4[0]>val4[1]) {
    printf("WARNING : Value[0] returned was greater than Value[1]\n");
    warnings++;
  }
  
  TestGet("GR_ZDEPTH_MIN_MAX", GR_ZDEPTH_MIN_MAX, 2 , &val4[0], &failures);
  if(val4[0]<val4[1]) {
    printf("WARNING : Value[0] returned was less than Value[1]\n");
    warnings++;
  }
  /*Now do the ones we cant verify*/
  TestGet("GR_FIFO_FULLNESS", GR_FIFO_FULLNESS, 2 , &val4[0], &failures);
  TestGet("GR_VIDEO_POSITION", GR_VIDEO_POSITION, 2 , &val4[0], &failures);
  
  
  
  /* Now do 16 byte grGet()*/
  TestGet("GR_BITS_RGBA", GR_BITS_RGBA, 4 , &val4[0], &failures);
  if((val4[0]!=CORRECT_GR_BITS_RGBA_R) | (val4[1]!=CORRECT_GR_BITS_RGBA_G) | 
     (val4[2]!=CORRECT_GR_BITS_RGBA_B) | (val4[3]!=CORRECT_GR_BITS_RGBA_A) ) {
    printf("WARNING : Values returned were not in range expected\n");
    warnings++;
  }
  
  TestGet("GR_VIEWPORT", GR_VIEWPORT, 4 , &val4[0], &failures);
  if((val4[0]!=CORRECT_GR_VIEWPORT_X) | (val4[1]!=CORRECT_GR_VIEWPORT_Y) |
     (val4[2]!=CORRECT_GR_VIEWPORT_WIDTH) |
     (val4[3]!=CORRECT_GR_VIEWPORT_HEIGHT)){
    printf("WARNING : Values returned were not in range expected\n");
    warnings++;
  }
  
  
  
  /* Now do the Swap History Test*/
  ret = grGet(GR_NUM_SWAP_HISTORY_BUFFER, sizeof(histsize), &histsize );
  
  if(ret==sizeof(histsize)) {
    histbuffer = malloc(histsize*sizeof(FxI32));
    
    ret = grGet(GR_SWAP_HISTORY, histsize*sizeof(FxI32), histbuffer );
    if(ret==(int)(histsize*sizeof(FxI32)))
      printf("Success(%d) - ",ret);
    else {
      printf("Failed (%d) - ", ret);
      failures++;
    }
    
    printf("GR_SWAP_HISTORY Result = ");
    
    for(i=0;i<histsize;i++) {
      printf("%d ", histbuffer[i]);
    }
    printf("\n");
    free(histbuffer);
  } else {
    printf("GR_NUM_SWAP_HISTORY_BUFFER failed, so unable to call GR_SWAP_HISTORY\n");   
  }
  
  /* Now Test The GetString Calls*/
  TestGetString("GR_HARDWARE", GR_HARDWARE, &failures);
  TestGetString("GR_RENDERER", GR_RENDERER, &failures);
  TestGetString("GR_VENDOR", GR_VENDOR, &failures);
  TestGetString("GR_VERSION", GR_VERSION, &failures);
  TestGetString("GR_EXTENSION", GR_EXTENSION, &failures);
  
  str = grGetString(GR_EXTENSION);
  
  if(strstr(str,"CHROMARANGE")) {
    /*try and do the getprocaddress calls for Chromarange*/
    func = grGetProcAddress ( "grChromaRangeModeExt");
    if(!func) {
      printf("GetProcAddress for grChromaRangeModeExt Failed\n");
      failures++;
    }     
    func = grGetProcAddress ( "grChromaRangeExt");
    if(!func) {
      printf("GetProcAddress for grChromaRangeExt Failed\n");
      failures++;
    }     
  }
  
  if(strstr(str,"TEXCHROMA")) {
    /* try and do the getprocaddress calls for TexChroma*/
    func = grGetProcAddress ( "grTexChromaModeExt");
    if(!func) {
      printf("GetProcAddress for grTexChromaModeExt Failed\n");
      failures++;
    }     
    func = grGetProcAddress ( "grTexChromaRangeExt");
    if(!func) {
      printf("GetProcAddress for grTexChromaRangeExt Failed\n");
      failures++;
    }     
  }
  
  if(failures)
    printf("\n%d failures encountered during this test\n",failures);
  else
    printf("\ntesting complete - no failures reported\n");
  
  if(warnings)
    printf("%d warnings encountered during this test\n",warnings);
  else
    printf("testing complete - no warnings reported\n");
  
 __errExit:    
  grGlideShutdown();
  
  return 0;
} /* main */


void
TestGet(char * getvalstr, int getval, int memcount , FxI32 * memval, int * failures)
{
  int ret;
  int i;
  int correct;
  
  correct = FXTRUE;                                                                                     /*Set to TRUE to start with*/
  
  for(i=0;i<4;i++)              /*Clear Memory to some unique Pattern*/
    memval[i] =0xdeadbeef;
  
  ret = grGet(getval, memcount*sizeof(FxI32), memval );  /*Do the Get*/
  
  for(i=0;i<memcount;i++)
    /*if pattern still exists, then we have a problem*/
    if(memval[i]==0xdeadbeef)
      correct=FXFALSE;
  
  /* if Number of bytes returned is wrong, then we have a problem*/
  if(ret!=(int)(memcount*sizeof(FxI32))) 
    correct=FXFALSE;
  
  if(correct)                   /*print the results*/
    printf("Success(%d) - ",ret);
  else {
    printf("Failed (%d) - ", ret);
    failures[0]++;
  }
  
  switch(memcount) {
  case 4:
    printf("%s Result = %d, %d, %d, %d\n",getvalstr, memval[0],memval[1],memval[2],memval[3]);
    break;    
    
  case 2:
    printf("%s Result = %d, %d\n",getvalstr, memval[0],memval[1]);
    break;    
    
  case 1:
  default:
    printf("%s Result = %d\n",getvalstr, memval[0]);
    break;    
  }
} /* TestGet */

void 
TestGetString(char * getvalstr, int getval, int * failures)
{
  const char * str;
  
  str = grGetString(getval);
  if(str)
      printf("Success - ");
  else {
    printf("Failed - ");
    failures[0]++;
  }
  printf("%s Result = %s\n",getvalstr, str);
} /* TestGetString */
