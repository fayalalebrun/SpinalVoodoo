#ifndef _FXSIM_H_
#define _FXSIM_H_

#include <3dfx.h>

/* Stubs for Linux-specific functions (defined in fxsim.c) */
FxBool pciPlatformInit(void);
FxBool hasDev3DfxLinux(void);
int getNumDevicesLinux(void);
FxU32 pciFetchRegisterLinux(FxU32 cmd, FxU32 size, FxU32 device);
int pciUpdateRegisterLinux(FxU32 cmd, FxU32 data, FxU32 size, FxU32 device);

/* Simulation IPC functions (called from SET/GET/SETF macros) */
void simWriteReg(volatile void *addr, FxU32 value);
FxU32 simReadReg(volatile void *addr);
void simWriteRegF(volatile void *addr, float value);

/* Bulk texture memory write */
void simWriteTex(FxU32 offset, const void *data, FxU32 length);

/* Wait for pipeline idle */
void simIdleWait(void);

/* Dump framebuffer to file */
void simDumpFB(const char *filename, FxU32 offset, FxU32 width, FxU32 height);

/* Set the base pointer for address offset calculation */
void simSetBasePtr(void *ptr);

/* Get the simulated base pointer */
void *simGetBasePtr(void);

#endif /* _FXSIM_H_ */
