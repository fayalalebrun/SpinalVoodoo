import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]

CHILD = '''#!/usr/bin/env python3
import json,os,re,signal,sys,time
from pathlib import Path
config = Path(sys.argv[-1]).read_text()
fds = [int(n) for n in os.environ.get('MISTER_MEDIA_FDS', '').split(',') if n]
paths = re.findall(r'/proc/self/fd/([0-9]+)', config)
stop = False
def request(*_):
    global stop
    stop = True
signal.signal(signal.SIGUSR1, request)
Path(os.environ['READY']).write_text(json.dumps({'pid':os.getpid(),'config':config,'fds':fds}))
mode = os.environ.get('MODE', 'success')
if mode == 'wait':
    while not stop: time.sleep(.01)
if fds:
    os.pwrite(fds[0], b'persist', 0)
    os.fsync(fds[0])
for n in set(map(int, paths)): os.close(n)
if mode != 'missing':
    os.write(int(os.environ['MISTER_SHUTDOWN_FD']), b'WRONG!!!' if mode == 'wrong' else b'MEDIAOK\\n' if fds else b'RETIRED\\n')
sys.exit(1 if mode == 'nonzero' else 0)
'''


class ComputerMediaTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.runtime = self.root / 'runtime'
        self.runtime.mkdir()
        (self.runtime / 'machine-profile.py').write_bytes((ROOT / 'software/integrations/dosbox/machine-profile.py').read_bytes())
        (self.runtime / 'launch').write_text(CHILD)
        (self.runtime / 'launch').chmod(0o700)
        (self.root / 'profiles').mkdir()
        (self.root / 'games').mkdir()
        self.image = self.root / 'games/disk.img'
        self.image.write_bytes(bytes(1024))
        (self.root / 'selection').write_text('default\n')
        self.profile(True)
        self.command = [sys.executable, str(ROOT / 'software/runtime/mister/computer_launch.py'),
                        '--runtime', str(self.runtime), '--profiles', str(self.root / 'profiles'),
                        '--selection', str(self.root / 'selection'), '--games', str(self.root / 'games'),
                        '--state', str(self.root / 'state')]
        self.env = {**os.environ, 'READY': str(self.root / 'ready')}

    def profile(self, writable):
        (self.root / 'profiles/default.json').write_text(json.dumps({'version': 1, 'drives':
            {'A': {'type': 'floppy', 'path': 'disk.img', 'writable': writable}}}))

    def run_child(self, mode='success'):
        return subprocess.run(self.command, env={**self.env, 'MODE': mode}, capture_output=True, timeout=5)

    def dirty(self):
        return list((self.root / 'media-leases').glob('*.dirty'))

    def wait_ready(self):
        deadline = time.monotonic() + 3
        while not (self.root / 'ready').exists():
            if time.monotonic() > deadline:
                self.fail('child readiness timeout')
            time.sleep(.01)
        return json.loads((self.root / 'ready').read_text())

    def test_clean_write_persists_and_only_then_forwards_receipt(self):
        reader, writer = os.pipe()
        try:
            result = subprocess.run(self.command, env={**self.env, 'MISTER_SHUTDOWN_FD': str(writer)},
                                    pass_fds=(writer,), capture_output=True, timeout=5)
            os.close(writer)
            writer = None
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(os.read(reader, 9), b'RETIRED\n')
            self.assertEqual(os.read(reader, 1), b'')
            self.assertEqual(self.image.read_bytes()[:7], b'persist')
            self.assertFalse(self.dirty())
            config = json.loads((self.root / 'ready').read_text())['config']
            self.assertIn('/proc/self/fd/', config)
            self.assertNotIn(str(self.image), config)
            self.assertEqual(self.run_child().returncode, 0)
        finally:
            os.close(reader)
            if writer is not None: os.close(writer)

    def test_bad_missing_and_failed_receipts_keep_recovery_gate(self):
        for mode in ('wrong', 'missing', 'nonzero'):
            with self.subTest(mode=mode):
                # Each failed image has its own durable gate. No automatic clearing.
                if self.dirty():
                    self.image = self.root / ('games/' + mode + '.img')
                    self.image.write_bytes(bytes(1024))
                    (self.root / 'profiles/default.json').write_text(json.dumps({'version': 1, 'drives':
                        {'A': {'type': 'floppy', 'path': self.image.name, 'writable': True}}}))
                count = len(self.dirty())
                self.assertNotEqual(self.run_child(mode).returncode, 0)
                self.assertEqual(len(self.dirty()), count + 1)
                self.assertNotEqual(self.run_child().returncode, 0)

    def test_readonly_default_uses_shared_ownership_and_no_writes(self):
        self.profile(False)
        result = self.run_child()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.image.read_bytes(), bytes(1024))
        self.assertFalse(self.dirty())
        self.assertIn('-t floppy -ro', json.loads((self.root / 'ready').read_text())['config'])

    def test_signal_waits_for_child_readiness_and_preserves_clean_retirement(self):
        child = subprocess.Popen(self.command, env={**self.env, 'MODE': 'wait'}, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            self.wait_ready()
            self.assertEqual(len(self.dirty()), 1)
            child.send_signal(signal.SIGUSR1)
            _, error = child.communicate(timeout=5)
            self.assertEqual(child.returncode, 0, error)
            self.assertFalse(self.dirty())
        finally:
            if child.poll() is None:
                child.kill()
                child.wait()

    def test_launcher_crash_kills_writer_and_retains_gate_across_readonly_restart(self):
        child = subprocess.Popen(self.command, env={**self.env, 'MODE': 'wait'}, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            pid = self.wait_ready()['pid']
            child.kill()
            child.communicate(timeout=3)
            deadline = time.monotonic() + 2
            while Path('/proc/%d/status' % pid).exists():
                if '\nState:\tZ' in Path('/proc/%d/status' % pid).read_text(): break
                if time.monotonic() > deadline: self.fail('writer survived launcher crash')
                time.sleep(.01)
            self.assertEqual(len(self.dirty()), 1)
            self.profile(False)
            self.assertNotEqual(self.run_child().returncode, 0)
            self.assertEqual(len(self.dirty()), 1)
        finally:
            if child.poll() is None:
                child.kill()
                child.communicate()


if __name__ == '__main__':
    unittest.main()
