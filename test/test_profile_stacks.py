import json
from pathlib import Path
import runpy
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
M = runpy.run_path(str(ROOT / 'scripts/profile_stacks.py'))


def event(tid, time, event, payload='', stack='leaf'):
    return f'game {tid} {time:.9f}: {event}: {payload}\n\t1234 {stack} (/game)\n\n'


def fixture():
    return (event(9, 0, 'sched:sched_switch', 'other:9 [120] R ==> game:42 [120]') +
            event(42, 1, 'cpu-clock/period=10000000/', stack='busy') +
            event(42, 2, 'sched:sched_switch', 'game:42 [120] S ==> other:9 [120]', 'sleep_site') +
            event(9, 5, 'sched:sched_wakeup', 'game:42 [120] CPU:000', 'waker_NOT_sleeper') +
            event(9, 6, 'sched:sched_switch', 'other:9 [120] R ==> game:42 [120]') +
            event(42, 8, 'sched:sched_switch', 'game:42 [120] R ==> other:9 [120]', 'preempted') +
            event(9, 10, 'sched:sched_switch', 'other:9 [120] R ==> game:42 [120]'))


class StackProfileTest(unittest.TestCase):
    def test_states_stacks_and_units(self):
        r = M['analyze'](M['parse'](fixture()), 42, 0, 10)
        self.assertEqual(r['states'], dict(on_cpu=4, blocked=3, runnable=3, unknown=0))
        self.assertEqual(r['sample_count'], 1)
        self.assertIn('sleep_site', json.dumps(r['hierarchies'][0]))
        self.assertNotIn('waker_NOT_sleeper', json.dumps(r['hierarchies']))
        self.assertEqual(r['hierarchies'][2]['unit'], 'samples')
        def check(node):
            if node['children']:
                self.assertAlmostEqual(sum(c['value'] for c in node['children']), node['value'])
                for c in node['children']: check(c)
        for tree in r['hierarchies']: check(tree)

    def test_boundary_clipping(self):
        r = M['analyze'](M['parse'](fixture()), 42, 3, 5.5)
        self.assertEqual(r['states'], dict(on_cpu=0, blocked=2, runnable=.5, unknown=0))

    def test_errors(self):
        for text in ('', fixture() + 'PERF_RECORD_LOST 10\n', fixture() + 'unsupported\n',
                     fixture().replace('6.000000000', '1.000000000')):
            with self.assertRaises(ValueError): M['parse'](text)
        for text in (fixture().replace('game:42 [120] S', 'game:42 [120] BAD'),
                     fixture().replace('game 42 2.000000000', 'game 9 2.000000000')):
            with self.assertRaises(ValueError): M['analyze'](M['parse'](text), 42, 0, 10)
        with self.assertRaises(ValueError): M['analyze'](M['parse'](fixture()), 42, 0, 11)

    def test_json_only_and_escaping(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)
            (p / 'script').write_text(fixture().replace('sleep_site', '<script>alert(1)</script>'))
            subprocess.run(['python3', str(ROOT / 'scripts/profile-game'), 'import', '--perf-script', str(p / 'script'),
                            '--tid', '42', '--output', str(p / 'json')], check=True)
            (p / 'script').unlink()
            subprocess.run(['python3', str(ROOT / 'scripts/profile-game'), 'render', str(p / 'json'),
                            '--output', str(p / 'html')], check=True)
            page = (p / 'html').read_text()
            self.assertNotIn('<script>', page)
            self.assertIn('&lt;script&gt;', page)

    def test_perf_loss_audit(self):
        from unittest.mock import patch
        decode = runpy.run_path(str(ROOT / 'scripts/profile-stacks-decode'))['decode']
        for record in ('SAMPLE', 'LOST', 'THROTTLE'):
            with tempfile.TemporaryDirectory() as tmp:
                p = Path(tmp)
                stats = 'Aggregated stats:\n SAMPLE events: 1\n' + record + ' events: 1\n'
                def export(*args, **kwargs): kwargs['stdout'].write(fixture())
                with patch('subprocess.check_output', return_value=stats), patch('subprocess.run', side_effect=export):
                    if record == 'SAMPLE':
                        decode('perf', p / 'data', p, p / 'kallsyms', p / 'script')
                        self.assertEqual((p / 'script').read_text(), fixture())
                    else:
                        with self.assertRaises(ValueError): decode('perf', p / 'data', p, p / 'kallsyms', p / 'script')
                        self.assertFalse((p / 'script').exists())

    def test_observer_identity_and_thread_gate(self):
        import os
        observe = runpy.run_path(str(ROOT / 'scripts/profile-stacks-collect'))['observe']
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            base = root / '42'
            base.mkdir()
            fields = ['0'] * 20
            fields[17], fields[19] = '1', '123'
            (base / 'stat').write_text('42 (name with ) parens) ' + ' '.join(fields))
            (base / 'maps').write_text('maps')
            (base / 'cmdline').write_bytes(b'game\0arg\0')
            os.symlink('/game', base / 'exe')
            self.assertEqual(observe(root, 42)['starttime'], 123)
            fields[17] = '2'
            (base / 'stat').write_text('42 (game) ' + ' '.join(fields))
            with self.assertRaises(ValueError): observe(root, 42)

    def test_device_mappings_not_copied(self):
        mapped = runpy.run_path(str(ROOT / 'scripts/profile-stacks-collect'))['mapped_files']
        self.assertEqual(mapped('1000-2000 rw-s 0000 00:00 1 /dev/null'), set())
        with self.assertRaises(ValueError): mapped('1000-2000 r-xs 0000 00:00 1 /dev/null')
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'library'
            path.write_bytes(b'library')
            self.assertEqual(mapped(f'1000-2000 r-xp 0000 00:00 1 {path}'), {str(path)})
            path.unlink()
            with self.assertRaises(OSError): mapped(f'1000-2000 r-xp 0000 00:00 1 {path}')

    def test_report_labels_and_caveats(self):
        report = M['analyze'](M['parse'](fixture()), 42, 0, 10)
        report['label'] = '<game>'
        report['warnings'].append('Scene not verified')
        page = runpy.run_path(str(ROOT / 'scripts/profile-game'))['render'](report)
        self.assertIn('&lt;game&gt;', page)
        self.assertIn('Scene not verified', page)
        self.assertIn('missing sample stacks: 0', page)

    def test_interactive_report_and_invalid_weights(self):
        render = runpy.run_path(str(ROOT / 'scripts/profile-game'))['render']
        report = M['analyze'](M['parse'](fixture()), 42, 0, 10)
        page = render(report)
        for text in ('Reset zoom', 'Find symbol', 'aria-live', 'No observations', 'type="module"'):
            self.assertIn(text, page)
        for weight in (-1, float('nan'), float('inf'), True):
            report['hierarchies'][0]['value'] = weight
            with self.assertRaises(ValueError): render(report)
        report = M['analyze'](M['parse'](fixture()), 42, 0, 10)
        report['hierarchies'][0]['value'] += 1
        with self.assertRaises(ValueError): render(report)

    def test_wall_time_overview(self):
        overview = runpy.run_path(str(ROOT / 'scripts/profile-game'))['wall_time_overview']
        report = M['analyze'](M['parse'](fixture()), 42, 0, 10)
        original = json.dumps(report)
        tree = overview(report)
        self.assertEqual(tree['value'], 10)
        self.assertEqual([c['value'] for c in tree['children']], [4, 3, 3, 0])
        cpu = tree['children'][0]['children'][0]
        self.assertEqual(cpu['value'], 4)
        self.assertTrue(cpu['estimated'])
        self.assertEqual(json.dumps(report), original)
        def check(node):
            if node['children']:
                self.assertAlmostEqual(sum(c['value'] for c in node['children']), node['value'])
                for child in node['children']: check(child)
        check(tree)
        report['hierarchies'][2]['children'][0]['label'] = '[missing sample stack]'
        self.assertIn('[missing sample stack]', json.dumps(overview(report)))
        report['sample_count'] = 0
        report['hierarchies'][2].update(value=0, children=[])
        tree = overview(report)
        check(tree)
        self.assertIn('attribution unavailable', json.dumps(tree))
        report['states']['unknown'] = 1
        with self.assertRaises(ValueError): overview(report)
        report['window']['end'] = 11
        self.assertEqual(overview(report)['children'][3]['value'], 1)
        report['states']['unknown'] = 0
        report['window']['end'] = 10
        report['hierarchies'][0]['value'] = 2
        with self.assertRaises(ValueError): overview(report)

    def test_sample_leaf_fallback_preserves_missing_callers(self):
        text = fixture().replace('\t1234 busy (/game)\n', '')
        leaf = 'game 42 1.000000000: cpu-clock/period=10000000/: 1234 dynrec-dispatch (/jit)\n'
        events = M['parse'](text)
        M['add_sample_leaves'](events, leaf, 42)
        report = M['analyze'](events, 42, 0, 10)
        self.assertEqual(report['sample_count'], 1)
        self.assertEqual(report['missing_sample_stacks'], 1)
        self.assertIn('dynrec-dispatch', json.dumps(report['hierarchies'][2]))
        self.assertIn('callers unavailable', json.dumps(report['hierarchies'][2]))
        self.assertEqual(report['states'], dict(on_cpu=4, blocked=3, runnable=3, unknown=0))
        events = M['parse'](fixture())
        M['add_sample_leaves'](events, leaf, 42)
        self.assertEqual(events[1]['stack'], ['busy (/game)'])
        for bad in ('', leaf + leaf, leaf.replace('1.000000000', '1.000000001'),
                    leaf.replace('1234 dynrec-dispatch (/jit)', 'invalid'), leaf + 'PERF_RECORD_LOST 1'):
            with self.subTest(bad=bad):
                with self.assertRaises(ValueError): M['add_sample_leaves'](M['parse'](text), bad, 42)

    def test_matching_debug_frame_merge(self):
        merge = runpy.run_path(str(ROOT / 'scripts/profile-stack-symbols'))['merge']
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)
            (p / 'source.c').write_text('int x=1; int main(void){return x;}\n')
            subprocess.run(['cc', '-g', '-fno-asynchronous-unwind-tables', '-fno-unwind-tables',
                            '-Wl,--build-id', str(p / 'source.c'), '-o', str(p / 'full')], check=True)
            subprocess.run(['objcopy', '--only-keep-debug', str(p / 'full'), str(p / 'debug')], check=True)
            subprocess.run(['objcopy', '--strip-debug', str(p / 'full'), str(p / 'binary')], check=True)
            before = (p / 'binary').read_bytes()
            result = merge(p / 'binary', p / 'debug', p / 'symbols')
            self.assertTrue(result['build_id'])
            self.assertEqual((p / 'binary').read_bytes(), before)
            with self.assertRaises(ValueError): merge(p / 'binary', p / 'debug', p / 'symbols')
            (p / 'source.c').write_text('int main(void){return 7;}\n')
            subprocess.run(['cc', '-g', '-Wl,--build-id', str(p / 'source.c'), '-o', str(p / 'different')], check=True)
            with self.assertRaises(ValueError): merge(p / 'binary', p / 'different', p / 'wrong')

    def test_capture_uses_stacks_and_outgoing_filter(self):
        command = runpy.run_path(str(ROOT / 'scripts/profile-stacks-collect'))['command']('perf', 42, 3, Path('/tmp/new'))
        self.assertIn('--call-graph', command)
        self.assertIn('cpu-clock/period=10000000/', command)
        self.assertIn('prev_pid == 42 || next_pid == 42', command)
        self.assertIn('pid == 42', command)
