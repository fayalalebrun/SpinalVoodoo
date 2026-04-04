#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

extern "C" {
#include "voodoo_trace_format.h"
}

#include "trace_replay_backend.h"
#include "trace_replay_image.h"
#include "trace_replay_io.h"
#include "trace_replay_runner.h"

namespace {

constexpr uint32_t kDefaultMmioBase = 0xC0000000u;
constexpr uint32_t kDefaultMmioSpan = 0x00400000u;
constexpr uint32_t kDefaultFbBase = 0x3F000000u;
constexpr uint32_t kResetManagerBase = 0xFFD05000u;
constexpr uint32_t kResetManagerSpan = 0x1000u;
constexpr uint32_t kBridgeModuleResetOff = 0x1Cu;
constexpr uint32_t kBridgeResetS2f = 1u << 6;
constexpr uint32_t kTextureOffset = 0x00400000u;
constexpr uint32_t kFramebufferBytes = 4u * 1024u * 1024u;
constexpr uint32_t kTextureBytes = 8u * 1024u * 1024u;

struct MapRegion {
  void *raw = MAP_FAILED;
  volatile uint8_t *base = nullptr;
  size_t size = 0;
};

struct PageCache {
  MapRegion region;
  uint32_t pageBase = 0xffffffffu;
  uint64_t mapCount = 0;
  uint64_t bytesCopied = 0;
};

struct Options {
  std::string inputPath;
  uint32_t mmioBase = kDefaultMmioBase;
  uint32_t mmioSpan = kDefaultMmioSpan;
  uint32_t fbBase = kDefaultFbBase;
  const char *memPath = "/dev/mem";
  bool skipState = false;
  bool skipFbState = false;
  bool dumpTexture = false;
  bool preloadTraceTextures = false;
  uint32_t texWriteDelayUs = 0;
  bool idleAfterTexBatch = false;
  bool resetBeforePlayback = false;
  uint32_t maxEntries = 0;
};

inline uint32_t mmioRead32(volatile uint8_t *mmio, uint32_t addr);
inline void mmioWrite32(volatile uint8_t *mmio, uint32_t addr, uint32_t value);
inline void mmioWrite16(volatile uint8_t *mmio, uint32_t addr, uint16_t value);
bool physWrite32(int fd, PageCache *cache, uint32_t physAddr, uint32_t value);
bool physWrite16(int fd, PageCache *cache, uint32_t physAddr, uint16_t value);
bool physWriteAll(int fd, PageCache *cache, uint32_t physBase, const void *src, size_t size);
bool physReadAll(int fd, PageCache *cache, uint32_t physBase, void *dst, size_t size);
bool physFillZero(int fd, PageCache *cache, uint32_t physBase, uint32_t size);
uint32_t remapBackendRegAddr(uint32_t addr);
uint32_t idleWait(volatile uint8_t *mmio);
bool pulseFpgaFabricReset(int fd);

class De10HardwareReplayBackend : public TraceReplayBackend {
public:
  De10HardwareReplayBackend(int fd, volatile uint8_t *mmio, PageCache *fbCache,
                            PageCache *texCache, uint32_t fbBase, uint32_t texWriteDelayUs)
      : fd_(fd), mmio_(mmio), fbCache_(fbCache), texCache_(texCache), fbBase_(fbBase),
        texWriteDelayUs_(texWriteDelayUs) {}

  bool useDe10RegisterMap() const override { return true; }
  bool writeReg32(uint32_t mappedAddr, uint32_t, uint32_t value) override {
    mmioWrite32(mmio_, mappedAddr, value);
    return true;
  }
  bool writeReg16(uint32_t mappedAddr, uint32_t, uint16_t value) override {
    mmioWrite16(mmio_, mappedAddr, value);
    return true;
  }
  bool writeFb32(uint32_t byteOffset, uint32_t value) override {
    return physWrite32(fd_, fbCache_, fbBase_ + byteOffset, value);
  }
  bool writeFb16(uint32_t byteOffset, uint16_t value) override {
    return physWrite16(fd_, fbCache_, fbBase_ + byteOffset, value);
  }
  bool writeTex32(uint32_t byteOffset, uint32_t value) override {
    const bool ok = physWrite32(fd_, texCache_, fbBase_ + kTextureOffset + byteOffset, value);
    if (ok && texWriteDelayUs_ != 0) usleep(texWriteDelayUs_);
    return ok;
  }
  bool writeFbBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) override {
    return physWriteAll(fd_, fbCache_, fbBase_ + byteOffset, data, size);
  }
  bool writeTexBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) override {
    const bool ok = physWriteAll(fd_, texCache_, fbBase_ + kTextureOffset + byteOffset, data, size);
    if (ok && texWriteDelayUs_ != 0) usleep(texWriteDelayUs_);
    return ok;
  }
  bool readFbWords(uint32_t byteOffset, uint32_t *dst, uint32_t wordCount) override {
    return physReadAll(fd_, fbCache_, fbBase_ + byteOffset, dst, wordCount * sizeof(uint32_t));
  }
  bool idleWait() override { ::idleWait(mmio_); return true; }
  bool invalidateFbCache() override { return true; }
  bool flushFbCache() override { return true; }
  bool setSwapCount(uint32_t count) override {
    mmioWrite32(mmio_, 0x128, count & 0x3u);
    return true;
  }

