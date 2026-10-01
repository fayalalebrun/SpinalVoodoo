# Agent Instructions

## Experiment journal and change ownership

- Before continuing MiSTer performance work, read [the Screamer 2 experiment journal](docs/SCREAMER2_EXPERIMENT_JOURNAL.md). Update it after every experiment, deployment, recovery, rejected approach, or change to the next step.
- Record measured evidence, candidate/control identities, changed files, validation results, decisions, and unfinished work. Preserve failed runs and signed candidates.
- Existing dirty files can be our own earlier edits. The rejected `sys/ascal.vhd` patch was assistant-owned work, confirmed by the user; it is now archived and the pinned framework checkout is clean. Inspect the journal and relevant nested diffs before attributing changes or asking for approval.
- Do not classify our own unfinished integration or a source-preparation guard refusal as an external permission blocker. If provenance is unknown, investigate and state the uncertainty instead of assigning ownership to the user.

## Profiling before optimization

- Profile the actual target workload before implementing any optimization. Do not implement speculative optimizations: proceed only when measured evidence establishes the bottleneck and gives near-complete confidence that the proposed change will improve the target metric without compromising correctness.
- Distinguish CPU execution from blocked time, attribute significant costs to concrete code paths, and account for profiling overhead. Aggregate samples, suspicious timing patterns, and subagent hypotheses alone are not sufficient evidence.
- If attribution or expected benefit remains uncertain, gather better measurements rather than change the implementation. Record the evidence and predicted benefit before proceeding, then verify the result with controlled, matching-workload before/after measurements and correctness tests.

## FPGA board authorization

- The user grants standing authorization to operate and modify the FPGA board without further approval prompts, including diagnostic kernel/driver preparation, loading and booting, CTI/PMU configuration, FPGA programming, and recovery operations needed for the performance goal.
- This authorization is board-specific; it does not authorize changes to unrelated host systems. Preserve recoverable backups and evidence, verify deployed identities, and retain a working recovery path before risky changes.
- Diagnostic authorization does not waive correctness checks, profiling requirements, final FPGA timing signoff, or the 900-second complete compile limit.

## Nix environments

- Use `nix develop` for the normal XLS, simulation, lint, and replay toolchain.
- `devenv shell` provides Quartus through `devenv.nix`, while the flake dev shell provides XLS tools but not Quartus.
- Commands needing both toolchains must nest them: `nix develop --command devenv shell -- bash -lc '<command>'`.
- Do not report Quartus as unavailable after checking only `nix develop`. Verify both toolchains with `nix develop --command devenv shell -- bash -lc 'command -v quartus_sh && command -v ir_converter_main'`.

## DE10 XLS FPGA flow

- Build and fit the XLS bitstream with `nix develop --command devenv shell -- make de10/xls/bitstream`.
- The build runs the complete Quartus compile flow for `SpinalVoodoo_de10_xls` and writes outputs under `output/de10/xls-bitstream` by default.
- A complete Quartus compile plus required timing signoff must take no more than 15 minutes. If it exceeds 15 minutes, stop; the design is too large and must be optimized for size before rerunning Quartus. Do not split, extend, or repeatedly retry an over-budget flow.
- Run standard STA with `quartus_sta SpinalVoodoo_de10_xls -c SpinalVoodoo_de10_xls`; `quartus_sta` does not accept the map/fitter `--read_settings_files` options.
- Set `DE10_PROJECT_DIR` to the build's private project under `build/quartus/`, not the authored input directory. Run core-clock STA with `nix develop --command devenv shell -- bash -lc 'cd "${DE10_PROJECT_DIR:?Set the private project path}" && quartus_sta -t report_core_clock_timing.tcl SpinalVoodoo_de10_xls SpinalVoodoo_de10_xls'`.
- Run worst-path STA similarly with `report_worst_timing.tcl`.
- XLS hardware trace replay must pass `--fb-base 0x3f400000`; the generic replay default targets the non-XLS layout. For verification/fixtures/traces without `state.bin`, `--preload-trace-textures` avoids the slow full texture-region clear.
