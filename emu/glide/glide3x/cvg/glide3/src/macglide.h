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

#ifndef _MAC_GLIDE_H_
#define _MAC_GLIDE_H_

/* Glide Global options */
#define CVG                     1
#define GLIDE_LIB					      1
#define GLIDE_PLUG				      1

/* Glide Platform Options */
#define SET_BSWAP		            1
#define HAL_HW						      1 
#define INIT_DOS					      1
#define HAS_CONSOLE_IO		      1

/* Glide Debugging options */
#define DEBUG						        0

#if DEBUG
#define GLIDE_DEBUG 			      1
#define GDBG_INFO_ON				    1
#define GLIDE_USE_DEBUG_FIFO    1

#define FIFO_ASSERT_FULL		    1
#define GLIDE_SANITY_SIZE		    1	
#define GLIDE_SANITY_ASSERT	    1
#ifdef __MWERKS__
#pragma global_optimizer		    off
#pragma scheduling					    off
#pragma traceback					      on
#endif
#else /* !DEBUG */
#ifdef __MWERKS__
#pragma global_optimizer		    on
#pragma optimization_level	    4
#pragma peephole					      on
#pragma scheduling					    603
#pragma traceback					      on
#pragma side_effects				    off
#endif
#endif /* !DEBUG */

#define GLIDE_USE_C_TRISETUP    1

#define WTF_P_COMDEX				    0
#define WTF_P_COMDEX_RESET	    0
#define GLIDE_FP_CLAMP			    0
#define GLIDE_FP_CLAMP_TEX	    0

/* Glide HW Options */
#define GLIDE_CHIP_BROADCAST		1
#define GLIDE_HW_TRI_SETUP			1
#define GLIDE_PACKET3_TRI_SETUP 1
#define GLIDE_TRI_CULLING			  1
#define GLIDE_PACKED_RGB			  1
#define GLIDE_DISPATCH_SETUP		0
#define GLIDE_BLIT_CLEAR			  1
#define USE_PACKET_FIFO				  1

#include <MacHeaders.h>

#endif /* _MAC_GLIDE_H_ */
