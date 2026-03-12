/* 
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONL
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGH
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DF
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com).
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR 
** FULL TEXT OF THE NON-WARRANTY PROVISIONS. 
**
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT T
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS I
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FA
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS O
** THE UNITED STATES. 
**
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
**
*/

#include "fxbldno.h"

#define MANVERSION              3
#define MANREVISION             10
#define MINVERSION              00
//#define BUILD_NUMBER            3001 /* hmm, must be at least 674 for the 3dfx OpenGL ICD to work correctly */

#define VERSIONSTR "3.10.00." BUILD_NUMBER_STR
#define CONTACTSTR "The Glide Open Source Project http://glide.sourceforge.net/"

#if defined(__WIN32__)
#define _TRADEMARK_  "(tm)"
#define _REGISTERED_ "\256"
#define _COPYRIGHT_  "\251"
#elif defined(macintosh)
#define _TRADEMARK_  "\252"
#define _REGISTERED_ "\250"
#define _COPYRIGHT_  "\251"
#else
#define _TRADEMARK_ "(tm)"
#define _REGISTERED_ "(R)"
#define _COPYRIGHT_  "(C)"
#endif

#ifdef SST1
#   define HWSTR   "Voodoo Graphics" _REGISTERED_
#   ifdef NT_BUILD
#     define PRODNAME "Glide" _REGISTERED_ " for Voodoo Graphics" _COPYRIGHT_ " & Windows" _REGISTERED_ " NT"
#   else
#     define PRODNAME "Glide" _REGISTERED_ " for Voodoo Graphics" _COPYRIGHT_ " & Windows" _REGISTERED_ " 95/98"
#   endif /* NT_BUILD */
#elif defined(SST96)
#   define HWSTR   " Voodoo Rush" _REGISTERED_ ""
#   ifdef NT_BUILD
#     define PRODNAME "Glide" _REGISTERED_ " for Voodoo Rush" _COPYRIGHT_ " & Windows" _REGISTERED_ " NT"
#   else
#     define PRODNAME "Glide" _REGISTERED_ " for Voodoo Rush" _COPYRIGHT_ " & Windows" _REGISTERED_ " 95/98"
#   endif /* NT_BUILD */
#elif defined(CVG) || defined(VOODOO2)
#   define HWSTR   " Voodoo^2" _REGISTERED_ ""
#   ifdef NT_BUILD
#     define PRODNAME "Glide" _REGISTERED_ " for Voodoo^2" _COPYRIGHT_ " & Windows" _REGISTERED_ " NT"
#   else
#     define PRODNAME "Glide" _REGISTERED_ " for Voodoo^2" _COPYRIGHT_ " & Windows" _REGISTERED_ " 95/98"
#   endif /* NT_BUILD */
#elif defined(H3) || defined(H5)
#   define HWSTR   " Voodoo Banshee" _REGISTERED_ ", Velocity" _TRADEMARK_ " 100/200, Voodoo3" _REGISTERED_ ", Voodoo4" _REGISTERED_ ", & Voodoo5" _REGISTERED_
#     define PRODNAME "Glide" _REGISTERED_ " for Voodoo Banshee" _REGISTERED_ ", Velocity" _TRADEMARK_ "100/200, Voodoo3" _REGISTERED_ " , Voodoo4" _REGISTERED_ ", & Voodoo5" _REGISTERED_ "; Windows" _REGISTERED_ " 95/98/Me/NT4/2000/XP"
#else
#   define HWSTR   "Some Hoopti Chip" _REGISTERED_ ""
#endif
