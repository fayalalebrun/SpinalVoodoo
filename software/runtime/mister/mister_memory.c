#define _POSIX_C_SOURCE 200809L
#include "mister_memory.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define RESERVATIONS "/proc/device-tree/reserved-memory/"
#define RESERVATION RESERVATIONS "spinalvoodoo@3e000000/"
#define LOCK_FILE "/run/lock/spinalvoodoo.lock"

static uint64_t big_endian(const uint8_t *bytes, size_t size) {
    uint64_t value = 0;
    for (size_t i = 0; i < size; ++i) value = (value << 8) | bytes[i];
    return value;
}

int mister_reservation_valid(const uint8_t *reg, size_t size, unsigned address_cells,
                             unsigned size_cells) {
    if (!reg || address_cells < 1 || address_cells > 2 || size_cells < 1 || size_cells > 2 ||
        size != (address_cells + size_cells) * 4) return 0;
    return big_endian(reg, address_cells * 4) == MISTER_HOST_BASE &&
           big_endian(reg + address_cells * 4, size_cells * 4) == MISTER_MEMORY_BYTES;
}

static int read_property(const char *path, uint8_t *bytes, size_t size) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    size_t done = 0;
    int error = 0;
    while (done < size) {
        ssize_t count = read(fd, bytes + done, size - done);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { error = count ? errno : EINVAL; break; }
        done += (size_t)count;
    }
    if (!error) {
        uint8_t extra;
        ssize_t count;
        do { count = read(fd, &extra, 1); } while (count < 0 && errno == EINTR);
        if (count != 0) error = count < 0 ? errno : EINVAL;
    }
    if (close(fd) && !error) error = errno;
    if (error) errno = error;
    return error ? -1 : 0;
}

static int check_stock_memory(void) {
    uint8_t reg[8];
    if (read_property("/proc/device-tree/memory/reg", reg, sizeof(reg))) return -1;
    if (big_endian(reg, 4) != 0 || big_endian(reg + 4, 4) != UINT64_C(0x40000000)) {
        errno = EINVAL;
        return -1;
    }
    int fd = open("/proc/cmdline", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    FILE *file = fdopen(fd, "r");
    if (!file) { int saved = errno; close(fd); errno = saved; return -1; }
    char line[4096];
    int error = 0, memory = 0, map = 0;
    if (!fgets(line, sizeof(line), file) || fgetc(file) != EOF || ferror(file)) error = EINVAL;
    if (fclose(file) && !error) error = errno;
    if (!error) {
        char *save = NULL;
        for (char *token = strtok_r(line, " \t\r\n", &save); token;
             token = strtok_r(NULL, " \t\r\n", &save)) {
            if (!strncmp(token, "mem=", 4)) {
                if (memory++ || strcmp(token, "mem=511M")) error = EINVAL;
            } else if (!strncmp(token, "memmap=", 7)) {
                if (map++ || strcmp(token, "memmap=513M$511M")) error = EINVAL;
            }
        }
        if (memory != 1 || map != 1) error = EINVAL;
    }
    if (error) errno = error;
    return error ? -1 : 0;
}

static int check_device_tree(void) {
    uint8_t address_cells[4], size_cells[4], reg[16];
    if (read_property(RESERVATIONS "#address-cells", address_cells, 4) ||
        read_property(RESERVATIONS "#size-cells", size_cells, 4) ||
        read_property(RESERVATIONS "ranges", NULL, 0) ||
        read_property(RESERVATION "no-map", NULL, 0)) return -1;
    unsigned addresses = (unsigned)big_endian(address_cells, 4);
    unsigned sizes = (unsigned)big_endian(size_cells, 4);
    if (addresses < 1 || addresses > 2 || sizes < 1 || sizes > 2) {
        errno = EINVAL;
        return -1;
    }
    size_t bytes = (addresses + sizes) * 4;
    if (read_property(RESERVATION "reg", reg, bytes)) return -1;
    if (!mister_reservation_valid(reg, bytes, addresses, sizes)) { errno = EINVAL; return -1; }
    return 0;
}

static int check_reservation(void) {
    int stock = 0;
    if (check_device_tree()) {
        if (errno != ENOENT || check_stock_memory()) return -1;
        stock = 1;
    }
    int fd = open("/proc/iomem", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    FILE *file = fdopen(fd, "r");
    if (!file) { int saved = errno; close(fd); errno = saved; return -1; }
    char line[512];
    int error = 0, ram_seen = 0;
    while (fgets(line, sizeof(line), file)) {
        uint64_t first, last;
        char label[256];
        if (!strchr(line, '\n') || sscanf(line, " %" SCNx64 "-%" SCNx64 " : %255[^\n]", &first, &last, label) != 3 ||
            first > last) { error = EINVAL; break; }
        if (stock && first < (uint64_t)MISTER_HOST_BASE + MISTER_MEMORY_BYTES && last >= MISTER_HOST_BASE) {
            error = EBUSY;
            break;
        }
        if (strcmp(label, "System RAM")) continue;
        if (stock && (first != 0 || last != UINT64_C(0x1fefffff))) { error = EINVAL; break; }
        if (first == 0 && last == 0) { error = EACCES; break; }
        ram_seen = 1;
        if (first < (uint64_t)MISTER_HOST_BASE + MISTER_MEMORY_BYTES && last >= MISTER_HOST_BASE) {
            error = EBUSY;
            break;
        }
    }
    if (ferror(file) && !error) error = errno ? errno : EIO;
    if (!ram_seen && !error) error = EACCES;
    if (fclose(file) && !error) error = errno;
    if (error) errno = error;
    return error ? -1 : 0;
}

int mister_memory_open(mister_memory *memory) {
    if (!memory) { errno = EINVAL; return -1; }
    if (memory->base || memory->lock_fd != -1) { errno = EALREADY; return -1; }
    if (check_reservation()) return -1;
    int lock = open(LOCK_FILE, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (lock < 0) return -1;
    int error = 0;
    struct stat st;
    if (fstat(lock, &st)) error = errno;
    else if (!S_ISREG(st.st_mode) || st.st_nlink != 1 || st.st_uid != geteuid() ||
             (st.st_mode & 0077)) error = EACCES;
    else if (flock(lock, LOCK_EX | LOCK_NB)) error = errno;
    if (error) { close(lock); errno = error; return -1; }
    int fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) { error = errno; close(lock); errno = error; return -1; }
    void *base = mmap(NULL, MISTER_MEMORY_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, (off_t)MISTER_HOST_BASE);
    error = base == MAP_FAILED ? errno : 0;
    if (close(fd) && !error) {
        error = errno;
        if (munmap(base, MISTER_MEMORY_BYTES)) {
            memory->base = base;
            memory->lock_fd = lock;
            errno = error;
            return -1;
        }
    }
    if (error) { close(lock); errno = error; return -1; }
    memory->base = base;
    memory->lock_fd = lock;
    return 0;
}

int mister_memory_close(mister_memory *memory) {
    if (!memory) { errno = EINVAL; return -1; }
    if (!memory->base) return 0;
    if (munmap(memory->base, MISTER_MEMORY_BYTES)) return -1;
    memory->base = NULL;
    int fd = memory->lock_fd;
    memory->lock_fd = -1;
    return close(fd);
}
