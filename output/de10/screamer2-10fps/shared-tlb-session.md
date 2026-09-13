# Shared-memory / TLB follow-up

Goal achieved in two full runs:10.255450 and10.266577FPS, pooled10.261014FPS, on unchanged DE10 pure-XLS50MHz and the same180-second Arcade / England / Condor / Horizon / automatic / Up-held / no-steering workload. Both postgame replays are byte-exact. All158 staged game files and protected runtime hashes verified unchanged, accounting for the pre-existing disabled GOG-overlay filename. No FPGA or system configuration changes.

## Retained

Retained: `dosbox-x-event-poll-screening`, SHA256 `fb9db5530b4cb499aacfb3463232e2dca96d5162290ea505d2601350d8b38fc0`, with process-local `CPU_CyclePercUsed=140` after race-ready. O3, fresh matching PGO,32-byte native alignment,80-byte Glide argument copy, ARM/Linux SDL2 event polling capped at250Hz, plus earlier shared-TLB/branch/FPU work.1846/1848 swaps in180.001855/180.001568s:10.255450/10.266577FPS, pooled10.261014FPS. Both replays exact. `event-poll-budget140-confirmed-results.json` is the final evidence manifest. The separate compiler/source contributions were not isolated. The unique archived binary, default profile-use output and RAM executable all match this SHA; game/Xvfb are stopped after validation.

Previous repeat-tested baseline: `dosbox-x-align32-screening`, SHA256 `af5ff769c84d32b39cd25dc74cb4a8c13bf52d3a9b26460618e3571fceb05569`, O3/PGO2/32-byte alignment and budget140:9.977688/9.855465FPS, pooled9.916576FPS, both replays exact. `align32-budget140-confirmed-results.json`. That earlier result did not meet10FPS; its alignment benefit alone was not isolated from pacing.

Original-policy reference remains `dosbox-x-pgo2-8.489fps`, SHA256 `0dd56c05fa1254fe0b59b14e6e06af75c1ca93a842ef152d1599381b5f1eb28d`:8.488813/8.599930FPS, pooled8.544371FPS; exact replay both. New-boot control8.538803FPS also exact. Previous non-PGO control `dosbox-x-shared-8.233fps`, SHA256 `99bb04fa695fffc4d0852bed7ef71e88aa0d3904ad61a12cc9948257e1f80152` remains archived.

Shared ARM JIT byte/word/dword TLB helpers, bounded emission, prior branch/FPU optimizations. Runtime helpers check the current TLB, preserve slow handler/fault paths, and use one relative BL per access. CPU TUs core_dynrec, flags, cpu, core_normal, paging are Thumb; other TUs are ARM. The hot FPU helpers instantiated in core_dynrec are already Thumb (verified disassembly); a subagent incorrectly claimed otherwise.

| Experiment | Swaps | FPS | Decision |
|---|---:|---:|---|
| Inline dword write (memory2) | 1381 | 7.672146 | discard |
| Shared memory helpers | 1482 | 8.233265 | retain |
| Shared repeat | 1453 | 8.072157 | retain; pooled 8.152711 |
| NOP compaction | 1458 | 8.099922 | no confirmed improvement |
| Compaction + register forwarding | 1422 | 7.899932 | discard |
| Main thread CPU1, Xvfb CPU0 | 1456 | 8.088808 | no confirmed improvement |
| memory.cpp + glide.cpp Thumb | 1474 | 8.188805 | no confirmed improvement |
| Four 32-byte TLB array gaps | 1462 | 8.122 | no confirmed improvement; reverted |
| Runtime signed-branch hot guard | — | 8.172 | no confirmed improvement; reverted |
| Register forwarding without compaction | 1481 | 8.227698 | no confirmed improvement; reverted |
| Mathematical DEC signed-branch fast path | 1459 | 8.105490 | no confirmed improvement; reverted |
| PGO2 use | 1528 | 8.488813 | retain |
| PGO2 repeat | 1548 | 8.599930 | retain; pooled 8.544371 |
| r9-cached TLB base, non-PGO | 1471 | 8.172136 | no confirmed improvement; reverted |

