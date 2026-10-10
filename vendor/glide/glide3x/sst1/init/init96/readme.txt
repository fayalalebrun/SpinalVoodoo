Different Build Environments For VG96:

Lab:
FX_GLIDE_HW=sst96
FX_GLIDE_CTRISETUP=1
FX_GLIDE_NO_SPLASH=1
INIT_ACCESS_DDRAW=


Direct Draw Window:
FX_GLIDE_HW=sst96
FX_GLIDE_CTRISETUP=1
FX_GLIDE_NO_SPLASH=1
INIT_ACCESS_DDRAW=1
SST_VG_MODE=WIN
*demos* ARCADE_RES_OVERRIDE=1

Direct Draw Fullscreen:
FX_GLIDE_HW=sst96
FX_GLIDE_CTRISETUP=1
FX_GLIDE_NO_SPLASH=1
INIT_ACCESS_DDRAW=1
SST_VG_MODE=
*demos* ARCADE_RES_OVERRIDE=

This mode sucks because you can't get any input to your application
and it is slow because you have to do sucky synchronized swapping.


DOS Fullscreen:
FX_GLIDE_HW=sst96
FX_GLIDE_CTRISETUP=1
FX_GLIDE_NO_SPLASH=1
*demos* ARCADE_RES_OVERRIDE=
INIT_ACCESS_DDRAW=
SST_VG_MODE=

