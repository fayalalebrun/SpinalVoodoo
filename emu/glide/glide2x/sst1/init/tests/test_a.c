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
#include <init.h>

#include <conio.h>
#include <stdarg.h>

#define NUM_LINES 25

void morePrintf( FxU32 *lines, const char *fmt, ... ) {
    va_list args;

    if ( *lines > NUM_LINES ) {
        printf( "<more>" );
        getch();
        *lines = 0;
    }

    va_start( args, fmt );
    vfprintf( stdout, fmt, args );
    va_end( args );
    (*lines)++;
    return;
}

static FxU32 lineCount;

static char *vgaChipNames[] = {
    "Alliance Semiconductor AT3D"
};

void detectCallback( const InitDeviceInfo *info ) {
    switch( info->hwClass ) {
    case INIT_VOODOO:
        morePrintf( &lineCount, 
                    "Device: %d Type: Voodoo\n", 
                    info->devNumber );
        morePrintf( &lineCount,
                    "- pfxRev: %.02d pfxRam: %.02d\n",
                    info->vgInfo.pfxRev, info->vgInfo.pfxRam );
        morePrintf( &lineCount,
                    "- nTFX:   %.02d\n",
                    info->vgInfo.nTFX );
        morePrintf( &lineCount,
                    "- tfxRev: %.02d tfxRam: %.02d\n",
                    info->vgInfo.tfxRev,
                    info->vgInfo.tfxRam );
        morePrintf( &lineCount,
                    "- sli:    %s\n",
                    info->vgInfo.sliDetect?"TRUE":"FALSE" );
        break;
    case INIT_VG96:
        morePrintf( &lineCount, 
                    "Device: %d Type: VG96\n", 
                    info->devNumber );     
        morePrintf( &lineCount,
                    "- vgaChip: %s\n",
                    vgaChipNames[info->vg96Info.vgaChip] );
        morePrintf( &lineCount,
                    "- vg96Rev: %.02d vg96Ram: %.02d\n",
                    info->vg96Info.vg96Rev, info->vg96Info.vg96Ram );
        morePrintf( &lineCount,
                    "- nTFX:   %.02d\n",
                    info->vg96Info.nTFX );
        morePrintf( &lineCount,
                    "- tfxRev: %.02d tfxRam: %.02d\n",
                    info->vg96Info.tfxRev,
                    info->vg96Info.tfxRam );
        break;
    }
    return;
}

/*-----------------------------------------------
  Detects the presence of 3DFX hardware and 
  prints out all info pertaining ot located boards.
  -----------------------------------------------*/
void main( void ) {
    while( initEnumHardware( detectCallback ) );
    return;
}
