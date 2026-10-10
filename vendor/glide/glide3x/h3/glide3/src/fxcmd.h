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
**
** Revision 1.1.1.1.6.4  2006/01/16 21:22:42  jwrdegoede
** Fix gcc 4.1 new type-punned ptr breaks antialias. warnings
**
** Revision 1.1.1.1.6.3  2005/06/09 18:32:28  jwrdegoede
** Fixed all warnings with gcc4 -Wall -W -Wno-unused-parameter, except for a couple I believe to be a gcc bug. This has been reported to gcc.
**
** Revision 1.1.1.1.6.2  2005/05/25 08:56:23  jwrdegoede
** Make h5 and h3 tree 64 bit clean. This is ported over from the non-devel branch so this might be incomplete
**
** Revision 1.1.1.1.6.1  2003/06/29 18:43:27  guillemj
** Fix compilation warnings.
**
** Revision 1.1.1.1  1999/11/24 21:44:55  joseph
** Initial checkin for SourceForge
**
** 
** 6     4/16/99 4:27p Kcd
** SET*_FIFO macros.
** 
** 34    4/16/99 2:47p Kcd
** Magic PowerPC Bump & Grind Stuff.
** 
** 33    4/05/99 8:25p Dow
** Alt tab mostly happy
** 
** 32    4/04/99 4:56p Atai
** invert the contextP checking in GR_SET_EXPECTED_SIZE
** 
** 31    4/01/99 7:55p Peter
** made names and comments more meaningful
** 
** 30    3/31/99 9:02p Dow
** context loosing means no writing to hw
** 
** 29    3/30/99 3:28p Atai
** added LINEAR_WRITE_SET_* for direct write
** 
** 28    3/24/99 6:17p Peter
** streamlined (made more dangerouse) state validation
** 
** 27    3/19/99 11:26a Peter
** expose direct fifo for gl
** 
** 26    3/17/99 6:13p Peter
** fixed debug error w/ status bottleneck
** 
** 25    3/17/99 5:08p Peter
** removed whacky stuff now that the command fifo threshold stuff appears
** to make all happy (including the k7)
** 
** 24    3/14/99 1:47p Peter
** cmd's bng optimization, really invoke gggoph
** 
** 23    3/12/99 5:43p Peter
** debug build happiness
** 
** 22    3/12/99 2:27p Dow
** great-great-great grandson of packer workaround
** 
** 21    3/10/99 10:42a Peter
** bump-n-grind workaround for katmai until the bug is better
** characterized
** 
** 20    3/02/99 2:03p Peter
** removed no_check variant that led me astray
** 
** 19    2/18/99 5:54p Peter
** removed no_tsu crapola
** 
** 18    2/11/99 1:38p Atai
** sync buffer swap pending code, the right way.
** 
** 17    2/02/99 4:39p Peter
** cleaned up wax commands, added assertion for alignmnet boundary
** 
** 16    1/28/99 3:04p Atai
** fixed clip coord fog extension (c and asm version)
** 
** 15    1/25/99 6:33p Peter
** removed some cruft I saw when cleaning up tiled textures
** 
** 14    1/06/99 11:30a Peter
** cleanup trinalge dispatch code
** 
** 13    11/30/98 6:57p Peter
** video memory fifo's
** 
** 12    11/19/98 11:57a Atai
** fixed clip coords textured line
** 
** 11    10/20/98 8:28p Peter
** shared files and fixed whackage
** 
** 10    10/12/98 9:51a Peter
** dynamic 3DNow!(tm)
** 
** 9     8/26/98 9:06p Jdt
** Removed all references to lfblockcount
** 
** 8     8/18/98 4:30p Atai
** fixed aa triangle in clip coords
** 
** 7     8/03/98 6:38a Jdt
** moved curvertexsize, stats, part of pool into gc
** 
** 6     7/19/98 12:06p Jdt
** 
** 5     7/18/98 1:45p Jdt
** Removed TACO_MEMORY_FIFO_HACK
** 
** 4     7/18/98 12:50a Jdt
** fix retail build of glide3 with command fifo
** 
** 3     7/18/98 12:25a Jdt
** Changes to reflect new shadow register structure
** 
** 2     7/17/98 10:44a Atai
** fixed grTexNCCTable and clip coords st with aspect ratio
** 
** 1     7/16/98 8:13a Jdt
** Separated out cmd fifo macros from fxglide.h
**
*/

#ifndef __FX_CMD_H__
#define __FX_CMD_H__

/*--------------------------------------------------------
  Command Transport Macros and Functions
  --------------------------------------------------------*/
#if USE_PACKET_FIFO
/* fifo.c */
extern void
_grSet32(volatile FxU32* const sstAddr, const FxU32 val);

extern FxU32
_grGet32(volatile FxU32* const sstAddr);
#endif /* USE_PACKET_FIFO */

#if !USE_PACKET_FIFO
/* NOTE: fifoFree is the number of entries, each is 8 bytes */
#define GR_CHECK_FOR_ROOM(n,p) \
do { \
  FxI32 fifoFree = gc->state.fifoFree - (n); \
  if (fifoFree < 0)          \
    fifoFree = _grSpinFifo(n); \
  gc->state.fifoFree = fifoFree;\
} while(0)
#elif USE_PACKET_FIFO
/* Stuff to manage the command fifo on cvg
 *
 * NB: All of the addresses are in 'virtual' address space, and the
 * sizes are in bytes.
 */

/* The Voodoo^2 fifo is 4 byte aligned */
#define FIFO_ALIGN_MASK      0x03

/* We need some slop in the fifo for writing some bookkeeping data
 * since we don't let the fifo autowrap. Its actually a little bit
 * bigger just in case someone does not read this comment.
 * 
 * Fullscreen:
 *   1 jmp (1 32-bit word)
 * Windowed:
 *   pci: ret (1 32-bit word)
 *   agp: jmp (2 32-bit words) 
 */
#define FIFO_END_ADJUST  (sizeof(FxU32) << 3)

/* NB: This should be used sparingly because it does a 'real' hw read
 * which is *SLOW*.
 */
unsigned long _grHwFifoPtr(FxBool);
#define HW_FIFO_PTR(a) _grHwFifoPtr(a)

#if FIFO_ASSERT_FULL
#define FIFO_ASSERT()  { \
    extern void _fifoAssertFull( void ); \
    _fifoAssertFull(); \
}
#else /* !FIFO_ASSERT_FULL */
#define FIFO_ASSERT() \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoRoom < gc->cmdTransportInfo.fifoSize); \
ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoPtr < (FxU32)gc->cmdTransportInfo.fifoEnd)
#endif /* !FIFO_ASSERT_FULL */

#if (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#define GR_WINFIFO_BEGIN() beginCriticalSection()
#define GR_WINFIFO_END()   endCriticalSection()
#else /* !(GLIDE_PLATFORM & GLIDE_OS_WIN32) */
#define GR_WINFIFO_BEGIN()
#define GR_WINFIFO_END()
#endif /* !(GLIDE_PLATFORM & GLIDE_OS_WIN32) */

extern struct cmdTransportInfo* FX_CALL _grCommandTransportInfo(void);
extern void FX_CALL _grCommandTransportMakeRoom(const FxI32 blockSize, const char* fName, const int fLine);
extern void _FifoFlush( void );

#if __POWERPC__
#define FIFO_CACHE_FLUSH(d)  __dcbf(d,-4)
#else
#define FIFO_CACHE_FLUSH(d)
#endif

#ifndef GLIDE_DEBUG
#define GR_BUMP_N_GRIND \
do { \
  FIFO_CACHE_FLUSH(gc->cmdTransportInfo.fifoPtr);\
  P6FENCE; \
  GR_CAGP_SET(bump, gc->cmdTransportInfo.fifoPtr - gc->cmdTransportInfo.lastBump); \
  gc->cmdTransportInfo.lastBump = gc->cmdTransportInfo.fifoPtr; \
  gc->cmdTransportInfo.bumpPos = gc->cmdTransportInfo.fifoPtr + gc->cmdTransportInfo.bumpSize; \
  if (gc->cmdTransportInfo.bumpPos > gc->cmdTransportInfo.fifoEnd) \
    gc->cmdTransportInfo.bumpPos = gc->cmdTransportInfo.fifoEnd; \
} while(0)
#else
/* fifo.c */
extern void _grBumpNGrind(void);
#define GR_BUMP_N_GRIND _grBumpNGrind()
#endif