All completed experiments' postgame framebuffer replays match `initial-replay.raw`, SHA256 `96d0cfd27ae4948d001625454722992c21407a61b4f0e7a9a658ce0f39192b45`. Each has its own results JSON, logs, binary/patch where applicable. Do not mistake within-range single-run deltas for gains.

## Experiment history

PGO3 steady-only training and build finished. Its 180-second uninstrumented run measured 1531 swaps / 8.505476 FPS, within retained PGO2 variation. The board rebooted before the PGO3 postgame replay could run: do not claim that replay passed. Binary archive `dosbox-x-pgo3-8.505fps`, SHA `f3b9c9576f2aa1b18de904c6522fbd99792f370bc8bb51150deb15dab2be2ba5`; training replay was exact. Retained binary remains PGO2.

User approved higher auto-cycle CPU-budget experiments without changing config files, game, quiet Glide, or FPGA clock. Default `CPU_CyclePercUsed=100` gives nominal90% usage; config parser caps105. Trials deliberately override this process-local variable through GDB while the owned game is stopped at race-ready.109/120/140/160 are above the UI cap; ratio multipliers above100% are controller inputs, not claims of more than100% actual host CPU utilization. No source/config-file budget edit or PGO input-module bypass.

| New-boot experiment | Swaps | FPS | CPU% | Decision |
|---|---:|---:|---:|---|
| PGO2 default100 control |1537|8.538803|88.621|reference|
| PGO2 budget109 |1645|9.138794|93.710|screening; exact replay|
| PGO2 budget120 |1722|9.566581|96.155|screening; exact replay|
| O3 align32 budget140 |1796|9.977688|98.049|retain combined variant|
| O3 align32 budget140 repeat |1774|9.855465|97.866|retain; pooled9.916576|
| O3 align32 budget160 |1768|9.822135|97.566|no improvement; exact replay|
| O3 align32 sample-density hot ordering budget140 |1791|9.949905|97.788|no demonstrated gain; exact replay|

Hot ordering relinked43 sampled functions (99,728 bytes) by samples/size; `hotorder-symbols.{txt,json}`, `hotorder-link-command.json`, `emulator-hotorder/dosbox-x` SHA `e71b8aecf00cf6c00e84b209b87603b0386d78ddf104014ce18e0e4ddcf06c82`. It is not retained.

Native-O2 attempt rejected before deployment:35 PGO CFG/hash mismatch warnings despite identical raw module. `o2-align32-rejected.json` and build log; O2 flag reverted. Changing early optimizer pipelines needs matching training, not a warning bypass.

Align16 screening completed:1790 swaps /180.001559s =9.944358FPS, exact postgame replay; no confirmed gain. `align16-budget140-results.json`. Linker alignment restored to32 bytes (`-mllvm -align-all-functions=5`).

Current source experiment: `glide.cpp` copies the80-byte parameter array with one current-TLB probe and SDL_memcpy on a little-endian same-page hit; all other cases retain MEM_BlockRead32. `test-glide-param.py` demonstrated20-probe baseline failure, then327680 differential copies per endian on host and actual ARM, including66560 partial faults; LE129024 single-probe copies. Full host/ARM/Thumb regression passed (`glide-param-full-{host,arm}-tests.log`). Subagent incorrectly claimed offset4016 was untested (it is16-aligned) and ARM tests unexecuted (board logs prove execution).

Fresh matching PGO training completed at process-local cycle budget140:513 swaps /180.002105s, instrumentation overhead means2.849967FPS is not a performance claim. Instrumented SHA `fa15cf2d0e88fdc21d94d4c2a650fc18028af03a0990331f32311b825767de5f`; snapshots `glide-param-training{,-before}.counters`; full counts exported/merged to `glide-param-training.profdata` (11664 functions,215942 blocks,3972359887 counts). Training postgame replay exact. `glide-param-baseline.tar` preserves original Glide object/archive and original training module/binary. New train/use raw module SHA `41af3d556c534497ea799cec14fdead2e3d8d8235afcdd24f970ddfcd74d4d86`; not compatible with old PGO2 module.

