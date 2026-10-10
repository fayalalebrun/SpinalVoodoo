#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MMIO_BASE 0xC0000000u
#define MMIO_SPAN 0x01000000u
#define REG_SPAN_DEFAULT 0x00004000u

#define REG_STATUS 0x000u
#define REG_VAX 0x008u
#define REG_VAY 0x00cu
#define REG_VBX 0x010u
#define REG_VBY 0x014u
#define REG_VCX 0x018u
#define REG_VCY 0x01cu
#define REG_S 0x034u
#define REG_T 0x038u
#define REG_W 0x03cu
#define REG_DSDX 0x054u
#define REG_DTDX 0x058u
#define REG_DWDX 0x05cu
#define REG_DSDY 0x074u
#define REG_DTDY 0x078u
#define REG_DWDY 0x07cu
#define REG_TRIANGLECMD 0x080u
#define REG_FBZCOLORPATH 0x104u
#define REG_FBZMODE 0x110u
#define REG_LFBMODE 0x114u
#define REG_CLIPLEFT_RIGHT 0x118u
#define REG_CLIPBOTTOM_TOP 0x11cu
#define REG_NOPCMD 0x120u
#define REG_FASTFILLCMD 0x124u
#define REG_ZACOLOR 0x130u
#define REG_C0 0x158u
#define REG_C1 0x15cu
#define REG_H2F_STATUS 0x254u
#define REG_TEXTUREMODE 0x300u
#define REG_TLOD 0x304u
#define REG_TEXBASEADDR 0x30cu
#define REG_TREXINIT0 0x31cu
#define REG_TREXINIT1 0x320u
#define REG_FBIINIT4 0x200u
#define REG_FBIINIT0 0x210u
#define REG_FBIINIT1 0x214u
#define REG_FBIINIT2 0x218u
#define REG_FBIINIT3 0x21cu

#define LFB_BASE 0x400000u
#define TEX_BASE 0x800000u

#define SST_FBI_BUSY (1u << 7)
#define SST_BUSY (1u << 9)
#define SST_RGBWRMASK 0x00000200u
#define SST_ZAWRMASK 0x00100000u
#define SST_ENDITHER 0x00000100u
#define SST_DRAWBUFFER_FRONT 0x00000000u
#define SST_DRAWBUFFER_BACK 0x00000400u
#define SST_RGBSEL_C1_CC_PASS 0x00000002u
#define SST_TEXTURED_COLORPATH 0x08000001u
#define SST_TEXTUREMODE_RGB565_REPLACE 0x08241a00u

#define SST_FBIINIT0_DEFAULT 0x00000410u
#define SST_FBIINIT1_DEFAULT 0x00201102u
#define SST_FBIINIT2_DEFAULT 0x80000040u
#define SST_FBIINIT3_DEFAULT 0x001e4000u
#define SST_FBIINIT4_DEFAULT 0x00000001u
#define SST_GRX_RESET (1u << 1)
#define SST_PCI_FIFO_RESET (1u << 2)
#define SST_VIDEO_RESET (1u << 8)
#define SST_LFB_READ_EN (1u << 12)
#define SST_EN_LFB_RDAHEAD (1u << 8)
#define SST_ALT_REGMAPPING (1u << 14)
#define SST_EN_DRAM_REFRESH (1u << 23)
#define SST_EN_LFB_MEMFIFO (1u << 14)
#define SST_EN_TEX_MEMFIFO (1u << 15)

static FILE *logf;
static volatile uint8_t *mmio;
static volatile uint8_t *lfb_mmio;
static volatile uint8_t *tex_mmio;
static uint32_t mmio_span = MMIO_SPAN;
static uint32_t reg_span = REG_SPAN_DEFAULT;
static int mem_fd = -1;
static int multi_mapping = 0;
static int split_mapping = 0;
static int split_map_lfbtex = 1;
static int split_unmap_reservation = 0;

static void flush_log(void) {
  fflush(logf);
  fsync(fileno(logf));
}

static void stage(const char *msg) {
  fprintf(logf, "%s\n", msg);
  flush_log();
}

static void stage_u32(const char *msg, uint32_t a) {
  fprintf(logf, "%s 0x%08x\n", msg, a);
  flush_log();
}

static void usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s <legacy-reset|init-sum [count]|tmu-info|fifo-tri [count]|tex-oob-read|tex-oob-write|probe-write <offset> [value]|status>\n",
          argv0);
}

