from contextlib import contextmanager
from pathlib import Path
from unittest.mock import patch
import os
import runpy
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
SCRIPT = ROOT / 'navigate.py'


class NavigationTest(unittest.TestCase):
    @contextmanager
    def fixture(self, timeout_status=0, owned=True):
        real_run = subprocess.run
        with tempfile.TemporaryDirectory(dir=ROOT) as directory:
            base = Path(directory)
            (base / 'output/de10/screamer2-10fps').mkdir(parents=True)
            binaries = base / 'bin'
            binaries.mkdir()
            scripts = {
                'cat': 'case "$1" in */dosbox.pid) echo 123;; /proc/sys/kernel/random/boot_id|*/programmed.boot-id) echo fixed-boot;; */paused-pair.raw) echo pixels;; *) exit 3;; esac\n',
                'sleep': 'exit 0\n',
                'sudo': 'test "$1" = -n || exit 2\nshift\ncase "$1" in readlink) echo "$EXE_PATH";; kill) printf "%s\\n" "$*" >> "$EVENT_LOG";; timeout) printf "%s\\n" "$*" >> "$EVENT_LOG"; exit "$TIMEOUT_STATUS";; *) exit 4;; esac\n',
            }
            for name, content in scripts.items():
                path = binaries / name
                path.write_text('#!/usr/bin/env bash\n' + content)
                path.chmod(0o700)
            events = base / 'events'
            environment = dict(os.environ, PATH=str(binaries) + os.pathsep + os.environ['PATH'], EVENT_LOG=str(events), TIMEOUT_STATUS=str(timeout_status), EXE_PATH='/dev/shm/screamer2-50mhz/dosbox-x-o3' if owned else '/other-process')
            def run(command, **kwargs):
                if command[0] == 'ssh':
                    return real_run(['bash', '-s'], env=environment, **kwargs)
                if command[:2] == ['python3', 'scripts/rgb565_to_png.py']:
                    return subprocess.CompletedProcess(command, 0)
                raise AssertionError(command)
            previous = Path.cwd()
            try:
                os.chdir(base)
                with patch('subprocess.run', run):
                    yield events
            finally:
                os.chdir(previous)

    def invoke(self, *arguments):
        with patch.object(sys, 'argv', [str(SCRIPT), 'test', *arguments]):
            runpy.run_path(str(SCRIPT), run_name='__main__')

    def test_success_stops_before_readout(self):
        with self.fixture() as events:
            self.invoke()
            log = events.read_text().splitlines()
            self.assertEqual(log[0], 'kill -CONT 123')
            self.assertEqual(log[-1], 'kill -STOP 123')
            self.assertEqual(log.count('kill -STOP 123'), 1)

    def test_timeout_still_stops_owned_emulator(self):
        with self.fixture(timeout_status=124) as events:
            with self.assertRaises(subprocess.CalledProcessError) as result:
                self.invoke()
            self.assertEqual(result.exception.returncode, 124)
            self.assertEqual(events.read_text().splitlines()[-1], 'kill -STOP 123')

    def test_custom_timeout(self):
        with self.fixture() as events:
            self.invoke('--timeout', '90')
            self.assertIn('timeout -s INT -k 5 90 gdb', events.read_text())

    def test_timeout_validation(self):
        for timeout in ('0', '121', 'wrong'):
            with self.subTest(timeout=timeout), self.fixture() as events:
                with self.assertRaises(SystemExit) as result:
                    self.invoke('--timeout', timeout)
                self.assertEqual(result.exception.code, 2)
                self.assertFalse(events.exists())

    def test_label_and_key_validation(self):
        for arguments in (['../invalid'], ['test', 'invalid-key']):
            with self.subTest(arguments=arguments), self.fixture() as events:
                with patch.object(sys, 'argv', [str(SCRIPT), *arguments]):
                    with self.assertRaises(SystemExit) as result:
                        runpy.run_path(str(SCRIPT), run_name='__main__')
                self.assertEqual(result.exception.code, 2)
                self.assertFalse(events.exists())

    def test_foreign_process_is_not_signalled(self):
        with self.fixture(owned=False) as events:
            with self.assertRaises(subprocess.CalledProcessError):
                self.invoke()
            self.assertFalse(events.exists())


if __name__ == '__main__':
    unittest.main()
