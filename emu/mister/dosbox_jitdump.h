#ifndef MISTER_JITDUMP_H
#define MISTER_JITDUMP_H
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error jitdump writer requires little-endian encoding
#endif

namespace mister_jit {
static void fail() {
    std::perror("DOSBox jitdump");
    std::abort();
}
static uint64_t timestamp() {
    timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now)) fail();
    return uint64_t(now.tv_sec) * 1000000000 + uint64_t(now.tv_nsec);
}
struct Writer {
    FILE *file = nullptr;
    void *marker = MAP_FAILED;
    size_t page = 0;
    uint64_t index = 0;
    void write(const void *data, size_t size) {
        if (std::fwrite(data, 1, size, file) != size) fail();
    }
    void flush() {
        if (std::fflush(file)) fail();
    }
    Writer() {
        const char *directory = std::getenv("MISTER_JITDUMP_DIR");
        if (!directory) return;
        if (directory[0] != '/') { errno = EINVAL; fail(); }
        char path[PATH_MAX];
        int length = std::snprintf(path, sizeof(path), "%s/jit-%ld.dump", directory, long(getpid()));
        if (length < 0 || size_t(length) >= sizeof(path)) { errno = ENAMETOOLONG; fail(); }
        int fd = open(path, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (fd < 0) fail();
        file = fdopen(fd, "w+b");
        if (!file) fail();
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size <= 0) fail();
        page = size_t(page_size);

        uint32_t machine;
#if defined(__arm__)
        machine = 40;
#elif defined(__x86_64__)
        machine = 62;
#elif defined(__aarch64__)
        machine = 183;
#else
#error Unsupported jitdump architecture
#endif
        const uint32_t header[] = {0x4a695444, 1, 40, machine, 0, uint32_t(getpid())};
        const uint64_t tail[] = {timestamp(), 0};
        write(header, sizeof(header));
        write(tail, sizeof(tail));
        flush();
        marker = mmap(nullptr, page, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
        if (marker == MAP_FAILED) fail();
    }
    void load(const void *address, const void *bytes, size_t size, const char *name) {
        if (!file) return;
        size_t length = std::strlen(name) + 1;
        if (!address || !bytes || !size || length > UINT32_MAX - 56 || size > UINT32_MAX - 56 - length) {
            errno = EINVAL; fail();
        }
        const uint32_t record[] = {0, uint32_t(56 + length + size)};
        const uint64_t stamp = timestamp();
        const uint32_t ids[] = {uint32_t(getpid()), uint32_t(syscall(SYS_gettid))};
        const uint64_t code[] = {uint64_t(uintptr_t(address)), uint64_t(uintptr_t(address)), uint64_t(size), ++index};
        write(record, sizeof(record));
        write(&stamp, sizeof(stamp));
        write(ids, sizeof(ids));
        write(code, sizeof(code));
        write(name, length);
        write(bytes, size);
        flush();
    }
    ~Writer() {
        if (!file) return;
        const uint32_t record[] = {3, 16};
        const uint64_t stamp = timestamp();
        write(record, sizeof(record));
        write(&stamp, sizeof(stamp));
        flush();
        off_t end = ftello(file);
        if (end < 0 || ftruncate(fileno(file), end)) fail();
        if (munmap(marker, page) || std::fclose(file)) fail();
    }
};
static void load(const void *address, const void *bytes, size_t size, const char *name) {
    static Writer writer;
    writer.load(address, bytes, size, name);
}
}
#endif
