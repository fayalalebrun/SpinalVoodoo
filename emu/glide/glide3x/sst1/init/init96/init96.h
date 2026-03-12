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
** SST-96 Initialization library
*/


#ifndef __INIT96_H__
#define __INIT96_H__

/* Partners' Chips */
#define AT3D            1       /* Alliance Semiconductor AT3D*/
#define T9685           2       /* Trident 9685 */
#define MR510           3       /* MRTI MR510 */
#define MX86251         4       /* Macronics MX 86251 */

#ifndef INIT96_LIB
#define INITVG96ENTRY(name, retType, args) FX_ENTRY retType FX_CALL name args
#else
#define INITVG96ENTRY(name, retType, args) FX_EXPORT retType FX_CSTYLE name args
#endif

typedef void Init96WriteMethod( FxU32 *addr, FxU32 data );

INITVG96ENTRY(init96MapBoard, FxBool,
              (void *rD, void *info, FxU16 vID, FxU16 dID) );
INITVG96ENTRY(init96SetVideo, FxBool ,
              (FxU32 hWnd, GrScreenResolution_t sRes,
               GrScreenRefresh_t vRefresh, InitColorFormat_t cFormat,
               InitOriginLocation_t yOrigin,int nColBuffers,
               int nAuxBuffers, InitRegisterDesc *rd, int *xres, 
               int *yres, int *fbStride)); 
INITVG96ENTRY(init96RestoreVideo, void, ( InitRegisterDesc *regDesc ));
INITVG96ENTRY(init96EnableTransport, FxBool, ( InitFIFOData *info, InitRegisterDesc* regDesc));
INITVG96ENTRY(init96DisableTransport, void, ( void ));
INITVG96ENTRY(init96Swap, void ,
              (FxU32 code, InitRegisterDesc *regDesc, 
               Init96WriteMethod *wcb) );
INITVG96ENTRY(init96Idle, void , (Init96WriteMethod *wcb) );
INITVG96ENTRY(init96GetBufferPtr, void*,
              (InitBuffer_t buffer, int *strideBytes )); 
INITVG96ENTRY(init96RenderBuffer, void,
              (InitBuffer_t buffer,Init96WriteMethod *wcb));
INITVG96ENTRY(init96Origin, void,
              (InitOriginLocation_t origin, Init96WriteMethod *wcb));
INITVG96ENTRY(init96Control, FxBool , (FxU32 code, InitRegisterDesc *rd) );
INITVG96ENTRY(init96WrapFIFO, FxBool ,
              (InitRegisterDesc *rd, InitFIFOData *fifoData) );
INITVG96ENTRY(init96GetFBVram, FxU32 , (void) );
#endif /* __INIT96_H__ */
