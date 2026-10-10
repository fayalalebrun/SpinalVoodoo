#define _POSIX_C_SOURCE 200809L
#include "mister_memory.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static char directory[] = "mister-memory-XXXXXX";
static int memory_opens;
static int fail_open;
static int fail_map;
static int fail_unmap;
static int fail_memory_close;
static int memory_fd = -1;

int __real_open(const char *, int, ...);
void *__real_mmap(void *, size_t, int, int, int, off_t);
int __real_munmap(void *, size_t);
int __real_close(int);

int __wrap_open(const char *name, int flags, ...) {
    const char *leaf = !strcmp(name, "/proc/device-tree/memory/reg") ? "/memory-reg" : strrchr(name, '/');
    assert(leaf && (!strncmp(name, "/proc/", 6) || !strcmp(name, "/dev/mem") ||
                    !strcmp(name, "/run/lock/spinalvoodoo.lock")));
    assert(flags & O_CLOEXEC);
    assert(flags & O_NOFOLLOW);
    if (!strcmp(name, "/dev/mem")) {
        ++memory_opens;
        assert(flags & O_SYNC);
        if (fail_open) { errno = EACCES; return -1; }
    }
    char path[256];
    assert(snprintf(path, sizeof(path), "%s/%s", directory, leaf + 1) < (int)sizeof(path));
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }
    int fd = __real_open(path, flags, mode);
    if (!strcmp(name, "/dev/mem")) memory_fd = fd;
    return fd;
}

void *__wrap_mmap(void *address, size_t size, int prot, int flags, int fd, off_t offset) {
    assert(!address && size == MISTER_MEMORY_BYTES && prot == (PROT_READ | PROT_WRITE));
    assert(flags == MAP_SHARED && fd >= 0 && offset == MISTER_HOST_BASE);
    if (fail_map) { errno = ENOMEM; return MAP_FAILED; }
    assert(ftruncate(fd, (off_t)size) == 0);
    return __real_mmap(NULL, size, prot, flags, fd, 0);
}

int __wrap_munmap(void *address, size_t size) {
    if (fail_unmap) { errno = EINVAL; return -1; }
    return __real_munmap(address, size);
}

int __wrap_close(int fd) {
    int is_memory = fd == memory_fd;
    int result = __real_close(fd);
    if (is_memory) {
        memory_fd = -1;
        if (fail_memory_close) { errno = EIO; return -1; }
    }
    return result;
}

static void put(const char *name, const void *data, size_t size) {
    char path[256];
    assert(snprintf(path, sizeof(path), "%s/%s", directory, name) < (int)sizeof(path));
    FILE *file = fopen(path, "wb");
    assert(file);
    if (size) assert(fwrite(data, 1, size, file) == size);
    assert(fclose(file) == 0);
}

