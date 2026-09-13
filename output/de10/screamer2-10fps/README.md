# Screamer 2: retained 10.261 FPS checkpoint

This directory deliberately versions only the retained emulator patch, small build/test/runtime helpers, and text evidence. Unrelated dirty FPGA/Glide files, downloaded source trees, game assets, sysroot, binaries, PGO data, screenshots and framebuffer captures remain outside this commit. This is a checkpoint of the existing workspace, not a self-contained clean-clone build.

## Result

Two unchanged 180-second Arcade / England / Condor / Horizon / automatic / Up-held, no-steering runs produced 1846 and 1848 swaps: **10.26101354 pooled live FPS**. Both postgame framebuffer replays were byte-exact on the unchanged signed-off pure-XLS 50 MHz FPGA image. `event-poll-budget140-confirmed-results.json` records the measurements and protected hashes.

The retained combination is shared-TLB/branch/FPU changes, O3 with fresh matching PGO, 32-byte function alignment, the fixed 80-byte Glide copy fast path, ARM/Linux SDL2 event polling limited to 250 Hz, and process-local auto-cycle percentage 140 after race-ready. Individual contributions were not isolated.

## Source and build prerequisites

`retained-emulator.patch` supersedes the older experimental patches for this checkpoint. It contains all eight differences found when comparing 1392 source/header/template files in `src/` and `include/` between local `emulator/dosboxx-src` and retained `emulator-o3`. `checkpoint-source-manifest.json` pins baseline/result file hashes, identical configuration hashes, and external artifact sizes/hashes. The patch was applied without fuzz or offsets to a separate copy of the baseline files; all eight outputs matched the retained tree byte-for-byte. Do not apply it again to the already-patched retained tree.

The baseline identifies itself as DOSBox-X 2025.10.07. Its upstream commit has not been independently established. The locally configured baseline, not an assumed pristine upstream checkout, is the patch prerequisite. Its recorded configure invocation was:

```sh
./configure --enable-sdl2 --disable-sdl2test --disable-sdlnet --disable-alsatest --disable-mt32 --disable-libfluidsynth --disable-avcodec --disable-dynamic-x86 --disable-fpu-x86 --disable-fpu-x64
```

Build helpers retain their existing paths and environment requirements:

- `cross-cc.py`: configured `emulator-o3`, `../sysroot` with ARM GCC 12 libraries, `../toolchain-bin/ld.lld`, and its pinned Nix Clang 21.1.7 path. CPU translation units must explicitly retain Thumb mode. Use the host's actual `SHELL="$(command -v bash)"` for make.
- `link-emulator.py`: existing LLVM-bitcode objects/archives and generated `emulator-o3/src/Makefile`, plus pinned LLVM 21.1.7. `--pgo-generate` and `--pgo-use event-poll-training.profdata` both disable value profiling; profile use rejects a different raw input module. Any source change requires fresh matching training, not stale-profile reuse.
- `export-pgo.py`: exports captured counters from the instrumented ELF for LLVM profile merging. Retained training used the full counters, not startup-subtracted counters. The profile, raw modules and executable are preserved locally and hash-pinned, not committed.

## Validation

Run from the repository root with the existing configured source trees and toolchain:

```sh
nix develop --command bash -lc '
  set -euo pipefail
  cd output/de10/screamer2-10fps
  for test in test-flags.py test-fpu.py test-cache-bounds.py test-export-pgo.py test-pgo-flags.py test-navigate.py test-glide-param.py test-event-poll.py test-event-poll-budget.py; do
    python3 "$test"
  done
  python3 test-memory.py
  python3 test-memory.py --thumb
  bash -n apply-event-poll-budget140.sh launch-o3.sh stop.sh
'
```

`test-flags.py` must precede `test-fpu.py`, which uses its generated stub headers. The memory commands cross-compile ARM/Thumb harnesses; they do not run them. `checkpoint-host-validation.log` records this checkpoint's successful host tests and cross-compilation. Earlier actual ARM/Thumb runs and emulator self-modifying-code positive/negative controls are recorded in `event-poll-full-arm-tests.log` and `event-poll-use-smc-test.log`. `smc.asm` and its isolated `smc.conf` are retained; binaries and discarded experiment fixtures are not.

The scoped Git attribute preserves patch context/removal whitespace as data. Added source lines are checked separately; the patch round-trip must remain byte-exact. Other checkpoint files use normal Git whitespace checks.

## Live runtime and next target

`shared-tlb-session.md` contains workload navigation, policy application, measurement, cleanup and replay procedures, plus rejected experiments. Runtime helpers require the already-staged RAM environment, its owned-process files, capture helpers, `key.py`, `benchmark.py`, configuration, game/ISO, quiet Glide library, and replay/bridge tools; they are not an installer. The policy helper intentionally pins the validated boot and executable. Do not bypass its identity checks or rerun the old image-programming/restoration scripts on the working board.

Three additional single-swap trace replays took **77.714, 75.619 and 68.212 ms**, with approximately **55.70 ms triangle-active time** each. Their logs are retained. That is trace timing, not live FPS; the optimistic ideal-feed trace ceiling is about 17.95 FPS before other costs. Triangle time includes memory/internal stalls; final-wait and triangle-active intervals must not be added as disjoint costs. Clarification of whether the next 17 FPS target means trace replay or live gameplay is pending. No new optimization or FPGA compile was started for that target in this checkpoint.
