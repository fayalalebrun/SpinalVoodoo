#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_MMIO_BASE 0xc0000000u
#define DEFAULT_MMIO_SPAN 0x1000u

#define REG_IDENT 0x000u
#define REG_VERSION 0x004u
#define REG_CAPABILITY 0x008u
#define REG_CONTROL 0x00cu
#define REG_STATUS 0x010u

#define PORT_STRIDE 0x80u
#define FB_PORT_BASE 0x100u
#define TEX_PORT_BASE (FB_PORT_BASE + PORT_STRIDE)

#define PORT_CONTROL 0x00u
#define PORT_BASE_ADDR 0x04u
#define PORT_ADDRESS_MASK 0x08u
#define PORT_STRIDE_BYTES 0x0cu
#define PORT_TRANSFER_COUNT 0x10u
#define PORT_SEED 0x14u
#define PORT_STATUS 0x18u
#define PORT_ISSUED_COUNT 0x20u
#define PORT_COMPLETED_COUNT 0x24u
#define PORT_BYTES_LO 0x28u
#define PORT_WAIT_CYCLES_LO 0x30u
#define PORT_ACTIVE_CYCLES_LO 0x38u
#define PORT_READ_LAT_MIN 0x40u
#define PORT_READ_LAT_MAX 0x44u
#define PORT_READ_LAT_SUM_LO 0x48u
#define PORT_READ_SAMPLES 0x50u
#define PORT_BURST_WORDS 0x58u

#define IDENT_VALUE 0x44444231u

#define CTRL_START (1u << 0)
#define CTRL_STOP (1u << 1)
#define CTRL_CLEAR (1u << 2)

#define PORT_CTRL_ENABLE (1u << 0)
#define PORT_CTRL_READ (1u << 1)
#define PORT_CTRL_WRITE (1u << 2)
#define PORT_CTRL_RANDOM (1u << 3)
#define PORT_CTRL_CONTINUOUS (1u << 4)

#define FB_DEFAULT_BASE 0x3f000000u
#define FB_DEFAULT_MASK 0x003ffffcu
#define TEX_DEFAULT_BASE 0x3f400000u
#define TEX_DEFAULT_MASK 0x007ffffcu

enum scenario_kind {
  SC_LATENCY,
  SC_SEQ_READ,
  SC_BURST_READ_8,
  SC_BURST_READ_16,
  SC_SEQ_WRITE,
  SC_CONTENDED_READ,
  SC_CONTENDED_READ_WRITE,
  SC_CONTENDED_BURST_READ_8,
  SC_MIXED
};

struct port_plan {
  const char *name;
  uint32_t port_base;
  uint32_t control;
  uint32_t base_addr;
  uint32_t address_mask;
  uint32_t stride_bytes;
  uint32_t transfer_count;
  uint32_t seed;
  uint32_t burst_words;
};

struct port_stats {
  uint32_t control;
  uint32_t issued;
  uint32_t completed;
  uint64_t bytes;
  uint64_t wait_cycles;
  uint64_t active_cycles;
  uint32_t latency_min;
  uint32_t latency_max;
  uint64_t latency_sum;
  uint32_t latency_samples;
};

static void usage(const char *argv0) {
  fprintf(stderr,
          "Usage: %s [--mmio-base <addr>] [--scenario <name>] [--count <n>] [--timeout-ms <ms>] [--burst-words <n>]\n"
          "\n"
          "Scenarios:\n"
          "  latency         single-port read probe, 64-byte stride\n"
          "  seq-read        single-port sequential reads\n"
          "  burst-read-8    single-port 8-word burst reads\n"
          "  burst-read-16   single-port 16-word burst reads\n"
          "  seq-write       single-port sequential writes\n"
          "  contended-read  fb reads while tex reads\n"
          "  contended-burst-read-8  fb 8-word burst reads while tex reads\n"
          "  contended-read-write    fb reads while tex writes\n"
          "  mixed           fb mixed read/write while tex random reads\n",
          argv0);
}

