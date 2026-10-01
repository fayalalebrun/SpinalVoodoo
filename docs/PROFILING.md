# Stack-based CPU profiling

The maintained profiler uses upstream Linux `perf`, not manual application scopes. It captures scheduler switch-out callchains for off-CPU attribution and ordinary 100 Hz software-clock samples for on-CPU attribution. No source injection, instrumented Glide/runtime build, debugger, or FPGA modification is required.

## Capture

Start an uninstrumented target, visually confirm the workload, and select its single-thread PID. On the authorized board:

```sh
python3 diagnostics/profiling/profile-stacks-collect --pid PID --duration 3 \
  --perf /path/to/perf --call-graph dwarf,8192 --output new-capture
```

Requirements: perf events and scheduler tracepoints enabled, tracefs mounted, sufficient permissions, and an upstream perf executable with libtraceevent support. The tool does not mount filesystems or change sysctls. `--perf` may name a launcher for the private ARM loader/library directory. Collection saves `perf.data`, kernel symbols, process maps, copied mapped ELFs, identities, hashes, and record logs. PID starttime/executable/command/maps must match before and after; multithreaded targets are rejected. `RECORDED` is not a correctness or overhead signoff.

The recorder requests `cpu-clock/period=10000000/`, `sched:sched_switch` filtered by outgoing/incoming target PID, and `sched:sched_wakeup` filtered by the woken target PID. CPU sampling is system-wide; import selects only the target thread. Perf collects register/stack snapshots for later DWARF unwinding and kernel callchains. Prefer short captures: capturing stack bytes at every scheduling event can be expensive. `--call-graph fp` is available for comparison but is not reliable across frame-pointer-omitting libc/Thumb code.

## ARM user-stack unwinding

The stripped ARM libc contains EHABI tables but essentially no DWARF unwind tables. Its matching split-debug ELF contains `.debug_frame`. Without that data, both frame-pointer and ordinary DWARF decoding stopped in libc on the test workload.

Use exact matching debug artifacts, never similarly named libraries. `diagnostics/profiling/profile-stack-symbols` verifies matching ELF build IDs, attaches `.debug_frame` to an **offline symbol copy**, and checks `.text` and `.data` bytes remain unchanged. It never deploys or changes the running library:

```sh
python3 diagnostics/profiling/profile-stack-symbols --binary captured/libc.so.6 \
  --debug matching-build-id.debug --output new-symbol-libc.so.6 \
  --readelf armv7l-unknown-linux-gnueabihf-readelf \
  --objcopy armv7l-unknown-linux-gnueabihf-objcopy
```

Put the new copy in a separate symbol-root directory at the original absolute DSO path, preserving other captured ELFs at their original paths. Keep the merge command's build-ID/hash manifest. For the validated diagnostic, libc build ID is `5ece342616bec68931a809f420036fb6daff0a60`; matching data came from the existing Nix cross-glibc debug output. This is not a universal libc identity.

## Decode, import, render

Copy capture artifacts to the host. Use the existing perf in `nix develop`:

```sh
nix develop --command python3 diagnostics/profiling/profile-stacks-decode \
  --data new-capture/perf.data --symbols /absolute/symbol-root \
  --kallsyms new-capture/kallsyms --output new-capture/perf.script
python3 diagnostics/profiling/profile-game import --perf-script new-capture/perf.script \
  --tid PID --output report.json
python3 diagnostics/profiling/profile-game render report.json --output report.html
```

Decode audits `perf report --stats` for lost/throttled records and requires sample records, then runs symbolized `perf script --ns --show-lost-events`. Decoder logs and statistics remain alongside the script. Import rejects malformed or unordered records and impossible target transitions. Optional `--start`/`--end` select an interior monotonic window; defaults cover the recorded event range, not recorder startup/shutdown overhead.

JSON (`profile-stacks/1`) is the source for self-contained, escaped HTML. Start with **Overall wall time**, which combines measured scheduled, blocked, runnable and unknown durations on one scale. CPU descendants are explicitly estimated as scheduled CPU duration multiplied by each stack's fraction of CPU samples; hatched blue frames and `ESTIMATED` details distinguish these from measured durations. Missing stacks retain their sample share, and a capture with no CPU samples leaves scheduled time unattributed. The original JSON and sample-count graph remain unchanged. Every seconds graph's frame details show its percentage of full wall time, even when zoomed. Parent weights include their children: never sum vertically.

