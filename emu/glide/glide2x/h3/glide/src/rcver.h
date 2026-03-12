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

#define MANVERSION              2
#define MANREVISION             60

#ifdef __WIN32__
#define BUILD_NUMBER            0415
#endif

#ifndef GLIDE3
#define VERSIONSTR "2.60.00.0415"
#else
#define VERSIONSTR "3.0\0"
#endif

#ifdef SST1
#   define HWSTR   "Voodoo Graphics(tm)\0"
#   ifdef NT_BUILD
#     define PRODNAME "Glide(tm) for Voodoo Graphics\251 and Windows\256 NT\0"
#   else
#     define PRODNAME "Glide(tm) for Voodoo Graphics\251 and Windows\256 95/98\0"
#   endif /* NT_BUILD */
#elif defined(SST96)
#   define HWSTR   " Voodoo Rush(tm)\0"
#   ifdef NT_BUILD
#     define PRODNAME "Glide(tm) for Voodoo Rush\251 and Windows\256 NT\0"
#   else
#     define PRODNAME "Glide(tm) for Voodoo Rush\251 and Windows\256 95/98\0"
#   endif /* NT_BUILD */
#elif defined(CVG) || defined(VOODOO2)
#   define HWSTR   " Voodoo^2(tm)\0"
#   ifdef NT_BUILD
#     define PRODNAME "Glide(tm) for Voodoo^2\251 and Windows\256 NT\0"
#   else
#     define PRODNAME "Glide(tm) for Voodoo^2\251 and Windows\256 95/98\0"
#   endif /* NT_BUILD */
#elif defined(H3)
#   define HWSTR   " Voodoo Banshee(tm) & Voodoo3(tm)\0"
#     define PRODNAME "Glide for Voodoo Banshee\256 and Voodoo3\256; Windows 95 \256, Windows 98 \256, and Windows NT\256\0"
#else
#   define HWSTR   "Unknown Chip\0"
#endif