#define GR_CHECK_FOR_ROOM(__n, __p) \
do { \
  const FxU32 writeSize = (__n) + ((__p) * sizeof(FxU32));            /* Adjust for size of hdrs */ \
  ASSERT(((FxU32)(gc->cmdTransportInfo.fifoPtr) & FIFO_ALIGN_MASK) == 0); /* alignment */ \
  ASSERT(writeSize < gc->cmdTransportInfo.fifoSize - sizeof(FxU32)); \
  FIFO_ASSERT(); \
  if (gc->cmdTransportInfo.fifoRoom < (FxI32)writeSize) { \
     GDBG_INFO(280, "Fifo Addr Check: (0x%X : 0x%X)\n", \
               gc->cmdTransportInfo.fifoRoom, writeSize); \
     _grCommandTransportMakeRoom(writeSize, __FILE__, __LINE__); \
  } \
  ASSERT((FxU32)gc->cmdTransportInfo.fifoRoom >= writeSize); \
  FIFO_ASSERT(); \
} while(0)
#else
#error "GR_CHECK_FOR_ROOM not defined"
#endif

#if GLIDE_SANITY_SIZE
#if USE_PACKET_FIFO
#define GR_CHECK_FIFO_PTR() \
if (gc->cmdTransportInfo.autoBump) {\
  if((FxU32)gc->cmdTransportInfo.fifoPtr != gc->checkPtr + gc->checkCounter)\
    GDBG_ERROR("GR_ASSERT_FIFO", "(%s : %d) : " \
               "fifoPtr should be 0x%X (0x%X : 0x%X) but is 0x%X\n", \
               __FILE__, __LINE__, gc->checkPtr + gc->checkCounter, \
               gc->checkPtr, gc->checkCounter, gc->cmdTransportInfo.fifoPtr);\
  ASSERT_FAULT_IMMED((FxU32)gc->cmdTransportInfo.fifoPtr == gc->checkPtr + gc->checkCounter);\
}

#define GR_SET_FIFO_PTR(__n, __p) \
  gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr; \
  gc->checkCounter = ((__n) + ((__p) << 2))
#else
#define GR_CHECK_FIFO_PTR() 
#define GR_SET_FIFO_PTR(__n, __p)
#endif

#  define GR_CHECK_SIZE() \
                if(gc->counter != gc->expected_counter) \
                  GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be %d but is %d\n", \
                              gc->expected_counter,gc->counter); \
                GR_CHECK_FIFO_PTR(); \
                gc->checkPtr = (FxU32)gc->cmdTransportInfo.fifoPtr; \
                gc->checkCounter = 0; \
                ASSERT(gc->counter == gc->expected_counter); \
                gc->counter = gc->expected_counter = 0

#  define GR_CHECK_SIZE_DIRECT() \
                if(gc->counter != gc->expected_counter) \
                  GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be %d but is %d\n", \
                              gc->expected_counter,gc->counter); \
                gc->checkCounter = 0; \
                ASSERT(gc->counter == gc->expected_counter); \
                gc->counter = gc->expected_counter = 0

#  define GR_SET_EXPECTED_SIZE(n,p) \
                ASSERT(gc->counter == 0); \
                ASSERT(gc->expected_counter == 0); \
                GR_CHECK_FOR_ROOM(n,p); \
                if (gc->contextP) { \
                  gc->expected_counter = n; \
                  GR_SET_FIFO_PTR(n, p); \
                }

#  define GR_INC_SIZE(n) gc->counter += n
#else
  /* define to do nothing */
#  define GR_CHECK_SIZE()
#  define GR_CHECK_SIZE_DIRECT()
#  define GR_SET_EXPECTED_SIZE(n,p) GR_CHECK_FOR_ROOM(n,p)
#  define GR_INC_SIZE(n)
#  define GR_SET_FIFO_PTR(__n, __p)
#endif



#if USE_PACKET_FIFO
#if GLIDE_DEBUG
void _grFifoWriteDebug(FxU32 addr, FxU32 val, FxU32 fifoPtr);
#define DEBUGFIFOWRITE(a,b,c) \
_grFifoWriteDebug((FxU32) a, (FxU32) b, (FxU32) c)
void _grFifoFWriteDebug(FxU32 addr, float val, FxU32 fifoPtr);
#define DEBUGFIFOFWRITE(a,b,c) \
_grFifoFWriteDebug((FxU32) a, (float) b, (FxU32) c)

extern void _reg_group_begin_internal_wax( FxU32 regBase,
                                           FxU32 groupNum,             
                                           FxU32 groupMask,            
                                           FxU32 pktHdr,               
                                           FxU32 checkP,               
                                           volatile FxU32 *regGroupFifoPtr ); 
extern void _reg_group_begin_internal( FxU32 chipId, FxU32 regBase,     
                                       FxU32 groupNum, FxU32 groupMask, 
                                       FxU32 pktHdr, FxU32 checkP,      
                                       volatile FxU32 *regGroupFifoPtr);
#define DBG_CALL( X )  X
#else /* ~GDBG_INFO_ON */
#define DEBUGFIFOWRITE(a,b,c)
#define DEBUGFIFOFWRITE(a,b,c)
#define DBG_CALL( X )
#endif /* !GDBG_INFO_ON */
#endif /* USE_PACKET_FIFO */

/* HW Setting macros. We redefine the default macros to:
 *  - add extra tracing
 *  - work around hw bugs
 *  - do platform specific whacky things.
 */
#  if SET_SWIZZLEHACK
#    undef GET
#    undef GET16
#    undef SET
#    undef SET16
#    undef SETF
#    undef SET_FIFO
#    undef SETF_FIFO

extern FxU32 swizzleRead32(volatile FxU32 *s);
extern FxU16 swizzleRead16(volatile FxU16 *s);
extern void  swizzleWrite32(volatile FxU32 *d, FxU32 s);
extern void  swizzleWrite16(volatile FxU16 *d, FxU16 s);
extern void  swizzleWriteF(volatile FxU32 *d, FxFloat s);
extern void  swizzleWriteLinear8(volatile FxU32 *d, FxU32 s);

#    define GET(s)          swizzleRead32(&(s))
#    define GET16(s)        swizzleRead16((FxU16 *)&(s))
#    define SET(d, s)       swizzleWrite32(&(d),(s))
#    define SET_FIFO(d, s)  swizzleWrite32(&(d),(s))
#    define SET16(d, s)     swizzleWrite16((FxU16 *)&(d),(s))
#    define SETF(d, s)      swizzleWriteF((volatile FxU32 *)&(d),(s))
#    define SETF_FIFO(d, s) swizzleWriteF((volatile FxU32 *)&(d),(s))

#  elif SET_BSWAP
#    undef GET
#    undef GET16
#    undef SET
#    undef SET16
#    undef SETF
#    undef SET_FIFO
#    undef SETF_FIFO

#    if  __POWERPC__ && defined(__MWERKS__)
#      define LWBRX(d, x)        __lwbrx(d, x)
#      define STWBRX(s, d, x)    __stwbrx(s, d, x)
#      define STHBRX(s, d, x)    __sthbrx(s, d, x)
#    else /* !__POWERPC__ && !defined(__MWERKS__) */
#      error "Define LWBRX/STWBRX macros"
#    endif /* !__POWERPC__ && !defined(__MWERKS__) */

#    define GET(s)               __lwbrx( (void*)&(s), 0 )
#    define GET16(s)             __lwbrx( (void*)&(s), 0 )
#    define SET(d, s)            __stwbrx( s, (void*)&(d), 0)
#    define SET16(d, s)          __sthbrx( s, (void*)&(d), 0 )
#    if !SLOW_SETF
#      define SETF(d, s)         __stwbrx( *((FxU32 *)&s), (void *)&(d), 0)
#    else /* !FAST_SETF */
#      define SETF(d, s)           { \
                                     const float temp = (s); \
                                     __stwbrx( *((FxU32*)&temp), (void*)&(d), 0 ); \
                                   }
#    endif
#    if PCI_BUMP_N_GRIND
#      define SET_FIFO(d,s)       setFifo((FxU32 *)&(d), s)
#      if !SLOW_SETF
#        define SETF_FIFO(d,s)      setFifo((FxU32 *)&(d), *(FxU32 *)&(s))
#      else
#        define SETF_FIFO(d,s)      setFifo(&(d), s)
#      endif
#      define SET_LINEAR_8(d, s)  setFifo8((FxU32 *)&(d), (s))
#    else  /* !PCI_BUMP_N_GRIND */
#      define SET_FIFO(d,s)       SET(d,s)
#      define SETF_FIFO(d,s)      SETF(d,s)
#      define SET_LINEAR_8(d, s)  ((d) = (s))
#    endif /* !PCI_BUMP_N_GRIND */
   
