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

#ifndef _PLIB_H_
#define _PLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { topOfMipMap, btmOfMipMap } mipMapEnd_t;

typedef FxU8 hTexId_t;
#define H_NULL 0x0
#define H_FSTTEXID 0x1

typedef struct {
  unsigned int isDownloadable  : 1;  /* should tex be downloaded? */
  unsigned int isMultibase     : 1;  /* downloaded as multibase? */
  unsigned int isTrilinear     : 1;  /* downloaded as EVEN/ODD? */
  unsigned int isBeenPopd      : 1;  /* has it been pop'd? - not used */
  unsigned int isDirty         : 1;  /* has it been touched? - not used */
}  tlTxMnFlags_t;
	
typedef struct {
  TlTexture*  lpTlTextureInfo;
  GrLOD_t     origLargeLod;  /* for pop 'n' push */
  GrLOD_t     origSmallLod;  /* for pop 'n' push */
	 /* I need a min of 5 start addresses worst case - Trilinear Multibase
	 ** The unions contain start addresses that are mutually exclusive 
	 ** ps, if the order of these unions ever change, some shit will seriously 
	 ** break, especially in plTxMnTexSource() */
  union u0_u {  /* can you believe softice doesn't understand unnamed unions? */
    FxU32   bothAdd;        /* BOTH linear start */
    FxU32   evenAdd;        /* EVEN linear start */
    FxU32   multiR0Add;     /* BOTH multi 1st range start */
    FxU32   multiR0EvenAdd; /* EVEN multi 1st range start */
  }  u0;
  union u1_u {
    FxU32  oddAdd;         /* ODD linear start */
    FxU32  multiR1Add;     /* BOTH multi 2nd range start */
    FxU32  multiR1OddAdd;  /* ODD multi 2nd range start */
  } u1;
  union u2_u {
    FxU32  multiR2Add;     /* BOTH multi 3rd range start */
    FxU32  multiR2EvenAdd; /* EVEN multi 3rd range start */
  } u2;
  union u3_u {
    FxU32  multiR3Add;     /* BOTH multi 4th range start */
    FxU32  multiR3OddAdd;  /* EVEN multi 4th range start */
  } u3;
  union u4_u {
    FxU32  multiR3EvenAdd;  /* ODD multi 4th range start */
  } u4;	/* just for consistency */
  tlTxMnFlags_t		flags; 
} tlTxMnTexture_t;				

/* exposed function prototypes for TxMn */
hTexId_t plTxMnLoadTxMngr( TlTexture* lpTexture );
void plTxMnDownloadAll( );
void plTxMnTexSource( hTexId_t deTexId, FxU32 deMipMask );
void plTxMnReset( hTexId_t deTexId );
void plTxMnSetDownload( hTexId_t theTexId, FxBool bDownLoad );
FxBool plTxMnGetMultibase( hTexId_t deTexId );
void plTxMnSetMultibase( hTexId_t deTexId, FxBool bMultibase );
FxBool plTxMnGetTrilinear( hTexId_t deTexId );
void plTxMnSetTrilinear( hTexId_t theTexId, FxBool bTrilinear );
FxBool plTxMnGetMultiRevOrder( );
void plTxMnSetMultiOffsetFix( FxBool bOffsetFix );
FxBool plTxMnGetMultiOffsetFix( );
void plTxMnSetMultiRevOrder( FxBool bOrder );
void plTxMnGetInfo( hTexId_t deTexHndl, GrTexInfo *deGrTexInfo );
void plTxMnDwnldMipMapLod( hTexId_t dstTexId, hTexId_t srcTexId, GrLOD_t deLod );
void plTxMnDwnldMipMapLodPrtl( hTexId_t dstTexId, hTexId_t srcTexId, GrLOD_t deLod,
                               int nFstRow, int nLstRow );
                               
/* public helper function prototypes */
void plPopLod( hTexId_t hCurrTexture, mipMapEnd_t mipMapEnd );
void plPushLod( hTexId_t hCurrTexture, mipMapEnd_t mipMapEnd );
void plCreateTlTexture( TlTexture *disTlTexture, FxU16 daColors[] );





/* some macros to play with the bit fields */
#define IS_DOWNLOADABLE( tex ) (tex).flags.isDownloadable ? FXTRUE : FXFALSE
#define IS_MULTIBASE( tex ) (tex).flags.isMultibase ? FXTRUE : FXFALSE
#define IS_BEENPOPD( tex ) (tex).flags.isBeenPopd ? FXTRUE : FXFALSE
#define IS_TRILINEAR( tex ) (tex).flags.isTrilinear ? FXTRUE : FXFALSE
#define IS_DIRTY( tex ) (tex).flags.isDirty ? FXTRUE : FXFALSE
#define SET_DOWNLOADABLE( tex, b ) (tex).flags.isDownloadable = b
#define SET_MULTIBASE( tex, b ) (tex).flags.isMultibase = b
#define SET_TRILINEAR( tex, b ) (tex).flags.isTrilinear = b
#define SET_BEENPOPD( tex, b ) (tex).flags.isBeenPopd = b
#define SET_DIRTY( tex, b ) (tex).flags.isDirty = b


#ifdef NDEBUG
#undef NDEBUG
#endif

#ifdef __cplusplus
}
#endif
#endif
