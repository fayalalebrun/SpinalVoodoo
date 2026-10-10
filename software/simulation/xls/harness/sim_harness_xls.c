#include "sim_harness.h"
#include "xls-bridge-protocol.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int bridge_fd = -1;
static int bridge_stalled;
static uint64_t bridge_ticks;

static int write_exact(int fd, const void *data, size_t size) {
    const unsigned char *bytes = (const unsigned char *)data;
    while (size != 0) {
        ssize_t written = write(fd, bytes, size);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return -1;
        bytes += written;
        size -= (size_t)written;
    }
    return 0;
}

static int read_exact(int fd, void *data, size_t size) {
    unsigned char *bytes = (unsigned char *)data;
    while (size != 0) {
        ssize_t received = read(fd, bytes, size);
        if (received < 0 && errno == EINTR) continue;
        if (received <= 0) return -1;
        bytes += received;
        size -= (size_t)received;
    }
    return 0;
}

static int exchange(xls_bridge_request_t request, const void *payload,
                    size_t payload_size, void *output, size_t output_size,
                    xls_bridge_response_t *received_response) {
    xls_bridge_response_t response;
    if (bridge_fd < 0 || write_exact(bridge_fd, &request, sizeof(request)) != 0 ||
        (payload_size != 0 && write_exact(bridge_fd, payload, payload_size) != 0) ||
        read_exact(bridge_fd, &response, sizeof(response)) != 0 ||
        response.magic != XLS_BRIDGE_MAGIC || response.status != 0 ||
        response.count > output_size ||
        (response.count != 0 && read_exact(bridge_fd, output, response.count) != 0)) {
        bridge_stalled = 1;
        return -1;
    }
    bridge_ticks = response.ticks;
    if (received_response) *received_response = response;
    return 0;
}

static int simple_request(uint32_t operation, uint32_t address, uint32_t data,
                          uint32_t count, uint32_t mask,
                          xls_bridge_response_t *response) {
    xls_bridge_request_t request = {
        XLS_BRIDGE_MAGIC, operation, address, data, count, mask
    };
    return exchange(request, NULL, 0, NULL, 0, response);
}

int sim_init(void) {
    const char *path = getenv("SIM_XLS_SOCKET");
    struct sockaddr_un address;
    int attempt;
    if (!path || strlen(path) >= sizeof(address.sun_path)) return -1;
    bridge_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (bridge_fd < 0) return -1;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path);
    for (attempt = 0; attempt < 500; ++attempt) {
        if (connect(bridge_fd, (struct sockaddr *)&address, sizeof(address)) == 0) {
            bridge_stalled = 0;
            bridge_ticks = 0;
            return 0;
        }
        if (errno != ENOENT && errno != ECONNREFUSED) break;
        usleep(10000);
    }
    close(bridge_fd);
    bridge_fd = -1;
    return -1;
}

void sim_shutdown(void) {
    if (bridge_fd >= 0) {
        simple_request(XLS_BRIDGE_CLOSE, 0, 0, 0, 0, NULL);
        close(bridge_fd);
        bridge_fd = -1;
    }
}

void sim_write(uint32_t addr, uint32_t data) {
    simple_request(XLS_BRIDGE_WRITE, addr, data, 0, 0xf, NULL);
}

void sim_write16(uint32_t addr, uint16_t data) {
    uint32_t high = (addr & 2u) != 0;
    simple_request(XLS_BRIDGE_WRITE, addr & ~3u,
                   high ? (uint32_t)data << 16 : data, 0,
                   high ? 0xc : 0x3, NULL);
}

uint32_t sim_read(uint32_t addr) {
    xls_bridge_response_t response;
    if (simple_request(XLS_BRIDGE_READ, addr, 0, 0, 0, &response) != 0) return 0;
    return response.data;
}

uint32_t sim_idle_wait(void) {
    xls_bridge_response_t response;
    if (simple_request(XLS_BRIDGE_IDLE, 0, 0, 0, 0, &response) != 0) return 0;
    return response.data;
}

void sim_tick(int n) {
    if (n > 0) simple_request(XLS_BRIDGE_TICK, 0, 0, (uint32_t)n, 0, NULL);
}

void sim_flush_fb_cache(void) {}
void sim_invalidate_fb_cache(void) {}

static void read_words(uint32_t operation, uint32_t byte_offset,
                       uint32_t *dst, uint32_t word_count) {
    xls_bridge_request_t request = {
        XLS_BRIDGE_MAGIC, operation, byte_offset, 0, word_count, 0
    };
    if (!dst || exchange(request, NULL, 0, dst,
                         (size_t)word_count * sizeof(uint32_t), NULL) != 0) {
        if (dst) memset(dst, 0, (size_t)word_count * sizeof(uint32_t));
    }
}

static void write_words(uint32_t operation, uint32_t byte_offset,
                        const uint32_t *src, uint32_t word_count) {
    xls_bridge_request_t request = {
        XLS_BRIDGE_MAGIC, operation, byte_offset, 0, word_count, 0
    };
    if (!src) return;
    exchange(request, src, (size_t)word_count * sizeof(uint32_t), NULL, 0, NULL);
}

void sim_read_fb(uint32_t byte_offset, uint32_t *dst, uint32_t word_count) {
    read_words(XLS_BRIDGE_READ_FB, byte_offset, dst, word_count);
}

void sim_write_fb_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count) {
    write_words(XLS_BRIDGE_WRITE_FB, byte_offset, src, word_count);
}

void sim_write_tex_bulk(uint32_t byte_offset, const uint32_t *src, uint32_t word_count) {
    write_words(XLS_BRIDGE_WRITE_BUS_BULK, 0x800000u + byte_offset, src, word_count);
}

