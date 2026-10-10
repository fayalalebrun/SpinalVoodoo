import importlib.util
from pathlib import Path
import os
import signal
import struct
import subprocess
import sys
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('stock_supervisor', ROOT / 'software/runtime/mister/stock_main_supervisor.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class StockSupervisorTest(unittest.TestCase):
    def test_permission_metadata_and_invalid_events(self):
        packet = module.HEADER.pack(24, 3, 0, 24, module.OPEN_PERMISSION, 12, 99)
        self.assertEqual(list(module.events(packet + packet)), [(module.OPEN_PERMISSION, 12, 99)] * 2)
        for data in (packet[:10], module.HEADER.pack(23, 3, 0, 24, 0, 0, 0), module.HEADER.pack(24, 2, 0, 24, 0, 0, 0)):
            with self.assertRaises(ValueError):
                list(module.events(data))
        with self.assertRaises(RuntimeError):
            list(module.events(module.HEADER.pack(24, 3, 0, 24, module.OVERFLOW, -1, 0)))

    def test_child_inherits_gate_and_no_second_session(self):
        machine = module.Machine(['/private/launch'], 42)
        child = Mock()
        child.poll.return_value = None
        child.wait.return_value = 0
        with patch.object(module.subprocess, 'Popen', return_value=child) as spawn:
            machine.start()
            spawn.assert_called_once_with(['/private/launch'], pass_fds=(42,), start_new_session=True)
            with self.assertRaises(RuntimeError):
                machine.start()
            self.assertTrue(machine.stop())
            child.send_signal.assert_called_once_with(signal.SIGUSR1)
            self.assertIsNone(machine.child)

    def test_timeout_and_unclean_exit_deny_switch(self):
        for result in (1, -signal.SIGUSR1, subprocess.TimeoutExpired('machine', 5)):
            machine = module.Machine(['/private/launch'], 42)
            child = machine.child = Mock()
            child.poll.return_value = None
            if isinstance(result, Exception):
                child.wait.side_effect = result
            else:
                child.wait.return_value = result
            self.assertFalse(machine.stop())
            self.assertIs(machine.child, child)
            child.kill.assert_not_called()

    def test_cleanup_open_events_are_serviced_during_shutdown(self):
        machine = module.Machine(['/private/launch'], 42)
        child = machine.child = Mock()
        child.poll.return_value = None
        child.wait.side_effect = [subprocess.TimeoutExpired('machine', 0.05), 0]
        service = Mock()
        self.assertTrue(machine.stop(service))
        service.assert_called_once()

    def test_inherited_gate_survives_parent_descriptor_close(self):
        read_fd, write_fd = os.pipe()
        command = [sys.executable, '-c',
            'import os,sys; sys.stdout.buffer.write(os.read(int(sys.argv[1]), 1))', str(read_fd)]
        machine = module.Machine(command, read_fd)
        with patch.object(module.subprocess, 'Popen', wraps=module.subprocess.Popen) as spawn:
            machine.start()
            os.close(read_fd)
            try:
                os.write(write_fd, b'x')
                self.assertEqual(machine.child.wait(timeout=5), 0)
                self.assertTrue(machine.stop())
                self.assertEqual(spawn.call_args.kwargs['pass_fds'], (read_fd,))
            finally:
                os.close(write_fd)
                if machine.child is not None and machine.child.poll() is None:
                    machine.child.kill()
                    machine.child.wait()

    def test_unavailable_kernel_never_returns_a_gate(self):
        libc = Mock()
        libc.fanotify_init.return_value = -1
        with patch.object(module.ctypes, 'CDLL', return_value=libc), patch.object(module.ctypes, 'get_errno', return_value=38):
            with self.assertRaisesRegex(OSError, 'machine will not start'):
                module.open_gate(['/media/fat'])
            libc.fanotify_init.assert_called_once_with(7, os.O_RDONLY | os.O_CLOEXEC)
            libc.fanotify_mark.assert_not_called()

    def test_permission_reply_and_descriptor_cleanup(self):
        with patch.object(module.os, 'write', return_value=8) as write, patch.object(module.os, 'close') as close:
            module.release_permission(42, 13, False)
            write.assert_called_once_with(42, struct.pack('=iI', 13, 2))
            close.assert_called_once_with(13)
        with patch.object(module.os, 'write', side_effect=OSError('disconnected')), patch.object(module.os, 'close') as close:
            with self.assertRaises(OSError):
                module.release_permission(42, 13, True)
            close.assert_called_once_with(13)


if __name__ == '__main__':
    unittest.main()
