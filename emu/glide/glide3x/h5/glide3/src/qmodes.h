/*
**
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
** File name:   qmodes.h
**
** Description: Structures, macros, etc to support query mode.
**
** $History: qmodes.h $
** 
** *****************  Version 21  *****************
** User: Stb_rbissell Date: 5/14/99    Time: 9:47p
** Updated in $/devel/h3/win95/dx/dd16
** tvout and dfp changes
** 
** *****************  Version 20  *****************
** User: Stuartb      Date: 2/25/99    Time: 12:48p
** Updated in $/devel/h3/Win95/dx/dd16
** Added FPFLAG_CAPABLE.
** 
** *****************  Version 19  *****************
** User: Stuartb      Date: 2/18/99    Time: 2:54p
** Updated in $/devel/h3/Win95/dx/dd16
** Added xlcd FPFLAGS for filter mode setting.
** 
** *****************  Version 18  *****************
** User: Andrew       Date: 2/11/99    Time: 11:17a
** Updated in $/devel/h3/Win95/dx/dd16
** Added AGP Cap Query
** 
** *****************  Version 17  *****************
** User: Stuartb      Date: 1/21/99    Time: 11:18a
** Updated in $/devel/h3/Win95/dx/dd16
** Added FPFLAG_SETRES and FPFLAG_SETREFR for app code.  Removed
** broadcastMonitorChange calls.
** 
** *****************  Version 16  *****************
** User: Stuartb      Date: 1/12/99    Time: 2:27p
** Updated in $/devel/h3/Win95/dx/dd16
** Added QUERY_ANALOG_MONITOR & modified QUERY_LCDCTRL.
** 
** *****************  Version 15  *****************
** User: Stuartb      Date: 1/08/99    Time: 3:34p
** Updated in $/devel/h3/Win95/dx/dd16
** Added QUERY_LCDCTRL for control panel flat panel ops.
** 
** *****************  Version 14  *****************
** User: Bob          Date: 1/05/99    Time: 5:57p
** Updated in $/devel/h3/WinNT/src/Video/Displays/H3
** BIOS version reporting to Control Panel applet.
** 
** Shared qmodes.h with Win9x and the rest of the universe.
** 
** *****************  Version 13  *****************
** User: Bob          Date: 1/05/99    Time: 5:14p
** Updated in $/devel/h3/Win95/dx/dd16
** Altered nesting of header files so that tv.h is not inside qmodes.h.
** 
** This is a sharing issue with NT.
** 
** *****************  Version 12  *****************
** User: Larryw       Date: 1/05/99    Time: 4:36p
** Updated in $/devel/swtools/3DfxV2ps
** Changed QUERY[GS]ETDESKTOPGAMMA back to QUERY[GS]ETGAMMA.
** 
** *****************  Version 11  *****************
** User: Larryw       Date: 1/05/99    Time: 3:45p
** Updated in $/devel/swtools/3DfxV2ps
** Put MAX_BIOS_VERSION_STRING in; changed some Gamma query values
** 
** *****************  Version 10  *****************
** User: Andrew       Date: 1/05/99    Time: 10:50a
** Updated in $/devel/h3/Win95/dx/dd16
** Added new function to get the bios version string
** 
** *****************  Version 9  *****************
** User: Michael      Date: 12/29/98   Time: 2:37p
** Updated in $/devel/h3/Win95/dx/dd16
** Implement the 3Dfx/STB unified header.
** 
** 8     10/10/98 5:21p Hsohel
** 
** 7     10/10/98 4:52p Hsohel
** 
** 6     10/10/98 3:36p Hsohel
** 
** 5     10/09/98 3:54p Hsohel
** 
** 4     7/13/98 5:26p Andrew
** Changed to support a gamma table
** 
** 3     7/11/98 8:16a Andrew
** Added gamma correction
** 
** 2     5/12/98 9:35a Andrew
** Added some minor changes for valid modes
** 
** 1     4/22/98 2:47p Andrew
** Query Modes Protocol Information
**
** 
*/

#ifndef _QMODES_H_
#define _QMODES_H_

#define CARD_TABLESIZEMASK 0xFF80
#define CARD_TABLEMODSIZE 0x50

/* The QUERYMODES ESC Code */
#define QUERYESCMODE (0x8001)

#define TDFXACK   (0x3DF0)
#define TDFXERR   (0xFFFF)

// General Queries
#define QUERYVERSION (0x0000)

// Mode Queries
#define QUERYNUMMODES (0x0001)
#define QUERYMODES (0x0002)
#define QUERYDEVNODE (0x0003)

// Virtual Desktop Queries
#define QUERYMAXFREEMEM (0x100)
#define QUERYSETVIRTUALSIZE (0x101)
#define QUERYGETSTARTADDR (0x102)
#define QUERYSETSTARTADDR (0x103)

// Gamma Queries
#define QUERYGETGAMMA		(0x200)
#define QUERYSETGAMMA		(0x201)
#define QUERYGETGLIDEGAMMA	(0x202)
#define QUERYSETGLIDEGAMMA	(0x203)

