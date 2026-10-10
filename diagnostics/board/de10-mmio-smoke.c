#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_SPAN 0x1000u

static void usage(const char *argv0) {
  fprintf(stderr,
          "Usage: %s <phys-base-addr> [--span <bytes>] [--read-offset <offset>] [--write32 <offset> <value>] [--write-test] [--swap-test <count>] [--fastfill-test <width> <height>] [--point-test <count>] [--fpoint-test <count>]\n"
          "\n"
          "Examples:\n"
          "  %s 0xc0000000\n"
          "  %s 0xc0000000 --read-offset 0x214\n"
          "  %s 0xc0000000 --span 0x200000 --write-test\n"
          "  %s 0xc0000000 --write32 0x088 0x3f800000 --read-offset 0x254\n"
          "  %s 0xc0000000 --swap-test 16\n"
          "  %s 0xc0000000 --fastfill-test 4 4\n"
          "  %s 0xc0000000 --point-test 100\n"
          "  %s 0xc0000000 --fpoint-test 100\n",
           argv0, argv0, argv0, argv0, argv0, argv0, argv0, argv0, argv0);
}

static uint32_t reg_read(volatile uint8_t *base, uint32_t offset) {
  volatile uint32_t *reg = (volatile uint32_t *)(base + offset);
  return *reg;
}

static void reg_write(volatile uint8_t *base, uint32_t offset, uint32_t value) {
  volatile uint32_t *reg = (volatile uint32_t *)(base + offset);
  *reg = value;
}

static void reg_write_float(volatile uint8_t *base, uint32_t offset, float value) {
  volatile float *reg = (volatile float *)(base + offset);
  *reg = value;
}

