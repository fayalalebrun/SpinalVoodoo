import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('computer_service', ROOT / 'software/runtime/mister/computer_service.py')
service = importlib.util.module_from_spec(spec)
spec.loader.exec_module(service)


class ComputerServiceTest(unittest.TestCase):
    def test_native_configuration_editor(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runtime = root / 'runtime'
            runtime.mkdir()
            for name, source in [('computer_configure.py', 'software/runtime/mister'), ('machine-profile.py', 'software/integrations/dosbox')]:
                shutil.copyfile(ROOT / source / name, runtime / name)
            config = root / 'config'
            (config / 'profiles').mkdir(parents=True)
            (config / 'games/shared').mkdir(parents=True)
            (config / 'games/disc.iso').write_bytes(b'iso')
            (config / 'games/boot.img').write_bytes(b'floppy')
            original = '{"version":1,"cpu":"normal","audio":"off","drives":{}}'
            (config / 'profiles/default.json').write_text(original)
            (config / 'selected').write_text('default\n')
            command = [sys.executable, str(runtime / 'computer_configure.py'), '--config', str(config)]
            actions = '2\ndynamic\n3\n20000\n4\n32\n5\noff\n6\nC\ndirectory\nshared\n6\nD\niso\ndisc.iso\n6\nA\nfloppy\nboot.img\nro\n6\nA\neject\n7\ncustom\n0\n'
            result = subprocess.run(command, input=actions, text=True, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((config / 'selected').read_text(), 'custom\n')
            profile = json.loads((config / 'profiles/custom.json').read_text())
            self.assertEqual((profile['cpu'], profile['cycles'], profile['memory_mb']), ('dynamic', 20000, 32))
            self.assertEqual(set(profile['drives']), {'C', 'D'})
            self.assertEqual((config / 'profiles/default.json').read_text(), original)
            before = (config / 'profiles/custom.json').read_bytes()
            result = subprocess.run(command, input='2\ninvalid\n6\nC\ndirectory\n../escape\n7\n../invalid\n4\n64\n0\n', text=True, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout.count('Rejected:'), 3)
            self.assertEqual(json.loads((config / 'profiles/custom.json').read_text())['memory_mb'], 64)
            self.assertEqual(next(config.glob('custom-*.json')).read_bytes(), before)
            before = (config / 'profiles/custom.json').read_bytes()
            self.assertEqual(subprocess.run(command, input='2\nnormal\nq\n', text=True, capture_output=True, timeout=5).returncode, 0)
            self.assertEqual((config / 'profiles/custom.json').read_bytes(), before)
            self.assertEqual(subprocess.run(command, input='1\ndefault\n0\n', text=True, capture_output=True, timeout=5).returncode, 0)
            self.assertEqual((config / 'selected').read_text(), 'default\n')
            (config / 'profiles/link.json').symlink_to(config / 'profiles/custom.json')
            (config / 'selected').write_text('link\n')
            self.assertNotEqual(subprocess.run(command, input='0\n', text=True, capture_output=True, timeout=5).returncode, 0)
            self.assertEqual((config / 'profiles/custom.json').read_bytes(), before)
            (config / 'profiles/large.json').write_bytes(b' ' * 65537)
            (config / 'selected').write_text('large\n')
            result = subprocess.run(command, input='q\n', text=True, capture_output=True, timeout=5)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('64 KiB', result.stderr)
            (config / 'selected').write_text('default\n')
            result = subprocess.run(command, input='7\nlarge\n0\nq\n', text=True, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('refusing replacement', result.stdout)
            self.assertEqual((config / 'profiles/large.json').stat().st_size, 65537)
            self.assertEqual((config / 'selected').read_text(), 'default\n')

    def test_configuration_atomic_write_failure_preserves_previous_file(self):
        spec = importlib.util.spec_from_file_location('computer_configure', ROOT / 'software/runtime/mister/computer_configure.py')
        editor = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(editor)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'profile.json'
            path.write_text('preserved')
            with patch.object(editor.os, 'replace', side_effect=OSError('write refused')):
                with self.assertRaises(OSError):
                    editor.atomic_write(path, 'new')
            self.assertEqual(path.read_text(), 'preserved')
            self.assertEqual(list(Path(directory).iterdir()), [path])

    def test_pidfd_syscall_fallback(self):
        child = subprocess.Popen([sys.executable, '-c', 'import time;time.sleep(10)'])
        try:
            fd = service.pidfd_syscall(434, child.pid, 0)
            try:
                service.pidfd_syscall(424, fd, service.signal.SIGTERM, 0, 0)
                self.assertEqual(child.wait(timeout=2), -service.signal.SIGTERM)
            finally:
                os.close(fd)
            with self.assertRaises(OSError):
                service.pidfd_syscall(434, -1, 0)
            with patch.object(service.platform, 'machine', return_value='unsupported'):
                with self.assertRaisesRegex(RuntimeError, 'Unsupported pidfd'):
                    service.pidfd_syscall(434, os.getpid(), 0)
        finally:
            if child.poll() is None:
                child.kill()
            child.wait(timeout=2)

    def test_service_stop_identity_and_timeout(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            child_code = ('import fcntl,json,os,signal,sys,time\nfrom pathlib import Path\n'
                'p=Path(sys.argv[1]);lock=(p/"service.lock").open("a");fcntl.flock(lock,fcntl.LOCK_EX)\n'
                'birth=Path("/proc/%d/stat"%os.getpid()).read_text().split(") ",1)[1].split()[19]\n'
                '(p/"service.pid").write_text(json.dumps([os.getpid(),birth]))\n'
                'signal.signal(signal.SIGTERM,signal.SIG_IGN) if len(sys.argv)>2 else None\n'
                'print("READY",flush=True);time.sleep(10)\n')
            with patch.object(service, 'renderer_clients', return_value=True):
                with self.assertRaisesRegex(RuntimeError, 'Unmanaged renderer client'):
                    service.stop_service(state)
            with patch.object(service, 'renderer_clients', return_value=False):
                service.stop_service(state)
            for ignore in (False, True):
                child = subprocess.Popen([sys.executable, '-c', child_code, str(state)] + (['ignore'] if ignore else []), stdout=subprocess.PIPE, text=True)
                try:
                    self.assertEqual(child.stdout.readline().strip(), 'READY')
                    identity = (state / 'service.pid').read_text()
                    (state / 'service.pid').write_text(json.dumps([child.pid, 'wrong']))
                    with self.assertRaisesRegex(RuntimeError, 'identity changed'):
                        service.stop_service(state, timeout=.05)
                    self.assertIsNone(child.poll())
                    (state / 'service.pid').write_text(identity)
                    with patch.object(service, 'renderer_clients', return_value=False), \
                            patch.object(service, 'os', SimpleNamespace(close=os.close, strerror=os.strerror)), \
                            patch.object(service, 'signal', SimpleNamespace(SIGTERM=15)):
                        if ignore:
                            with self.assertRaisesRegex(RuntimeError, 'no forced kill'):
                                service.stop_service(state, timeout=.05)
                            self.assertIsNone(child.poll())
                        else:
                            service.stop_service(state, timeout=2)
                            self.assertEqual(child.wait(timeout=2), -service.signal.SIGTERM)
                finally:
                    if child.poll() is None:
                        child.kill()
                    child.wait(timeout=2)
                    child.stdout.close()

    def test_installer_preserves_existing_hook_and_checks_identities(self):
        spec = importlib.util.spec_from_file_location('install_computer', ROOT / 'software/runtime/mister/install_computer.py')
        installer = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(installer)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runtime = root / 'runtime'
            runtime.mkdir()
            image = root / 'core.rbf'
            image.write_bytes(b'core')
            digest = service.hashlib.sha256(b'core').hexdigest()
            (runtime / 'main_control_manifest.json').write_text(json.dumps({'sha256': digest, 'core_sha256': digest, 'menu_sha256': digest}))
            startup = root / 'user-startup.sh'
            original = Path.read_bytes
            def read_bytes(path):
                if str(path) in ('/media/fat/MiSTer', '/media/fat/menu.rbf'):
                    return b'core'
                return original(path)
            with patch.object(installer.subprocess, 'run'), patch.object(installer.os, 'geteuid', return_value=0), patch.object(Path, 'read_bytes', read_bytes):
                with self.assertRaisesRegex(ValueError, 'script paths'):
                    installer.install(runtime, root / 'config', image, root / 'bad scripts', startup)
                self.assertFalse((root / 'config').exists())
                startup.write_text('preserved')
                with self.assertRaisesRegex(ValueError, 'never overwritten'):
                    installer.install(runtime, root / 'config', image, root / 'scripts', startup)
                self.assertEqual(startup.read_text(), 'preserved')
                self.assertFalse((root / 'config').exists())
                target = root / 'fresh-startup.sh'
                installer.install(runtime, root / 'config', image, root / 'scripts', target)
                self.assertIn('nohup', target.read_text())
                self.assertIn('restart)', target.read_text())
                subprocess.run(['/bin/sh', '-n', str(target)], check=True)
                self.assertEqual((root / 'config/selected').read_text(), 'default\n')
                self.assertTrue((root / 'scripts/dynamic32.sh').exists())
                self.assertIn('computer_configure.py', (root / 'scripts/configure.sh').read_text())
                subprocess.run(['/bin/sh', '-n', str(root / 'scripts/configure.sh')], check=True)
                image.write_bytes(b'changed')
                with self.assertRaisesRegex(ValueError, 'identity'):
                    installer.install(runtime, root / 'other-config', image, root / 'other-scripts', root / 'other-hook')
                self.assertFalse((root / 'other-config').exists())

    def test_unknown_recovery_core_and_changed_image_denied(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / 'core.rbf'
            image.write_bytes(b'core')
            manifest = {'core_sha256': service.hashlib.sha256(b'core').hexdigest()}
            self.assertEqual(service.recovery_image('SpinalVoodoo', image, manifest), image)
            with self.assertRaisesRegex(RuntimeError, 'Unknown active core'):
                service.recovery_image('other', image, manifest)
            image.write_bytes(b'changed')
            with self.assertRaisesRegex(RuntimeError, 'identity changed'):
                service.recovery_image('SpinalVoodoo', image, manifest)

    def test_selected_profile_launch_and_validation_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runtime = root / 'runtime'
            runtime.mkdir()
            shutil.copyfile(ROOT / 'software/integrations/dosbox/machine-profile.py', runtime / 'machine-profile.py')
            output = root / 'result'
            launcher = runtime / 'launch'
            launcher.write_text('#!/usr/bin/env python3\nimport json,os,sys\nfrom pathlib import Path\n'
                'Path(os.environ["TEST_OUTPUT"]).write_text(json.dumps({"args":sys.argv[1:],"config":Path(sys.argv[-1]).read_text(),"receipt_fd":os.environ.get("MISTER_SHUTDOWN_FD")}))\n')
            launcher.chmod(0o700)
            profiles = root / 'profiles'
            profiles.mkdir()
            selected = root / 'selected'
            selected.write_text('default\n')
            (profiles / 'default.json').write_text('{"version":1,"cpu":"dynamic","memory_mb":32}')
            games = root / 'games'
            games.mkdir()
            command = [sys.executable, str(ROOT / 'software/runtime/mister/computer_launch.py'),
                '--runtime', str(runtime), '--profiles', str(profiles), '--selection', str(selected),
                '--games', str(games), '--state', str(root / 'state')]
            env = {**os.environ, 'TEST_OUTPUT': str(output), 'MISTER_SHUTDOWN_FD': '42'}
            result = subprocess.run(command, env=env, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)
            data = json.loads(output.read_text())
            self.assertIn('core=dynamic', data['config'])
            self.assertEqual(data['receipt_fd'], '42')
            self.assertEqual(data['args'][0], '-conf')
            selected.write_text('../default')
            self.assertNotEqual(subprocess.run(command, env=env, capture_output=True, timeout=5).returncode, 0)
            selected.write_text('default')
            (profiles / 'default.json').write_text('{"version":1,"cpu":"invalid"}')
            self.assertNotEqual(subprocess.run(command, env=env, capture_output=True, timeout=5).returncode, 0)
            (profiles / 'default.json').unlink()
            external = root / 'external.json'
            external.write_text('{"version":1}')
            (profiles / 'default.json').symlink_to(external)
            self.assertNotEqual(subprocess.run(command, env=env, capture_output=True, timeout=5).returncode, 0)


if __name__ == '__main__':
    unittest.main()
