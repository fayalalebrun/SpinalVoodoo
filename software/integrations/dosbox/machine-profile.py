#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
import re
import sys


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate profile field: ' + key)
        result[key] = value
    return result


def render(profile, games, media_paths=None):
    if not isinstance(profile, dict) or set(profile) - {'version', 'cpu', 'cycles', 'memory_mb', 'audio', 'drives'}:
        raise ValueError('Unknown profile fields')
    if type(profile.get('version')) is not int or profile['version'] != 1:
        raise ValueError('Unsupported profile version')
    cpu = profile.get('cpu', 'normal')
    cycles = profile.get('cycles', 'auto')
    memory = profile.get('memory_mb', 16)
    audio = profile.get('audio', 'off')
    if cpu not in ('normal', 'dynamic', 'auto'):
        raise ValueError('Unsupported CPU core')
    if not ((type(cycles) is int and 100 <= cycles <= 1000000) or (type(cycles) is str and cycles in ('auto', 'max'))):
        raise ValueError('Cycles must be auto, max or an integer from 100 to 1000000')
    if type(memory) is not int or memory not in (8, 16, 32, 64, 128):
        raise ValueError('Unsupported memory size')
    if audio not in ('off', 'sb16', 'sbpro2'):
        raise ValueError('Unsupported audio mode')
    games = games.resolve(strict=True)
    if not games.is_dir():
        raise ValueError('Games root must be a directory')
    drives = profile.get('drives', {})
    if not isinstance(drives, dict) or set(drives) - {'A', 'B', 'C', 'D', 'E'}:
        raise ValueError('Supported drives are A through E')
    commands = []
    for letter, drive in sorted(drives.items()):
        if not isinstance(drive, dict) or not {'type', 'path'} <= set(drive) or set(drive) - {'type', 'path', 'writable'}:
            raise ValueError('Each drive requires type and path; writable is optional')
        kind, name = drive['type'], drive['path']
        writable = drive.get('writable', False)
        if type(writable) is not bool:
            raise ValueError('Writable must be a boolean')
        if kind not in ('directory', 'hdd', 'floppy', 'iso') or not isinstance(name, str) or not name:
            raise ValueError('Unsupported drive type or path')
        relative = Path(name)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Media path must stay inside the games root')
        path = (games / relative).resolve(strict=True)
        if not path.is_relative_to(games):
            raise ValueError('Media symlink escapes the games root')
        if not re.fullmatch(r'[A-Za-z0-9_ ./()+-]+', str(path)):
            raise ValueError('Media path contains DOS command metacharacters or unsupported characters')
        if writable and (kind not in ('floppy', 'hdd') or path.suffix.lower() != '.img'):
            raise ValueError('Writable media require a raw floppy or HDD .img image')
        if kind == 'directory':
            if letter not in ('C', 'D', 'E') or not path.is_dir():
                raise ValueError('Directory drives require C, D or E and an existing directory')
            commands.append(f'mount {letter} "{path}" -ro')
        else:
            if not path.is_file():
                raise ValueError('Image must be a regular file')
            if writable:
                info = path.stat()
                if info.st_nlink != 1 or info.st_size <= 0 or info.st_size % 512:
                    raise ValueError('Writable image must have one link and a nonempty sector-aligned size')
            if kind == 'floppy' and letter not in ('A', 'B') or kind == 'hdd' and letter not in ('C', 'D') or kind == 'iso' and letter not in ('D', 'E'):
                raise ValueError('Image type is incompatible with the selected drive')
            if path.suffix.lower() not in {'floppy': ('.img',), 'hdd': ('.img', '.vhd'), 'iso': ('.iso',)}[kind]:
                raise ValueError('Unsupported image extension')
            if media_paths is not None and letter in media_paths:
                path = Path(media_paths[letter])
                if not re.fullmatch(r'/proc/self/fd/[0-9]+', str(path)):
                    raise ValueError('Leased descriptor path required')
            commands.append(f'imgmount {letter} "{path}" -t {kind}' + ('' if writable else ' -ro'))
    commands.append('C:' if 'C' in drives else 'Z:')
    sound = 'none' if audio == 'off' else audio
    return f'[dosbox]\nmemsize={memory}\n\n[cpu]\ncore={cpu}\ncycles={cycles}\n\n[mixer]\nnosound={str(audio == "off").lower()}\n\n[sblaster]\nsbtype={sound}\n\n[autoexec]\n' + '\n'.join(commands) + '\n'


def main():
    parser = argparse.ArgumentParser(description='Validate a saved machine profile and emit a DOSBox configuration; no machine or core is started')
    parser.add_argument('profile', type=Path)
    parser.add_argument('--games-root', type=Path, default=Path('/media/fat/games/SpinalVoodoo'))
    args = parser.parse_args()
    try:
        with args.profile.open('rb') as file:
            content = file.read(65537)
        if len(content) > 65536:
            raise ValueError('Profile exceeds 64 KiB')
        profile = json.loads(content, object_pairs_hook=unique_object)
        sys.stdout.write(render(profile, args.games_root))
    except (OSError, ValueError, TypeError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
