;;
;; THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
;; PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
;; TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
;; INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
;; DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
;; THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
;; EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
;; FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
;; 
;; USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
;; RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
;; TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
;; AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
;; SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
;; THE UNITED STATES.  
;; 
;; COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
;; 
;; 3     3/17/99 6:16p Dow
;; Phantom Menace and other fixes.
;; 
;; 10    1/17/98 2:23p Dow
;; Changed headers a bit
;**
;** IMPLIB.ASM:  This is a template for an import library.
;**

.386p
.MODEL FLAT

.CODE

EXTRN   __loadme:NEAR                   ; magic symbol names
PUBLIC  __dllname, __dllfirst, __dlltab, __dllcount

ENTRYNAMES      SEGMENT DWORD USE32 PUBLIC 'CODE'
ENTRYNAMES      ENDS

ENTRYTHUNKS     SEGMENT DWORD USE32 PUBLIC 'CODE'
                ALIGN   4
__dllfirst:
ENTRYTHUNKS     ENDS

ENTRYTABLE      SEGMENT DWORD USE32 PUBLIC 'CODE'
                ALIGN   4
__dlltab        LABEL   DWORD
ENTRYTABLE      ENDS

CGROUP          GROUP _TEXT, ENTRYNAMES, ENTRYTHUNKS, ENTRYTABLE

;** This macro enforces consistency among the three tables which are
;** used to implement the __loadme mechanism.  The ENTRYNAMES segment
;** contains the names of the exported functions.  The ENTRYTHUNKS
;** segment contains a label and a five-byte call to __loadme, nothing
;** else.  (Implementation note: to extend this code to support multiple
;** import libraries, the mechanism could be changed to pass the names
;** of the DLL and the symbol to __loadme; this module wouldn't have 
;** to export any public symbols except the entry point stubs.  However,
;** doing it this way is much smaller when there are many entry points,
;** since each one is only five bytes.)  Finally, the ENTRYTABLE
;** segment contains an indexed array of pointers to the function
;** names; this table is kept in the same order as the ENTRYTHUNKS
;** segment.

EntryPoint      MACRO   name
ENTRYNAMES      SEGMENT
_$&name         DB      '&name',0
ENTRYNAMES      ENDS
ENTRYTHUNKS     SEGMENT
                PUBLIC  &name
&name:          call    __loadme
ENTRYTHUNKS     ENDS
ENTRYTABLE      SEGMENT
                DD      _$&name
ENTRYTABLE      ENDS
                ENDM

;** Declare the DLL name here, without an extension.  (The DLL loader
;** looks for extensions of .EXE, .DLL, or .OVL.)  Follow the name with
;** any function names that the DLL exports (if you will be looking the
;** functions up by name).  The names should match those of the 
;** entry-point labels defined below, including underscores and @<argbytes>
;** __stdcall decorations, to prevent accidental calling convention
;** mismatches.
;**
;** Note that dllentry0 and dllentry1 are prototyped as __cdecl functions.


