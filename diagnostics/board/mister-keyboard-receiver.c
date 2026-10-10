#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int event(int fd, unsigned code, int value) {
    struct input_event events[2] = {
        {.type = EV_KEY, .code = code, .value = value},
        {.type = EV_SYN, .code = SYN_REPORT}
    };
    return write(fd, events, sizeof(events)) == sizeof(events) ? 0 : -1;
}

int main(void) {
    int fd = open("/dev/uinput", O_WRONLY | O_CLOEXEC);
    if (fd < 0) { perror("uinput"); return 1; }
    struct uinput_user_dev device = {.name = "SpinalVoodoo remote keyboard",
        .id = {.bustype = BUS_USB, .vendor = 0x1209, .product = 0xffff, .version = 1}};
    int failed = ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (unsigned key = 1; key <= KEY_MAX && !failed; ++key)
        failed = ioctl(fd, UI_SET_KEYBIT, key);
    if (failed || write(fd, &device, sizeof(device)) != sizeof(device) || ioctl(fd, UI_DEV_CREATE)) {
        perror("create keyboard"); close(fd); return 1;
    }
    sleep(3);
    puts("READY"); fflush(stdout);
    unsigned char held[KEY_MAX + 1] = {0};
    unsigned char packet[3];
    size_t used = 0;
    for (;;) {
        struct pollfd input = {.fd = STDIN_FILENO, .events = POLLIN};
        int ready = poll(&input, 1, 2000);
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0) { failed = 1; break; }
        ssize_t count = read(STDIN_FILENO, packet + used, sizeof(packet) - used);
        if (count == 0) break;
        if (count < 0) { failed = 1; break; }
        used += count;
        if (used != sizeof(packet)) continue;
        used = 0;
        unsigned key = packet[0] | ((unsigned)packet[1] << 8);
        unsigned value = packet[2];
        if (!key && !value) continue;
        if (!key || key > KEY_MAX || value > 1) { failed = 1; break; }
        if (event(fd, key, value)) { failed = 1; break; }
        held[key] = value;
    }
    if (used) failed = 1;
    for (unsigned key = 1; key <= KEY_MAX; ++key)
        if (held[key] && event(fd, key, 0)) failed = 1;
    if (ioctl(fd, UI_DEV_DESTROY)) failed = 1;
    close(fd);
    return failed ? 1 : 0;
}