private:
  int fd_;
  volatile uint8_t *mmio_;
  PageCache *fbCache_;
  PageCache *texCache_;
  uint32_t fbBase_;
  uint32_t texWriteDelayUs_;
};

using Clock = std::chrono::steady_clock;

double elapsedMs(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void usage(const char *argv0) {
  fprintf(stderr,
          "Usage: %s <trace.bin|trace-dir> [--mmio-base HEX] [--mmio-span HEX] [--fb-base HEX] [--mem-path PATH] [--skip-state] [--skip-fb-state] [--dump-texture] [--preload-trace-textures] [--tex-write-delay-us N] [--idle-after-tex-batch] [--max-entries N]\n",
          argv0);
}

bool parseArgs(int argc, char **argv, Options *opts) {
  if (argc < 2) {
    usage(argv[0]);
    return false;
  }
  opts->inputPath = argv[1];
  for (int i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "--mmio-base") == 0 && i + 1 < argc) {
      opts->mmioBase = static_cast<uint32_t>(strtoul(argv[++i], nullptr, 0));
    } else if (strcmp(argv[i], "--mmio-span") == 0 && i + 1 < argc) {
      opts->mmioSpan = static_cast<uint32_t>(strtoul(argv[++i], nullptr, 0));
    } else if (strcmp(argv[i], "--fb-base") == 0 && i + 1 < argc) {
      opts->fbBase = static_cast<uint32_t>(strtoul(argv[++i], nullptr, 0));
    } else if (strcmp(argv[i], "--mem-path") == 0 && i + 1 < argc) {
      opts->memPath = argv[++i];
    } else if (strcmp(argv[i], "--skip-state") == 0) {
      opts->skipState = true;
    } else if (strcmp(argv[i], "--skip-fb-state") == 0) {
      opts->skipFbState = true;
    } else if (strcmp(argv[i], "--dump-texture") == 0) {
      opts->dumpTexture = true;
    } else if (strcmp(argv[i], "--preload-trace-textures") == 0) {
      opts->preloadTraceTextures = true;
    } else if (strcmp(argv[i], "--tex-write-delay-us") == 0 && i + 1 < argc) {
      opts->texWriteDelayUs = static_cast<uint32_t>(strtoul(argv[++i], nullptr, 0));
    } else if (strcmp(argv[i], "--idle-after-tex-batch") == 0) {
      opts->idleAfterTexBatch = true;
    } else if (strcmp(argv[i], "--reset-before-playback") == 0) {
      opts->resetBeforePlayback = true;
    } else if (strcmp(argv[i], "--max-entries") == 0 && i + 1 < argc) {
      opts->maxEntries = static_cast<uint32_t>(strtoul(argv[++i], nullptr, 0));
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      exit(0);
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      return false;
    }
  }
  return true;
}

bool mapRegion(int fd, uint32_t physBase, uint32_t span, MapRegion *out) {
  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize <= 0) {
    perror("sysconf(_SC_PAGESIZE)");
    return false;
  }
  uint64_t pageMask = static_cast<uint64_t>(pageSize) - 1u;
  uint64_t mapBase = physBase & ~pageMask;
  uint64_t pageOffset = physBase - mapBase;
  uint64_t mapSize = pageOffset + span;
  fprintf(stderr, "[de10-trace] mmap phys=0x%08x span=0x%08x aligned=0x%08llx size=0x%08llx\n",
          physBase, span, (unsigned long long)mapBase, (unsigned long long)mapSize);
  void *raw = mmap(nullptr, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, static_cast<off_t>(mapBase));
  fprintf(stderr, "[de10-trace] mmap returned %p errno=%d\n", raw, errno);
  if (raw == MAP_FAILED) {
    perror("mmap");
    return false;
  }
  out->raw = raw;
  out->base = static_cast<volatile uint8_t *>(raw) + pageOffset;
  out->size = static_cast<size_t>(mapSize);
  fprintf(stderr, "[de10-trace] mmap ok phys=0x%08x virt=%p\n", physBase, (const void *)out->base);
  return true;
}

