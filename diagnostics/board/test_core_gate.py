#!/usr/bin/env python3
import argparse
import hashlib
import os
from pathlib import Path
import queue
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time


def main():
    parser = argparse.ArgumentParser(description='Isolated FUSE acceptance test; never loads a core or starts DOSBox')
    parser.add_argument('binary', type=Path)
    parser.add_argument('image', type=Path)
    args = parser.parse_args()
    root = Path(tempfile.mkdtemp(prefix='spinal-fuse-test.'))
    root.chmod(0o700)
    mount = root / 'view'
    mount.mkdir(mode=0o700)
    backing = root / 'backing.rbf'
    backing.write_bytes(args.image.read_bytes())
    backing.chmod(0o400)
    digest = hashlib.sha256(backing.read_bytes()).hexdigest()
    print('TEST_DIRECTORY', root, 'IMAGE_SHA256', digest, flush=True)
    control = root / 'control'
    requests = queue.Queue()
    entered = threading.Event()
    release = threading.Event()
    stopped = threading.Event()
    completed = threading.Event()
    guest = subprocess.Popen([sys.executable, '-c',
        'import signal,time,sys; signal.signal(signal.SIGUSR1, lambda *_: sys.exit(0)); print("READY",flush=True); time.sleep(60)'], stdout=subprocess.PIPE, text=True)
    assert guest.stdout.readline().strip() == 'READY'
    server = socket.socket(socket.AF_UNIX)
    server.bind(str(control))
    control.chmod(0o600)
    server.listen(4)
    server.settimeout(0.2)
    def respond():
        while not stopped.is_set():
            try:
                connection, _ = server.accept()
            except socket.timeout:
                continue
            with connection:
                connection.settimeout(2)
                assert connection.recv(5) == b'STOP\n'
                mode = requests.get(timeout=2)
                entered.set()
                if mode == 'allow':
                    release.wait(3)
                    guest.send_signal(signal.SIGUSR1)
                    assert guest.wait(timeout=3) == 0
                    connection.sendall(b'OK\n')
                elif mode == 'deny':
                    connection.sendall(b'NO\n')
                elif mode == 'timeout':
                    release.wait(7)
                elif mode == 'crash':
                    release.wait(3)
            completed.set()
    thread = threading.Thread(target=respond)
    thread.start()
    log = (root / 'daemon.log').open('wb')
    daemon = subprocess.Popen([str(args.binary.resolve()), str(backing), str(control), str(mount)], stdout=log, stderr=log)
    reader_command = [sys.executable, '-c',
        'import hashlib,sys; print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())', str(mount / 'core.rbf')]
    reader = None
    def is_mounted():
        return any(line.split()[4] == str(mount) for line in Path('/proc/self/mountinfo').read_text().splitlines())
    try:
        deadline = time.monotonic() + 5
        while not is_mounted():
            if daemon.poll() is not None or time.monotonic() > deadline:
                raise RuntimeError('FUSE mount failed: ' + (root / 'daemon.log').read_text())
            time.sleep(0.05)
        assert sorted(p.name for p in mount.iterdir()) == ['core.rbf']
        requests.put('allow')
        reader = subprocess.Popen(reader_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert entered.wait(3)
        time.sleep(0.3)
        assert reader.poll() is None and guest.poll() is None
        release.set()
        output, error = reader.communicate(timeout=5)
        assert reader.returncode == 0 and output.strip() == digest, error
        assert guest.poll() == 0
        print('PASS open held until cooperative surrogate exit; bytes match backing', flush=True)
        requests.put('deny')
        result = subprocess.run(reader_command, capture_output=True, text=True, timeout=5)
        assert result.returncode != 0 and 'Input/output error' in result.stderr
        print('PASS refused shutdown denies a new open after successful earlier read', flush=True)
        try:
            with (mount / 'core.rbf').open('wb'):
                pass
            raise AssertionError('Write permitted')
        except OSError:
            pass
        print('PASS writes denied', flush=True)
        control.chmod(0o666)
        result = subprocess.run(reader_command, capture_output=True, text=True, timeout=3)
        assert result.returncode != 0
        control.chmod(0o600)
        print('PASS insecure control socket denies open', flush=True)
        offline = root / 'offline-control'
        control.rename(offline)
        result = subprocess.run(reader_command, capture_output=True, text=True, timeout=3)
        assert result.returncode != 0
        offline.rename(control)
        print('PASS missing control socket denies open', flush=True)
        completed.clear()
        release.clear()
        requests.put('timeout')
        started = time.monotonic()
        result = subprocess.run(reader_command, capture_output=True, text=True, timeout=8)
        assert result.returncode != 0 and time.monotonic() - started < 7
        release.set()
        assert completed.wait(3)
        print('PASS unresponsive shutdown times out and denies open', flush=True)
        entered.clear()
        release.clear()
        requests.put('crash')
        reader = subprocess.Popen(reader_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert entered.wait(3)
        assert reader.poll() is None
        daemon.kill()
        daemon.wait(timeout=3)
        output, error = reader.communicate(timeout=3)
        assert reader.returncode != 0, output
        result = subprocess.run(reader_command, capture_output=True, text=True, timeout=3)
        assert result.returncode != 0
        print('PASS daemon crash fails pending and subsequent opens closed', flush=True)
        assert hashlib.sha256(backing.read_bytes()).hexdigest() == digest
    finally:
        release.set()
        stopped.set()
        thread.join(8)
        server.close()
        if reader is not None and reader.poll() is None:
            reader.kill()
            reader.wait()
        if guest.poll() is None:
            guest.terminate()
            guest.wait(timeout=3)
        if daemon.poll() is None:
            daemon.terminate()
            daemon.wait(timeout=3)
        if is_mounted():
            subprocess.run(['umount', str(mount)], check=True, timeout=5)
        log.close()
    print('PASS isolated cleanup; test artifacts retained; Main/kernel/boot unchanged', flush=True)


if __name__ == '__main__':
    main()