The overview supports the same search, zoom, back/reset, full-symbol details and keyboard controls as the separate category graphs:

- Blocked seconds, charged to the target's switch-out stack until wakeup.
- Runnable seconds, charged to the preceding switch-out stack until scheduled again.
- On-CPU **sample counts**, not exact elapsed seconds.

Wakeup callchains belong to the waker, not the sleeping thread; they are never substituted. Unknown initial scheduler state and missing callchains remain explicit. Scheduler state totals conserve the window; every stack-tree parent conserves its child weights. On-CPU samples are statistical and are never added to off-CPU seconds. Symbols, CFI coverage, stack truncation and JIT unwinding still require validation; a tall graph alone is not proof of correct unwinding.

## Verified evidence and limitations

On the ARM board, the uninstrumented synthetic workload in `diagnostics/profiling/tests/profile_stack_workload.c` produced 174 target CPU samples over about two seconds. With matching libc CFI, blocked stacks resolve through `main → sample_work → sample_sleep → __nanosleep → __clock_nanosleep_time64 → __internal_syscall_cancel`, with kernel frames through `do_nanosleep`; CPU samples resolve `sample_busy`. See `output/profiles/stack-dwarf-audited-v2.{json,html}`, its `.script.stats`, original data, and preserved failed decodes. This validates the capture/unwind path, not DOSBox guest/JIT stacks or game performance.

Hardware PMU interrupts on this board remain unsuitable; software-clock sampling avoids that issue. No eBPF support is assumed. The private upstream ARM perf build omits optional BPF metadata and warns about it; scheduler/software sampling works. A host perf raw `script -D` audit crashed on ARM register dumps; the maintained decoder instead uses the successful record-statistics audit.

The pipeline is also wired to the original uninstrumented DOSBox/quiet-idle runtime (the existing frame logger remains preloaded for input/completion logging). `output/profiles/tomb-stack-real-02/diagnostic.{json,html}` contains a real approximately one-second process capture with 65 target CPU samples, 0.244072s blocked and 0.135733s runnable; the perf record audit shows no lost/throttled records. Kernel nanosleep paths resolve, and 104 of 2351 sleeping switch-out stacks reach `drain_locked`. Most stop at libc: the deployed DOSBox/runtime ELFs have no `.debug_frame`, and 29 CPU samples have no recovered stack. HDMI was black, so this is explicitly diagnostic, not validated Caves gameplay. Report `--label` and repeatable `--warning` preserve those caveats in JSON and HTML.

The collector retains non-executable device mappings such as `/dev/mem` in maps but does not copy devices as symbol files. Missing regular mapped files and executable device mappings still fail closed.

Next: obtain matching DOSBox/runtime unwind data or build a controlled debug-info variant without manual probes, resolve video/scene correctness, validate JIT coverage, and measure capture-off/on overhead. Previous manual-scope/scheduler-only calibration is not applicable to stack capture. No current game FPS or precise bottleneck claim follows from these diagnostic captures.

## Naming DOSBox dynamic code

`python3 diagnostics/profiling/enable-mister-jitdump PRIVATE_BUILD_COPY` adds opt-in metadata emission to an existing private DOSBox source/build copy under `output/`. It refuses pinned sources and duplicate integration; it is deliberately separate from normal `prepare-mister-dosbox --check`. Preserve the original build and rebuild the private copy using its original compiler/flags. The retained Clang PIE diagnostic build is `output/mister/dosbox-jitdump-build`; its build command/logs are under `output/profiles/build-dosbox-jitdump.sh` and `dosbox-jitdump-build-v2.log`.