static volatile uint32_t *map_regs(uint32_t phys_base, uint32_t span, void **mapped_out, size_t *map_size_out) {
  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    perror("sysconf(_SC_PAGESIZE)");
    return NULL;
  }

  uint64_t page_mask = (uint64_t)page_size - 1u;
  uint64_t map_base = phys_base & ~page_mask;
  uint64_t page_offset = phys_base - map_base;
  uint64_t map_size = page_offset + span;

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    perror("open(/dev/mem)");
    return NULL;
  }

  void *mapped = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)map_base);
  close(fd);
  if (mapped == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }

  *mapped_out = mapped;
  *map_size_out = (size_t)map_size;
  return (volatile uint32_t *)((volatile uint8_t *)mapped + page_offset);
}

static uint32_t reg_read32(volatile uint32_t *regs, uint32_t offset) {
  return regs[offset >> 2];
}

static uint64_t reg_read64(volatile uint32_t *regs, uint32_t offset) {
  uint64_t lo = reg_read32(regs, offset);
  uint64_t hi = reg_read32(regs, offset + 4u);
  return lo | (hi << 32);
}

static void reg_write32(volatile uint32_t *regs, uint32_t offset, uint32_t value) {
  regs[offset >> 2] = value;
}

static void configure_port(volatile uint32_t *regs, const struct port_plan *plan) {
  reg_write32(regs, plan->port_base + PORT_CONTROL, 0u);
  reg_write32(regs, plan->port_base + PORT_BASE_ADDR, plan->base_addr);
  reg_write32(regs, plan->port_base + PORT_ADDRESS_MASK, plan->address_mask);
  reg_write32(regs, plan->port_base + PORT_STRIDE_BYTES, plan->stride_bytes);
  reg_write32(regs, plan->port_base + PORT_TRANSFER_COUNT, plan->transfer_count);
  reg_write32(regs, plan->port_base + PORT_SEED, plan->seed);
  reg_write32(regs, plan->port_base + PORT_BURST_WORDS, plan->burst_words);
  reg_write32(regs, plan->port_base + PORT_CONTROL, plan->control);
}

static void collect_port(volatile uint32_t *regs, uint32_t port_base, struct port_stats *stats) {
  memset(stats, 0, sizeof(*stats));
  stats->control = reg_read32(regs, port_base + PORT_CONTROL);
  stats->issued = reg_read32(regs, port_base + PORT_ISSUED_COUNT);
  stats->completed = reg_read32(regs, port_base + PORT_COMPLETED_COUNT);
  stats->bytes = reg_read64(regs, port_base + PORT_BYTES_LO);
  stats->wait_cycles = reg_read64(regs, port_base + PORT_WAIT_CYCLES_LO);
  stats->active_cycles = reg_read64(regs, port_base + PORT_ACTIVE_CYCLES_LO);
  stats->latency_min = reg_read32(regs, port_base + PORT_READ_LAT_MIN);
  stats->latency_max = reg_read32(regs, port_base + PORT_READ_LAT_MAX);
  stats->latency_sum = reg_read64(regs, port_base + PORT_READ_LAT_SUM_LO);
  stats->latency_samples = reg_read32(regs, port_base + PORT_READ_SAMPLES);
}

