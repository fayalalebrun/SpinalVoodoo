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
** 3     3/17/99 6:14p Dow
** Fixes Phantom Menace, OGL, and others.
** 
** 10    3/02/99 4:20p Stuartb
** Removed obsolete TV_STANDARD_XXX defines.
** 
** 9     2/16/99 3:47p Stuartb
** Added some BIOS_XXXX  tvout & LCD defines.
** 
** 8     2/13/99 6:45p Stuartb
** First cut at PAL_M, PAL_N, PAL_NC support in tvout.
** 
** 7     99/01/26 11:51 Larryw
** Bringing the other tv.h's into sync with the changes made to
** $/devel/h3/Win95/dx/inc/tv.h (including QUERYENABLETV).
** 
** 2     1/14/99 3:50p Stuartb
** Added #define QUERYENABLETV.
** 
** 1     9/10/98 2:55p Stuartb
** Interface file for TVOUT.
** 
** 2     7/11/98 8:18a Andrew
** changed some comments
** 
** 1     5/12/98 9:34a Andrew
** First attempt at TV out interface
** 
*/

#ifndef _TV_H_
#define _TV_H_

#ifdef VDDONLY
#define FAR
#endif 

// TVOut Queries
#define QUERYTVAVAIL            (0x300)           // Returns Status Determined at Boot
#define QUERYTVSENSE            (0x301)           // Performs a Sense on the available connectors

// Individual Queries on Capabilities
#define QUERYGETPICCAP          (0x321)          // Get Individual Picture Control Capability
#define QUERYGETFILTERCAP       (0x322)          // Get Individual Filter Control Capability
#define QUERYGETPOSCAP          (0x323)          // Get Individual Position Control Capability
#define QUERYGETSIZECAP         (0x324)          // Get Individual Size Control Capability
#define QUERYGETSPECIALCAP      (0x325)          // Get Individual Special Control Capability

// Get Current Settings
#define QUERYGETSTANDARD        (0x340)          // Get Current Standard Setting
#define QUERYGETPICCONTROL      (0x341)          // Get Individual Picture Control Setting
#define QUERYGETFILTERCONTROL   (0x342)          // Get Individual Filter Control Setting  
#define QUERYGETPOSCONTROL      (0x343)          // Get Position Control Setting
#define QUERYGETSIZECONTROL     (0x344)          // Get Size Control Setting
#define QUERYGETSPECIAL         (0x345)          // Get Individual Special Control Setting
#define QUERYGETCONSTATUS       (0x346)          // Get Connector Status

// Set Settings
#define QUERYSETSTANDARD        (0x360)         // Set Current Standard Setting
#define QUERYSETPICCONTROL      (0x361)         // Set Individual Picture Control Setting
#define QUERYSETFILTERCONTROL   (0x362)         // Set Individual Filter Control Setting  
#define QUERYSETPOSCONTROL      (0x363)         // Set Individual Position Control Setting
#define QUERYSETSIZECONTROL     (0x364)         // Set Individual Size Control Setting
#define QUERYSSETSPECIAL        (0x365)         // Set Individual Special Control Setting
#define QUERYSETCONSTATUS       (0x366)         // Set Connector Status

// Registery Updates
#define QUERYCOMMITREG          (0x380)         // This should be called after the user hits ok
#define QUERYREFRESH            (0x381)         // This should be called if the user hits cancel

// Enable/Disable TV-Out
#define QUERYDISABLETV          (0x500)         // Disable TV-Out and switch CRT On
#define QUERYENABLETV           (0x501)         // This just enables mode selection

/*
**  Call this function first to know how if a TV Encoder is 
**  available and what general functionality it provides 
**
**  INPUT:      qin.dwSubFunc = QUERYTVAVAIL
**  OUTPUT: TVSTATUS
*/

typedef struct tvcon {
   DWORD dwType;                       // Type of the Connector <Composite, S-Video, SCART>
   DWORD dwStatus;                     // Connector Status <Tv Present/Enabled/Disabled>
} TVCON, FAR * LPTVCON, * PTVCON;

