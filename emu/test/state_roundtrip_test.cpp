#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "ref_model.h"
#include "../../emu/trace/voodoo_trace_format.h"
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

    ref_shutdown();
    fprintf(stderr, "state_roundtrip_test: PASS\n");
    return 0;
}
