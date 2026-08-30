#include <cstdint>
#include <cstdio>
#include <deque>

#include "VXlsDe10Top.h"
#include "verilated.h"

struct BurstObserver {
  bool active = false;
  bool valid = true;
  uint32_t address = 0;
  uint32_t count = 0;
  uint32_t beats = 0;
  uint64_t bursts = 0;
  uint64_t multibeat = 0;

  void sample(bool write, uint32_t nextAddress, uint32_t nextCount) {
    if (!write) {
      if (active) valid = false;
      return;
    }
    if (!active) {
      active = true;
      address = nextAddress;
      count = nextCount;
      beats = 0;
      if (count == 0) valid = false;
    } else if (address != nextAddress || count != nextCount) {
      valid = false;
    }
    ++beats;
    if (beats == count) {
      active = false;
      ++bursts;
      if (count > 1) ++multibeat;
    } else if (beats > count) {
      valid = false;
    }
  }
};

struct ReadBurst {
  uint32_t address = 0;
  uint32_t count = 0;
  uint32_t beat = 0;
};

struct ReadResponder {
  bool valid = true;
  bool driven = false;
  std::deque<ReadBurst> pending;
  uint64_t bursts = 0;
  uint64_t lineBursts = 0;

  void capture(bool read, uint32_t nextAddress, uint32_t nextCount) {
    if (!read) return;
    if (nextCount == 0) valid = false;
    pending.push_back(ReadBurst{nextAddress, nextCount, 0});
    ++bursts;
    if (nextCount > 1) ++lineBursts;
  }

  uint32_t data(uint32_t salt) const {
    if (pending.empty()) return 0;
    const ReadBurst &burst = pending.front();
    return salt ^ ((burst.address - 0x3f400000) >> 2) ^ burst.beat;
  }

  void advance() {
    if (!driven) return;
    ReadBurst &burst = pending.front();
    --burst.count;
    ++burst.beat;
    if (burst.count == 0) pending.pop_front();
    driven = false;
  }
};

static BurstObserver colorWrites;
static BurstObserver auxWrites;
static ReadResponder colorReads;
static ReadResponder auxReads;

static void prepareMemory(VXlsDe10Top &dut) {
  colorReads.driven = !colorReads.pending.empty();
  auxReads.driven = !auxReads.pending.empty();
  dut.io_memFbColorRead_readDataValid = colorReads.driven;
  dut.io_memFbAuxRead_readDataValid = auxReads.driven;
  dut.io_memFbColorRead_readData = colorReads.data(0xc0100000);
  dut.io_memFbAuxRead_readData = auxReads.data(0xa0100000);
}

static void sampleMemory(VXlsDe10Top &dut) {
  colorWrites.sample(dut.io_memFbColorWrite_write, dut.io_memFbColorWrite_address,
                     dut.io_memFbColorWrite_burstCount);
  auxWrites.sample(dut.io_memFbAuxWrite_write, dut.io_memFbAuxWrite_address,
                   dut.io_memFbAuxWrite_burstCount);
  colorReads.capture(dut.io_memFbColorRead_read, dut.io_memFbColorRead_address,
                     dut.io_memFbColorRead_burstCount);
  auxReads.capture(dut.io_memFbAuxRead_read, dut.io_memFbAuxRead_address,
                   dut.io_memFbAuxRead_burstCount);
}

static void advanceMemory() {
  colorReads.advance();
  auxReads.advance();
}

static void setInputs(VXlsDe10Top &dut) {
  dut.io_memFbWrite_waitRequestn = 1;
  dut.io_memFbColorWrite_waitRequestn = 1;
  dut.io_memFbAuxWrite_waitRequestn = 1;
  dut.io_memFbColorRead_waitRequestn = 1;
  dut.io_memFbAuxRead_waitRequestn = 1;
  dut.io_memTex_waitRequestn = 1;
  dut.io_memScanout_waitRequestn = 1;
  dut.io_memFbWrite_readDataValid = 0;
  dut.io_memFbColorWrite_readDataValid = 0;
  dut.io_memFbAuxWrite_readDataValid = 0;
  dut.io_memFbColorRead_readDataValid = 0;
  dut.io_memFbAuxRead_readDataValid = 0;
  dut.io_memTex_readDataValid = 0;
  dut.io_memScanout_readDataValid = 0;
  dut.io_hdmi_clock = 0;
  dut.io_hdmi_reset = 1;
}

static void tick(VXlsDe10Top &dut) {
  prepareMemory(dut);
  dut.clk = 0;
  dut.eval();
  sampleMemory(dut);
  dut.clk = 1;
  dut.eval();
  advanceMemory();
}

static bool writeMmio(VXlsDe10Top &dut, uint32_t address, uint32_t data) {
  dut.io_h2fLw_address = address >> 2;
  dut.io_h2fLw_writedata = data;
  dut.io_h2fLw_write = 1;
  for (int i = 0; i < 200000; ++i) {
    prepareMemory(dut);
    dut.clk = 0;
    dut.eval();
    sampleMemory(dut);
    const bool accepted = !dut.io_h2fLw_waitrequest;
    dut.clk = 1;
    dut.eval();
    advanceMemory();
    if (accepted) {
      dut.io_h2fLw_write = 0;
      return true;
    }
  }
  return false;
}