// To be used in the Type Field of TVCON
#define TV_TYPE_SVIDEO (0x00000001L)
#define TV_TYPE_COMPOSITE (0x00000002L)
#define TV_TYPE_SCART (0x00000004L)
#define TV_TYPE_UNKNOWN    (0x08L)        // for broken bt868 rev d

// To be used in the Status Field of TVCON
#define TV_PRESENT          (0x00000001L)       // Did sense determine TV on connector ?
#define TV_CONNECTOR_ENABLED (0x00000002L)       // Is Connector Currently Enabled ?

#define MAX_CONNECTORS (0x000000004L)

// This structure defines what we are capable of 
typedef struct qtvstatus {
   DWORD dwEncoder;                       // Did I2C Query find a TV Encoder
   DWORD dwStandard;                      // Bit Map of Standard's Supported
   DWORD dwPicControl;                    // Bit Map of Picture Controls
   DWORD dwFilterControl;                 // Bit Map of Filter Controls
   DWORD dwPosControl;                    // Bit Map of Position Controls
   DWORD dwSizeControl;                   // Bit Map of Size Controls
   DWORD dwSpecial;                       // Bit Map of Special Controls
   DWORD dwNumSimultaneous;               // How many outputs can you support at the same time?
   DWORD dwNumConnectors;                 // How many connectors does the Encoder support
   TVCON TVCon[MAX_CONNECTORS];           // TV Connector Status
   BYTE  szName[10];                      // Name of the Encoder <Ch7003, Ch7004, BT868, BT869>
   } QTVSTATUS, FAR * LPQTVSTATUS, * PTVSTATUS;

// To be used in TVStatus.dwEncoder
#define TV_ENCODER_PRESENT (0x00000001L)

// To be used in TVStatus.dwPicControl <Note: Flag Set implies capability present>
#define TV_BRIGHTNESS (0x00000001)
#define TV_CONTRAST   (0x00000002)
#define TV_GAMMA      (0x00000004)
#define TV_HUE        (0x00000008)
#define TV_SATURATION (0x00000010)
#define TV_SHARPNESS  (0x00000020)

// To be used in TVStatus.dwFilter <Note: Flag Set implies capability present>
#define TV_FLICKER (0x10000001)
#define TV_CHROMA  (0x10000002)
#define TV_LUMA    (0x10000004)

// To be used in TVStatus.dwPosControl <Note: Flag Set implies capability present>
#define TV_HORIZONTAL (0x00000001)
#define TV_VERTICAL   (0x00000002)

// To be used in TVStatus.dwSizeControl <Note: Flag Set implies capability present>
#define TV_UNDERSCAN          (0x00000001)
#define TV_OVERSCAN           (0x00000002)
#define TV_ADJUST_UNDERSCAN   (0x00000004)
#define TV_ADJUST_OVERSCAN    (0x00000008)

// To be used in TVStatus.dwSpecial <Note: Flag Set implies capability present>
#define TV_CLOSED_CAPTION           (0x00000001)
#define TV_MACROVISION              (0x00000002)
#define TV_LCDPANEL                 (0x00000004)      /* Flat panel out */

// To be used in TVSetSpec.dwCap for miscellaneous features
#define TV_SETSPECIAL_CCAPTION      (0x00000001)
#define TV_SETSPECIAL_MACROVISION   (0x00000002)
#define TV_SETSPECIAL_LCDPANEL      (0x00000004)      /* Flat panel out */
#define TV_SETSPECIAL_WR_LCDREG     (0x00000005)      /* For debug */
#define TV_SETSPECIAL_RD_LCDREG     (0x00000006)      /* For debug */

#define MAX_SPECIAL_DATA            512
/*
**  Call this function to request the Driver to perform
**  a sense on every connector
**
**  INPUT:      qin.dwSubFunc = QUERYTVSENSE
**  OUTPUT: TVCONSTATUS
*/

