#include <array>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "../../emu/mister/mister_runtime.h"
#include "../../emu/mister/mister_memory.h"
#ifdef MISTER_GLIDE_TEST
#include "glide.h"
#include "../../emu/mister/glide_guest.h"
#endif
#include "VMisterPlatform.h"
#include "verilated.h"

static void require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}

alignas(8) static uint32_t transportMemory[MISTER_MEMORY_BYTES / 4];

struct Simulation {
  static constexpr uint32_t host = 0x3e000000;
  VMisterPlatform dut;
  std::unordered_map<uint32_t, uint64_t> memory;
  std::deque<uint64_t> pending;
  std::mt19937 random{47};
  uint64_t cycles = 0;
  uint64_t writes = 0;
  uint64_t scanoutBursts = 0;
  bool protectRenderer = false;
  uint32_t sequence = 0;

  void tick() {
    ++cycles;
#ifdef MISTER_GLIDE_TEST
    if (cycles % 5000000 == 0)
      std::fprintf(stderr, "cycles=%llu writes=%llu seq=%u ack=%u op=%u address=%08x\n",
                   static_cast<unsigned long long>(cycles), static_cast<unsigned long long>(writes),
                   transportMemory[1], transportMemory[MISTER_MAILBOX_ACK / 4],
                   transportMemory[MISTER_MAILBOX_COMMANDS / 4], transportMemory[MISTER_MAILBOX_COMMANDS / 4 + 1]);
#endif
    dut.clk = 0;
    dut.io_hdmi_clock = (cycles >> 1) & 1;
    dut.io_vblank = cycles % 2000 < 40;
    dut.io_ddr_busy = random() % 4 == 0;
    dut.io_ddr_readDataValid = !pending.empty() && random() % 3 != 0;
    dut.io_ddr_readData = pending.empty() ? 0 : pending.front();
    dut.eval();
    const bool delivered = dut.io_ddr_readDataValid;
    if (!dut.reset && !dut.io_ddr_busy && (dut.io_ddr_read || dut.io_ddr_write)) {
      const uint32_t address = dut.io_ddr_address << 3;
      const uint32_t count = dut.io_ddr_burstCount;
      require((address >= host && uint64_t(address) + count * 8 <= host + 0x3000) ||
               (address >= 0x3e400000 && uint64_t(address) + count * 8 <= 0x3f000000) ||
               (address >= 0x3f400000 && uint64_t(address) + count * 8 <= 0x40000000),
              "DDR address outside reserved windows");
      require(count >= 1 && count <= 128, "invalid DDR burst count");
      require(!(dut.io_ddr_read && dut.io_ddr_write), "simultaneous DDR read/write");
      if (dut.io_ddr_read) {
        require(pending.size() + count <= 1024, "DDR response queue exceeded eight full bursts");
        if (address >= 0x3e400000 && address < 0x3f000000) {
          require(dut.io_frame_enable && address >= dut.io_frame_base &&
                  uint64_t(address) + count * 8 <= uint64_t(dut.io_frame_base) + 0x400000,
                  "scanout accessed a retired host slot");
          ++scanoutBursts;
        }
        if (dut.io_frame_enable && dut.io_frame_base >= MISTER_RENDERER_BASE &&
            address >= dut.io_frame_base && count > 1 &&
            uint64_t(address) + count * 8 <= uint64_t(dut.io_frame_base) +
              479 * dut.io_frame_stride + 1280) ++scanoutBursts;
        for (uint32_t i = 0; i < count; ++i) {
          const uint32_t word = address + i * 8;
          if ((word >= MISTER_FRAME_BASE && word < MISTER_FRAME_BASE + 3 * MISTER_FRAME_SLOT_BYTES) ||
              (word >= MISTER_RENDERER_BASE && word < MISTER_RENDERER_BASE + MISTER_RENDERER_BYTES)) {
            const unsigned offset = (word - host) / 4;
            pending.push_back(uint64_t(transportMemory[offset + 1]) << 32 | transportMemory[offset]);
          } else pending.push_back(memory[word]);
        }
      } else {
        require(count == 1, "multi-beat DDR write");
          if (protectRenderer && dut.io_frame_enable && dut.io_frame_base >= MISTER_RENDERER_BASE)
          require(address + 8 <= dut.io_frame_base ||
                  address >= uint64_t(dut.io_frame_base) + 479 * dut.io_frame_stride + 1280,
                   ("renderer wrote active buffer: address=" + std::to_string(address) +
                    " base=" + std::to_string(dut.io_frame_base) +
                    " cycle=" + std::to_string(cycles) +
                    " op=" + std::to_string(transportMemory[MISTER_MAILBOX_COMMANDS / 4]) +
                    " reg=" + std::to_string(transportMemory[MISTER_MAILBOX_COMMANDS / 4 + 1])).c_str());
        uint64_t mask = 0;
        for (unsigned i = 0; i < 8; ++i)
          if (dut.io_ddr_byteEnable & (1u << i)) mask |= uint64_t(255) << (i * 8);
        const bool renderer = address >= MISTER_RENDERER_BASE && address < MISTER_RENDERER_BASE + MISTER_RENDERER_BYTES;
        const unsigned offset = (address - host) / 4;
        const uint64_t previous = renderer ? uint64_t(transportMemory[offset + 1]) << 32 | transportMemory[offset] : memory[address];
        memory[address] = (previous & ~mask) | (dut.io_ddr_writeData & mask);
        if (renderer) {
          transportMemory[offset] = memory[address];
          transportMemory[offset + 1] = memory[address] >> 32;
        }
        if (address >= 0x3f400000) ++writes;
      }
    }
    dut.clk = 1;
    dut.eval();
    if (delivered) pending.pop_front();
  }

