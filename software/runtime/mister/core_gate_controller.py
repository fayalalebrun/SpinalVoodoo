#!/usr/bin/env python3
import argparse
import ctypes
import fcntl
import os
from pathlib import Path
import signal
import socket
import subprocess
import time


def terminate_with_parent(parent):
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.prctl(1, signal.SIGKILL, 0, 0, 0) or os.getppid() != parent:
        os._exit(1)


class Session:
    def __init__(self, command):
        self.child = None
        self.reader = None
        reader, writer = os.pipe()
        os.set_blocking(reader, False)
        try:
            parent = os.getpid()
            self.child = subprocess.Popen(command, env={**os.environ, 'MISTER_SHUTDOWN_FD': str(writer)},
                                          pass_fds=(writer,), start_new_session=True,
                                          preexec_fn=lambda: terminate_with_parent(parent))
            self.reader = reader
        except BaseException:
            os.close(reader)
            raise
        finally:
            os.close(writer)
        self.retired = False

    def stop(self):
        if self.retired:
            return True
        deadline = time.monotonic() + 4
        while self.child.poll() is None:
            try:
                status_text = Path('/proc/%d/status' % self.child.pid).read_text()
                caught = int(status_text.split('SigCgt:\t', 1)[1].splitlines()[0], 16)
                if caught & (1 << (signal.SIGUSR1 - 1)):
                    self.child.send_signal(signal.SIGUSR1)
                    break
            except (OSError, ValueError, IndexError):
                pass
            if time.monotonic() >= deadline:
                return False
            time.sleep(0.02)
        try:
            status = self.child.wait(timeout=max(0.01, deadline - time.monotonic()))
        except subprocess.TimeoutExpired:
            return False
        try:
            receipt = os.read(self.reader, 9)
            complete = os.read(self.reader, 1) == b''
        except BlockingIOError:
            return False
        if status != 0 or receipt != b'RETIRED\n' or not complete:
            return False
        os.close(self.reader)
        self.reader = None
        self.retired = True
        return True


def serve_connection(connection, session):
    connection.settimeout(1)
    request = bytearray()
    try:
        while len(request) < 5:
            data = connection.recv(5 - len(request))
            if not data:
                return
            request.extend(data)
        connection.sendall(b'OK\n' if request == b'STOP\n' and session.stop() else b'NO\n')
    except OSError:
        return


def main():
    parser = argparse.ArgumentParser(description='Experimental FUSE retirement controller; not a full core-switch service')
    parser.add_argument('--state-dir', type=Path, required=True)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command or not Path(command[0]).is_absolute():
        parser.error('An absolute source-matched launcher command is required')
    if Path('/tmp/CORENAME').read_text().strip() != 'SpinalVoodoo':
        parser.error('The approved SpinalVoodoo core must already be active')
    args.state_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
    status = args.state_dir.lstat()
    if args.state_dir.is_symlink() or not args.state_dir.is_dir() or status.st_uid != os.geteuid() or status.st_mode & 0o077:
        parser.error('State directory must be private, owned and not a symlink')
    with (args.state_dir / 'controller.lock').open('a') as lock, socket.socket(socket.AF_UNIX) as server:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        endpoint = args.state_dir / 'control.sock'
        server.bind(str(endpoint))
        endpoint.chmod(0o600)
        server.listen(4)
        server.settimeout(0.2)
        stopping = False
        def request_stop(_signum, _frame):
            nonlocal stopping
            stopping = True
        signal.signal(signal.SIGTERM, request_stop)
        signal.signal(signal.SIGINT, request_stop)
        session = None
        try:
            session = Session(command)
            while True:
                if stopping and session.stop():
                    break
                try:
                    connection, _ = server.accept()
                except socket.timeout:
                    continue
                with connection:
                    credentials = connection.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, 12)
                    uid = int.from_bytes(credentials[4:8], byteorder=os.sys.byteorder)
                    if uid == os.geteuid():
                        serve_connection(connection, session)
        finally:
            if session is not None and not session.retired:
                if not session.stop():
                    while session.child.poll() is None:
                        time.sleep(0.2)
                if session.reader is not None:
                    os.close(session.reader)
            endpoint.unlink()


if __name__ == '__main__':
    main()
