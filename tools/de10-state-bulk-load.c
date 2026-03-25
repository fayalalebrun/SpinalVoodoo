#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "voodoo_trace_format.h"

#define FB_BASE 0x3F000000u
#define TEX_BASE (FB_BASE + 0x00400000u)

typedef struct {
  void *raw;
  volatile uint8_t *base;
  size_t size;
  uint32_t page_base;
} page_cache_t;

static void unmap_page(page_cache_t *cache) {
  if (cache->raw && cache->raw != MAP_FAILED) munmap(cache->raw, cache->size);
  cache->raw = MAP_FAILED;
  cache->base = NULL;
  cache->size = 0;
  cache->page_base = 0xffffffffu;
}

static int ensure_page(int fd, uint32_t phys, page_cache_t *cache) {
  uint32_t page_base = phys & ~0xfffu;
  if (cache->raw != MAP_FAILED && cache->page_base == page_base) return 0;
  unmap_page(cache);
  cache->raw = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)page_base);
  if (cache->raw == MAP_FAILED) return -1;
  cache->base = (volatile uint8_t *)cache->raw;
  cache->size = 0x1000;
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
    memcpy((void *)(cache->base + off), p + done, chunk);
    done += chunk;
  }
  return 0;
}

static int read_exact(int fd, void *dst, size_t size) {
  uint8_t *p = (uint8_t *)dst;
  size_t done = 0;
  while (done < size) {
    ssize_t rc = read(fd, p + done, size - done);
    if (rc <= 0) return -1;
    done += (size_t)rc;
  }
  return 0;
}

int main(int argc, char **argv) {
  const char *path;
  int state_fd;
  int mem_fd;
  voodoo_state_header_t shdr;
  page_cache_t fb = {MAP_FAILED, NULL, 0, 0xffffffffu};
  page_cache_t tex = {MAP_FAILED, NULL, 0, 0xffffffffu};
  uint8_t *buf;
  uint32_t remaining;
  uint32_t offset;
  const size_t chunk_size = 1 << 20;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <state.bin>\n", argv[0]);
    return 2;
  }
  path = argv[1];
  state_fd = open(path, O_RDONLY);
  if (state_fd < 0) {
    perror("open(state.bin)");
    return 1;
  }
  if (read_exact(state_fd, &shdr, sizeof(shdr)) != 0) {
    perror("read(header)");
    close(state_fd);
    return 1;
  }
  if (shdr.magic != VOODOO_STATE_MAGIC || shdr.version != VOODOO_STATE_VERSION) {
    fprintf(stderr, "bad state header\n");
    close(state_fd);
    return 1;
  }
  if (lseek(state_fd, (off_t)(sizeof(shdr) + shdr.reg_count * sizeof(voodoo_state_reg_t)), SEEK_SET) < 0) {
    perror("lseek(state)\n");
    close(state_fd);
    return 1;
  }

  mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (mem_fd < 0) {
    perror("open(/dev/mem)");
    close(state_fd);
    return 1;
  }

  buf = (uint8_t *)malloc(chunk_size);
  if (!buf) {
    close(mem_fd);
    close(state_fd);
    return 1;
  }

  fprintf(stderr, "[de10-state-bulk] fb=%u tex=%u\n", shdr.fb_size, shdr.tex_size);

  remaining = shdr.fb_size;
  offset = 0;
  while (remaining) {
    size_t chunk = remaining > chunk_size ? chunk_size : remaining;
    if (read_exact(state_fd, buf, chunk) != 0) {
      perror("read(fb)");
      return 1;
    }
    if (phys_write_all(mem_fd, &fb, FB_BASE + offset, buf, chunk) != 0) {
      perror("write(fb)");
      return 1;
    }
    remaining -= (uint32_t)chunk;
    offset += (uint32_t)chunk;
  }

  remaining = shdr.tex_size;
  offset = 0;
  while (remaining) {
    size_t chunk = remaining > chunk_size ? chunk_size : remaining;
    if (read_exact(state_fd, buf, chunk) != 0) {
      perror("read(tex)");
      return 1;
    }
    if (phys_write_all(mem_fd, &tex, TEX_BASE + offset, buf, chunk) != 0) {
      perror("write(tex)");
      return 1;
    }
    remaining -= (uint32_t)chunk;
    offset += (uint32_t)chunk;
  }

  fprintf(stderr, "[de10-state-bulk] done\n");
  free(buf);
  unmap_page(&fb);
  unmap_page(&tex);
  close(mem_fd);
  close(state_fd);
  return 0;
}