  void reset() {
    pending.clear();
    dut.io_ps2Key = 0;
    dut.io_osdOpen = 0;
    dut.reset = 1;
    dut.io_hdmi_reset = 1;
    for (int i = 0; i < 12; ++i) tick();
    dut.reset = 0;
    dut.io_hdmi_reset = 0;
    memory[host + 0x80] = ~uint64_t(0);
    for (int i = 0; i < 20000 && uint32_t(memory[host + 0x80]) != sequence; ++i) tick();
    require(uint32_t(memory[host + 0x80]) == sequence, "boot acknowledgment timeout");
    require((memory[host + 0x80] >> 32) == 4, "boot must not replay old commands");
  }

  void submit(const std::vector<std::array<uint32_t, 4>> &commands, uint8_t error = 0) {
    require(commands.size() <= 256, "too many host commands");
    memory[host + 8] = commands.size();
    for (size_t i = 0; i < commands.size(); ++i) {
      const auto &c = commands[i];
      memory[host + 0x100 + i * 16] = uint64_t(c[1]) << 32 | c[0];
      memory[host + 0x108 + i * 16] = uint64_t(c[3]) << 32 | c[2];
    }
    ++sequence;
    memory[host] = uint64_t(sequence) << 32 | 0x4d565331;
    for (int i = 0; i < 3 * 4 * 858 * 525 && uint32_t(memory[host + 0x80]) != sequence; ++i) tick();
    require(uint32_t(memory[host + 0x80]) == sequence, "mailbox acknowledgment timeout");
    require(((memory[host + 0x80] >> 32) & 255) == error, "mailbox error mismatch");
    if (!error) require((memory[host + 0x80] >> 40) == commands.size(), "incomplete command batch");
  }

  uint32_t result(unsigned index) {
    return memory[host + 0x2000 + (index / 2) * 8] >> ((index & 1) * 32);
  }
};

static Simulation *transportSimulation = nullptr;


static void readMailbox(Simulation &sim) {
  for (unsigned offset = 0; offset < MISTER_MAILBOX_BYTES; offset += 8) {
    const uint64_t word = sim.memory[Simulation::host + offset];
    transportMemory[offset / 4] = word;
    transportMemory[offset / 4 + 1] = word >> 32;
  }
}

extern "C" int mister_sim_pause(const struct timespec *, struct timespec *);

extern "C" int mister_sim_clock_gettime(clockid_t, struct timespec *time) {
  mister_sim_pause(nullptr, nullptr);
  const uint64_t ns = transportSimulation->cycles * 20;
  time->tv_sec = ns / 1000000000;
  time->tv_nsec = ns % 1000000000;
  return 0;
}

