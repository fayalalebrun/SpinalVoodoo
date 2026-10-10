#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static int backing = -1;
static struct stat image;
static const char *control;

static int stop_machine(void) {
    struct stat status;
    char parent[sizeof(((struct sockaddr_un *)0)->sun_path)];
    if (!control || strlen(control) >= sizeof(parent)) return -ENAMETOOLONG;
    memcpy(parent, control, strlen(control) + 1);
    char *slash = strrchr(parent, '/');
    if (!slash || slash == parent) return -EACCES;
    *slash = '\0';
    if (lstat(parent, &status) || !S_ISDIR(status.st_mode) ||
        status.st_uid != geteuid() || (status.st_mode & 077)) return -EACCES;
    if (lstat(control, &status) || !S_ISSOCK(status.st_mode) ||
        status.st_uid != geteuid() || (status.st_mode & 077)) return -EACCES;
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return -errno;
    struct timeval timeout = {.tv_sec = 5};
    struct sockaddr_un address = {.sun_family = AF_UNIX};
    memcpy(address.sun_path, control, strlen(control) + 1);
    int result = -EIO;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) ||
        connect(fd, (struct sockaddr *)&address, sizeof(address))) goto done;
    struct ucred peer;
    socklen_t length = sizeof(peer);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &length) ||
        length != sizeof(peer) || peer.uid != geteuid()) goto done;
    if (send(fd, "STOP\n", 5, MSG_NOSIGNAL) != 5) goto done;
    char reply[4];
    size_t count = 0;
    while (count < sizeof(reply)) {
        ssize_t received = recv(fd, reply + count, sizeof(reply) - count, 0);
        if (received < 0 && errno == EINTR) continue;
        if (received < 0) goto done;
        if (!received) break;
        count += (size_t)received;
    }
    if (count == 3 && !memcmp(reply, "OK\n", 3)) result = 0;
done:
    close(fd);
    return result;
}

static int core_getattr(const char *path, struct stat *status, struct fuse_file_info *info) {
    (void)info;
    memset(status, 0, sizeof(*status));
    status->st_uid = geteuid();
    status->st_gid = getegid();
    if (!strcmp(path, "/")) {
        status->st_mode = S_IFDIR | 0500;
        status->st_nlink = 2;
    } else if (!strcmp(path, "/core.rbf")) {
        status->st_mode = S_IFREG | 0400;
        status->st_nlink = 1;
        status->st_size = image.st_size;
    } else return -ENOENT;
    return 0;
}

static int core_readdir(const char *path, void *buffer, fuse_fill_dir_t fill,
                        off_t offset, struct fuse_file_info *info, enum fuse_readdir_flags flags) {
    (void)offset; (void)info; (void)flags;
    if (strcmp(path, "/")) return -ENOENT;
    fill(buffer, ".", NULL, 0, 0);
    fill(buffer, "..", NULL, 0, 0);
    fill(buffer, "core.rbf", NULL, 0, 0);
    return 0;
}

static int core_open(const char *path, struct fuse_file_info *info) {
    if (strcmp(path, "/core.rbf")) return -ENOENT;
    if ((info->flags & O_ACCMODE) != O_RDONLY ||
        (info->flags & (O_TRUNC | O_CREAT | O_APPEND))) return -EROFS;
    int result = stop_machine();
    if (!result) info->direct_io = 1;
    return result;
}

static int core_read(const char *path, char *buffer, size_t size, off_t offset,
                     struct fuse_file_info *info) {
    (void)info;
    if (strcmp(path, "/core.rbf")) return -ENOENT;
    if (offset < 0) return -EINVAL;
    ssize_t count = pread(backing, buffer, size, offset);
    return count < 0 ? -errno : (int)count;
}

int main(int argc, char **argv) {
    if (argc != 4 || strlen(argv[2]) >= sizeof(((struct sockaddr_un *)0)->sun_path) ||
        argv[1][0] != '/' || argv[2][0] != '/' || argv[3][0] != '/') {
        fprintf(stderr, "Usage: %s ABSOLUTE_BACKING_FILE ABSOLUTE_CONTROL_SOCKET ABSOLUTE_MOUNTPOINT\n", argv[0]);
        return 2;
    }
    backing = open(argv[1], O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (backing < 0 || fstat(backing, &image) || !S_ISREG(image.st_mode) || !image.st_size) {
        fprintf(stderr, "Backing file must be a nonempty regular file, not a symlink\n");
        if (backing >= 0) close(backing);
        return 2;
    }
    control = argv[2];
    struct fuse_operations operations = {
        .getattr = core_getattr, .readdir = core_readdir, .open = core_open, .read = core_read
    };
    char *arguments[] = {argv[0], "-f", "-s", "-o", "ro,nodev,nosuid,noexec", argv[3]};
    int result = fuse_main(6, arguments, &operations, NULL);
    close(backing);
    return result;
}
