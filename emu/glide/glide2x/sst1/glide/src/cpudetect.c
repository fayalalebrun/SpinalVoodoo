/* 
 * THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
 * PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
 * TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
 * INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
 * DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
 * THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
 * FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
 * 
 * USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
 * RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
 * TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
 * AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
 * SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
 * THE UNITED STATES.  
 * 
 * COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
 */

#include <sys/utsname.h>
#include <stdio.h>
#include <string.h>

int _cpu_detect_asm() {
  struct utsname name;

  uname(&name);
  if (!strcmp(name.machine, "i386")) return 3;
  if (!strcmp(name.machine, "i486")) return 4;
  if (!strcmp(name.machine, "i586")) return 5;
  if (!strcmp(name.machine, "i686")) return 6;
  fprintf(stderr, "Couldn't determine cpu type. Using i586\n");
  return 5;
}

void single_precision_asm() {
  asm("push %eax \n fnclex \n fstcw (%esp) \n movl (%esp), %eax \n "
      "and $0x0000fcff, %eax \n movl %eax, (%esp) \n fldcw (%esp) \n pop %eax");
}


void double_precision_asm() {
  asm("push %eax \n fnclex \n fstcw (%esp) \n movw (%esp), %eax \n "
      "and $0x0000fcff, %eax \n or $0x000002ff, %eax \n mov %eax, (%esp) \n "
      "fldcw (%esp) \n pop %eax");
}

