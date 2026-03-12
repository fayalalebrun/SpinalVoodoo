/*
 * GPIO code
 */

#include <unistd.h> /*usleep*/

#include "minihwc.h"
#ifdef __WATCOMC__
#include <i86.h>
#define usleep(x) delay((x) / 1000)
#endif
#include "h3cinitdd.h"


#define USEC (1000)
#define DTIME (5*USEC)
#define MSEC (1000000)
#define D60HZ (166*MSEC)
#define DELAY(x) usleep(x)
#define CLOCK_STRETCH (1000)
#define NUM_RETRY (5)

#define BUILD_BYTE(data, mask, position) ((unsigned char)((data & mask)<<position))

#define ABS(A, B) ((A) < (B) ? (B) - (A) : (A) - (B))
#define MHz(A) ((A) * 1000000)
#define kHz(A) ((A) * 1000)

typedef struct {
        unsigned long dwBus;    /* Bus */
        unsigned long dwDevFunc;/* DevNode */
        unsigned long RN;       /* Register Number */
        unsigned long dInMask;  /* Data Mask */
        unsigned long dOutMask; /* Data Mask */
        unsigned long dInShift; /* Data Shift */
        unsigned long dOutShift;/* Data Shift */
} PCI_BIT;

typedef struct {
        PCI_BIT Data;
        PCI_BIT Clk;
        PCI_BIT Strobe;
        PCI_BIT HiVoltage;
} GPIOMASK;



static unsigned long pci_read_cfg (unsigned long bus,
                                   unsigned long dwDevFunc,
                                   unsigned long offset)
{
  FxU32 data, result;

  PciRegister reg;
  reg.regAddress = offset;
  reg.sizeInBytes = 4;
  
  result = pciGetConfigData/*Raw*/(reg, ((dwDevFunc & 7) << 13) | (bus << 5) | (dwDevFunc & 0x1f), &data);
                                     /* --------function-------   ----bus---   ------device------ */

  return data;
}

static unsigned long pci_write_cfg (unsigned long bus,
                                    unsigned long dwDevFunc,
                                    unsigned long offset,
                                    unsigned long value)
{
  PciRegister reg;
  reg.regAddress = offset;
  reg.sizeInBytes = 4;

  return pciSetConfigData/*Raw*/(reg, ((dwDevFunc & 7) << 13) | (bus << 5) | (dwDevFunc & 0x1f), &value);
                                   /* --------function-------   ----bus---   ------device------ */
}



/* Desc: scan for GPIO
 *
 * In  : device+vendor ID, bus number to match
 * Out : 0 for failure
 *
 * Note:
 */
int gpio_find_device (unsigned long dwDevVenID,
                      unsigned long dwBus,
                      unsigned long *pdwBus,
                      unsigned long *pdwDevFunc)
{
 for (*pdwBus = 0; *pdwBus < 256; *pdwBus += 1) {
     for (*pdwDevFunc = 0; *pdwDevFunc < 0x100; *pdwDevFunc += 0x08) {
         if ((dwDevVenID == pci_read_cfg(*pdwBus, *pdwDevFunc, 0x0)) &&
             (dwBus == (pci_read_cfg(*pdwBus, *pdwDevFunc, 0x19) & 0xFF))) {
            return 1;
         }
     }
 }

 return 0;
}



/* Desc: init GPIO port
 *
 * In  :
 * Out :
 *
 * Note: 0 for failure
 */
int gpio_init (GPIOMASK *pGPIOMask, unsigned long dwNapalmBus)
{
 static int isGPIOInit = 0;

 unsigned long dwBus;
 unsigned long dwDevFunc;
 unsigned long dwData;
   
 if (0 == isGPIOInit) {
    if (gpio_find_device(0x00213388, dwNapalmBus, &dwBus, &dwDevFunc)) {
       isGPIOInit = 1;
       pGPIOMask->Data.dwBus = pGPIOMask->Clk.dwBus = pGPIOMask->Strobe.dwBus = dwBus;
       pGPIOMask->Data.dwDevFunc = pGPIOMask->Clk.dwDevFunc = pGPIOMask->Strobe.dwDevFunc = dwDevFunc;

       pGPIOMask->Data.RN = 0xC4;
       pGPIOMask->Data.dInMask = 0x0100;
       pGPIOMask->Data.dInShift = 0x08;
       pGPIOMask->Data.dOutMask = 0x0400;
       pGPIOMask->Data.dOutShift = 0x0A;

       pGPIOMask->Clk.RN = 0xC4;
       pGPIOMask->Clk.dInMask = 0x010000;
       pGPIOMask->Clk.dInShift = 0x10;
       pGPIOMask->Clk.dOutMask = 0x040000;
       pGPIOMask->Clk.dOutShift = 0x12;

       pGPIOMask->Strobe.RN = 0xC4;
       pGPIOMask->Strobe.dInMask = 0x1000;
       pGPIOMask->Strobe.dInShift = 0x0C;
       pGPIOMask->Strobe.dOutMask = 0x4000;
       pGPIOMask->Strobe.dOutShift = 0x0E;

       pGPIOMask->HiVoltage.RN = 0xC4;
       pGPIOMask->HiVoltage.dInMask = 0x100000;
       pGPIOMask->HiVoltage.dInShift = 0x14;
       pGPIOMask->HiVoltage.dOutMask = 0x400000;
       pGPIOMask->HiVoltage.dOutShift = 0x16;

       /* Enable all GPIO as Output */
       dwData = pci_read_cfg(dwBus, dwDevFunc, 0xC4);
       dwData &= 0xFF0000FF;
       /*          C7C6C5C4 */
       dwData |= 0x00222200;
       pci_write_cfg(dwBus, dwDevFunc, 0xC4, dwData);
    }
 } else {
    dwData = pci_read_cfg(pGPIOMask->Data.dwBus, pGPIOMask->Data.dwDevFunc, 0xC4);
    dwData &= 0xFF0000FF;
    /*          C7C6C5C4 */
    dwData |= 0x00222200;
    pci_write_cfg(pGPIOMask->Data.dwBus, pGPIOMask->Data.dwDevFunc, 0xC4, dwData);
 }

 return isGPIOInit;
}