#    define SET_LINEAR(d, s)     SET_FIFO((d), (s))
#    define SET_LINEAR_16(d, s)  SET_FIFO((d), ((((FxU32)(s)) >> 16UL) | \
                                               (((FxU32)(s)) << 16UL)))
#  else /* !SET_BSWAP */
#    undef GET
#    undef GET16

#    define GET(s)   s
#    define GET16(s) s
#  endif /* !SET_BSWAP */


#if GLIDE_USE_DEBUG_FIFO
#define kDebugFifoSize 0x1000UL
#endif /* GLIDE_USE_DEBUG_FIFO */

/* If there wasn't a platform defined SET_LINEAR_XXX then just use
 * the default SET for the rest of the hw writes.
 */
#ifndef SET_LINEAR
#define SET_LINEAR(__addr, __val)    SET_FIFO(__addr, __val)
#define SET_LINEAR_16(__addr, __val) SET_FIFO(__addr, __val)
#define SET_LINEAR_8(__addr, __val)  SET_FIFO(__addr, __val)
#endif /* !defined(SET_LINEAR) */

/* Extract the fp exponent from a floating point value.
 * NB: The value passed to this macro must be convertable
 * into an l-value.
 */
#define kFPExpMask        0x7F800000UL
#define kFPZeroMask       0x80000000UL
#define kFPExpShift       0x17UL
#define FP_FLOAT_EXP(__fpVal)   ((FxU32)(((*(const FxU32*)(&(__fpVal))) & kFPExpMask) >> kFPExpShift))
#define FP_FLOAT_ZERO(__fpVal)  (((*(const FxU32*)(&(__fpVal))) & ~kFPZeroMask) == 0x00)

/* The two most commonly defined macros in the known universe */
#define MIN(__x, __y) (((__x) < (__y)) ? (__x) : (__y))
#define MAX(__x, __y) (((__x) < (__y)) ? (__y) : (__x))

/* Simple macro to make selecting a value against a boolean flag
 * simpler w/o a conditional. 
 *
 * NB: This requires that the boolean value being passed in be the
 * result of one of the standard relational operators. 
 */
#define MaskSelect(__b, __val) (~(((FxU32)(__b)) - 1UL) & (__val))

/* Chipfield ids that glide uses. */
#define kChipFieldShift (8UL + 3UL)
typedef enum {
  eChipBroadcast    = 0x00UL,
  eChipFBI          = 0x01UL,
  eChipTMU0         = 0x02UL,
  eChipTMU1         = 0x04UL,
  eChipTMU2         = 0x08UL,
  eChipAltBroadcast = 0x0FUL,
} FifoChipField;

#define BROADCAST_ID eChipBroadcast

#define WAX_ID       FX_BIT(14)

/* Although these are named reg_group_xxx they are generic options for
 * grouping register writes and should be fine w/ and w/o the fifo
 * being enabled.  
 */
#if GLIDE_DEBUG
#define REG_GROUP_DCL(__regMask, __regBase, __groupNum, __checkP) \
const FxBool _checkP = (__checkP); \
const FxU32 _groupNum = (__groupNum);\
const FxU32 _regMask = (__regMask); \
FxU32 _regCheckMask = (__regMask); \
FxU32 _regBase = offsetof(SstRegs, __regBase)

#define REG_GROUP_DCL_WAX(__regMask, __regBase, __groupNum, __checkP) \
const FxBool _checkP = (__checkP); \
const FxU32 _groupNum = (__groupNum);\
const FxU32 _regMask = (__regMask); \
FxU32 _regCheckMask = (__regMask); \
FxU32 _regBase = offsetof(SstGRegs, __regBase)


#define REG_GROUP_ASSERT(__regAddr, __val, __floatP) \
{ \
  const FxU32 curRegAddr = offsetof(SstRegs, __regAddr); \
  const FxU32 curRegIndex = (curRegAddr - _regBase) >> 2; \
  const FxU32 curRegBit = (0x01UL << curRegIndex); \
  const float floatVal = (const float)(__val); \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X : 0x%X) : 0x%X\n", \
            curRegIndex, curRegAddr, *(const FxU32*)&floatVal); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
  GR_ASSERT((_regMask & curRegBit) == curRegBit);                            /* reg allocated in mask */ \
  if (curRegIndex > 0) \
    GR_ASSERT(((0xFFFFFFFFUL >> (32 - curRegIndex)) & _regCheckMask) == 0x00); /* All previous regs done */ \
  _regCheckMask ^= curRegBit;                                                /* Mark current reg */ \
}

#define REG_GROUP_ASSERT_WAX(__regAddr, __val, __floatP) \
{ \
  const FxU32 curRegAddr = offsetof(SstGRegs, __regAddr); \
  const FxU32 curRegIndex = (curRegAddr - _regBase) >> 2; \
  const FxU32 curRegBit = (0x01UL << curRegIndex); \
  const float floatVal = (const float)(__val); \
  GDBG_INFO(220, "\t(0x%X : 0x%X) : 0x%X\n", \
            curRegIndex, curRegAddr, *(const FxU32*)&floatVal); \
   GR_CHECK_COMPATABILITY(FN_NAME, \
                          !gc->open, \
                          "Called before grSstWinOpen()"); \
  GR_ASSERT((_regMask & curRegBit) == curRegBit);                            /* reg allocated in mask */ \
  if (curRegIndex > 0) \
  GR_ASSERT(((0xFFFFFFFFUL >> (32 - curRegIndex)) & _regCheckMask) == 0x00); /* All previous regs done */ \
  _regCheckMask ^= curRegBit;                                                /* Mark current reg */ \
}

#else /* !GDBG_INFO_ON */
#define REG_GROUP_DCL(__regMask, __regBase, __groupNum, __checkP) 
#define REG_GROUP_DCL_WAX(__regMask, __regBase, __groupNum, __checkP) 
#define REG_GROUP_ASSERT(__regAddr, __val, __floatP)
#define REG_GROUP_ASSERT_WAX(__regAddr, __val, __floatP)
#endif /* !GDBG_INFO_ON */

#if GLIDE_HW_TRI_SETUP
enum {
   kSetupStrip           = 0x00,
   kSetupFan             = 0x01,
   kSetupCullDisable     = 0x00,
   kSetupCullEnable      = 0x02,
   kSetupCullPositive    = 0x00,
   kSetupCullNegative    = 0x04,
   kSetupPingPongNorm    = 0x00,
   kSetupPingPongDisable = 0x08
};
#define GR_CULL_MASK 0xff7fffff
#endif /* GLIDE_HW_TRI_SETUP */

#define REGNUM(__reg)   (offsetof(SstRegs, __reg) >> 2)
#define REGNUM_WAX(_reg) (offsetof(SstGRegs, __reg) >> 2)

#define PACKET_HEADER_ADD(__start, __reg, __header) \
do {\
  GDBG_PRINTF("%x, %x\n", REGNUM(__reg), REGNUM(__start));\
  GR_ASSERT((REGNUM(__reg) - REGNUM(__start)) <= 14);\
  __header |= ((1 << (REGNUM(__reg) - REGNUM(__start))) << 15);\
} while (0)


#if USE_PACKET_FIFO

#define REGNUM(__reg)  (offsetof(SstRegs, __reg) >> 2)
#define REGNUM_WAX(_reg) (offsetof(SstGRegs, __reg) >> 2)

/* The shift below is a bit tricky.  Watch out! */
#define FIFO_REG(__chipField, __field) \
   (GR_ASSERT(((FxU32)(__chipField)) < 0x10UL), \
    ((((FxU32)offsetof(SstRegs, __field)) << 1) | (((FxU32)(__chipField)) << kChipFieldShift)))    

/* And here's the WAX version */
/* The shift below is a bit tricky.  Watch out! */
#define FIFO_REG_WAX(__field) ((((FxU32)offsetof(SstGRegs, __field)) << 1) | SSTCP_PKT4_2D)
  
/* The REG_GROUP_XXX macros do writes to a monotonically increasing
 * set of registers. There are three flavors of the macros w/
 * different restrictions etc.
 *
 * NB: Care must be taken to order the REG_GROUP_SET macro uses to
 * match the actual register order, otherwise all hell breaks loose.  
 */

/* Write to __groupNum registers (max 14) starting at __regBase under
 * the control of __groupMask (lsb->msb).
 */
#define REG_GROUP_BEGIN(__chipId, __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, \
                         __groupMask, (((__groupMask) << SSTCP_PKT4_MASK_SHIFT) | \
                                       FIFO_REG(__chipId, __regBase) | \
                                       SSTCP_PKT4), \
                         FXTRUE)

#define REG_GROUP_BEGIN_WAX(__regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum, __groupMask, \
(((__groupMask) << SSTCP_PKT4_MASK_SHIFT) | FIFO_REG_WAX(__regBase) | \
SSTCP_PKT4), FXTRUE)
  
