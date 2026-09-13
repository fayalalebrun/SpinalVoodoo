from pathlib import Path
import argparse
import os
import re
import shutil
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--arm', action='store_true')
args = parser.parse_args()
source = root / 'emulator-o3'
build = root / 'fpu-test'
build.mkdir(exist_ok=True)
for name in ('regs.h', 'fpu.h'):
    shutil.copyfile(source / 'include' / name, build / name)
for name in ('mem.h', 'logging.h'):
    shutil.copyfile(root / 'flags-test' / name, build / name)
(build / 'cpu.h').write_text('#pragma once\n#include <string>\n#include "regs.h"\nconstexpr int FPU_ARCHTYPE_8087 = 0;\nextern int FPU_ArchitectureType;\n')
(build / 'mmx.h').write_text('#pragma once\n')
text = (source / 'src/fpu/fpu_instructions.h').read_text()
functions = ['FPU_PREP_PUSH', 'FPU_FPOP', 'FROUND', 'FPU_ST80', 'FPU_FLD_F32', 'FPU_FLD_F64', 'FPU_FLD_I16', 'FPU_FLD_I32', 'FPU_FLD_I64', 'FPU_FBLD', 'FPU_FST_F32', 'FPU_FST_F64', 'FPU_FST_F80', 'FPU_FST_I16', 'FPU_FST_I32', 'FPU_FST_I64', 'FPU_FBST', 'FPU_FXCH', 'FPU_FST']
code = '#define BIAS80 16383\n#define BIAS64 1023\n#define LONGTYPE(x) INT64_C(x)\n#define ULONGTYPE(x) UINT64_C(x)\n'
for name in functions:
    match = re.search(r'^static (?:void|double) ' + name + r'\([^\n]*\)\s*\{', text, re.M)
    assert match, name
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    code += text[match.start():end] + '\n'
dyn = (source / 'src/cpu/core_dynrec/dyn_fpu.h').read_text()
code += dyn[dyn.index('template <void (*load)(PhysPt, Bitu)>'):dyn.index('static INLINE void dyn_fpu_top()')]
(build / 'helpers.h').write_text(code)
compiler = ['c++', '-O2', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all']
output = build / 'fpu-test-host'
if args.arm:
    sysroot = root.parent / 'sysroot'
    compiler = ['/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang++', '--target=armv7a-linux-gnueabihf', '--sysroot=' + str(sysroot), '--gcc-install-dir=' + str(sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'), '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-mthumb', '-O3', '-fno-pie', '-no-pie', '--ld-path=' + str(root.parent / 'toolchain-bin/ld.lld'), '-Wl,--dynamic-linker=/lib/ld-linux-armhf.so.3', '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf')]
    output = build / 'fpu-test-arm'
env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
subprocess.run([*compiler, '-std=c++14', '-Wall', '-Wextra', '-Werror', '-I' + str(build), str(root / 'fpu-test.cpp'), '-o', str(output)], env=env, check=True)
if not args.arm:
    subprocess.run([str(output)], check=True)
