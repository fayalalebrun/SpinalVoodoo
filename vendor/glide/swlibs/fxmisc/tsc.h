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

#ifndef __TSC_H__
#define __TSC_H__

#ifndef __3DFX_H__
#  include <3dfx.h>
#endif

/*
** timing routines
*/
#define TSC_MAX_CLOCKS 5000

extern FxU32 tsc_begin, tsc_end;

FxU32 *tscGetHistogram( void );
FxU32  tscGetMostFrequent(void);
void   tscInit( void );
FxU32  tscReadTSC( void );
void   tscPrintHistogram( void );
void   tscStoreDelta();
void   tscResetHistogram(void);

#define tscBegin() \
          tsc_begin = tscReadTSC();
#define tscEnd() \
          tsc_end = tscReadTSC(); \
          tscStoreDelta();

#if defined(__WATCOMC__)
#pragma aux tscReadTSC = \
   ".586" \
   "rdtsc" \
   modify [edx eax] \
   value [eax];

#elif defined(__MSC__)
FxU32 tscReadTSC(void);
#else
#error "TSC:  unsupported compiler"
#endif

#endif
