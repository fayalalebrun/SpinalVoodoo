from pathlib import Path
import argparse
import os
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--arm', action='store_true')
args = parser.parse_args()
source = (root / 'emulator-o3/src/hardware/glide.cpp').read_text()
start = source.index('\n', source.index('    // Process function parameters (80 bytes)')) + 1
fragment = source[start:source.index('    process_msg(val);', start)]
build = root / 'glide-param-test'
build.mkdir(exist_ok=True)
code = r'''
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321
#define SDL_memcpy std::memcpy
using PhysPt = uint32_t;
using HostPt = uint8_t*;
static std::array<uint8_t, 0x101000> ram;
static uint32_t param[20];
static uint32_t missing_page;
static unsigned probes, blocks, fault_at;
static bool warm;
static std::vector<uint32_t> accesses;
struct Fault {};
static PhysPt PhysMake(uint16_t segment, uint16_t offset) { return (uint32_t(segment) << 4) + offset; }
static HostPt get_tlb_read(PhysPt address) {
    ++probes;
    return (address >> 12) == missing_page ? nullptr : ram.data();
}
static uint32_t read_word(PhysPt address) {
    if (!get_tlb_read(address)) {
        accesses.push_back(address);
        if (accesses.size() == fault_at) throw Fault{};
        if (warm) missing_page = UINT32_MAX;
    }
    return uint32_t(ram.at(address)) | uint32_t(ram.at(address + 1)) << 8 |
           uint32_t(ram.at(address + 2)) << 16 | uint32_t(ram.at(address + 3)) << 24;
}
static void MEM_BlockRead32(PhysPt address, void* dest, size_t size) {
    ++blocks;
    auto* out = static_cast<uint32_t*>(dest);
    for (size_t i = 0; i < size / 4; ++i) out[i] = read_word(address + uint32_t(4 * i));
}
static void candidate(uint16_t glsegment) {
''' + fragment + r'''
}
struct Result {
    std::array<uint32_t, 20> words;
    std::vector<uint32_t> reads;
    unsigned probes, blocks;
    bool fault;
};
static Result run(uint16_t segment, unsigned mode, bool test) {
    probes = blocks = 0;
    accesses.clear();
    std::fill(std::begin(param), std::end(param), 0xa5a5a5a5u);
    const uint32_t page = PhysMake(segment, 0) >> 12;
    missing_page = mode == 0 ? UINT32_MAX : mode == 2 ? page + 1 : page;
    fault_at = mode == 2 ? 1 : mode == 3 ? 3 : UINT32_MAX;
    warm = mode == 4;
    bool fault = false;
    try {
        if (test) candidate(segment);
        else MEM_BlockRead32(PhysMake(segment, 0), param, sizeof(param));
    } catch (const Fault&) { fault = true; }
    Result result{{}, accesses, probes, blocks, fault};
    std::copy(std::begin(param), std::end(param), result.words.begin());
    return result;
}
int main() {
    for (size_t i = 0; i < ram.size(); ++i) ram[i] = uint8_t((i * 37) ^ (i >> 5) ^ (i >> 13));
    unsigned checks = 0, fast = 0, faults = 0;
    for (uint32_t segment = 0; segment <= UINT16_MAX; ++segment) {
        for (unsigned mode = 0; mode < 5; ++mode) {
            const auto reference = run(uint16_t(segment), mode, false);
            const auto actual = run(uint16_t(segment), mode, true);
            if (actual.words != reference.words || actual.reads != reference.reads || actual.fault != reference.fault) {
                std::fprintf(stderr, "FAIL semantics segment=%u mode=%u\n", segment, mode);
                return 1;
            }
            const bool eligible = SDL_BYTEORDER == SDL_LIL_ENDIAN && (mode == 0 || mode == 2) &&
                                  (PhysMake(uint16_t(segment), 0) & 4095) <= 4096 - sizeof(param);
            if (actual.blocks != unsigned(!eligible) || (eligible && actual.probes != 1)) {
                std::fprintf(stderr, "FAIL copy path segment=%u mode=%u blocks=%u probes=%u fast=%u\n",
                             segment, mode, actual.blocks, actual.probes, unsigned(eligible));
                return 1;
            }
            ++checks;
            fast += eligible;
            faults += actual.fault;
        }
    }
    std::printf("PASS %u Glide argument copies, %u single-probe copies, %u partial faults; all segments, page edges, TLB misses/warming, handler order, endian=%u\n",
                checks, fast, faults, unsigned(SDL_BYTEORDER));
}
'''
cpp = build / 'glide-param-test.cpp'
cpp.write_text(code)
flags = ['c++', '-O2', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
if args.arm:
    sysroot = root.parent / 'sysroot'
    flags = ['/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang++',
             '--target=armv7a-linux-gnueabihf', '--sysroot=' + str(sysroot),
             '--gcc-install-dir=' + str(sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'),
             '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-O3', '-fno-pie', '-no-pie',
             '--ld-path=' + str(root.parent / 'toolchain-bin/ld.lld'),
             '-Wl,--dynamic-linker=/lib/ld-linux-armhf.so.3',
             '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf')]
env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
for endian, value in [('le', 1234), ('be', 4321)]:
    output = build / ('glide-param-test-' + ('arm-' if args.arm else 'host-') + endian)
    subprocess.run([*flags, '-std=c++14', '-Wall', '-Wextra', '-Werror', '-DSDL_BYTEORDER=' + str(value), str(cpp), '-o', str(output)], env=env, check=True)
    if not args.arm:
        subprocess.run([str(output)], check=True)