static void print_port_report(const struct port_plan *plan, const struct port_stats *stats, uint32_t clock_mhz) {
  double throughput_mb_s = 0.0;
  double avg_latency = 0.0;
  double stall_pct = 0.0;

  if (stats->active_cycles != 0) {
    throughput_mb_s = ((double)stats->bytes * (double)(clock_mhz * 1000000u)) /
                      ((double)stats->active_cycles * 1024.0 * 1024.0);
    stall_pct = 100.0 * (double)stats->wait_cycles / (double)stats->active_cycles;
  }
  if (stats->latency_samples != 0) {
    avg_latency = (double)stats->latency_sum / (double)stats->latency_samples;
  }

  printf("%s:\n", plan->name);
  printf("  control=0x%08" PRIx32 " burst_words=%" PRIu32 " issued=%" PRIu32 " completed=%" PRIu32 "\n",
         stats->control,
         plan->burst_words,
         stats->issued,
         stats->completed);
  printf("  bytes=%" PRIu64 " active_cycles=%" PRIu64 " wait_cycles=%" PRIu64 " throughput=%.2f MiB/s stall=%.2f%%\n",
         stats->bytes,
         stats->active_cycles,
         stats->wait_cycles,
         throughput_mb_s,
         stall_pct);
  if (stats->latency_samples != 0) {
    printf("  read_latency_cycles min=%" PRIu32 " avg=%.2f max=%" PRIu32 " samples=%" PRIu32 "\n",
           stats->latency_min,
           avg_latency,
           stats->latency_max,
           stats->latency_samples);
  }
}

static enum scenario_kind parse_scenario(const char *name) {
  if (strcmp(name, "latency") == 0) return SC_LATENCY;
  if (strcmp(name, "seq-read") == 0) return SC_SEQ_READ;
  if (strcmp(name, "burst-read-8") == 0) return SC_BURST_READ_8;
  if (strcmp(name, "burst-read-16") == 0) return SC_BURST_READ_16;
  if (strcmp(name, "seq-write") == 0) return SC_SEQ_WRITE;
  if (strcmp(name, "contended-read") == 0) return SC_CONTENDED_READ;
  if (strcmp(name, "contended-read-write") == 0) return SC_CONTENDED_READ_WRITE;
  if (strcmp(name, "contended-burst-read-8") == 0) return SC_CONTENDED_BURST_READ_8;
  if (strcmp(name, "mixed") == 0) return SC_MIXED;
  fprintf(stderr, "Unknown scenario: %s\n", name);
  exit(2);
}

static void make_plans(enum scenario_kind scenario,
                       uint32_t count,
                       uint32_t burst_override,
                       struct port_plan *fb,
                       struct port_plan *tex) {
  memset(fb, 0, sizeof(*fb));
  memset(tex, 0, sizeof(*tex));

  *fb = (struct port_plan){
      .name = "fb",
      .port_base = FB_PORT_BASE,
      .control = 0,
      .base_addr = FB_DEFAULT_BASE,
      .address_mask = FB_DEFAULT_MASK,
      .stride_bytes = 4u,
      .transfer_count = count,
      .seed = 0x1badb002u,
      .burst_words = 1u,
  };
  *tex = (struct port_plan){
      .name = "tex",
      .port_base = TEX_PORT_BASE,
      .control = 0,
      .base_addr = TEX_DEFAULT_BASE,
      .address_mask = TEX_DEFAULT_MASK,
      .stride_bytes = 4u,
      .transfer_count = count,
      .seed = 0x00c0ffeeu,
      .burst_words = 1u,
  };

  switch (scenario) {
    case SC_LATENCY:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      fb->stride_bytes = 64u;
      break;
    case SC_SEQ_READ:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      break;
    case SC_BURST_READ_8:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      fb->burst_words = 8u;
      break;
    case SC_BURST_READ_16:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      fb->burst_words = 16u;
      break;
    case SC_SEQ_WRITE:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_WRITE;
      break;
    case SC_CONTENDED_READ:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      tex->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      break;
    case SC_CONTENDED_READ_WRITE:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      tex->control = PORT_CTRL_ENABLE | PORT_CTRL_WRITE;
      break;
    case SC_CONTENDED_BURST_READ_8:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      fb->burst_words = 8u;
      tex->control = PORT_CTRL_ENABLE | PORT_CTRL_READ;
      tex->burst_words = 8u;
      break;
    case SC_MIXED:
      fb->control = PORT_CTRL_ENABLE | PORT_CTRL_READ | PORT_CTRL_WRITE;
      tex->control = PORT_CTRL_ENABLE | PORT_CTRL_READ | PORT_CTRL_RANDOM;
      tex->stride_bytes = 64u;
      break;
  }

  if (burst_override != 0u) {
    fb->burst_words = burst_override;
    if ((tex->control & PORT_CTRL_READ) != 0u) {
      tex->burst_words = burst_override;
    }
  }
}

