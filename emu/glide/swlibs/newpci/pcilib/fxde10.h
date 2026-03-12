#ifndef _FXDE10_H_
#define _FXDE10_H_

#include <3dfx.h>

FxBool pciPlatformInit(void);
FxBool hasDev3DfxLinux(void);
int getNumDevicesLinux(void);
FxU32 pciFetchRegisterLinux(FxU32 cmd, FxU32 size, FxU32 device);
int pciUpdateRegisterLinux(FxU32 cmd, FxU32 data, FxU32 size, FxU32 device);

#endif
