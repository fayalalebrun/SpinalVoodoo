#!/usr/bin/env python3
import argparse
import ctypes
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import time

from media_lease import ImageLease, prepare_state


def supervise(command, environment, leases):
    """Keep image ownership until checked child exit, then forward retirement."""
    requested = False
    def stop(_signum, _frame):
        nonlocal requested
        requested = True
    for signum in (signal.SIGUSR1, signal.SIGTERM, signal.SIGINT):
        signal.signal(signum, stop)
    reader, writer = os.pipe2(os.O_NONBLOCK | os.O_CLOEXEC)
    parent = os.getpid()
    def child_setup():
        # No worker threads run here. A launcher crash must kill the writer.
        if ctypes.CDLL(None, use_errno=True).prctl(1, signal.SIGKILL, 0, 0, 0):
            os._exit(126)
        if os.getppid() != parent:
            os._exit(126)
    writable = [lease.fd for lease in leases if lease.writable]
    child_env = {**environment, 'MISTER_SHUTDOWN_FD': str(writer),
                 'MISTER_MEDIA_FDS': ','.join(map(str, writable))}
    try:
        child = subprocess.Popen(command, env=child_env, preexec_fn=child_setup,
                                 pass_fds=(writer, *(lease.fd for lease in leases)))
        os.close(writer)
        writer = None
        sent = False
        while child.poll() is None:
            if requested and not sent:
                try:
                    status = Path('/proc/%d/status' % child.pid).read_text()
                    caught = int(status.split('SigCgt:\t', 1)[1].splitlines()[0], 16)
                    if caught & (1 << (signal.SIGUSR1 - 1)):
                        child.send_signal(signal.SIGUSR1)
                        sent = True
                except (OSError, ValueError, IndexError):
                    pass
            time.sleep(0.02)
        receipt = os.read(reader, 9)
        eof = os.read(reader, 1) == b''
        expected = b'MEDIAOK\n' if writable else b'RETIRED\n'
        if child.returncode != 0 or receipt != expected or not eof:
            raise RuntimeError('Child media retirement failed; image recovery is required')
        for lease in leases:
            lease.complete(exit_status=0, receipt=b'MEDIAOK\n' if lease.writable else b'RETIRED\n', eof=True)
        outer = environment.get('MISTER_SHUTDOWN_FD', '')
        if outer:
            if os.write(int(outer), b'RETIRED\n') != 8:
                raise OSError('Short outer retirement receipt')
        return 0
    finally:
        os.close(reader)
        if writer is not None:
            os.close(writer)
        for lease in leases:
            lease.close()


def main():
    parser = argparse.ArgumentParser(description='Start a validated machine profile with checked raw-image ownership')
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--profiles', type=Path, required=True)
    parser.add_argument('--selection', type=Path, required=True)
    parser.add_argument('--games', type=Path, required=True)
    parser.add_argument('--state', type=Path, required=True)
    parser.add_argument('--media-state', type=Path, help='Durable image recovery directory; defaults beside profiles')
    args = parser.parse_args()
    name = args.selection.read_text().strip()
    if not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', name):
        parser.error('Invalid selected profile name')
    profiles = args.profiles.resolve(strict=True)
    profile = (profiles / (name + '.json')).resolve(strict=True)
    if not profile.is_relative_to(profiles):
        parser.error('Profile escapes its directory')
    with profile.open('rb') as source:
        raw = source.read(65537)
    if len(raw) > 65536:
        parser.error('Profile exceeds 64 KiB')

    args.state.mkdir(mode=0o700, parents=True, exist_ok=True)
    status = args.state.lstat()
    if args.state.is_symlink() or status.st_uid != os.geteuid() or status.st_mode & 0o077:
        parser.error('Private owned state directory required')
    fd, snapshot = tempfile.mkstemp(prefix='profile-', suffix='.json', dir=args.state)
    with os.fdopen(fd, 'wb') as output:
        output.write(raw)
    result = subprocess.run([sys.executable, str(args.runtime / 'machine-profile.py'),
                             snapshot, '--games-root', str(args.games)], capture_output=True, text=True)
    if result.returncode:
        parser.exit(1, result.stderr)
    spec = importlib.util.spec_from_file_location('machine_profile', args.runtime / 'machine-profile.py')
    validator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(validator)
    profile_data = json.loads(raw, object_pairs_hook=validator.unique_object)
    audio = profile_data.get('audio', 'off')
    leases = []
    paths = {}
    try:
        for letter, drive in sorted(profile_data.get('drives', {}).items()):
            if drive['type'] in ('floppy', 'hdd') and Path(drive['path']).suffix.lower() == '.img':
                media_state = args.media_state or profiles.parent / 'media-leases'
                prepare_state(media_state)
                lease = ImageLease(args.games, drive['path'], media_state, drive.get('writable', False))
                leases.append(lease)
                paths[letter] = '/proc/self/fd/%d' % lease.fd
        configuration_text = validator.render(profile_data, args.games, media_paths=paths)
    except BaseException:
        for lease in leases:
            lease.close()
        raise
    fd, configuration = tempfile.mkstemp(prefix='machine-', suffix='.conf', dir=args.state)
    with os.fdopen(fd, 'w') as output:
        output.write(configuration_text)
    launcher = args.runtime.resolve(strict=True) / 'launch'
    command = [str(launcher)] + (['--audio'] if audio != 'off' else []) + ['-conf', configuration]
    environment = dict(os.environ)
    environment.pop('MISTER_MEDIA_FDS', None)
    if leases:
        return supervise(command, environment, leases)
    os.execve(launcher, command, environment)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError) as error:
        sys.exit(str(error))
