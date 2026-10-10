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

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s <phys-addr> <size-bytes> <output>\n", argv[0]);
    return 2;
  }

  uint32_t phys = (uint32_t)strtoul(argv[1], NULL, 0);
  size_t size = (size_t)strtoul(argv[2], NULL, 0);
  const char *out_path = argv[3];

  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    perror("sysconf(_SC_PAGESIZE)");
    return 1;
  }

  off_t page_base = (off_t)(phys & ~((uint32_t)page_size - 1u));
  size_t page_offset = (size_t)(phys - (uint32_t)page_base);
  size_t map_size = page_offset + size;
  size_t map_rounded = (map_size + (size_t)page_size - 1u) & ~((size_t)page_size - 1u);

  int mem_fd = open("/dev/mem", O_RDONLY | O_SYNC);
  if (mem_fd < 0) {
    perror("open(/dev/mem)");
    return 1;
  }

  void *map = mmap(NULL, map_rounded, PROT_READ, MAP_SHARED, mem_fd, page_base);
  if (map == MAP_FAILED) {
    fprintf(stderr, "mmap phys=0x%08x size=%zu failed: %s\n", phys, size, strerror(errno));
    close(mem_fd);
    return 1;
  }

  int out_fd = open(out_path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (out_fd < 0) {
    perror("open(output)");
    munmap(map, map_rounded);
    close(mem_fd);
    return 1;
  }

  const uint8_t *src = (const uint8_t *)map + page_offset;
  size_t done = 0;
  while (done < size) {
    ssize_t wr = write(out_fd, src + done, size - done);
    if (wr < 0) {
      perror("write(output)");
      close(out_fd);
      munmap(map, map_rounded);
      close(mem_fd);
      return 1;
    }
    done += (size_t)wr;
  }

  close(out_fd);
  munmap(map, map_rounded);
  close(mem_fd);
  printf("dumped %zu bytes from 0x%08x to %s\n", size, phys, out_path);
  return 0;
}