/* Desc: read a dword
 *
 * In  :
 * Out :
 *
 * Note: dword
 */
unsigned long gpio_get (PCI_BIT *pPCI_BIT)
{
 return pci_read_cfg(pPCI_BIT->dwBus, pPCI_BIT->dwDevFunc, pPCI_BIT->RN);
}



/* Desc: read a bit
 *
 * In  :
 * Out : bit
 *
 * Note:
 */
unsigned long gpio_get_bit (PCI_BIT *pPCI_BIT)
{
 unsigned long bData;
 bData = pci_read_cfg(pPCI_BIT->dwBus, pPCI_BIT->dwDevFunc, pPCI_BIT->RN);
 return (bData & pPCI_BIT->dInMask) >> pPCI_BIT->dInShift;
}



/* Desc: write a bit
 *
 * In  :
 * Out :
 *
 * Note:
 */
unsigned long gpio_set_bit (PCI_BIT *pPCI_BIT, unsigned long dwValue)
{
 unsigned long dwReg;

 dwReg = gpio_get(pPCI_BIT);
 dwReg = dwReg & ~pPCI_BIT->dOutMask;
 dwReg |= ((dwValue & 0x01) << pPCI_BIT->dOutShift);

 return pci_write_cfg(pPCI_BIT->dwBus, pPCI_BIT->dwDevFunc, pPCI_BIT->RN, dwReg);
}



/* Desc: read the clock
 *
 * In  :
 * Out :
 *
 * Note:
 */
int gpio_read_scl (GPIOMASK *pGPIOMask)
{
 return gpio_get_bit(&pGPIOMask->Clk);
}



/* Desc: set the Clock to bit
 *
 * In  :
 * Out :
 *
 * Note:
 */
void gpio_scl (GPIOMASK *pGPIOMask, unsigned long bit)
{
 int nCount = 0;

 DELAY(DTIME);
 gpio_set_bit(&pGPIOMask->Clk, bit);
 DELAY(DTIME);

 if (bit) {
    while (!gpio_read_scl(pGPIOMask) && (nCount++ < CLOCK_STRETCH)) {
          DELAY(DTIME);
    }

    if (nCount++ >= CLOCK_STRETCH) {
       /*fprintf(stderr, "Slave did not respond %s %d\n", __FILE__, __LINE__);*/
    }
 }
}



/* Desc: set the Data to bit.
 *
 * In  :
 * Out :
 *
 * Note:
 */
void gpio_sda (GPIOMASK *pGPIOMask, unsigned long bit)
{
 DELAY(DTIME);
 bit = bit >> 7;
 gpio_set_bit(&pGPIOMask->Data, bit);
 DELAY(DTIME);
}



/* Desc: send the byte data
 *
 * In  :
 * Out :
 *
 * Note:
 */
