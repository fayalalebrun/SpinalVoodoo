import importlib.util
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('core_controller', ROOT / 'software/runtime/mister/core_gate_controller.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class CoreControllerTest(unittest.TestCase):
    def test_real_cooperative_process_retirement(self):
        with tempfile.TemporaryDirectory() as directory:
            ready = Path(directory) / 'ready'
            command = [sys.executable, '-c',
                'import os,signal,sys,time; from pathlib import Path; '
                'fd=int(os.environ["MISTER_SHUTDOWN_FD"]); '
                'signal.signal(signal.SIGUSR1,lambda *_:(os.write(fd,b"RETIRED\\n"),os.close(fd),sys.exit(0))); '
                'Path(sys.argv[1]).touch(); time.sleep(30)', str(ready)]
            session = module.Session(command)
            try:
                deadline = time.monotonic() + 3
                while not ready.exists() and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertTrue(ready.exists())
                self.assertTrue(session.stop())
                self.assertTrue(session.stop())
                self.assertTrue(session.retired)
                self.assertEqual(session.child.poll(), 0)
            finally:
                if session.child.poll() is None:
                    session.child.kill()
                    session.child.wait()
                if session.reader is not None:
                    os.close(session.reader)

    def test_exit_without_receipt_nonzero_and_malformed_receipts_deny(self):
        for status, receipt in ((0, b''), (1, b'RETIRED\n'), (0, b'RETIRED\nX'), (-9, b'')):
            with self.subTest(status=status, receipt=receipt):
                session = module.Session.__new__(module.Session)
                session.retired = False
                session.child = Mock()
                session.child.poll.return_value = status
                session.child.wait.return_value = status
                reader, writer = os.pipe()
                session.reader = reader
                os.write(writer, receipt)
                os.close(writer)
                try:
                    self.assertFalse(session.stop())
                    session.child.kill.assert_not_called()
                finally:
                    os.close(reader)

    def test_immediate_stop_waits_for_handler_instead_of_killing_launcher(self):
        command = [sys.executable, '-c',
            'import os,signal,sys,time; time.sleep(0.2); '
            'fd=int(os.environ["MISTER_SHUTDOWN_FD"]); '
            'signal.signal(signal.SIGUSR1,lambda *_:(os.write(fd,b"RETIRED\\n"),os.close(fd),sys.exit(0))); time.sleep(10)']
        session = module.Session(command)
        try:
            self.assertTrue(session.stop())
            self.assertEqual(session.child.poll(), 0)
        finally:
            if session.child.poll() is None:
                session.child.kill()
                session.child.wait()
            if session.reader is not None:
                os.close(session.reader)

    def test_parent_death_kills_guest(self):
        with tempfile.TemporaryDirectory() as directory:
            record = Path(directory) / 'pid'
            parent = subprocess.Popen([sys.executable, '-c',
                'import sys,time; from pathlib import Path; sys.path.insert(0,sys.argv[1]); '
                'from core_gate_controller import Session; s=Session([sys.executable,"-c","import time; time.sleep(30)"]); '
                'Path(sys.argv[2]).write_text(str(s.child.pid)); time.sleep(30)',
                str(ROOT / 'software/runtime/mister'), str(record)])
            child = None
            try:
                deadline = time.monotonic() + 3
                while not record.exists() and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertTrue(record.exists())
                child = int(record.read_text())
                parent.kill()
                parent.wait(timeout=3)
                deadline = time.monotonic() + 3
                while time.monotonic() < deadline:
                    process = Path('/proc') / str(child)
                    if not process.exists() or process.joinpath('stat').read_text().split(') ', 1)[1].startswith('Z'):
                        break
                    time.sleep(0.01)
                else:
                    self.fail('Guest survived controller death')
            finally:
                if parent.poll() is None:
                    parent.kill()
                    parent.wait()
                if child is not None:
                    try:
                        os.kill(child, signal.SIGKILL)
                    except ProcessLookupError:
                        pass

    def test_timeout_does_not_kill_or_authorize(self):
        session = module.Session.__new__(module.Session)
        session.retired = False
        session.child = Mock()
        session.child.pid = 123
        session.child.poll.return_value = None
        session.child.wait.side_effect = subprocess.TimeoutExpired('guest', 4)
        with patch.object(module.Path, 'read_text', return_value='SigCgt:\t00000200\n'):
            self.assertFalse(session.stop())
        session.child.send_signal.assert_called_once_with(signal.SIGUSR1)
        session.child.kill.assert_not_called()

    def test_retirement_receipt_without_pipe_eof_is_denied(self):
        session = module.Session.__new__(module.Session)
        session.retired = False
        session.child = Mock()
        session.child.poll.return_value = 0
        session.child.wait.return_value = 0
        reader, writer = os.pipe()
        session.reader = reader
        os.set_blocking(reader, False)
        os.write(writer, b'RETIRED\n')
        try:
            self.assertFalse(session.stop())
        finally:
            os.close(reader)
            os.close(writer)

    def test_fragmented_request_ack_and_invalid_request(self):
        for request, allowed, expected in ((b'STOP\n', True, b'OK\n'), (b'STOP\n', False, b'NO\n'), (b'NOPE\n', True, b'NO\n')):
            with self.subTest(request=request, allowed=allowed):
                client, server = socket.socketpair()
                session = Mock()
                session.stop.return_value = allowed
                with client, server:
                    thread = threading.Thread(target=module.serve_connection, args=(server, session))
                    thread.start()
                    for byte in request:
                        client.sendall(bytes([byte]))
                    self.assertEqual(client.recv(3), expected)
                    thread.join(3)
                    self.assertFalse(thread.is_alive())
                if request == b'STOP\n':
                    session.stop.assert_called_once()
                else:
                    session.stop.assert_not_called()


if __name__ == '__main__':
    unittest.main()
