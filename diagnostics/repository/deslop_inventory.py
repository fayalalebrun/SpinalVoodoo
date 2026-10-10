import csv
import gzip
import json
import os
from collections import Counter
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
out = root / 'output/deslop-audit'
out.mkdir(parents=True, exist_ok=True)
tracked = set(subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0')) - {''}
retired = root / 'output/deslop-cleanup/deletion-manifest.json'
archived = json.loads(retired.read_text()).get('retired_source', {}) if retired.exists() else {}
migration = root / 'experiments/organization-migration/moves.json'
moves = json.loads(migration.read_text()) if migration.exists() else {}
tracked = {moves.get(name, name) for name in tracked}
texts = {}
for name in tracked:
    p = root / name
    if p.is_file() and p.stat().st_size < 500000 and not name.startswith(('build/generated/xls/rtl/', 'docs/reference/', 'output/', 'vendor/glide/')):
        try:
            texts[name] = p.read_text()
        except (UnicodeError, OSError):
            pass

cache_parts = {'.scala-build', '.bsp', '__pycache__', 'simWorkspace', 'build/test-results/replay', 'obj_dir', 'incremental_db'}
review_tools = {'de10-mmio-smoke.c', 'de10-ddr-stress.c', 'de10-trace-replay.cpp', 'trace_dump.c', 'sim_trace_capture.cpp', 'de10_bridge_fix.c'}

def classify(name, is_dir=False):
    parts = Path(name).parts
    if name in archived and not (root / name).exists():
        return 'REMOVED-ARCHIVED', 'Removed in validated cleanup; verified source archive: output/deslop-cleanup/retired-source.tar.gz'
    if '.git' in parts or name == '.git':
        return 'KEEP-METADATA', 'Git identity/history; excluded internals, never a cleanup target'
    if is_dir:
        if name in {'.learning', 'docs'}:
            return 'KEEP-DOC', 'Maintained documentation and learning navigation'
        if name in {'hardware', 'software', 'environment'}:
            return 'KEEP-SOURCE', 'Authored implementation and toolchain container'
        if name in {'workflows', 'diagnostics'}:
            return 'KEEP-TOOL', 'Maintained workflow and diagnostic container'
        if name in {'vendor', 'vendor/patches', 'vendor/patches/xls', 'vendor/stb'}:
            return 'KEEP-DEPENDENCY', 'Vendored dependencies and owned downstream patches'
        if name in {'verification', 'verification/integration'}:
            return 'KEEP-TEST', 'Verification infrastructure container'
        if name == 'verification/fixtures' or name == 'traces' or name.startswith('traces/'):
            return 'KEEP-FIXTURE', 'Preserved workload fixture container, including currently empty directories'
        if name in {'build', 'releases', 'experiments'}:
            return 'MIXED-CONTAINER', 'Maintained output/evidence boundary; classify child contents separately'
    if name.startswith('output/deslop-audit') or name.startswith('.learning/'):
        return 'KEEP-DOC', 'This audit and its reproducible inventory'
    if '__pycache__' in parts or name.endswith('.pyc'):
        return 'REMOVE-CACHE', 'Python bytecode, reproducible; not source or experimental evidence'
    if name.startswith('build/deps/'):
        return 'KEEP-DEPENDENCY', 'Pinned working dependency checkout; verify provenance and dirt before removal; current MiSTer template is clean'
    if name.startswith('build/generated/'):
        return 'GENERATED', 'Regenerated from authored hardware inputs; historical tracked RTL snapshot retained under releases/generated-baseline'
    if name.startswith('build/'):
        return 'REMOVE-CACHE', 'Normal build, simulation or packaging output; preserve deployed identities before removal'
    if name.startswith(('releases/', 'experiments/', 'output/')):
        return 'ARCHIVE-EVIDENCE', 'Release/experiment manifests, historical sources and raw evidence; preserve hashes and restoration paths'
    if any(p in cache_parts for p in parts) or name.startswith(('.devenv', '.direnv', '.playwright-cli')):
        if name.startswith('.playwright-cli'):
            return 'ARCHIVE-SESSION', 'Browser-session logs/screenshots; preserve relevant evidence then remove from active tree'
        return 'REMOVE-CACHE', 'Reproducible build/tool/test cache outside preserved experiment area'
    if is_dir and name in {'artifacts', 'docs', 'emu', 'hw', 'nix', 'output', 'scripts', 'src', 'test', 'tools', 'verification/fixtures/traces', 'xls', 'vendor/glide', 'software/runtime/mister', 'software/simulation/xls', 'software/reference', 'software/trace', 'hw/de10', 'hardware/platforms/mister', 'build/generated/xls/work', 'hardware/renderer/dslx', 'build/generated/xls/rtl', 'verification/integration/xls'}:
        return 'MIXED-CONTAINER', 'Directory boundary; child rows carry actual keep/archive/remove decisions'
    if name in {'result', '.pre-commit-config.yaml'}:
        return 'GENERATED-LINK', 'Nix/devenv-managed link; preserve hook behavior, remove result link only when unused'
    if name.startswith('vendor/glide/') and name not in tracked and not is_dir and (name.endswith(('.o', '.obj', '.lnk', '.a', '.so', '.so.2')) or Path(name).name in {'fxgasm.h', 'fxinline.h', '.de10-cross-build.sig'}):
        return 'REMOVE-CACHE', 'Glide build output; Makefile.sim creates headers/objects, preserve any deployed binary identity first'
    if name.startswith('vendor/86box'):
        return 'KEEP-DEPENDENCY', 'Pinned reference model dependency; software/reference/Makefile compiles its Voodoo sources'
    if name == 'tools/TmuTextureCacheBenchmark.scala':
        return 'REMOVE-DEAD', 'References deleted TmuTextureCache/Tmu and removed Config fields; archive history, do not port blindly'
    if name in ('docs/xls-architecture.md', 'docs/reference/software-origin.md'):
        return 'REWRITE-DOC', 'Stale native-renderer claims or context-free link; replace with current XLS flows'
    if name == 'devenv.nix':
        return 'KEEP-SUPPORT', 'Quartus environment and supported public workflow aliases'
    if name == 'Makefile':
        return 'KEEP-SUPPORT', 'Canonical XLS workflow; de10/plan now points at maintained architecture and board documentation'
    if name == '.gitignore':
        return 'KEEP-SUPPORT', 'Build/Python/browser transient rules; output ignore is not evidence that ignored source or experiments are disposable'

    if name.startswith('hardware/platforms/de10/quartus/hps_isw_handoff/') or name.endswith(('.sopcinfo', '_assignment_defaults.qdf')) or name == 'hardware/platforms/de10/quartus/hps_sdram_p0_summary.csv':
        return 'REVIEW-GENERATED', 'Quartus/Qsys generated handoff/report; prove clean regeneration and boot dependencies before untracking'
    if name.startswith('hardware/platforms/de10/quartus/') and name not in tracked:
        return 'GENERATED', 'Local Quartus results; preserve signed outputs/constraints before cache cleanup'
    if name.startswith(('vendor/glide/glide3x/', 'vendor/glide/glide2x/cvg/', 'vendor/glide/glide2x/h3/', 'vendor/glide/Device3Dfx/', 'vendor/glide/swlibs/texus/', 'vendor/glide/swlibs/texus2/')):
        return 'REVIEW-VENDOR', 'Outside inspected SST1 build lists; candidate for vendor archive, not proven safe to delete transitively'
    if name.startswith('vendor/glide/glide2x/sst1/glide/tests/') and name.endswith('.exe'):
        return 'KEEP-TEST', 'Preserved DOS guest test executable; retain until reproducible replacement is verified'
    if name.startswith('vendor/glide/'):
        if name not in tracked and not is_dir:
            return 'REVIEW-LOCAL', 'Local file within vendor tree; distinguish generated build product from patch before deletion'
        return 'KEEP-VENDOR', 'SST1 Glide/shared support/license provenance; active builds name sources here, not all historical files are required'
    if 'tests' in parts or name.startswith('verification/integration/'):
        return 'KEEP-TEST', 'Colocated unit/formal or cross-component integration regression'
    if name.startswith('diagnostics/board/'):
        if Path(name).name in review_tools:
            return 'KEEP-DIAGNOSTIC', 'Canonical replay/capture/MMIO/DDR utility; keep separate from release payload'
        return 'REVIEW-DIAGNOSTIC', 'Bring-up/probe or duplicate utility; confirm current invocation and recovery need before archive'
    if name.startswith(('workflows/', 'diagnostics/')):
        if Path(name).name.startswith(('simulate_', 'query_shared_', 'export_focused_', 'export_dut_')):
            return 'ARCHIVE-RESEARCH', 'Narrow experiment/analysis CLI rather than release flow; preserve if journal/diagnostic tooling needs it'
        return 'KEEP-TOOL', 'Build, replay, packaging, board operation or profiling entrypoint; CLI existence is not dead-code evidence'
    if name.startswith(('hardware/', 'software/', 'environment/')):
        return 'KEEP-SOURCE', 'Maintained renderer/platform/runtime/transport/replay/toolchain boundary; includes explicitly experimental variants'
    if name.startswith(('test/', 'verification/integration/xls/', 'software/reference/')):
        return 'KEEP-TEST', 'Correctness/formal/reference regression infrastructure; retain fixtures and dependencies'
    if name.startswith('verification/fixtures/traces/'):
        return 'KEEP-FIXTURE', 'Replay input/reference image/state; binary size does not make correctness fixtures disposable'
    if name.startswith('artifacts/'):
        return 'ARCHIVE-EVIDENCE', 'Historical Quartus summaries, not sufficient timing signoff on their own'
    if name.startswith(('docs/', 'hw/de10/', 'software/simulation/xls/')) or name in tracked:
        return 'KEEP-SUPPORT', 'Docs/reference/board/build/legal support; maintain unless specific review proves retirement'
    return 'REVIEW-LOCAL', 'Untracked/ignored local path: ownership or regeneration not established'

fields = ['path', 'kind', 'tracked', 'bytes', 'category', 'reason', 'direct_consumers']
counts = Counter()
size = Counter()
dirrows = []
seen = set()
with gzip.open(out / 'all-paths.csv.gz', 'wt', newline='') as f, (out / 'tracked-files.csv').open('w', newline='') as tf:
    w = csv.writer(f)
    tw = csv.writer(tf)
    w.writerow(fields)
    tw.writerow(fields)
    for base, dirs, files in os.walk(root, followlinks=False):
        dirs.sort()
        files.sort()
        relative = Path(base).relative_to(root)
        names = [(str(relative / d), 'symlink' if (Path(base) / d).is_symlink() else 'directory') for d in dirs]
        names += [(str(relative / p), 'symlink' if (Path(base) / p).is_symlink() else 'file') for p in files]
        dirs[:] = [d for d in dirs if d != '.git' and not (Path(base) / d).is_symlink() and str(relative / d) != 'output/deslop-audit']
        for name, kind in names:
            if name.startswith('output/deslop-audit'):
                continue
            p = root / name
            try:
                b = p.lstat().st_size if kind != 'directory' else 0
            except OSError:
                b = 0
            cat, why = classify(name, kind == 'directory')
            consumers = []
            if name in tracked and kind != 'directory' and name.startswith(('workflows/', 'diagnostics/', 'hardware/', 'software/')):
                key = Path(name).stem if name.endswith(('.scala', '.x')) else Path(name).name
                consumers = sorted(q for q, t in texts.items() if q != name and (name in t or key in t))
            row = [name, kind, name in tracked, b, cat, why, ';'.join(consumers[:12])]
            w.writerow(row)
            seen.add(name)
            counts[(cat, kind)] += 1
            size[cat] += b
            if name in tracked:
                tw.writerow(row)
            if kind == 'directory':
                dirrows.append(row)
    for name in sorted(tracked - seen):
        cat, why = classify(name)
        row = [name, 'missing', True, 0, cat, why, '']
        w.writerow(row)
        tw.writerow(row)
with (out / 'directories.csv').open('w', newline='') as f:
    w = csv.writer(f)
    w.writerow(fields)
    w.writerows(dirrows)
summary = {'tracked_paths': len(tracked), 'observed_paths': len(seen), 'git_internals': 'opaque KEEP-METADATA subtree; not traversed', 'symlink_targets': 'not traversed; links inventoried', 'excluded_audit_output': str(out.relative_to(root)), 'counts': {f'{cat}/{kind}': n for (cat, kind), n in sorted(counts.items())}, 'apparent_bytes_by_category': dict(size)}
(out / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary, indent=2))