Uninstrumented candidate `dosbox-x-glide-param-screening`, SHA `1563c5fa418c3427369e647dfebb8e370c2692b4102579cc0da3a9cbf7924cda`; build status0, no warnings/instrumentation, matching module validated, SMC positive/negative controls passed. For this screening, `emulator-pgo-use/dosbox-x` and RAM held the Glide-copy candidate; both were subsequently replaced by the retained event-poll build. At budget140 the unchanged180s workload measured1795 swaps /180.001739s =9.972126FPS (98.1768% CPU), within retained variation and below target. `glide-param-budget140-race-180s.json`; postgame replay exact; owned PID14848/Xvfb stopped. No isolated gain was confirmed from this screening alone.

Budget130 on the Glide-copy candidate regressed:1766 swaps /180.001804s =9.811013FPS,97.0935% CPU, exact postgame replay; owned PID16519/Xvfb stopped. `glide-param-budget130-results.json`. No more pacing-only trials planned; use140 for the next code experiment.

Profile follow-up: repeated-floorl caching was rejected before editing: actual optimized FPU_FST_I32 and its fused store-pop wrapper already execute exactly one floorl call on each nearest-mode path. Subagent's claim of repeated native floorl calls was false. Native code-page handler inspection found no compelling cheap change; no edits made there.

User approved proceeding with ARM/Linux SDL2 event-poll throttling. `sdlmain.cpp` extends the existing macOS4ms/250Hz limiter to `LINUX && __arm__`; tick storage/subtraction is now unsigned32 to handle rollover. Other Linux architectures, Windows and Linux SDL1 are unchanged. Rendering/timer ticks are not skipped; keyboard-layout checks remain outside the guard and the normal event sweep drains all queued events. `GetTicks` is SDL_GetTicks wall clock; pause loops use their existing blocking waits. Current source still includes the earlier Glide80byte copy.

`test-event-poll.py` first reproduced Linux ARM's unbounded polling, then passed225 timestamps/1058 calls/225 ordered synthetic events per platform in host UBSan and actual ARM builds: ARM/Linux SDL2 and macOS cap at4ms; Linux non-ARM, Windows SDL2 and ARM/Linux SDL1 remain unthrottled. Covers repeated ticks, signed32 boundary, two unsigned rollovers, full queue drain and unchanged layout checks. `event-poll-tests-{before,fixed,arm}.log`; full current host/ARM regression also passed (`event-poll-full-{host,arm}-tests.log`). The queue is a test stub; actual game key navigation/benchmark still needed for the new full binary.

Event-poll fresh training completed. GUI baseline object/archive and prior Glide-copy training module/binary are preserved in `event-poll-baseline.tar`. Instrumented build status0, no warnings; SHA `7d8fb6b524b111f972c4f625b04464ff5a096d3ce7b206c0814c77d30d8e58ad`; actual limiter state symbol verified. SMC controls and actual game key navigation passed. PID17872 trained on the selected workload for180.002009s at budget140,554 swaps;3.077743FPS includes instrumentation and is not a performance claim. Counter snapshots `event-poll-training{,-before}.counters` (1727544 bytes) exported/merged into `event-poll-training.profdata` (11664 functions,215943 blocks,3988171639 counts). Postgame replay exact; owned game/Xvfb stopped. That instrumented RAM executable was subsequently replaced by the retained optimized binary.

Matching event-poll profile-use build completed with status0, no warnings/instrumentation and identical raw module SHA `9820b19d1218b8f82b2919bb6698c4c2d075e2f13ec25b29b017c966ab3ee671`. `event-poll-use-validation.log` and native `event-poll-use-gfx-events.disasm` verify the unsigned4ms guard before SDL_PollEvent. SMC positive/negative controls passed. PID19613 and fresh repeat PID20699 both exceeded10FPS for the full180s workload; both exact replays completed and both owned games/Xvfb were stopped. Signed image SHA remains `2ee50a4bb68fa5c2e22b145c70ca4f0e213e3aabf073bb6a02914b2720772aeb`.