void unmapRegion(MapRegion *region) {
  if (region->raw != MAP_FAILED) {
    munmap(region->raw, region->size);
    region->raw = MAP_FAILED;
    region->base = nullptr;
    region->size = 0;
  }
}

bool loadStateFromFile(
    const std::string &stateFile,
    int fd,
    volatile uint8_t *mmio,
    PageCache *fbCache,
    PageCache *texCache,
    uint32_t fbBase
) {
  auto stateStart = Clock::now();
  fprintf(stderr, "[de10-trace] loadStateFromFile begin %s\n", stateFile.c_str());
  int stateFd = open(stateFile.c_str(), O_RDONLY);
  if (stateFd < 0) {
    fprintf(stderr, "[de10-trace] loadStateFromFile open failed errno=%d\n", errno);
    return false;
  }
  fprintf(stderr, "[de10-trace] loadStateFromFile open ok\n");

  auto readExact = [&](void *dst, size_t size) -> bool {
    uint8_t *ptr = static_cast<uint8_t *>(dst);
    size_t done = 0;
    while (done < size) {
      ssize_t rc = read(stateFd, ptr + done, size - done);
      if (rc <= 0) {
        return false;
      }
      done += static_cast<size_t>(rc);
    }
    return true;
  };

  voodoo_state_header_t shdr;
  if (!readExact(&shdr, sizeof(shdr))) {
    close(stateFd);
    return false;
  }
  if (shdr.magic != VOODOO_STATE_MAGIC || shdr.version != VOODOO_STATE_VERSION) {
    close(stateFd);
    return false;
  }

  fprintf(stderr, "[de10-trace] Loading state: regs=%u fb=%u tex=%u\n", shdr.reg_count, shdr.fb_size, shdr.tex_size);

  auto *regs = static_cast<voodoo_state_reg_t *>(malloc(shdr.reg_count * sizeof(voodoo_state_reg_t)));
  if (!regs) {
    close(stateFd);
    return false;
  }
  if (!readExact(regs, shdr.reg_count * sizeof(voodoo_state_reg_t))) {
    free(regs);
    close(stateFd);
    return false;
  }

  for (uint32_t i = 0; i < shdr.reg_count; ++i) {
    uint32_t regAddr = regs[i].addr & 0x3fc;
    if (regAddr == 0x128 || regAddr == 0x124 || regAddr == 0x080 || regAddr == 0x100 || regAddr == 0x120) continue;
    if ((i % 128u) == 0u) {
      fprintf(stderr, "[de10-trace] State reg %u raw=0x%06x mapped=0x%06x value=0x%08x\n", i,
              regs[i].addr & 0x3fffff, remapBackendRegAddr(regs[i].addr), regs[i].value);
    }
    mmioWrite32(mmio, remapBackendRegAddr(regs[i].addr), regs[i].value);
  }
  fprintf(stderr, "[de10-trace] State register restore took %.3f ms\n", elapsedMs(stateStart));
  free(regs);

  if (shdr.fb_size > kFramebufferBytes || shdr.tex_size > kTextureBytes) {
    close(stateFd);
    return false;
  }

  constexpr size_t chunkSize = 1 << 20;
  auto *buffer = static_cast<uint8_t *>(malloc(chunkSize));
  if (!buffer) {
    close(stateFd);
    return false;
  }

  uint32_t remaining = shdr.fb_size;
  uint32_t offset = 0;
  auto fbStart = Clock::now();
  while (remaining) {
    size_t chunk = remaining > chunkSize ? chunkSize : remaining;
    if (!readExact(buffer, chunk)) {
      free(buffer);
      close(stateFd);
      return false;
    }
    if (!physWriteAll(fd, fbCache, fbBase + offset, buffer, chunk)) {
      free(buffer);
      close(stateFd);
      return false;
    }
    remaining -= chunk;
    offset += static_cast<uint32_t>(chunk);
  }
  fprintf(stderr,
          "[de10-trace] Framebuffer bulk copy took %.3f ms (%llu maps, %llu bytes)\n",
          elapsedMs(fbStart),
          static_cast<unsigned long long>(fbCache->mapCount),
          static_cast<unsigned long long>(fbCache->bytesCopied));

  remaining = shdr.tex_size;
  offset = 0;
  auto texStart = Clock::now();
  while (remaining) {
    size_t chunk = remaining > chunkSize ? chunkSize : remaining;
    if (!readExact(buffer, chunk)) {
      free(buffer);
      close(stateFd);
      return false;
    }
    if (!physWriteAll(fd, texCache, fbBase + kTextureOffset + offset, buffer, chunk)) {
      free(buffer);
      close(stateFd);
      return false;
    }
    remaining -= chunk;
    offset += static_cast<uint32_t>(chunk);
  }
  fprintf(stderr,
          "[de10-trace] Texture bulk copy took %.3f ms (%llu maps, %llu bytes)\n",
          elapsedMs(texStart),
          static_cast<unsigned long long>(texCache->mapCount),
          static_cast<unsigned long long>(texCache->bytesCopied));

  free(buffer);
  close(stateFd);
  fprintf(stderr, "[de10-trace] State bulk memory copied\n");
  auto idleStart = Clock::now();
  idleWait(mmio);
  fprintf(stderr, "[de10-trace] State load idle wait took %.3f ms\n", elapsedMs(idleStart));
  fprintf(stderr, "[de10-trace] State load idle wait complete\n");
  fprintf(stderr, "[de10-trace] Total state load took %.3f ms\n", elapsedMs(stateStart));
  return true;
}

