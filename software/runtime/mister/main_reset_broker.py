#!/usr/bin/env python3
import argparse
import ctypes
import errno
import fcntl
import hashlib
import json
import os
from pathlib import Path
import signal
import re
import subprocess
import time

from core_gate_controller import Session

OPTIONS = 0x100000 | 2 | 4 | 8 | 16
CONTROL = (3 << 5) | (2 << 1) | 1
WALL = 0x40000000


def validate_manifest(manifest):
    names = {'reboot', 'app_restart', 'fpga_load_rbf', 'fpga_core_reset', 'fpga_wait_to_reset'}
    if not isinstance(manifest, dict) or any(not isinstance(manifest.get(key), str) or
            not re.fullmatch(r'[0-9a-f]{64}', manifest[key]) for key in ('sha256', 'core_sha256', 'menu_sha256')):
        raise ValueError('Invalid control identity manifest')
    sites = manifest.get('sites')
    if not isinstance(sites, list) or len(sites) != 5:
        raise ValueError('Exactly five control boundaries required')
    seen = set()
    for site in sites:
        if not isinstance(site, dict) or set(site) != {'candidate_function', 'address', 'first_instruction_bytes', 'width'}:
            raise ValueError('Invalid control boundary')
        if site['candidate_function'] not in names or site['candidate_function'] in seen:
            raise ValueError('Duplicate or unknown control boundary')
        seen.add(site['candidate_function'])
        if type(site['width']) is not int or site['width'] not in (2, 4) or not isinstance(site['address'], str) or not re.fullmatch(r'0x[0-9a-f]{5,6}', site['address']):
            raise ValueError('Invalid control instruction geometry')
        if int(site['address'], 16) & 1 or not isinstance(site['first_instruction_bytes'], str) or not re.fullmatch('[0-9a-f]{%d}' % (site['width'] * 2), site['first_instruction_bytes']):
            raise ValueError('Invalid control instruction bytes')
    if len({site['address'] for site in sites}) != 5:
        raise ValueError('Duplicate control address')