int main(int argc, char **argv) {
  uint32_t mmio_base = DEFAULT_MMIO_BASE;
  uint32_t mmio_span = DEFAULT_MMIO_SPAN;
  uint32_t count = 1u << 20;
  uint32_t timeout_ms = 5000u;
  uint32_t burst_override = 0u;
  enum scenario_kind scenario = SC_CONTENDED_READ;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--mmio-base") == 0 && i + 1 < argc) {
      mmio_base = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
      scenario = parse_scenario(argv[++i]);
    } else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
      count = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--burst-words") == 0 && i + 1 < argc) {
      burst_override = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "--timeout-ms") == 0 && i + 1 < argc) {
      timeout_ms = (uint32_t)strtoul(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    } else {
      usage(argv[0]);
      return 2;
    }
  }

  if (scenario == SC_LATENCY && count == (1u << 20)) count = 4096u;

  void *mapped = NULL;
  size_t map_size = 0;
  volatile uint32_t *regs = map_regs(mmio_base, mmio_span, &mapped, &map_size);
  if (regs == NULL) return 1;

  uint32_t ident = reg_read32(regs, REG_IDENT);
  uint32_t version = reg_read32(regs, REG_VERSION);
  uint32_t capability = reg_read32(regs, REG_CAPABILITY);
  uint32_t clock_mhz = (capability >> 24) & 0xffu;

  if (ident != IDENT_VALUE) {
    fprintf(stderr, "Unexpected bench ID 0x%08" PRIx32 " at 0x%08" PRIx32 "\n", ident, mmio_base);
    munmap(mapped, map_size);
    return 1;
  }

  struct port_plan fb_plan;
  struct port_plan tex_plan;
  struct port_stats fb_stats;
  struct port_stats tex_stats;
  make_plans(scenario, count, burst_override, &fb_plan, &tex_plan);

  reg_write32(regs, REG_CONTROL, CTRL_STOP);
  reg_write32(regs, REG_CONTROL, CTRL_CLEAR);
  configure_port(regs, &fb_plan);
  configure_port(regs, &tex_plan);
  reg_write32(regs, REG_CONTROL, CTRL_START);

  uint32_t elapsed_ms = 0;
  while (((reg_read32(regs, REG_STATUS) & 0x2u) == 0u) && elapsed_ms < timeout_ms) {
    usleep(1000);
    elapsed_ms++;
  }

  if ((reg_read32(regs, REG_STATUS) & 0x2u) == 0u) {
    fprintf(stderr, "Timed out after %" PRIu32 " ms, stopping benchmark\n", timeout_ms);
    reg_write32(regs, REG_CONTROL, CTRL_STOP);
  }

  collect_port(regs, FB_PORT_BASE, &fb_stats);
  collect_port(regs, TEX_PORT_BASE, &tex_stats);

  printf("DDR bench @ 0x%08" PRIx32 " version=0x%08" PRIx32 " capability=0x%08" PRIx32 "\n",
         mmio_base,
         version,
         capability);
  print_port_report(&fb_plan, &fb_stats, clock_mhz != 0 ? clock_mhz : 50u);
  print_port_report(&tex_plan, &tex_stats, clock_mhz != 0 ? clock_mhz : 50u);
  printf("Hint: rerun `--scenario latency` while a CPU DDR workload is active on Linux to capture shared-controller interference.\n");

  munmap(mapped, map_size);
  return 0;
}
