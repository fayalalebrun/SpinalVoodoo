#!/usr/bin/env python3
"""Board acceptance on a new disposable FAT12 image; never edits game media."""
import argparse
import json
import mmap
import os
import signal
from pathlib import Path
import struct
import subprocess
import threading
import time

CONFIG = Path('/media/fat/spinalvoodoo-computer-config')
RUNTIME = Path('/media/fat/spinalvoodoo-computer-runtime07')


def fat12_image():
    image = bytearray(1440 * 1024)
    image[:3] = b'\xeb\x3c\x90'
    image[3:11] = b'SPINAL  '
    struct.pack_into('<HBHBHHBHHHII', image, 11, 512, 1, 1, 2, 224, 2880, 0xf0, 9, 18, 2, 0, 0)
    image[38] = 0x29
    struct.pack_into('<I', image, 39, 0x53505633)
    image[43:54] = b'MEDIA TEST '
    image[54:62] = b'FAT12   '
    image[510:512] = b'\x55\xaa'
    for start in (512, 10 * 512):
        image[start:start + 3] = b'\xf0\xff\xff'
    return bytes(image)


def open_file_image():
    image = bytearray(fat12_image())
    # DOS COM: create SAVED.TXT, write without close, then loop forever.
    content = b'openbuffer\r\n'
    filename = b'SAVED.TXT\0'
    code = bytearray(b'\xb8\x00\x3c\x31\xc9\xba\0\0\xcd\x21\x93\xb4\x40\xba\0\0\xb9')
    code += struct.pack('<H', len(content)) + b'\xcd\x21\xeb\xfe'
    struct.pack_into('<H', code, 6, 0x100 + len(code))
    struct.pack_into('<H', code, 14, 0x100 + len(code) + len(filename))
    program = bytes(code) + filename + content
    for sector in (1, 10): image[sector*512+3:sector*512+5] = b'\xff\x0f'
    entry = 19 * 512
    image[entry:entry+11] = b'OPEN    COM'
    image[entry+11] = 0x20
    struct.pack_into('<H', image, entry+26, 2)
    struct.pack_into('<I', image, entry+28, len(program))
    image[33*512:33*512+len(program)] = program
    return bytes(image)


def fat16_hdd_image():
    # 20 cylinders, 16 heads, 63 sectors; one partition starting at LBA63.
    image = bytearray(20160 * 512)
    struct.pack_into('<8BII', image, 446, 0x80, 1, 1, 0, 6, 15, 63, 19, 63, 20097)
    image[510:512] = b'\x55\xaa'
    boot = 63 * 512
    image[boot:boot+3] = b'\xeb\x3c\x90'
    image[boot+3:boot+11] = b'SPINAL  '
    struct.pack_into('<HBHBHHBHHHII', image, boot+11, 512, 1, 1, 2, 512, 20097, 0xf8, 79, 63, 16, 63, 0)
    image[boot+38] = 0x29
    image[boot+43:boot+54] = b'MEDIA HDD  '
    image[boot+54:boot+62] = b'FAT16   '
    image[boot+510:boot+512] = b'\x55\xaa'
    for sector in (64, 143): image[sector*512:sector*512+4] = b'\xf8\xff\xff\xff'
    return bytes(image)


