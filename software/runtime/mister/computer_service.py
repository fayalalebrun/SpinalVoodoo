#!/usr/bin/env python3
import argparse
import ctypes
import platform
import fcntl
import hashlib
import json
import os
from pathlib import Path
import signal
import select
import subprocess
import sys
import time


def main_processes(binary):
    found = []
    for path in Path('/proc').glob('[0-9]*'):
        try:
            if (path / 'exe').resolve(strict=True) == binary:
                found.append(int(path.name))
        except (OSError, RuntimeError):
            pass
    return found


def renderer_clients():
    for path in Path('/proc').glob('[0-9]*'):
        try:
            if 'libmister_runtime.so' in (path / 'maps').read_text():
                return True
        except FileNotFoundError:
            pass
    return False


def recovery_image(core_name, approved, manifest):
    if core_name == 'SpinalVoodoo':
        path, expected = approved, manifest['core_sha256']
    elif core_name == 'MENU':
        path, expected = Path('/media/fat/menu.rbf'), manifest['menu_sha256']
    else:
        raise RuntimeError('Unknown active core after broker failure; no automatic recovery')
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        raise RuntimeError('Recovery image identity changed')
    return path


def pidfd_syscall(number, *arguments):
    if platform.system() != 'Linux' or platform.machine() not in ('armv7l', 'aarch64', 'x86_64'):
        raise RuntimeError('Unsupported pidfd syscall platform')
    libc = ctypes.CDLL(None, use_errno=True)
    result = libc.syscall(ctypes.c_long(number), *(ctypes.c_long(value) for value in arguments))
    if result < 0:
        error = ctypes.get_errno()
        raise OSError(error, os.strerror(error))
    return result


def stop_service(state, timeout=30):
    with (state / 'service.lock').open('a') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            if renderer_clients():
                raise RuntimeError('Unmanaged renderer client exists; refusing stopped status')
            return
        except BlockingIOError:
            pass
        pid, birth = json.loads((state / 'service.pid').read_text())
        fd = os.pidfd_open(pid) if hasattr(os, 'pidfd_open') else pidfd_syscall(434, pid, 0)
        try:
            actual = (Path('/proc') / str(pid) / 'stat').read_text().split(') ', 1)[1].split()[19]
            if actual != birth:
                raise RuntimeError('Service process identity changed; refusing signal')
            if hasattr(signal, 'pidfd_send_signal'):
                signal.pidfd_send_signal(fd, signal.SIGTERM)
            else:
                pidfd_syscall(424, fd, signal.SIGTERM, 0, 0)
            poll = select.poll()
            poll.register(fd, select.POLLIN)
            if not poll.poll(int(timeout * 1000)):
                raise RuntimeError('Service retirement timed out; no forced kill')
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            if renderer_clients():
                raise RuntimeError('Renderer client remains after service stop')
        finally:
            os.close(fd)


def main():
    parser = argparse.ArgumentParser(description='Exact-version MiSTer computer service; fail-closed recovery')
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--approved-core', type=Path, required=True)
    parser.add_argument('--profiles', type=Path, required=True)
    parser.add_argument('--selection', type=Path, required=True)
    parser.add_argument('--games', type=Path, required=True)
    parser.add_argument('--state', type=Path, default=Path('/run/spinalvoodoo-computer'))
    parser.add_argument('--stop', action='store_true')
    args = parser.parse_args()
    binary = Path('/media/fat/MiSTer').resolve(strict=True)
    manifest = json.loads(args.manifest.read_text())
    if hashlib.sha256(binary.read_bytes()).hexdigest() != manifest['sha256']:
        parser.error('Main identity mismatch')
    args.state.mkdir(mode=0o700, parents=True, exist_ok=True)
    status = args.state.lstat()
    if args.state.is_symlink() or status.st_uid != os.geteuid() or status.st_mode & 0o077:
        parser.error('Private owned state directory required')
    if args.stop:
        stop_service(args.state)
        return
    stopping = False
    worker = None
    def terminate(_signum, _frame):
        nonlocal stopping
        stopping = True
        if worker is not None and worker.poll() is None:
            worker.send_signal(signal.SIGTERM)
    signal.signal(signal.SIGTERM, terminate)
    signal.signal(signal.SIGINT, terminate)
    directory = Path(__file__).resolve().parent
    command = [sys.executable, str(directory / 'main_reset_broker.py'), '--manifest', str(args.manifest),
               '--state-dir', str(args.state), '--approved-core', str(args.approved_core), '--',
               sys.executable, str(directory / 'computer_launch.py'), '--runtime', str(args.runtime),
               '--profiles', str(args.profiles), '--selection', str(args.selection), '--games', str(args.games),
               '--state', str(args.state)]
    recovered_main = None
    with (args.state / 'service.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        pid = os.getpid()
        birth = (Path('/proc') / str(pid) / 'stat').read_text().split(') ', 1)[1].split()[19]
        temporary = args.state / 'service.pid.tmp'
        temporary.write_text(json.dumps([pid, birth]))
        temporary.replace(args.state / 'service.pid')
        deadline = time.monotonic() + 30
        while len(main_processes(binary)) != 1:
            if stopping:
                return
            if time.monotonic() >= deadline:
                raise RuntimeError('Main did not become available')
            time.sleep(0.1)
        for attempt in range(4):
            if stopping:
                return
            if renderer_clients():
                raise RuntimeError('Unmanaged renderer client exists; refusing startup')
            worker = subprocess.Popen(command)
            if stopping:
                worker.send_signal(signal.SIGTERM)
            status = worker.wait()
            if stopping:
                if status:
                    raise RuntimeError('Broker did not stop cleanly')
                return
            print('Broker exited%d; recovery attempt%d' % (status, attempt + 1), flush=True)
            deadline = time.monotonic() + 5
            while renderer_clients() and time.monotonic() < deadline:
                time.sleep(0.05)
            if renderer_clients():
                raise RuntimeError('Renderer client survived broker failure; refusing recovery')
            mains = main_processes(binary)
            if not mains:
                image = recovery_image(Path('/tmp/CORENAME').read_text().strip(), args.approved_core, manifest)
                if hashlib.sha256(binary.read_bytes()).hexdigest() != manifest['sha256']:
                    raise RuntimeError('Main changed before recovery')
                recovered_main = subprocess.Popen([str(binary), str(image)], start_new_session=True)
                time.sleep(1)
                if recovered_main.poll() is not None:
                    raise RuntimeError('Unchanged Main recovery failed')
            elif len(mains) != 1:
                raise RuntimeError('Ambiguous Main identity during recovery')
            time.sleep(1)
        raise RuntimeError('Repeated broker failures; service latched off')


if __name__ == '__main__':
    main()