class Broker:
    def __init__(self, binary, manifest, command, approved, core_name, state):
        self.binary = binary.resolve(strict=True)
        self.manifest = manifest
        self.command = command
        self.approved = approved.resolve(strict=True)
        self.core_name = core_name
        self.state = state
        self.tasks = {}
        self.session = None
        self.stable = None
        self.stopping = False
        self.libc = ctypes.CDLL(None, use_errno=True)
        self.libc.ptrace.argtypes = [ctypes.c_uint, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]
        self.libc.ptrace.restype = ctypes.c_long

    def trace(self, request, tid, address=0, data=0):
        result = self.libc.ptrace(request, tid, ctypes.c_void_p(address),
                                 ctypes.c_void_p(data) if isinstance(data, int) else data)
        if result == -1:
            error = ctypes.get_errno()
            raise OSError(error, 'ptrace request=%d tid=%d' % (request, tid))
        return result

    def hbp(self, tid, number, value):
        word = ctypes.c_uint(value)
        self.trace(30, tid, number, ctypes.byref(word))

    def arm(self, tid):
        process = Path('/proc') / str(tid)
        if (process / 'exe').resolve(strict=True) != self.binary:
            if not self.retire():
                raise RuntimeError('Cannot allow Main helper with unretired guest')
            task = self.tasks[tid]
            task.update(armed=0, step=None, ready=True, helper=True)
            self.status('guarded-helper:' + str((process / 'exe').resolve(strict=True)) + ':' + repr((process / 'cmdline').read_bytes()))
            return
        self.tasks[tid]['helper'] = False
        if hashlib.sha256((process / 'exe').read_bytes()).hexdigest() != self.manifest['sha256']:
            raise RuntimeError('Main identity changed')
        with (process / 'mem').open('rb', buffering=0) as memory:
            for site in self.manifest['sites']:
                memory.seek(int(site['address'], 16))
                if memory.read(site['width']).hex() != site['first_instruction_bytes']:
                    raise RuntimeError('Main instruction identity changed')
        task = self.tasks[tid]
        task['armed'] = 0
        for index, site in enumerate(self.manifest['sites']):
            self.hbp(tid, index * 2 + 1, int(site['address'], 16))
            task['armed'] = index + 1
            self.hbp(tid, index * 2 + 2, CONTROL)
        task['step'] = None
        task['ready'] = True

    def resume(self, tid, signum=0):
        self.trace(7, tid, data=signum)
        self.tasks[tid]['stopped'] = False
        self.tasks[tid]['pending_status'] = None

    def enroll(self):
        mains = []
        for process in Path('/proc').glob('[0-9]*'):
            try:
                if (process / 'exe').resolve(strict=True) == self.binary:
                    mains.append(process)
            except (OSError, RuntimeError):
                continue
        if len(mains) != 1:
            raise RuntimeError('Exactly one Main process must be running')
        main = mains[0]
        for _ in range(8):
            tids = {int(task.name) for task in (main / 'task').iterdir()}
            for tid in sorted(tids - self.tasks.keys()):
                self.trace(0x4206, tid, data=OPTIONS)
                self.tasks[tid] = {'stopped': False, 'ready': False, 'armed': 0, 'step': None}
                self.trace(0x4207, tid)
                waited, status = os.waitpid(tid, WALL)
                if waited != tid or not os.WIFSTOPPED(status):
                    raise RuntimeError('Cannot stop Main thread before enrollment')
                self.tasks[tid]['stopped'] = True
                self.arm(tid)
            if {int(task.name) for task in (main / 'task').iterdir()} <= self.tasks.keys():
                break
        else:
            raise RuntimeError('Main thread set did not stabilize')
        for tid in list(self.tasks):
            self.resume(tid)
        self.status('guarding')

    def status(self, text):
        temporary = self.state / 'status.tmp'
        temporary.write_text(text + '\n')
        temporary.replace(self.state / 'status')
        print(text, flush=True)

    def retire(self):
        if self.session is None:
            return True
        if not self.session.stop():
            self.status('retirement-refused:exit=%s' % self.session.child.poll())
            return False
        self.session = None
        self.status('retired')
        return True

    def event(self, tid, status):
        task = self.tasks[tid]
        if os.WIFEXITED(status) or os.WIFSIGNALED(status):
            del self.tasks[tid]
            self.stable = None
            return
        if not os.WIFSTOPPED(status):
            raise RuntimeError('Unexpected Main wait state')
        task['stopped'] = True
        task['pending_status'] = status
        event = status >> 16
        signum = os.WSTOPSIG(status)
        if event in (1, 2, 3):
            child = ctypes.c_ulong()
            self.trace(0x4201, tid, data=ctypes.byref(child))
            self.tasks[child.value] = {'stopped': False, 'ready': False, 'armed': 0, 'step': None}
            self.resume(tid)
            return
        if event == 4:
            previous = ctypes.c_ulong()
            self.trace(0x4201, tid, data=ctypes.byref(previous))
            if previous.value != tid:
                self.tasks.pop(previous.value, None)
            if not self.retire():
                return
            self.arm(tid)
            self.stable = None
            self.resume(tid)
            return
        if event == 128:
            if not task['ready']:
                self.trace(0x4200, tid, data=OPTIONS)
                self.arm(tid)
            if not self.stopping:
                self.resume(tid)
            return
        if signum != signal.SIGTRAP:
            self.resume(tid, signum)
            return
        registers = (ctypes.c_uint * 18)()
        self.trace(12, tid, data=ctypes.byref(registers))
        if not registers[16] & 0x20:
            raise RuntimeError('Unexpected non-Thumb hardware trap')
        pc = registers[15]
        if task['step'] is not None:
            index = task['step']
            site = self.manifest['sites'][index]
            address = int(site['address'], 16)
            if pc != address + site['width']:
                raise RuntimeError('Unexpected hardware stepping PC')
            self.hbp(tid, index * 2 + 2, CONTROL & ~1)
            self.hbp(tid, index * 2 + 1, address)
            self.hbp(tid, index * 2 + 2, CONTROL)
            task['step'] = None
            self.resume(tid)
            return
        for index, site in enumerate(self.manifest['sites']):
            if pc == int(site['address'], 16):
                self.stable = None
                self.status('intercepted:' + site['candidate_function'])
                if not self.retire():
                    return
                self.hbp(tid, index * 2 + 2, CONTROL & ~1)
                self.hbp(tid, index * 2 + 1, pc + site['width'])
                self.hbp(tid, index * 2 + 2, CONTROL)
                task['step'] = index
                self.resume(tid)
                return
        raise RuntimeError('Unrecognized Main SIGTRAP')

    def maybe_start(self):
        if self.session is not None or self.stopping or not self.tasks or any(
                task['stopped'] or not task['ready'] or task['step'] is not None or task.get('helper', False) for task in self.tasks.values()):
            self.stable = None
            return
        matches = []
        for tid in self.tasks:
            process = Path('/proc') / str(tid)
            try:
                if process.name != (process / 'status').read_text().split('Tgid:\t', 1)[1].splitlines()[0]:
                    continue
                arguments = (process / 'cmdline').read_bytes().split(b'\0')
                if len(arguments) > 1 and Path(os.fsdecode(arguments[1])).resolve(strict=True) == self.approved:
                    matches.append((tid, (process / 'stat').read_text().split(') ', 1)[1].split()[19]))
            except (OSError, ValueError, IndexError):
                self.stable = None
                return
        if len(matches) != 1 or Path('/tmp/CORENAME').read_text().strip() != self.core_name:
            self.stable = None
            return
        identity = tuple(matches)
        if self.stable is None or self.stable[0] != identity:
            self.stable = (identity, time.monotonic())
            return
        if time.monotonic() - self.stable[1] < 8:
            return
        if hashlib.sha256(self.approved.read_bytes()).hexdigest() != self.manifest['core_sha256']:
            raise RuntimeError('Approved core image changed')
        self.session = Session(self.command)
        self.status('running:pid=%d' % self.session.child.pid)

    def run(self):
        self.enroll()
        try:
            while self.tasks:
                for tid in list(self.tasks):
                    if tid not in self.tasks:
                        continue
                    pending = self.tasks[tid].get('pending_status')
                    if pending is not None:
                        self.event(tid, pending)
                        continue
                    try:
                        waited, status = os.waitpid(tid, os.WNOHANG | WALL)
                    except ChildProcessError:
                        self.tasks.pop(tid, None)
                        continue
                    if waited:
                        self.event(tid, status)
                if self.stopping and self.retire():
                    break
                self.maybe_start()
                time.sleep(0.01)
        finally:
            if not self.retire():
                self.status('fatal:guest-not-retired;Main-EXITKILL-on-broker-exit')
                raise RuntimeError('Refusing to release Main with unretired guest')
            for tid, task in list(self.tasks.items()):
                try:
                    if not task['stopped']:
                        self.trace(0x4207, tid)
                        waited, status = os.waitpid(tid, WALL)
                        if waited != tid or not os.WIFSTOPPED(status):
                            continue
                    for index in range(task['armed']):
                        self.hbp(tid, index * 2 + 2, CONTROL & ~1)
                    self.trace(17, tid)
                except (OSError, ChildProcessError) as error:
                    if isinstance(error, OSError) and error.errno != errno.ESRCH:
                        raise
            self.status('stopped')