typedef struct tvconstatus {
   DWORD dwNumConnectors;                 // How many connectors does the Encoder support
   TVCON TVCon[MAX_CONNECTORS];           // TV Connector Status
   } TVCONSTATUS, FAR * LPTVCONSTATUS, * PTVCONSTATUS;

/****************************************************************************
**
** Get Capabilities
**
****************************************************************************/

typedef struct qind {
   DWORD dwSubFunc;                 // Sub Function
   DWORD dwCap;                     // Capability of Interest <one capability at a time>
} QIND, FAR * LPQIND, * PQIND;

/*
**  Call this function to request the Driver to
**  report individual Picture Capability
**
**  INPUT:      qind.dwSubFunc = QUERYGETPICCAP
**              qind.dwCap = TV_BRIGHTNESS |
**                           TV_CONTRAST |
**                           TV_GAMMA |
**                           TV_HUE |      
**                           TV_SATURATION |
**                           TV_SHARPNESS 
**  OUTPUT: TVCAPDATA      
*/

typedef struct tvcapdata {
   DWORD dwCap;                     // Capability of Interest <one capability at a time>
   DWORD dwNumSteps;                // Number of Steps Supported by Hardware
   } TVCAPDATA, FAR * LPTVCAPDATA, * PTVCAPDATA;                    

/*
**  Call this function to request the Driver to
**  report all Position Capability
**
**  INPUT:      qin.dwSubFunc = QUERYGETPOSITIONCAP
**  OUTPUT:       TVPOSCAP
**  Note this is a good match for Chrontel but unknown now to do this on Brooktree
*/
typedef struct tvposcap {
   DWORD  dwMaxLeft;               // Maximum Left Position in Hardware Units
   DWORD  dwMaxRight;              // Maximum Right Position in Hardware Units
   DWORD  dwHorGranularity;        // Size of Movement in Pixels
   DWORD  dwMaxTop;                // Maximum Top Position in Hardware Units
   DWORD  dwMaxBottom;             // Maximum Bottom Position in Hardware Units
   DWORD  dwVGAGranularity;        // Granularity in VGA Lines
   } TVPOSCAP, FAR * LPTVPOSCAP, * PTVPOSCAP;  

/*
**  Call this function to request the Driver to
**  report all Size Capability
**
**  INPUT:      qin.dwSubFunc = QUERYGETSIZECAP
**  OUTPUT:       TVSIZECAP
**  Note this is a good match for BrookTree but unknown now to do this on Chrontel
*/
typedef struct tvsizecap {
   DWORD  dwMaxHorInput;               // Maximum Input Horizontal Size in Active Pixels
   DWORD  dwMaxVerInput;               // Maximum Input Vertical Size in Active Lines
   DWORD  dwMaxHorOutput;              // Maximum Output Horizontal Size in Active Pixels
   DWORD  dwMaxVerOutput;              // Maximum Output Vertical Size in Active Pixels
   DWORD  dwMinHorInput;               // Minimum Input Horizontal Size in Active Pixels
   DWORD  dwMinVerInput;               // Minimum Input Vertical Size in Active Lines
   DWORD  dwMinHorOutput;              // Minimum Output Horizontal Size in Active Pixels
   DWORD  dwMinVerOutput;              // Minimum Output Vertical Size in Active Pixels
   DWORD  dwHorStepSize;               // In Percentage * 1000
   DWORD  dwVerStepSize;               // In Percentage * 1000
   } TVSIZECAP, FAR * LPTVSIZECAP, * PTVSIZECAP;  

/*
**  Call this function to request the Driver to
**  report on a Special Capability
**
**  INPUT:      qind.dwSubFunc = QUERYGETSPECIALCAP
**              qind.dwCap = TV_CLOSED_CAPTION |
**                           TV_MACROVISION    |
**                           TV_LCDPANEL           
**  OUTPUT:       TVSPECCAP <TBD>
*/

/****************************************************************************
**
** Get Current Settings
**
****************************************************************************/

