from pathlib import Path
import argparse
import os
import re
import shutil
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--arm', action='store_true')
parser.add_argument('--thumb', action='store_true')
args = parser.parse_args()
if args.thumb and not args.arm:
    parser.error('--thumb requires --arm')
source = root / 'emulator-o3'
original = root / 'emulator/dosboxx-src'
build = root / 'flags-test'
build.mkdir(exist_ok=True)
for name in ('regs.h',):
    shutil.copyfile(source / 'include' / name, build / name)
(build / 'mem.h').write_text('''#pragma once
#include <cstdint>
using Bitu = uint32_t;
using PhysPt = uint32_t;
using RealPt = uint32_t;
#define INLINE inline
inline RealPt RealMake(uint16_t seg, uint16_t off) { return (uint32_t(seg) << 16) | off; }
''')
(build / 'cpu.h').write_text('''#pragma once
#include <iostream>
#include "regs.h"
#define WRITE_POD(ptr, obj) stream.write(reinterpret_cast<const char*>(ptr), sizeof(obj))
#define READ_POD(ptr, obj) stream.read(reinterpret_cast<char*>(ptr), sizeof(obj))
''')
(build / 'logging.h').write_text('''#pragma once
extern unsigned log_count;
inline void test_log(const char*, ...) { ++log_count; }
#define LOG(a, b) test_log
''')
def wrappers(path):
    text = path.read_text()
    return text[text.index('static uint32_t DRC_CALL_CONV dynrec_get_of'):text.index('static void dyn_branchflag_to_reg')]
base = wrappers(original / 'src/cpu/core_dynrec/operators.h').replace('dynrec_get_', 'reference_get_')
candidate = wrappers(source / 'src/cpu/core_dynrec/operators.h')
decode = (source / 'src/cpu/core_dynrec/decoder_basic.h').read_text()
enum = re.search(r'enum BranchTypes \{.*?\};', decode, re.S).group()
names = ['of', 'nof', 'cf', 'ncf', 'zf', 'nzf', 'cf_or_zf', 'ncf_and_nzf', 'sf', 'nsf', 'pf', 'npf', 'sf_neq_of', 'sf_eq_of', 'zf_or_sf_neq_of', 'nzf_and_sf_eq_of']
prefix = '#include "cpu.h"\n#include "lazyflags.h"\n'
if (source / 'src/cpu/core_dynrec/branch_flags.h').exists():
    candidate = '#include "branch_flags.h"\n' + candidate
prefix += '#define DRC_CALL_CONV\n#define DRC_FC\n' + enum + '\n'
prefix += base + candidate
unary_pattern = r'static uint(?:8|16|32)_t DRC_CALL_CONV dynrec_(?:inc|dec)_(?:byte|word|dword)\([^)]*\) \{.*?\n\}'
for label, tree in [('reference', original), ('candidate', source)]:
    text = (tree / 'src/cpu/core_dynrec/operators.h').read_text()
    if label == 'candidate' and 'static inline void dynrec_preserve_cf()' in text:
        prefix += re.search(r'static inline void dynrec_preserve_cf\(\) \{.*?\n\}', text, re.S).group() + '\n'
    unary = re.findall(unary_pattern, text, re.S)
    assert len(unary) == 6
    prefix += '\n'.join(unary).replace('dynrec_', label + '_') if label == 'reference' else '\n'.join(unary)
    stem = 'reference_' if label == 'reference' else 'dynrec_'
    names_unary = [op + '_' + width for width in ('byte', 'word', 'dword') for op in ('inc', 'dec')]
    for name in names_unary:
        prefix += '\nstatic uint32_t ' + label + '_' + name + '_u32(uint32_t op) { return ' + stem + name + '(op); }\n'
    prefix += 'uint32_t (*' + label + '_unary[])(uint32_t) = {' + ','.join(label + '_' + name + '_u32' for name in names_unary) + '};\n'
prefix += '\nusing Getter = uint32_t (*)();\n'
for label, stem in [('reference', 'reference_get_'), ('candidate', 'dynrec_get_')]:
    prefix += 'Getter ' + label + '[] = {' + ','.join(stem + n for n in names) + '};\n'
(build / 'wrappers.h').write_text(prefix)
compiler = ['c++', '-O2', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
output = build / 'flags-test-host'
if args.arm:
    sysroot = root.parent / 'sysroot'
    compiler = ['/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang++',
                '--target=armv7a-linux-gnueabihf', '--sysroot=' + str(sysroot),
                '--gcc-install-dir=' + str(sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'),
                '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-O3', '-fno-pie', '-no-pie',
                '--ld-path=' + str(root.parent / 'toolchain-bin/ld.lld'),
                '-Wl,--dynamic-linker=/lib/ld-linux-armhf.so.3',
                '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf')]
    output = build / 'flags-test-arm'
    if args.thumb:
        compiler.append('-mthumb')
        output = build / 'flags-test-thumb'
command = [*compiler, '-std=c++14', '-Wall', '-Wextra', '-Werror', '-I' + str(build), '-I' + str(source / 'src/cpu'), '-I' + str(source / 'src/cpu/core_dynrec'), str(original / 'src/cpu/flags.cpp'), str(root / 'flags-test.cpp'), '-o', str(output)]
env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
subprocess.run(command, env=env, check=True)
if not args.arm:
    subprocess.run([str(output)], check=True)
