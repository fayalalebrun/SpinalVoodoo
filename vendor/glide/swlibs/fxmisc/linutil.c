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

#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "linutil.h"

static int init_done=0;
static int init_failed=0;
static struct termios save_termdata;

static void reset_term() {
  if (tcsetattr(0, 0, &save_termdata)<0)
    fprintf(stderr, "Failed to reset termio data for stdin\n");
}

static void do_init() {
  struct termios termdata;

  if (tcgetattr(0, &termdata)<0) {
    fprintf(stderr, "Failed to get termio data for stdin\n");
    init_done=1;
    init_failed=1;
    return;
  }
  save_termdata=termdata;
  cfmakeraw(&termdata);
  if (tcsetattr(0, 0, &termdata)<0) {
    fprintf(stderr, "Failed to set termio data for stdin\n");
    init_done=1;
    init_failed=1;
    return;
  }
  atexit(reset_term);
  init_done=1;
}

int lin_kbhit() {
  fd_set set;
  struct timeval to;

  if (!init_done) do_init();
  if (init_failed) return 0;
  FD_ZERO(&set);
  FD_SET(0, &set);
  to.tv_sec=0;
  to.tv_usec=0;
  if (select(1, &set, 0, 0, &to)<1) return 0;
  return 1;
}

char lin_getch() {
  char c;

  if (!init_done) do_init();
  if (init_failed) return EOF;
  if (read(0, &c, 1) != 1)
    return EOF;
  return c;
}
