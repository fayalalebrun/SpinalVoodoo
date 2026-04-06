#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <cerrno>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../emu/test/stb_image_write.h"

extern "C" {
#include "../emu/trace/voodoo_trace_format.h"
}

#include "../emu/trace/trace_replay_backend.h"
#include "../emu/trace/trace_replay_image.h"
#include "../emu/trace/trace_replay_io.h"
#include "../emu/trace/trace_replay_runner.h"
#include "../emu/sim/harness/sim_harness.h"

static bool ensure_directory(const char *path) {
  if (!path || !path[0]) return false;
  std::string cur;
  if (path[0] == '/') cur = "/";
  for (const char *p = path; *p; ++p) {
    if (*p == '/') {
      if (!cur.empty() && !traceReplayIsDirectory(cur.c_str()) && mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
        return false;
      }
    }
    cur.push_back(*p);
  }
  if (!traceReplayIsDirectory(cur.c_str()) && mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
    return false;
  }
  return true;
}

class SimReplayBackend : public TraceReplayBackend {
public:
  bool useDe10RegisterMap() const override { return false; }
  bool writeReg32(uint32_t mappedAddr, uint32_t, uint32_t value) override {
    sim_write(mappedAddr, value);
    return !sim_stalled();
  }
  bool writeReg16(uint32_t mappedAddr, uint32_t, uint16_t value) override {
    sim_write16(mappedAddr, value);
    return !sim_stalled();
  }
  bool writeFb32(uint32_t byteOffset, uint32_t value) override {
    sim_write(0x400000 | byteOffset, value);
    return !sim_stalled();
  }
  bool writeFb16(uint32_t byteOffset, uint16_t value) override {
    sim_write16(0x400000 | byteOffset, value);
    return !sim_stalled();
  }
  bool writeTex32(uint32_t byteOffset, uint32_t value) override {
    sim_write(0x800000 | byteOffset, value);
    return !sim_stalled();
  }
  bool writeFbBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) override {
    std::vector<uint32_t> words((size + 3) / 4, 0);
    memcpy(words.data(), data, size);
    sim_write_fb_bulk(byteOffset, words.data(), words.size());
    return !sim_stalled();
  }
  bool writeTexBulk(uint32_t byteOffset, const uint8_t *data, uint32_t size) override {
    std::vector<uint32_t> words((size + 3) / 4, 0);
    memcpy(words.data(), data, size);
    sim_write_tex_bulk(byteOffset, words.data(), words.size());
    return !sim_stalled();
  }
  bool readFbWords(uint32_t byteOffset, uint32_t *dst, uint32_t wordCount) override {
    sim_read_fb(byteOffset, dst, wordCount);
    return true;
  }
  bool idleWait() override {
    sim_idle_wait();
    return !sim_stalled();
  }
  bool invalidateFbCache() override {
    sim_invalidate_fb_cache();
    return !sim_stalled();
  }
  bool flushFbCache() override {
    sim_flush_fb_cache();
    return !sim_stalled();
  }
  bool setSwapCount(uint32_t count) override {
    sim_set_swap_count(count);
    return !sim_stalled();
  }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <trace.bin|trace-dir> <output-dir>\n", argv[0]);
    return 1;
  }

  TraceReplayFiles files;
  if (!traceReplayResolveFiles(argv[1], &files)) {
    fprintf(stderr, "failed to resolve input files\n");
    return 1;
  }
  if (!ensure_directory(argv[2])) {
    fprintf(stderr, "failed to create output directory %s\n", argv[2]);
    return 1;
  }

  TraceReplayLoadedTrace trace;
  if (!traceReplayLoadTraceFile(files.traceFile, &trace)) {
    fprintf(stderr, "failed to load trace %s\n", files.traceFile.c_str());
    return 1;
  }

  uint8_t *stateData = nullptr;
  uint32_t stateSize = 0;
  if (!files.stateFile.empty()) {
    stateData = traceReplayReadFile(files.stateFile.c_str(), &stateSize);
    if (!stateData) {
      fprintf(stderr, "failed to load state %s\n", files.stateFile.c_str());
      return 1;
    }
  }

  if (sim_init() != 0) {
    fprintf(stderr, "sim_init failed\n");
    return 1;
  }

  SimReplayBackend backend;
  TraceReplayStateInfo stateInfo{};
  if (stateData && !traceReplayLoadState(backend, stateData, stateSize, false, &stateInfo)) {
    fprintf(stderr, "failed to load state into simulator\n");
    sim_shutdown();
    return 1;
  }

  TraceReplayRunResult runResult{};
  if (!traceReplayRunEntries(backend, trace.entries, trace.entryCount, stateInfo.drawOffset, stateInfo.bufferOffset, &runResult)) {
    fprintf(stderr, "trace replay failed\n");
    sim_shutdown();
    return 1;
  }
  if (!traceReplayFinish(backend)) {
    fprintf(stderr, "trace replay finish failed\n");
    sim_shutdown();
    return 1;
  }

  const uint32_t fbOffset = runResult.havePresentedOffset ? runResult.presentedOffset : stateInfo.drawOffset;
  const uint32_t rowStridePixels = stateInfo.rowWidth ? (stateInfo.rowWidth / 2u) : stateInfo.hDisp;
  std::vector<uint16_t> rgb565;
  if (!traceReplayCaptureRgb565(backend, fbOffset, rowStridePixels, stateInfo.hDisp, stateInfo.vDisp, &rgb565)) {
    fprintf(stderr, "framebuffer capture failed\n");
    sim_shutdown();
    return 1;
  }

  uint8_t srgbLut[256];
  traceReplayInitSrgbLut(srgbLut);
  std::string pngPath = std::string(argv[2]) + "/screenshot.png";
  if (!traceReplayWriteRgb565Png(pngPath.c_str(), rgb565.data(), stateInfo.hDisp, stateInfo.vDisp, rowStridePixels, srgbLut, stbi_write_png)) {
    fprintf(stderr, "png write failed\n");
    sim_shutdown();
    return 1;
  }

  std::string rawPath = std::string(argv[2]) + "/frame.raw";
  FILE *raw = fopen(rawPath.c_str(), "wb");
  if (raw) {
    fwrite(rgb565.data(), sizeof(uint16_t), rgb565.size(), raw);
    fclose(raw);
  }

  printf("wrote %s\n", pngPath.c_str());
  printf("fb_fill_hits=%u fb_fill_misses=%u read_req_count=%u\n",
         sim_get_fb_fill_hits(), sim_get_fb_fill_misses(), sim_get_fb_read_req_count());
  sim_shutdown();
  free(trace.traceData);
  free(stateData);
  return 0;
}