def main():
    parser = argparse.ArgumentParser(description='Experimental exact-version stock-Main hardware-reset broker')
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--state-dir', type=Path, required=True)
    parser.add_argument('--approved-core', type=Path, required=True)
    parser.add_argument('--core-name', default='SpinalVoodoo')
    parser.add_argument('--main', type=Path, default=Path('/media/fat/MiSTer'))
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command or not Path(command[0]).is_absolute():
        parser.error('Absolute source-matched launcher required')
    manifest = json.loads(args.manifest.read_text())
    validate_manifest(manifest)
    if hashlib.sha256(args.main.read_bytes()).hexdigest() != manifest['sha256']:
        parser.error('Main identity does not match')
    if hashlib.sha256(args.approved_core.read_bytes()).hexdigest() != manifest['core_sha256']:
        parser.error('Core identity does not match')
    args.state_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
    status = args.state_dir.lstat()
    if args.state_dir.is_symlink() or status.st_uid != os.geteuid() or status.st_mode & 0o077:
        parser.error('Private owned state directory required')
    with (args.state_dir / 'broker.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        broker = Broker(args.main, manifest, command, args.approved_core, args.core_name, args.state_dir)
        def terminate(_signum, _frame):
            broker.stopping = True
        signal.signal(signal.SIGTERM, terminate)
        signal.signal(signal.SIGINT, terminate)
        broker.run()


if __name__ == '__main__':
    main()