// BIOS Version
#define QUERYGETBIOSVERSION	(0x280)

// AGP Query
#define QUERYGETAGPCAPS (0x2A0)

// TVOUT Queries
// see ..\inc\tv.h   uses 0x300 -> 0x38f approx.

// LCD CONTROL
#define QUERY_LCDCTRL       (0x3a0)

// ANALOG MONITOR ON/OFF override

#define QUERY_ANALOG_MONITOR (0x3b0)


/*
**  Protocol:
**
**  Call will be of the form 
**   ExtEscape(hdc, QUERYESCMODE, sizeof(QIN), &Qin, sizeof(Output), &Output);
**   Escape(hdc, QUERYESCMODE, sizeof(QIN), &Qin, &Output);
**  
*/

 
/*
**  Standard Input Structure 
**
*/
typedef struct qin {
   DWORD dwSubFunc;        // Subfunction
} QIN, FAR * LPQIN, * PQIN;

/*
**  
**
**  INPUT:      qin.dwSubFunc = QUERYVERSION
**  OUTPUT: Driver Major & Minor Version
*/
typedef struct qversion {
   DWORD dwMajor;
   DWORD dwMinor;
   } QVERSION, FAR * LPQVERSION, * PQVERSION;

#define QUERYMODE_MAJOR 0x00000000
#define QUERYMODE_MINOR 0x00009999

/*
**  Call this function first to know how many modes to allow
**  the following structure for 
**
**  INPUT:      qin.dwSubFunc = QUERYNUMMODES
**  OUTPUT: Number of modes
*/
typedef struct qnummode {
   DWORD dwNum;
   } QNUMMODE, FAR * LPQNUMMODE, * PQNUMODE;

/*
**  This is the mode information.  You will need to allocate
**  # modes * QMODE structures that the Driver will fill in.
**
**  INPUT:      qin.dwSubFunc = QUERYMODES
**  OUTPUT: #Modes * QMODE
*/
typedef struct qmode {
   DWORD dwX;
   DWORD dwY;
   DWORD dwBpp;
   DWORD dwRef;
   DWORD dwValid;       // This will be a field of flags
   } QMODE, FAR * LPQMODE, * PQMODE;

#define QUERY_MODE_VALID (0x000000001L)
#define QUERY_TV_MODE (0x000000002L)

/*
**  This is Devnode that the driver is using
**
**  INPUT:      qin.dwSubFunc = QUERYDEVNODE
**  OUTPUT: dwDevNode
*/
typedef struct qdevnode {
   DWORD dwDevNode;                       // Monitor Device Node
   DWORD dwValidDefGamma;                 // 1 ==> bGamma is valid; 0 ==> Invalid
   BYTE bGamma;                           // Monitor Default Gamma <To Convert to float (bGamma + 100)/100.00>
   } QDEVNODE, FAR * LPQDEVNODE, * PQDEVNODE;

#define QUERY_MONITOR_GAMMA_VALID (0x00000001L)

/*
**  This is maximum free memory available for Virtual Desktop Usage in
**  this mode
**
**  INPUT:      qin.dwSubFunc = QUERYMAXFREEMEM
**  OUTPUT: Maximum Free Memory
*/
typedef struct qmaxfree {
   DWORD dwSubFunc;           // Should be set to QUERYSETVIRTUALSIZE
   DWORD dwMaxFree;           // Free Memory Available for Virtual Desktop
   DWORD dwMaxX;              // Maximum Free X ==> min(dwMaxFree/Y, MaxHardwareX) <not simultaneous with MaxY>
   DWORD dwMaxY;              // Maximum Free Y ==> min(dwMaxFree/(X*BPP), MaxHardwareY) <not simultaneous with MaxX>
   } QMAXFREE, FAR * LPQMAXFREE, * PQMAXFREE;

/*
**  This is used to set the Virtual Desktop Size
**
**  INPUT:      qsetvsize.dwSubFunc = QUERYSETVIRTUALSIZE
**  OUTPUT: None <Returns error on Failure>
*/
typedef struct qsetvsize {
   DWORD dwSubFunc;           // Should be set to QUERYSETVIRTUALSIZE
   DWORD dwX;                 // X Size in Pixels  (X * BPP * Y <= MaxMemSize)
   DWORD dwY;                 // Y Size in Lines
   } QSETVSIZE, FAR * LPQSETVSIZE, * PQSETVSIZE;

/*
**  This is used to get the Virtual Desktop Start Address
**
**  INPUT:      qin.dwSubFunc = QUERYGETSTARTADDR
**  OUTPUT: QGetStartAddr
*/
typedef struct qgetstartaddr {
   DWORD dwX;                 // X Location in Pixels  
   DWORD dwY;                 // Y Location in Lines
   } QGETSTARTADDR, FAR * LPQGETSTARTADDR, * PQGETSTARTADDR;

