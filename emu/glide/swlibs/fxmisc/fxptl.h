//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define FILE_DEVICE_MAPMEM  0x00008000



//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define MAPMEM_IOCTL_INDEX  0x800


//
// Define our own private IOCTL
//

#ifndef NT_INCLUDED
/* xxx The following two typedefs are here to avoid including
 * miniport.h and ntddk.h from the NT DDK from user code because
 * its not always in the build environment.
 */
typedef enum 
{
  Internal,
  Isa,
  Eisa,
  MicroChannel,
  TurboChannel,
  PCIBus,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;
typedef LARGE_INTEGER PHYSICAL_ADDRESS;
#endif /* !NT_INCLUDED */

// Our user mode app will pass an initialized structure like this
// down to the kernel mode driver to map memory.
typedef struct {
  INTERFACE_TYPE   InterfaceType; // Isa, Eisa, etc....
  ULONG            BusNumber;     // Bus number
  PHYSICAL_ADDRESS BusAddress;    // Bus-relative address
  ULONG            AddressSpace;  // 0 is memory, 1 is I/O
  ULONG            Length;        // Length of section to map
} PHYSICAL_MEMORY_INFO, *PPHYSICAL_MEMORY_INFO;

#define IOCTL_MAPMEM_MAP_USER_PHYSICAL_MEMORY   CTL_CODE(FILE_DEVICE_MAPMEM ,    \
                                                         MAPMEM_IOCTL_INDEX + 0, \
                                                         METHOD_BUFFERED,        \
                                                         FILE_ANY_ACCESS)

/* Passes the linear address to unmap */
#define IOCTL_MAPMEM_UNMAP_USER_PHYSICAL_MEMORY CTL_CODE(FILE_DEVICE_MAPMEM,     \
                                                         MAPMEM_IOCTL_INDEX + 1, \
                                                         METHOD_BUFFERED,        \
                                                         FILE_ANY_ACCESS)

/* Stuff for programming MSRs */
#define IOCTL_MAPMEM_GET_MSR                    CTL_CODE(FILE_DEVICE_MAPMEM,     \
                                                         MAPMEM_IOCTL_INDEX + 2, \
                                                         METHOD_BUFFERED,        \
                                                         FILE_ANY_ACCESS)
#define IOCTL_MAPMEM_SET_MSR                    CTL_CODE(FILE_DEVICE_MAPMEM,     \
                                                         MAPMEM_IOCTL_INDEX + 3, \
                                                         METHOD_BUFFERED,        \
                                                         FILE_ANY_ACCESS)
