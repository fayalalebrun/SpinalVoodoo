from pathlib import Path
import argparse
import os
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--arm', action='store_true')
args = parser.parse_args()
source = (root / 'emulator-o3/src/cpu/core_dynrec/cache.h').read_text()
start = source.index('static INLINE void cache_check_space(') if 'static INLINE void cache_check_space(' in source else source.index('// place an 8bit value')
code = source[start:source.index('static void dyn_return(', start)]
build = root / 'cache-test'
build.mkdir(exist_ok=True)
prefix = '''#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#define INLINE inline
using Bitu = uintptr_t;
struct { uint8_t *pos, *emit_start; Bitu emit_size; } cache;
struct Full {};
[[noreturn]] static void E_Exit(const char*, ...) { throw Full{}; }
'''
main = '''
static void emit(unsigned width, uint8_t* pos, bool patch) {
    switch (width) {
    case 1: if (patch) cache_addb(0x12, pos); else cache_addb(0x12); break;
    case 2: if (patch) cache_addw(0x1234, pos); else cache_addw(0x1234); break;
    case 4: if (patch) cache_addd(0x12345678, pos); else cache_addd(0x12345678); break;
    case 8: if (patch) cache_addq(0x123456789abcdef0ull, pos); else cache_addq(0x123456789abcdef0ull); break;
    }
}
int main() {
    alignas(8) uint8_t buffer[256], before[256];
    unsigned checks = 0;
    for (bool patch : {false, true})
        for (unsigned width : {1u, 2u, 4u, 8u})
            for (unsigned size = 0; size <= 64; ++size)
                for (unsigned offset = 0; offset <= 160; offset += width) {
                    std::memset(buffer, 0xa5, sizeof(buffer));
                    std::memcpy(before, buffer, sizeof(buffer));
                    cache.emit_start = buffer + 64;
                    cache.emit_size = size;
                    cache.pos = buffer + offset;
                    bool rejected = false;
                    try { emit(width, cache.pos, patch); } catch (const Full&) { rejected = true; }
                    const bool invalid = offset < 64 || offset > 64 + size || width > 64 + size - offset;
                    if (rejected != invalid || (invalid && (std::memcmp(buffer, before, sizeof(buffer)) || cache.pos != buffer + offset))) {
                        std::fprintf(stderr, "FAIL width=%u offset=%u size=%u patch=%u rejected=%u expected=%u\\n", width, offset, size, unsigned(patch), unsigned(rejected), unsigned(invalid));
                        return 1;
                    }
                    if (!invalid) {
                        const uint64_t values[] = {0x12, 0x1234, 0x12345678, 0x123456789abcdef0ull};
                        const uint64_t value = values[width == 1 ? 0 : width == 2 ? 1 : width == 4 ? 2 : 3];
                        std::memcpy(before + offset, &value, width);
                        if (std::memcmp(buffer, before, sizeof(buffer)) || cache.pos != buffer + offset + (patch ? 0 : width)) return 1;
                    }
                    ++checks;
                }
    std::printf("PASS %u cache emission boundary checks: byte/word/dword/qword, append/patch, exact fit, lower/upper overflow, no write or pointer change on rejection\\n", checks);
}
'''
file = build / 'cache-test.cpp'
file.write_text(prefix + code + main)
flags = ['c++', '-O2', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
output = build / 'cache-test-host'
if args.arm:
    sysroot = root.parent / 'sysroot'
    flags = ['/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang++', '--target=armv7a-linux-gnueabihf', '--sysroot=' + str(sysroot), '--gcc-install-dir=' + str(sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'), '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-mthumb', '-O3', '-fno-pie', '-no-pie', '--ld-path=' + str(root.parent / 'toolchain-bin/ld.lld'), '-Wl,--dynamic-linker=/lib/ld-linux-armhf.so.3', '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf')]
    output = build / 'cache-test-arm'
env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
subprocess.run([*flags, '-std=c++14', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', str(file), '-o', str(output)], env=env, check=True)
if not args.arm:
    subprocess.run([str(output)], check=True)