`apply-event-poll-budget140.sh` makes the retained process-local policy reproducible. It rejects wrong boot/PID/executable/hash or running processes, verifies auto-adjust enabled and skip disabled before assignment in one GDB Python command, checks140 after assignment, and leaves the owned process stopped on success/failure. `test-event-poll-budget.py` covers success, eight identity/state/hash rejection cases, disabled/skip-auto policies and timeout cleanup; host final suite passed (`event-poll-final-host-tests.log`). Actual helper use passed on the repeat (`event-poll-budget140-repeat-pacing-applied.log`). ARM/Thumb full regression is `event-poll-full-arm-tests.log`.

Final158-file preservation audit passed: `event-poll-preserved-game-files-verified.json`, plus all four runtime hashes. The first audit failed because it omitted the existing `glide2x.ovl` → `glide2x.ovl.gog-disabled` mapping established by `restore.py:28`; correcting only that mapping verified every original staged file byte-for-byte. No game file was changed to satisfy the audit.

Full current-source host/ARM/Thumb regression rerun passed: `budget-layout-full-host-tests.log` and `budget-layout-full-arm-tests.log` cover flags/unary transitions, FPU/faults, emission bounds, actual shared-memory helpers, interworking, archived compaction, exporter/profile flags/subtraction/navigation. Native layout builds additionally get SMC/startup, visual workload checks, four hashes and postgame replay.

## Recovery after reboot

LAN discovery found `192.168.2.56`; SSH host key matches `debian-fpga.local`. Passive UART is FTDI AU00S3JC `/dev/ttyUSB0`,1152008N1; capture PID is recorded in `recovery-b443-serial.pid`, output `recovery-b443-serial.log`.

Signed-off image programming on boot `b44399ce-7dbb-429f-ba4b-1c59db3b6529` reached FPGA_OPERATING, then UART recorded an unexpected HPS reboot. Cause remains unconfirmed. On new boot `32bbda2e-4586-4ff7-b759-bff08e8cd7d7`, FPGA remained operating. No second image load was needed: standard bridge-reset and exact replay succeeded. `recovery-32bb-provenance.json` records both boots, unchanged image SHA and replay SHA. Do not rerun `program-ram.sh` on this operating image; its old-boot guard deliberately prevents that.

RAM game/tools/ISO/config/quietGlide/PGO2 were restored and all hashes matched. Restore must include `lib/libglide2x.so -> libglide2x.so.2`; otherwise DOSBox silently disables Glide despite the .so.2 hash passing. SMC PASS and deliberate negative FAIL passed (`recovery-32bb-validation.log`).

New-boot PGO2 control: `reboot-control-race-180s.json`,1537 swaps /180.001808s =8.538803FPS,88.6213% CPU. Same selected workload, four input hashes checked, default cycle percentage100. Postgame replay exactly matches golden (`reboot-control-postgame-replay.raw`). Owned game/Xvfb were stopped. This reproduces retained PGO2 performance on the recovered hardware.

The cached-TLB-base trial passed SMC, extended ARM/Thumb/bootstrap/return/register-preservation tests and exact replay, but measured only8.172136FPS, within non-PGO control variation. It was archived and reverted (`cached-tlb-results.json`). Active emitter matches `pgo2-retained-risc_armv4le-o3.h`; core object matches the original PGO training object hash742c9eecc77b5c15aee70a0a47ee4a2b602153e02323664bdc3070f374b2aa93; CPU archive restored from `pgo2-retained-libcpu.a`. `paging.h` is unpadded. Local `emulator-linked/dosbox-x` still contains the discarded cached-TLB screening binary; retained PGO2 lives separately. Do not reuse the screening binary accidentally.

## Completed PGO2 work

Source at training was verified byte-for-byte against `shared-emulator.patch`; `paging.h` matched `tlb-unpadded-paging.h`. All earlier padding, hot-guard, compaction, and forwarding changes were reverted. Archived latest forwarding emitter: `forward2-risc_armv4le-o3.h`.

