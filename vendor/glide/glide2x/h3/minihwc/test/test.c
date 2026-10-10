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
#include <stdlib.h>
#include <minihwc.h>

void
main(int argc, char **argv)
{
  FxU32
    devId, venId,
    i;
  hwcInfo *hInfo;

  if (argc < 3) {
    printf("usage:  test vid did\n");
    exit(1);
  }

  sscanf(argv[1], "%x", &venId);
  sscanf(argv[2], "%x", &devId);

  if ((hInfo = hwcInit(venId, devId)) == NULL) {
    printf(hwcGetErrorString());
    exit(2);
  }

  printf("nBoards = %d\n", hInfo->nBoards);

  for (i = 0; i < hInfo->nBoards; i++) {
    int pbAddr;
    hwcPCIInfo *pciInfo = &hInfo->boardInfo[i].pciInfo;

    printf("Board %d\n", i);
    printf("\tVendor ID = 0x%x\n", pciInfo->vendorID);
    printf("\tDevice ID = 0x%x\n", pciInfo->deviceID);

    hwcMapBoard(&hInfo->boardInfo[i], 0x1ff);
    for (pbAddr = 0; pbAddr < 9; pbAddr++) {
      printf("\tPCI Base Address %d = 0x%x\n", pbAddr, pciInfo->pciBaseAddr[pbAddr]);
      printf("\tLinear Address: 0x%x\n",
             hInfo->boardInfo[i].linearInfo.linearAddress[pbAddr]); 
    }
  }
} /* main */

