import os
from pathlib import Path
import shlex
import socket
import subprocess
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CoreGateTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory()
        directory = Path(cls.build.name)
        source = directory / 'test.c'
        source.write_text('#include <stdlib.h>\n#define main gate_main\n#include "' + str(ROOT / 'software/runtime/mister/core_gate.c') + '"\n#undef main\n'
            'int main(int argc, char **argv) {\n'
            'if (argc != 3) { return 2; }\ncontrol = argv[1];\n'
            'struct fuse_file_info info = {.flags = atoi(argv[2])};\n'
            'int result = core_open("/core.rbf", &info);\n'
            'printf("%d %d\\n", result, info.direct_io); return 0; }\n')
        cls.binary = directory / 'test'
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'fuse3'], text=True))
        subprocess.run(['cc', '-D_GNU_SOURCE', '-Wall', '-Wextra', '-Werror', str(source), *flags, '-o', str(cls.binary)], check=True)
        cls.program = directory / 'core-gate'
        subprocess.run(['cc', '-D_GNU_SOURCE', '-Wall', '-Wextra', '-Werror', str(ROOT / 'software/runtime/mister/core_gate.c'), *flags, '-o', str(cls.program)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def invoke(self, path, flags=0):
        return subprocess.check_output([str(self.binary), str(path), str(flags)], text=True, timeout=8).strip()

    def test_missing_and_non_socket_denied(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'control'
            self.assertEqual(self.invoke(path), '-13 0')
            path.touch(mode=0o600)
            self.assertEqual(self.invoke(path), '-13 0')

    def test_acknowledgment_and_error_paths(self):
        for reply, expected in ((b'OK\n', '0 1'), (b'NO\n', '-5 0'), (b'OK\nX', '-5 0'), (b'', '-5 0')):
            with self.subTest(reply=reply), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'control'
                with socket.socket(socket.AF_UNIX) as server:
                    server.bind(str(path))
                    os.chmod(path, 0o600)
                    server.listen(1)
                    requests = []
                    def respond():
                        connection, _ = server.accept()
                        with connection:
                            requests.append(connection.recv(5))
                            for byte in reply:
                                connection.sendall(bytes([byte]))
                    thread = threading.Thread(target=respond)
                    thread.start()
                    self.assertEqual(self.invoke(path), expected)
                    thread.join(5)
                    self.assertFalse(thread.is_alive())
                    self.assertEqual(requests, [b'STOP\n'])

    def test_socket_symlink_public_parent_and_disconnected_socket_denied(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'control'
            with socket.socket(socket.AF_UNIX) as server:
                server.bind(str(path))
                os.chmod(path, 0o600)
                self.assertEqual(self.invoke(path), '-5 0')
                link = Path(directory) / 'link'
                link.symlink_to(path)
                self.assertEqual(self.invoke(link), '-13 0')
                os.chmod(directory, 0o755)
                self.assertEqual(self.invoke(path), '-13 0')
        self.assertEqual(self.invoke('/' + 'x' * 110), '-36 0')

    def test_insecure_socket_and_writes_denied(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'control'
            with socket.socket(socket.AF_UNIX) as server:
                server.bind(str(path))
                os.chmod(path, 0o666)
                self.assertEqual(self.invoke(path), '-13 0')
                self.assertEqual(self.invoke(path, os.O_WRONLY), '-30 0')
                self.assertEqual(self.invoke(path, os.O_TRUNC), '-30 0')

    def test_invalid_cli_and_symlink_backing(self):
        result = subprocess.run([str(self.program)], capture_output=True)
        self.assertEqual(result.returncode, 2)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'backing').write_bytes(b'core')
            (root / 'link').symlink_to(root / 'backing')
            result = subprocess.run([str(self.program), str(root / 'link'), str(root / 'socket'), str(root)], capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn(b'not a symlink', result.stderr)


if __name__ == '__main__':
    unittest.main()
