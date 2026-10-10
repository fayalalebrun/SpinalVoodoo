#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shlex
import subprocess
import sys


def install(runtime, config, core, scripts, startup):
    runtime = runtime.resolve(strict=True)
    config = config.absolute()
    core = core.resolve(strict=True)
    subprocess.run(['sha256sum', '-c', 'SHA256SUMS'], cwd=runtime, check=True, stdout=subprocess.DEVNULL)
    manifest = json.loads((runtime / 'main_control_manifest.json').read_text())
    for path, expected in ((Path('/media/fat/MiSTer'), manifest['sha256']),
                           (core, manifest['core_sha256']), (Path('/media/fat/menu.rbf'), manifest['menu_sha256'])):
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError('Unsupported Main/core/menu identity: ' + str(path))
    if not re.fullmatch(r'[A-Za-z0-9_./-]+', str(scripts)):
        raise ValueError('Native Main script paths must not contain spaces or shell metacharacters')
    if os.geteuid() != 0:
        raise ValueError('Board root privileges required')
    if any(path.exists() or path.is_symlink() for path in (config, scripts, startup)):
        raise ValueError('Configuration, script directory and startup hook must be fresh; existing files are never overwritten')
    config.mkdir(parents=True)
    (config / 'profiles').mkdir()
    (config / 'games').mkdir()
    (config / 'selected').write_text('default\n')
    profiles = {'default': {'version': 1, 'cpu': 'normal', 'cycles': 'auto', 'memory_mb': 16, 'audio': 'off', 'drives': {}},
                'dynamic32': {'version': 1, 'cpu': 'dynamic', 'cycles': 'max', 'memory_mb': 32, 'audio': 'off', 'drives': {}}}
    scripts.mkdir(parents=True)
    for name, profile in profiles.items():
        (config / 'profiles' / (name + '.json')).write_text(json.dumps(profile, indent=2) + '\n')
        script = scripts / (name + '.sh')
        selected = shlex.quote(str(config / 'selected'))
        script.write_text('#!/bin/sh\nset -eu\nprintf "%s\\n" ' + shlex.quote(name) + ' > ' + selected + '.tmp\n'
                          'mv ' + selected + '.tmp ' + selected + '\nprintf "%s\\n" "Selected machine profile: ' + name + '"\n')
        script.chmod(0o755)
    configure = scripts / 'configure.sh'
    configure.write_text('#!/bin/sh\nset -eu\nexec ' + shlex.join([sys.executable, '-B',
                         str(runtime / 'computer_configure.py'), '--config', str(config)]) + '\n')
    configure.chmod(0o755)
    command = [sys.executable, '-B', str(runtime / 'computer_service.py'), '--runtime', str(runtime),
               '--manifest', str(runtime / 'main_control_manifest.json'), '--approved-core', str(core),
               '--profiles', str(config / 'profiles'), '--selection', str(config / 'selected'),
               '--games', str(config / 'games')]
    launch = config / 'start-computer.sh'
    launch.write_text('#!/bin/sh\nset -eu\ncd ' + shlex.quote(str(runtime)) + '\n'
                      'sha256sum -c SHA256SUMS > /dev/null\n'
                      'exec ' + shlex.join(command) + ' "$@"\n')
    launch.chmod(0o755)
    start = 'nohup ' + shlex.quote(str(launch)) + ' >> ' + shlex.quote(str(config / 'service.log')) + ' 2>&1 < /dev/null &\n'
    stop = shlex.join(command + ['--stop']) + '\n'
    startup.write_text('#!/bin/sh\nset -eu\ncase "${1:-start}" in\n'
                       'start) ' + start + ';;\nstop) ' + stop + ';;\nrestart) ' + stop + start +
                       ';;\n*) exit 2;;\nesac\n')
    startup.chmod(0o755)
    (config / 'installation.json').write_text(json.dumps({'runtime': str(runtime), 'core': str(core),
        'scripts': str(scripts), 'startup': str(startup), 'manifest': manifest}, indent=2) + '\n')
    print('Installed automatic computer service; start-computer.sh starts it now, user-startup.sh starts it after boot.')


def main():
    parser = argparse.ArgumentParser(description='Install exact-version automatic MiSTer computer without changing Main or kernel')
    parser.add_argument('--config', type=Path, default=Path('/media/fat/spinalvoodoo-computer-config'))
    parser.add_argument('--core', type=Path, default=Path('/media/fat/SpinalVoodoo_game_fixes.rbf'))
    parser.add_argument('--scripts', type=Path, default=Path('/media/fat/Scripts/SpinalVoodoo_Computer'))
    parser.add_argument('--startup', type=Path, default=Path('/media/fat/linux/user-startup.sh'))
    args = parser.parse_args()
    try:
        install(Path(__file__).resolve().parent, args.config, args.core, args.scripts, args.startup)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