static uint32_t parse_u32(const char *s) {
  char *end = NULL;
  unsigned long value = strtoul(s, &end, 0);
  if (s[0] == '\0' || (end != NULL && *end != '\0')) {
    fprintf(stderr, "invalid integer: %s\n", s);
    exit(2);
  }
  return (uint32_t)value;
}

static volatile uint32_t *ptr32(uint32_t off) {
  if (multi_mapping) {
    if (off >= TEX_BASE) {
      return (volatile uint32_t *)(tex_mmio + (off - TEX_BASE));
    }
    if (off >= LFB_BASE) {
      return (volatile uint32_t *)(lfb_mmio + (off - LFB_BASE));
    }
  }
  return (volatile uint32_t *)(mmio + off);
}

static uint32_t rd32(uint32_t off, const char *tag) {
  char buf[160];
  snprintf(buf, sizeof(buf), "before read %s off=0x%06x", tag, off);
  stage(buf);
  uint32_t v = *ptr32(off);
  snprintf(buf, sizeof(buf), "after read %s off=0x%06x value=0x%08x", tag, off, v);
  stage(buf);
  return v;
}

static void wr32(uint32_t off, uint32_t value, const char *tag) {
  char buf[160];
  snprintf(buf, sizeof(buf), "before write %s off=0x%06x value=0x%08x", tag, off, value);
  stage(buf);
  *ptr32(off) = value;
  snprintf(buf, sizeof(buf), "after write %s off=0x%06x", tag, off);
  stage(buf);
}

static void wr32_fast(uint32_t off, uint32_t value) {
  *ptr32(off) = value;
}

static uint32_t rd32_fast(uint32_t off) {
  return *ptr32(off);
}

static void log_status(const char *tag) {
  uint32_t status = rd32(REG_STATUS, tag);
  uint32_t bridge = rd32(REG_H2F_STATUS, "h2f-status");
  fprintf(logf, "%s summary status=0x%08x h2f=0x%08x\n", tag, status, bridge);
  flush_log();
}

static void idle_fbi(const char *tag) {
  char buf[160];
  for (unsigned i = 0; i < 1000000; i++) {
    snprintf(buf, sizeof(buf), "idle_fbi %s read status iter=%u", tag, i);
    stage(buf);
    uint32_t status = rd32_fast(REG_STATUS);
    if ((status & SST_FBI_BUSY) == 0) {
      snprintf(buf, sizeof(buf), "idle_fbi %s done iter=%u status=0x%08x", tag, i, status);
      stage(buf);
      return;
    }
  }
  stage("idle_fbi timeout");
}

static void idle_full(const char *tag) {
  char buf[160];
  unsigned idle_count = 0;
  wr32(REG_NOPCMD, 0, "idle-full nop");
  for (unsigned i = 0; i < 1000000; i++) {
    snprintf(buf, sizeof(buf), "idle_full %s read status iter=%u", tag, i);
    stage(buf);
    uint32_t status = rd32_fast(REG_STATUS);
    if ((status & SST_BUSY) == 0) {
      idle_count++;
      if (idle_count >= 3) {
        snprintf(buf, sizeof(buf), "idle_full %s done iter=%u status=0x%08x", tag, i, status);
        stage(buf);
        return;
      }
    } else {
      idle_count = 0;
    }
  }
  stage("idle_full timeout");
}

static void draw_triangle_fast(int x, int y, int size) {
  const uint32_t xy_one = 1u << 4;
  wr32_fast(REG_VAX, (uint32_t)x);
  wr32_fast(REG_VAY, (uint32_t)y);
  wr32_fast(REG_VBX, (uint32_t)(x + (int)(xy_one * (uint32_t)size)));
  wr32_fast(REG_VBY, (uint32_t)y);
  wr32_fast(REG_VCX, (uint32_t)x);
  wr32_fast(REG_VCY, (uint32_t)(y + (int)(xy_one * (uint32_t)size)));
  wr32_fast(REG_S, 0);
  wr32_fast(REG_T, 0);
  wr32_fast(REG_W, 0);
  wr32_fast(REG_DSDX, 1u << 18);
  wr32_fast(REG_DTDX, 0);
  wr32_fast(REG_DWDX, 0);
  wr32_fast(REG_DSDY, 0);
  wr32_fast(REG_DTDY, 1u << 18);
  wr32_fast(REG_DWDY, 0);
  wr32_fast(REG_TRIANGLECMD, 0);
}

