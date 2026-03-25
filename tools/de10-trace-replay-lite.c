#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "voodoo_trace_format.h"

#define MMIO_BASE 0xC0000000u
#define MMIO_SPAN 0x1000u
#define FB_BASE 0x3F000000u
#define TEX_BASE (FB_BASE + 0x00400000u)
#define FB_BYTES (4u * 1024u * 1024u)
#define TEX_BYTES (8u * 1024u * 1024u)

typedef struct {
  void *raw;
  volatile uint8_t *base;
  size_t size;
} map_region_t;

typedef struct {
  map_region_t region;
  uint32_t page_base;
} page_cache_t;

static uint8_t *read_file(const char *path, uint32_t *out_size) {
  FILE *f = fopen(path, "rb");
  long sz;
  uint8_t *buf;
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  sz = ftell(f);
  if (sz < 0 || fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }
  buf = (uint8_t *)malloc((size_t)sz);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);
  *out_size = (uint32_t)sz;
  return buf;
}

static int is_dir(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int map_region(int fd, uint32_t phys, uint32_t span, map_region_t *out) {
  long page_size = sysconf(_SC_PAGESIZE);
  uint64_t page_mask;
  uint64_t map_base;
  uint64_t page_off;
  uint64_t map_size;
  void *raw;
  if (page_size <= 0) return -1;
  page_mask = (uint64_t)page_size - 1u;
  map_base = phys & ~page_mask;
  page_off = phys - map_base;
  map_size = page_off + span;
  raw = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)map_base);
  if (raw == MAP_FAILED) return -1;
  out->raw = raw;
  out->base = (volatile uint8_t *)raw + page_off;
  out->size = (size_t)map_size;
  return 0;
}

static void unmap_region(map_region_t *region) {
  if (region->raw && region->raw != MAP_FAILED) {
    munmap(region->raw, region->size);
  }
  region->raw = MAP_FAILED;
  region->base = NULL;
  region->size = 0;
}

static int ensure_page(int fd, uint32_t phys, page_cache_t *cache) {
  uint32_t page_base = phys & ~0xfffu;
  void *raw;
  if (cache->region.raw != MAP_FAILED && cache->page_base == page_base) return 0;
  unmap_region(&cache->region);
  raw = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)page_base);
  if (raw == MAP_FAILED) return -1;
  cache->region.raw = raw;
  cache->region.base = (volatile uint8_t *)raw;
  cache->region.size = 0x1000;
  cache->page_base = page_base;
  return 0;
}

static int phys_write_all(int fd, page_cache_t *cache, uint32_t phys, const void *src, size_t size) {
  const uint8_t *p = (const uint8_t *)src;
  size_t done = 0;
  while (done < size) {
    uint32_t addr = phys + (uint32_t)done;
    uint32_t off = addr & 0xfffu;
    size_t chunk = size - done;
    if (chunk > (size_t)(0x1000u - off)) chunk = 0x1000u - off;
    if (ensure_page(fd, addr, cache) != 0) return -1;
    memcpy((void *)(cache->region.base + off), p + done, chunk);
    done += chunk;
  }
  return 0;
}

static inline uint32_t mmio_read32(volatile uint8_t *mmio, uint32_t addr) {
  return *(volatile uint32_t *)(mmio + addr);
}

static inline void mmio_write32(volatile uint8_t *mmio, uint32_t addr, uint32_t value) {
  *(volatile uint32_t *)(mmio + addr) = value;
}

static inline void mmio_write16(volatile uint8_t *mmio, uint32_t addr, uint16_t value) {
  *(volatile uint16_t *)(mmio + addr) = value;
}