/* Register writes (<= 32) sequentially starting at __regBase */
#define REG_GROUP_LONG_BEGIN(__chipId, __regBase, __groupNum) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 32)); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, \
                         (0xFFFFFFFF >> (32 - (__groupNum))), \
                         (((__groupNum) << SSTCP_PKT1_NWORDS_SHIFT) | \
                          FIFO_REG(__chipId, __regBase) | \
                          SSTCP_INC | \
                          SSTCP_PKT1), \
                         FXTRUE)

#define REG_GROUP_BEGIN_INTERNAL(__chipId, __regBase, __groupNum, __groupMask,\
                                 __pktHdr, __checkP)                          \
if (gc->contextP) {                                                       \
  GR_DCL_GC;                                                              \
  volatile FxU32* _regGroupFifoPtr = gc->cmdTransportInfo.fifoPtr;        \
  REG_GROUP_DCL(__groupMask, __regBase, __groupNum, __checkP);            \
  DBG_CALL( _reg_group_begin_internal( __chipId, offsetof(SstRegs, __regBase), __groupNum,   \
                                      __groupMask, __pktHdr, __checkP,    \
                                      _regGroupFifoPtr ) );               \
  SET_FIFO(*_regGroupFifoPtr++, (__pktHdr));                                   


#define REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum,      \
                                     __groupMask, __pktHdr, __checkP)      \
if (gc->contextP) {                                                        \
    GR_DCL_GC;                                                             \
    volatile FxU32* _regGroupFifoPtr = gc->cmdTransportInfo.fifoPtr;       \
    REG_GROUP_DCL_WAX(__groupMask, __regBase, __groupNum, __checkP);       \
    DBG_CALL( _reg_group_begin_internal_wax( offsetof(SstGRegs, __regBase), __groupNum,\
                                             __groupMask, __pktHdr, __checkP,\
                                             _regGroupFifoPtr ) );          \
    SET_FIFO(*_regGroupFifoPtr++, (__pktHdr));                                   


#define REG_GROUP_SET(__regBase, __regAddr, __val)                       \
do {                                                                     \
  REG_GROUP_ASSERT(__regAddr, __val, FXFALSE);                           \
  FXUNUSED(__regBase);                                                   \
  if ( !gc->windowed ) {                                                 \
    GDBG_INFO(120, "fifoReadPtr(HW): 0x%x\n",                            \
              GET(gc->cRegs->cmdFifo0.readPtrL));                        \
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) >=                       \
              gc->cmdTransportInfo.fifoOffset);                          \
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) <                        \
              (gc->cmdTransportInfo.fifoOffset +                         \
               gc->cmdTransportInfo.fifoSize));                          \
    GDBG_INFO(120, "REG_GROUP_SET:\n");                                  \
  }                                                                      \
  GDBG_INFO(120, "\tFile: %s Line %d\n", __FILE__, __LINE__);            \
  GDBG_INFO(120, "\tfifoPtr: 0x%x, Val: 0x%x\n", (FxU32) _regGroupFifoPtr - (FxU32)gc->rawLfb, __val);\
  SET_FIFO(*_regGroupFifoPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

/* NB: This is a completely unsafe variant if you know that you built
 * your packet header correctly. You have been warned.  
 */
#define REG_GROUP_INDEX_SET(__val)                                       \
do {                                                                     \
  if ( !gc->windowed ) {                                                 \
    GDBG_INFO(120, "fifoReadPtr(HW): 0x%x\n",                            \
              GET(gc->cRegs->cmdFifo0.readPtrL));                        \
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) >=                       \
              gc->cmdTransportInfo.fifoOffset);                          \
    GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) <                        \
              (gc->cmdTransportInfo.fifoOffset +                         \
               gc->cmdTransportInfo.fifoSize));                          \
    GDBG_INFO(120, "REG_GROUP_SET:\n");                                  \
  }                                                                      \
  GDBG_INFO(120, "\tFile: %s Line %d\n", __FILE__, __LINE__);            \
  GDBG_INFO(120, "\tfifoPtr: 0x%x, Val: 0x%x\n", (FxU32) _regGroupFifoPtr - (FxU32)gc->rawLfb, __val);\
  SET_FIFO(*_regGroupFifoPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define REG_GROUP_SET_WAX(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT_WAX(__regAddr, __val, FXFALSE); \
  FXUNUSED(__regBase); \
  if ( !gc->windowed ) {                                                 \
  GDBG_INFO(220, "fifoReadPtr(HW): 0x%x\n", gc->cRegs->cmdFifo0.readPtrL);\
  GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) >= gc->cmdTransportInfo.fifoOffset);\
  GR_ASSERT(GET(gc->cRegs->cmdFifo0.readPtrL) < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));\
  }                                                                      \
  GDBG_INFO(220, "REG_GROUP_SET_WAX:\n");\
  GDBG_INFO(220, "\tFile: %s Line %d\n", __FILE__, __LINE__);\
  GDBG_INFO(220, "\tfifoPtr: 0x%x, Val: 0x%x\n", (FxU32) _regGroupFifoPtr - (FxU32)gc->rawLfb, __val);\
  SET_FIFO(*_regGroupFifoPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)


#define REG_GROUP_SETF(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXTRUE); \
  FXUNUSED(__regBase); \
  SETF_FIFO(*(FxFloat*)_regGroupFifoPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)

#if GLIDE_FP_CLAMP
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
do { \
  const FxU32 fpClampVal = FP_FLOAT_CLAMP(__val); \
  REG_GROUP_ASSERT(__regAddr, fpClampVal, FXTRUE); \
  FXUNUSED(__regBase); \
  SETF_FIFO(*(FxFloat*)_regGroupFifoPtr++, fpClampVal); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
#else
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
  REG_GROUP_SETF(__regBase, __regAddr, __val)
#endif

#define REG_GROUP_END() \
  ASSERT(_checkP); \
  ASSERT((((unsigned long)_regGroupFifoPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr) >> 2) == _groupNum + 1); \
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)_regGroupFifoPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = (FxU32*)_regGroupFifoPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tGroupEnd: (0x%X : 0x%X)\n", \
            gc->cmdTransportInfo.fifoPtr, gc->cmdTransportInfo.fifoRoom); \
  FIFO_ASSERT(); \
} \
GR_CHECK_SIZE()

#define STORE_FIFO(__chipId, __base, __field, __val) \
do { \
  if (gc->contextP) { \
    FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
    FXUNUSED(__base); \
    GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
    GR_CHECK_COMPATABILITY(FN_NAME, \
                           !gc->open, \
                           "Called before grSstWinOpen()"); \
    DEBUGFIFOWRITE(&((SstRegs*)(__base))->__field, __val, curFifoPtr); \
    SET_FIFO(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                        FIFO_REG(__chipId, __field) |          /* chip[14:10] num[9:3] */ \
                        SSTCP_PKT1));                          /* type (1) */ \
    SET_FIFO(*curFifoPtr++, __val); \
    gc->cmdTransportInfo.fifoPtr += 2; \
    gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
    FIFO_ASSERT(); \
    GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
  } \
} while(0)

#define STORE_FIFO_WAX(__chipId, __base, __field, __val) \
do { \
  if (gc->contextP) { \
    FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
    FXUNUSED(__base); \
    GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
    GR_CHECK_COMPATABILITY(FN_NAME, \
                           !gc->open, \
                           "Called before grSstWinOpen()"); \
    DEBUGFIFOWRITE(&((SstGRegs*)(__base))->__field, __val, curFifoPtr); \
    SET_FIFO(*curFifoPtr++,\
      ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
        FIFO_REG_WAX(__field) | /* chip[14:10] num[9:3] */ \
        SSTCP_PKT1 | FXBIT(14)));                          /* type (1) */ \
    SET_FIFO(*curFifoPtr++, __val); \
    gc->cmdTransportInfo.fifoPtr += 2; \
    gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
    FIFO_ASSERT(); \
    GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
  } \
} while(0)

#define STORE_FIFO_INDEX(__chipId, __base, __regIndex, __val) \
do { \
  if (gc->contextP) { \
    FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
    FXUNUSED(__base); \
    GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
    GR_CHECK_COMPATABILITY(FN_NAME, \
                           !gc->open, \
                           "Called before grSstWinOpen()"); \
    DEBUGFIFOWRITE(&((FxU32*)(__base))[__regIndex], __val, curFifoPtr); \
    SET_FIFO(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                        ((__chipId) << kChipFieldShift) |      /* chip[14:10] */ \
                        ((__regIndex) << 3) |                    /* Reg Num[9:3] */ \
                        SSTCP_PKT1));                          /* type (1) */ \
    SET_FIFO(*curFifoPtr++, __val); \
    gc->cmdTransportInfo.fifoPtr += 2; \
    gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
    FIFO_ASSERT(); \
    GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
  } \
} while(0) 

