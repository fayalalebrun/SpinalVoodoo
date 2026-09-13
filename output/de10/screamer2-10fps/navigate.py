import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('label')
parser.add_argument('key', nargs='?', choices=['Return', 'Up', 'Down', 'Escape', 'Right', 'Left', 'space'])
parser.add_argument('--timeout', type=int, default=25)
args = parser.parse_args()
if not args.label.replace('-', '').isalnum():
    parser.error('label must contain only letters, digits and hyphens')
if not 1 <= args.timeout <= 120:
    parser.error('timeout must be between 1 and 120 seconds')
root = Path('output/de10/screamer2-10fps')
ssh = ['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=10', 'fpga@debian-fpga.local']
command = '''set -e
r=/dev/shm/screamer2-50mhz
p=$(cat "$r/dosbox.pid")
test "$(cat /proc/sys/kernel/random/boot_id)" = "$(cat "$r/programmed.boot-id")"
test "$(sudo -n readlink /proc/"$p"/exe)" = "$r/dosbox-x-o3"
trap 'sudo -n kill -STOP "$p"' EXIT
'''
if args.key:
    command += 'python3 "$r/key.py" ' + args.key + ' --hold 1 >&2 &\n'
command += f'''sudo -n kill -CONT "$p"
sleep 2
sudo -n timeout -s INT -k 5 {args.timeout} gdb -nx -q -batch -p "$p" -x "$r/capture-pair.gdb" >&2
sudo -n kill -STOP "$p"
trap - EXIT
cat "$r/paused-pair.raw"
'''
with (root / (args.label + '.raw')).open('wb') as raw, (root / (args.label + '.log')).open('w') as log:
    result = subprocess.run(ssh + ['bash -s'], input=command.encode(), stdout=raw, stderr=log)
    result.check_returncode()
subprocess.run(['python3', 'scripts/rgb565_to_png.py', str(root / (args.label + '.raw')), str(root / (args.label + '.png')), '640', '960', '1280'], check=True)