__dllname       DB      'glide2x',0

    EntryPoint _GRAADRAWLINE@8
    EntryPoint _GRAADRAWPOINT@4
    EntryPoint _GRAADRAWPOLYGON@12
    EntryPoint _GRAADRAWPOLYGONVERTEXLIST@8
    EntryPoint _GRAADRAWTRIANGLE@24
    EntryPoint _GRALPHABLENDFUNCTION@16
    EntryPoint _GRALPHACOMBINE@20
    EntryPoint _GRALPHACONTROLSITRGBLIGHTING@4
    EntryPoint _GRALPHATESTFUNCTION@4
    EntryPoint _GRALPHATESTREFERENCEVALUE@4
    EntryPoint _GRBUFFERCLEAR@12
    EntryPoint _GRBUFFERNUMPENDING@0
    EntryPoint _GRBUFFERSWAP@4
    EntryPoint _GRCHECKFORROOM@4
    EntryPoint _GRCHROMAKEYMODE@4
    EntryPoint _GRCHROMAKEYVALUE@4
    EntryPoint _GRCLIPWINDOW@16
    EntryPoint _GRCOLORCOMBINE@20
    EntryPoint _GRCOLORMASK@8
    EntryPoint _GRCONSTANTCOLORVALUE4@16
    EntryPoint _GRCONSTANTCOLORVALUE@4
    EntryPoint _GRCULLMODE@4
    EntryPoint _GRDEPTHBIASLEVEL@4
    EntryPoint _GRDEPTHBUFFERFUNCTION@4
    EntryPoint _GRDEPTHBUFFERMODE@4
    EntryPoint _GRDEPTHMASK@4
    EntryPoint _GRDISABLEALLEFFECTS@0
    EntryPoint _GRDITHERMODE@4
    EntryPoint _GRDRAWLINE@8
    EntryPoint _GRDRAWPLANARPOLYGON@12
    EntryPoint _GRDRAWPLANARPOLYGONVERTEXLIST@8
    EntryPoint _GRDRAWPOINT@4
    EntryPoint _GRDRAWPOLYGON@12
    EntryPoint _GRDRAWPOLYGONVERTEXLIST@8
    EntryPoint _GRDRAWTRIANGLE@12
    EntryPoint _GRERRORSETCALLBACK@4
    EntryPoint _GRFOGCOLORVALUE@4
    EntryPoint _GRFOGMODE@4
    EntryPoint _GRFOGTABLE@4
    EntryPoint _GRGAMMACORRECTIONVALUE@4
    EntryPoint _GRGLIDEGETSTATE@4
    EntryPoint _GRGLIDEGETVERSION@4
    EntryPoint _GRGLIDEINIT@0
    EntryPoint _GRGLIDESETSTATE@4
    EntryPoint _GRGLIDESHAMELESSPLUG@4
    EntryPoint _GRGLIDESHUTDOWN@0
    EntryPoint _GRHINTS@8
    EntryPoint _GRLFBCONSTANTALPHA@4
    EntryPoint _GRLFBCONSTANTDEPTH@4
    EntryPoint _GRLFBLOCK@24
    EntryPoint _GRLFBREADREGION@28
    EntryPoint _GRLFBWRITEREGION@32
    EntryPoint _GRLFBUNLOCK@8
    EntryPoint _GRLFBWRITECOLORFORMAT@4
    EntryPoint _GRLFBWRITECOLORSWIZZLE@8
    EntryPoint _GRRENDERBUFFER@4
    EntryPoint _GRRESETTRISTATS@0
    EntryPoint _GRSPLASH@20
    EntryPoint _GRSSTCONFIGPIPELINE@12
    EntryPoint _GRSSTCONTROL@4
    EntryPoint _GRSSTIDLE@0
    EntryPoint _GRSSTISBUSY@0
    EntryPoint _GRSSTORIGIN@4
    EntryPoint _GRSSTPERFSTATS@4
    EntryPoint _GRSSTQUERYBOARDS@4
    EntryPoint _GRSSTQUERYHARDWARE@4
    EntryPoint _GRSSTRESETPERFSTATS@0
    EntryPoint _GRSSTSCREENHEIGHT@0
    EntryPoint _GRSSTSCREENWIDTH@0
    EntryPoint _GRSSTSELECT@4
    EntryPoint _GRSSTSTATUS@0
    EntryPoint _GRSSTVIDEOLINE@0
    EntryPoint _GRSSTVIDMODE@8
    EntryPoint _GRSSTVRETRACEON@0
    EntryPoint _GRSSTWINCLOSE@0
    EntryPoint _GRSSTWINOPEN@28
    EntryPoint _GRTEXCALCMEMREQUIRED@16
    EntryPoint _GRTEXCLAMPMODE@12
    EntryPoint _GRTEXCOMBINE@28
    EntryPoint _GRTEXCOMBINEFUNCTION@8
    EntryPoint _GRTEXDETAILCONTROL@16
    EntryPoint _GRTEXDOWNLOADMIPMAP@16
    EntryPoint _GRTEXDOWNLOADMIPMAPLEVEL@32
    EntryPoint _GRTEXDOWNLOADMIPMAPLEVELPARTIAL@40
    EntryPoint _GRTEXDOWNLOADTABLE@12
    EntryPoint _GRTEXDOWNLOADTABLEPARTIAL@20
    EntryPoint _GRTEXFILTERMODE@12
    EntryPoint _GRTEXLODBIASVALUE@8
    EntryPoint _GRTEXMAXADDRESS@4
    EntryPoint _GRTEXMINADDRESS@4
    EntryPoint _GRTEXMIPMAPMODE@12
    EntryPoint _GRTEXMULTIBASE@8
    EntryPoint _GRTEXMULTIBASEADDRESS@20
    EntryPoint _GRTEXNCCTABLE@8
    EntryPoint _GRTEXSOURCE@16
    EntryPoint _GRTEXTEXTUREMEMREQUIRED@8
    EntryPoint _GRTRISTATS@8
    EntryPoint _GU3DFGETINFO@8
    EntryPoint _GU3DFLOAD@8
    EntryPoint _GUAADRAWTRIANGLEWITHCLIP@12
    EntryPoint _GUALPHASOURCE@4
    EntryPoint _GUCOLORCOMBINEFUNCTION@4
    EntryPoint _GUDRAWPOLYGONVERTEXLISTWITHCLIP@8
    EntryPoint _GUDRAWTRIANGLEWITHCLIP@12
    EntryPoint _GUENCODERLE16@16
    EntryPoint _GUENDIANSWAPBYTES@4
    EntryPoint _GUENDIANSWAPWORDS@4
    EntryPoint _GUFOGGENERATEEXP2@8
    EntryPoint _GUFOGGENERATEEXP@8
    EntryPoint _GUFOGGENERATELINEAR@12
    EntryPoint _GUFOGTABLEINDEXTOW@4
    EntryPoint _GUMOVIESETNAME@4
    EntryPoint _GUMOVIESTART@0
    EntryPoint _GUMOVIESTOP@0
    EntryPoint _GUMPDRAWTRIANGLE@12
    EntryPoint _GUMPINIT@0
    EntryPoint _GUMPTEXCOMBINEFUNCTION@4
    EntryPoint _GUMPTEXSOURCE@8
    EntryPoint _GUTEXALLOCATEMEMORY@60
    EntryPoint _GUTEXCHANGEATTRIBUTES@48
    EntryPoint _GUTEXCOMBINEFUNCTION@8
    EntryPoint _GUTEXCREATECOLORMIPMAP@0
    EntryPoint _GUTEXDOWNLOADMIPMAP@12
    EntryPoint _GUTEXDOWNLOADMIPMAPLEVEL@12
    EntryPoint _GUTEXGETCURRENTMIPMAP@4
    EntryPoint _GUTEXGETMIPMAPINFO@4
    EntryPoint _GUTEXMEMQUERYAVAIL@4
    EntryPoint _GUTEXMEMRESET@0
    EntryPoint _GUTEXSOURCE@4
    EntryPoint _CONVERTANDDOWNLOADRLE@64

ENTRYTABLE      SEGMENT
__dlltabend     LABEL   DWORD
__dllcount      EQU     ((__dlltabend - __dlltab) / 4)
ENTRYTABLE      ENDS

END
