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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <glide.h>
#include "tlib.h"
#include "plib.h"

#define MAX_HANDLES  32

//the following may need to go onto the heap...  currently need 10K, not bad
static tlTxMnTexture_t tlTxMnMngr[MAX_HANDLES];
static hTexId_t hFstOpnTexId = H_FSTTEXID;  //0 is the NULL handle
static GrChipID_t currTMU = GR_TMU0;
static FxBool bMultiRevOrder;
static FxBool bMultiOffsetFix;

/* static helper prototypes */
static FxU32 prGetLodSize( GrTexInfo *const disTexInfo, GrLOD_t disLOD );
static FxU32 prTexMultiMemRequired( FxU32 deMipMask, GrTexBaseRange_t deRange,
			    	    GrTexInfo* deTexInfo );
static void prGetLod( GrTexInfo *lpTexInfo, GrLOD_t deLod, void *data );
static void prPopLod( GrTexInfo *lpDeTexInfo, mipMapEnd_t deMipMapEnd );
static void prPushLod( GrTexInfo *lpDeTexInfo, mipMapEnd_t deMipMapEnd );
static FxU32 prGetTxMnAdd( hTexId_t deTexId, GrLOD_t deLod );
static int prGetLodHeight( GrLOD_t lod, GrAspectRatio_t aspect );
static FxU32 prGetMultiMemBump( const GrTexInfo *const deTexInfo, 
                                  GrTexBaseRange_t deTexBase,
                                  FxU32 deMipMask );


/*-------------------------------------------------------------------
  Function: plTxMnLoadTxMngr
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
  Arguments:
  Return: 
  -------------------------------------------------------------------*/