inline uint32_t mmioRead32(volatile uint8_t *mmio, uint32_t addr) {
  return *reinterpret_cast<volatile uint32_t *>(const_cast<volatile uint8_t *>(mmio) + addr);
}

inline void mmioWrite32(volatile uint8_t *mmio, uint32_t addr, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(const_cast<volatile uint8_t *>(mmio) + addr) = value;
}

inline void mmioWrite16(volatile uint8_t *mmio, uint32_t addr, uint16_t value) {
  *reinterpret_cast<volatile uint16_t *>(const_cast<volatile uint8_t *>(mmio) + addr) = value;
}

bool ensurePageMapped(int fd, uint32_t physAddr, PageCache *cache) {
  uint32_t pageBase = physAddr & ~0xfffu;
  if (cache->region.raw != MAP_FAILED && cache->pageBase == pageBase) return true;
  unmapRegion(&cache->region);
  void *raw = mmap(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, static_cast<off_t>(pageBase));
  if (raw == MAP_FAILED) {
    perror("mmap-page");
    return false;
  }
  cache->region.raw = raw;
  cache->region.base = static_cast<volatile uint8_t *>(raw);
  cache->region.size = 0x1000;
  cache->pageBase = pageBase;
  cache->mapCount++;
  return true;
}

bool physWriteAll(int fd, PageCache *cache, uint32_t physBase, const void *src, size_t size) {
  const uint8_t *ptr = static_cast<const uint8_t *>(src);
  size_t done = 0;
  while (done < size) {
    uint32_t addr = physBase + static_cast<uint32_t>(done);
    if (!ensurePageMapped(fd, addr, cache)) return false;
    uint32_t pageOff = addr & 0xfffu;
    size_t chunk = size - done;
    if (chunk > (0x1000u - pageOff)) chunk = 0x1000u - pageOff;
    memcpy((void *)(cache->region.base + pageOff), ptr + done, chunk);
    cache->bytesCopied += chunk;
    done += chunk;
  }
  unmapRegion(&cache->region);
  return true;
}

bool physReadAll(int fd, PageCache *cache, uint32_t physBase, void *dst, size_t size) {
  uint8_t *ptr = static_cast<uint8_t *>(dst);
  size_t done = 0;
  while (done < size) {
    uint32_t addr = physBase + static_cast<uint32_t>(done);
    if (!ensurePageMapped(fd, addr, cache)) return false;
    uint32_t pageOff = addr & 0xfffu;
    size_t chunk = size - done;
    if (chunk > (0x1000u - pageOff)) chunk = 0x1000u - pageOff;
    memcpy(ptr + done, (void *)(cache->region.base + pageOff), chunk);
    done += chunk;
  }
  return true;
}

bool physFillZero(int fd, PageCache *cache, uint32_t physBase, uint32_t size) {
  static uint8_t zeros[4096] = {};
  uint32_t offset = 0;
  while (offset < size) {
    size_t chunk = (size - offset) > sizeof(zeros) ? sizeof(zeros) : (size - offset);
    if (!physWriteAll(fd, cache, physBase + offset, zeros, chunk)) return false;
    offset += static_cast<uint32_t>(chunk);
  }
  return true;
}

bool physWrite32(int fd, PageCache *cache, uint32_t physAddr, uint32_t value) {
  return physWriteAll(fd, cache, physAddr, &value, sizeof(value));
}

bool physWrite16(int fd, PageCache *cache, uint32_t physAddr, uint16_t value) {
  return physWriteAll(fd, cache, physAddr, &value, sizeof(value));
}

