#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <csignal>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "ref_model.h"
#include "../../software/trace/voodoo_trace_format.h"
}

typedef struct {
    const voodoo_state_header_t *hdr;
    const voodoo_state_reg_t *regs;
    const uint8_t *fb;
    const uint8_t *tex;
} parsed_state_t;

static std::vector<uint8_t> read_file(const std::string &path)
{
    std::vector<uint8_t> data;
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return data;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) {
        fclose(f);
        return data;
    }

    data.resize((size_t)sz);
    if ((long)fread(data.data(), 1, (size_t)sz, f) != sz)
        data.clear();
    fclose(f);
    return data;
}

static bool parse_state_blob(const std::vector<uint8_t> &blob, parsed_state_t *out)
{
    if (blob.size() < sizeof(voodoo_state_header_t))
        return false;

    const voodoo_state_header_t *hdr = (const voodoo_state_header_t *)blob.data();
    if (hdr->magic != VOODOO_STATE_MAGIC)
        return false;

    uint64_t reg_bytes = (uint64_t)hdr->reg_count * sizeof(voodoo_state_reg_t);
    uint64_t total = sizeof(voodoo_state_header_t) + reg_bytes + hdr->fb_size +
                     (uint64_t)hdr->tex_size * hdr->num_tmus;
    if (blob.size() < total)
        return false;

    const uint8_t *ptr = blob.data() + sizeof(voodoo_state_header_t);
    out->hdr = hdr;
    out->regs = (const voodoo_state_reg_t *)ptr;
    ptr += reg_bytes;
    out->fb = ptr;
    ptr += hdr->fb_size;
    out->tex = ptr;
    return true;
}

static bool find_last_reg(const parsed_state_t &st, uint32_t addr, uint32_t *value_out)
{
    for (int64_t i = (int64_t)st.hdr->reg_count - 1; i >= 0; i--) {
        if ((st.regs[i].addr & 0x3ffc) == (addr & 0x3ffc)) {
            *value_out = st.regs[i].value;
            return true;
        }
    }
    return false;
}

static bool has_clut_entry(const parsed_state_t &st, int index, uint32_t rgb)
{
    for (uint32_t i = 0; i < st.hdr->reg_count; i++) {
        if ((st.regs[i].addr & 0x3ffc) != 0x228)
            continue;
        if (((st.regs[i].value >> 24) & 0x3f) != (uint32_t)index)
            continue;
        if ((st.regs[i].value & 0x00ffffff) == (rgb & 0x00ffffff))
            return true;
    }
    return false;
}

static bool has_palette_entry(const parsed_state_t &st, unsigned index, uint32_t rgb)
{
    const uint32_t address = 0x334 + 4 * (index & 1);
    const uint32_t value = 0x80000000u | ((index >> 1) << 24) | (rgb & 0xffffff);
    for (uint32_t i = 0; i < st.hdr->reg_count; ++i)
        if (st.regs[i].addr == address && st.regs[i].value == value)
            return true;
    return false;
}

static int fail(const char *msg)
{
    fprintf(stderr, "state_roundtrip_test: FAIL: %s\n", msg);
    ref_shutdown();
    return 1;
}

