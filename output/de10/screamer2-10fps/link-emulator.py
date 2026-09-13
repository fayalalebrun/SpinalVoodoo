from pathlib import Path
import argparse
import hashlib
import os
import re
import shlex
import subprocess

root = Path(__file__).resolve().parent
source = root / 'emulator-o3'
parser = argparse.ArgumentParser()
mode = parser.add_mutually_exclusive_group()
mode.add_argument('--pgo-generate', action='store_true')
mode.add_argument('--pgo-use', type=Path)
args = parser.parse_args()
if args.pgo_use and not args.pgo_use.is_file():
    parser.error('PGO profile does not exist')
build = root / ('emulator-pgo-train' if args.pgo_generate else 'emulator-pgo-use' if args.pgo_use else 'emulator-linked')
build.mkdir(exist_ok=True)
llvm = Path('/nix/store/2i1v8rlyvl0lhh6p4qx1ksj0g4r02j6c-llvm-21.1.7/bin')
compiler = Path('/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang++')
sysroot = root.parent / 'sysroot'
text = (source / 'src/Makefile').read_text().replace('\\\n', ' ')
variables = dict(re.findall(r'^([A-Za-z_][A-Za-z0-9_]*)[ \t]*=[ \t]*(.*)$', text, re.M))
def expand(value):
    return re.sub(r'\$\(([^)]+)\)', lambda m: expand(variables.get(m[1], '')), value)
inputs = list(dict.fromkeys(shlex.split(expand(variables['dosbox_x_OBJECTS'])) + shlex.split(expand(variables['dosbox_x_LDADD']))))
objects = []
for name in inputs:
    path = source / 'src' / name
    if path.suffix == '.a':
        target = build / name.replace('/', '_')
        target.mkdir(exist_ok=True)
        subprocess.run([str(llvm / 'llvm-ar'), 'x', str(path)], cwd=target, check=True)
        objects.extend(target.glob('*.o'))
    else:
        objects.append(path)
assert all(p.read_bytes()[:4] == b'BC\xc0\xde' for p in objects)
subprocess.run([str(llvm / 'llvm-link'), '-o', str(build / 'dosbox.bc'), *map(str, objects)], check=True)
env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
flags = ['--target=armv7a-linux-gnueabihf', '--sysroot=' + str(sysroot), '--gcc-install-dir=' + str(sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'), '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-O3', '-fno-pie', '-fno-lto']
pgo_flags = []
extra_objects = []
if args.pgo_generate:
    pgo_flags = ['-fprofile-generate', '-mllvm', '-disable-vp']
if args.pgo_use:
    original = root / 'emulator-pgo-train/dosbox.bc'
    if hashlib.sha256(original.read_bytes()).digest() != hashlib.sha256((build / 'dosbox.bc').read_bytes()).digest():
        raise RuntimeError('PGO input module differs from training module')
    pgo_flags = ['-fprofile-use=' + str(args.pgo_use.resolve()), '-mllvm', '-disable-vp']
compile_command = [str(compiler), *flags, *pgo_flags, '-mllvm', '-align-all-functions=5', '-ffunction-sections', '-fdata-sections', '-c', str(build / 'dosbox.bc'), '-o', str(build / 'dosbox.o')]
print(shlex.join(compile_command), flush=True)
subprocess.run(compile_command, env=env, check=True)
if args.pgo_generate:
    runtime = build / 'profile-runtime.o'
    subprocess.run([str(compiler), *flags, '-x', 'c', '-c', '-', '-o', str(runtime)], input='int __llvm_profile_runtime;\n', text=True, env=env, check=True)
    extra_objects.append(str(runtime))
command = [str(compiler), *flags, '-no-pie', '-Wl,--dynamic-linker=/lib/ld-linux-armhf.so.3', '--ld-path=' + str(root.parent / 'toolchain-bin/ld.lld'), '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf'), '-Wl,-z,max-page-size=4096', *([] if args.pgo_generate else ['-Wl,--gc-sections']), '-o', str(build / 'dosbox-x'), str(build / 'dosbox.o'), *extra_objects, *shlex.split(variables['LIBS'])]
print(shlex.join(command), flush=True)
subprocess.run(command, env=env, check=True)
subprocess.run([str(llvm / 'llvm-strip'), '--strip-debug', str(build / 'dosbox-x')], check=True)
print('BUILT', build / 'dosbox-x')