static uint32_t lfb_pixel_pair_offset(unsigned x, unsigned y) {
  return LFB_BASE + y * 2048u + x * 2u;
}

static void read_4x4(unsigned iter) {
  char buf[160];
  uint32_t sum = 0;
  wr32(REG_LFBMODE, 0, "read4x4-lfbMode");
  idle_full("read4x4 before lfb reads");
  for (unsigned y = 0; y < 4; y++) {
    for (unsigned x = 0; x < 4; x += 2) {
      uint32_t off = lfb_pixel_pair_offset(x, y);
      snprintf(buf, sizeof(buf), "init-sum iter=%u lfb-read x=%u y=%u off=0x%06x", iter, x, y, off);
      stage(buf);
      sum ^= rd32_fast(off);
      snprintf(buf, sizeof(buf), "init-sum iter=%u lfb-read-complete x=%u y=%u", iter, x, y);
      stage(buf);
    }
  }
  snprintf(buf, sizeof(buf), "init-sum iter=%u read4x4 xor=0x%08x", iter, sum);
  stage(buf);
}

static void seq_status(void) {
  stage("seq status begin");
  log_status("status");
  stage("seq status end");
}

static void seq_legacy_reset(void) {
  stage("seq legacy-reset begin");
  log_status("legacy-reset initial");

  wr32(REG_FBIINIT3, SST_FBIINIT3_DEFAULT, "fbiInit3 prelim");
  log_status("after fbiInit3 prelim");

  uint32_t init1 = rd32(REG_FBIINIT1, "fbiInit1 before video reset");
  wr32(REG_FBIINIT1, init1 | SST_VIDEO_RESET, "assert video reset");
  log_status("after assert video reset");

  uint32_t init0 = rd32(REG_FBIINIT0, "fbiInit0 before grx+pci reset");
  wr32(REG_FBIINIT0, init0 | SST_GRX_RESET | SST_PCI_FIFO_RESET, "assert grx+pci reset");
  idle_fbi("after assert grx+pci reset");

  init0 = rd32(REG_FBIINIT0, "fbiInit0 before release pci fifo");
  wr32(REG_FBIINIT0, init0 & ~SST_PCI_FIFO_RESET, "release pci fifo reset");
  idle_fbi("after release pci fifo reset");

  init0 = rd32(REG_FBIINIT0, "fbiInit0 before release grx");
  wr32(REG_FBIINIT0, init0 & ~SST_GRX_RESET, "release graphics reset");
  idle_fbi("after release graphics reset");

  wr32(REG_FBIINIT0, SST_FBIINIT0_DEFAULT, "fbiInit0 default");
  wr32(REG_FBIINIT1, SST_FBIINIT1_DEFAULT, "fbiInit1 default");
  wr32(REG_FBIINIT2, SST_FBIINIT2_DEFAULT, "fbiInit2 default");
  wr32(REG_FBIINIT3, SST_FBIINIT3_DEFAULT, "fbiInit3 default");
  wr32(REG_FBIINIT4, SST_FBIINIT4_DEFAULT, "fbiInit4 default");
  idle_fbi("after init defaults");

  wr32(REG_TREXINIT0, 0, "trex0 init0");
  idle_fbi("after trex0 init0");
  wr32(REG_TREXINIT1, 0, "trex0 init1");
  idle_fbi("after trex0 init1");

  init1 = rd32(REG_FBIINIT1, "fbiInit1 before lfb read enable");
  wr32(REG_FBIINIT1, init1 | SST_LFB_READ_EN, "enable lfb reads");
  uint32_t init4 = rd32(REG_FBIINIT4, "fbiInit4 before lfb read ahead");
  wr32(REG_FBIINIT4, init4 | SST_EN_LFB_RDAHEAD, "enable lfb read ahead");
  uint32_t init3 = rd32(REG_FBIINIT3, "fbiInit3 before alt mapping");
  wr32(REG_FBIINIT3, init3 | SST_ALT_REGMAPPING, "enable alt reg mapping");
  uint32_t init2 = rd32(REG_FBIINIT2, "fbiInit2 before dram refresh");
  wr32(REG_FBIINIT2, init2 | SST_EN_DRAM_REFRESH, "enable dram refresh");
  idle_fbi("after late init tweaks");

  log_status("legacy-reset final");
  stage("seq legacy-reset end");
}