int main(void)
{
    char tmp1[] = "/tmp/voodoo-state-rt1-XXXXXX";
    char tmp2[] = "/tmp/voodoo-state-rt2-XXXXXX";
    if (!mkdtemp(tmp1) || !mkdtemp(tmp2))
        return fail("mkdtemp failed");

    const std::string state1_path = std::string(tmp1) + "/state.bin";
    const std::string state2_path = std::string(tmp2) + "/state.bin";

    const uint32_t clip_lr = (0x0123u << 16) | 0x0045u;
    const uint32_t clip_y  = (0x01abu << 16) | 0x0067u;
    const uint32_t tex_base0_raw = 0x00012345u;
    const uint32_t tex_base1_raw = 0x00023456u;
    const uint32_t swap_cmd = 0x00000007u;
    const int clut_index = 7;
    const uint32_t clut_rgb = 0x00a1b2c3u;

    if (ref_init(4, 4) != 0)
        return fail("ref_init failed");

    ref_write_reg(0x214, 0x000000a0u);
    ref_write_reg(0x218, 0x00020000u);
    ref_write_reg(0x104, 0x01020304u);
    ref_write_reg(0x110, 0x05060708u);
    ref_write_reg(0x118, clip_lr);
    ref_write_reg(0x11c, clip_y);
    ref_write_reg(0x30c, tex_base0_raw);
    ref_write_reg(0x310, tex_base1_raw);
    ref_write_reg(0x228, ((uint32_t)clut_index << 24) | (clut_rgb & 0x00ffffffu));
    ref_write_reg(0x128, swap_cmd);
    ref_write_reg(0x160, 0x12345678u);
    ref_write_reg(0x324, 0x01020304u);
    ref_write_reg(0x33c, 0x00090807u);
    ref_write_reg(0x334, 0x80112233u);
    ref_write_reg(0x338, 0xffaabbccu);

    uint32_t *tex_mut = (uint32_t *)ref_get_tex();
    for (uint32_t i = 0; i < 64; i++)
        tex_mut[i] = 0xdead0000u | i;

    if (ref_dump_state_to_dir(tmp1, 1) != 0)
        return fail("ref_dump_state_to_dir(tmp1) failed");

    std::vector<uint8_t> state1 = read_file(state1_path);
    if (state1.empty())
        return fail("failed to read first dumped state.bin");

    parsed_state_t parsed1;
    if (!parse_state_blob(state1, &parsed1))
        return fail("failed to parse first state blob");

    if (parsed1.hdr->version != VOODOO_STATE_VERSION)
        return fail("first dump has wrong state version");

    uint32_t reg_val = 0;
    if (!find_last_reg(parsed1, 0x118, &reg_val) || reg_val != clip_lr)
        return fail("first dump clipLeftRight mismatch");
    if (!find_last_reg(parsed1, 0x11c, &reg_val) || reg_val != clip_y)
        return fail("first dump clipLowYHighY mismatch");
    if (!find_last_reg(parsed1, 0x30c, &reg_val) || reg_val != tex_base0_raw)
        return fail("first dump texBaseAddr mismatch");
    if (!find_last_reg(parsed1, 0x310, &reg_val) || reg_val != tex_base1_raw)
        return fail("first dump texBaseAddr1 mismatch");
    if (!has_clut_entry(parsed1, clut_index, clut_rgb))
        return fail("first dump CLUT entry missing");

    std::vector<uint8_t> state_v1 = state1;
    ((voodoo_state_header_t *)state_v1.data())->version = 1;
    if (ref_load_state(state_v1.data(), (uint32_t)state_v1.size()) == 0)
        return fail("v1 state unexpectedly accepted");

    std::vector<uint8_t> load_blob = state1;
    parsed_state_t parsed_load;
    if (!parse_state_blob(load_blob, &parsed_load))
        return fail("failed to parse load_blob");
    uint8_t *mutable_fb = (uint8_t *)parsed_load.fb;
    size_t poison_bytes = parsed_load.hdr->fb_size < 64 ? parsed_load.hdr->fb_size : 64;
    memset(mutable_fb, 0xa5, poison_bytes);

    ref_shutdown();
    if (ref_init(4, 4) != 0)
        return fail("second ref_init failed");

    if (ref_load_state(load_blob.data(), (uint32_t)load_blob.size()) != 0)
        return fail("loading v2 state failed");

    if (ref_get_clip_left_right() != clip_lr)
        return fail("loaded clipLeftRight mismatch");
    if (ref_get_clip_lowy_highy() != clip_y)
        return fail("loaded clipLowYHighY mismatch");
    if (ref_get_swapbuffer_cmd() != swap_cmd)
        return fail("loaded swapbufferCMD mismatch");

    if (ref_get_tex_base_addr(0, 0) != ((tex_base0_raw & 0x7ffffu) << 3))
        return fail("loaded texBaseAddr internal value mismatch");
    if (ref_get_tex_base_addr(0, 1) != ((tex_base1_raw & 0x7ffffu) << 3))
        return fail("loaded texBaseAddr1 internal value mismatch");
    if (ref_get_clut_rgb(clut_index) != (clut_rgb & 0x00ffffffu))
        return fail("loaded CLUT entry mismatch");

    const uint32_t *tex_words = (const uint32_t *)ref_get_tex();
    for (uint32_t i = 0; i < 64; i++) {
        if (tex_words[i] != (0xdead0000u | i))
            return fail("loaded texture contents mismatch");
    }

    const uint8_t *fb_bytes = (const uint8_t *)ref_get_fb();
    if (fb_bytes[0] != 0xa5)
        return fail("framebuffer preload was not restored");

    if (ref_dump_state_to_dir(tmp2, 2) != 0)
        return fail("ref_dump_state_to_dir(tmp2) failed");

    std::vector<uint8_t> state2 = read_file(state2_path);
    if (state2.empty())
        return fail("failed to read second dumped state.bin");

    parsed_state_t parsed2;
    if (!parse_state_blob(state2, &parsed2))
        return fail("failed to parse second state blob");
    if (parsed2.hdr->version != VOODOO_STATE_VERSION)
        return fail("second dump has wrong state version");

    if (!find_last_reg(parsed2, 0x118, &reg_val) || reg_val != clip_lr)
        return fail("second dump clipLeftRight mismatch");
    if (!find_last_reg(parsed2, 0x11c, &reg_val) || reg_val != clip_y)
        return fail("second dump clipLowYHighY mismatch");
    if (!find_last_reg(parsed2, 0x30c, &reg_val) || reg_val != tex_base0_raw)
        return fail("second dump texBaseAddr mismatch");
    if (!find_last_reg(parsed2, 0x310, &reg_val) || reg_val != tex_base1_raw)
        return fail("second dump texBaseAddr1 mismatch");
    if (!has_clut_entry(parsed2, clut_index, clut_rgb))
        return fail("second dump CLUT entry missing");
    for (const uint32_t address : {0x160u, 0x324u, 0x33cu}) {
        uint32_t before = 0, after = 0;
        if (!find_last_reg(parsed1, address, &before) ||
            !find_last_reg(parsed2, address, &after) || before != after)
            return fail("fog/NCC register did not roundtrip");
    }
    if (!has_palette_entry(parsed2, 0, 0x112233u) ||
        !has_palette_entry(parsed2, 255, 0xaabbccu))
        return fail("even/odd texture palette entries did not roundtrip");
    if (parsed1.hdr->h_disp != parsed2.hdr->h_disp || parsed1.hdr->v_disp != parsed2.hdr->v_disp)
        return fail("display dimensions did not roundtrip");
    if (ref_dump_state_to_dir(nullptr, 3) == 0 || ref_dump_state_to_dir("", 3) == 0 ||
        ref_dump_state_to_dir(state1_path.c_str(), 3) == 0 ||
        ref_dump_state_to_dir(std::string(4096, 'x').c_str(), 3) == 0)
        return fail("invalid snapshot output path accepted");
    if (read_file(state1_path) != state1)
        return fail("failed snapshot write changed existing state");
    const pid_t child = fork();
    if (child < 0) return fail("fork failed");
    if (child == 0) {
        const rlimit limit = {1024, 1024};
        if (signal(SIGXFSZ, SIG_IGN) == SIG_ERR || setrlimit(RLIMIT_FSIZE, &limit)) _exit(2);
        _exit(ref_dump_state_to_dir(tmp1, 3) == -1 ? 0 : 1);
    }
    int child_status = 0;
    if (waitpid(child, &child_status, 0) != child || !WIFEXITED(child_status) || WEXITSTATUS(child_status))
        return fail("snapshot writer ignored a file-write failure");
    if (read_file(state1_path) != state1)
        return fail("short snapshot write damaged existing state");
    auto malformed = state2;
    auto *bad_header = (voodoo_state_header_t *)malformed.data();
    bad_header->num_tmus = 2;
    if (ref_load_state(malformed.data(), (uint32_t)malformed.size()) == 0)
        return fail("single-TMU model accepted a two-TMU snapshot");
    if (ref_load_state(state2.data(), (uint32_t)state2.size() - 1) == 0)
        return fail("truncated snapshot accepted");
    bad_header->num_tmus = 1;
    bad_header->reg_count = UINT32_MAX;
    if (ref_load_state(malformed.data(), (uint32_t)malformed.size()) == 0 ||
        ref_load_state(nullptr, 64) == 0)
        return fail("invalid snapshot span accepted");

    for (size_t offset : {offsetof(voodoo_state_header_t, magic),
                          offsetof(voodoo_state_header_t, voodoo_type),
                          offsetof(voodoo_state_header_t, flags),
                          offsetof(voodoo_state_header_t, reserved),
                          offsetof(voodoo_state_header_t, reserved) + sizeof(uint32_t),
                          offsetof(voodoo_state_header_t, fb_size),
                          offsetof(voodoo_state_header_t, tex_size)}) {
        malformed = state2;
        const uint32_t invalid = UINT32_MAX;
        memcpy(malformed.data() + offset, &invalid, sizeof(invalid));
        if (ref_load_state(malformed.data(), (uint32_t)malformed.size()) == 0)
            return fail("invalid snapshot header accepted");
    }
    if (ref_load_state(state2.data(), sizeof(voodoo_state_header_t) - 1) == 0 ||
        ref_dump_state_to_dir(tmp2, 2) != 0 || read_file(state2_path) != state2)
        return fail("invalid snapshot load changed model state");

    ref_write_reg(0x334, 0x81234567u);
    if (ref_load_state(state2.data(), (uint32_t)state2.size()) != 0 ||
        ref_dump_state_to_dir(tmp2, 2) != 0)
        return fail("reload into a modified model failed");
    if (read_file(state2_path) != state2)
        return fail("reload retained palette state absent from snapshot");

    for (uint32_t init2 : {0u, 0x20000u, 0x20010u}) {
        for (uint32_t mode : {0x05060708u, 0x05064708u}) {
            ref_write_reg(0x218, init2);
            ref_write_reg(0x110, mode);
            for (unsigned swap = 0; swap < 3; ++swap) {
                ref_write_reg(0x128, 0);
                if (ref_dump_state_to_dir(tmp2, 2) != 0)
                    return fail("buffer-selection snapshot failed");
                const auto before = read_file(state2_path);
                if (before.empty() || ref_load_state(before.data(), (uint32_t)before.size()) != 0 ||
                    ref_dump_state_to_dir(tmp2, 2) != 0 || read_file(state2_path) != before)
                    return fail("buffer-selection state did not roundtrip");
            }
        }
    }
    ref_shutdown();
    fprintf(stderr, "state_roundtrip_test: PASS\n");
    return 0;
}