#define STOREF_FIFO_INDEX(__chipId, __base, __regIndex, __val) \
do { \
  if (gc->contextP) { \
    FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
    FXUNUSED(__base); \
    GR_ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
    GR_CHECK_COMPATABILITY(FN_NAME, \
                           !gc->open, \
                           "Called before grSstWinOpen()"); \
    DEBUGFIFOFWRITE(&((FxU32*)(__base))[__regIndex], __val, curFifoPtr); \
    SET_FIFO(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                        ((__chipId) << kChipFieldShift) |      /* chip[14:10] */ \
                        ((__regIndex) << 3) |                    /* Reg Num[9:3] */ \
                        SSTCP_PKT1));                          /* type (1) */ \
    SETF_FIFO(*curFifoPtr++, __val); \
    gc->cmdTransportInfo.fifoPtr += 2; \
    gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
    FIFO_ASSERT(); \
    GR_INC_SIZE(sizeof(FxU32));  /* Size of actual write not including header */ \
  } \
} while(0)

#define STORE16_FIFO(__chipId, __base, __field, __val) \
do { \
  if (gc->contextP) { \
    FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
    const FxU32 temp32 = (((FxU32)(__val)) & 0x0000FFFF); \
    FXUNUSED(__base); \
    ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
    GR_CHECK_COMPATABILITY(FN_NAME, \
                           !gc->open, \
                           "Called before grSstWinOpen()"); \
    DEBUGFIFOWRITE(&((SstRegs*)(__base))->__field, __val, curFifoPtr); \
    SET_FIFO(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |       /* size (32bit words) */ \
                        FIFO_REG(__chipId, __field) |             /* chip[14:10] num[9:3] */ \
                        SSTCP_PKT1));                             /* type (1) */ \
    SET_FIFO(*curFifoPtr++, temp32); \
    gc->cmdTransportInfo.fifoPtr += 2; \
    gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
    FIFO_ASSERT(); \
    GR_INC_SIZE(sizeof(FxU32)); /* Size of actual write not including header */ \
  } \
} while(0)

#define STOREF_FIFO(__chipId, __base, __field, __val) \
do { \
  if (gc->contextP) { \
    FxU32* curFifoPtr = gc->cmdTransportInfo.fifoPtr; \
    FXUNUSED(__base); \
    ASSERT(((FxU32)(curFifoPtr) & FIFO_ALIGN_MASK) == 0);    /* alignment */ \
    GR_CHECK_COMPATABILITY(FN_NAME, \
                           !gc->open, \
                           "Called before grSstWinOpen()"); \
    DEBUGFIFOFWRITE(&((SstRegs*)(__base))->__field, __val, curFifoPtr); \
    SET_FIFO(*curFifoPtr++, ((0x01 << SSTCP_PKT1_NWORDS_SHIFT) |    /* size (32bit words) */ \
                        FIFO_REG(__chipId, __field) |          /* chip[14:10] num[9:3] */ \
                        SSTCP_PKT1));                          /* type (1) */ \
    SETF_FIFO(*(FxFloat*)curFifoPtr, __val); \
    curFifoPtr++; \
    gc->cmdTransportInfo.fifoPtr += 2; \
    gc->cmdTransportInfo.fifoRoom -= (sizeof(FxU32) << 1); \
    FIFO_ASSERT(); \
    GR_INC_SIZE(sizeof(FxU32)); /* Size of actual write not including header */ \
  } \
} while(0)

/* There are now three different flavors of the packet 3 macros for
 * your coding pleasure. In increasing order of complexity and control
 * they are TRI_BEGIN, TRI_STRIP_BEGIN, TRI_PACKET_BEGIN.
 * 
 * NB: All of these macros must be terminated w/ a matching invocation of
 *     TRI_END otherwise all sorts of hell will break loose.
 * 
 * TRI_BEGIN: 
 *   The simplest form that draws a single indepependent triangle whose 
 *   parameters and culling are all the glide defaults for grDrawTriangle.
 *
 * TRI_STRIP_BEGIN:
 *   setupMode:  [kSetupStrip | kSetupFan]. Culling defaults to the current
 *               glide setting, w/ strips/fans defaulting to ping-pong culling
 *   nVertex:    The number of vertices for the current packet (max 15).
 *   vertexSize: Size in bytes of the parameters for the vertices making up
 *               the current packet.
 *   cmd:        [SSTCP_PKT3_BDDBDD (Independent)
 *                SSTCP_PKT3_BDDDDD (Start strip/fan)
 *                SSTCP_PKT3_DDDDDD (Continue strip)]
 *
 * TRI_PACKET_BEGIN:
 *   setupMode:  The same as with TRI_STRIP_BEGIN, except that the caller
 *               needs to specify the culling bits kSetupCullXXX/kSetupPingPongXXX.
 *   params:     Bits matching the descriptin of the sMode register describing 
 *               which parameters are specified in the packet.
 *   nVertex:    See TRI_STRIP_BEGIN.
 *   vertexSize: See TRI_STRIP_BEGIN.
 *   cmd:        See TRI_STRIP_BEGIN.
 */
