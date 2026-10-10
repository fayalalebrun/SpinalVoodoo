import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'software/integrations/dosbox/machine-profile.py'
spec = importlib.util.spec_from_file_location('machine_profile', SOURCE)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class MachineProfileTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.games = self.root / 'games'
        self.games.mkdir()
        (self.games / 'installed').mkdir()
        for name in ('disk.vhd', 'floppy.img', 'disc.iso'):
            (self.games / name).write_bytes(b'fixture')

    def test_dos_prompt_and_declarative_settings(self):
        text = module.render({'version': 1}, self.games)
        self.assertIn('core=normal\ncycles=auto', text)
        self.assertTrue(text.endswith('[autoexec]\nZ:\n'))
        profile = {'version': 1, 'cpu': 'dynamic', 'cycles': 40000, 'memory_mb': 32, 'audio': 'sb16',
            'drives': {'C': {'type': 'directory', 'path': 'installed'},
                       'A': {'type': 'floppy', 'path': 'floppy.img'},
                       'D': {'type': 'iso', 'path': 'disc.iso'}}}
        text = module.render(profile, self.games)
        self.assertIn('cycles=40000', text)
        self.assertIn('nosound=false', text)
        self.assertIn('sbtype=sb16', text)
        self.assertIn('mount C "' + str(self.games / 'installed') + '" -ro', text)
        self.assertIn('-t floppy -ro', text)
        self.assertIn('-t iso -ro', text)
        self.assertTrue(text.endswith('C:\n'))
        hdd = module.render({'version': 1, 'drives': {'C': {'type': 'hdd', 'path': 'disk.vhd'}}}, self.games)
        self.assertIn('-t hdd -ro', hdd)

    def test_explicit_raw_writes_and_leased_paths(self):
        (self.games / 'floppy.img').write_bytes(bytes(1024))
        profile = {'version': 1, 'drives': {'A': {'type': 'floppy', 'path': 'floppy.img', 'writable': True}}}
        self.assertIn('-t floppy\n', module.render(profile, self.games))
        text = module.render(profile, self.games, media_paths={'A': '/proc/self/fd/8'})
        self.assertIn('imgmount A "/proc/self/fd/8" -t floppy\n', text)
        with self.assertRaises(ValueError):
            module.render(profile, self.games, media_paths={'A': '/other/file'})
        for writable in (1, 'yes', None):
            profile['drives']['A']['writable'] = writable
            with self.assertRaises(ValueError):
                module.render(profile, self.games)
        for letter, kind, path in [('C', 'directory', 'installed'), ('C', 'hdd', 'disk.vhd'), ('D', 'iso', 'disc.iso')]:
            with self.assertRaises(ValueError):
                module.render({'version': 1, 'drives': {letter: {'type': kind, 'path': path, 'writable': True}}}, self.games)
        (self.games / 'floppy.img').write_bytes(b'invalid size')
        profile['drives']['A']['writable'] = True
        with self.assertRaises(ValueError):
            module.render(profile, self.games)

    def test_invalid_settings(self):
        for field, value in (('version', True), ('cpu', 'shell'), ('cycles', True), ('cycles', 0),
                             ('cycles', 'max\n[autoexec]'), ('memory_mb', 1024), ('audio', 'anything'),
                             ('autostart', 'TOMB.EXE'), ('drives', [])):
            with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                module.render(dict(version=1, **{field: value}) if field != 'version' else {'version': value}, self.games)

    def test_paths_drive_types_and_command_injection_fail_closed(self):
        outside = self.root / 'outside.img'
        outside.write_bytes(b'preserved')
        (self.games / 'escape.img').symlink_to(outside)
        (self.games / 'bad&command.img').write_bytes(b'fixture')
        for letter, kind, path in (('A', 'floppy', '../outside.img'), ('A', 'floppy', str(outside)),
                                  ('A', 'floppy', 'escape.img'), ('A', 'floppy', 'bad&command.img'),
                                  ('C', 'floppy', 'floppy.img'), ('A', 'directory', 'installed'),
                                  ('C', 'hdd', 'disc.iso'), ('E', 'iso', 'installed'), ('Z', 'hdd', 'disk.vhd')):
            with self.subTest(path=path, letter=letter), self.assertRaises(ValueError):
                module.render({'version': 1, 'drives': {letter: {'type': kind, 'path': path}}}, self.games)
        with self.assertRaises(FileNotFoundError):
            module.render({'version': 1, 'drives': {'A': {'type': 'floppy', 'path': 'missing.img'}}}, self.games)
        self.assertEqual(outside.read_bytes(), b'preserved')

    def test_cli_success_and_duplicate_or_oversized_profile(self):
        profile = self.root / 'profile.json'
        def run(content):
            profile.write_text(content)
            return subprocess.run([sys.executable, str(SOURCE), str(profile), '--games-root', str(self.games)], capture_output=True, text=True)
        good = run(json.dumps({'version': 1}))
        self.assertEqual(good.returncode, 0, good.stderr)
        self.assertIn('[autoexec]', good.stdout)
        for content in ('{"version":1,"version":1}', ' ' * 65537, '{invalid'):
            result = run(content)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, '')


if __name__ == '__main__':
    unittest.main()
