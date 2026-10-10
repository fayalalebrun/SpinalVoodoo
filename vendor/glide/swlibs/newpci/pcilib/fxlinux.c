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
#include <stdio.h>
#include <3dfx.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <sys/io.h>
#endif
#ifdef __FreeBSD__
#include <machine/cpufunc.h>
#endif
#include <fcntl.h>
#include "fxpci.h"
#include "pcilib.h"

static const char* pciIdentifyLinux(void);
static FxBool pciOutputStringLinux(const char *msg);
static FxBool pciInitializeLinux(void);
static FxBool pciShutdownLinux(void);
static FxBool pciMapLinearLinux(FxU32, FxU32 physical_addr, unsigned long *linear_addr,
				FxU32 *length);
static FxBool pciUnmapLinearLinux(unsigned long linear_addr, FxU32 length);
static FxBool pciSetPermissionLinux(const unsigned long, const FxU32, const FxBool);
static FxU8 pciPortInByteLinux(unsigned short port);
static FxU16 pciPortInWordLinux(unsigned short port);
static FxU32 pciPortInLongLinux(unsigned short port);
static FxBool pciPortOutByteLinux(unsigned short port, FxU8 data);
static FxBool pciPortOutWordLinux(unsigned short port, FxU16 data);
static FxBool pciPortOutLongLinux(unsigned short port, FxU32 data);
static FxBool pciMsrGetLinux(MSRInfo *, MSRInfo *);
static FxBool pciMsrSetLinux(MSRInfo *, MSRInfo *);
static FxBool pciSetPassThroughBaseLinux(FxU32 *, FxU32);

const FxPlatformIOProcs ioProcsLinux = {
  pciInitializeLinux,
  pciShutdownLinux,
  pciIdentifyLinux,

  pciPortInByteLinux,
  pciPortInWordLinux,
  pciPortInLongLinux,

  pciPortOutByteLinux,
  pciPortOutWordLinux,
  pciPortOutLongLinux,

  pciMapLinearLinux,
  pciUnmapLinearLinux,
  pciSetPermissionLinux,

  pciMsrGetLinux,
  pciMsrSetLinux,

  pciOutputStringLinux,
  pciSetPassThroughBaseLinux
};

#if 0 /* unused */
static const FxU32 PCI_VENDOR_ID_LINUX = 0x0;
static const FxU32 PCI_DEVICE_ID_LINUX = 0x2;
static const FxU32 PCI_COMMAND_LINUX = 0x4;
static const FxU32 PCI_BASE_ADDRESS_0_LINUX = 0x10;
static const FxU32 SST1_PCI_INIT_ENABLE_LINUX = 0x40;
static const FxU32 SST1_PCI_BUS_SNOOP0_LINUX = 0x44;
static const FxU32 SST1_PCI_BUS_SNOOP1_LINUX = 0x48;
#endif

static int linuxDevFd=-1;
#ifdef __FreeBSD__
static int freebsdIoFd=-1;
#endif

struct pioData {
  short port;
  short size;
  int device;
  void *value;
};

FxBool
pciPlatformInit(void)
{
  gCurPlatformIO = &ioProcsLinux;
  return FXTRUE;
}

FxBool 
hasDev3DfxLinux(void) 
{
  if (linuxDevFd==-1) return FXFALSE;
  return FXTRUE;
}

FxU32
pciFetchRegisterLinux( FxU32 cmd, FxU32 size, FxU32 device) 
{
  struct pioData desc;
  char cval;
  short sval;
  int ival;

  if (linuxDevFd==-1) return -1;
  desc.port=cmd;
  desc.size=size;
  desc.device=device;
  switch (size) {
  case 1:
    desc.value=&cval;
    break;
  case 2:
    desc.value=&sval;
    break;
  case 4:
    desc.value=&ival;
    break;
  default:
    return 0;
  }
  if (ioctl(linuxDevFd, _IOR('3', 3, struct pioData), &desc)==-1)
    return 0;
  switch (size) {
  case 1:
    return cval;
  case 2:
    return sval;
  case 4:
    return ival;
  default:
    return 0;
  }
}

FxBool
pciUpdateRegisterLinux(FxU32 cmd, FxU32 data, FxU32 size, FxU32 device) 
{
  struct pioData desc;

  if (linuxDevFd==-1) return -1;
  desc.port=cmd;
  desc.size=size;
  desc.device=device;
  desc.value=&data;
  if (ioctl(linuxDevFd, _IOW('3', 4, struct pioData), &desc)==-1)
    return FXFALSE;
  return FXTRUE;
}

int 
getNumDevicesLinux(void) 
{
  if (linuxDevFd==-1) return -1;
  return ioctl(linuxDevFd, _IO('3', 2));
}

static const char*
pciIdentifyLinux(void)
{
  return "fxPCI for Linux";
}

static FxBool 
pciOutputStringLinux(const char *msg) 
{
  printf("%s", msg);
  return FXTRUE;
}