/*
**  Call this function to request the Driver to
**  report current Standard Setting
**
**  INPUT:      qin.dwSubFunc = QUERYGETSTANDARD
**  OUTPUT: TVGETSTANDARD      
*/
typedef struct tvgetstandard {
   DWORD dwStandard;                   // Standard Currently in Use
} TVGETSTANDARD, FAR * LPTVGETSTANDARD, * PTVGETSTANDARD;

/*
**  Call this function to request the Driver to
**  report current individual Picture Capability
**
**  INPUT:      qind.dwSubFunc = QUERYGETPICCONTROL
**              qind.dwCap = TV_BRIGHTNESS |
**                           TV_CONTRAST |
**                           TV_GAMMA |
**                           TV_HUE |      
**                           TV_SATURATION |
**                           TV_SHARPNESS 
**  OUTPUT: TVCURCAP      
*/

typedef struct tvcurcap {
   DWORD dwCap;                 // Capability Requested
   DWORD dwStep;                // Current Hardware Step we are on
   } TVCURCAP, FAR * LPTVCURCAP, * PTVCURCAP;                    

/*
**  Call this function to request the Driver to
**  report current individual Filter Capability
**
**  INPUT:      qind.dwSubFunc = QUERYGETFILTERCONTROL
**              qind.dwCap = TV_FLICKER |
**                           TV_CHROMA |
**                           TV_LUMA
**  OUTPUT: TVCURCAP
*/

/*
**  Call this function to request the Driver to
**  report all current Position Capability
**
**  INPUT:      qin.dwSubFunc = QUERYGETPOSITIONCONTROL
**  OUTPUT:       TVCURPOS
**  Note this is a good match for Chrontel but unknown now to do this on Brooktree
*/
typedef struct tvcurpos {
   DWORD  dwCurLeft;               // Current Left Position in Hardware Units
   DWORD  dwCurRight;              // Current Right Position in Hardware Units
   DWORD  dwCurTop;                // Current Top Position in Hardware Units
   DWORD  dwCurBottom;             // Current Bottom Position in Hardware Units
   } TVCURPOS, FAR * LPTVCURPOS, * PTVCURPOS;  

/*
**  Call this function to request the Driver to
**  report current Size Capability
**
**  INPUT:      qin.dwSubFunc = QUERYGETSIZECONTROL
**  OUTPUT:       TVCURSIZE
**  Note this is a good match for BrookTree but unknown now to do this on Chrontel
*/
typedef struct tvcursize {
   DWORD  dwCurHorInput;               // Current Input Horizontal Size in Active Pixels
   DWORD  dwCurVerInput;               // Current Input Vertical Size in Active Lines
   DWORD  dwCurHorOutput;              // Current Output Horizontal Size in Active Pixels
   DWORD  dwCurVerOutput;              // Current Output Vertical Size in Active Pixels
   } TVCURSIZE, FAR * LPTVCURSIZE, * PTVCURSIZE;  

/*
**  Call this function to request the Driver to
**  report on current a Special Capability
**
**  INPUT:      qind.dwSubFunc = QUERYGETSPECIAL
**              qind.dwCap = TV_CLOSED_CAPTION |
**                           TV_MACROVISION
**  OUTPUT:       TVCURSPEC <TBD>
*/

/*
**  Call this function to request the Driver to return
**  in memory data on every connector
**
**  INPUT:      qin.dwSubFunc = QUERYGETCONSTATUS
**  OUTPUT: TVCONSTATUS
*/


/****************************************************************************
**
** Set Functions
**
****************************************************************************/

/*
**  Call this function to request the Driver to
**  change the current Standard Setting
**
**  INPUT:      TvSetStandard.dwSubFunc = QUERYSETSTANDARD
**              TvSetStandard.dwStandard = TV_STANDARD_NTSCRS170A |      
**                                         TV_STANDARD_NTSCM |           
**                                         TV_STANDARD_NTSCN |           
**                                         TV_STANDARD_NTSC443 |         
**                                         TV_STANDARD_NTSCJAPAN |        
**                                         TV_STANDARD_PALBDGHI |         
**                                         TV_STANDARD_PALM  |           
**                                         TV_STANDARD_PALN  |           
**                                         TV_STANDARD_PALN_ARGENTINA |  
**
**  OUTPUT: None
*/
typedef struct tvsetstandard {
   DWORD dwSubFunc;                    // Sub Function
   DWORD dwStandard;                   // Standard Currently in Use
} TVSETSTANDARD, FAR * LPTVSETSTANDARD, * PTVSETSTANDARD;

