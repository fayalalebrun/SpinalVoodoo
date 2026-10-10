#!/usr/bin/env python3
import argparse
import importlib.util
import json
import os
from pathlib import Path
import re
import tempfile


def profile_path(root, name):
    if not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', name):
        raise ValueError('Profile names require 1–64 letters, digits, underscores or hyphens')
    path = root / 'profiles' / (name + '.json')
    if path.is_symlink() or path.parent.resolve(strict=True) != root / 'profiles':
        raise ValueError('Profile symlinks are not editable')
    return path


def atomic_write(path, content):
    fd, temporary = tempfile.mkstemp(prefix='.' + path.name + '-', dir=path.parent)
    try:
        with os.fdopen(fd, 'w') as output:
            output.write(content)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, path)
        directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def configure(root, validator):
    root = root.resolve(strict=True)
    name = (root / 'selected').read_text().strip()
    path = profile_path(root, name)
    with path.open('rb') as source:
        raw = source.read(65537)
    if len(raw) > 65536:
        raise ValueError('Profile exceeds 64 KiB')
    profile = json.loads(raw, object_pairs_hook=validator.unique_object)
    validator.render(profile, root / 'games')
    while True:
        print('\nMachine: ' + name + '\n' + json.dumps(profile, indent=2))
        print('1 Select profile  2 CPU  3 Cycles  4 Memory  5 Sound  6 Media  7 Save as  0 Save and exit  q Cancel')
        try:
            action = input('Action: ').strip()
            candidate = json.loads(json.dumps(profile))
            if action == 'q':
                return
            if action == '1':
                names = sorted(p.stem for p in (root / 'profiles').glob('*.json')
                               if not p.is_symlink() and re.fullmatch(r'[A-Za-z0-9_-]{1,64}', p.stem))
                print('Profiles: ' + ', '.join(names))
                selected = input('Profile: ').strip()
                path = profile_path(root, selected)
                with path.open('rb') as source:
                    raw = source.read(65537)
                if len(raw) > 65536:
                    raise ValueError('Profile exceeds 64 KiB')
                candidate = json.loads(raw, object_pairs_hook=validator.unique_object)
                validator.render(candidate, root / 'games')
                name = selected
            elif action in ('2', '3', '4', '5'):
                field, prompt = {'2': ('cpu', 'CPU (normal/dynamic/auto): '),
                                 '3': ('cycles', 'Cycles (auto/max/100–1000000): '),
                                 '4': ('memory_mb', 'Memory (8/16/32/64/128 MiB): '),
                                 '5': ('audio', 'Sound (off/sb16/sbpro2; audio-enabled core required): ')}[action]
                value = input(prompt).strip()
                candidate[field] = int(value) if field == 'memory_mb' or field == 'cycles' and value.isdecimal() else value
            elif action == '6':
                letter = input('Drive (A/B/C/D/E): ').strip().upper()
                if letter not in ('A', 'B', 'C', 'D', 'E'):
                    raise ValueError('Supported drives are A through E')
                kind = input('Type (directory/hdd/floppy/iso/eject): ').strip()
                drives = candidate.setdefault('drives', {})
                if kind == 'eject':
                    drives.pop(letter, None)
                else:
                    print('Media root: ' + str(root / 'games'))
                    drives[letter] = {'type': kind, 'path': input('Relative media path: ').strip()}
                    if kind in ('hdd', 'floppy'):
                        access = input('Access (ro/rw; rw requires an exclusively owned raw .img copy): ').strip()
                        if access not in ('ro', 'rw'):
                            raise ValueError('Access must be ro or rw')
                        if access == 'rw':
                            drives[letter]['writable'] = True
            elif action == '7':
                selected = input('Save profile as: ').strip()
                profile_path(root, selected)
                name = selected
            elif action == '0':
                validator.render(candidate, root / 'games')
                path = profile_path(root, name)
                content = json.dumps(candidate, indent=2) + '\n'
                if path.exists():
                    with path.open('rb') as source:
                        previous = source.read(65537)
                    if len(previous) > 65536:
                        raise ValueError('Previous profile exceeds 64 KiB; refusing replacement')
                    fd, backup = tempfile.mkstemp(prefix=name + '-', suffix='.json', dir=root)
                    with os.fdopen(fd, 'wb') as output:
                        output.write(previous)
                        output.flush()
                        os.fsync(output.fileno())
                    print('Previous profile preserved: ' + backup)
                atomic_write(path, content)
                atomic_write(root / 'selected', name + '\n')
                print('Saved; settings apply on next core activation. Media are read-only.')
                return
            else:
                raise ValueError('Unknown action')
            validator.render(candidate, root / 'games')
            profile = candidate
        except (OSError, ValueError, TypeError) as error:
            print('Rejected: ' + str(error))


def main():
    parser = argparse.ArgumentParser(description='Saved machine settings and read-only media editor for stock MiSTer Scripts')
    parser.add_argument('--config', type=Path, default=Path('/media/fat/spinalvoodoo-computer-config'))
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('machine_profile', Path(__file__).resolve().parent / 'machine-profile.py')
    validator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(validator)
    try:
        configure(args.config, validator)
    except (OSError, ValueError, TypeError, EOFError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
