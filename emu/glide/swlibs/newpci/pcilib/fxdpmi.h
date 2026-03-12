/*
 * FxDPMI for DOS.
 *
 *  Copyright (c) 2002 - Borca Daniel
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef FXDPMI_H_included
#define FXDPMI_H_included

#ifdef __DJGPP__
#include <pc.h>

#define inp inportb
#define inpw inportw
#define inpd inportl

#define outp outportb
#define outpw outportw
#define outpd outportl

#define _inp inportb
#define _inpw inportw
#define _inpd inportl

#define _outp outportb
#define _outpw outportw
#define _outpd outportl
#endif

#endif