extern "C" int mister_sim_pause(const struct timespec *, struct timespec *) {
  auto &sim = *transportSimulation;
  for (unsigned offset = 0; offset < 0x1100; offset += 8) {
    if (offset >= 0x80 && offset < MISTER_MAILBOX_COMMANDS) continue;
    sim.memory[Simulation::host + offset] = uint64_t(transportMemory[offset / 4 + 1]) << 32 |
                                          transportMemory[offset / 4];
  }
  for (int i = 0; i < 1024; ++i) sim.tick();
  readMailbox(sim);
  return 0;
}

#ifndef MISTER_GLIDE_TEST
static void checkTransport(Simulation &sim) {
  readMailbox(sim);
  transportSimulation = &sim;
  mister_mailbox mailbox;
  require(mister_mailbox_attach(&mailbox, transportMemory, sizeof(transportMemory), 1000) == 0,
          "C transport failed to attach to RTL");
  std::array<mister_command, 256> commands;
  std::array<uint32_t, 256> results;
  for (auto &command : commands) command = {2, 0x3ff004, 0, 15};
  require(mister_mailbox_exchange(&mailbox, commands.data(), commands.size(), results.data(), nullptr) == 0,
          "C transport full batch failed against RTL");
  for (const auto result : results) require(result == 0x42415431, "C transport MMIO result mismatch");
  const mister_command frame = {3, 0x3ec00000, 640 | (480 << 12) | (4 << 24) | (1 << 29), 1280};
  require(mister_mailbox_exchange(&mailbox, &frame, 1, nullptr, nullptr) == 0,
          "C transport frame publication failed against RTL");
  require(sim.dut.io_frame_enable && sim.dut.io_frame_base == frame.address,
          "C transport frame did not activate");
  mister_mailbox_detach(&mailbox);
  transportSimulation = nullptr;
}

#endif

#ifdef MISTER_GLIDE_TEST
extern "C" int __wrap_mister_memory_open(mister_memory *memory) {
  require(!memory->base && memory->lock_fd == -1, "duplicate runtime mapping");
  memory->base = reinterpret_cast<uint8_t *>(transportMemory);
  memory->lock_fd = 42;
  return 0;
}

extern "C" int __wrap_mister_memory_close(mister_memory *memory) {
  require(!transportSimulation->dut.io_frame_enable, "runtime unmapped before disabling scanout");
  memory->base = nullptr;
  memory->lock_fd = -1;
  return 0;
}