int main(void) {
    const uint8_t reg[] = {0x3e, 0, 0, 0, 2, 0, 0, 0};
    const uint8_t wide[] = {0, 0, 0, 0, 0x3e, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0};
    const uint8_t cells[] = {0, 0, 0, 1};
    const char iomem[] = "00000000-3dffffff : System RAM\n3e000000-3fffffff : reserved\n";
    mister_memory a = MISTER_MEMORY_INIT, b = MISTER_MEMORY_INIT;
    assert(mister_memory_open(NULL) == -1 && errno == EINVAL);
    assert(mister_memory_close(NULL) == -1 && errno == EINVAL);
    assert(mister_reservation_valid(reg, sizeof(reg), 1, 1));
    assert(mister_reservation_valid(wide, sizeof(wide), 2, 2));
    assert(!mister_reservation_valid(NULL, sizeof(reg), 1, 1));
    assert(!mister_reservation_valid(reg, sizeof(reg) - 1, 1, 1));
    assert(!mister_reservation_valid(reg, sizeof(reg), 0, 1));
    assert(!mister_reservation_valid(wide, sizeof(wide), 3, 1));
    assert(mkdtemp(directory));
    assert(mister_memory_open(&a) == -1 && errno == ENOENT && memory_opens == 0);
    put("#address-cells", cells, 4);
    put("#size-cells", cells, 4);
    put("reg", reg, sizeof(reg));
    put("ranges", NULL, 0);
    put("no-map", NULL, 0);
    put("iomem", iomem, strlen(iomem));
    put("mem", NULL, 0);
    assert(mister_memory_open(&a) == 0);
    assert(a.base && a.lock_fd >= 0 && memory_opens == 1);
    a.base[0] = 123;
    a.base[MISTER_MEMORY_BYTES - 1] = 45;
    uint8_t *owned_base = a.base;
    int owned_lock = a.lock_fd;
    assert(mister_memory_open(&a) == -1 && errno == EALREADY);
    assert(a.base == owned_base && a.lock_fd == owned_lock && memory_opens == 1);
    assert(mister_memory_open(&b) == -1 && (errno == EWOULDBLOCK || errno == EAGAIN));
    assert(!b.base && memory_opens == 1);
    fail_unmap = 1;
    assert(mister_memory_close(&a) == -1 && a.base);
    assert(mister_memory_open(&b) == -1 && memory_opens == 1);
    fail_unmap = 0;
    assert(mister_memory_close(&a) == 0 && !a.base && a.lock_fd == -1);
    assert(mister_memory_close(&a) == 0);
    fail_open = 1;
    assert(mister_memory_open(&a) == -1 && errno == EACCES);
    fail_open = 0;
    fail_map = 1;
    assert(mister_memory_open(&a) == -1 && errno == ENOMEM);
    fail_map = 0;
    assert(mister_memory_open(&a) == 0);
    assert(mister_memory_close(&a) == 0);
    fail_memory_close = 1;
    assert(mister_memory_open(&a) == -1 && errno == EIO && !a.base && a.lock_fd == -1);
    fail_unmap = 1;
    assert(mister_memory_open(&a) == -1 && errno == EIO && a.base && a.lock_fd >= 0);
    assert(mister_memory_open(&b) == -1 && (errno == EWOULDBLOCK || errno == EAGAIN));
    fail_unmap = 0;
    fail_memory_close = 0;
    assert(mister_memory_close(&a) == 0);
    int previous_opens = memory_opens;
    const char *invalid[] = {"00000000-3fffffff : System RAM\n", "00000000-00000000 : System RAM\n",
                            "garbage\n", "3e000000-3fffffff : reserved\n"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        put("iomem", invalid[i], strlen(invalid[i]));
        assert(mister_memory_open(&a) == -1 && !a.base && memory_opens == previous_opens);
    }
    put("iomem", iomem, strlen(iomem));
    put("no-map", cells, 4);
    assert(mister_memory_open(&a) == -1 && errno == EINVAL);
    put("no-map", NULL, 0);
    put("reg", reg, sizeof(reg) - 1);
    assert(mister_memory_open(&a) == -1 && errno == EINVAL);
    put("reg", wide, sizeof(wide));
    assert(mister_memory_open(&a) == -1 && errno == EINVAL);
    put("reg", reg, sizeof(reg));
    char path[256];
    snprintf(path, sizeof(path), "%s/spinalvoodoo.lock", directory);
    assert(chmod(path, 0666) == 0);
    assert(mister_memory_open(&a) == -1 && errno == EACCES);
    assert(unlink(path) == 0);
    assert(symlink("mem", path) == 0);
    assert(mister_memory_open(&a) == -1 && errno == ELOOP);
    assert(memory_opens == previous_opens);
    assert(unlink(path) == 0);
    const char stock[] = "console=ttyS0 mem=511M memmap=513M$511M ro rootwait\n";
    const char stock_iomem[] = "00000000-1fefffff : System RAM\nff704000-ff704fff : mmc\n";
    const uint8_t ddr[] = {0, 0, 0, 0, 0x40, 0, 0, 0};
    put("cmdline", stock, strlen(stock));
    put("memory-reg", ddr, sizeof(ddr));
    put("iomem", stock_iomem, strlen(stock_iomem));
    snprintf(path, sizeof(path), "%s/#address-cells", directory);
    assert(unlink(path) == 0);
    assert(mister_memory_open(&a) == 0);
    assert(mister_memory_close(&a) == 0);
    previous_opens = memory_opens;
    const char *bad_boot[] = {"", "mem=511M\n", "memmap=513M$511M\n",
        "mem=1024M memmap=513M$511M\n", "mem=511M memmap=513M$511M mem=1024M\n",
        "mem=511M memmap=513M$511M memmap=1M$0\n", "xmem=511M memmap=513M$511M\n"};
    for (unsigned i = 0; i < sizeof(bad_boot) / sizeof(*bad_boot); ++i) {
        put("cmdline", bad_boot[i], strlen(bad_boot[i]));
        assert(mister_memory_open(&a) == -1 && !a.base && memory_opens == previous_opens);
    }
    char oversized[4096];
    memset(oversized, ' ', sizeof(oversized));
    memcpy(oversized, stock, strlen(stock));
    put("cmdline", oversized, sizeof(oversized));
    assert(mister_memory_open(&a) == -1 && memory_opens == previous_opens);
    put("cmdline", stock, strlen(stock));
    put("memory-reg", reg, sizeof(reg));
    assert(mister_memory_open(&a) == -1 && memory_opens == previous_opens);
    put("memory-reg", ddr, sizeof(ddr));
    const char *bad_stock[] = {"00000000-1fffffff : System RAM\n",
        "00000000-1fefffff : System RAM\n3e000000-3fffffff : other device\n",
        "00000000-1fefffff : System RAM\n3f000000-3fffffff : System RAM\n",
        "00000000-00000000 : System RAM\n", "garbage\n"};
    for (unsigned i = 0; i < sizeof(bad_stock) / sizeof(*bad_stock); ++i) {
        put("iomem", bad_stock[i], strlen(bad_stock[i]));
        assert(mister_memory_open(&a) == -1 && !a.base && memory_opens == previous_opens);
    }
    put("#address-cells", cells, 4);
    const char *files[] = {"#address-cells", "#size-cells", "reg", "ranges", "no-map", "iomem", "mem", "spinalvoodoo.lock", "cmdline", "memory-reg"};
    for (unsigned i = 0; i < sizeof(files) / sizeof(*files); ++i) {
        snprintf(path, sizeof(path), "%s/%s", directory, files[i]);
        assert(unlink(path) == 0);
    }
    assert(rmdir(directory) == 0);
    puts("MiSTer memory reservation/ownership/error-path checks PASS (no physical memory accessed)");
    return 0;
}
