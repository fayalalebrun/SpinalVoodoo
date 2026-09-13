from pathlib import Path
import ast
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
LLVM = Path('/nix/store/2i1v8rlyvl0lhh6p4qx1ksj0g4r02j6c-llvm-21.1.7/bin')
CLANG = '/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang'


class ProfileFlagsTest(unittest.TestCase):
    def test_ir_instrumentation_needs_only_plain_counters(self):
        tree = ast.parse((ROOT / 'link-emulator.py').read_text())
        flags = next(ast.literal_eval(node.value) for node in ast.walk(tree)
                     if isinstance(node, ast.Assign) and isinstance(node.value, ast.List)
                     and any(isinstance(target, ast.Name) and target.id == 'pgo_flags' for target in node.targets)
                     and '-fprofile-generate' in ast.literal_eval(node.value))
        env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
        with tempfile.TemporaryDirectory(dir=ROOT) as directory:
            path = Path(directory)
            source = path / 'test.c'
            source.write_text('typedef unsigned (*Function)(unsigned);\nunsigned work(Function f, void *out, const void *in, unsigned size) { __builtin_memcpy(out, in, size); return f(size); }\nint __llvm_profile_runtime;\nint main(void) { return 0; }\n')
            for mode in ('-marm', '-mthumb'):
                with self.subTest(mode=mode):
                    common = ['--target=armv7a-linux-gnueabihf', '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-O3', '-fno-pie', mode]
                    subprocess.run([CLANG, *common, '-emit-llvm', '-c', str(source), '-o', str(path / 'test.bc')], env=env, check=True, capture_output=True)
                    subprocess.run([CLANG, *common, *flags, '-c', str(path / 'test.bc'), '-o', str(path / 'test.o')], env=env, check=True, capture_output=True)
                    symbols = subprocess.check_output([str(LLVM / 'llvm-nm'), str(path / 'test.o')], text=True)
                    self.assertIn('__profc_work', symbols)
                    self.assertNotIn('__llvm_profile_instrument_target', symbols)
                    self.assertNotIn('__llvm_profile_instrument_memop', symbols)
                    sysroot = ROOT.parent / 'sysroot'
                    executable = path / 'training'
                    link_flags = ['--sysroot=' + str(sysroot), '--gcc-install-dir=' + str(sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'), '--ld-path=' + str(ROOT.parent / 'toolchain-bin/ld.lld'), '-Wl,--dynamic-linker=/lib/ld-linux-armhf.so.3', '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf'), '-no-pie']
                    subprocess.run([CLANG, *common, *link_flags, str(path / 'test.o'), '-o', str(executable)], env=env, check=True, capture_output=True)
                    exporter = [sys.executable, str(ROOT / 'export-pgo.py'), str(executable)]
                    description = json.loads(subprocess.check_output([*exporter, '--describe'], text=True))
                    counts = path / 'counts'
                    counts.write_bytes(struct.pack('<Q', 1) * (description['size'] // 8))
                    text = path / 'profile.txt'
                    text.write_bytes(subprocess.check_output([*exporter, str(counts)]))
                    profile = path / 'profile.data'
                    subprocess.run([str(LLVM / 'llvm-profdata'), 'merge', str(text), '-o', str(profile)], check=True, capture_output=True)
                    use = next(node.value for node in ast.walk(tree) if isinstance(node, ast.Assign) and isinstance(node.value, ast.List) and node.value.elts and isinstance(node.value.elts[0], ast.BinOp) and isinstance(node.value.elts[0].left, ast.Constant) and node.value.elts[0].left.value == '-fprofile-use=')
                    use_flags = ['-fprofile-use=' + str(profile), *(ast.literal_eval(flag) for flag in use.elts[1:])]
                    consumed = subprocess.run([CLANG, *common, *use_flags, '-c', str(path / 'test.bc'), '-o', str(path / 'optimized.o')], env=env, check=True, capture_output=True, text=True)
                    self.assertNotIn('Inconsistent number of value sites', consumed.stderr)
                    optimized_symbols = subprocess.check_output([str(LLVM / 'llvm-nm'), str(path / 'optimized.o')], text=True)
                    self.assertNotIn('__profc_', optimized_symbols)


if __name__ == '__main__':
    unittest.main()