/*
**  This is used to move the Virtual Desktop Start Address
**
**  INPUT:      qsetstartaddr.dwSubFunc = QUERYSETSTARTADDR
**  OUTPUT: None <Returns Error on Failure>
*/
typedef struct qsetstartaddr {
   DWORD dwSubFunc;           // Should be set to QUERYSETSTARTADDR
   DWORD dwX;                 // X Location in Pixels  (Note: Start Addr = X * BPP + Y * Display Pitch)
   DWORD dwY;                 // Y Location in Lines
   } QSETSTARTADDR, FAR * LPQSETSTARTADDR, * PQSETSTARTADDR;

/*
**  This is used to get the gamma values currently in use
**
**  INPUT:      qin.dwSubFunc = QUERYGETGAMMA
**  OUTPUT: QGETGAMMA
*/
typedef struct qgetgamma {
   DWORD dwRed;                 // Red Gamma * 100
   DWORD dwGreen;               // Green Gamma * 100
   DWORD dwBlue;                // Blue Gamma * 100
   DWORD GammaTable[256];       // Gamma Table Defined as 0x00BBGGRR
   } QGETGAMMA, FAR * LPQGETGAMMA, * PQGETGAMMA;

/*
**  This is used to set the Gamma Value
**
**  INPUT:      qSetGamma.dwSubFunc = QUERYSETSTARTADDR
**  OUTPUT: None
*/
typedef struct qsetgamma {
   DWORD dwSubFunc;             // Should be set to QUERYSETGAMMA
   DWORD dwRed;                 // Red Gamma * 100
   DWORD dwGreen;               // Green Gamma * 100
   DWORD dwBlue;                // Blue Gamma * 100
   DWORD GammaTable[256];       // Gamma Table Defined as 0x00BBGGRR
   } QSETGAMMA, FAR * LPQSETGAMMA, * PQSETGAMMA;

#define MAX_BIOS_VERSION_STRING (32)

/*
**  This is used to get the BIOS version 
**
**  INPUT:      qin.dwSubFunc = QUERYGETBIOSVERSION
**  OUTPUT: QGETBIOSVERSION
*/
typedef struct qgetbiosversion {
   BYTE bBIOSVersion[MAX_BIOS_VERSION_STRING];
   } QGETBIOSVERSION, FAR * LPQGETBIOSVERSION, * PQGETBIOSVERSION;


/*
**  This is used to get the AGP Caps
**
**  INPUT:      qin.dwSubFunc = QUERYGETAGPCAPS
**  OUTPUT: QGETAGPCAPS
*/
// These are also defined in h3g.h
#define IS_AGP_CARD       (0x00000001L)        //Card has AGP capabilities 
#define IS_GART_AVAILABLE (0x00000002L)        //VMM Gart Functions are present
#define IS_AGP_READY      (IS_AGP_CARD|IS_GART_AVAILABLE) //We can do AGP Functions on this card 

typedef struct qgetagpcaps {
   DWORD dwAGPCaps;
   } QGETAGPCAPS, FAR * LPQGETAGPCAPS, * PQGETAGPCAPS;

/* HWCEXT_LCDCTRL
*
* Enables or disables flat panel monitor and also returns current status as
* given in IN-OUT struct below:
*
*/

// CAUTION: this is mirrored in h3g.h!!!

typedef struct {
	DWORD dwSubFunc;    // Should be set to QGETSET_LCDCTRL
	DWORD maxWidth;     // out
	DWORD maxHeight;    // out
	DWORD refreshRate;  // out
	DWORD fpFlags;      // in - out
}  QGETSET_LCDCTRL;

#define FPFLAG_ENABLED            BIT(0)   // is currently enabled (OUT)
#define FPFLAG_PRESENT            BIT(1)   // panel & controller connected (OUT)
#define FPFLAG_PRESENT_AT_BOOT    BIT(2)   // panel & controller connected
                                           // at boot time (OUT)
#define FPFLAG_ENABLE             BIT(3)   // try to enable panel (IN)
#define FPFLAG_DISABLE            BIT(4)   // try to disable panel (IN)
#define FPFLAG_GET_DIMS           BIT(5)   // read the EDID to get panel's
                                           // physical dimensions (IN)
#define FPFLAG_SETRES             BIT(6)   // current desktop resolution too hi
                                           // (OUT)
#define FPFLAG_SETREFR            BIT(7)   // current refresh rate wrong  (OUT)

// filter bits
// 00   bilinear    01   gausian
// 10   crisp       11   extra crisp
#define FPFLAG_FILTER_SOFT        BIT(8)   // use these 2 bits together (IN)
#define FPFLAG_FILTER_HARD        BIT(9)
#define FPFLAG_CAPABLE            BIT(10)  // board has controller & BIOS


typedef struct {
	DWORD dwSubFunc;    // Should be set to QUERY_ANALOG_MONITOR
	DWORD monitorStatus;
	DWORD monitorControl;
}  QGETSET_MONITOR_CTL;

#define ENABLE_MONITOR         BIT(0)
#define DISABLE_MONITOR        BIT(1)
#define MONITOR_IS_ENABLED     BIT(2)


#ifdef WINNT
int QueryMode(PDEV *, LPQIN lpQIN, LPVOID lpOutput);
#else
int QueryMode(LPQIN lpQIN, LPVOID lpOutput);
#endif

#endif