uint32_t remapBackendRegAddr(uint32_t addr) {
  const uint32_t base = addr & 0x3FFFFFu;
  if ((base & 0x200000u) == 0) return base;
  switch (base & 0xfffu) {
    case 0x088: return 0x088;
    case 0x08c: return 0x08c;
    case 0x090: return 0x090;
    case 0x094: return 0x094;
    case 0x098: return 0x098;
    case 0x09c: return 0x09c;
    case 0x0a0: return 0x0a0;
    case 0x0a4: return 0x0c0;
    case 0x0a8: return 0x0e0;
    case 0x0ac: return 0x0a4;
    case 0x0b0: return 0x0c4;
    case 0x0b4: return 0x0e4;
    case 0x0b8: return 0x0a8;
    case 0x0bc: return 0x0c8;
    case 0x0c0: return 0x0e8;
    case 0x0c4: return 0x0ac;
    case 0x0c8: return 0x0cc;
    case 0x0cc: return 0x0ec;
    case 0x0d0: return 0x0b0;
    case 0x0d4: return 0x0d0;
    case 0x0d8: return 0x0f0;
    case 0x0dc: return 0x0b4;
    case 0x0e0: return 0x0d4;
    case 0x0e4: return 0x0f4;
    case 0x0e8: return 0x0b8;
    case 0x0ec: return 0x0d8;
    case 0x0f0: return 0x0f8;
    case 0x0f4: return 0x0bc;
    case 0x0f8: return 0x0dc;
    case 0x0fc: return 0x0fc;
    default: return base & ~0x200000u;
  }
}

uint32_t idleWait(volatile uint8_t *mmio) {
  constexpr uint32_t kSstBusy = 1u << 9;
  constexpr uint32_t kFifoMask = 0x3Fu;
  constexpr uint32_t kSwapsPendingMask = 7u << 28;
  int idleCount = 0;
  int idleNoBusyCount = 0;
  for (int timeout = 0; timeout < 5000000; ++timeout) {
    uint32_t status = mmioRead32(mmio, 0x000);
    bool busy = (status & kSstBusy) != 0;
    uint32_t fifoFree = status & kFifoMask;
    uint32_t swapsPending = (status & kSwapsPendingMask) >> 28;
    if (!busy && fifoFree == 0x3F && swapsPending == 0) {
      if (++idleCount >= 3) return status;
    } else {
      idleCount = 0;
    }
    if (fifoFree == 0x3F && swapsPending == 0) {
      if (++idleNoBusyCount >= 1024) {
        fprintf(stderr,
                "[de10-trace] WARNING: idle_wait accepting sticky-busy status=0x%08x after %d polls\n",
                status, idleNoBusyCount);
        return status;
      }
    } else {
      idleNoBusyCount = 0;
    }
  }
  uint32_t status = mmioRead32(mmio, 0x000);
  fprintf(stderr, "[de10-trace] WARNING: idle_wait timeout status=0x%08x\n", status);
  return status;
}

bool pulseFpgaFabricReset(int fd) {
  MapRegion rstmgr;
  if (!mapRegion(fd, kResetManagerBase, kResetManagerSpan, &rstmgr)) {
    fprintf(stderr, "[de10-trace] ERROR: failed to map reset manager\n");
    return false;
  }

  auto *brgmodrst = reinterpret_cast<volatile uint32_t *>(const_cast<volatile uint8_t *>(rstmgr.base) + kBridgeModuleResetOff);
  const uint32_t before = *brgmodrst;
  const uint32_t asserted = before | kBridgeResetS2f;
  const uint32_t released = before & ~kBridgeResetS2f;
  fprintf(stderr, "[de10-trace] Pulsing FPGA fabric reset: brgmodrst before=0x%08x asserted=0x%08x released=0x%08x\n",
          before, asserted, released);
  *brgmodrst = asserted;
  __sync_synchronize();
  usleep(1000);
  *brgmodrst = released;
  __sync_synchronize();
  usleep(1000);
  fprintf(stderr, "[de10-trace] FPGA fabric reset complete: brgmodrst now=0x%08x\n", *brgmodrst);
  unmapRegion(&rstmgr);
  return true;
}

}  // namespace