static void seq_init_sum(unsigned count) {
  char buf[160];
  stage_u32("seq init-sum begin count", count);
  wr32(REG_FBZCOLORPATH, SST_RGBSEL_C1_CC_PASS, "init-sum fbzColorPath");
  wr32(REG_FBZMODE, SST_DRAWBUFFER_FRONT | SST_RGBWRMASK | SST_ENDITHER, "init-sum fbzMode");
  for (unsigned color = 0; color < count; color++) {
    uint32_t rgb = (color << 16) | (color << 8) | color;
    snprintf(buf, sizeof(buf), "init-sum iter=%u before c1", color);
    stage(buf);
    wr32_fast(REG_C1, rgb);
    snprintf(buf, sizeof(buf), "init-sum iter=%u before triangle", color);
    stage(buf);
    draw_triangle_fast(0, 0, 36);
    snprintf(buf, sizeof(buf), "init-sum iter=%u after triangle", color);
    stage(buf);
    read_4x4(color);
  }
  log_status("init-sum final");
  stage("seq init-sum end");
}

static void seq_tmu_info(void) {
  stage("seq tmu-info begin");
  seq_init_sum(256);
  wr32(REG_TREXINIT1, 1u << 18, "tmu-info trex0 config output");
  wr32(REG_FBZCOLORPATH, SST_TEXTURED_COLORPATH, "tmu-info fbzColorPath");
  wr32(REG_TEXBASEADDR, 0, "tmu-info texBaseAddr");
  wr32(REG_TEXTUREMODE, 0x00081a00u, "tmu-info textureMode AI88 pass");
  wr32(REG_TLOD, 0, "tmu-info tLOD");
  stage("tmu-info before config triangle");
  draw_triangle_fast(0, 0, 36);
  stage("tmu-info after config triangle");
  read_4x4(0x10000u);
  log_status("tmu-info final");
  stage("seq tmu-info end");
}

static void seq_fifo_tri(unsigned count) {
  char buf[160];
  stage_u32("seq fifo-tri begin count", count);
  wr32(REG_FBZCOLORPATH, SST_RGBSEL_C1_CC_PASS, "fifo-tri fbzColorPath");
  wr32(REG_FBZMODE, SST_DRAWBUFFER_FRONT | SST_RGBWRMASK | SST_ENDITHER, "fifo-tri fbzMode");
  wr32(REG_C1, 0x00010101u, "fifo-tri c1");
  for (unsigned i = 0; i < count; i++) {
    snprintf(buf, sizeof(buf), "fifo-tri iter=%u before triangle", i);
    stage(buf);
    draw_triangle_fast(0, 0, 36);
    snprintf(buf, sizeof(buf), "fifo-tri iter=%u after triangle", i);
    stage(buf);
  }
  log_status("fifo-tri final");
  stage("seq fifo-tri end");
}

static void seq_tex_oob_read(void) {
  stage("seq tex-oob-read begin");
  wr32(REG_TEXBASEADDR, 0x40000u, "tex-oob texBaseAddr 2MB");
  rd32(TEX_BASE + 0x7ffffcu, "tex-oob read top with base 2MB");
  log_status("tex-oob-read final");
  stage("seq tex-oob-read end");
}

static void seq_tex_oob_write(void) {
  stage("seq tex-oob-write begin");
  wr32(REG_TEXBASEADDR, 0x40000u, "tex-oob-write texBaseAddr 2MB");
  wr32(TEX_BASE + 0x7ffffcu, 0xfeed1234u, "tex-oob write top with base 2MB");
  log_status("tex-oob-write final");
  stage("seq tex-oob-write end");
}

static void seq_probe_write(uint32_t off, uint32_t value) {
  char buf[160];
  stage("seq probe-write begin");
  snprintf(buf, sizeof(buf), "probe-write span=0x%08x off=0x%06x value=0x%08x", mmio_span, off, value);
  stage(buf);
  if ((off & 3u) != 0 || off + 4u > mmio_span) {
    stage("probe-write rejected offset outside mapped span or unaligned");
    return;
  }
  wr32(off, value, "probe-write target");
  stage("seq probe-write end");
}