/*
**  Call this function to request the Driver to
**  set individual Picture Capability
**
**  INPUT:      TVSetCap.dwSubFunc = QUERYSETPICCONTROL
**              TVSetCap.dwCap = TV_BRIGHTNESS |
**                               TV_CONTRAST |
**                               TV_GAMMA |
**                               TV_HUE |      
**                               TV_SATURATION |
**                               TV_SHARPNESS 
**              TVSetCap.dwStep =
**  OUTPUT: None      
*/

typedef struct tvsetcap {
   DWORD dwSubFunc;             // SubFunction
   DWORD dwCap;                 // Capability Requested
   LONG  dwStep;                // Hardware Step to set
   } TVSETCAP, FAR * LPTVSETCAP, * PTVSETCAP;                    

/*
**  Call this function to request the Driver to
**  set individual Filter Capability
**
**  INPUT:      TVSetCap.dwSubFunc = QUERYSETFILTERCONTROL
**              TVSetCap.dwCap = TV_FLICKER |
**                               TV_CHROMA |
**                               TV_LUMA
**              TVSetCap.dwStep = 
**  OUTPUT: None
*/

/*
**  Call this function to request the Driver to
**  move the Output Image 
**
**  INPUT:      TVSetPos.dwSubFunc = QUERYSETPOSCONTROL
**              TVSetPos.dwLeft = 
**              TVSetPos.dwRight = 
**              TVSetPos.dwTop = 
**              TVSetPos.dwBottom = 
**  OUTPUT:       None
**  Note this is a good match for Chrontel but unknown now to do this on Brooktree
*/
typedef struct tvsetpos {
   DWORD  dwSubFunc;            // SubFunction
   DWORD  dwLeft;               // Left Position in Hardware Units
   DWORD  dwRight;              // Right Position in Hardware Units
   DWORD  dwTop;                // Top Position in Hardware Units
   DWORD  dwBottom;             // Bottom Position in Hardware Units
   } TVSETPOS, FAR * LPTVSETPOS, * PTVSETPOS;  

/*
**  Call this function to request the Driver to
**  set the resize the input/output 
**
**  INPUT:      TVSetSize.dwSubFunc = QUERYSETSIZECONTROL
**              TvSetSize.dwHorOutput =
**              TvSetSize.dwVerOutput =
**  OUTPUT:     None
**  Note this is a good match for BrookTree but unknown now to do this on Chrontel
*/
typedef struct tvsetsize {
   DWORD  dwSubFunc;                   // SubFunction
   DWORD  dwOverScan;                  // Overscan enable/disable
   DWORD  dwHorOutput;                 // Output Horizontal Size in Active Pixels
   DWORD  dwVerOutput;                 // Output Vertical Size in Active Lines
   } TVSETSIZE, FAR * LPTVSETSIZE, * PTVSETSIZE;  

/*
**  Call this function to request the Driver to
**  set the a Special Capability
**
**  INPUT:      TVSetSpec.dwSubFunc = QUERYSETSPECIAL
**              TVSetSpec.dwCap = TV_SETSPECIAL_CCAPTION |
**                                TV_SETSPECIAL_MACROVISION |
**                                TV_SETSPECIAL_LCDPANEL    
**  OUTPUT:     None
*/
typedef struct tvsetspecial {
   DWORD dwSubFunc;                    // Sub Function
   DWORD dwCap;                        // Feature to set
   DWORD dwIndex;                      // Index to data array: 0 - none
   WORD  wData[MAX_SPECIAL_DATA];      // Data array: format specific to caps
   DWORD dwIPclock;                    // Flat panel clock
   } TVSETSPECIAL, FAR * LPTVSETSPECIAL, * PTVSETSPECIAL;