static FxBool 
pciInitializeLinux(void)
{
  if (!getenv("SST_NO_DEV3DFX"))
    linuxDevFd=open("/dev/3dfx", O_RDWR, 0);
  if (linuxDevFd==-1) {
#ifdef __FreeBSD__
    freebsdIoFd=open("/dev/io", O_RDWR, 0);
    if (freebsdIoFd<0) {
#else
    if (iopl(3)<0) {
#endif
      pciErrorCode = PCI_ERR_NO_IO_PERM;
      return FXFALSE;
    }
  }
  return FXTRUE;
}

static FxBool 
pciShutdownLinux(void)
{
  if (linuxDevFd>=0) {
    close(linuxDevFd);
  } else {
#ifdef __FreeBSD__
    if (freebsdIoFd>=0)
      close(freebsdIoFd);
#else
    iopl(0);
#endif
  }
  return FXTRUE;
}

static FxBool 
pciMapLinearLinux(FxU32 bus, FxU32 physical_addr,
		  unsigned long *linear_addr, FxU32 *length) 
{
  int fd;
  if (linuxDevFd!=-1) {
    fd=linuxDevFd;
  } else {
    if ((fd=open("/dev/mem", O_RDWR))<0) {
      pciErrorCode=PCI_ERR_NO_MEM_PERM;
      return FXFALSE;
    }
  }
  if (((*linear_addr)=(unsigned long)mmap(0, *length, PROT_READ|PROT_WRITE,
				  MAP_SHARED, fd, physical_addr)) ==
       (unsigned long)MAP_FAILED)
  {
    if (fd!=linuxDevFd) close(fd);
    return FXFALSE;
  }
  if (fd!=linuxDevFd) close(fd);
  return FXTRUE;
}

static FxBool
pciUnmapLinearLinux(unsigned long linear_addr, FxU32 length) 
{
  munmap((void *) linear_addr, length);
  return FXTRUE;
}

static FxBool
pciSetPermissionLinux(const unsigned long addrBase, const FxU32 addrLen, 
		      const FxBool writePermP)
{
  return FXTRUE;
}

static FxU8 
pciPortInByteLinux(unsigned short port) 
{
  char tmp;
  struct pioData desc;

  if (linuxDevFd==-1) {
    tmp=inb(port);
    /* fprintf(stderr, "Read byte at %x got %d\n", port, tmp); */
    return tmp;
  }
  desc.port=port;
  desc.size=sizeof(tmp);
  desc.value=&tmp;
  /* fprintf(stderr, "Read byte desc at %x tmp at %x\n", &desc, &tmp); */
  ioctl(linuxDevFd, _IOR(0, 0, struct pioData), &desc);
  /* fprintf(stderr, "Got byte %d versus %d\n", tmp, inb(port)); */
  return tmp;
}

static FxU16 
pciPortInWordLinux(unsigned short port) 
{
  short tmp;
  struct pioData desc;

  if (linuxDevFd==-1) {
    tmp=inw(port);
    /* fprintf(stderr, "Read word at %x got %x\n", port, tmp); */
    return tmp;
  }
  desc.port=port;
  desc.size=sizeof(tmp);
  desc.value=&tmp;
  /* fprintf(stderr, "Read word desc at %x tmp at %x\n", &desc, &tmp); */
  ioctl(linuxDevFd, _IOR(0, 0, struct pioData), &desc);
  /* fprintf(stderr, "Got word %d versus %d\n", tmp, inw(port)); */
  return tmp;
}

static FxU32 
pciPortInLongLinux(unsigned short port) 
{
  int tmp;
  struct pioData desc;

  if (linuxDevFd==-1) {
    tmp=inl(port);
    /* fprintf(stderr, "Read long at %x got %x\n", port, tmp); */
    return tmp;
  }
  desc.port=port;
  desc.size=sizeof(tmp);
  desc.value=&tmp;
  /* fprintf(stderr, "Read long desc at %x tmp at %x\n", &desc, &tmp); */
  ioctl(linuxDevFd, _IOR(0, 0, struct pioData), &desc);
  /* fprintf(stderr, "Got long %x versus %x\n", tmp, inl(port)); */
  return tmp;
}

static FxBool 
pciPortOutByteLinux(unsigned short port, FxU8 data) 
{
  struct pioData desc;
  /* fprintf(stderr, "write byte=%d desc at %x data at %x\n", data,
      &desc, &data); */
  if (linuxDevFd==-1) {
#ifdef __linux__
    outb(data, port);
#else
    outb(port, data);
#endif
    return FXTRUE;
  }
  desc.port=port;
  desc.size=sizeof(data);
  desc.value=&data;
  return ioctl(linuxDevFd, _IOW(0, 1, struct pioData), &desc)!=-1;
}

static FxBool 
pciPortOutWordLinux(unsigned short port, FxU16 data) 
{
  struct pioData desc;
  /* fprintf(stderr, "write word=%x to port=%x desc at %x data at %x\n", 
	  data, port, &desc, &data); */
  if (linuxDevFd==-1) {
#ifdef __linux__
    outw(data, port);
#else
    outw(port, data);
#endif
    return FXTRUE;
  }
  desc.port=port;
  desc.size=sizeof(data);
  desc.value=&data;
  return ioctl(linuxDevFd, _IOW(0, 1, struct pioData), &desc)!=-1;
}

static FxBool 
pciPortOutLongLinux(unsigned short port, FxU32 data) 
{
  struct pioData desc;
  /* fprintf(stderr, "write long=%x to port=%x desc at %x data at %x\n", 
	  data, port, &desc, &data); */
  if (linuxDevFd==-1) {
#ifdef __linux__
    outl(data, port);
#else
    outl(port, data);
#endif
    return FXTRUE;
  }
  desc.port=port;
  desc.size=sizeof(data);
  desc.value=&data;
  return ioctl(linuxDevFd, _IOW(0, 1, struct pioData), &desc)!=-1;
}

static FxBool
pciMsrGetLinux(MSRInfo *in, MSRInfo *out)
{
  return FXTRUE;
}

static FxBool
pciMsrSetLinux(MSRInfo *in, MSRInfo *out)
{
  return FXTRUE;
}

static FxBool
pciSetPassThroughBaseLinux(FxU32 *baseAddr, FxU32 baseAddrLen)
{
  return FXTRUE;
}
