import collections
import math
import re

HEADER = re.compile(r'^.*?\s+(\d+)\s+(\d+\.\d+):\s+(cpu-clock(?:/period=\d+/)?|task-clock(?:/period=\d+/)?|sched:sched_switch|sched:sched_wakeup):\s*(.*)$')
FRAME = re.compile(r'^\s+([0-9a-fA-F]+)\s+(.+)\s+\((.*)\)$')
SWITCH = re.compile(r'^.*:(\d+)\s+\[\d+\]\s+(\S+)\s+==>\s+.*:(\d+)\s+\[\d+\]$')
WAKE = re.compile(r'^.*:(\d+)\s+\[\d+\].*CPU:\d+$')


def parse(text):
    events, current = [], None
    for line in text.splitlines():
        if re.search(r'PERF_RECORD_(LOST|THROTTLE)|LOST .*EVENT|lost \d+ event', line, re.I):
            raise ValueError('lost/throttled samples invalidate capture')
        if not line.strip() or line.startswith('#'):
            continue
        match = HEADER.match(line)
        if match:
            tid, stamp, kind, payload = match.groups()
            kind = kind.split('/')[0]
            current = dict(tid=int(tid), time=float(stamp), kind=kind, payload=payload, stack=[])
            events.append(current)
            continue
        frame = FRAME.match(line)
        if frame and current is not None:
            address, symbol, dso = frame.groups()
            label = symbol if symbol != '[unknown]' else '0x' + address
            current['stack'].append(f'{label} ({dso})')
            continue
        raise ValueError('unsupported perf script line: ' + line[:160])
    if not events:
        raise ValueError('no perf events')
    if any(b['time'] < a['time'] for a, b in zip(events, events[1:])):
        raise ValueError('perf events are not time ordered')
    return events


def add_sample_leaves(events, text, tid):
    leaves = {}
    for line in text.splitlines():
        if re.search(r'PERF_RECORD_(LOST|THROTTLE)|lost \d+ event', line, re.I):
            raise ValueError('lost/throttled leaf export')
        match = HEADER.match(line)
        if not match or int(match[1]) != tid or not match[3].startswith(('cpu-clock', 'task-clock')):
            continue
        frame = FRAME.match(' ' + match[4])
        if not frame:
            raise ValueError('invalid sampled instruction')
        key = (int(match[1]), float(match[2]), match[3].split('/')[0])
        if key in leaves:
            raise ValueError('duplicate sampled instruction identity')
        address, symbol, dso = frame.groups()
        leaves[key] = f'{symbol if symbol != "[unknown]" else "0x" + address} ({dso})'
    samples = [e for e in events if e['tid'] == tid and e['kind'] in ('cpu-clock', 'task-clock')]
    keys = [(e['tid'], e['time'], e['kind']) for e in samples]
    if len(set(keys)) != len(keys) or set(keys) != set(leaves):
        raise ValueError('leaf export must match every target sample identity exactly')
    for event, key in zip(samples, keys):
        if not event['stack']:
            event['sample_leaf'] = leaves[key]


def tree(label, rows, unit):
    root = dict(label=label, value=0., children={})
    for stack, weight in sorted(rows.items()):
        node = root
        node['value'] += weight
        for name in stack:
            node = node['children'].setdefault(name, dict(label=name, value=0., children={}))
            node['value'] += weight
        node['children'].setdefault('[self]', dict(label='[self]', value=0., children={}))['value'] += weight
    def finish(node):
        node['children'] = [finish(child) for child in node['children'].values()]
        node['unit'] = unit
        return node
    return finish(root)


def analyze(events, tid, start, end):
    if tid <= 0 or not all(math.isfinite(v) for v in (start, end)) or not 0 <= start < end:
        raise ValueError('invalid thread/window')
    if events[0]['time'] > start or events[-1]['time'] < end:
        raise ValueError('events do not cover requested window')
    states = dict(on_cpu=0., blocked=0., runnable=0., unknown=0.)
    stacks = {k: collections.defaultdict(float) for k in ('blocked', 'runnable', 'samples')}
    state, cursor, outgoing = 'unknown', start, ('[missing switch-out stack]',)
    samples = missing = leaf_only = 0
    def charge(until):
        nonlocal cursor
        stop = min(end, max(start, until))
        delta = stop - cursor
        states[state] += delta
        if delta and state in ('blocked', 'runnable'):
            stacks[state][outgoing] += delta
        cursor = stop
    for event in events:
        stamp, kind = event['time'], event['kind']
        if kind in ('cpu-clock', 'task-clock'):
            if event['tid'] == tid and start <= stamp < end:
                stack = tuple(reversed(event['stack']))
                if not stack:
                    stack = (('[Sampled instruction only — callers unavailable]', event['sample_leaf'])
                             if 'sample_leaf' in event else ('[missing sample stack]',))
                stacks['samples'][stack] += 1
                samples += 1
                missing += not bool(event['stack'])
                leaf_only += not bool(event['stack']) and 'sample_leaf' in event
            continue
        match = (SWITCH if kind == 'sched:sched_switch' else WAKE).match(event['payload'])
        if not match:
            raise ValueError('unsupported scheduler payload: ' + event['payload'])
        if kind == 'sched:sched_switch':
            prev, status, nxt = match.groups()
            prev, nxt = int(prev), int(nxt)
            if tid not in (prev, nxt):
                continue
            if prev == nxt:
                raise ValueError('invalid self switch')
            charge(stamp)
            if prev == tid:
                if event['tid'] != tid:
                    raise ValueError('switch-out callchain belongs to a different thread')
                if state not in ('unknown', 'on_cpu'):
                    raise ValueError('missing switch-in')
                if status not in ('R', 'R+', 'S', 'D', 'I', 'T', 't', 'X', 'Z', 'P'):
                    raise ValueError('unsupported task state')
                outgoing = tuple(reversed(event['stack'])) or ('[missing switch-out stack]',)
                state = 'runnable' if status.startswith('R') else 'blocked'
            else:
                if state == 'on_cpu':
                    raise ValueError('missing switch-out')
                state = 'on_cpu'
        elif int(match[1]) == tid:
            charge(stamp)
            if state in ('unknown', 'blocked'):
                state = 'runnable'
    charge(end)
    if not math.isclose(sum(states.values()), end - start, rel_tol=0, abs_tol=1e-8):
        raise ValueError('nonconserving scheduler accounting')
    return dict(schema='profile-stacks/1', tid=tid, window=dict(start=start, end=end),
                states=states, sample_count=samples, missing_sample_stacks=missing, leaf_only_samples=leaf_only,
                hierarchies=[tree('Blocked time by switch-out stack', stacks['blocked'], 'seconds'),
                             tree('Runqueue delay by switch-out stack', stacks['runnable'], 'seconds'),
                             tree('On-CPU samples (not elapsed time)', stacks['samples'], 'samples')],
                warnings=['Sample counts are statistical, not exact CPU seconds.',
                          'Wakeup stacks belong to the waker and are never used as the blocked thread stack.',
                          'Unknown symbols, truncated/frame-pointer/JIT unwinds remain explicit; stack depth alone does not prove unwind correctness.',
                          'Capture overhead and workload correctness require a matched off/on comparison.'])