/*
**  Call this function to request the Driver to enable/disable
**  connectors
**
**  INPUT:      TVSetConnector.dwSubFunc = QUERYSETCONSTATUS
**              Note i can range from 0 to < dwNumSimultaneous
**              TVSetConnector.TVCon[i].dwType = TV_TYPE_SVIDEO | TV_TYPE_COMPOSITE | TV_TYPE_SCART
**              TVSetConnector.TVCon[i].dwStatus = TV_CONNECTOR_ENABLED |  0x00
**
**  OUTPUT: None
*/

typedef struct tvsetconnector {
   DWORD dwSubFunc;                       // SubFunction
   TVCON TVCon[MAX_CONNECTORS];           // TV Connector Status
   } TVSETCONNECTOR, FAR * LPTVSETCONNECTOR, * PTVSETCONNECTOR;

/****************************************************************************
**
** Registry Functions
**
****************************************************************************/
/*
**  Call this function to request the Driver to save
**  in memory data to registry
**
**  INPUT:      qin.dwSubFunc = QUERYCOMMITREG
**  OUTPUT: None
*/

/*
**  Call this function to request the Driver to refresh
**  in memory data with registry data
**
**  INPUT:      qin.dwSubFunc = QUERYREFRESH
**  OUTPUT: None
*/



/****************************************************************************
**
** Error codes
**
****************************************************************************/
#define TV_STANDARD_UNSUPPORTED       (0x00000001L) 
#define TV_CONTROL_UNSUPPORTED        (0x00000002L)
#define TV_INVALID_DEVICE             (0x000000ffL)


/****************************************************************************
**
** TV standards that we support taken from win98 ddk tvout.h
**
****************************************************************************/

#define VP_TV_STANDARD_NTSC_M   0x0001  //        75 IRE Setup
#define VP_TV_STANDARD_NTSC_M_J 0x0002  // Japan,  0 IRE Setup
#define VP_TV_STANDARD_PAL_B    0x0004
#define VP_TV_STANDARD_PAL_D    0x0008
#define VP_TV_STANDARD_PAL_H    0x0010
#define VP_TV_STANDARD_PAL_I    0x0020
#define VP_TV_STANDARD_PAL_M    0x0040
#define VP_TV_STANDARD_PAL_N    0x0080
#define VP_TV_STANDARD_SECAM_B  0x0100
#define VP_TV_STANDARD_SECAM_D  0x0200
#define VP_TV_STANDARD_SECAM_G  0x0400
#define VP_TV_STANDARD_SECAM_H  0x0800
#define VP_TV_STANDARD_SECAM_K  0x1000
#define VP_TV_STANDARD_SECAM_K1 0x2000
#define VP_TV_STANDARD_SECAM_L  0x4000
#define VP_TV_STANDARD_WIN_VGA  0x8000
// and the rest
#define VP_TV_STANDARD_NTSC_433 0x00010000
#define VP_TV_STANDARD_PAL_G    0x00020000
#define VP_TV_STANDARD_PAL_60   0x00040000
#define VP_TV_STANDARD_SECAM_L1 0x00080000

#define VP_TV_STANDARD_PAL_NC   VP_TV_STANDARD_SECAM_L1


#define BIOS_NTSC   		0x01   // 	00001
#define BIOS_PAL    		0x02   //   (BGDHI)	00010
#define BIOS_PAL_N  		0x0a   // 	01010
#define BIOS_PAL_M  		0x12   // 	10010
#define BIOS_PAL_Nc			0x1a   //   11010

#define BIOS_TVSTD_MASK     (BIOS_NTSC | BIOS_PAL | BIOS_PAL_N | BIOS_PAL_M | BIOS_PAL_Nc)
#define BIOS_LCD_ACTIVE     0x04
#define BIOS_TVOUT_ACTIVE	0x20


#endif