hTexId_t
plTxMnLoadTxMngr( TlTexture* lpTexture )
{
  /* assumptions */
  assert( lpTexture );

  /* code */
  tlTxMnMngr[ hFstOpnTexId ].lpTlTextureInfo = lpTexture;
  tlTxMnMngr[ hFstOpnTexId ].origLargeLod = lpTexture->info.largeLod;
  tlTxMnMngr[ hFstOpnTexId ].origSmallLod = lpTexture->info.smallLod;
  SET_DOWNLOADABLE( tlTxMnMngr[hFstOpnTexId], FXFALSE);
  SET_MULTIBASE( tlTxMnMngr[hFstOpnTexId], FXFALSE);
  SET_TRILINEAR( tlTxMnMngr[hFstOpnTexId], FXFALSE);
  SET_BEENPOPD( tlTxMnMngr[hFstOpnTexId], FXFALSE);
  SET_DIRTY( tlTxMnMngr[hFstOpnTexId], FXFALSE);

  return (hFstOpnTexId++);
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnSetDownload
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
  Arguments:
  Return: 
  -------------------------------------------------------------------*/
#if 0 /* NOT USED */
static void
prTxMnSetBeenPopd( hTexId_t deTexId, FxBool bBeenPopd )
{
  SET_BEENPOPD( tlTxMnMngr[deTexId], bBeenPopd );
  SET_DIRTY( tlTxMnMngr[deTexId], FXTRUE );  /* just for messing with it */
}
#endif
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnSetDownload
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
  Arguments:
  Return: 
  -------------------------------------------------------------------*/
void 
plTxMnSetDownload( hTexId_t deTexId, FxBool bDownLoad )
{
  /* assumptions */
  assert( deTexId < hFstOpnTexId );

  /* code */
  SET_DOWNLOADABLE( tlTxMnMngr[deTexId], bDownLoad );
  SET_DIRTY( tlTxMnMngr[deTexId], FXTRUE );  /* just for messing with it */
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnSetMultibase
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
  Arguments:
  Return: 
  -------------------------------------------------------------------*/
void 
plTxMnSetMultibase( hTexId_t deTexId, FxBool bMultibase )
{
  /* assumptions */
  assert( deTexId < hFstOpnTexId );

  /* code */
  SET_MULTIBASE( tlTxMnMngr[deTexId], bMultibase );
  SET_DIRTY( tlTxMnMngr[deTexId], FXTRUE );  /* just for messing with it */
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnGetMultibase
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
  Arguments:
  Return: 
  -------------------------------------------------------------------*/
FxBool 
plTxMnGetMultibase( hTexId_t deTexId )
{
  assert( deTexId < hFstOpnTexId );
  return (IS_MULTIBASE(tlTxMnMngr[deTexId]) );
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnSetMultiRevOrder
  Date: 7/6/97
  Implementor(s): psw
  Library: 
  Description:
    Sets the global bMultiRevOrder which is used by plTxMnDownloadAll
    to determine the order of the multibase segments in tmu mem
  Arguments:
    FxBool bOrder
  Return: 
    void
  -------------------------------------------------------------------*/
void 
plTxMnSetMultiRevOrder( FxBool bOrder )
{
  bMultiRevOrder = bOrder;
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnGetMultiRevOrder
  Date: 7/6/97
  Implementor(s): psw
  Library: 
  Description:
    Returns the current status of the bMultiRevOrder static global
  Arguments:
    none
  Return:
    FxBool
  -------------------------------------------------------------------*/
FxBool 
plTxMnGetMultiRevOrder( )
{
  return ( bMultiRevOrder );
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnSetMultiOffsetFix
  Date: 7/16/97
  Implementor(s): psw
  Library: 
  Description:
    Sets the global bMultiOffsetFix which is used by plTxMnTextureSource
    to determine to use the Multibase offset fix
  Arguments:
    FxBool bOrder
  Return: 
    void
  -------------------------------------------------------------------*/
void 
plTxMnSetMultiOffsetFix( FxBool bOffsetFix )
{
  bMultiOffsetFix = bOffsetFix;
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnGetMultiOffsetFix
  Date: 7/16/97
  Implementor(s): psw
  Library: 
  Description:
    Returns the current status of the bMultiOffsetFix static global
  Arguments:
    none
  Return:
    FxBool
  -------------------------------------------------------------------*/
FxBool 
plTxMnGetMultiOffsetFix( )
{
  return ( bMultiOffsetFix );
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnSetTrilinear
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
  Arguments:
  Return: 
  -------------------------------------------------------------------*/
void 
plTxMnSetTrilinear( hTexId_t deTexId, FxBool bTrilinear )
{
  /* assumptions */
  assert( deTexId < hFstOpnTexId );

  /* trilinear && <2-lods are mutex */
  if ( (tlTxMnMngr[ deTexId ].lpTlTextureInfo->info.smallLod - 
        tlTxMnMngr[ deTexId ].lpTlTextureInfo->info.largeLod) >= 1)
  {
    SET_TRILINEAR( tlTxMnMngr[deTexId], bTrilinear );
    SET_DIRTY( tlTxMnMngr[deTexId], FXTRUE );  /* just for messing with it */
  }
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnGetTrilinear
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
  Arguments:
  Return: 
  -------------------------------------------------------------------*/
FxBool 
plTxMnGetTrilinear( hTexId_t deTexId )
{
  assert( deTexId < hFstOpnTexId );
  return (IS_TRILINEAR(tlTxMnMngr[deTexId]) );
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnReset
  Date: 6/26/97
  Implementor(s): psw
  Library: 
  Description:
    Resets the respective texture back to its base state
  Arguments:
    hTexId_t deTexId - the id of the tlTxMnMngr texture to reset
  Return: 
    void
  -------------------------------------------------------------------*/
void 
plTxMnReset( hTexId_t deTexId )
{
  /* initializations */
  tlTxMnTexture_t *lpThisTxMnTexture;  
  GrTexInfo *lpThisTexInfo;  
  
  /* assumptions */
  assert( deTexId < hFstOpnTexId );

  /* initializations */
  lpThisTxMnTexture = &tlTxMnMngr[deTexId];
  lpThisTexInfo = &lpThisTxMnTexture->lpTlTextureInfo->info;

  /* code */
  SET_MULTIBASE( tlTxMnMngr[deTexId], FXFALSE );
  SET_TRILINEAR( tlTxMnMngr[deTexId], FXFALSE );
  bMultiRevOrder = FXFALSE;
  /* SET_DIRTY( tlTxMnMngr[deTexId], FXFALSE ); */

  while ( lpThisTexInfo->largeLod > lpThisTxMnTexture->origLargeLod )
    prPushLod( lpThisTexInfo, topOfMipMap );/* reset d'm data ptrs */
  
  while ( lpThisTexInfo->smallLod < lpThisTxMnTexture->origSmallLod )
    prPushLod (lpThisTexInfo, btmOfMipMap );/* reset d'm data ptrs */
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnDownloadAll
  Date: 6/27/97
  Implementor(s): psw
  Library: 
  Description:
    Downloads all the textures be held by tlTexManMngr[]
    Will determine if the textures are to be downloaded 
    in (linear || multibase) && (BOTH || EVEN/ODD) mode based upon 
    their flags and do the right thang
  Arguments:
    None
  Return:
    void
  -------------------------------------------------------------------*/
void 
plTxMnDownloadAll( )
{
  /* definitions */
  hTexId_t         hCurrTexId;
  FxU32            currTexMemReq;  
  FxU32             currStartAdd;    
  tlTxMnTexture_t  *lpThisTxMnTexture;  
  GrTexInfo        *lpThisTexInfo;  
  TlTexture        *lpThisTlTex;

  /* assumptions */
  assert(hFstOpnTexId != 0);

  /* initializations */
  currStartAdd = grTexMinAddress(currTMU);

  /* code */
  for ( hCurrTexId = H_FSTTEXID; hCurrTexId < hFstOpnTexId; ++hCurrTexId )
  {    /* here we go */
    if (IS_DOWNLOADABLE(tlTxMnMngr[hCurrTexId]))
    {  /* download it */
      lpThisTxMnTexture = &tlTxMnMngr[hCurrTexId];
      lpThisTlTex = lpThisTxMnTexture->lpTlTextureInfo;
      lpThisTexInfo = &lpThisTlTex->info;
      if (!IS_MULTIBASE(*lpThisTxMnTexture))
      {  /* not multibased */
        if (!IS_TRILINEAR(*lpThisTxMnTexture))
        {  /* not trilinear mip'd */
          currTexMemReq = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, lpThisTexInfo);
          assert( currStartAdd + currTexMemReq < grTexMaxAddress(currTMU) );
          grTexDownloadMipMap( currTMU, currStartAdd, 
                                 GR_MIPMAPLEVELMASK_BOTH, lpThisTexInfo );
          lpThisTxMnTexture->u0.bothAdd = currStartAdd;
          currStartAdd += currTexMemReq;
        }  /* if !IS_TRILINEAR && !IS_MULTIBASE */
        else
        { /* else it is being rendered as trilinear mip'd */
          /* first do the even segment */
          currTexMemReq = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_EVEN, lpThisTexInfo);
          assert( currStartAdd + currTexMemReq < grTexMaxAddress(currTMU) );
          grTexDownloadMipMap( currTMU, currStartAdd, 
                                 GR_MIPMAPLEVELMASK_EVEN, lpThisTexInfo );
          lpThisTxMnTexture->u0.evenAdd = currStartAdd;
          currStartAdd += currTexMemReq;
          /* next do the odd segment */
          currTexMemReq = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD, lpThisTexInfo);
          assert( currStartAdd + currTexMemReq < grTexMaxAddress(currTMU) );
          grTexDownloadMipMap( currTMU, currStartAdd, 
                                 GR_MIPMAPLEVELMASK_ODD, lpThisTexInfo );
          lpThisTxMnTexture->u1.oddAdd = currStartAdd;
          currStartAdd += currTexMemReq;
        } /* else !IS_MULTIBASE && IS_TRILINEAR */
      } /* if !IS_MULTIBASE */
      else /* else it is multibased */
      {  
        /* stack frame definitions */
        FxU32                   *lpxAdd;
        GrTexBaseRange_t   tTxBse;

        grTexMultibase(currTMU, FXTRUE);  /* turn on multibase */
        if (!IS_TRILINEAR(*lpThisTxMnTexture))
        { /* non trilinear mip'd multibase texture
          ** if bMultiRevOrder is True, get the addresses for the Multibase segs
          ** in reverse order, 32_TO_1 to 256, else get them in 256 to 32_TO_1 */

          for ( tTxBse = bMultiRevOrder ? GR_TEXBASE_32_TO_1 : GR_TEXBASE_256,
                  lpxAdd = bMultiRevOrder ? &lpThisTxMnTexture->u3.multiR3Add :
                  &lpThisTxMnTexture->u0.multiR0Add; 
                bMultiRevOrder ? lpxAdd >= &lpThisTxMnTexture->u0.multiR0Add :
                  lpxAdd <= &lpThisTxMnTexture->u3.multiR3Add;
                (bMultiRevOrder ? --tTxBse : ++tTxBse), 
                  (bMultiRevOrder ? --lpxAdd : ++lpxAdd) )
          { /* this loop just gets the texture addresses - the multibase 
            ** segments will be in reverse order, but in the same linear block */
            currTexMemReq = prTexMultiMemRequired( GR_MIPMAPLEVELMASK_BOTH,
                                                     tTxBse, lpThisTexInfo);
            if (currTexMemReq != 0)  /* if the LOD(s) !missing, ie. pop'd */
            {
              assert( currStartAdd + currTexMemReq < grTexMaxAddress(currTMU) );
              *lpxAdd = currStartAdd;
              currStartAdd += currTexMemReq; /* bump the current TMU address */
            }
          } /* for */
          for ( tTxBse = GR_TEXBASE_256, lpxAdd = &lpThisTxMnTexture->u0.multiR0Add;
                  lpxAdd <= &lpThisTxMnTexture->u3.multiR3Add;
                  ++tTxBse, ++lpxAdd )
          { /* this loop actually does the downloading - in the proper order...
            ** GR_TEXBASE_256 to GR_TEXBASE_32_TO_1 */

            /* stack frame definitions */
            GrTexInfo   tmpInfo;
            GrTexInfo    *lpTmpInfo = &tmpInfo;

            /* initializations */
            /* multibase requires that I munge the data for downloading */
            *lpTmpInfo = *lpThisTexInfo;
            if ((lpThisTexInfo->largeLod <= (FxI32) tTxBse ) && 
                (lpThisTexInfo->smallLod >= (FxI32) tTxBse ))
            { /* if the TexBase exists in the lod range */
              while ( lpTmpInfo->largeLod < (FxI32) tTxBse )
                prPopLod( lpTmpInfo, topOfMipMap ); /* munge the top */
  
              if ( tTxBse != GR_TEXBASE_32_TO_1 )
                lpTmpInfo->smallLod = lpTmpInfo->largeLod; /* munge the btm */

              grTexDownloadMipMap(currTMU, *lpxAdd,  /* download mung'd data */
                                    GR_MIPMAPLEVELMASK_BOTH, lpTmpInfo );
            }  /* if */
          }  /* for */
        } /* if !IS_TRILINEAR && IS_MULTIBASE */
        else
        { /* else it is a multibased trilinear mipmap, aka motherfucker
          ** if bMultiRevOrder is True, get the addresses for the Multibase segs
          ** in reverse order, 32_TO_1 to 256, else get them in 256 to 32_TO_1 */
          for (tTxBse = bMultiRevOrder ? GR_TEXBASE_32_TO_1+1 : GR_TEXBASE_256,
                 lpxAdd = bMultiRevOrder ? &lpThisTxMnTexture->u4.multiR3EvenAdd :
                 &lpThisTxMnTexture->u0.multiR0EvenAdd; 
               bMultiRevOrder ? lpxAdd >= &lpThisTxMnTexture->u0.multiR0EvenAdd :
                 lpxAdd <= &lpThisTxMnTexture->u4.multiR3EvenAdd;
               (bMultiRevOrder ? --tTxBse : ++tTxBse), 
                 (bMultiRevOrder ? --lpxAdd : ++lpxAdd) )
          { /* this loop just gets the texture addresses */
            if ( tTxBse % 2 ) /* is it odd? */
              currTexMemReq = prTexMultiMemRequired(GR_MIPMAPLEVELMASK_ODD,
                                                      tTxBse, lpThisTexInfo); /* 1,3 */
            else  /* even */
              currTexMemReq = prTexMultiMemRequired(GR_MIPMAPLEVELMASK_EVEN,
                                                     (tTxBse>3?3:tTxBse),
                                                     lpThisTexInfo);/* 0,2,3 */
                                        /* prTexMulti.. only knows about 0-3 */
            if (currTexMemReq != 0)  /* if the LOD(s) !missing, ie. pop'd */
            {
              assert( currStartAdd + currTexMemReq < grTexMaxAddress(currTMU) );
              *lpxAdd = currStartAdd;
              currStartAdd += currTexMemReq;/* bump the current TMU address */
            }
          } /* for */
          for (tTxBse = GR_TEXBASE_256,
                 lpxAdd = &lpThisTxMnTexture->u0.multiR0EvenAdd;
               lpxAdd <= &lpThisTxMnTexture->u4.multiR3EvenAdd;
               ++tTxBse, ++lpxAdd ) /* tTxBse will actually overrun... 
                                    ** overrun = GR_TEXBASE_32_TO_1 + 1
                                    ** (tTxBse>3?3:tTxBse) will fix this
                                    ** required for 32to1_ODD and 32to1_EVEN */
            /* actually (tTxBse>GR_TEXBASE_32_TO_1:GR_TEXBASE_32_TO_1?tTxBse) */
          { /* this loop actually does the downloading - in the proper order...
            ** GR_TEXBASE_256 to GR_TEXBASE_32_TO_1 */
  
            /* stack frame definitions */
            GrTexInfo   tmpInfo;
            GrTexInfo    *lpTmpInfo = &tmpInfo;

            /* initializations */
              /* multibase requires that I munge the data, hence the data copies */
            *lpTmpInfo = *lpThisTexInfo;

            if ((lpThisTexInfo->largeLod <= (FxI32) (tTxBse>3?3:tTxBse)) && 
                (lpThisTexInfo->smallLod >= (FxI32) (tTxBse>3?3:tTxBse)))
            { /* if an lod actually exits in the range */
              while ( lpTmpInfo->largeLod < (FxI32)(tTxBse>3?3:tTxBse) )
                prPopLod( lpTmpInfo, topOfMipMap ); /* munge the top */
  
              if ( (tTxBse>3?3:tTxBse) != GR_TEXBASE_32_TO_1 )
                lpTmpInfo->smallLod = lpTmpInfo->largeLod; /* munge the btm */

              if ( tTxBse % 2 ) /* is it odd? */
                grTexDownloadMipMap(currTMU, *lpxAdd,  /* download mung'd odd data */
                                      GR_MIPMAPLEVELMASK_ODD, lpTmpInfo );
              else
                grTexDownloadMipMap(currTMU, *lpxAdd,  /* download mung'd even data */
                                      GR_MIPMAPLEVELMASK_EVEN, lpTmpInfo );
            } /* if */
          }  /* for */
        } /* else IS_TRILINEAR && IS_MULTIBASE */
        grTexMultibase(currTMU, FXFALSE);  /* turn off multibase */
      } /* else IS_MULTIBASE */
      if ( lpThisTlTex->tableType != NO_TABLE )
        grTexDownloadTable(currTMU, lpThisTlTex->tableType, 
                             &lpThisTlTex->tableData );
    }  /* if IS_DOWNLOADABLE */
  } /* for ( i = 0; i < fstOpnTexId; ++i ) */
}  /* plTxMnDownloadAll */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnTexSource
  Date: 6/28/97
  Implementor(s): psw
  Library: 
  Description:
    Renders the texture associated with hTexId.  Will setup the rendering
    modes based upon the tlTxMnMngr[deTexId].flags fields, i.e., 
    (linear || multibase) && (BOTH || EVEN/ODD) modes.  Also uses
    the addresses saved by previous call to tlTxMnDownloadAll 
  Arguments:
    hTexId_t deTexId - handle to the texture
    FxU32 deMipMask - mipmap mask
  Return: 
    void
  -------------------------------------------------------------------*/
void
plTxMnTexSource( hTexId_t deTexId, FxU32 deMipMask )
{
  /* definitions */
  tlTxMnTexture_t   *lpThisTxMnTexture;  
  GrTexInfo         thisTexInfo,
                    *lpThisTexInfo;
  TlTexture         *lpThisTlTex;
  FxU32             *lpxAdd,
                    xAdd;
  GrTexBaseRange_t  tTexBase;

  /* assumptions */
  assert( deTexId < hFstOpnTexId );
  assert((deMipMask >= GR_TEXBASE_256) &&
         (deMipMask <= GR_TEXBASE_32_TO_1));

  /* initializations */
  lpThisTxMnTexture = &tlTxMnMngr[deTexId];
  lpThisTlTex = lpThisTxMnTexture->lpTlTextureInfo;
  lpThisTexInfo = &thisTexInfo;
  *lpThisTexInfo = lpThisTxMnTexture->lpTlTextureInfo->info;

/*
  if ((lpThisTlTex->tableType == GR_NCCTABLE_NCC0) || 
      (lpThisTlTex->tableType == GR_NCCTABLE_NCC1))
    grTexNCCTable( currTMU, lpThisTlTex->tableType );
*/
  if (!IS_MULTIBASE(*lpThisTxMnTexture))
  {  /* not multibased */
    if (!IS_TRILINEAR(*lpThisTxMnTexture))
      grTexSource( currTMU, lpThisTxMnTexture->u0.bothAdd, deMipMask, lpThisTexInfo );
    else  /* IS_TRILINEAR */
      if ( deMipMask == GR_MIPMAPLEVELMASK_EVEN ) /* first do the even segment */
        grTexSource( currTMU, lpThisTxMnTexture->u0.evenAdd, deMipMask, lpThisTexInfo );
      else        /* next do the odd segment */
        grTexSource( currTMU, lpThisTxMnTexture->u1.oddAdd, deMipMask, lpThisTexInfo );
  }/* if !IS_MULTIBASE */
  else
  {  /* else it is multibased */   
    grTexMultibase(currTMU, FXTRUE); /* turn on multibase */
    if (!IS_TRILINEAR(*lpThisTxMnTexture))
    {  /* really dependant on the unions being in the proper order */
      lpxAdd = &lpThisTxMnTexture->u0.multiR0Add;
      for  (tTexBase = GR_TEXBASE_256; tTexBase <= GR_TEXBASE_32_TO_1; 
              ++tTexBase, ++lpxAdd)  
      /* next i check that the current TexBase is in the lod range */
        if ((lpThisTexInfo->largeLod <= (FxI32) tTexBase) &&
            (lpThisTexInfo->smallLod >= (FxI32) tTexBase))
        {
          xAdd = *lpxAdd;
          if (bMultiOffsetFix)
            xAdd -= prGetMultiMemBump(lpThisTexInfo, tTexBase, deMipMask);
          grTexMultibaseAddress(currTMU, tTexBase, xAdd, deMipMask,
                                  lpThisTexInfo );
        }
    } /* if !IS_TRILINEAR */
    else 
    { /* else it is a multibased trilinear mipmap */
      if ( deMipMask == GR_MIPMAPLEVELMASK_EVEN ) /* do the even segments */
      {  
        lpxAdd = &lpThisTxMnTexture->u0.multiR0EvenAdd;/* set lpxAdd to 1st add */
        for  (tTexBase = GR_TEXBASE_256; tTexBase <= GR_TEXBASE_32_TO_1;
              tTexBase = ( tTexBase + 2 == GR_TEXBASE_32_TO_1 + 1 ) ? /* 32t1 is both even & odd */
              GR_TEXBASE_32_TO_1 : tTexBase + 2, lpxAdd += 2 ) /* even segs r at even indices */
        /* next i check that the current TexBase is in the lod range */
          if ((lpThisTexInfo->largeLod <= (FxI32) tTexBase) &&
              (lpThisTexInfo->smallLod >= (FxI32) tTexBase))
          {
            xAdd = *lpxAdd;
            if (bMultiOffsetFix)
              xAdd -= prGetMultiMemBump(lpThisTexInfo, tTexBase, deMipMask);
            grTexMultibaseAddress(currTMU, tTexBase, xAdd, deMipMask,
                                    lpThisTexInfo);
          }
      } /* if - even */
      else  /* else do the odd segments */
      {
        lpxAdd = &lpThisTxMnTexture->u1.multiR1OddAdd;
        for  (tTexBase = GR_TEXBASE_128; tTexBase <= GR_TEXBASE_32_TO_1; 
                tTexBase+=2, lpxAdd+=2 ) /* odd segs r at odd indices */
        /* next i check that the current TexBase is in the lod range */
          if ((lpThisTexInfo->largeLod <= (FxI32) tTexBase) &&
              (lpThisTexInfo->smallLod >= (FxI32) tTexBase))
          {
            xAdd = *lpxAdd;
            if (bMultiOffsetFix)
              xAdd -= prGetMultiMemBump(lpThisTexInfo, tTexBase, deMipMask);
            grTexMultibaseAddress( currTMU, tTexBase, xAdd, deMipMask,
                                     lpThisTexInfo );
          }
      } /* else - odd */
    } /* else IS_TRILINEAR && IS_MULTIBASE */
    grTexMultibase(currTMU, FXFALSE);  /* turn off multibase */
  } /* else IS_MULTIBASE */
}  /* plTxMnRender */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnGetInfo
  Date: 6/27/97
  Implementor(s): psw
  Library: 
  Description:
    simply fills in GrTexInfo struct with info about Texture
  Arguments:
    hTexId_t deTexHndl - the texture that you want to know about
    GrTexInfo *deGrTexInfo - the info struct you want filled
  Return: 
    void
  -------------------------------------------------------------------*/
void 
plTxMnGetInfo( hTexId_t deTexHndl, GrTexInfo *deGrTexInfo )
{

  /* assumptions */
  assert (deGrTexInfo);
  assert ( deTexHndl < hFstOpnTexId );

  *deGrTexInfo =  tlTxMnMngr[deTexHndl].lpTlTextureInfo->info;
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plPopLod
  Date: 6/17/97
  Implementor(s): psw
  Library: 
  Description:
    Basically a wrapper for prPopLod  
  Arguments:
    hTexId_t thisTexId
    mipMapEnd_t deMipMapEnd
  Return: 
    none
  -------------------------------------------------------------------*/
void 
plPopLod( hTexId_t hCurrTexture, mipMapEnd_t deMipMapEnd )
{
  /* definitions  */
  GrTexInfo        *lpThisTexInfo;    
  tlTxMnTexture_t  *lpThisTxMnTexture;

  /* assumptions */
  assert( hCurrTexture < hFstOpnTexId );
 

  /* initializations */
  lpThisTxMnTexture = &tlTxMnMngr[hCurrTexture];
  lpThisTexInfo = &lpThisTxMnTexture->lpTlTextureInfo->info;

  /* code */
  if ( lpThisTexInfo->smallLod == lpThisTexInfo->largeLod )
    return;  /* no reason to go on, nothing to do */

  if ((IS_TRILINEAR(*lpThisTxMnTexture)) && 
       ((lpThisTexInfo->smallLod - lpThisTexInfo->largeLod) == 2 ) )
    return;  /* trilinear && <2-lods are mutex */

  prPopLod( lpThisTexInfo, deMipMapEnd );
} /* plPopLod */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plPushLod
  Date: 6/17/97
  Implementor(s): psw
  Library: 
  Description:
    Basically a wrapper for prPushLod  
  Arguments:
    hTexId_t thisTexId
    mipMapEnd_t deMipMapEnd
  Return: 
    none
  -------------------------------------------------------------------*/
void 
plPushLod( hTexId_t hCurrTexture, mipMapEnd_t deMipMapEnd )
{
  /* definitions  */
  GrTexInfo *lpThisTexInfo;/* info struct associated with curr tlTxMnMnger obj */
  tlTxMnTexture_t *lpThisTxMnTexture;

  /* assumptions */
  assert( hCurrTexture < hFstOpnTexId );
  
  /* initializations */
  lpThisTxMnTexture = &tlTxMnMngr[hCurrTexture];
  lpThisTexInfo = &lpThisTxMnTexture->lpTlTextureInfo->info;

  /* code */
  if ((deMipMapEnd == topOfMipMap) && 
      (lpThisTexInfo->largeLod == lpThisTxMnTexture->origLargeLod))
    return;  /* already at the top of the stack */
  
  if ((deMipMapEnd == btmOfMipMap) && 
      (lpThisTexInfo->smallLod == lpThisTxMnTexture->origSmallLod))
    return;  /* already at the bottom of the stack */

  prPushLod( lpThisTexInfo, deMipMapEnd);
} /* plPushLod */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plCreateTlTexture
  Date: 6/20/97
  Implementor(s): psw
  Library: 
  Description:
    creates a TlTexture
  Arguments:
    GrTexInfo*
    FxU16 []
  Return: 
    void
  -------------------------------------------------------------------*/
void 
plCreateTlTexture( TlTexture *deTlTexture, FxU16 daColors[] )
{
  /* definitions  */
  GrLOD_t    tCurrLod;        /* the current LOD */
  int        nBytePerPixel;   /* number of bytes per pixel */
  FxU8       *spLilTex;       /* 8bit LOD data ptr */
  FxU16      *mpBigTex;       /* 16bit LOD data ptr */
  void       *lpvCurrLod;     /* curr LOD data ptr */ 
  FxU32      xLodNumBytes;    /* number of bytes to write in curr LOD */
  GrTexInfo  *lpThisTexInfo;  /* ptr to GrTexInfo in TlTexture struct */

  /* assumptions */
  assert( deTlTexture );
  assert( deTlTexture->info.data );
  assert( deTlTexture->info.smallLod > deTlTexture->info.largeLod );

  /* no palettized */
  assert( (deTlTexture->info.format != GR_TEXFMT_YIQ_422)   ||
          (deTlTexture->info.format != GR_TEXFMT_P_8)       ||
          (deTlTexture->info.format != GR_TEXFMT_AYIQ_8422) ||
          (deTlTexture->info.format != GR_TEXFMT_AP_88) );

  /* initializations */
  lpThisTexInfo = &deTlTexture->info;

  /* code */
  if ((lpThisTexInfo->format == GR_TEXFMT_RGB_332)     ||
      (lpThisTexInfo->format == GR_TEXFMT_YIQ_422)     ||
      (lpThisTexInfo->format == GR_TEXFMT_ALPHA_8)     ||
      (lpThisTexInfo->format == GR_TEXFMT_INTENSITY_8) ||
      (lpThisTexInfo->format == GR_TEXFMT_P_8))
    nBytePerPixel = 1;  /* 8-bit formats */
  else
    nBytePerPixel = 2;  /* 16-bit formats */

  lpvCurrLod = lpThisTexInfo->data;

  if ((deTlTexture->info.format != GR_TEXFMT_YIQ_422)   ||
      (deTlTexture->info.format != GR_TEXFMT_P_8)       ||
      (deTlTexture->info.format != GR_TEXFMT_AYIQ_8422) ||
      (deTlTexture->info.format != GR_TEXFMT_AP_88) )
    deTlTexture->tableType = NO_TABLE; /* fuck me */
  
  for ( tCurrLod = lpThisTexInfo->largeLod; 
        tCurrLod <= (lpThisTexInfo->smallLod); ++tCurrLod )
  {
    if ( nBytePerPixel == 1 )
    {
      xLodNumBytes = prGetLodSize( lpThisTexInfo, tCurrLod );
      for (spLilTex = (FxU8*) lpvCurrLod; 
           spLilTex < (FxU8*) ((FxU32)lpvCurrLod + xLodNumBytes);
           ++spLilTex )  
        *spLilTex = (FxU8) daColors[tCurrLod];

      lpvCurrLod = (void*) spLilTex;
    }
    else
    {
      xLodNumBytes = prGetLodSize( lpThisTexInfo, tCurrLod );
      for (mpBigTex = (FxU16*) lpvCurrLod; 
           mpBigTex < (FxU16*) ((FxU32)lpvCurrLod + xLodNumBytes);
           ++mpBigTex )  
        *mpBigTex = daColors[tCurrLod];

      lpvCurrLod = (void*) mpBigTex;
    }
  }
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnDwnldMipMapLod
  Date: 7/7/97
  Implementor(s): psw
  Library: 
  Description:
    downloads a mipmap lod, deLod, from a src TxMnTexture to a dst
    TxMnTexture.  
    ***multibase not yet supported, not clearly defined
  Arguments:
    hTexId_t dstTexId - destination TxMnTexture id
    hTexId_t srcTexId - source TxMnTexture id
    GrLOD_t deLod - the lod to download in srcTexId 
  Return: 
    void
  -------------------------------------------------------------------*/
void 
plTxMnDwnldMipMapLod( hTexId_t dstTexId, hTexId_t srcTexId, GrLOD_t deLod )
{
  /* definitions  */
  GrTexInfo        *lpDstTexInfo,
                   *lpSrcTexInfo;
  tlTxMnTexture_t  *lpDstTxMnTexture,
                   *lpSrcTxMnTexture;
  void             *lpvData;
  FxU32            thisAdd;
  GrLOD_t          thisLgLod;
  FxU32            thisMask;

  /* assumptions */
  assert( dstTexId < hFstOpnTexId );
  assert( srcTexId < hFstOpnTexId );
  assert( IS_DOWNLOADABLE(tlTxMnMngr[dstTexId]) );

  /* initializations */
  lpDstTxMnTexture = &tlTxMnMngr[dstTexId];
  lpSrcTxMnTexture = &tlTxMnMngr[srcTexId];
  lpDstTexInfo = &lpDstTxMnTexture->lpTlTextureInfo->info;
  lpSrcTexInfo = &lpSrcTxMnTexture->lpTlTextureInfo->info;

  /* just some error checking */
  if ( ((lpDstTexInfo->format) != (lpSrcTexInfo->format)) ||
       ((lpDstTexInfo->aspectRatio) != (lpSrcTexInfo->aspectRatio)) ||
       ((lpSrcTxMnTexture->origLargeLod) > deLod) ||
       ((lpSrcTxMnTexture->origSmallLod) < deLod) ||
       ((lpDstTxMnTexture->origLargeLod) > deLod) ||
       ((lpDstTxMnTexture->origSmallLod) < deLod) )
    return; /* nonfatal error, just return */

  /* code */
  /* get the data */
  lpvData = malloc(prGetLodSize(lpSrcTexInfo, deLod));
  assert(lpvData);
  prGetLod(lpSrcTexInfo, deLod, lpvData);  
  /* now i have an lod to download */

  /* get the address */
  thisAdd = prGetTxMnAdd( dstTexId, deLod );
  /* get the mask */
  if (!IS_TRILINEAR(*lpDstTxMnTexture))
    thisMask = GR_MIPMAPLEVELMASK_BOTH;
  else  /* it is trilinear */
    thisMask = deLod%2 ? GR_MIPMAPLEVELMASK_ODD : GR_MIPMAPLEVELMASK_EVEN;

  /* get the largeLod, SPECIAL case for multibase */  
  if (!IS_MULTIBASE(*lpDstTxMnTexture))
    thisLgLod = lpDstTexInfo->largeLod;
  else /* is is multibase */
    thisLgLod = deLod < GR_LOD_32 ? deLod : 
      lpDstTexInfo->largeLod > GR_LOD_32 ? 
      lpDstTexInfo->largeLod : GR_LOD_32; 

  grTexDownloadMipMapLevel( currTMU, thisAdd, deLod, thisLgLod,
                              lpDstTexInfo->aspectRatio, lpDstTexInfo->format, 
                              thisMask, lpvData );
  free(lpvData);
} /* plTxMnDwnldMipMapLod */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: plTxMnDwnldMipMapLod
  Date: 7/7/97
  Implementor(s): psw
  Library: 
  Description:
    downloads a mipmap lod, deLod, from a src TxMnTexture to a dst
    TxMnTexture.  
  Arguments:
    hTexId_t dstTexId - destination TxMnTexture id
    hTexId_t srcTexId - source TxMnTexture id
    GrLOD_t deLod - the lod to download in srcTexId 
  Return: 
    void
  -------------------------------------------------------------------*/
void 
plTxMnDwnldMipMapLodPrtl( hTexId_t dstTexId, hTexId_t srcTexId, GrLOD_t deLod,
                          int nFstRow, int nLstRow )
{
  /* definitions  */
  GrTexInfo        *lpDstTexInfo,
                   *lpSrcTexInfo;
  tlTxMnTexture_t  *lpDstTxMnTexture,
                   *lpSrcTxMnTexture;
  void             *lpvData;
  FxU32            thisAdd;
  GrLOD_t          thisLgLod;
  FxU32            thisMask;

  /* assumptions */
  assert( dstTexId < hFstOpnTexId );
  assert( srcTexId < hFstOpnTexId );
  assert( IS_DOWNLOADABLE(tlTxMnMngr[dstTexId]) );
  assert( nFstRow <= nLstRow );

  /* initializations */
  lpDstTxMnTexture = &tlTxMnMngr[dstTexId];
  lpSrcTxMnTexture = &tlTxMnMngr[srcTexId];
  lpDstTexInfo = &lpDstTxMnTexture->lpTlTextureInfo->info;
  lpSrcTexInfo = &lpSrcTxMnTexture->lpTlTextureInfo->info;

  /* code */
  /* just some error checking */
  if ( ((lpDstTexInfo->format) != (lpSrcTexInfo->format))            ||
       ((lpDstTexInfo->aspectRatio) != (lpSrcTexInfo->aspectRatio))  ||
       ((lpSrcTxMnTexture->origLargeLod) > deLod)                    ||
       ((lpSrcTxMnTexture->origSmallLod) < deLod)                    ||
       ((lpDstTxMnTexture->origLargeLod) > deLod)                    ||
       ((lpDstTxMnTexture->origSmallLod) < deLod)                    ||
       (nFstRow > prGetLodHeight( deLod, lpDstTexInfo->aspectRatio)) )
    return; /* nonfatal error, just return */

  /* because i'm a nice guy */
//  if (nLstRow >= prGetLodHeight( deLod, lpDstTexInfo->aspectRatio))
//    nLstRow = (-1) + prGetLodHeight( deLod, lpDstTexInfo->aspectRatio);
  /* not so nice */

  /* get the data */
  lpvData = malloc(prGetLodSize(lpSrcTexInfo, deLod));
  assert(lpvData);
  prGetLod(lpSrcTexInfo, deLod, lpvData);
  /* now i have an lod to download */
 
  /* get the address */
  thisAdd = prGetTxMnAdd( dstTexId, deLod );

  /* get the mask */
  if (!IS_TRILINEAR(*lpDstTxMnTexture))
    thisMask = GR_MIPMAPLEVELMASK_BOTH;
  else  /* it is trilinear */
    thisMask = deLod%2 ? GR_MIPMAPLEVELMASK_ODD : GR_MIPMAPLEVELMASK_EVEN;

  /* get the largeLod, SPECIAL case for multibase */  
  if (!IS_MULTIBASE(*lpDstTxMnTexture))
    thisLgLod = lpDstTexInfo->largeLod;
  else /* is is multibase */
    thisLgLod = deLod < GR_LOD_32 ? deLod : 
      lpDstTexInfo->largeLod > GR_LOD_32 ? 
      lpDstTexInfo->largeLod : GR_LOD_32; 

  grTexDownloadMipMapLevelPartial( currTMU, thisAdd, deLod, thisLgLod,
                                     lpDstTexInfo->aspectRatio, lpDstTexInfo->format, 
                                     thisMask, lpvData, nFstRow, nLstRow );
   free(lpvData);
}
/*-------------------------------------------------------------------*/



/* static helpers */

/*-------------------------------------------------------------------
  Function: prGetTxMnAdd
  Date: 7/14/97
  Implementor(s): psw
  Library: 
  Description:
    returns the address of the texture seg in linear memory that contains
    the LOD being sought based on TxMn id 
  Arguments:
    hTexId_t deTexId - destination TxMnTexture id
    GrLOD_t deLod - the lod to locate the seg in
  Return: 
    FxU32 - address of the seg containing deLod
  -------------------------------------------------------------------*/
static FxU32
prGetTxMnAdd( hTexId_t deTexId, GrLOD_t deLod )
{
  /* definitions */
  tlTxMnTexture_t  *lpThisTxMnTxtre;
  FxU32            xAdd;

  /* initializations */
  lpThisTxMnTxtre = &tlTxMnMngr[deTexId];
 
  /* code */
  if (!IS_MULTIBASE(*lpThisTxMnTxtre))
  {  /* not multibased */
    if (!IS_TRILINEAR(*lpThisTxMnTxtre))
      xAdd = lpThisTxMnTxtre->u0.bothAdd;
    else  /* IS_TRILINEAR */
      xAdd = (deLod%2) ? lpThisTxMnTxtre->u1.oddAdd :
                         lpThisTxMnTxtre->u0.evenAdd ;
  } /* if !IS_MULTIBASE */
  else /* this is going to be fucking ugly! */
  {  /* else it is multibased */
    if (!IS_TRILINEAR(*lpThisTxMnTxtre))
    {
      switch ( deLod )
      {  /* some things are better done with a switch... */
        case GR_LOD_256:
          xAdd = (FxU32) &lpThisTxMnTxtre->u0.multiR0Add;
        break;          

        case GR_LOD_128:
          xAdd = (FxU32) &lpThisTxMnTxtre->u1.multiR1Add; 
        break;          

        case GR_LOD_64:
          xAdd = (FxU32) &lpThisTxMnTxtre->u2.multiR2Add; 
        break;          

        default:
          xAdd = (FxU32) &lpThisTxMnTxtre->u3.multiR3Add; 
        break;
      }
    } /* if !IS_TRILINEAR */
    else 
    {
      switch ( deLod )
      {  /* some things are better done with a switch... */
        case GR_LOD_256:
          xAdd = (FxU32) &lpThisTxMnTxtre->u0.multiR0EvenAdd;
        break;          

        case GR_LOD_128:
          xAdd = (FxU32) &lpThisTxMnTxtre->u1.multiR1OddAdd; 
        break;          

        case GR_LOD_64:
          xAdd = (FxU32) &lpThisTxMnTxtre->u2.multiR2EvenAdd; 
        break;          

        default: /* deLod <= 32_TO_1, key is could be < */
          xAdd = (deLod % 2) ? (FxU32) &lpThisTxMnTxtre->u3.multiR3OddAdd :
                               (FxU32) &lpThisTxMnTxtre->u4.multiR3EvenAdd;
        break;
      }
    }  /* else IS_TRILINEAR */
  } /* else IS_MULTIBASE */

  return (xAdd);
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: prGetLod
  Date: 7/7/97
  Implementor(s): psw
  Library: 
  Description:
    gets the lod specific data out of a GrTexInfo designated by 
    lpTexInfo, used primarily by plTxMnDwnldMipMapLod
  Arguments:
    GrTexInfo *lpTexInfo - destination TxMnTexture id
    GrLOD_t deLod - the lod get in deTexId 
    void *lpvData - pointer to data to propagate
  Return: 
    void
  -------------------------------------------------------------------*/
static void
prGetLod( GrTexInfo *lpTexInfo, GrLOD_t deLod, void *lpvData )
{
  /* definitions */
  GrTexInfo  thisTexInfo, 
             *lpThisTexInfo;
  GrLOD_t    lod;
  FxU32      i, xNumBytes;
  FxU8       *lpSrc,
             *lpDst;
  
  /* initializations */
  lpThisTexInfo = &thisTexInfo;
  *lpThisTexInfo = *lpTexInfo;  /* munge a copy */

  /* code */
  /* mod copy to get deLod at the top */
  for ( lod = lpThisTexInfo->largeLod; lod < deLod; ++lod )
    prPopLod( lpThisTexInfo, topOfMipMap );

  /* get the size of top lod  */
  xNumBytes = prGetLodSize( lpThisTexInfo, lpThisTexInfo->largeLod );

  /* read the top lod data into *lpvData */
  lpSrc = (FxU8*) lpThisTexInfo->data;
  lpDst = (FxU8*) lpvData;

  for ( i = 0; i < xNumBytes; ++i )
    *lpDst++ = *lpSrc++;
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: prPushLod
  Date: 7/7/97
  Implementor(s): psw
  Library: 
  Description:
    This routine actually modifies the GrTexInfo struct data to push
    data BACK on the top or bottom of the mipmap. pushes a single lod
  Arguments:
    GrTexInfo *lpDeTexInfo
    mipMapEnd_t deMipMapEnd
  Return: 
    none
  -------------------------------------------------------------------*/
static void 
prPushLod( GrTexInfo *lpDeTexInfo, mipMapEnd_t deMipMapEnd )
{
  if (deMipMapEnd == topOfMipMap)
    lpDeTexInfo->data = (void*)( (FxU32) lpDeTexInfo->data - 
                          prGetLodSize(lpDeTexInfo, --lpDeTexInfo->largeLod));
  else
    ++lpDeTexInfo->smallLod;
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: prPopLod
  Date: 7/7/97
  Implementor(s): psw
  Library: 
  Description:
    This routine actually modifies the GrTexInfo struct data to pop
    data off the top or bottom of the mipmap. pops off a single lod
  Arguments:
    GrTexInfo *lpDeTexInfo
    mipMapEnd_t deMipMapEnd
  Return: 
    none
  -------------------------------------------------------------------*/
static void 
prPopLod( GrTexInfo *lpDeTexInfo, mipMapEnd_t deMipMapEnd )
{
  if (deMipMapEnd == topOfMipMap)
    lpDeTexInfo->data = (void*)( (FxU32) lpDeTexInfo->data + 
                          prGetLodSize(lpDeTexInfo, lpDeTexInfo->largeLod++)); 
  else
    --lpDeTexInfo->smallLod;
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: prGetLodSize
  Date: 6/18/97
  Implementor(s): psw
  Library: 
  Description:
    returns the size, in number of bytes, of an LOD  based upon format, 
    LOD level and aspect ratio.  this routine is meant to be used on 
    the void* in the GrTexInfo struct.
  Arguments:
    GrTexInfo *const disTexInfo - pointer to the Texture Info
    GrLOD_t disLOD - the lod to get the size of
  Return: 
    FxU32
  -------------------------------------------------------------------*/
static FxU32 
prGetLodSize( GrTexInfo *const deTexInfo, GrLOD_t deLod )
{
  /* definitions */
  int    nPxlFctr;
  int    yAspctDvdr[] =  {3, 2, 1, 0, 1, 2, 3};  /* ie. GR_ASPECT_1x8 = 2^3 */
  int    yLodFctr[]   =  {8, 7, 6, 5, 4, 3, 2, 1, 0};  /* ie. GR_LOD_256 = 2^8 */
  FxU32  xNumBytes;
  GrAspectRatio_t  thisAR;
  GrLOD_t          thisLod;

  /* code */
  if ((deTexInfo->format == GR_TEXFMT_RGB_332)     ||
      (deTexInfo->format == GR_TEXFMT_YIQ_422)     ||
      (deTexInfo->format == GR_TEXFMT_ALPHA_8)     ||
      (deTexInfo->format == GR_TEXFMT_INTENSITY_8) ||
      (deTexInfo->format == GR_TEXFMT_P_8))
    nPxlFctr = 1;  /* 8-bit formats */
  else
    nPxlFctr = 2;  /* all else are 16-bit formats */
    
  thisAR = deTexInfo->aspectRatio;  /* thisAspectRatio */
  thisLod = deLod;                  /* thisLod */

  /* the general formula is xNumBytes=nBytesPerPixel*nPixels
  ** nPixels=maxPixels/aspectRatio
  ** maxPixels for GR_LOD_256=256*256 or 2^8*2^8, hence the following formula...*/
  if ( yAspctDvdr[thisAR] < yLodFctr[thisLod] )
    xNumBytes = nPxlFctr << yLodFctr[thisLod] << yLodFctr[thisLod] >> 
                  yAspctDvdr[thisAR];
  else
    xNumBytes = nPxlFctr << yLodFctr[thisLod];

  return xNumBytes;
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------
  Function: prTexMultiMemRequired
  Date: 6/27/97
  Implementor(s): psw
  Library: 
  Description:
    calculates and returns the amount of memory required for the 
    individual multibase texture segments in BOTH, EVEN or ODD modes
    traps cases for missing LODs
  Arguments:
    FxU32 deMipMask (GR_MIPMAPLEVELMASK)
    GrTexBaseRange_t deRange
    GrAspectRatio_t deRatio
    GrTextureFormat_t deFormat
  Return: 
    FxU32 - memory required
  -------------------------------------------------------------------*/
static 
FxU32 prTexMultiMemRequired( FxU32 deMipMask, GrTexBaseRange_t deMultiSeg,
                             GrTexInfo* deTexInfo )
{
  /* definitions */
  GrTexInfo   tmpTexInfo, 
              *lpTmpTexInfo;  /* create a copy of the GrTexInfo */
  FxU32       thisSegMemReq = 0x0;
  GrLOD_t     lod;

  /* initializations */
  lpTmpTexInfo = &tmpTexInfo; 
  *lpTmpTexInfo = *deTexInfo;  /* copy passed data, then prepare to munge */

  /* code */
  if ((lpTmpTexInfo->largeLod > (FxI32) deMultiSeg) ||
      (lpTmpTexInfo->smallLod < (FxI32) deMultiSeg))
    return 0; /* the deMultiSeg !exist in the lod range of info */

  /* else it does */
  for (lod = lpTmpTexInfo->largeLod; lod < (FxI32) deMultiSeg; ++lod)
    prPopLod( lpTmpTexInfo, topOfMipMap ); /* munge the top */

  if ( deMultiSeg != GR_TEXBASE_32_TO_1)
    lpTmpTexInfo->smallLod = lpTmpTexInfo->largeLod; /* munge the btm */

  thisSegMemReq = grTexTextureMemRequired( deMipMask, lpTmpTexInfo );

  return thisSegMemReq;
}  /* prTexMultiMemRequired */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
#if 0 /* NOT USED */
static int 
prGetLodWidth( GrLOD_t lod, GrAspectRatio_t aspect )
{
  int   yAspctWdvdr[] = {0, 0, 0, 0, 1, 2, 3};
  int   yLodFctr[] = {8, 7, 6, 5, 4, 3, 2, 1, 0};
  int   val;

  val = yLodFctr[lod] > yAspctWdvdr[aspect] ? 
          1 << yLodFctr[lod] >> yAspctWdvdr[aspect] : 1;

  return( val );
}
#endif
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
static int 
prGetLodHeight( GrLOD_t lod, GrAspectRatio_t aspect )
{
  int   yAspctHdvdr[] = {3, 2, 1, 0, 0, 0, 0};
  int   yLodFctr[] = {8, 7, 6, 5, 4, 3, 2, 1, 0};
  int   val;

  val = yLodFctr[lod] > yAspctHdvdr[aspect] ? 
          1 << yLodFctr[lod] >> yAspctHdvdr[aspect] : 1;
  
  return( val );
}
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
static FxU32
prGetMultiMemBump( const GrTexInfo *const deTexInfo, 
                     GrTexBaseRange_t deTexBase,
                     FxU32 deMipMask )
{ /* this fixes  some multibase fuckage */
  /* definitions */
  FxU32      xMemBump,
             xAllMemReq,
             xSegMemReq;
  GrTexInfo  tmpTexInfo,
             *lpTmpTexInfo;

  /* initializations */
  lpTmpTexInfo = &tmpTexInfo;
  *lpTmpTexInfo = *deTexInfo;

  /* code */
  lpTmpTexInfo->largeLod = GR_LOD_256;
  xAllMemReq = grTexTextureMemRequired( deMipMask, lpTmpTexInfo );

  lpTmpTexInfo->largeLod = deTexBase != GR_TEXBASE_32_TO_1 ? deTexBase :
                             lpTmpTexInfo->largeLod > (FxI32) deTexBase ? 
                             lpTmpTexInfo->largeLod : GR_LOD_32;
  xSegMemReq = grTexTextureMemRequired( deMipMask, lpTmpTexInfo );

  xMemBump = xAllMemReq - xSegMemReq;
  return (xMemBump);
}