int main(int argc, char **argv) {
  uint64_t phys_base = 0;
  uint64_t span = DEFAULT_SPAN;
  bool write_test = false;
  uint32_t swap_test_count = 0;
  uint32_t fastfill_width = 0;
  uint32_t fastfill_height = 0;
  uint32_t point_test_count = 0;
  uint32_t fpoint_test_count = 0;
  uint32_t read_offsets[64];
  size_t read_offset_count = 0;
  struct {
    uint32_t offset;
    uint32_t value;
  } write_ops[64];
  size_t write_op_count = 0;
  struct {
    uint32_t offset;
    float value;
  } writef_ops[64];
  size_t writef_op_count = 0;

  if (argc < 2) {
    usage(argv[0]);
    return 2;
  }

  phys_base = strtoull(argv[1], NULL, 0);

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--span") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Missing value for --span\n");
        return 2;
      }
      span = strtoull(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--read-offset") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Missing value for --read-offset\n");
        return 2;
      }
      if (read_offset_count >= (sizeof(read_offsets) / sizeof(read_offsets[0]))) {
        fprintf(stderr, "Too many --read-offset arguments\n");
        return 2;
      }
      read_offsets[read_offset_count++] = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--write-test") == 0) {
      write_test = true;
    } else if (strcmp(argv[i], "--write32") == 0) {
      if (i + 2 >= argc) {
        fprintf(stderr, "Missing values for --write32\n");
        return 2;
      }
      if (write_op_count >= (sizeof(write_ops) / sizeof(write_ops[0]))) {
        fprintf(stderr, "Too many --write32 arguments\n");
        return 2;
      }
      write_ops[write_op_count].offset = (uint32_t)strtoul(argv[++i], NULL, 0);
      write_ops[write_op_count].value = (uint32_t)strtoul(argv[++i], NULL, 0);
      write_op_count++;
    } else if (strcmp(argv[i], "--writef32") == 0) {
      if (i + 2 >= argc) {
        fprintf(stderr, "Missing values for --writef32\n");
        return 2;
      }
      if (writef_op_count >= (sizeof(writef_ops) / sizeof(writef_ops[0]))) {
        fprintf(stderr, "Too many --writef32 arguments\n");
        return 2;
      }
      writef_ops[writef_op_count].offset = (uint32_t)strtoul(argv[++i], NULL, 0);
      writef_ops[writef_op_count].value = strtof(argv[++i], NULL);
      writef_op_count++;
    } else if (strcmp(argv[i], "--swap-test") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Missing value for --swap-test\n");
        return 2;
      }
      swap_test_count = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--fastfill-test") == 0) {
      if (i + 2 >= argc) {
        fprintf(stderr, "Missing values for --fastfill-test\n");
        return 2;
      }
      fastfill_width = (uint32_t)strtoul(argv[++i], NULL, 0);
      fastfill_height = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--point-test") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Missing value for --point-test\n");
        return 2;
      }
      point_test_count = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--fpoint-test") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Missing value for --fpoint-test\n");
        return 2;
      }
      fpoint_test_count = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      usage(argv[0]);
      return 2;
    }
  }

  if (span < DEFAULT_SPAN) {
    fprintf(stderr, "Span must be at least 0x%x bytes\n", DEFAULT_SPAN);
    return 2;
  }

  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    perror("sysconf(_SC_PAGESIZE)");
    return 1;
  }

  uint64_t page_mask = (uint64_t)page_size - 1u;
  uint64_t map_base = phys_base & ~page_mask;
  uint64_t page_offset = phys_base - map_base;
  uint64_t map_size = page_offset + span;

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    perror("open(/dev/mem)");
    return 1;
  }

  void *mapped = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)map_base);
  if (mapped == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }

  volatile uint8_t *regs = (volatile uint8_t *)mapped + page_offset;

  // Common Voodoo register offsets in BAR0 region.
  const uint32_t status_off = 0x000;
  const uint32_t v_retrace_off = 0x204;
  const uint32_t pixels_in_off = 0x14c;
  const uint32_t pixels_out_off = 0x15c;
  const uint32_t color0_off = 0x144;
  const uint32_t color1_off = 0x148;
  const uint32_t fbz_mode_off = 0x110;
  const uint32_t clip_left_right_off = 0x118;
  const uint32_t clip_low_y_high_y_off = 0x11c;
  const uint32_t vertex_ax_off = 0x008;
  const uint32_t vertex_ay_off = 0x00c;
  const uint32_t vertex_bx_off = 0x010;
  const uint32_t vertex_by_off = 0x014;
  const uint32_t vertex_cx_off = 0x018;
  const uint32_t vertex_cy_off = 0x01c;
  const uint32_t fvertex_ax_off = 0x088;
  const uint32_t fvertex_ay_off = 0x08c;
  const uint32_t fvertex_bx_off = 0x090;
  const uint32_t fvertex_by_off = 0x094;
  const uint32_t fvertex_cx_off = 0x098;
  const uint32_t fvertex_cy_off = 0x09c;
  const uint32_t triangle_cmd_off = 0x080;
  const uint32_t ftriangle_cmd_off = 0x100;
  const uint32_t nop_cmd_off = 0x120;
  const uint32_t swapbuffer_cmd_off = 0x128;
  const uint32_t fastfill_cmd_off = 0x124;

  uint32_t status = reg_read(regs, status_off);
  uint32_t v_retrace = reg_read(regs, v_retrace_off);
  uint32_t pixels_in = reg_read(regs, pixels_in_off);
  uint32_t pixels_out = reg_read(regs, pixels_out_off);

  printf("MMIO base   : 0x%" PRIx64 "\n", phys_base);
  printf("MMIO span   : 0x%" PRIx64 "\n", span);
  printf("status      : 0x%08" PRIx32 "\n", status);
  printf("vRetrace    : 0x%08" PRIx32 "\n", v_retrace);
  printf("fbiPixelsIn : 0x%08" PRIx32 "\n", pixels_in);
  printf("fbiPixelsOut: 0x%08" PRIx32 "\n", pixels_out);

  for (size_t idx = 0; idx < read_offset_count; idx++) {
    uint32_t off = read_offsets[idx];
    uint32_t value = reg_read(regs, off);
    printf("read[0x%03" PRIx32 "] : 0x%08" PRIx32 "\n", off, value);
  }

  if (write_test) {
    const uint32_t pattern = 0x00AA55CCu;
    reg_write(regs, color0_off, pattern);
    __sync_synchronize();
    uint32_t color0 = reg_read(regs, color0_off);
    printf("write-test  : color0 <= 0x%08" PRIx32 ", readback 0x%08" PRIx32 "\n", pattern, color0);
  }

  for (size_t idx = 0; idx < write_op_count; idx++) {
    reg_write(regs, write_ops[idx].offset, write_ops[idx].value);
    __sync_synchronize();
    printf("write32     : [0x%03" PRIx32 "] <= 0x%08" PRIx32 "\n",
           write_ops[idx].offset,
           write_ops[idx].value);
  }

  for (size_t idx = 0; idx < writef_op_count; idx++) {
    union {
      float f;
      uint32_t u;
    } bits;
    bits.f = writef_ops[idx].value;
    reg_write_float(regs, writef_ops[idx].offset, writef_ops[idx].value);
    __sync_synchronize();
    printf("writef32    : [0x%03" PRIx32 "] <= %f (0x%08" PRIx32 ")\n",
           writef_ops[idx].offset,
           writef_ops[idx].value,
           bits.u);
  }

  if (swap_test_count != 0) {
    const uint32_t busy_mask = (1u << 7) | (1u << 8) | (1u << 9);
    uint32_t initial_status = reg_read(regs, status_off);
    uint32_t initial_display = (initial_status >> 10) & 0x3u;
    uint32_t observed_busy = 0;
    uint32_t display_changes = 0;
    uint32_t last_display = initial_display;

    for (uint32_t swap = 0; swap < swap_test_count; swap++) {
      reg_write(regs, swapbuffer_cmd_off, 0u);
      __sync_synchronize();

      for (uint32_t poll = 0; poll < 50000; poll++) {
        uint32_t cur_status = reg_read(regs, status_off);
        uint32_t cur_display = (cur_status >> 10) & 0x3u;
        observed_busy |= cur_status & busy_mask;
        if (cur_display != last_display) {
          display_changes++;
          last_display = cur_display;
          break;
        }
      }
    }

    uint32_t final_status = reg_read(regs, status_off);
    printf("swap-test   : requested=%" PRIu32 ", display_changes=%" PRIu32 ", initial_display=%" PRIu32 ", final_display=%" PRIu32 ", busy_seen=0x%03" PRIx32 "\n",
           swap_test_count,
           display_changes,
           initial_display,
           (final_status >> 10) & 0x3u,
           observed_busy >> 7);
  }

  if (fastfill_width != 0 && fastfill_height != 0) {
    uint32_t old_fbz_mode = reg_read(regs, fbz_mode_off);
    uint32_t old_clip_lr = reg_read(regs, clip_left_right_off);
    uint32_t old_clip_y = reg_read(regs, clip_low_y_high_y_off);
    uint32_t old_color1 = reg_read(regs, color1_off);
    uint32_t before_pixels_in = reg_read(regs, pixels_in_off);
    uint32_t before_pixels_out = reg_read(regs, pixels_out_off);

    reg_write(regs, fbz_mode_off, 1u << 9);
    reg_write(regs, color1_off, 0x0000ff00u);
    reg_write(regs, clip_left_right_off, fastfill_width & 0x3ffu);
    reg_write(regs, clip_low_y_high_y_off, fastfill_height & 0x3ffu);
    reg_write(regs, fastfill_cmd_off, 0u);
    reg_write(regs, nop_cmd_off, 0u);
    __sync_synchronize();

    uint32_t after_pixels_in = reg_read(regs, pixels_in_off);
    uint32_t after_pixels_out = reg_read(regs, pixels_out_off);

    printf("fastfill    : %" PRIu32 "x%" PRIu32 ", pixelsIn delta=%" PRIu32 ", pixelsOut delta=%" PRIu32 "\n",
           fastfill_width,
           fastfill_height,
           after_pixels_in - before_pixels_in,
           after_pixels_out - before_pixels_out);

    reg_write(regs, fbz_mode_off, old_fbz_mode);
    reg_write(regs, clip_left_right_off, old_clip_lr);
    reg_write(regs, clip_low_y_high_y_off, old_clip_y);
    reg_write(regs, color1_off, old_color1);
  }

  if (point_test_count != 0) {
    for (uint32_t i = 0; i < point_test_count; i++) {
      uint32_t x = 0x00000008u + ((i & 0x3ffu) << 4);
      uint32_t y = 0x00000008u + ((i & 0x3ffu) << 4);
      reg_write(regs, vertex_ax_off, x);
      reg_write(regs, vertex_ay_off, y);
      reg_write(regs, vertex_bx_off, x + 0x10u);
      reg_write(regs, vertex_by_off, y);
      reg_write(regs, vertex_cx_off, x + 0x10u);
      reg_write(regs, vertex_cy_off, y + 0x10u);
      reg_write(regs, triangle_cmd_off, 1u);
    }
    printf("point-test  : count=%" PRIu32 "\n", point_test_count);
  }

  if (fpoint_test_count != 0) {
    for (uint32_t i = 0; i < fpoint_test_count; i++) {
      float p = (float)i;
      union {
        float f;
        uint32_t u;
      } ax, ay, bx, by, cx, cy;
      ax.f = p;
      ay.f = p;
      bx.f = p + 1.0f;
      by.f = p;
      cx.f = p + 1.0f;
      cy.f = p + 1.0f;
      reg_write(regs, fvertex_ax_off, ax.u);
      reg_write(regs, fvertex_ay_off, ay.u);
      reg_write(regs, fvertex_bx_off, bx.u);
      reg_write(regs, fvertex_by_off, by.u);
      reg_write(regs, fvertex_cx_off, cx.u);
      reg_write(regs, fvertex_cy_off, cy.u);
      reg_write(regs, ftriangle_cmd_off, 1u);
    }
    printf("fpoint-test : count=%" PRIu32 "\n", fpoint_test_count);
  }

  if (munmap(mapped, map_size) != 0) {
    perror("munmap");
  }
  close(fd);
  return 0;
}