static uint32_t remap_backend_reg_addr(uint32_t addr) {
  uint32_t base = addr & 0x3FFFFFu;
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

static uint32_t wait_quiescent(volatile uint8_t *mmio, const char *tag) {
  int stable = 0;
  int i;
  for (i = 0; i < 5000000; ++i) {
    uint32_t status = mmio_read32(mmio, 0x000);
    uint32_t fifo_free = status & 0x3fu;
    uint32_t swaps_pending = (status >> 28) & 0x7u;
    if (fifo_free == 0x3fu && swaps_pending == 0) {
      if (++stable >= 128) {
        printf("[de10-trace-lite] %s quiescent status=0x%08x\n", tag, status);
        fflush(stdout);
        return status;
      }
    } else {
      stable = 0;
    }
  }
  printf("[de10-trace-lite] WARNING: %s timeout status=0x%08x\n", tag, mmio_read32(mmio, 0x000));
  fflush(stdout);
  return mmio_read32(mmio, 0x000);
}

int main(int argc, char **argv) {
  const char *input;
  const char *trace_file;
  const char *state_file = NULL;
  uint32_t trace_size = 0;
  uint32_t state_size = 0;
  uint8_t *trace_data;
  uint8_t *state_data = NULL;
  const voodoo_trace_header_t *hdr;
  const voodoo_trace_entry_t *entries;
  uint32_t num_entries;
  int fd;
  map_region_t mmio = {MAP_FAILED, NULL, 0};
  page_cache_t fb_cache = {{MAP_FAILED, NULL, 0}, 0xffffffffu};
  page_cache_t tex_cache = {{MAP_FAILED, NULL, 0}, 0xffffffffu};
  uint32_t i;
  uint32_t reg_writes = 0;
  uint32_t tex_writes = 0;
  uint32_t fb_writes = 0;
  uint32_t swap_writes = 0;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <trace.bin|trace-dir>\n", argv[0]);
    return 2;
  }

  input = argv[1];
  if (is_dir(input)) {
    static char trace_buf[512];
    static char state_buf[512];
    snprintf(trace_buf, sizeof(trace_buf), "%s/trace.bin", input);
    snprintf(state_buf, sizeof(state_buf), "%s/state.bin", input);
    trace_file = trace_buf;
    state_file = state_buf;
  } else {
    trace_file = input;
  }

  trace_data = read_file(trace_file, &trace_size);
  if (!trace_data) {
    fprintf(stderr, "failed to read %s\n", trace_file);
    return 1;
  }
  hdr = (const voodoo_trace_header_t *)trace_data;
  if (trace_size < sizeof(*hdr) || hdr->magic != VOODOO_TRACE_MAGIC) {
    fprintf(stderr, "bad trace\n");
    free(trace_data);
    return 1;
  }
  entries = (const voodoo_trace_entry_t *)(trace_data + sizeof(*hdr));
  num_entries = (trace_size - sizeof(*hdr)) / sizeof(*entries);
  printf("[de10-trace-lite] Loaded %s (%u entries)\n", trace_file, num_entries);
  fflush(stdout);

  if (state_file) {
    state_data = read_file(state_file, &state_size);
    if (!state_data) {
      fprintf(stderr, "failed to read %s\n", state_file);
      free(trace_data);
      return 1;
    }
  }

  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    perror("open(/dev/mem)");
    free(state_data);
    free(trace_data);
    return 1;
  }
  if (map_region(fd, MMIO_BASE, MMIO_SPAN, &mmio) != 0) {
    perror("mmap(mmio)");
    close(fd);
    free(state_data);
    free(trace_data);
    return 1;
  }

  if (state_data) {
    const voodoo_state_header_t *shdr = (const voodoo_state_header_t *)state_data;
    const uint8_t *ptr;
    const voodoo_state_reg_t *regs;
    if (state_size < sizeof(*shdr) || shdr->magic != VOODOO_STATE_MAGIC || shdr->version != VOODOO_STATE_VERSION) {
      fprintf(stderr, "bad state\n");
      unmap_region(&mmio);
      close(fd);
      free(state_data);
      free(trace_data);
      return 1;
    }
    ptr = state_data + sizeof(*shdr);
    regs = (const voodoo_state_reg_t *)ptr;
    printf("[de10-trace-lite] Loading state regs=%u fb=%u tex=%u\n", shdr->reg_count, shdr->fb_size, shdr->tex_size);
    fflush(stdout);
    for (i = 0; i < shdr->reg_count; ++i) {
      uint32_t reg = regs[i].addr & 0x3fcu;
      if (reg == 0x080u || reg == 0x100u || reg == 0x120u || reg == 0x124u || reg == 0x128u) continue;
      mmio_write32(mmio.base, remap_backend_reg_addr(regs[i].addr), regs[i].value);
    }
    ptr += shdr->reg_count * sizeof(*regs);
    if (shdr->fb_size > FB_BYTES || shdr->tex_size > TEX_BYTES) {
      fprintf(stderr, "state bulk too large\n");
      unmap_region(&mmio);
      close(fd);
      free(state_data);
      free(trace_data);
      return 1;
    }
    if (phys_write_all(fd, &fb_cache, FB_BASE, ptr, shdr->fb_size) != 0) {
      perror("phys_write_all(fb)");
      return 1;
    }
    ptr += shdr->fb_size;
    if (phys_write_all(fd, &tex_cache, TEX_BASE, ptr, shdr->tex_size) != 0) {
      perror("phys_write_all(tex)");
      return 1;
    }
    wait_quiescent(mmio.base, "state-load");
  }

  for (i = 0; i < num_entries; ++i) {
    const voodoo_trace_entry_t *e = &entries[i];
    uint32_t repeat = e->count ? e->count : 1u;
    uint32_t rep;
    for (rep = 0; rep < repeat; ++rep) {
      switch (e->cmd_type) {
        case VOODOO_TRACE_WRITE_REG_L: {
          uint32_t addr = remap_backend_reg_addr(e->addr);
          uint32_t reg = addr & 0x3fcu;
          if (reg == 0x128u) wait_quiescent(mmio.base, "pre-swap");
          mmio_write32(mmio.base, addr, e->data);
          if (reg == 0x128u) {
            wait_quiescent(mmio.base, "post-swap");
            swap_writes++;
          }
          reg_writes++;
          break;
        }
        case VOODOO_TRACE_WRITE_REG_W:
          mmio_write16(mmio.base, remap_backend_reg_addr(e->addr), (uint16_t)e->data);
          reg_writes++;
          break;
        case VOODOO_TRACE_WRITE_TEX_L:
          if (phys_write_all(fd, &tex_cache, TEX_BASE + (e->addr & 0x7fffffu), &e->data, sizeof(e->data)) != 0) return 1;
          tex_writes++;
          break;
        case VOODOO_TRACE_WRITE_FB_L:
          if (phys_write_all(fd, &fb_cache, FB_BASE + (e->addr & 0x3fffffu), &e->data, sizeof(e->data)) != 0) return 1;
          fb_writes++;
          break;
        case VOODOO_TRACE_WRITE_FB_W: {
          uint16_t value = (uint16_t)e->data;
          if (phys_write_all(fd, &fb_cache, FB_BASE + (e->addr & 0x3fffffu), &value, sizeof(value)) != 0) return 1;
          fb_writes++;
          break;
        }
        default:
          break;
      }
    }
    if ((i % 4096u) == 0u && i != 0u) {
      printf("[de10-trace-lite] Progress %u/%u status=0x%08x\n", i, num_entries, mmio_read32(mmio.base, 0x000));
      fflush(stdout);
    }
  }

  wait_quiescent(mmio.base, "final");
  printf("[de10-trace-lite] Replay complete reg=%u tex=%u fb=%u swaps=%u status=0x%08x pixelsIn=%u pixelsOut=%u\n",
         reg_writes,
         tex_writes,
         fb_writes,
         swap_writes,
         mmio_read32(mmio.base, 0x000),
         mmio_read32(mmio.base, 0x14c),
         mmio_read32(mmio.base, 0x15c));
  fflush(stdout);

  unmap_region(&mmio);
  unmap_region(&fb_cache.region);
  unmap_region(&tex_cache.region);
  close(fd);
  free(state_data);
  free(trace_data);
  return 0;
}
