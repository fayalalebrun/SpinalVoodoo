#ifndef __VECTOR_H__
#define __VECTOR_H__

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

#include <stdio.h>

// Defines for different types of vector files 
#define VECTORID_MAX           2
#define VECTORID_CACHE         0
#define VECTORID_DECOMPRESSOR  1
#define VECTORID_CACHEm        1 << VECTORID_CACHE
#define VECTORID_DECOMPRESSORm 1 << VECTORID_DECOMPRESSOR

extern void openVectorFiles(); 
extern void closeVectorFiles( );
extern void dumpVector( );

//extern void openVectorFiles( 
//		CsimPrivate) *, 
//		unsigned int vectorGenMask );
//extern void closeVectorFiles( unsigned int vectorGenMask );
//extern void dumpVector( unsigned char * vectorData, unsigned int vectorType );

typedef struct vectorFiles { 
  FILE * vectorHandles[VECTORID_MAX];
  void (* dumpVector)();
} vectorFiles;

// extern vectorFiles vectorf;

typedef struct {
  FxU32 data[2];
} CBlock;

typedef struct CacheOutput { 
  int u;
  int v;
  CBlock bank[ 4 ];
} CacheOutput;

typedef struct DecompOutput { 
  FxU32 Texel[4];
} DecompOutput;


#endif
