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

#include <3dfx.h>
#include <tsc.h>

static FxU32 tsc_histogram[TSC_MAX_CLOCKS];

FxU32        tsc_begin, tsc_end;
static FxU32 tsc_bias;

void 
tscInit( void ) {
   FxU32 test = 100;
   int   i;
   

   for ( i = 0; i < 10; i++ )
   {
     tscBegin();
     tsc_end = tscReadTSC();
     if ( ( tsc_end - tsc_begin ) < test )
        test = tsc_end - tsc_begin;
   }
   tsc_bias = test;
}

/*
** timing routines
*/
void tscStoreDelta( void )
{
   FxU32 value = tsc_end - tsc_begin;

   if ( value < TSC_MAX_CLOCKS && value > tsc_bias )
      tsc_histogram[value-tsc_bias]++;
}

void tscPrintHistogram( void )
{
   float total = 0.0F;
   int   i;

   for ( i = 0; i < TSC_MAX_CLOCKS; i++ )
      total += tsc_histogram[i];

   for ( i = 0; i < TSC_MAX_CLOCKS; i++ )
   {
      float percent = ( tsc_histogram[i] * 100.0F ) / total;

      if ( percent > 1.0 )
      {
         printf( "%04d: %3.2f\n", i, percent );
      }
   }
}

FxU32 
tscGetMostFrequent(void) {
  float total = 0.0F;
  int   i;
  float
    maxPercent = 0.f;
  FxU32
    mostFrequentClockCount;
  
  for ( i = 0; i < TSC_MAX_CLOCKS; i++ )
    total += tsc_histogram[i];
  
  for ( i = 0; i < TSC_MAX_CLOCKS; i++ ) {
    float percent = ( tsc_histogram[i] * 100.0F ) / total;
    
    if ( percent > maxPercent ) {
      maxPercent = percent;
      mostFrequentClockCount = i;
    }
  }
  return mostFrequentClockCount;
}

FxU32 *tscGetHistogram( void )
{
   return tsc_histogram;
}

void tscResetHistogram(void)
{
  int i;
  
  for (i = 0; i < TSC_MAX_CLOCKS; i++)
    tsc_histogram[i] = 0;
}
 
