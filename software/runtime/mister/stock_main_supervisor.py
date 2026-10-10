#!/usr/bin/env python3
import argparse
import ctypes
import fcntl
import hashlib
import json
import os
from pathlib import Path
import select
import signal
import struct
import subprocess
import sys
import time

HEADER = struct.Struct('=IBBHQii')
OPEN_PERMISSION = 0x10000
OVERFLOW = 0x4000


def events(data):
    while data:
        if len(data) < HEADER.size:
            raise ValueError('Truncated fanotify event')
        size, version, _, metadata_size, mask, fd, pid = HEADER.unpack_from(data)
        if version != 3 or size < metadata_size or metadata_size < HEADER.size or size > len(data):
            raise ValueError('Invalid fanotify metadata')
        if mask & OVERFLOW:
            raise RuntimeError('Permission queue overflow; interlock cannot continue')
        yield mask, fd, pid
        data = data[size:]


def open_gate(mounts):
    libc = ctypes.CDLL(None, use_errno=True)
    libc.fanotify_init.argtypes = [ctypes.c_uint, ctypes.c_uint]
    libc.fanotify_mark.argtypes = [ctypes.c_int, ctypes.c_uint, ctypes.c_uint64, ctypes.c_int, ctypes.c_char_p]
    fd = libc.fanotify_init(0x4 | 0x2 | 0x1, os.O_RDONLY | os.O_CLOEXEC)
    if fd < 0:
        error = ctypes.get_errno()
        raise OSError(error, 'Kernel permission interlock unavailable; machine will not start')
    try:
        for mount in mounts:
            if libc.fanotify_mark(fd, 0x1 | 0x10, OPEN_PERMISSION, -100, os.fsencode(mount)):
                error = ctypes.get_errno()
                raise OSError(error, 'Cannot guard media mount: ' + str(mount))
        return fd
    except BaseException:
        os.close(fd)
        raise


class Machine:
    def __init__(self, command, gate):
        self.command = command
        self.gate = gate
        self.child = None
        self.state = 'idle'

    def start(self):
        if self.child is not None:
            raise RuntimeError('Previous machine session must be stopped first')
        self.child = subprocess.Popen(self.command, pass_fds=(self.gate,), start_new_session=True)
        self.state = 'running'

    def stop(self, service=None):
        if self.child is None:
            return True
        if self.child.poll() is None:
            self.child.send_signal(signal.SIGUSR1)
        deadline = time.monotonic() + 5
        while True:
            try:
                status = self.child.wait(timeout=0.05 if service else 5)
                break
            except subprocess.TimeoutExpired:
                if service:
                    service()
                if not service or time.monotonic() >= deadline:
                    self.state = 'shutdown-timeout'
                    return False
        if status != 0:
            self.state = 'unclean-exit'
            return False
        self.child = None
        self.state = 'idle'
        return True


def release_permission(gate, fd, allow):
    try:
        response = struct.pack('=iI', fd, 1 if allow else 2)
        if os.write(gate, response) != len(response):
            raise OSError('Short fanotify permission response')
    finally:
        os.close(fd)


def main():
    parser = argparse.ArgumentParser(description='Experimental stock-Main supervisor; requires kernel fanotify permission support. Not release-validated.')
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--state-dir', type=Path, default=Path('/run/spinalvoodoo'))
    args = parser.parse_args()
    config = json.loads(args.config.read_text())
    if not isinstance(config, dict) or set(config) != {'command', 'images', 'mounts'}:
        parser.error('Config requires command, images and mounts')
    command, images, mounts = config['command'], config['images'], config['mounts']
    if not isinstance(command, list) or not command or any(not isinstance(x, str) or '\0' in x for x in command) or not Path(command[0]).is_absolute():
        parser.error('Command must be an argument array with an absolute executable')
    if not isinstance(images, dict) or not images or any(not Path(name).is_absolute() or not isinstance(digest, str) or len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest) for name, digest in images.items()):
        parser.error('Images must map absolute RBF paths to approved SHA256 hashes')
    if not isinstance(mounts, list) or not mounts or any(not isinstance(x, str) or not Path(x).is_absolute() or not Path(x).is_mount() for x in mounts):
        parser.error('Mounts must list every MiSTer core-storage mount point')
    if any(not any(Path(name).is_relative_to(Path(mount)) for mount in mounts) for name in images):
        parser.error('Approved images must be inside guarded mounts')
    args.state_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
    if args.state_dir.is_symlink() or args.state_dir.stat().st_uid != os.geteuid() or args.state_dir.stat().st_mode & 0o077:
        parser.error('State directory must be private and owned by this user')
    with (args.state_dir / 'supervisor.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        gate = open_gate(mounts)
        machine = Machine(command, gate)
        pending = None
        identity = None
        seen = None
        stopping = False
        def service_shutdown():
            if not select.select([gate], [], [], 0)[0]:
                return
            for mask, fd, _pid in events(os.read(gate, 65536)):
                if fd < 0:
                    continue
                if mask & OPEN_PERMISSION:
                    path = os.readlink('/proc/self/fd/' + str(fd))
                    release_permission(gate, fd, not path.lower().endswith('.rbf'))
                else:
                    os.close(fd)
        def terminate(_signum, _frame):
            nonlocal stopping
            stopping = True
        signal.signal(signal.SIGTERM, terminate)
        signal.signal(signal.SIGINT, terminate)
        try:
            while not stopping:
                if select.select([gate], [], [], 0.05)[0]:
                    for mask, fd, pid in events(os.read(gate, 65536)):
                        if fd < 0:
                            continue
                        if not mask & OPEN_PERMISSION:
                            os.close(fd)
                            continue
                        path = os.readlink('/proc/self/fd/' + str(fd))
                        if pid == os.getpid() or not path.lower().endswith('.rbf'):
                            release_permission(gate, fd, True)
                            continue
                        allowed = machine.stop(service_shutdown)
                        if allowed:
                            pending = path if path in images else None
                            identity = None
                            seen = None
                            if pending:
                                digest_state = hashlib.sha256()
                                offset = 0
                                while True:
                                    data = os.pread(fd, 65536, offset)
                                    if not data:
                                        break
                                    digest_state.update(data)
                                    offset += len(data)
                                digest = digest_state.hexdigest()
                                if digest != images[pending]:
                                    pending = None
                                    allowed = False
                        release_permission(gate, fd, allowed)
                if pending and machine.child is None:
                    mains = []
                    for proc in Path('/proc').glob('[0-9]*'):
                        try:
                            argv = (proc / 'cmdline').read_bytes().split(b'\0')
                            if len(argv) > 1 and os.fsdecode(argv[1]) == pending and Path(os.fsdecode(argv[0])).name == 'MiSTer':
                                mains.append((proc.name, (proc / 'stat').read_text().split(') ', 1)[1].split()[19]))
                        except (OSError, ValueError, IndexError):
                            continue
                    current = tuple(mains)
                    if len(current) == 1 and Path('/tmp/CORENAME').read_text() == 'SpinalVoodoo':
                        if current != identity:
                            identity, seen = current, time.monotonic()
                        elif time.monotonic() - seen >= 2:
                            machine.start()
                            pending = None
                    else:
                        identity = seen = None
                temporary = args.state_dir / 'status.tmp'
                temporary.write_text(machine.state + '\n')
                temporary.replace(args.state_dir / 'status')
        finally:
            clean = machine.stop(service_shutdown)
            if not clean and machine.child.poll() is None:
                print('Shutdown failed; retaining permission gate until child exits', file=sys.stderr)
                machine.child.wait()
            os.close(gate)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