#define TRI_PACKET_BEGIN(__setupMode, __params, __nVertex, __vertexSize, __cmd) \
if (gc->contextP) { \
  FxU32* tPackPtr = gc->cmdTransportInfo.fifoPtr; \
  const FxU32 packetVal = (((__setupMode) << SSTCP_PKT3_SMODE_SHIFT) |   /* [27:22] */ \
                           (__params) |                                  /* pack[28] params[21:10] */ \
                           ((__nVertex) << SSTCP_PKT3_NUMVERTEX_SHIFT) | /* [9:6] */ \
                           (__cmd) |                                     /* command [5:3] */ \
                           SSTCP_PKT3);                                  /* type [2:0] */ \
  TRI_ASSERT_DECL(__nVertex, __vertexSize, packetVal); \
  SET_FIFO(*tPackPtr++, packetVal)

#define TRI_STRIP_BEGIN(__setupMode, __nVertex, __vertexSize, __cmd) \
if (gc->contextP) { \
  FxU32* tPackPtr = gc->cmdTransportInfo.fifoPtr; \
  const FxU32 packetVal = (((__setupMode) << SSTCP_PKT3_SMODE_SHIFT) |   /* [27:22] */ \
                           ((__nVertex) << SSTCP_PKT3_NUMVERTEX_SHIFT) | /* [9:6] */ \
                           (__cmd) |                                     /* command [5:3] */ \
                           gc->cmdTransportInfo.cullStripHdr); \
  TRI_ASSERT_DECL(__nVertex, __vertexSize, packetVal); \
  SET_FIFO(*tPackPtr++, packetVal)

#define TRI_BEGIN() \
if (gc->contextP) { \
  FxU32* tPackPtr = gc->cmdTransportInfo.fifoPtr; \
  TRI_ASSERT_DECL(3, gc->curVertexSize, gc->cmdTransportInfo.triPacketHdr); \
  SET_FIFO(*tPackPtr++, gc->cmdTransportInfo.triPacketHdr)

#if GDBG_INFO_ON
extern void
_grH3FifoDump_TriHdr(const FxU32 triPacketHdr);
extern void
_grH3FifoDump_Linear(const FxU32* const linearPacketAddr);

#define DEBUGFIFODUMP_TRI(__packetAddr)    _grH3FifoDump_TriHdr(__packetAddr)
#define DEBUGFIFODUMP_LINEAR(__packetAddr) _grH3FifoDump_Linear(__packetAddr)

#define TRI_ASSERT_DECL(__nVerts, __vertSize, __packetHdr) \
  const FxU32 nVertex = (__nVerts); \
  const FxU32 sVertex = (__vertSize); \
  FxU32 pCount = 0; \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         !gc->open, \
                         "Called before grSstWinOpen()"); \
  GR_ASSERT(((FxU32)(tPackPtr) & FIFO_ALIGN_MASK) == 0);   /* alignment */ \
  GR_ASSERT((((__nVerts) * (__vertSize)) + sizeof(FxU32)) <= (FxU32)gc->cmdTransportInfo.fifoRoom); \
  GR_ASSERT((((FxU32)tPackPtr) + ((__nVerts) * (__vertSize)) + sizeof(FxU32)) < \
            (FxU32)gc->cmdTransportInfo.fifoEnd); \
  GR_ASSERT(nVertex < 0x10); \
  GR_ASSERT(nVertex > 0x00); \
  GR_ASSERT(((__packetHdr) & 0xE0000000UL) == 0x00UL); \
  FIFO_ASSERT(); \
  GDBG_INFO(120, "Triangle(0x%X): (0x%X : 0x%X)\n", (__packetHdr), __nVerts, __vertSize); \
  DEBUGFIFODUMP_TRI(__packetHdr)
#define CLAMP_DUMP(__val, __floatVal) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X) : V#: 0x%X - P#: 0x%X - ParamVal: (%f : 0x%X)\n", \
            (FxU32)tPackPtr, \
            ((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (((__val) < 786432.875) ? (__val) : ((__val) - 786432.875)), \
            (__floatVal))
#define SETF_DUMP(__val) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X) : V#: 0x%X - P#: 0x%X - ParamVal: %f\n", \
            (FxU32)tPackPtr, \
            ((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (((__val) < 786432.875) ? (__val) : ((__val) - 786432.875)))
#define SET_DUMP(__val) \
  pCount++; \
  GDBG_INFO(gc->myLevel + 200, "\t(0x%X) : V#: 0x%X - P#: 0x%X - ParamVal: 0x%X\n", \
            (FxU32)tPackPtr, \
            ((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) / sVertex, \
             (((FxU32)tPackPtr - ((FxU32)gc->cmdTransportInfo.fifoPtr + sizeof(FxU32))) % sVertex) >> 2, \
            (__val))
#define TRI_ASSERT() \
  GR_ASSERT(pCount == (nVertex * (sVertex >> 2))); \
  ASSERT(((FxU32)tPackPtr - (FxU32)gc->cmdTransportInfo.fifoPtr) == (nVertex * sVertex) + sizeof(FxU32))
#else /* !GDBG_INFO_ON */
#define DEBUGFIFODUMP_TRI(__packetAddr)
#define DEBUGFIFODUMP_LINEAR(__packetAddr)

#define CLAMP_DUMP(__val, __floatVal) 
#define SETF_DUMP(__val)
#define SET_DUMP(__val)

#define TRI_ASSERT_DECL(__nVerts, __vertSize, __packetHdr)
#define TRI_ASSERT()
#endif /* !GDBG_INFO_ON */

/* Get the integer representation of the color component.  Currently,
 * following in the 'Glide is not an API for kids' tradition we'll
 * probably do something silly like wrap around zero.
 */
#if GLIDE_PACKED_RGB
#define RGBA_COMP(__fpVal, __fpBias, __fpShift, __fpMask) \
((gc->pool.temp1.f = (float)((float)(__fpVal + (float)(__fpBias))), \
 GR_ASSERT((__fpVal) >= 0.0f), \
 GR_ASSERT((__fpVal) < 256.0f), \
 ((gc->pool.temp1.u & (__fpMask)) << (__fpShift)))
                                                  
#define RGBA_COMP_CLAMP(__fpVal, __compToken) \
   RGBA_COMP(__fpVal, kPackBias##__compToken, kPackShift##__compToken, kPackMask##__compToken)
#endif /* GLIDE_PACKED_RGB */

/* First stage tsu-subtractor chec/fix. 
 * Mmm..... sequence operator.
 */
#if GLIDE_FP_CLAMP
#define kFPClampThreshold 0x20UL
#define FP_FLOAT_CLAMP(__fpVal) ((FP_FLOAT_EXP(__fpVal) < kFPClampThreshold) \
                                 ? (gc->stats.tsuValClamp++, 0x00UL) \
                                 : *(const FxU32*)(&(__fpVal)))

#define TRI_SETF_CLAMP(__val) \
do { \
  const FxU32 floatCastVal = FP_FLOAT_CLAMP(__val); \
  CLAMP_DUMP(__val, floatCastVal); \
  SET_FIFO(*tPackPtr++, floatCastVal); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)
#else
#define TRI_SETF_CLAMP(__val) \
    TRI_SETF(__val)
#endif

#define TRI_SETF(__val) \
do { \
  SETF_DUMP(__val); \
  SETF_FIFO(*tPackPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)

#define TRI_SET(__val) \
do { \
  SET_DUMP(__val); \
  SET_FIFO(*tPackPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define TRI_END \
  TRI_ASSERT(); \
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)tPackPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = tPackPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tTriEnd: (0x%X : 0x%X)\n", tPackPtr, gc->cmdTransportInfo.fifoRoom); \
  FIFO_ASSERT(); \
}

#define FIFO_LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN, __f, __l) \
{ \
  FxU32* packetPtr = gc->cmdTransportInfo.fifoPtr; \
  const FxU32 __writeSize = (__numWords);       /* Add size of packet header */ \
  const FxU32 hdr1 = ((__type) | \
                      (((FxU32)(__maskW2)) << SSTCP_PKT5_BYTEN_W2_SHIFT) | \
                      (((FxU32)(__maskWN)) << SSTCP_PKT5_BYTEN_WN_SHIFT) | \
                      (__writeSize << SSTCP_PKT5_NWORDS_SHIFT) | \
                      SSTCP_PKT5); \
  const FxU32 hdr2 = ((FxU32)(__addr)) & SSTCP_PKT5_BASEADDR; \
  GR_CHECK_COMPATABILITY(FN_NAME, \
                         !gc->open, \
                         "Called before grSstWinOpen()"); \
  GR_ASSERT(((unsigned long)(packetPtr) & FIFO_ALIGN_MASK) == 0);        /* alignment */ \
  GR_ASSERT((__numWords) > 0);                                   /* packet size */ \
  GR_ASSERT((__numWords) < ((0x01 << 19) - 2)); \
  GR_ASSERT((((FxU32)(__numWords) + 2) << 2) <= (FxU32)gc->cmdTransportInfo.fifoRoom); \
  GR_ASSERT(((unsigned long)packetPtr + (((__numWords) + 2) << 2)) < \
            (unsigned long)gc->cmdTransportInfo.fifoEnd); \
  GR_ASSERT((hdr2 & 0xE0000000UL) == 0x00UL); \
  GR_ASSERT(((__addr) & 0x03UL) == 0x00UL); \
  FIFO_ASSERT(); \
  GDBG_INFO(120, "LinearWrite(0x%X : 0x%X)\n", hdr1, hdr2); \
  GDBG_INFO(gc->myLevel + 200, "\tFile: %s - Line: %ld\n", __f, __l); \
  GDBG_INFO(gc->myLevel + 200, "\tType: 0x%X\n", (FxU32)(__type)); \
  GDBG_INFO(gc->myLevel + 200, "\tAddr: 0x%X\n", (FxU32)(__addr)); \
  GDBG_INFO(gc->myLevel + 200, "\tMaskW2: 0x%X\n", (FxU32)(__maskW2)); \
  GDBG_INFO(gc->myLevel + 200, "\tMaskWN: 0x%X\n", (FxU32)(__maskWN)); \
  GDBG_INFO(gc->myLevel + 200, "\twriteSize: 0x%X\n", __writeSize); \
  GDBG_INFO(gc->myLevel + 200, "\thdr 1: 0x%X\n", hdr1); \
  GDBG_INFO(gc->myLevel + 200, "\thdr 2: 0x%X\n", hdr2); \
  SET_FIFO(*packetPtr++, hdr1); \
  SET_FIFO(*packetPtr++, hdr2); \
  GR_INC_SIZE(sizeof(FxU32))

#define FIFO_LINEAR_WRITE_SET(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%X : 0x%X\n", packetPtr, (__val)); \
  GDBG_INFO(120, "fifPtr: 0x%x, val: 0x%x\n\n", packetPtr, __val);\
  if ( !gc->windowed ) { \
    GR_ASSERT(gc->cRegs->cmdFifo0.readPtrL >= gc->cmdTransportInfo.fifoOffset);\
    GR_ASSERT(gc->cRegs->cmdFifo0.readPtrL < (gc->cmdTransportInfo.fifoOffset + gc->cmdTransportInfo.fifoSize));\
  } \
  SET_LINEAR(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
  
#define FIFO_LINEAR_WRITE_SET_16(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%X : 0x%X\n", packetPtr, (__val)); \
  SET_LINEAR_16(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
  
#define FIFO_LINEAR_WRITE_SET_8(__val) \
do { \
  GDBG_INFO(gc->myLevel + 205, "\t0x%X : 0x%X\n", packetPtr, (__val)); \
  SET_LINEAR_8(*packetPtr++, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define FIFO_LINEAR_WRITE_END \
  DEBUGFIFODUMP_LINEAR(gc->cmdTransportInfo.fifoPtr); \
  GR_ASSERT((((unsigned long)packetPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr) >> 2) == __writeSize + 2); \
  gc->cmdTransportInfo.fifoRoom -= ((unsigned long)packetPtr - (unsigned long)gc->cmdTransportInfo.fifoPtr); \
  gc->cmdTransportInfo.fifoPtr = packetPtr; \
  GDBG_INFO(gc->myLevel + 200, "\tLinearEnd: (0x%X : 0x%X)\n", \
            packetPtr, gc->cmdTransportInfo.fifoRoom); \
  FIFO_ASSERT(); \
}

#  define GR_GET(s)                               GET(s)
#  define GR_CAGP_GET(__reg)                      GET(gc->cRegs->cmdFifo0.__reg)
#  define GR_CAGP_SET(__reg, __val)               SET(gc->cRegs->cmdFifo0.__reg, __val)
#  define GR_GET16(s)                             ((FxU16)GET16(s))
#  define GR_SET(c, h, f, s)                      STORE_FIFO(c, h, f, s)
#  define GR_SET_WAX(c, h, f, s)                  STORE_FIFO_WAX(c, h, f, s)
#  define GR_SET_INDEX(c, h, r, s)                STORE_FIFO_INDEX(c, h, r, s)
#  define GR_SET16(c, h, f, s)                    STORE16_FIFO(c, h, f, s)
#  define GR_SETF(c, h, f, s)                     STOREF_FIFO(c, h, f, s)
#  define GR_SETF_INDEX(c, h, r, s)               STOREF_FIFO_INDEX(c, h, r, s)
#  define GR_SET_DIRECT(__bc, __hw, __reg, __val) SET(gc->sstRegs->__reg, __val)
#  define GR_SET_IO(c, h, f, s)                   SET(gc->ioRegs->f, s)
#else /* !USE_PACKET_FIFO */
#  ifndef DIRECT_IO
#    define GR_GET(s)                 GET(s)
#    define GR_CAGP_GET(__reg)        GET(gc->cRegs->cmdFifo0.__reg)
#    define GR_GET16(s)               ((FxU16)GET16(s))
#    define GR_SET(c, h, f, s)        do {SET((h)->f, s); GR_INC_SIZE(4);} while(0)
#    define GR_SET_INDEX(c, h, r, s)  do {SET(((FxU32*)(h))[r], s); GR_INC_SIZE(sizeof(FxU32));} while(0)
#    define GR_SETF(c, h, f, s)       do {SETF(h->f, s); GR_INC_SIZE(4);} while(0)
#    define GR_SETF_INDEX(c, h, r, s) do {SETF(((FxU32*)(h))[r], s); GR_INC_SIZE(sizeof(FxU32));} while(0)
#    define GR_SET16(c, h, f, s)      do {SET16((h)->f, s); GR_INC_SIZE(2);} while(0)
#    define GR_SET_IO(c,h,f,s)        GR_SET(c, h, f, s)
#    define GR_SET_DIRECT(c,h,f,s)    GR_SET(c, h, f, s)
#  else
#    define GR_GET(s)   GET(s)
#    define GR_CAGP_GET(__reg) gc->cRegs->cmdFifo0.reg
#    define GR_GET16(s) ((FxU16)GET16(s))
#    define GR_SET(c, h, f, s) {(gc->sstRegs)->f = s; GR_INC_SIZE(4);}
#    define GR_SETF(c, h, f, s) {((volatile float) (gc->sstRegs)->f) = (float) (s); GR_INC_SIZE(4);}
#    define GR_SET_INDEX(c, h, r, s) {((FxU32 *)((gc->sstRegs)[r] = s; GR_INC_SIZE(4);}
#    define GR_SETF_INDEX(c, h, r, s) {((volatile float *)(gc->sstRegs))[r] = (float) (s); GR_INC_SIZE(4);}
#    define GR_SET16(c, h, f, s) {*((FxU16 *)((gc->sstRegs)->f)) = s; GR_INC_SIZE(4);}
#    define GR_SET_DIRECT(__b, __ptr, __reg, __val) \
{\
  __ptr->__reg = __val;\
  GR_INC_SIZE(4);\
}
#    define GR_SET_IO(c, h, f, s) (gc->ioRegs)->f = s
#  endif                                   
#endif /* !USE_PACKET_FIFO */

/* Macros to do linear writes to lfb/tex memory. 
 *
 * LINEAR_WRITE_BEGIN - Setup stuff for the linear write. 
 *
 * numWords: The number of words to actually write to the destination
 * address. This does *NOT* include the packet headers etc for any
 * command fifos etc.
 *
 * type: One of the kLinearWriteXXX enum values above. This can
 * control what the legal values for addr and maskXX are.
 *
 * addr: Base address to the start the write.
 *
 * maskXX: Control what bytes in a write are active, these are active
 * low. W2 controls the masking of the first 32bit word written, and
 * WN controls all of the other writes.
 *
 * LINEAR_WRITE_SET - Writes are done in 32-bit increments, and must
 * be properly aligned etc. This can only be used inside of a
 * LINEAR_WRITE_BEGIN/LINEAR_WRITE_END pair.
 *
 * LINEAR_WRITE_EDGE - Write to a 16-bit value to an address. The
 * address must be aligned for at 16-bit access, and should not appear
 * within a LINEAR_WRITE_BEGIN/LINEAR_WRITE_END pair.
 *
 * LINEAR_WRITE_END - Finish off any stuff for the linear write.  
 */

#if USE_PACKET_FIFO
#define LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN) \
{ \
   GR_SET_EXPECTED_SIZE(((FxU32)((__numWords) + 1UL) << 2UL), 1); \
   FIFO_LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN, __FILE__, __LINE__)
#define LINEAR_WRITE_SET(__addr, __val) \
   FIFO_LINEAR_WRITE_SET(__val)
#define LINEAR_WRITE_SET_16(__addr, __val) \
   FIFO_LINEAR_WRITE_SET_16(__val)
#define LINEAR_WRITE_SET_8(__addr, __val) \
   FIFO_LINEAR_WRITE_SET_8(__val)
#define LINEAR_WRITE_END() \
   FIFO_LINEAR_WRITE_END \
   GR_CHECK_SIZE(); \
}

/* Macro to write the edge cases of a linear write, for example to the
 * lfb w/ a 16-bit pixel value. We do some address manipulation here
 * since the cmd fifo only addresses 32-bit quantities, but allows us
 * to mask of crap for the actual write.
 */
#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
#define FIFO_LINEAR_EDGE_MASK_ADJUST(__mask) ((~(__mask)) & 0x0FUL)
#define FIFO_LINEAR_EDGE_SET(__val) FIFO_LINEAR_WRITE_SET((((__val) & 0xFFFF0000UL) >> 16UL) | \
                                                          (((__val) & 0x0000FFFFUL) << 16UL))
#else /* !USE_PACKET_FIFO */
#define FIFO_LINEAR_EDGE_SET(__val) FIFO_LINEAR_WRITE_SET(__val)
#define FIFO_LINEAR_EDGE_MASK_ADJUST(__mask) (__mask)
#endif

#define LINEAR_WRITE_EDGE(__type, __addr, __val, __valBytes) \
do { \
  const FxU32 edgeAddr = (FxU32)(((FxU32)__addr) & 0x03UL); \
  GR_ASSERT((__valBytes) <= sizeof(FxU32)); \
  GR_ASSERT((((FxU32)(__addr)) + (__valBytes)) <= ((((FxU32)(__addr)) & ~0x03UL) + sizeof(FxU32))); \
  LINEAR_WRITE_BEGIN(1, __type, ((FxU32)__addr & ~0x03UL), \
                     FIFO_LINEAR_EDGE_MASK_ADJUST((0xF0UL | (0x0FUL >> (__valBytes))) >> edgeAddr), \
                     0x00); \
  FIFO_LINEAR_EDGE_SET(((FxU32)(__val)) << (((sizeof(FxU32) - edgeAddr) << 3UL) - \
                                             ((__valBytes) << 3UL))); \
  LINEAR_WRITE_END(); \
} while(0) 
#else /* !USE_PACKET_FIFO */
# define LINEAR_WRITE_BEGIN(__numWords, __type, __addr, __maskW2, __maskWN) \
{ \
    GR_SET_EXPECTED_SIZE(((__numWords) << 2), (__numWords))
# define LINEAR_WRITE_SET(__addr, __val) \
do { \
   FxU32* tempAddr = (FxU32*)((FxU32)(__addr) + (FxU32)gc->rawLfb); \
   SET(*tempAddr, __val); \
   GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

# define LINEAR_WRITE_SET_16(__addr, __val)\
    LINEAR_WRITE_SET(__addr, (FxU32) __val)
# define LINEAR_WRITE_SET_8(__addr, __val)  \
    LINEAR_WRITE_SET(__addr, (FxU32) __val)

# define LINEAR_WRITE_EDGE(__type, __addr, __val, __isLeftP) \
do { \
   FxU32* tempAddr = (FxU32*)(__addr); \
   SET16(*tempAddr, __val); \
   GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
# define LINEAR_WRITE_END() \
   GR_CHECK_SIZE(); \
}

/* The REG_GROUP_XXX macros do writes to a monotonically increasing
 * set of registers. There are three flavors of the macros w/
 * different restrictions etc.
 *
 * NB: Care must be taken to order the REG_GROUP_SET macro uses to
 * match the actual register order, otherwise all hell breaks loose.  
 */

/* Write to __groupNum registers (max 14) starting at __regBase under
 * the control of __groupMask (lsb->msb).
 */
#define REG_GROUP_BEGIN(__chipId, __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__regBase, __groupNum, __groupMask, FXTRUE)

#define REG_GROUP_BEGIN_WAX(__chipId, __regBase, __groupNum, __groupMask) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 21)); \
GR_ASSERT(((__groupMask) & (SSTCP_PKT4_MASK >> SSTCP_PKT4_MASK_SHIFT)) != 0x00); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum, __groupMask, FXTRUE)

/* Register writes (<= 32) sequentially starting at __regBase */
#define REG_GROUP_LONG_BEGIN(__chipId, __regBase, __groupNum) \
GR_ASSERT(((__groupNum) >= 1) && ((__groupNum) <= 32)); \
GR_SET_EXPECTED_SIZE(sizeof(FxU32) * (__groupNum), 1); \
REG_GROUP_BEGIN_INTERNAL(__regBase, __groupNum, (0xFFFFFFFF >> (32 - (__groupNum))), FXTRUE)

#define REG_GROUP_BEGIN_INTERNAL(__regBase, __groupNum, __groupMask, __checkP) \
{ \
  GR_DCL_GC; \
  REG_GROUP_DCL(__groupMask, __regBase, __groupNum, __checkP); \
  GDBG_INFO(gc->myLevel + 100, "REG_GROUP_BEGIN: (0x%X : 0x%X)\n", \
            (__groupMask), offsetof(SstRegs, __regBase) >> 2)

#define REG_GROUP_BEGIN_INTERNAL_WAX(__regBase, __groupNum, __groupMask, __checkP) \
{ \
  GR_DCL_GC; \
  REG_GROUP_DCL_WAX(__groupMask, __regBase, __groupNum, __checkP); \
  GDBG_INFO(220, "REG_GROUP_BEGIN_WAX: (0x%X : 0x%X)\n", \
            (__groupMask), offsetof(SstGRegs, __regBase) >> 2)


#define REG_GROUP_SET(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXFALSE); \
  SET(((SstRegs*)(__regBase))->__regAddr, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)

#define REG_GROUP_SET_WAX(__regBase, __regAddr, __val) \
do { \
  SET(((SstGRegs*)((FxU32) __regBase + SST_2D_OFFSET))->__regAddr, (__val)); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)


#define REG_GROUP_SETF(__regBase, __regAddr, __val) \
do { \
  REG_GROUP_ASSERT(__regAddr, __val, FXTRUE); \
  SETF(((SstRegs*)(__regBase))->__regAddr, (__val)); \
  GR_INC_SIZE(sizeof(FxFloat)); \
} while(0)

#if GLIDE_FP_CLAMP
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
do { \
  const FxU32 fpClampVal = FP_FLOAT_CLAMP(__val); \
  REG_GROUP_ASSERT(__regAddr, fpClampVal, FXTRUE); \
  SET(((FxU32*)(__regBase))[offsetof(SstRegs, __regAddr) >> 2], fpClampVal); \
  GR_INC_SIZE(sizeof(FxU32)); \
} while(0)
#else
#define REG_GROUP_SETF_CLAMP(__regBase, __regAddr, __val) \
  REG_GROUP_SETF(__regBase, __regAddr, __val)
#endif

#define REG_GROUP_END() \
  ASSERT(_checkP); \
} \
GR_CHECK_SIZE()

#endif /* !USE_PACKET_FIFO */

#ifdef GLIDE3
/*
** Macro to handle clip space and viewport stuff
*/
#define TRI_SETF_SCALE_ADVANCE(_ptr,_scaler) \
    TRI_SETF(FARRAY(_ptr, i)*_scaler); dataElem++; i = gc->tsuDataList[dataElem]

#define DA_SETF_SCALE_ADVANCE(_ptr,_scaler) \
    DA_SETF(FARRAY(_ptr, i)*_scaler); dataElem++; i = gc->tsuDataList[dataElem]

#define DA_VP_SETFS(_s,_oow) \
{ \
  FxI32 i, dataElem=0; \
  i = gc->tsuDataList[dataElem]; \
  if (gc->state.paramIndex & (STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA)) { \
    if (gc->state.vData.colorType == GR_FLOAT) { \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) { \
        DA_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
    } \
    else { \
      DA_SETF(FARRAY(_s, i)); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \
    if (gc->state.shadow.fbzMode & SST_DEPTH_FLOAT_SEL) { \
      if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
        DA_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
      } else { \
        DA_SETF(_oow); \
      } \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
    else { \
      DA_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth + gc->state.Viewport.oz); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \
    if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) { \
      DA_SETF(FARRAY(_s, gc->state.vData.fogInfo.offset)*_oow); \
    } \
    else if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
      DA_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      DA_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \
      DA_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow); \
    } \
    else { \
      DA_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[0].s_scale); \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[0].t_scale); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \
      DA_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow); \
    } \
    else { \
      DA_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[1].s_scale); \
    DA_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[1].t_scale); \
  } \
}

#define TRI_VP_SETFS(_s,_oow) \
{ \
  FxI32 i, dataElem=0; \
  i = gc->tsuDataList[dataElem]; \
  if (gc->state.paramIndex & (STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA)) { \
    if (gc->state.vData.colorType == GR_FLOAT) { \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_ALPHA) { \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
    } \
    else { \
      TRI_SETF(FARRAY(_s, i)); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \
    if (gc->state.shadow.fbzMode & SST_DEPTH_FLOAT_SEL) { \
      if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
        TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
      } else { \
        TRI_SETF(_oow); \
      } \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
    else { \
      TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \
    if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.fogInfo.offset)*_oow); \
    } \
    else if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[0].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[0].t_scale); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow); \
    } \
    else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[1].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[1].t_scale); \
  } \
}

#define AA_TRI_VP_SETFS(_s,_oow) \
{ \
  FxI32 i, dataElem=0; \
  i = gc->tsuDataList[dataElem]; \
  if (gc->state.paramIndex & (STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA)) { \
    if (gc->state.vData.colorType == GR_FLOAT) { \
      if (gc->state.paramIndex & STATE_REQUIRES_IT_DRGB) { \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
        TRI_SETF_SCALE_ADVANCE(_s,_GlideRoot.pool.f255); \
      } \
      TRI_SETF(0.0f); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
    else { \
      union { float f; FxU32 u; } argb; \
      argb.f = *(float *)((unsigned char *)_s + i); \
      argb.u &= 0x00ffffff; \
      TRI_SETF(argb.f); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOZ) { \
    if (gc->state.shadow.fbzMode & SST_DEPTH_FLOAT_SEL) { \
      if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
        TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
      } else { \
        TRI_SETF(_oow); \
      } \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
    else { \
      TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz); \
      dataElem++; \
      i = gc->tsuDataList[dataElem]; \
    } \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_OOW_FBI) { \
    if (gc->state.vData.fogInfo.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.fogInfo.offset)*_oow); \
    } \
    else if (gc->state.vData.qInfo.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.qInfo.offset)*_oow); \
    } else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU0) { \
    if (gc->state.vData.q0Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q0Info.offset)*_oow); \
    } \
    else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU0) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[0].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[0].t_scale); \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_W_TMU1) { \
    if (gc->state.vData.q1Info.mode == GR_PARAM_ENABLE) { \
      TRI_SETF(FARRAY(_s, gc->state.vData.q1Info.offset)*_oow); \
    } \
    else { \
      TRI_SETF(_oow); \
    } \
    dataElem++; \
    i = gc->tsuDataList[dataElem]; \
  } \
  if (gc->state.paramIndex & STATE_REQUIRES_ST_TMU1) { \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[1].s_scale); \
    TRI_SETF_SCALE_ADVANCE(_s,_oow*gc->state.per_tmu[1].t_scale); \
  } \
}

#endif

#endif /* __FX_CMD_H__ */
