#!/usr/bin/env python3
import argparse
import hashlib
import importlib.util
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import threading
import time


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def wait_core(name):
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        if Path('/tmp/CORENAME').read_text().strip() == name:
            return
        time.sleep(0.1)
    raise RuntimeError('Core activation did not complete: ' + name)


def load_core(path):
    fd = os.open('/dev/MiSTer_cmd', os.O_WRONLY | os.O_NONBLOCK)
    try:
        request = ('load_core ' + str(path) + '\n').encode()
        assert os.write(fd, request) == len(request)
    finally:
        os.close(fd)


def main():
    parser = argparse.ArgumentParser(description='Bounded real DOSBox gate trial; no guest media, no boot installation')
    parser.add_argument('controller', type=Path)
    parser.add_argument('gate', type=Path)
    parser.add_argument('runtime', type=Path)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('controller', args.controller)
    controller = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(controller)
    assert Path('/tmp/CORENAME').read_text().strip() == 'MENU'
    approved = Path('/media/fat/SpinalVoodoo_game_fixes.rbf')
    assert digest(approved) == 'd644e5202a5fc439f08bcb2d59435f6dfafcf634d937949ed46b0e84445c7928'
    subprocess.run(['sha256sum', '-c', 'SHA256SUMS'], cwd=args.runtime, stdout=subprocess.DEVNULL, check=True)
    preserved = {str(path): digest(path) for path in ('/media/fat/MiSTer', '/media/fat/menu.rbf', '/media/fat/linux/zImage_dtb')}
    root = Path(tempfile.mkdtemp(prefix='spinal-real-fuse.'))
    root.chmod(0o700)
    mount = root / 'view'
    mount.mkdir(mode=0o700)
    backing = root / 'menu-copy.rbf'
    backing.write_bytes(Path('/media/fat/menu.rbf').read_bytes())
    backing.chmod(0o400)
    print('TEST_DIRECTORY', root, 'PRESERVED', preserved, flush=True)
    session = None
    daemon = None
    thread = None
    server = socket.socket(socket.AF_UNIX)
    server.bind(str(root / 'control.sock'))
    (root / 'control.sock').chmod(0o600)
    server.listen(1)
    server.settimeout(15)
    log = (root / 'gate.log').open('wb')
    def mounted():
        return any(line.split()[4] == str(mount) for line in Path('/proc/self/mountinfo').read_text().splitlines())
    try:
        load_core(approved)
        wait_core('SpinalVoodoo')
        print('PASS approved signed game-fixes core activated through unchanged Main', flush=True)
        session = controller.Session([str(args.runtime / 'launch')])
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline:
            if session.child.poll() is not None:
                raise RuntimeError('DOSBox exited before retirement trial: ' + str(session.child.returncode))
            maps = Path('/proc/' + str(session.child.pid) + '/maps').read_text()
            if '/dev/mem' in maps and 'libmister_runtime' in maps:
                break
            time.sleep(0.1)
        else:
            raise RuntimeError('Source-matched DOSBox never acquired renderer mapping')
        time.sleep(3)
        assert session.child.poll() is None
        print('PASS real DOSBox session owns renderer mapping; no guest media mounted', flush=True)
        daemon = subprocess.Popen([str(args.gate), str(backing), str(root / 'control.sock'), str(mount)], stdout=log, stderr=log)
        deadline = time.monotonic() + 5
        while not mounted():
            if daemon.poll() is not None or time.monotonic() >= deadline:
                raise RuntimeError('FUSE mount failed')
            time.sleep(0.05)
        observed = []
        def handle():
            connection, _ = server.accept()
            with connection:
                controller.serve_connection(connection, session)
                observed.append((session.retired, session.child.poll()))
        thread = threading.Thread(target=handle)
        thread.start()
        load_core(mount / 'core.rbf')
        wait_core('MENU')
        thread.join(2)
        assert not thread.is_alive() and observed == [(True, 0)], observed
        assert session.retired and session.child.poll() == 0
        print('PASS unchanged Main loaded guarded menu only after real DOSBox fence/release receipt and exit0', flush=True)
    finally:
        safe = session is None or session.retired or session.stop()
        if not safe:
            print('FAIL retirement not acknowledged; leaving active core and gate intact for investigation', flush=True)
            raise RuntimeError('Refusing unsafe automatic core recovery')
        if thread is not None:
            thread.join(16)
        server.close()
        if daemon is not None and daemon.poll() is None:
            daemon.terminate()
            daemon.wait(timeout=5)
        if mounted():
            subprocess.run(['umount', str(mount)], check=True, timeout=5)
        log.close()
        if Path('/tmp/CORENAME').read_text().strip() != 'MENU':
            load_core(Path('/media/fat/menu.rbf'))
            wait_core('MENU')
        for path, expected in preserved.items():
            assert digest(path) == expected, path
    print('PASS MENU restored; Main/kernel/menu hashes unchanged; isolated mounts removed; no boot changes', flush=True)


if __name__ == '__main__':
    main()
