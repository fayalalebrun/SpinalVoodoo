
import os
from pathlib import Path
import runpy
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class JitDumpTest(unittest.TestCase):
    def test_records_reuse_disabled_and_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)
            source = path / 'test.cpp'
            source.write_text('''#include "dosbox_jitdump.h"
int main(int argc, char **) {
    unsigned char code[] = {0x90, 0xc3};
    if (argc > 1) {
        char name[64];
        std::snprintf(name, sizeof(name), "%s/jit-%ld.dump", std::getenv("MISTER_JITDUMP_DIR"), long(getpid()));
        FILE *existing = std::fopen(name, "w");
        if (!existing) return 2;
        std::fputs("preserve", existing); std::fclose(existing);
    }
    mister_jit::load(code, code, sizeof(code), "dynrec-dispatch");
    code[0] = 0xcc;
    mister_jit::load(code, code, sizeof(code), "dynrec-guest-linear-00001234-32bit");
}
''')
            binary = path / 'test'
            subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                            '-I' + str(ROOT / 'software/integrations/dosbox'), str(source), '-o', str(binary)], check=True)
            env = dict(os.environ)
            env.pop('MISTER_JITDUMP_DIR', None)
            subprocess.run([str(binary)], env=env, check=True)
            self.assertFalse(list(path.glob('jit-*.dump')))
            env['MISTER_JITDUMP_DIR'] = str(path)
            subprocess.run([str(binary)], env=env, check=True)
            dump = next(path.glob('jit-*.dump')).read_bytes()
            magic, version, size, machine, pad, pid, stamp, flags = struct.unpack_from('<6I2Q', dump)
            self.assertEqual((magic, version, size, pad, flags), (0x4a695444, 1, 40, 0, 0))
            self.assertIn(machine, (40, 62, 183))
            records = []
            offset = size
            while offset < len(dump):
                kind, length, time = struct.unpack_from('<IIQ', dump, offset)
                self.assertGreaterEqual(time, stamp)
                stamp = time
                if kind == 0:
                    p, tid, vma, address, count, index = struct.unpack_from('<II4Q', dump, offset + 16)
                    name_end = dump.index(b'\0', offset + 56)
                    name = dump[offset + 56:name_end].decode()
                    code = dump[name_end + 1:offset + length]
                    self.assertEqual((p, tid, vma, count), (pid, pid, address, len(code)))
                    records.append((address, index, name, code))
                else:
                    self.assertEqual((kind, length), (3, 16))
                offset += length
            self.assertEqual(offset, len(dump))
            self.assertEqual([r[1] for r in records], [1, 2])
            self.assertEqual(records[0][0], records[1][0])
            self.assertEqual(records[0][3], b'\x90\xc3')
            self.assertEqual(records[1][3], b'\xcc\xc3')
            self.assertIn('guest-linear-00001234', records[1][2])
            env['MISTER_JITDUMP_DIR'] = 'relative'
            self.assertNotEqual(subprocess.run([str(binary)], env=env, capture_output=True).returncode, 0)
            env['MISTER_JITDUMP_DIR'] = str(path / 'absent')
            self.assertNotEqual(subprocess.run([str(binary)], env=env, capture_output=True).returncode, 0)
            env['MISTER_JITDUMP_DIR'] = str(path)
            self.assertNotEqual(subprocess.run([str(binary), 'collision'], env=env, capture_output=True).returncode, 0)
            self.assertTrue(any(p.read_bytes() == b'preserve' for p in path.glob('jit-*.dump')))

    def test_patch_sites(self):
        integrate = runpy.run_path(str(ROOT / 'diagnostics/profiling/enable-mister-jitdump'))['integrate']
        cache = ('dyn_return(BR_Link1,false);\ndyn_return(BR_Link2,false);\n' * 2 + '\t\tdyn_run_code();')
        decoder = '\tcache_block_closing(decode.block->cache.start,decode.block->cache.size);'
        patched, code = integrate(cache, decoder)
        self.assertEqual(patched.count('mister_jit::load'), 5)
        self.assertIn('decode.block->cache.xstart', code)
        self.assertIn('decode.code_start', code)
        with self.assertRaises(ValueError): integrate('', decoder)
        with self.assertRaises(ValueError): integrate(cache, decoder + decoder)
        enable = runpy.run_path(str(ROOT / 'diagnostics/profiling/enable-mister-jitdump'))['enable']
        with self.assertRaises(ValueError): enable(ROOT / 'build/deps/mister/dosbox-x')
        with self.assertRaises(ValueError): enable(Path('/tmp/outside'))
