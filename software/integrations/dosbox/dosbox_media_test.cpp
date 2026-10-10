#include "dosbox_media.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <sys/file.h>
#pragma GCC diagnostic pop

static const char *phase;
static bool armed;
extern "C" {
size_t __real_fwrite(const void *, size_t, size_t, FILE *);
int __real_fflush(FILE *);
int __real_fclose(FILE *);
int __real_fsync(int);
int __real_fseeko64(FILE *, int64_t, int);
size_t __wrap_fwrite(const void *data, size_t size, size_t count, FILE *stream) {
    if (armed && !strcmp(phase, "write")) { errno = ENOSPC; return 0; }
    return __real_fwrite(data, size, count, stream);
}
int __wrap_fflush(FILE *stream) {
    if (armed && !strcmp(phase, "flush")) { errno = ENOSPC; return EOF; }
    return __real_fflush(stream);
}
int __wrap_fclose(FILE *stream) {
    int result = __real_fclose(stream);
    if (armed && !strcmp(phase, "close")) { errno = EIO; return EOF; }
    return result;
}
int __wrap_fsync(int fd) {
    if (armed && !strcmp(phase, "sync")) { errno = EIO; return -1; }
    return __real_fsync(fd);
}
int __wrap_fseeko64(FILE *stream, int64_t offset, int whence) {
    if (armed && !strcmp(phase, "seek")) { errno = EIO; return -1; }
    return __real_fseeko64(stream, offset, whence);
}
}
int main(int argc, char **argv) {
    assert(argc == 2);
    phase = argv[1];
    char path[] = "/tmp/spinal-media-unit-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 3);
    FILE *image = fdopen(fd, "w+b");
    assert(image && !ftruncate(fileno(image), 1024));
    const std::string descriptor = std::to_string(fileno(image));
    setenv("MISTER_MEDIA_FDS", descriptor.c_str(), 1);
    assert(!mister_media_start() && mister_media_enabled());
    // Reopening a leased path would conflict with its existing exclusive flock.
    assert(!flock(fd, LOCK_EX | LOCK_NB));
    FILE *leased = nullptr;
    const std::string leased_path = "/proc/self/fd/" + descriptor;
    assert(mister_media_open(leased_path.c_str(), "rb+", &leased) && leased);
    assert(!flock(fileno(leased), LOCK_EX | LOCK_NB));
    assert(!mister_media_close(leased));
    assert(!mister_media_open(path, "rb+", &leased));
    mister_media_track(image);
    if (!strcmp(phase, "unclosed")) {
        assert(mister_media_finish() == -1 && errno == EBUSY);
        assert(!mister_media_close(image));
        assert(mister_media_finish() == -1); // error remains latched
        assert(!unlink(path));
        return 0;
    }
    armed = true;
    int seek_result = mister_media_seek(image, 0, SEEK_SET);
    size_t wrote = mister_media_write("durable", 1, 7, image);
    int closed = mister_media_close(image);
    if (!strcmp(phase, "success")) {
        assert(!seek_result && wrote == 7 && !closed && !mister_media_finish());
    } else {
        assert(mister_media_finish() == -1);
        if (!strcmp(phase, "seek")) assert(seek_result == -1);
        if (!strcmp(phase, "write")) assert(!wrote);
        if (!strcmp(phase, "flush") || !strcmp(phase, "close") || !strcmp(phase, "sync")) assert(closed == EOF);
    }
    assert(!unlink(path));
    return 0;
}