static void checkGlide(Simulation &sim) {
  sim.submit({{3, MISTER_RENDERER_BASE, 640 | (480 << 12) | (4 << 24) | (1 << 29), 1280}});
  sim.protectRenderer = true;
  readMailbox(sim);
  transportSimulation = &sim;
  grGlideInit();
  GrHwConfiguration hardware;
  require(grSstQueryHardware(&hardware), "Glide failed to discover XLS");
  require(hardware.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam == 4,
          "XLS reported zero or incorrect texture capacity");
  grSstSelect(0);
  require(grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB,
                      GR_ORIGIN_UPPER_LEFT, 2, 1), "Glide failed to open XLS");
  guTexMemReset();
  const FxU32 texture_available = guTexMemQueryAvail(GR_TMU0);
  require(texture_available == 4 * 1024 * 1024, "XLS utility texture allocator has no memory");
  const GrMipMapId_t texture_id = guTexAllocateMemory(GR_TMU0, GR_MIPMAPLEVELMASK_BOTH, 256, 256, GR_TEXFMT_P_8,
                             GR_MIPMAP_DISABLE, GR_LOD_256, GR_LOD_256, GR_ASPECT_1x1,
                             GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP,
                             GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR, 0, FXFALSE);
  require(texture_id != GR_NULL_MIPMAP_HANDLE, "Tomb texture allocation failed");
  require(guTexMemQueryAvail(GR_TMU0) == texture_available - 65536, "texture allocation size mismatch");
  grBufferClear(0xffff0000, 255, 0xffff);
  grBufferSwap(0);
  grSstIdle();
  require(sim.dut.io_frame_enable, "Glide did not publish a frame");
  const uint32_t first_base = sim.dut.io_frame_base;
  require(first_base >= MISTER_RENDERER_BASE && first_base < MISTER_RENDERER_BASE + MISTER_RENDERER_BYTES,
          "Glide copied pixels instead of publishing renderer memory");
  const auto pixel = [&sim](unsigned x, unsigned y) {
    const uint32_t address = sim.dut.io_frame_base + y * sim.dut.io_frame_stride + x * 2;
    return uint16_t(transportMemory[(address - Simulation::host) / 4] >> ((address & 3u) * 8));
  };
  for (unsigned y = 0; y < 480; ++y)
    for (unsigned x = 0; x < 640; ++x) require(pixel(x, y) == 0xf800, "Glide frame pixel mismatch");
  sim.protectRenderer = true;
  grBufferClear(0xff0000ff, 255, 0xffff);
  grColorCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_CONSTANT,
                 GR_COMBINE_OTHER_NONE, FXFALSE);
  grConstantColorValue(0xff00ff00);
  GrVertex a = {}, b = {}, c = {};
  a.x = 10; a.y = 10; a.oow = 1;
  b.x = 100; b.y = 10; b.oow = 1;
  c.x = 10; c.y = 100; c.oow = 1;
  grDrawTriangle(&a, &b, &c);
  grBufferSwap(1);
  require(sim.dut.io_frame_base != first_base, "Glide overwrote the active renderer buffer");
  require(pixel(20, 20) == 0x07e0 && pixel(200, 200) == 0x001f,
          "Glide triangle/background pixel mismatch");
  grBufferClear(0xffff0000, 255, 0xffff);
  grBufferSwap(0);
  require(sim.dut.io_frame_base == first_base && pixel(20, 20) == 0xf800,
          "Glide did not safely reuse the retired renderer buffer");
  for (unsigned i = MISTER_FRAME_OFFSET / 4; i < MISTER_RENDERER_OFFSET / 4; ++i)
    require(transportMemory[i] == 0, "direct presentation copied into host slots");
  require(sim.scanoutBursts >= 2 * 480 * 10, "renderer scanout did not read complete frames");
  mister_guest_lfb_info guest = {sizeof(guest), 0, 0, 0, 0};
  require(mister_glide_lfb_lock(1, 1, 0, 0, 0, &guest), "guest back-buffer lock failed");
  const mister_command lfb_registers[] = {{2, 0x114, 0, 15}, {2, 0x21c, 0, 15}};
  uint32_t lfb_values[2] = {};
  require(!mister_runtime_exchange(lfb_registers, 2, lfb_values, nullptr), "guest LFB register read failed");
  require(lfb_values[1] >> 22 == 479, "Glide did not initialize the Y origin");
  require((lfb_values[0] & 0x30) == 0x10, "guest lock did not select the back buffer");
  mister_glide_lfb_write(1, 20 * guest.stride + 40, 0x07e0, 2);
  require(mister_glide_lfb_unlock(1, 1), "guest back-buffer unlock failed");
  grBufferSwap(0);
  require(pixel(20, 20) == 0x07e0, "guest LFB write did not reach GPU memory");
  require(mister_glide_lfb_lock(0, 0, 255, 0, 0, &guest), "guest front-buffer read lock failed");
  require(mister_glide_lfb_read(0, 20 * guest.stride + 40, 2) == 0x07e0,
          "guest LFB read bypassed GPU memory");
  require(mister_glide_lfb_unlock(0, 0), "guest front-buffer read unlock failed");
  require(mister_glide_lfb_lock(0, 1, 0, 0, 0, &guest), "Screamer read lock failed");
  require(mister_glide_lfb_lock(1, 1, 0, 0, 0, &guest), "Screamer companion write lock failed");
  mister_glide_lfb_write(1, 479 * guest.stride, 0x12345678, 4);
  require(mister_glide_lfb_read(1, 479 * guest.stride, 4) == 0x12345678,
          "Screamer paired lock read/write mismatch");
  require(mister_glide_lfb_unlock(1, 1), "Screamer companion unlock failed");
  require(mister_glide_lfb_unlock(0x10, 1), "Screamer read unlock failed");
  require(mister_glide_lfb_lock(1, 0, 0, 0, 0, &guest), "guest front-buffer write lock failed");
  require(!sim.dut.io_frame_enable, "guest front-buffer write did not retire scanout");
  mister_glide_lfb_write(0, 20 * guest.stride + 40, 0x001f, 2);
  require(mister_glide_lfb_unlock(1, 0), "guest front-buffer write unlock failed");
  require(sim.dut.io_frame_enable && pixel(20, 20) == 0x001f, "guest front-buffer unlock did not resume scanout");
  require(mister_glide_lfb_lock(1, 2, 15, 1, 0, &guest), "guest lower-origin aux write lock failed");
  mister_glide_lfb_write(2, 10 * guest.stride + 40, 0x1234, 2);
  require(mister_glide_lfb_unlock(1, 2), "guest aux write unlock failed");

  require(mister_glide_lfb_lock(0, 2, 255, 0, 0, &guest), "guest upper-origin aux read lock failed");
  require(mister_glide_lfb_read(2, 469 * guest.stride + 40, 2) == 0x1234,
          "guest aux/origin addressing mismatch");
  require(mister_glide_lfb_unlock(0, 2), "guest aux read unlock failed");
  for (unsigned pass = 0; pass < 4; ++pass) {
    require(mister_glide_lfb_lock(1, 1, 0, pass & 1, 0, &guest), "direct blend lock failed");
    const unsigned sequence = transportMemory[1];
    const uint32_t color = pass & 1 ? 0x001f : 0xf800;
    for (unsigned y = 10; y < 100; ++y)
      for (unsigned x = 10; x < 100; x += 2)
        mister_glide_lfb_write(1, ((pass & 1) ? 479 - y : y) * guest.stride + x * 2,
                              color | (color << 16), 4);
    require(transportMemory[1] == sequence, "direct LFB pixels still use mailbox commands");
    require(mister_glide_lfb_unlock(1, 1), "direct blend unlock failed");
    grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ZERO);
    grDrawTriangle(&a, &b, &c);
    grBufferSwap(0);
    require(pixel(20, 20) == ((pass & 1) ? 0x07ff : 0xffe0),
            "GPU blend read stale framebuffer after direct LFB writes");
  }
  grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);
  std::vector<uint8_t> texture(65536, 1);
  uint32_t palette[256] = {};
  palette[1] = 0xff00ff00;
  guTexDownloadMipMap(texture_id, texture.data(), nullptr);
  grTexDownloadTable(GR_TMU0, GR_TEXTABLE_PALETTE, palette);
  guTexSource(texture_id);
  grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
               GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
                 GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
  a.tmuvtx[0].oow = b.tmuvtx[0].oow = c.tmuvtx[0].oow = 1;
  grBufferClear(0xff000000, 255, 0xffff);
  grDrawTriangle(&a, &b, &c);
  grBufferSwap(0);

  require(pixel(20, 20) == 0x07e0 && pixel(200, 200) == 0,
          "Tomb paletted texture download/render failed");
  grBufferClear(0xff000000, 255, 0xffff);
  for (unsigned pass = 0; pass < 3; ++pass) {
    palette[1] = pass == 1 ? 0xff00ff00 : 0xffff0000;
    grTexDownloadTable(GR_TMU0, GR_TEXTABLE_PALETTE, palette);
    GrVertex left = a, right = b, bottom = c;
    left.x += pass * 200;
    right.x += pass * 200;
    bottom.x += pass * 200;
    grDrawTriangle(&left, &right, &bottom);
  }
  grBufferSwap(0);
  require(pixel(20, 20) == 0xf800 && pixel(220, 20) == 0x07e0 &&
              pixel(420, 20) == 0xf800,
          "palette bank miss/hit switches reordered queued triangles");
  guTexMemReset();
  grSstWinClose();
  require(!sim.dut.io_frame_enable, "Glide close retained a renderer buffer");
  require(grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB,
                      GR_ORIGIN_UPPER_LEFT, 2, 1), "Glide failed to reopen XLS");
  grBufferClear(0xff0000ff, 255, 0xffff);
  grBufferSwap(0);
  require(sim.dut.io_frame_enable && pixel(20, 20) == 0x001f,
          "Glide reopen did not publish renderer pixels");
  grGlideShutdown();
  require(!sim.dut.io_frame_enable, "Glide shutdown retained an active slot");
  transportSimulation = nullptr;
  std::puts("MiSTer complete Glide initialization/clear/triangle/swap/LFB/presentation/shutdown PASS");
}
#endif

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  try {
    Simulation sim;
    sim.reset();
#ifdef MISTER_GLIDE_TEST
    checkGlide(sim);
#else
    sim.submit({{2, 0x3ff004, 0, 15}, {1, 0x088, 0x3fc00000, 15}, {2, 0x008, 0, 15}});
    require(sim.result(0) == 0x42415431, "XLS capability register mismatch");
    require(sim.result(2) == 24, "XLS floating point register conversion mismatch");
    sim.submit({{1, 0x300, 0xa00, 15}, {1, 0x120, 0, 15}, {1, 0x3cc, 1, 15},
                {1, 0x008, 8, 15}, {1, 0x00c, 8, 15}, {1, 0x010, 56, 15},
                {1, 0x014, 8, 15}, {1, 0x018, 8, 15}, {1, 0x01c, 56, 15},
                {1, 0x110, 0x806f0, 15}, {1, 0x10c, 0x410, 15}, {1, 0x080, 0, 15},
                {2, 0x3d8, 0, 15}, {2, 0x3e0, 0, 15}});
    require(sim.result(12) > 0 && sim.result(12) == sim.result(13), "XLS triangle did not retire");
    require(sim.writes > 0, "XLS triangle did not write DDR");
    for (int i = 0; i < 200000; ++i) sim.tick();
    require(sim.scanoutBursts == 0, "scanout accessed host slots before publication");
    sim.submit({{3, 0x3e400000, 640 | (480 << 12) | (4 << 24) | (1 << 29), 1280}});
    for (int i = 0; i < 10000; ++i) sim.tick();
    require(sim.scanoutBursts > 0, "local scanout read path did not reach host DDR");
    require(sim.dut.io_frame_enable && sim.dut.io_frame_base == 0x3e400000 &&
            sim.dut.io_frame_width == 640 && sim.dut.io_frame_height == 480 &&
            sim.dut.io_frame_stride == 1280 && sim.dut.io_frame_format == 4,
            "frame descriptor did not activate");
    sim.dut.io_ps2Key = 1024 | 512 | 0x1c;
    for (int i = 0; i < 4; ++i) sim.tick();
    sim.submit({{4, 0, 0, 0}});
    require(sim.memory[Simulation::host + 0x2800] == uint64_t(1) << 0x1c, "keyboard press lost");
    sim.dut.io_osdOpen = 1;
    for (int i = 0; i < 4; ++i) sim.tick();
    sim.submit({{4, 0, 0, 0}});
    require(sim.memory[Simulation::host + 0x2800] == 0, "OSD did not release keys");
    sim.submit({{3, 0x3e400008, 640 | (480 << 12) | (4 << 24) | (1 << 29), 1280}}, 3);
    require(sim.dut.io_frame_base == 0x3e400000, "invalid frame replaced active buffer");
    for (const uint32_t base : {0x3e800000u, 0x3e400000u, 0x3e400000u}) {
      sim.submit({{3, base, 640 | (480 << 12) | (4 << 24) | (1 << 29), 1280}});
      require(sim.dut.io_frame_base == base, "frame handoff did not activate");
    }
    require(sim.scanoutBursts >= 3 * 480 * 10, "frame handoff did not drain complete frames");
    sim.submit({{3, 0, 0, 0}});
    const auto reads = sim.scanoutBursts;
    for (int i = 0; i < 10000; ++i) sim.tick();
    require(sim.scanoutBursts == reads && sim.pending.empty(), "disable did not retire slot reads");
    const auto writes = sim.writes;
    sim.reset();
    for (int i = 0; i < 5000; ++i) sim.tick();
    require(sim.writes == writes, "reset replayed previous rendering commands");
    require(!sim.dut.io_frame_enable, "reset retained frame descriptor");
    checkTransport(sim);
#endif
    sim.dut.final();
    std::printf("MiSTer platform simulation PASS (%llu cycles, %llu DDR writes, %llu scanout bursts)\n",
                static_cast<unsigned long long>(sim.cycles), static_cast<unsigned long long>(sim.writes),
                static_cast<unsigned long long>(sim.scanoutBursts));
  } catch (const std::exception &error) {
    std::fprintf(stderr, "MiSTer platform FAIL: %s\n", error.what());
    return 1;
  }
}
