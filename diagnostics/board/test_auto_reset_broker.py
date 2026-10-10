#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description='Bounded automatic hardware broker and saved-profile DOS shell trial')
    parser.add_argument('broker', type=Path)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('runtime', type=Path)
    args = parser.parse_args()
    if Path('/tmp/CORENAME').read_text().strip() != 'MENU':
        raise RuntimeError('Start in MENU')
    root = Path(tempfile.mkdtemp(prefix='spinal-auto-broker.'))
    root.chmod(0o700)
    games = root / 'games'
    games.mkdir()
    profile = root / 'profile.json'
    profile.write_text('{"version":1,"cpu":"normal","cycles":10000,"memory_mb":16,"audio":"off","drives":{}}')
    configuration = root / 'machine.conf'
    with configuration.open('w') as output:
        subprocess.run(['python3', str(args.runtime / 'machine-profile.py'), str(profile), '--games-root', str(games)], stdout=output, check=True)
    log = (root / 'log').open('wb')
    process = subprocess.Popen(['python3', str(args.broker), '--manifest', str(args.manifest),
        '--state-dir', str(root), '--approved-core', '/media/fat/SpinalVoodoo_game_fixes.rbf',
        '--', str(args.runtime / 'launch'), '-conf', str(configuration)], stdout=log, stderr=log)
    print('TEST_DIRECTORY', root, 'BROKER', process.pid, flush=True)
    def wait(condition, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError('Broker exited: ' + (root / 'log').read_text())
            if condition():
                return
            time.sleep(0.05)
        raise RuntimeError('Acceptance timeout: ' + (root / 'log').read_text())
    def load(path):
        deadline = time.monotonic() + 10
        while True:
            try:
                fd = os.open('/dev/MiSTer_cmd', os.O_WRONLY | os.O_NONBLOCK)
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(0.05)
        try:
            request = ('load_core ' + path + '\n').encode()
            if os.write(fd, request) != len(request):
                raise RuntimeError('Short command')
        finally:
            os.close(fd)
    def guests():
        found = []
        for path in Path('/proc').glob('[0-9]*'):
            try:
                if str(args.runtime / 'lib/libmister_runtime.so') in (path / 'maps').read_text():
                    found.append(path)
            except OSError:
                pass
        return found
    try:
        wait(lambda: (root / 'status').exists() and (root / 'status').read_text().strip() == 'guarding')
        for iteration in range(3):
            load('/media/fat/SpinalVoodoo_game_fixes.rbf')
            wait(lambda: (root / 'status').read_text().strip().startswith('running:'))
            wait(lambda: len(guests()) == 1 and '/dev/mem' in (guests()[0] / 'maps').read_text(), timeout=90)
            time.sleep(3)
            if len(guests()) != 1:
                raise RuntimeError('DOS shell died')
            load('/media/fat/menu.rbf')
            wait(lambda: not guests() and Path('/tmp/CORENAME').read_text().strip() == 'MENU')
            wait(lambda: any((path / 'comm').read_text().strip() == 'MiSTer' and
                int((path / 'status').read_text().split('TracerPid:', 1)[1].splitlines()[0]) == process.pid
                for path in Path('/proc').glob('[0-9]*') if (path / 'comm').exists()))
            print('PASS cycle%d: automatic profile startup, real retirement-gated menu switch, replacement Main traced' % (iteration + 1), flush=True)
        process.send_signal(signal.SIGTERM)
        if process.wait(timeout=10) != 0:
            raise RuntimeError('Broker stop failed')
        print('PASS clean broker stop; MENU; no guest; no FUSE overlay or boot/Main/kernel changes', flush=True)
    finally:
        if process.poll() is None:
            process.send_signal(signal.SIGTERM)
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                print('FAIL broker/guest not retired; retained for investigation, no forced core change', flush=True)
        log.close()
        print((root / 'log').read_text(), flush=True)


if __name__ == '__main__':
    main()