CPU archive restoration succeeded (`pgo-baseline-restore-build.log`). First PGO link failed: `-enable-value-profiling=false` does not disable IR value profiling, leaving unresolved `__llvm_profile_instrument_{target,memop}`. New `test-pgo-flags.py` reproduces both ARM/Thumb failures using an indirect call plus variable memcpy (`pgo-flag-tests-before.log`). Linker now uses LLVM21 `-disable-vp`; both modes and all ten exporter tests pass (`pgo-flag-tests-fixed.log`). Corrected training rebuild succeeded via `link-emulator.py --pgo-generate`, status `pgo-train-fixed-build.status`, log `pgo-train-fixed-build.log`; output `emulator-pgo-train/dosbox-x`. Do not use superseded `pgo-train-build.status` (failure).

CPU compilation explicitly keeps `-mthumb` and the host's actual bash path. The normal linker path is reused by screening trials; the retained PGO2 binary is separately archived and is not whatever currently occupies `emulator-linked/dosbox-x`.

Instrumented training binary `emulator-pgo-train/dosbox-x`, SHA `ce21a18edfdf049c2b72df5f43dda5bc56ffe9ca0cd3bf4a7d84f226c0f3fa38`, passed SMC controls and actual game navigation. PGO2 trained for 180 seconds on Arcade/England/Condor/Horizon/automatic, Up held/no steering. Training produced 397 swaps (2.2055 FPS with instrumentation, not a performance claim). Owned PID 27369 was stopped and hash/boot/state checked before capturing 1,727,520 counter bytes into `pgo2-training.counters`. `pgo2-training.proftext` and `.profdata` cover 11,664 functions / 215,940 blocks; LLVM accepted 3,095,618,156 total counts. `pgo2-training-replay.raw` matches the initial framebuffer exactly. The training process was stopped after its replay; it is not the current board process.

`link-emulator.py --pgo-use pgo2-training.profdata` verifies identical uninstrumented input module hashes: `8ee2eff78af12c9bae5f11c38beb7cd48931196301a67013577a8e206a497122`. First use build succeeded but warned about missing value-profile sites; it was not deployed. Added `-disable-vp` to profile use as well as generation. Expanded `test-pgo-flags.py` proves generation/export/merge/use with an indirect call and variable memcpy in ARM and Thumb modes, with no site-mismatch warnings or instrumentation symbols in optimized objects. Corrected full build succeeded: `pgo2-use-fixed-build.status`=0, `pgo2-use-fixed-build.log`, output `emulator-pgo-use/dosbox-x`. `pgo2-use-validation.log` confirms identical input modules, no stale/value-site warnings, and no instrumentation in the measured binary.

## Tests

- Memory harness now extracts the actual PagingBlock declaration. `test-memory.py --check-tlb-padding [--thumb]`: actual 4-byte entries/array sizes and 32-byte offset spacing, 1,572,864 writes + 3,145,728 reads per mode, all offsets/high/wrapping addresses, slow handlers/faults, aliases. `tlb-padding-arm-tests.log` passes. New spacing assertion was demonstrated to fail on the unpadded layout (`tlb-layout-tests-before.log`); this is an experimental layout assertion, not a baseline correctness bug.
- 54,150,800 branch checks + 1,440,768 INC/DEC transitions per host/ARM/Thumb regression, 39,520 cache emission boundary checks, 276,480 FPU checks with 35,840 faults pass.
- New compaction/forwarding tests remain reproducible against archived emitter using `test-compaction.py` (defaults to archived header) and `test-memory.py --compact --arm-header <archive>`; neither optimization is retained.
- SMC.COM / NEG.COM exercise real emulator byte/word/dword code modification, current-block modification, equal-value and same-page data writes, and dead arithmetic slots. Separate smc.conf disables Glide; runtime result files are reset to NOTRUN before each test. Positive PASS and deliberate negative FAIL verified for completed candidates. This does not directly assert internal TLB/codepage counters.
- Build driver grouping/forced rebuild/error propagation tested in `tlb-build-driver-tests.log`.
- Bitcode-pipeline ARM PGO smoke executed on board and counters exported/merged (`pgo-bc-smoke*`); main counts 99/1 (optimized loop backedge), work counts 100/50. This smoke lacks indirect calls; the new flag regression separately covers indirect calls and variable memcpy.