def saved_file(image):
    data = image.read_bytes()
    base = 63 if data[450] == 6 else 0
    boot = base * 512
    sectors_per_fat = struct.unpack_from('<H', data, boot+22)[0]
    root_entries = struct.unpack_from('<H', data, boot+17)[0]
    root_sector = base + 1 + 2 * sectors_per_fat
    data_sector = root_sector + root_entries * 32 // 512
    fat16 = base != 0
    root = data[root_sector * 512:data_sector * 512]
    entry = next((root[i:i+32] for i in range(0, len(root), 32) if root[i:i+11] == b'SAVED   TXT'), None)
    if entry is None:
        raise RuntimeError('Guest saved file is absent')
    cluster = struct.unpack_from('<H', entry, 26)[0]
    size = struct.unpack_from('<I', entry, 28)[0]
    result = bytearray()
    seen = set()
    while cluster < (0xfff8 if fat16 else 0xff8):
        if cluster < 2 or cluster in seen or cluster >= (19908 if fat16 else 2849):
            raise RuntimeError('Invalid FAT12 chain')
        seen.add(cluster)
        start = (data_sector + cluster - 2) * 512
        result.extend(data[start:start+512])
        offset = (base + 1) * 512 + (cluster * 2 if fat16 else cluster * 3 // 2)
        word = struct.unpack_from('<H', data, offset)[0]
        cluster = word if fat16 else word >> 4 if cluster & 1 else word & 0xfff
    return bytes(result[:size])


def clients():
    found = []
    for process in Path('/proc').glob('[0-9]*'):
        try:
            if str(RUNTIME / 'lib/libmister_runtime.so') in (process / 'maps').read_text():
                found.append(int(process.name))
        except OSError:
            pass
    return found


def main():
    global RUNTIME
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag', default='writable33')
    parser.add_argument('--receiver', default='/tmp/mister-keyboard-receiver35.bin')
    parser.add_argument('--runtime', type=Path, default=RUNTIME)
    parser.add_argument('--scenario', choices=('normal', 'crash', 'reboot', 'open'), default='normal')
    parser.add_argument('--existing-media', help='Read and append to an earlier reboot-test image')
    parser.add_argument('--kind', choices=('floppy', 'hdd'), default='floppy')
    args = parser.parse_args()
    RUNTIME = args.runtime
    tag = args.tag
    drive = 'a' if args.kind == 'floppy' else 'c'
    clean_image = fat12_image if args.kind == 'floppy' else fat16_hdd_image
    if args.scenario == 'open':
        assert args.kind == 'floppy'
        clean_image = open_file_image
    if not tag.isalnum(): raise ValueError('Alphanumeric evidence tag required')
    evidence = CONFIG / 'diagnostic-evidence' / tag
    evidence.mkdir(exist_ok=False)
    assert Path('/tmp/CORENAME').read_text().strip() == 'MENU' and not clients()
    initial_markers = set((CONFIG / 'media-leases').glob('*.dirty'))
    selection = (CONFIG / 'selected').read_bytes()
    (evidence / 'original-selection').write_bytes(selection)
    media_tag = args.existing_media or tag
    if not media_tag.isalnum(): raise ValueError('Alphanumeric media tag required')
    media = CONFIG / 'games' / media_tag
    image = media / 'disk.img'
    baseline = media / 'clean-baseline.img'
    if args.existing_media:
        assert saved_file(image) == b'durablefirst\r\ndurablesecond\r\n'
        print('POST_BOOT_FILE', repr(saved_file(image)), flush=True)
    else:
        media.mkdir(exist_ok=False)
        for path in (image, baseline):
            path.write_bytes(clean_image())
        profile = CONFIG / 'profiles' / (tag + '.json')
        profile.write_text(json.dumps({'version': 1, 'cpu': 'normal', 'cycles': 20000,
            'memory_mb': 16, 'audio': 'off', 'drives': {drive.upper(): {'type': args.kind, 'path': tag + '/disk.img', 'writable': True}}}, indent=2))
    (CONFIG / 'selected').write_text(media_tag + '\n')
    os.sync()
    receiver = subprocess.Popen([str(RUNTIME / 'lib/ld-linux-armhf.so.3'), '--library-path', str(RUNTIME / 'lib'),
        args.receiver], stdin=subprocess.PIPE, stdout=subprocess.PIPE, bufsize=0)
    assert receiver.stdout.readline().strip() == b'READY'
    lock = threading.Lock()
    def send(key, value):
        with lock:
            receiver.stdin.write(struct.pack('<HB', key, value))
    stopped = threading.Event()
    def heartbeat():
        while not stopped.wait(.5): send(0, 0)
    worker = threading.Thread(target=heartbeat)
    worker.start()
    def wait(seconds): time.sleep(seconds)
    def tap(key, shift=False):
        if shift: send(42, 1)
        send(key, 1); wait(.10); send(key, 0)
        if shift: send(42, 0)
        wait(.12)
    keys = dict(zip('qwertyuiopasdfghjklzxcvbnm', [16,17,18,19,20,21,22,23,24,25,30,31,32,33,34,35,36,37,38,44,45,46,47,48,49,50]))
    keys.update({' ':57, '.':52, '\\':43, '-':12})
    def text(command):
        for char in command:
            if char in (':', '>'): tap(39 if char == ':' else 52, True)
            else: tap(keys[char])
        tap(28)
        wait(3)
    def snapshot(name):
        with open('/dev/mem', 'rb', buffering=0) as memory:
            with mmap.mmap(memory.fileno(), 12*1024*1024, flags=mmap.MAP_SHARED, prot=mmap.PROT_READ, offset=0x3e400000) as mapping:
                (evidence / (name + '.hostslots')).write_bytes(mapping[:])
    def enter():
        with open('/dev/MiSTer_cmd', 'w') as command:
            command.write('load_core /media/fat/SpinalVoodoo_game_fixes.rbf\n')
        wait(75)
        print('ENTER', Path('/tmp/CORENAME').read_text(), clients(), flush=True)
        assert Path('/tmp/CORENAME').read_text().strip() == 'SpinalVoodoo' and len(clients()) == 1
        assert len(set((CONFIG / 'media-leases').glob('*.dirty')) - initial_markers) == 1
    expected_dirty = 0
    def depart():
        for key in (88, 106, 103, 103, 28): tap(key)
        wait(12)
        print('DEPART', Path('/tmp/CORENAME').read_text(), clients(), flush=True)
        assert Path('/tmp/CORENAME').read_text().strip() == 'MENU' and not clients()
        assert len(set((CONFIG / 'media-leases').glob('*.dirty')) - initial_markers) == expected_dirty
    try:
        enter()
        if args.scenario == 'open':
            text('a:')
            text('open.com')
            snapshot('guest-file-left-open')
            depart()
            data = saved_file(image)
            assert data == b'openbuffer\r\n', repr(data)
            (evidence / 'open-file').write_bytes(data)
            (evidence / 'open-image.img').write_bytes(image.read_bytes())
            assert baseline.read_bytes() == clean_image()
            print('PASS native departure flushes guest file left open by running COM', repr(data), flush=True)
            return
        if args.existing_media:
            text('type ' + drive + ':\\saved.txt')
            text('echo durablethird>>' + drive + ':\\saved.txt')
            snapshot('post-boot-append')
            depart()
            data = saved_file(image)
            assert data == b'durablefirst\r\ndurablesecond\r\ndurablethird\r\n'
            (evidence / 'post-boot-file').write_bytes(data)
            (evidence / 'post-boot-image.img').write_bytes(image.read_bytes())
            assert baseline.read_bytes() == clean_image()
            print('PASS native-cold-reboot persistence and post-boot guest append', repr(data), flush=True)
            return
        text('echo durablefirst>' + drive + ':\\saved.txt')
        snapshot('first-write')
        depart()
        data = saved_file(image)
        print('FIRST_FILE', repr(data), flush=True)
        assert data == b'durablefirst\r\n'
        (evidence / 'first-file').write_bytes(data)
        (evidence / 'first-image.img').write_bytes(image.read_bytes())
        enter()
        if args.scenario == 'crash':
            (CONFIG / 'selected').write_bytes(selection)
            os.sync()
            mains = [p for p in Path('/proc').glob('[0-9]*')
                     if (p / 'exe').exists() and (p / 'exe').resolve() == Path('/media/fat/MiSTer')]
            assert len(mains) == 1
            broker = int((mains[0] / 'status').read_text().split('TracerPid:\t', 1)[1].splitlines()[0])
            assert broker > 0
            previous = clients()[0]
            os.kill(broker, signal.SIGKILL)
            expected_dirty = 1
            wait(90)
            print('CRASH_RECOVERY', previous, clients(), Path('/run/spinalvoodoo-computer/status').read_text(), flush=True)
            assert previous not in clients()
            assert len(set((CONFIG / 'media-leases').glob('*.dirty')) - initial_markers) == 1
            program = 'import sys; sys.path.insert(0, sys.argv[1]); from media_lease import ImageLease; ImageLease(sys.argv[2], sys.argv[3], sys.argv[4], writable=False)'
            probe = subprocess.run(['python3', '-B', '-c', program, str(RUNTIME), str(CONFIG / 'games'),
                                    tag + '/disk.img', str(CONFIG / 'media-leases')], capture_output=True)
            (evidence / 'readonly-recovery-refusal').write_bytes(probe.stderr)
            assert probe.returncode != 0 and b'Unfinished image requires recovery' in probe.stderr
            depart()
            assert saved_file(image) == b'durablefirst\r\n'
            print('PASS actual broker crash containment, durable gate and clean-profile recovery', flush=True)
            return
        text('type ' + drive + ':\\saved.txt')
        text('echo durablesecond>>' + drive + ':\\saved.txt')
        snapshot('second-write')
        if args.scenario == 'reboot':
            # Keep selected writable profile across boot. The launcher has not
            # cleared its marker until the native reboot boundary retires it.
            for key in (88, 106, 103, 103): tap(key)
            send(28, 1)
            wait(5)
            send(28, 0)
            wait(120)
            raise RuntimeError('Native cold reboot did not disconnect the session')
        depart()
        data = saved_file(image)
        print('SECOND_FILE', repr(data), flush=True)
        assert data == b'durablefirst\r\ndurablesecond\r\n'
        (evidence / 'second-file').write_bytes(data)
        (evidence / 'second-image.img').write_bytes(image.read_bytes())
        assert baseline.read_bytes() == clean_image()
        print('PASS write/native-departure/restart with unchanged baseline', flush=True)
    finally:
        if Path('/tmp/CORENAME').read_text().strip() == 'SpinalVoodoo':
            depart()
        if Path('/tmp/CORENAME').read_text().strip() == 'MENU' and not clients():
            (CONFIG / 'selected').write_bytes(selection)
        stopped.set(); worker.join()
        receiver.stdin.close()
        assert receiver.wait(timeout=5) == 0
        os.sync()
        print('FINAL', Path('/tmp/CORENAME').read_text(), (CONFIG / 'selected').read_text(), flush=True)


if __name__ == '__main__':
    main()
