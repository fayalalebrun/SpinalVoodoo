#define _LARGEFILE64_SOURCE
#include "dosbox_media.h"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>
// These Linux headers use GNU extensions in the pinned native toolchain.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#pragma GCC diagnostic pop

namespace {
struct Image { dev_t device; ino_t inode; int fd; } images[5];
unsigned image_count;
bool initialized;
FILE *streams[16];
int failed;
void fail(int error) { if (!failed) failed = error ? error : EIO; }
bool managed(FILE *stream) {
    if (!image_count) return false;
    struct stat info;
    if (fstat(fileno(stream), &info)) { fail(errno); return false; }
    for (unsigned i = 0; i < image_count; ++i)
        if (info.st_dev == images[i].device && info.st_ino == images[i].inode) return true;
    return false;
}
int flush(FILE *stream) {
    int error = ferror(stream) ? EIO : 0;
    if (fflush(stream) && !error) error = errno ? errno : EIO;
    if (fsync(fileno(stream)) && !error) error = errno ? errno : EIO;
    if (error) fail(error);
    return error;
}
}

int mister_media_start() {
    if (initialized) return 0;
    const char *text = std::getenv("MISTER_MEDIA_FDS");
    if (!text || !*text) return 0;
    if (image_count) { errno = EALREADY; return -1; }
    while (*text) {
        if (*text < '0' || *text > '9' || image_count == 5) { errno = EINVAL; return -1; }
        char *end;
        errno = 0;
        long fd = std::strtol(text, &end, 10);
        struct stat info;
        if (errno || fd < 3 || fd > INT_MAX || (*end && *end != ',') ||
            fstat(int(fd), &info) || !S_ISREG(info.st_mode) || info.st_nlink != 1 ||
            info.st_size <= 0 || info.st_size % 512 ||
            (fcntl(int(fd), F_GETFL) & O_ACCMODE) != O_RDWR) {
            errno = EINVAL; return -1;
        }
        for (unsigned i = 0; i < image_count; ++i)
            if (images[i].device == info.st_dev && images[i].inode == info.st_ino) {
                errno = EINVAL; return -1;
            }
        images[image_count++] = {info.st_dev, info.st_ino, int(fd)};
        text = *end ? end + 1 : end;
        if (*end && !*text) { errno = EINVAL; return -1; }
    }
    initialized = true;
    return 0;
}

bool mister_media_enabled() { return image_count != 0; }

bool mister_media_open(const char *path, const char *mode, FILE **result) {
    for (unsigned i = 0; i < image_count; ++i) {
        char expected[64];
        snprintf(expected, sizeof(expected), "/proc/self/fd/%d", images[i].fd);
        if (std::string(path) != expected) continue;
        *result = nullptr;
        if (std::string(mode) != "rb+" && std::string(mode) != "rb") {
            fail(EINVAL); errno = EINVAL; return true;
        }
        // dup retains the existing flock open-file description. Reopening the
        // proc path creates a new description that conflicts with our own lease.
        int fd = dup(images[i].fd);
        if (fd < 0) { fail(errno); return true; }
        *result = fdopen(fd, mode);
        if (!*result) { int error = errno; close(fd); fail(error); errno = error; }
        return true;
    }
    return false;
}

void mister_media_track(FILE *stream) {
    if (!managed(stream)) return;
    for (FILE *entry : streams) if (entry == stream) return;
    for (FILE *&entry : streams) if (!entry) { entry = stream; return; }
    fail(EMFILE);
}

size_t mister_media_write(const void *data, size_t size, size_t count, FILE *stream) {
    const bool checked = managed(stream);
    if (checked) mister_media_track(stream);
    // A failed seek or earlier I/O must not write to an unknown position.
    if (checked && failed) { errno = failed; return 0; }
    const size_t result = fwrite(data, size, count, stream);
    if (checked && size && result != count) fail(errno);
    return result;
}

int mister_media_seek(FILE *stream, int64_t offset, int whence) {
    const bool checked = managed(stream);
    if (checked) mister_media_track(stream);
    const int result = fseeko64(stream, offset, whence);
    if (checked && result) fail(errno);
    return result;
}

int mister_media_close(FILE *stream) {
    const bool checked = managed(stream);
    const int error = checked ? flush(stream) : 0;
    if (checked) for (FILE *&entry : streams) if (entry == stream) entry = nullptr;
    const int result = fclose(stream);
    if (checked && result) fail(errno);
    if (error) { errno = error; return EOF; }
    return result;
}

int mister_media_finish() {
    // Receipt requires normal DOSBox cleanup to close every managed FILE first.
    for (FILE *entry : streams) if (entry) fail(EBUSY);
    if (failed) { errno = failed; return -1; }
    return 0;
}
