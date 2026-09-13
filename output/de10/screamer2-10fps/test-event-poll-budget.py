from pathlib import Path
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

root = Path(__file__).resolve().parent
helper = root / 'apply-event-poll-budget140.sh'
mock = r'''
import json,os,sys,types
from pathlib import Path
name = Path(sys.argv[0]).name
args = sys.argv[1:]
def record(value):
    with open(os.environ['TRACE'], 'a') as out:
        out.write(json.dumps(value) + '\n')
record([name, *args])
if name == 'cat':
    if args == ['/dev/shm/screamer2-50mhz/dosbox.pid']:
        print(os.environ.get('PID', '1234'))
    elif args == ['/proc/sys/kernel/random/boot_id']:
        print(os.environ.get('BOOT', '32bbda2e-4586-4ff7-b759-bff08e8cd7d7'))
    else:
        sys.exit(2)
elif name == 'grep':
    assert args == ['-Eq', '^State:[[:space:]]+T[[:space:]]', '/proc/1234/status']
    sys.exit(int(os.environ.get('BAD_STATE', '0')))
elif name == 'sha256sum':
    assert args == ['-c', '-']
    lines = sys.stdin.read().splitlines()
    assert len(lines) == 4 and all(len(line.split()[0]) == 64 for line in lines)
    sys.exit(int(os.environ.get('BAD_HASH', '0')))
elif name == 'sudo':
    assert args.pop(0) == '-n'
    if args[0] == 'readlink':
        assert args == ['readlink', '/proc/1234/exe']
        print(os.environ.get('EXE', '/dev/shm/screamer2-50mhz/dosbox-x-o3'))
    elif args[0] == 'kill':
        assert args == ['kill', '-STOP', '1234']
    else:
        assert args[:7] == ['timeout', '-s', 'INT', '-k', '5', '45', 'gdb']
        if os.environ.get('GDB_STATUS'):
            sys.exit(int(os.environ['GDB_STATUS']))
        values = {'CPU_CyclePercUsed': 100, 'CPU_CycleAutoAdjust': int(os.environ.get('AUTO', '1')),
                  'CPU_SkipCycleAutoAdjust': int(os.environ.get('SKIP', '0'))}
        def execute(command):
            assert command == 'set *(int *)&CPU_CyclePercUsed = 140'
            values['CPU_CyclePercUsed'] = 140
            record(['set', 140])
        gdb = types.SimpleNamespace(parse_and_eval=lambda expression: values[expression.split('&')[-1]], execute=execute)
        failed = False
        for i, arg in enumerate(args[:-1]):
            if arg != '-ex':
                continue
            command = args[i + 1]
            try:
                if command.startswith('python '):
                    exec(command[7:], {'gdb': gdb})
                elif command.startswith('set '):
                    execute(command)
            except Exception:
                failed = True
        sys.exit(int(failed))
else:
    sys.exit(2)
'''


class BudgetTests(unittest.TestCase):
    def run_helper(self, **overrides):
        with tempfile.TemporaryDirectory(dir=root) as directory:
            path = Path(directory)
            for name in ('cat', 'grep', 'sha256sum', 'sudo'):
                script = path / name
                script.write_text('#!' + sys.executable + '\n' + mock)
                script.chmod(0o755)
            trace = path / 'trace.jsonl'
            env = dict(os.environ, PATH=str(path) + os.pathsep + os.environ['PATH'], TRACE=str(trace), **overrides)
            result = subprocess.run([shutil.which('bash'), str(helper)], env=env, capture_output=True, text=True)
            calls = [json.loads(line) for line in trace.read_text().splitlines()]
            return result, calls

    def test_success(self):
        result, calls = self.run_helper()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(calls.count(['set', 140]), 1)
        self.assertEqual(calls[-1], ['sudo', '-n', 'kill', '-STOP', '1234'])

    def test_identity_and_state_rejection(self):
        for override in ({'PID': ''}, {'PID': '0'}, {'PID': '1'}, {'PID': '1234;bad'}, {'BOOT': 'wrong'},
                         {'EXE': '/unowned'}, {'BAD_STATE': '1'}, {'BAD_HASH': '1'}):
            with self.subTest(override=override):
                result, calls = self.run_helper(**override)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn(['set', 140], calls)
                self.assertFalse(any('gdb' in call for call in calls))

    def test_policy_rejection_and_timeout(self):
        for override in ({'AUTO': '0'}, {'SKIP': '1'}, {'GDB_STATUS': '124'}):
            with self.subTest(override=override):
                result, calls = self.run_helper(**override)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn(['set', 140], calls)
                self.assertEqual(calls[-1], ['sudo', '-n', 'kill', '-STOP', '1234'])


if __name__ == '__main__':
    unittest.main()