int main(int argc, char **argv) {
  void *raw;
  void *lfb_raw = MAP_FAILED;
  void *tex_raw = MAP_FAILED;
  void *mapped;
  const char *span_env;
  const char *reg_span_env;
  const char *multi_env;
  const char *split_env;
  const char *map_lfbtex_env;
  const char *unmap_reservation_env;

  if (argc < 2) {
    usage(argv[0]);
    return 2;
  }

  logf = fopen("/home/fpga/de10-glide-adv-test/wedge_probe.log", "w");
  if (logf == NULL) {
    perror("fopen log");
    return 1;
  }
  setvbuf(logf, NULL, _IOLBF, 0);

  span_env = getenv("DE10_PROBE_SPAN");
  if (span_env != NULL && span_env[0] != '\0') {
    mmio_span = parse_u32(span_env);
  }
  reg_span_env = getenv("DE10_PROBE_REG_SPAN");
  if (reg_span_env != NULL && reg_span_env[0] != '\0') {
    reg_span = parse_u32(reg_span_env);
  }
  multi_env = getenv("DE10_PROBE_MULTI");
  multi_mapping = multi_env != NULL && multi_env[0] != '\0';
  split_env = getenv("DE10_PROBE_SPLIT");
  split_mapping = split_env != NULL && split_env[0] != '\0';
  map_lfbtex_env = getenv("DE10_PROBE_MAP_LFBTEX");
  if (map_lfbtex_env != NULL && map_lfbtex_env[0] != '\0') {
    split_map_lfbtex = strcmp(map_lfbtex_env, "0") != 0;
  }
  unmap_reservation_env = getenv("DE10_PROBE_UNMAP_RESERVATION");
  split_unmap_reservation = unmap_reservation_env != NULL && unmap_reservation_env[0] != '\0';
  stage_u32("probe mmap span", mmio_span);
  stage_u32("probe split reg span", reg_span);
  stage_u32("probe multi mapping", (uint32_t)multi_mapping);
  stage_u32("probe split mapping", (uint32_t)split_mapping);
  stage_u32("probe split map lfbtex", (uint32_t)split_map_lfbtex);
  stage_u32("probe split unmap reservation", (uint32_t)split_unmap_reservation);

  stage("before open /dev/mem");
  mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (mem_fd < 0) {
    fprintf(logf, "open /dev/mem failed: %s\n", strerror(errno));
    flush_log();
    fclose(logf);
    return 1;
  }
  stage("after open /dev/mem");

  if (multi_mapping) {
    stage("before multi mmap regs");
    raw = mmap(NULL, reg_span, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, MMIO_BASE);
    if (raw == MAP_FAILED) {
      fprintf(logf, "multi regs mmap failed: %s\n", strerror(errno));
      flush_log();
      close(mem_fd);
      fclose(logf);
      return 1;
    }
    fprintf(logf, "after multi mmap regs raw=%p\n", raw);
    flush_log();

    stage("before multi mmap lfb");
    lfb_raw = mmap(NULL, 0x00400000u, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, MMIO_BASE + LFB_BASE);
    if (lfb_raw == MAP_FAILED) {
      fprintf(logf, "multi lfb mmap failed: %s\n", strerror(errno));
      flush_log();
      munmap(raw, reg_span);
      close(mem_fd);
      fclose(logf);
      return 1;
    }
    fprintf(logf, "after multi mmap lfb raw=%p\n", lfb_raw);
    flush_log();

    stage("before multi mmap tex");
    tex_raw = mmap(NULL, 0x00800000u, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, MMIO_BASE + TEX_BASE);
    if (tex_raw == MAP_FAILED) {
      fprintf(logf, "multi tex mmap failed: %s\n", strerror(errno));
      flush_log();
      munmap(lfb_raw, 0x00400000u);
      munmap(raw, reg_span);
      close(mem_fd);
      fclose(logf);
      return 1;
    }
    fprintf(logf, "after multi mmap tex raw=%p\n", tex_raw);
    flush_log();

    lfb_mmio = (volatile uint8_t *)lfb_raw;
    tex_mmio = (volatile uint8_t *)tex_raw;
    mmio_span = MMIO_SPAN;
  } else if (split_mapping) {
    stage("before reserve split virtual span");
    raw = mmap(NULL, MMIO_SPAN, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) {
      fprintf(logf, "reserve mmap failed: %s\n", strerror(errno));
      flush_log();
      close(mem_fd);
      fclose(logf);
      return 1;
    }
    fprintf(logf, "after reserve split virtual span raw=%p\n", raw);
    flush_log();

    if (split_unmap_reservation) {
      stage("before unmap split reservation");
      munmap(raw, MMIO_SPAN);
      stage("after unmap split reservation");
    }

    stage("before split mmap regs");
    mapped = mmap(raw, reg_span, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, mem_fd, MMIO_BASE);
    if (mapped == MAP_FAILED) {
      fprintf(logf, "split regs mmap failed: %s\n", strerror(errno));
      flush_log();
      munmap(raw, MMIO_SPAN);
      close(mem_fd);
      fclose(logf);
      return 1;
    }
    stage("after split mmap regs");

    if (split_map_lfbtex) {
      stage("before split mmap lfb");
      mapped = mmap((uint8_t *)raw + LFB_BASE,
                    0x00400000u,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_FIXED,
                    mem_fd,
                    MMIO_BASE + LFB_BASE);
      if (mapped == MAP_FAILED) {
        fprintf(logf, "split lfb mmap failed: %s\n", strerror(errno));
        flush_log();
        munmap(raw, MMIO_SPAN);
        close(mem_fd);
        fclose(logf);
        return 1;
      }
      stage("after split mmap lfb");

      stage("before split mmap tex");
      mapped = mmap((uint8_t *)raw + TEX_BASE,
                    0x00800000u,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_FIXED,
                    mem_fd,
                    MMIO_BASE + TEX_BASE);
      if (mapped == MAP_FAILED) {
        fprintf(logf, "split tex mmap failed: %s\n", strerror(errno));
        flush_log();
        munmap(raw, MMIO_SPAN);
        close(mem_fd);
        fclose(logf);
        return 1;
      }
      stage("after split mmap tex");
    } else {
      stage("skip split mmap lfb/tex");
    }
    mmio_span = MMIO_SPAN;
  } else {
    stage("before mmap /dev/mem");
    raw = mmap(NULL, mmio_span, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, MMIO_BASE);
    if (raw == MAP_FAILED) {
      fprintf(logf, "mmap failed: %s\n", strerror(errno));
      flush_log();
      close(mem_fd);
      fclose(logf);
      return 1;
    }
  }
  mmio = (volatile uint8_t *)raw;
  fprintf(logf, "after mmap /dev/mem raw=%p span=0x%08x\n", raw, mmio_span);
  flush_log();

  if (strcmp(argv[1], "legacy-reset") == 0) {
    seq_legacy_reset();
  } else if (strcmp(argv[1], "init-sum") == 0) {
    unsigned count = argc >= 3 ? (unsigned)strtoul(argv[2], NULL, 0) : 256u;
    seq_init_sum(count);
  } else if (strcmp(argv[1], "tmu-info") == 0) {
    seq_tmu_info();
  } else if (strcmp(argv[1], "fifo-tri") == 0) {
    unsigned count = argc >= 3 ? (unsigned)strtoul(argv[2], NULL, 0) : 4096u;
    seq_fifo_tri(count);
  } else if (strcmp(argv[1], "tex-oob-read") == 0) {
    seq_tex_oob_read();
  } else if (strcmp(argv[1], "tex-oob-write") == 0) {
    seq_tex_oob_write();
  } else if (strcmp(argv[1], "probe-write") == 0) {
    uint32_t off = argc >= 3 ? parse_u32(argv[2]) : REG_FBZCOLORPATH;
    uint32_t value = argc >= 4 ? parse_u32(argv[3]) : SST_RGBSEL_C1_CC_PASS;
    seq_probe_write(off, value);
  } else if (strcmp(argv[1], "status") == 0) {
    seq_status();
  } else {
    usage(argv[0]);
    if (multi_mapping) {
      munmap(raw, reg_span);
      munmap(lfb_raw, 0x00400000u);
      munmap(tex_raw, 0x00800000u);
    } else {
      munmap(raw, mmio_span);
    }
    fclose(logf);
    return 2;
  }

  stage("before munmap /dev/mem");
  if (multi_mapping) {
    munmap(raw, reg_span);
    munmap(lfb_raw, 0x00400000u);
    munmap(tex_raw, 0x00800000u);
  } else {
    munmap(raw, mmio_span);
  }
  stage("after munmap /dev/mem");
  close(mem_fd);
  stage("after close /dev/mem");
  fclose(logf);
  return 0;
}
