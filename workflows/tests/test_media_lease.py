import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'software/runtime/mister/media_lease.py'
SPEC = importlib.util.spec_from_file_location('media_lease', SOURCE)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ImageLeaseTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.games = self.root / 'games'
        self.state = self.root / 'state'
        self.games.mkdir()
        self.state.mkdir(mode=0o700)
        self.image = self.games / 'disk.img'
        self.image.write_bytes(bytes(1024))

    def lease(self, name='disk.img'):
        return MODULE.ImageLease(self.games, name, self.state)

    def test_exclusive_lease_sync_and_reopen(self):
        with self.lease() as lease:
            with self.assertRaises(BlockingIOError):
                self.lease()
            os.pwrite(lease.fd, b'checked write', 0)
            with patch.object(MODULE.os, 'fsync', wraps=os.fsync) as sync:
                lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
                self.assertEqual(sync.call_args_list[0].args, (lease.fd,))
                self.assertEqual(sync.call_args_list[1].args, (lease.state_fd,))
            with self.assertRaises(RuntimeError):
                lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
        with self.lease():
            self.assertEqual(self.image.read_bytes()[:13], b'checked write')

    def test_incomplete_lease_and_inode_replacement_require_recovery(self):
        with self.lease():
            pass
        self.image.unlink()
        self.image.write_bytes(bytes(1024))
        with self.assertRaises(FileExistsError):
            self.lease()

    def test_root_alias_cannot_bypass_unfinished_marker(self):
        alias = self.root / 'alias'
        alias.symlink_to(self.games, target_is_directory=True)
        with self.lease():
            pass
        with self.assertRaises(FileExistsError):
            MODULE.ImageLease(alias, 'disk.img', self.state)

    def test_failed_receipts_and_sync_keep_recovery_marker(self):
        with self.lease() as lease:
            for status, receipt, eof in [(1, b'MEDIAOK\n', True), (False, b'MEDIAOK\n', True),
                                          (0, b'', True), (0, b'RETIRED\nextra', True),
                                          (0, b'MEDIAOK\n', False)]:
                with self.assertRaises(ValueError):
                    lease.complete(exit_status=status, receipt=receipt, eof=eof)
            with patch.object(MODULE.os, 'fsync', side_effect=OSError('disk failure')):
                with self.assertRaises(OSError):
                    lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
        with self.assertRaises(FileExistsError):
            self.lease()

    def test_marker_creation_sync_failure_keeps_recovery_gate(self):
        with patch.object(MODULE.os, 'fsync', side_effect=OSError('marker failure')):
            with self.assertRaises(OSError):
                self.lease()
        with self.assertRaises(FileExistsError):
            self.lease()

    def test_shared_image_lock_and_marker_symlink_refused(self):
        import fcntl
        descriptor = os.open(self.image, os.O_RDONLY)
        try:
            fcntl.flock(descriptor, fcntl.LOCK_SH | fcntl.LOCK_NB)
            with self.assertRaises(BlockingIOError):
                self.lease()
        finally:
            os.close(descriptor)
        with self.lease() as lease:
            marker = self.state / lease.marker
            lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
        marker.symlink_to(self.image)
        original = self.image.read_bytes()
        with self.assertRaises(FileExistsError):
            self.lease()
        self.assertEqual(self.image.read_bytes(), original)

    def test_directory_sync_failure_restores_marker(self):
        with self.lease() as lease:
            with patch.object(MODULE.os, 'fsync', side_effect=[None, OSError('directory failure')]):
                with self.assertRaises(OSError):
                    lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
        with self.assertRaises(FileExistsError):
            self.lease()

    def test_paths_types_links_and_permissions_fail_closed(self):
        (self.games / 'link.img').symlink_to(self.image)
        (self.games / 'alias').symlink_to(self.games, target_is_directory=True)
        (self.games / 'directory.img').mkdir()
        (self.games / 'short.img').write_bytes(b'x')
        os.mkfifo(self.games / 'fifo.img')
        for name in ('../disk.img', str(self.image), 'disk.iso', 'link.img',
                     'alias/disk.img', 'directory.img', 'short.img', 'fifo.img'):
            with self.subTest(name=name), self.assertRaises((ValueError, OSError)):
                self.lease(name)
        os.link(self.image, self.games / 'hardlink.img')
        with self.assertRaises(ValueError):
            self.lease()
        (self.games / 'hardlink.img').unlink()
        self.state.chmod(0o777)
        with self.assertRaises(ValueError):
            self.lease()

    def test_readable_metadata_allowed_but_writable_lock_rejected(self):
        self.state.chmod(0o755)
        with self.lease() as lease:
            lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
        lock = next(self.state.glob('*.lock'))
        lock.chmod(0o755)
        with self.lease() as lease:
            lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
        lock.chmod(0o666)
        with self.assertRaises(ValueError):
            self.lease()

    def test_process_crash_leaves_gate_after_kernel_unlock(self):
        program = '''import importlib.util, os, sys
spec = importlib.util.spec_from_file_location('lease', sys.argv[1])
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
lease = module.ImageLease(sys.argv[2], 'disk.img', sys.argv[3])
os._exit(23)
'''
        result = subprocess.run([sys.executable, '-B', '-c', program, str(SOURCE),
                                 str(self.games), str(self.state)], timeout=5)
        self.assertEqual(result.returncode, 23)
        with self.assertRaises(FileExistsError):
            self.lease()


if __name__ == '__main__':
    unittest.main()
