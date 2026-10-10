#!/usr/bin/env python3
import argparse
import ctypes
import errno
import hashlib
import json
import os
from pathlib import Path
import signal
import struct
import time


def main():
    parser = argparse.ArgumentParser(description='No-guest stock-Main hardware loader-entry trial; not a runtime broker')
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--all-sites', action='store_true')
    parser.add_argument('--follow-restart', action='store_true')
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    binary = Path('/media/fat/MiSTer')
    assert hashlib.sha256(binary.read_bytes()).hexdigest() == manifest['sha256']
    assert Path('/tmp/CORENAME').read_text().strip() == 'MENU'
    for process in Path('/proc').glob('[0-9]*'):
        try:
            assert 'dosbox' not in (process / 'comm').read_text().lower(), 'DOSBox must not be running'
        except FileNotFoundError:
            pass
    mains = [p for p in Path('/proc').glob('[0-9]*') if (p / 'exe').exists() and (p / 'exe').resolve() == binary]
    assert len(mains) == 1
    pid = int(mains[0].name)
    tids = sorted(int(p.name) for p in (mains[0] / 'task').iterdir())
    libc = ctypes.CDLL(None, use_errno=True)
    libc.ptrace.argtypes = [ctypes.c_uint, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]
    libc.ptrace.restype = ctypes.c_long
    def ptrace(request, tid, address=0, data=0):
        result = libc.ptrace(request, tid, ctypes.c_void_p(address), data if not isinstance(data, int) else ctypes.c_void_p(data))
        if result == -1:
            error = ctypes.get_errno()
            raise OSError(error, 'ptrace request=%d tid=%d' % (request, tid))
        return result
    def hbp(tid, number, value):
        word = ctypes.c_uint(value)
        ptrace(30, tid, number, ctypes.byref(word))
    site = next(site for site in manifest['sites'] if site['candidate_function'] == 'fpga_load_rbf')
    address = int(site['address'], 16)
    sites = manifest['sites'] if args.all_sites else [site]
    programmed = {}
    attached = set()
    stopped = set()
    requested = False
    trial_passed = False
    hit = None
    def wait_core(name):
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            if Path('/tmp/CORENAME').read_text().strip() == name:
                return
            time.sleep(0.1)
        raise RuntimeError('Core activation timeout: ' + name)
    def load(path):
        deadline = time.monotonic() + 10
        while True:
            try:
                fd = os.open('/dev/MiSTer_cmd', os.O_WRONLY | os.O_NONBLOCK)
                break
            except OSError as error:
                if error.errno not in (errno.ENXIO, errno.ENOENT) or time.monotonic() >= deadline:
                    raise
                time.sleep(0.05)
        try:
            request = ('load_core ' + path + '\n').encode()
            assert os.write(fd, request) == len(request)
        finally:
            os.close(fd)
    try:
        for tid in tids:
            ptrace(0x4206, tid)
            attached.add(tid)
            ptrace(0x4207, tid)
            waited, status = os.waitpid(tid, 0x40000000)
            assert waited == tid and os.WIFSTOPPED(status)
            stopped.add(tid)
        with (mains[0] / 'mem').open('rb', buffering=0) as memory:
            for candidate in sites:
                memory.seek(int(candidate['address'], 16))
                assert memory.read(candidate['width']).hex() == candidate['first_instruction_bytes']
        for tid in tids:
            programmed[tid] = []
            for index, candidate in enumerate(sites):
                hbp(tid, index * 2 + 1, int(candidate['address'], 16))
                programmed[tid].append(index)
                hbp(tid, index * 2 + 2, (3 << 5) | (2 << 1) | 1)
        print('PASS %d candidate boundaries simultaneously armed on %d threads' % (len(sites), len(tids)), flush=True)
        for tid in tids:
            ptrace(7, tid)
            stopped.remove(tid)
        approved = '/media/fat/SpinalVoodoo_game_fixes.rbf'
        assert hashlib.sha256(Path(approved).read_bytes()).hexdigest() == 'd644e5202a5fc439f08bcb2d59435f6dfafcf634d937949ed46b0e84445c7928'
        load(approved)
        requested = True
        deadline = time.monotonic() + 5
        hit = None
        while time.monotonic() < deadline:
            tid, status = os.waitpid(-1, os.WNOHANG | 0x40000000)
            if not tid:
                time.sleep(0.01)
                continue
            assert tid in attached and os.WIFSTOPPED(status) and os.WSTOPSIG(status) == signal.SIGTRAP
            stopped.add(tid)
            registers = (ctypes.c_uint * 18)()
            ptrace(12, tid, data=ctypes.byref(registers))
            assert registers[15] == address and registers[16] & 0x20
            hit = tid
            break
        assert hit is not None, 'Loader hardware breakpoint was not reached'
        time.sleep(0.5)
        assert Path('/tmp/CORENAME').read_text().strip() == 'MENU'
        print('PASS unchanged Main paused at validated Thumb loader entry before core switch; tid=%d pc=0x%x' % (hit, address), flush=True)
        if args.follow_restart:
            assert args.all_sites
            index = sites.index(site)
            control = (3 << 5) | (2 << 1) | 1
            hbp(hit, index * 2 + 2, control & ~1)
            hbp(hit, index * 2 + 1, address + site['width'])
            hbp(hit, index * 2 + 2, control)
            ptrace(7, hit)
            stopped.remove(hit)
            waited, status = os.waitpid(hit, 0x40000000)
            assert waited == hit and os.WIFSTOPPED(status) and os.WSTOPSIG(status) == signal.SIGTRAP
            stopped.add(hit)
            registers = (ctypes.c_uint * 18)()
            ptrace(12, hit, data=ctypes.byref(registers))
            assert registers[15] == address + site['width']
            hbp(hit, index * 2 + 2, control & ~1)
            hbp(hit, index * 2 + 1, address)
            hbp(hit, index * 2 + 2, control)
            print('PASS real Main loader first instruction stepped with hardware-only comparator rearm', flush=True)
            ptrace(7, hit)
            stopped.remove(hit)
            deadline = time.monotonic() + 15
            target = next(candidate for candidate in sites if candidate['candidate_function'] == 'app_restart')
            while time.monotonic() < deadline:
                tid, status = os.waitpid(-1, os.WNOHANG | 0x40000000)
                if not tid:
                    time.sleep(0.01)
                    continue
                assert tid in attached and os.WIFSTOPPED(status) and os.WSTOPSIG(status) == signal.SIGTRAP
                stopped.add(tid)
                ptrace(12, tid, data=ctypes.byref(registers))
                assert registers[15] == int(target['address'], 16), hex(registers[15])
                hit = tid
                print('PASS real Main intercepted app_restart before its inlined reset and fork pc=0x%x' % registers[15], flush=True)
                break
            else:
                raise RuntimeError('Main restart boundary was not reached')
        trial_passed = True
    finally:
        for tid in sorted(attached, key=lambda tid: tid == hit):
            if tid not in stopped:
                ptrace(0x4207, tid)
                waited, status = os.waitpid(tid, 0x40000000)
                assert waited == tid and os.WIFSTOPPED(status)
            for index in programmed.get(tid, []):
                hbp(tid, index * 2 + 2, (3 << 5) | (2 << 1))
            ptrace(17, tid)
        if requested:
            wait_core('SpinalVoodoo')
            load('/media/fat/menu.rbf')
            wait_core('MENU')
    assert trial_passed
    assert hashlib.sha256(binary.read_bytes()).hexdigest() == manifest['sha256']
    print('PASS debugger detached; signed core load completed; MENU restored; Main unchanged; no guest started', flush=True)


if __name__ == '__main__':
    main()