Set `MISTER_JITDUMP_DIR` to an existing absolute directory on an executable filesystem **before starting DOSBox**. With the variable absent, no dump is opened. Record with upstream `perf record --clockid mono` from process launch. Keep the complete `jit-PID.dump` alongside `perf.data`, exact binary identities and logs. Each translated block records its executable address, actual generated length, monotonic timestamp, unique generation index, guest linear start address and 16/32-bit code mode. Link stubs and dispatch have distinct names. A timestamped load record is emitted on each reuse, so old samples are not assigned the final occupant of an address as a static perf map could do. This is single-threaded dynrec metadata, not a scope profiler.

On the host, make the dump available at the exact path recorded in its executable marker mapping, using a workspace-owned path. Use a host perf with libelf/JIT support (the private minimal board perf cannot inject JIT ELF files):

```sh
nix develop --command perf inject --jit -i capture.perf -o capture-jit.perf
nix develop --command perf script -G -i capture-jit.perf -F comm,tid,time,event,ip,sym,dso
```

`-G` reports sampled instruction names even when JIT callchains cannot unwind. Keep the generated `jitted-PID-INDEX.so` files for symbolization and disassembly. **This does not add JIT unwind rules, guest function names or retroactive names to old captures.** Code bytes are emission-time snapshots; subsequent in-place link/immediate patches are not a live instruction trace. CPU-only diagnostics have no scheduler accounting and must not be imported as a complete wall-time report. Dump writes are synchronous and may materially affect translation-heavy workloads: use bounded runs and calibrate overhead before performance conclusions. File creation is exclusive/private, and metadata errors abort the diagnostic rather than silently producing incomplete evidence.

Hardware validation: ARM synthetic address reuse resolves 204 samples to generation 1 and 189 to generation 2 through upstream `perf inject`. The bounded 15-second DOSBox/Tomb launch records 5,278 loads and resolves 234 JIT samples: 68 dispatch and 166 guest blocks (`output/profiles/tomb-jitdump-01-summary.json`). It completed zero renderer frames and is **startup validation, not gameplay or an FPS comparison**. The signed FPGA and original installed DOSBox/runtime libraries were unchanged.

## Unified JIT/native wall-time reports

Capture scheduler events and native stacks with `profile-stacks-collect` while JIT-enabled DOSBox runs. Preserve the complete dump after the process stops; attaching after JIT initialization works when the executable jitdump marker is in the initial maps. Run `perf inject --jit`, then decode stacks and sampled instructions from that **same injected file**, using matching native CFI and the generated JIT ELF files in the symbol root:

```sh
perf script --ns -G -i injected.perf --symfs /absolute/symbol-root \
  --kallsyms capture/kallsyms -F comm,tid,time,event,ip,sym,dso > leaves.script
python3 diagnostics/profiling/profile-game import --perf-script stacks.script \
  --sample-leaves leaves.script --tid PID --output unified.json
python3 diagnostics/profiling/profile-game render unified.json --output unified.html
```

The importer requires every target CPU sample's PID, timestamp and event type to match exactly, rejects duplicates, and fills only empty callchains. These remain counted as missing caller stacks and appear under **Sampled instruction only — callers unavailable**. Existing native stacks and scheduler accounting are unchanged. Never combine exports from different recordings.

`output/profiles/tomb-unified-01/report.html` combines all three time categories from one 3.013820-second recording: 1.892222s CPU, 0.769527s blocked, 0.352071s runnable. All 79 empty CPU callchains receive JIT instruction names; the other 106 CPU callchains have native/JIT symbols, with no unresolved-address frames in this export. This does not establish complete callers. The launch completed renderer frames but its scene was not visually verified, and overhead remains uncalibrated.

## Removed machinery

Manual scope/wait probes, injected-build preparation, debugger control, instrumented launcher, custom ftrace collectors/parsers, and their tests were removed from maintained source at the user's request. Retired source and all historical JSON/HTML/raw evidence remain under `output/profiles/retired-manual-source` and the original evidence paths. Historical HTML remains viewable; the current importer intentionally does not support retired manual formats.

Validation: `nix develop --command python3 -m unittest discover -s diagnostics/profiling/tests -p 'test_*.py'`. Tests cover state/stack accounting, waker ownership, clipping, malformed/lost records, loss audit, identity/thread checks, matching-debug validation, conservation, and JSON-only escaped rendering.