## Reproduce retained result

With the validated signed image/runtime, stage the retained binary as `/dev/shm/screamer2-50mhz/dosbox-x-o3`, use `launch-o3.sh`, and navigate the same workload with `navigate.py`. Once race-ready and paused, run `ssh -o BatchMode=yes -o ConnectTimeout=10 fpga@debian-fpga.local 'bash -s' < output/de10/screamer2-10fps/apply-event-poll-budget140.sh`. This does not edit any config file; repeat it after each new game process. The helper intentionally pins the validated boot and executable hash.

Use the existing RAM `key.py Up --hold 200`, wait2s, resume the owned game, run `sudo -n python3 "$r/benchmark.py" 180 --runtime-dir "$r" --frame-offset 0x2df70`, then stop the game and wait for key release. Verify policy after measurement. Capture, stop owned game/Xvfb with `stop.sh`, bridge-reset, and replay with `--fb-base 0x3f400000 --core-clock-hz 50000000`; expected614400-byte framebuffer SHA is `96d0cfd27ae4948d001625454722992c21407a61b4f0e7a9a658ce0f39192b45`. Here `r=/dev/shm/screamer2-50mhz`. Keep the PGO module guard enabled; retained profile is `event-poll-training.profdata`. Navigation supports `--timeout 1..120` and stops on success/error; six tests pass.

SSH `fpga@debian-fpga.local` or `fpga@192.168.2.56` with `HostKeyAlias=debian-fpga.local` and strict host-key checking; current boot `32bbda2e-4586-4ff7-b759-bff08e8cd7d7`, runtime `/dev/shm/screamer2-50mhz`. Use `launch-o3.sh`, `navigate.py LABEL [KEY]`, `stop.sh`. CPU rebuild commands must include `SHELL=$(command -v bash)`; `/bin/bash` is absent on host. Long links use background status files.

The user subsequently authorized a progress checkpoint commit before pursuing17FPS. Preserve pre-existing dirty repository files; no clean/reset. No new FPGA image: unchanged signed-off 50 MHz bitstream remains loaded. Any future Quartus compile must finish within900 seconds and get full50MHz timing signoff.

## Trace bottleneck baseline before the17FPS follow-up

`traces/screamer2/{trace.bin,state.bin}` is a single-swap trace:46,338 entries,23,725 register writes,1,268 texture writes,21,465 framebuffer writes,636,378 input and612,411 output pixels. Three fresh exact replays are archived as `trace-bottleneck-baseline-{1,2,3}.log` and `.raw`.

| Run | Loop ms | Final wait ms | Total ms | Triangle-active ms |
| --- | ---: | ---: | ---: | ---: |
| 1 | 51.533 | 26.181 | 77.714 | 55.70072 |
| 2 | 49.528 | 26.091 | 75.619 | 55.69762 |
| 3 | 42.306 | 25.906 | 68.212 | 55.70500 |

Total excludes initial state loading and final readback. Triangle-active time is72–82% of total, including internal stalls/memory waits from accepted triangle command to completion. Accepted triangles do not depend on future host commands. It is not arithmetic-only time. The counter exposes64 bits but increments only low32, which wraps. Detailed stall counters are unwired in the current top; zeros do not prove no stalls.

The loop includes swap synchronization; final wait includes `traceReplayFinish` and additional status/video reads. Do not add triangle-active time to final-wait time or label the whole final wait framebuffer drain: intervals can overlap. The measured rendering cost gives an optimistic ideal-feed ceiling of about17.95FPS for this trace, not the live game.17FPS requires58.824ms total per replay. Live baseline remains10.261014FPS; clarification of the new target (trace or live game) is pending.

Current `core_sim.x` SHA256 `b3111f30a27e31dbeb0dfd844892ca5ec4f3471f13f08e6e552765c57c4e06fe` and generated core SHA256 `568761b1b87e03768d4f0b75aed3e07ca218298a797e0ee3edc983ba86025da7` match the signed-image manifest. That manifest does not pin top/counter sources, so this is not full source-to-image provenance.