void sim_read_tex(uint32_t byte_offset, uint32_t *dst, uint32_t word_count) {
    read_words(XLS_BRIDGE_READ_TEX, byte_offset, dst, word_count);
}

int sim_read_frontbuffer(uint32_t width, uint32_t height, uint16_t *dst) {
    xls_bridge_request_t request = {
        XLS_BRIDGE_MAGIC, XLS_BRIDGE_READ_FRONTBUFFER, width, height, 0, 0
    };
    if (!dst) return -1;
    return exchange(request, NULL, 0, dst,
                    (size_t)width * height * sizeof(uint16_t), NULL);
}

void sim_set_swap_count(uint32_t count) {
    simple_request(XLS_BRIDGE_SET_SWAP, 0, count, 0, 0, NULL);
}

uint32_t sim_get_swap_count(void) {
    xls_bridge_response_t response;
    if (simple_request(XLS_BRIDGE_GET_SWAP, 0, 0, 0, 0, &response) != 0) return 0;
    return response.data;
}

void sim_dump_first_fb_writes(const char *path, uint32_t limit) {
    (void)path;
    (void)limit;
}

uint64_t sim_get_cycle(void) { return bridge_ticks; }
uint64_t sim_get_total_read_ticks(void) { return 0; }
uint64_t sim_get_total_read_count(void) { return 0; }
uint64_t sim_get_total_write_ticks(void) { return 0; }
uint64_t sim_get_total_write_count(void) { return 0; }

#define ZERO_U32(name) uint32_t name(void) { return 0; }
ZERO_U32(sim_get_pixels_in)
ZERO_U32(sim_get_pixels_out)
ZERO_U32(sim_get_fb_fill_hits)
ZERO_U32(sim_get_fb_fill_misses)
ZERO_U32(sim_get_fb_fill_burst_count)
ZERO_U32(sim_get_fb_fill_burst_beats)
ZERO_U32(sim_get_fb_fill_stall_cycles)
ZERO_U32(sim_get_fb_write_stall_cycles)
ZERO_U32(sim_get_fb_write_drain_count)
ZERO_U32(sim_get_fb_write_full_drain_count)
ZERO_U32(sim_get_fb_write_partial_drain_count)
ZERO_U32(sim_get_fb_write_drain_reason_full_count)
ZERO_U32(sim_get_fb_write_drain_reason_rotate_count)
ZERO_U32(sim_get_fb_write_drain_reason_flush_count)
ZERO_U32(sim_get_fb_write_drain_dirty_word_total)
ZERO_U32(sim_get_fb_write_rotate_blocked_cycles)
ZERO_U32(sim_get_fb_write_single_word_drain_count)
ZERO_U32(sim_get_fb_write_single_word_drain_start_at_zero_count)
ZERO_U32(sim_get_fb_write_single_word_drain_start_at_last_count)
ZERO_U32(sim_get_fb_write_rotate_adjacent_line_count)
ZERO_U32(sim_get_fb_write_rotate_same_line_gap_count)
ZERO_U32(sim_get_fb_write_rotate_other_line_count)
ZERO_U32(sim_get_fb_mem_color_write_cmd_count)
ZERO_U32(sim_get_fb_mem_aux_write_cmd_count)
ZERO_U32(sim_get_fb_mem_color_read_cmd_count)
ZERO_U32(sim_get_fb_mem_aux_read_cmd_count)
ZERO_U32(sim_get_fb_mem_lfb_read_cmd_count)
ZERO_U32(sim_get_fb_mem_color_write_blocked_cycles)
ZERO_U32(sim_get_fb_mem_aux_write_blocked_cycles)
ZERO_U32(sim_get_fb_mem_color_read_blocked_cycles)
ZERO_U32(sim_get_fb_mem_aux_read_blocked_cycles)
ZERO_U32(sim_get_fb_mem_lfb_read_blocked_cycles)
ZERO_U32(sim_get_fb_read_req_count)
ZERO_U32(sim_get_fb_read_req_forward_step_count)
ZERO_U32(sim_get_fb_read_req_backward_step_count)
ZERO_U32(sim_get_fb_read_req_same_word_count)
ZERO_U32(sim_get_fb_read_req_same_line_count)
ZERO_U32(sim_get_fb_read_req_other_count)
ZERO_U32(sim_get_fb_read_single_beat_burst_count)
ZERO_U32(sim_get_fb_read_multi_beat_burst_count)
ZERO_U32(sim_get_fb_read_max_queue_occupancy)
ZERO_U32(sim_get_tex_fill_hits)
ZERO_U32(sim_get_tex_fill_misses)
ZERO_U32(sim_get_tex_fill_burst_count)
ZERO_U32(sim_get_tex_fill_burst_beats)
ZERO_U32(sim_get_tex_fill_stall_cycles)
ZERO_U32(sim_get_tex_fast_bilinear_hits)
ZERO_U32(sim_get_tex_compare_miss_samples)
ZERO_U32(sim_get_tex_lookup_blocked_cycles)
ZERO_U32(sim_get_tex_lookup_blocked_by_owner_cycles)
ZERO_U32(sim_get_tex_lookup_blocked_by_fill_cycles)
ZERO_U32(sim_get_tex_lookup_blocked_by_hold_cycles)
ZERO_U32(sim_get_tex_lookup_blocked_by_live_cycles)
ZERO_U32(sim_get_tex_fill_evict_valid)
ZERO_U32(sim_get_tex_fill_evict_ready)
ZERO_U32(sim_get_tex_fill_evict_inflight)
#undef ZERO_U32

int sim_stalled(void) { return bridge_stalled; }
void sim_tex_access_dump(const char *path) { (void)path; }
void sim_tex_access_close(void) {}
