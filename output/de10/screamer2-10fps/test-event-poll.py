from pathlib import Path
import argparse
import os
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--arm', action='store_true')
args = parser.parse_args()
source = (root / 'emulator-o3/src/gui/sdlmain.cpp').read_text()
start = source.index('\n', source.index('bool gfx_in_mapper = false;')) + 1
fragment = source[start:source.index('    SDL_Event event;', start)]
build = root / 'event-poll-test'
build.mkdir(exist_ok=True)
prefix = r'''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#undef MACOSX
#undef LINUX
#undef WIN32
#undef __arm__
#undef C_SDL2
static uint32_t ticks;
static unsigned layout_checks, polls;
static std::vector<unsigned> queued, delivered;
static uint32_t GetTicks() { return ticks; }
static void CheckMapperKeyboardLayout() { ++layout_checks; }
'''
suffix = r'''
#endif
    ++polls;
    delivered.insert(delivered.end(), queued.begin(), queued.end());
    queued.clear();
}
static uint32_t reference_last;
static unsigned reference_polls, calls, checks;
static std::vector<unsigned> reference_queued, reference_delivered;
static void step(uint64_t time, unsigned repeats, unsigned interval) {
    ticks = uint32_t(time);
    const unsigned event = ++checks;
    queued.push_back(event);
    reference_queued.push_back(event);
    for (unsigned i = 0; i < repeats; ++i) {
        ++calls;
        if (!interval || uint32_t(ticks - reference_last) >= interval) {
            reference_last = ticks;
            ++reference_polls;
            reference_delivered.insert(reference_delivered.end(), reference_queued.begin(), reference_queued.end());
            reference_queued.clear();
        }
        GFX_Events();
        if (polls != reference_polls || delivered != reference_delivered || queued != reference_queued || layout_checks != calls) {
            std::fprintf(stderr, "FAIL event polling ticks=%u interval=%u polls=%u expected=%u calls=%u\n",
                         ticks, interval, polls, reference_polls, calls);
            std::exit(1);
        }
    }
}
int main() {
    const unsigned interval = INTERVAL;
    for (uint64_t time = 0; time <= 160; ++time) step(time, 5, interval);
    for (uint64_t base : {uint64_t(INT32_MAX) - 8, uint64_t(UINT32_MAX) - 8,
                          uint64_t(UINT32_MAX) * 2 - 8}) {
        for (uint64_t time = base; time <= base + 20; ++time) step(time, 4, interval);
    }
    step(uint64_t(UINT32_MAX) * 2 + 32, 1, interval);
    if (!queued.empty() || delivered.size() != checks) return 1;
    std::printf("PASS %u event times, %u calls, %u polls, %zu ordered events; interval=%u ms, repeated ticks, signed boundary, two clock rollovers, full queue drain, layout checks unchanged\n",
                checks, calls, polls, delivered.size(), interval);
}
'''
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
for platform, defines, interval in [('linux-arm-sdl2', ['LINUX', '__arm__', 'C_SDL2'], 4),
                                    ('linux-sdl2', ['LINUX', 'C_SDL2'], 0),
                                    ('mac-sdl2', ['MACOSX', 'C_SDL2'], 4),
                                    ('win-sdl2', ['WIN32', 'C_SDL2'], 0),
                                    ('linux-arm-sdl1', ['LINUX', '__arm__'], 0)]:
    cpp = build / (platform + '.cpp')
    cpp.write_text(prefix + ''.join('#define ' + name + ' 1\n' for name in defines) + fragment + suffix)
    output = build / ('event-poll-' + ('arm-' if args.arm else 'host-') + platform)
    subprocess.run([*flags, '-std=c++14', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                    '-DINTERVAL=' + str(interval), str(cpp), '-o', str(output)], env=env, check=True)
    if not args.arm:
        subprocess.run([str(output)], check=True)