int gpio_send_byte (GPIOMASK *pGPIOMask,
                    unsigned char b1,
                    unsigned char b2,
                    unsigned char b3)
{
 int i;
 int nReturn = 0;

 for (i = 0; i < 8; i++) {
     gpio_sda(pGPIOMask, (b1 << i) & 0x80);
     gpio_scl(pGPIOMask, 1);
     if (gpio_read_scl(pGPIOMask) == 0) {
        /*fprintf(stderr, "nc\n");*/
     }
     gpio_scl(pGPIOMask, 0);
 }

 for (i = 0; i < 8; i++) {
     gpio_sda(pGPIOMask, (b2 << i) & 0x80);
     gpio_scl(pGPIOMask, 1);
     if (gpio_read_scl(pGPIOMask) == 0) {
        /*fprintf(stderr, "nc\n");*/
     }
     gpio_scl(pGPIOMask, 0);
 }

 for (i = 0; i < 8; i++) {
     gpio_sda(pGPIOMask, (b3 << i) & 0x80);
     gpio_scl(pGPIOMask, 1);
     if (gpio_read_scl(pGPIOMask) == 0) {
        /*fprintf(stderr, "nc\n");*/
     }
     gpio_scl(pGPIOMask, 0);
 }

 /* Strobe to make it happen */
 DELAY(DTIME);
 gpio_set_bit(&pGPIOMask->Strobe, 1);
 DELAY(DTIME);

 DELAY(DTIME);
 gpio_set_bit(&pGPIOMask->Strobe, 0);
 DELAY(DTIME);

 return nReturn;
}



/* Desc: build and send clock
 *
 * In  :
 * Out :
 *
 * Note:
 */
unsigned long gpio_output_clock (hwcBoardInfo *bInfo,
                                 unsigned long c,
                                 unsigned long ttl,
                                 unsigned long f,
                                 unsigned long s,
                                 unsigned long v,
                                 unsigned long r)
{
 static GPIOMASK GpioMask;

 unsigned char b1, b2, b3;

 b1 = BUILD_BYTE(c, 3, 6);
 b1 |= BUILD_BYTE(ttl, 1, 5);
 b1 |= BUILD_BYTE(f, 3, 3);
 b1 |= BUILD_BYTE(s, 7, 0);

 b2 = (unsigned char)((v & 0x1Fe) >> 1);

 b3 = BUILD_BYTE(v, 1, 7);
 b3 |= BUILD_BYTE(r, 0x7F, 0);

 /* Use the bus number embedded at deviceNum[5:12]. */
 if (gpio_init(&GpioMask, (bInfo->deviceNum >> 5) & 0xFF))
    gpio_send_byte(&GpioMask, b1, b2, b3);

 return 0;
}



/* Desc:
 *
 * In  :
 * Out : 0 for failure
 *
 * Note:
 */
unsigned long gpio_6k_clock (hwcBoardInfo *bInfo)
{
 static unsigned long OD[] = {2, 3, 4, 5, 6, 7, 8, 10};
 static unsigned long S2S1S0[] = {15, 15, 1, 6, 3, 4, 7, 5, 2, 15, 0};

 double d_clk1;
 double clk1;
 double d_diff;
 double d_partial;
 unsigned long rdw;
 unsigned long vdw;
 unsigned long od;
 unsigned long i;
 unsigned long b_rdw;
 unsigned long b_vdw;
 unsigned long b_od;
 unsigned long b_i;
 unsigned long bFound;
 unsigned long ic;
 unsigned long pixelclock;
 unsigned long n;   
 unsigned long m;   
 unsigned long k;
 unsigned long kpow;   

 /* Get the Clock */
 FxU32 regBase = bInfo->regInfo.ioPortBase;
 pixelclock = IGET32(pllCtrl0);
 n = ((pixelclock & 0xFF00) >> 8)+2;
 m = ((pixelclock & 0xFC) >> 2)+2;
 k = pixelclock & 0x03;

 kpow = 1;
 for (i = 0; i < k; i++) {
     kpow <<= 1;
 }

 /* This should work to 4 GigaHertz */
 ic = 14318180 * n / m / kpow;
 ic >>= 2;

 d_diff = MHz(500.0);
 i = 0;
 bFound = 0;
 for (i = 0; i < sizeof(OD)/sizeof(unsigned long); i++) {
     od = OD[i];
     for (rdw = 1; rdw < 128; rdw++) {
         for (vdw = 4; vdw < 512; vdw++) {
             clk1 = 14318180.0*2.0*((double)vdw+8.0)/(((double)rdw+2.0)*(double)od);
             if (ABS(clk1, (double)ic) < d_diff) {
                /* Some constraints on vdw & rdw
                 * 55 Mhz < 14318180 * 2* (vdw+8)/(rdw + 2) < 400 Mhz
                 */
                d_partial = 14318180.0*2.0*((double)vdw+8.0)/((double)rdw+2.0);
                if ((d_partial > MHz(55)) && (d_partial < MHz(400))) {
                   /* Another constraints
                    * 200 kHz < 14318180/(rdw+2)
                    */
                   if (14318180/(rdw+2) > kHz(200)) {
                      d_diff = ABS(clk1, (double)ic);
                      bFound = 1;
                      b_rdw = rdw;
                      b_vdw = vdw;
                      b_od = od;
                      b_i = i;
                      d_clk1 = clk1;
                   }
                }
             }
         }
     }
 }

 if (bFound) {
    gpio_output_clock(bInfo, 0x0, 0x1, 0x0, S2S1S0[b_od], b_vdw, b_rdw);
    return ic;
 } else {
    return 0;
 }
}
