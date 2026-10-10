#!/usr/bin/env python3
"""Isolated ownership/fsync probe. Never opens real mounted guest media."""
import argparse
import importlib.util
import os
from pathlib import Path
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', type=Path)
    parser.add_argument('--evidence-root', type=Path, required=True)
    args = parser.parse_args()
    if Path('/tmp/CORENAME').read_text().strip() != 'MENU':
        raise RuntimeError('Start in MENU')
    spec = importlib.util.spec_from_file_location('lease', args.module)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    root = Path(tempfile.mkdtemp(prefix='media-lease-', dir=args.evidence_root))
    print('EVIDENCE', root, flush=True)
    games, state = root / 'games', root / 'state'
    games.mkdir()
    state.mkdir(mode=0o700)
    image = games / 'disk.img'
    image.write_bytes(bytes(1024))
    with module.ImageLease(games, 'disk.img', state) as lease:
        try:
            module.ImageLease(games, 'disk.img', state)
        except BlockingIOError:
            print('PASS concurrent ownership refused', flush=True)
        else:
            raise RuntimeError('Concurrent ownership accepted')
        os.pwrite(lease.fd, b'isolated board write', 0)
        lease.complete(exit_status=0, receipt=b'MEDIAOK\n', eof=True)
    print('PASS image and marker-directory fsync on board filesystem', flush=True)
    with module.ImageLease(games, 'disk.img', state):
        pass
    try:
        module.ImageLease(games, 'disk.img', state)
    except FileExistsError:
        print('PASS unfinished ownership refused; marker retained', flush=True)
    else:
        raise RuntimeError('Unfinished ownership accepted')
    if image.read_bytes()[:20] != b'isolated board write':
        raise RuntimeError('Diagnostic written bytes mismatch')
    for process in Path('/proc').glob('[0-9]*'):
        try:
            if '/media/fat/spinalvoodoo-computer-runtime06/lib/libmister_runtime.so' in (process / 'maps').read_text():
                raise RuntimeError('Unexpected runtime client ' + process.name)
        except OSError:
            pass
    if Path('/tmp/CORENAME').read_text().strip() != 'MENU':
        raise RuntimeError('Core changed during probe')
    print('PASS MENU/no clients; no runtime deployment or original-media writes', flush=True)


if __name__ == '__main__':
    main()
