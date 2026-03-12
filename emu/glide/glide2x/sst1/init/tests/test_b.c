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
#include <conio.h>

#include <init.h>

static InitDeviceInfo devInfo;
static FxBool         found = FXFALSE;

FxBool detectCallback( const InitDeviceInfo *info ) {
    if ( info->hwClass == INIT_VG96 ) {
        devInfo = *info;
        found = FXTRUE;
        return FXTRUE;
    }
    return FXFALSE;
}

void cb( FxU32 addr, FxU32 val ) {
    return;
}

/*-----------------------------------------------
  Detects the presence of 3DFX hardware and 
  prints out all info pertaining ot located boards.
  -----------------------------------------------*/
int main( void ) {
    FxU32 *hw;
    InitBufferDesc desc;

    initEnumHardware( detectCallback );
    if ( found ) {
        initDeviceSelect( devInfo.devNumber );
        hw = initMapDevice();
        if ( !hw ) {
            puts( "initMapDevice failed." );
            exit( -1 );
        }
        initResetRegisters();

        if ( !initAllocBuffers( &desc ) ) {
            puts( "intiAllocBuffers failed." );
        }
        puts( "Press a key to exit." );
        getch();
    }
    return 0;
}
