from pathlib import Path
import importlib.util
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

root = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('export_pgo', root / 'export-pgo.py')
export = importlib.util.module_from_spec(spec)
spec.loader.exec_module(export)


class ExportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.binary = root / 'pgo-smoke'
        cls.blob = cls.binary.read_bytes()
        cls.shoff = struct.unpack_from('<I', cls.blob, 32)[0]
        size, count, strings = struct.unpack_from('<HHH', cls.blob, 46)
        cls.sections = [struct.unpack_from('<10I', cls.blob, cls.shoff + i * size) for i in range(count)]
        table = cls.sections[strings]
        names = cls.blob[table[4]:table[4] + table[5]]
        cls.indices = {names[s[0]:names.index(b'\0', s[0])].decode(): i for i, s in enumerate(cls.sections)}

    def invalid(self, offset, replacement):
        blob = bytearray(self.blob)
        blob[offset:offset + len(replacement)] = replacement
        with tempfile.TemporaryDirectory(dir=root) as directory:
            path = Path(directory) / 'invalid'
            path.write_bytes(blob)
            with self.assertRaises((ValueError, zlib.error)):
                export.profile(path)

    def test_profile(self):
        description, records = export.profile(self.binary)
        self.assertEqual(description['functions'], 2)
        self.assertEqual(description['size'], 32)
        self.assertEqual(description['version'], (1 << 56) | 10)
        counters = (root / 'pgo-smoke.counters').read_bytes()
        for (name, function_hash), (offset, number) in records.items():
            self.assertGreater(function_hash, 0)
            self.assertEqual(number, 2)
            expected = (100, 1) if name == 'main' else (100, 50)
            self.assertTrue(name == 'main' or name.endswith(';work'))
            self.assertEqual(struct.unpack_from('<2Q', counters, offset), expected)

    def test_regions_and_strings(self):
        self.assertEqual(export.region(b'ab', 1, 1), b'b')
        self.assertEqual(export.string(b'a\0', 0), 'a')
        for offset, size in ((-1, 1), (0, -1), (1, 2)):
            with self.subTest(offset=offset, size=size), self.assertRaises(ValueError):
                export.region(b'ab', offset, size)
        for data, offset in ((b'a', 0), (b'a\0', 2), (b'a\0', -1)):
            with self.subTest(data=data, offset=offset), self.assertRaises(ValueError):
                export.string(data, offset)

    def test_uleb(self):
        self.assertEqual(export.uleb(b'\xac\x02', 0), (300, 2))
        self.assertEqual(export.uleb(b'\xff' * 9 + b'\x01', 0), ((1 << 64) - 1, 10))
        for data in (b'', b'\x80', b'\x80' * 10, b'\x80' * 9 + b'\x02'):
            with self.subTest(data=data), self.assertRaises(ValueError):
                export.uleb(data, 0)

    def test_headers(self):
        for offset, replacement in ((0, b'BAD!'), (4, b'\x02'), (5, b'\x02'), (16, b'\x03\0'), (18, b'\x3e\0'), (46, b'\x27\0'), (48, b'\0\0'), (50, b'\xff\xff')):
            with self.subTest(offset=offset):
                self.invalid(offset, replacement)
        with tempfile.TemporaryDirectory(dir=root) as directory:
            path = Path(directory) / 'short'
            path.write_bytes(self.blob[:20])
            with self.assertRaises(ValueError):
                export.profile(path)

    def test_record_fields(self):
        offset = self.sections[self.indices['__llvm_prf_data']][4]
        for relative, replacement in ((0, bytes(8)), (16, struct.pack('<i', 0x7fffffff)), (32, bytes(4)), (36, b'\x01\0'), (44, b'\x01\0\0\0')):
            with self.subTest(relative=relative):
                self.invalid(offset + relative, replacement)

    def test_record_sizes(self):
        for name in ('__llvm_prf_data', '__llvm_prf_cnts'):
            index = self.indices[name]
            self.invalid(self.shoff + index * 40 + 20, struct.pack('<I', self.sections[index][5] - 1))

    def test_version(self):
        encoded = struct.pack('<Q', (1 << 56) | 10)
        self.assertEqual(self.blob.count(encoded), 1)
        self.invalid(self.blob.index(encoded), struct.pack('<Q', (1 << 56) | 9))

    def test_names(self):
        section = self.sections[self.indices['__llvm_prf_names']]
        offset = section[4]
        self.invalid(offset, b'\x7f')
        self.invalid(offset + 2, b'\0')

    def test_duplicate_sections(self):
        source = self.sections[self.indices['__llvm_prf_cnts']]
        self.invalid(self.shoff + self.indices['.interp'] * 40, struct.pack('<I', source[0]))

    def test_cli_and_llvm_reader(self):
        command = [sys.executable, str(root / 'export-pgo.py'), str(self.binary)]
        for arguments in ([], ['--describe', str(root / 'pgo-smoke.counters')]):
            result = subprocess.run([*command, *arguments], capture_output=True)
            self.assertEqual(result.returncode, 2)
        with tempfile.TemporaryDirectory(dir=root) as directory:
            path = Path(directory) / 'counts'
            path.write_bytes(bytes(1))
            self.assertEqual(subprocess.run([*command, str(path)], capture_output=True).returncode, 2)
            result = subprocess.run([*command, str(root / 'pgo-smoke.counters')], capture_output=True, check=True)
            text = Path(directory) / 'profile.txt'
            text.write_bytes(result.stdout)
            merged = Path(directory) / 'profile.data'
            llvm = '/nix/store/2i1v8rlyvl0lhh6p4qx1ksj0g4r02j6c-llvm-21.1.7/bin/llvm-profdata'
            subprocess.run([llvm, 'merge', str(text), '-o', str(merged)], check=True)
            shown = subprocess.check_output([llvm, 'show', '--all-functions', '--counts', str(merged)], text=True)
            self.assertIn('Block counts: [100, 1]', shown)
            self.assertIn('Block counts: [100, 50]', shown)
            self.assertIn('Instrumentation level: IR', shown)


if __name__ == '__main__':
    unittest.main()
