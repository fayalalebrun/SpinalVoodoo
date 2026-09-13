from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parent
source = root / 'emulator-o3'
sysroot = root.parent / 'sysroot'
compiler = Path('/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang++')
linker = root.parent / 'toolchain-bin/ld.lld'
gcc = sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'
args = sys.argv[1:]
if args[0] == '--c':
    compiler = compiler.with_name('clang')
    args.pop(0)
args = [arg.replace('/home/fpga/de10-cross/dosboxx-src', str(source)).replace('-I/usr/', '-I' + str(sysroot) + '/usr/').replace('-L/usr/', '-L' + str(sysroot) + '/usr/') for arg in args]
args += ['--target=armv7a-linux-gnueabihf', '--sysroot=' + str(sysroot), '--gcc-install-dir=' + str(gcc), '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-O3', '-fno-pie', '-flto=thin']
if '-c' not in args:
    args += ['-no-pie', '-fuse-ld=' + str(linker), '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf'), '-Wl,-z,max-page-size=4096']
env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
sys.exit(subprocess.run([str(compiler), *args], env=env).returncode)