static bool readMmio(VXlsDe10Top &dut, uint32_t address, uint32_t *data) {
  dut.io_h2fLw_address = address >> 2;
  dut.io_h2fLw_read = 1;
  bool accepted = false;
  for (int i = 0; i < 200000; ++i) {
    prepareMemory(dut);
    dut.clk = 0;
    dut.eval();
    sampleMemory(dut);
    if (!accepted && !dut.io_h2fLw_waitrequest) accepted = true;
    dut.clk = 1;
    dut.eval();
    advanceMemory();
    if (accepted) dut.io_h2fLw_read = 0;
    if (dut.io_h2fLw_readdatavalid) {
      *data = dut.io_h2fLw_readdata;
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  VXlsDe10Top dut;
  setInputs(dut);
  dut.io_h2fLw_read = 0;
  dut.io_h2fLw_write = 0;
  dut.io_h2fLw_byteenable = 0xf;
  dut.reset = 1;
  for (int i = 0; i < 8; ++i) tick(dut);
  dut.reset = 0;
  for (int i = 0; i < 8; ++i) tick(dut);

  uint32_t status = 0;
  uint32_t cyclesLow = 0;
  uint32_t pixelsIn = 0;
  uint32_t pixelsOut = 0;
  uint32_t profileVersion = 0;
  uint32_t readStalls = 0;
  bool ok = writeMmio(dut, 0x300, 0x00000a00) && readMmio(dut, 0x000, &status) &&
            readMmio(dut, 0x3d0, &cyclesLow) && cyclesLow == 0 &&
            writeMmio(dut, 0x120, 0) && writeMmio(dut, 0x3cc, 1) &&
            writeMmio(dut, 0x008, 8) && writeMmio(dut, 0x00c, 8) &&
            writeMmio(dut, 0x010, 56) && writeMmio(dut, 0x014, 8) &&
            writeMmio(dut, 0x018, 8) && writeMmio(dut, 0x01c, 56) &&
            writeMmio(dut, 0x110, 0x806f0) && writeMmio(dut, 0x080, 0) &&
            readMmio(dut, 0x3d0, &cyclesLow) && readMmio(dut, 0x3d8, &pixelsIn) &&
            readMmio(dut, 0x3e0, &pixelsOut) &&
             readMmio(dut, 0x100000, &profileVersion) &&
             readMmio(dut, 0x100004, &readStalls);
   ok = ok && status == 0x0ffff03f && cyclesLow != 0 && pixelsIn != 0 &&
            pixelsOut == pixelsIn && profileVersion == 0x00140000;
  ok = ok && writeMmio(dut, 0x120, 0) && readMmio(dut, 0x14c, &pixelsIn) && pixelsIn == 0 &&
       writeMmio(dut, 0x3cc, 1) && readMmio(dut, 0x100004, &readStalls) && readStalls == 0 &&
       readMmio(dut, 0x3d0, &cyclesLow) &&
       readMmio(dut, 0x3d8, &pixelsIn) && cyclesLow == 0 && pixelsIn == 0;
  ok = ok && writeMmio(dut, 0x010, 8) && writeMmio(dut, 0x01c, 8) &&
       writeMmio(dut, 0x080, 0) && readMmio(dut, 0x3d0, &cyclesLow) &&
       readMmio(dut, 0x3d8, &pixelsIn) && cyclesLow != 0 && pixelsIn == 0 &&
       writeMmio(dut, 0x3cc, 1);
   ok = ok && colorWrites.valid && auxWrites.valid && colorReads.valid && auxReads.valid &&
        !colorWrites.active && !auxWrites.active && colorWrites.bursts != 0 &&
        auxWrites.bursts != 0 && colorReads.bursts != 0 && auxReads.bursts != 0 &&
       colorReads.bursts < pixelsOut &&
       auxReads.bursts < pixelsOut && colorReads.lineBursts != 0 && auxReads.lineBursts != 0;
  dut.final();
  if (!ok) {
    std::fprintf(stderr, "status=%08x cycles=%u pixels=%u/%u profile=%08x read-stalls=%u color=%llu/%llu/%llu/%llu aux=%llu/%llu/%llu/%llu valid=%d/%d/%d/%d active=%d/%d\n",
                 status, cyclesLow, pixelsIn, pixelsOut, profileVersion, readStalls,
                 static_cast<unsigned long long>(colorWrites.bursts),
                 static_cast<unsigned long long>(colorWrites.multibeat),
                 static_cast<unsigned long long>(colorReads.bursts),
                 static_cast<unsigned long long>(colorReads.lineBursts),
                 static_cast<unsigned long long>(auxWrites.bursts),
                 static_cast<unsigned long long>(auxWrites.multibeat),
                 static_cast<unsigned long long>(auxReads.bursts),
                 static_cast<unsigned long long>(auxReads.lineBursts), colorWrites.valid,
                 auxWrites.valid, colorReads.valid, auxReads.valid, colorWrites.active,
                 auxWrites.active);
    return 1;
  }
  std::puts("XlsDe10TopMmio PASS");
  return 0;
}