int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);

  Options opts;
  if (!parseArgs(argc, argv, &opts)) return 2;

  TraceReplayFiles files;
  traceReplayResolveFiles(opts.inputPath.c_str(), &files);

  TraceReplayLoadedTrace trace;
  if (!traceReplayLoadTraceFile(files.traceFile, &trace)) {
    fprintf(stderr, "[de10-trace] ERROR: cannot read %s\n", files.traceFile.c_str());
    return 1;
  }
  fprintf(stderr, "[de10-trace] Loaded trace %s (%u entries)\n", files.traceFile.c_str(), trace.entryCount);

  int fd = open(opts.memPath, O_RDWR | O_SYNC);
  fprintf(stderr, "[de10-trace] open(%s) returned %d errno=%d\n", opts.memPath, fd, errno);
  if (fd < 0) {
    perror("open(/dev/mem)");
    free(trace.traceData);
    return 1;
  }

  if (opts.resetBeforePlayback && !pulseFpgaFabricReset(fd)) {
    close(fd);
    free(trace.traceData);
    return 1;
  }

  MapRegion mmio;
  PageCache fbCache;
  PageCache texCache;
  fprintf(stderr, "[de10-trace] Mapping MMIO region\n");
  if (!mapRegion(fd, opts.mmioBase, opts.mmioSpan, &mmio)) {
    unmapRegion(&mmio);
    close(fd);
    free(trace.traceData);
    return 1;
  }
  fprintf(stderr, "[de10-trace] MMIO mapping ready; DDR writes via pwrite at fb=0x%08x tex=0x%08x\n",
          opts.fbBase, opts.fbBase + kTextureOffset);

  if (files.stateFile.empty() && !opts.preloadTraceTextures) {
    fprintf(stderr, "[de10-trace] No state.bin; clearing texture region before replay\n");
    if (!physFillZero(fd, &texCache, opts.fbBase + kTextureOffset, kTextureBytes)) {
      fprintf(stderr, "[de10-trace] ERROR: failed to clear texture region\n");
      unmapRegion(&mmio);
      unmapRegion(&texCache.region);
      close(fd);
      free(trace.traceData);
      return 1;
    }
  }
  unmapRegion(&fbCache.region);
  unmapRegion(&texCache.region);

  if (opts.preloadTraceTextures) {
    uint32_t preloadWrites = 0;
    for (uint32_t i = 0; i < trace.entryCount; ++i) {
      const auto &e = trace.entries[i];
      if (e.cmd_type != VOODOO_TRACE_WRITE_TEX_L) continue;
      const uint32_t repeat = e.count ? e.count : 1u;
      const uint32_t texAddr = e.addr & 0x7fffffu;
      for (uint32_t r = 0; r < repeat; ++r) {
        if (!physWrite32(fd, &texCache, opts.fbBase + kTextureOffset + texAddr + r * 4u, e.data)) {
          fprintf(stderr, "[de10-trace] ERROR: failed to preload texture writes\n");
          unmapRegion(&mmio);
          close(fd);
          free(trace.traceData);
          return 1;
        }
        preloadWrites++;
      }
    }
    unmapRegion(&texCache.region);
    fprintf(stderr, "[de10-trace] Preloaded %u texture writes from trace before replay\n", preloadWrites);
  }

  TraceReplayStateInfo stateInfo;
  if (files.stateFile.empty()) {
    De10HardwareReplayBackend hwBackend(fd, mmio.base, &fbCache, &texCache, opts.fbBase, opts.texWriteDelayUs);
    if (!hwBackend.setSwapCount(0)) {
      fprintf(stderr, "[de10-trace] ERROR: failed to initialize swapCount for no-state replay\n");
      unmapRegion(&mmio);
      close(fd);
      free(trace.traceData);
      return 1;
    }
    fprintf(stderr, "[de10-trace] No state.bin; initialized swapCount=0 for replay\n");
    for (uint32_t i = 0; i < trace.entryCount; ++i) {
      const auto &e = trace.entries[i];
      if (e.cmd_type != VOODOO_TRACE_WRITE_REG_L) continue;
      if ((e.addr & 0x3fcu) != 0x218u) continue;
      stateInfo.bufferOffset = ((e.data >> 11) & 0x1ffu) << 12;
      stateInfo.drawOffset = stateInfo.bufferOffset;
    }
  }

  if (!files.stateFile.empty() && !opts.skipState) {
    fprintf(stderr, "[de10-trace] Reading state %s\n", files.stateFile.c_str());
    uint32_t stateSize = 0;
    uint8_t *stateData = traceReplayReadFile(files.stateFile.c_str(), &stateSize);
    if (!stateData) {
      fprintf(stderr, "[de10-trace] ERROR: failed to read %s\n", files.stateFile.c_str());
      unmapRegion(&mmio);
      close(fd);
      free(trace.traceData);
      return 1;
    }
    De10HardwareReplayBackend hwBackend(fd, mmio.base, &fbCache, &texCache, opts.fbBase, opts.texWriteDelayUs);
    if (!traceReplayLoadState(hwBackend, stateData, stateSize, opts.skipFbState, &stateInfo)) {
      free(stateData);
      fprintf(stderr, "[de10-trace] ERROR: failed to load %s\n", files.stateFile.c_str());
      unmapRegion(&mmio);
      close(fd);
      free(trace.traceData);
      return 1;
    }
    fprintf(stderr,
            "[de10-trace] State loaded via shared engine: draw=0x%x aux=0x%x row=%u disp=%ux%u\n",
            stateInfo.drawOffset, stateInfo.auxOffset, stateInfo.rowWidth, stateInfo.hDisp,
            stateInfo.vDisp);
    free(stateData);
  }

  uint32_t replayEntries = opts.maxEntries && opts.maxEntries < trace.entryCount ? opts.maxEntries : trace.entryCount;
  fbCache.mapCount = 0;
  fbCache.bytesCopied = 0;
  texCache.mapCount = 0;
  texCache.bytesCopied = 0;
  De10HardwareReplayBackend replayBackend(fd, mmio.base, &fbCache, &texCache, opts.fbBase, opts.texWriteDelayUs);
  auto replayStart = Clock::now();
  bool previousWasTexWrite = false;
  TraceReplayRunResult replayResult;
  if (!traceReplayRunEntries(replayBackend, trace.entries, replayEntries,
                             stateInfo.drawOffset, stateInfo.bufferOffset,
                             &replayResult,
                             [&](uint32_t i, const voodoo_trace_entry_t &e, uint32_t,
                                 const TraceReplayRunResult &, bool) {
                               const bool isTexWrite = e.cmd_type == VOODOO_TRACE_WRITE_TEX_L;
                               if (opts.idleAfterTexBatch && previousWasTexWrite && !isTexWrite) {
                                 replayBackend.idleWait();
                               }
                               if (opts.preloadTraceTextures && e.cmd_type == VOODOO_TRACE_WRITE_TEX_L) {
                                 previousWasTexWrite = true;
                                 return false;
                               }
                               if (i < 4) {
                                 fprintf(stderr, "[de10-trace] Entry %u type=%u addr=0x%08x data=0x%08x repeat=%u\n",
                                         i, e.cmd_type, e.addr, e.data, e.count ? e.count : 1u);
                               }
                               if ((i % 4096u) == 0u && i != 0u) {
                                 fprintf(stderr, "[de10-trace] Progress: %u/%u entries\n", i, replayEntries);
                               }
                               previousWasTexWrite = isTexWrite;
                               return true;
                             })) {
    unmapRegion(&mmio);
    close(fd);
    free(trace.traceData);
    return 1;
  }

  fprintf(stderr,
          "[de10-trace] Replay loop body took %.3f ms (reg=%u tex=%u fb=%u swaps=%u fbMaps=%llu texMaps=%llu fbBytes=%llu texBytes=%llu)\n",
          elapsedMs(replayStart), replayResult.regWrites, replayResult.texWrites,
          replayResult.fbWrites, replayResult.swapWrites,
          static_cast<unsigned long long>(fbCache.mapCount),
          static_cast<unsigned long long>(texCache.mapCount),
          static_cast<unsigned long long>(fbCache.bytesCopied),
          static_cast<unsigned long long>(texCache.bytesCopied));
  fprintf(stderr, "[de10-trace] Replay loop complete, entering final idle wait\n");
  auto finalIdleStart = Clock::now();
  traceReplayFinish(replayBackend);
  uint32_t finalStatus = mmioRead32(mmio.base, 0x000);
  uint32_t finalVideoDimensions = mmioRead32(mmio.base, 0x20c);
  uint32_t finalFbiInit1 = mmioRead32(mmio.base, 0x214);
  uint32_t finalFbiInit2 = mmioRead32(mmio.base, 0x218);
  fprintf(stderr, "[de10-trace] Final idle wait took %.3f ms\n", elapsedMs(finalIdleStart));
  uint32_t pixelsIn = mmioRead32(mmio.base, 0x14c);
  uint32_t pixelsOut = mmioRead32(mmio.base, 0x15c);
  fprintf(stderr,
          "[de10-trace] Replay complete: reg=%u tex=%u fb=%u swaps=%u status=0x%08x pixelsIn=%u pixelsOut=%u\n",
          replayResult.regWrites, replayResult.texWrites, replayResult.fbWrites,
          replayResult.swapWrites, finalStatus, pixelsIn, pixelsOut);

  if (!replayResult.havePresentedOffset) {
    replayResult.presentedOffset = replayResult.currentDrawOffset;
    fprintf(stderr,
            "[de10-trace] WARNING: Trace contains no swapbufferCMD; falling back to final draw_offset=0x%x\n",
            replayResult.presentedOffset);
  }

  if (stateInfo.hDisp == 0 || stateInfo.vDisp == 0) {
    stateInfo.hDisp = (finalVideoDimensions & 0xfffu) + 1u;
    stateInfo.vDisp = (finalVideoDimensions >> 16) & 0xfffu;
    if (stateInfo.vDisp == 386 || stateInfo.vDisp == 402 || stateInfo.vDisp == 482 || stateInfo.vDisp == 602)
      stateInfo.vDisp -= 2;
    if (stateInfo.hDisp <= 1 || stateInfo.vDisp == 0) {
      stateInfo.hDisp = 640;
      stateInfo.vDisp = 480;
    }
  }
  if (stateInfo.rowWidth == 0) {
    uint32_t tilesX = (finalFbiInit1 >> 4) & 0xfu;
    if (tilesX > 0) stateInfo.rowWidth = (tilesX << 7);
    if (stateInfo.rowWidth == 0) stateInfo.rowWidth = stateInfo.hDisp * 2;
  }
  if (stateInfo.bufferOffset == 0) {
    stateInfo.bufferOffset = ((finalFbiInit2 >> 11) & 0x1ffu) << 12;
  }

  if (stateInfo.rowWidth && stateInfo.hDisp && stateInfo.vDisp) {
    std::vector<uint16_t> fb565;
    if (!traceReplayCaptureRgb565(replayBackend, replayResult.presentedOffset, stateInfo.rowWidth / 2,
                                  stateInfo.hDisp, stateInfo.vDisp, &fb565)) {
      fprintf(stderr, "[de10-trace] ERROR: failed to capture framebuffer via shared backend\n");
      unmapRegion(&mmio);
      unmapRegion(&fbCache.region);
      unmapRegion(&texCache.region);
      close(fd);
      free(trace.traceData);
      return 1;
    }
    std::string rawPath = opts.inputPath + "/trace_board_fb.raw";
    FILE *raw = fopen(rawPath.c_str(), "wb");
    if (!raw) {
      fprintf(stderr, "[de10-trace] ERROR: cannot open %s for write\n", rawPath.c_str());
    } else {
      for (uint32_t y = 0; y < stateInfo.vDisp; ++y) {
        fwrite(&fb565[y * (stateInfo.rowWidth / 2)], sizeof(uint16_t), stateInfo.rowWidth / 2, raw);
      }
      fclose(raw);
      fprintf(stderr,
              "[de10-trace] Captured framebuffer via shared backend: %s (presented=0x%x row=%u disp=%ux%u)\n",
              rawPath.c_str(), replayResult.presentedOffset, stateInfo.rowWidth, stateInfo.hDisp, stateInfo.vDisp);
    }

    uint8_t srgbLut[256];
    traceReplayInitSrgbLut(srgbLut);
    std::string pngPath = opts.inputPath + "/trace_board_fb.png";
    if (!traceReplayWriteRgb565Png(pngPath.c_str(), fb565.data(),
                                   static_cast<int>(stateInfo.hDisp),
                                   static_cast<int>(stateInfo.vDisp),
                                   static_cast<int>(stateInfo.rowWidth / 2),
                                   srgbLut, stbi_write_png)) {
      fprintf(stderr, "[de10-trace] ERROR: failed to write %s\n", pngPath.c_str());
    } else {
      fprintf(stderr, "[de10-trace] Wrote PNG via shared LUT path: %s\n", pngPath.c_str());
    }
  }

  if (opts.dumpTexture) {
    std::vector<uint8_t> texDump(kTextureBytes);
    if (!physReadAll(fd, &texCache, opts.fbBase + kTextureOffset, texDump.data(), texDump.size())) {
      fprintf(stderr, "[de10-trace] ERROR: failed to dump texture memory\n");
    } else {
      std::string dumpPath = opts.inputPath + "/trace_board_tex.bin";
      FILE *dump = fopen(dumpPath.c_str(), "wb");
      if (!dump) {
        perror("fopen-tex-dump");
      } else {
        fwrite(texDump.data(), 1, texDump.size(), dump);
        fclose(dump);
        fprintf(stderr, "[de10-trace] Dumped texture memory: %s\n", dumpPath.c_str());
      }
    }
  }

  unmapRegion(&mmio);
  unmapRegion(&fbCache.region);
  unmapRegion(&texCache.region);
  close(fd);
    free(trace.traceData);
    return 0;
}
