# Repository organization and migration

## Ownership boundaries

Authored hardware is under `hardware/`, authored CPU-side code under `software/`. Tests live with their owners; cross-component tests and trace fixtures live in `verification/`. `workflows/` groups public operations and `diagnostics/` groups profiling/recovery/inspection tools. `vendor/` makes upstream ownership and downstream compiler patches explicit. `environment/` holds Nix definitions, not runtime C++.

Normal generated files belong under ignored `build/`: XLS/Scala RTL, Qsys outputs, simulation objects, host builds, fetched pinned dependencies, private Quartus projects and packaged runtimes. Root Make target names are unchanged. `make check` aggregates the moved test suites. Scala packages retain their names.

`hardware/shared/rtl/XlsDe10Top.sv` remains shared despite its name: the MiSTer platform instantiates the same renderer integration. No second renderer was introduced. Glide and DOSBox integrations share `software/runtime/mister/`; its Makefile currently aggregates their native tests to preserve the existing check surface.

## Preserved historical material

`output/` and `artifacts/` remain historical evidence, not disposable build cache. Signed bitstreams, failed candidates, matching source/runtime snapshots and raw measurements are not removed. `experiments/organization-migration/moves.json` maps the original source paths to their destinations. Full pre-move source backups, original generated build directories, logs and protected hashes are under `output/deslop-cleanup/organization/`.

The previous tracked combinational RTL is preserved under `releases/generated-baseline/`, with hashes and its original paths. This is a historical generated snapshot, **not a signed release or a build input**. Clean regeneration from current DSLX differs in four TMU/address modules. Whether the historical mismatch comes from stale snapshots, toolchain changes or both has not been established; release equivalence is unverified. All current build consumers use `build/generated/xls/rtl/`; do not equate byte differences to a new authored optimization or a release-equivalence proof.

Fetched MiSTer framework/Main/DOSBox-X working checkouts now live under `build/deps/mister/`. Temporary links at the historical `output/mister/{sources,rtl}` paths preserve existing candidate links and recovery recipes. New workflows use the canonical build paths. The framework checkout is clean at its pinned revision; the rejected scaler patch is archived historical evidence only.

## Remaining explicit exceptions

- Legacy tracked HPS handoffs, assignment defaults, SOPCINFO and the SDRAM report remain alongside the DE10 inputs until regeneration and board boot dependencies are proved. Moving sources does not establish that these are disposable.
- Vendored Glide retains its upstream layout, local edits and legacy in-tree Makefile outputs. Native/ARM integration builds and dependency identity must be preserved before converting this tree into a patch-only checkout or changing its entire build system.
- Tool-managed `.direnv`, `.devenv`, Scala and simulation caches can retain their tool-required locations. Residual legacy roots (`xls`, `hw`, `src`, `scripts`, `tools`, `emu`, `nix`, `test`) were relocated intact to `output/deslop-cleanup/organization/legacy-residuals/`; its manifest verifies SHA256 hashes for all 1,161 remaining files. These roots no longer occupy the active tree.
- Large evidence archives are not relocated to guessed storage. Release manifests identify preserved artifacts, not a promise that the current experimental checkout reproduces them.

## Build entrypoints

- `nix develop --command make check`
- `nix develop --command make xls`
- `nix develop --command workflows/check/check-de10-xls-rtl`
- `nix develop --command workflows/check/check-de10-xls-replay`
- `nix develop --command workflows/check/check-mister-platform`
- `nix develop --command bash workflows/check/check-mister-glide`
- `nix develop --command devenv shell -- make de10/xls/bitstream`

`workflows/build/prepare-de10-project` creates a fresh private Quartus project under `build/quartus/`; authored QSF/Tcl inputs keep repository-relative source paths, while the private copy resolves them. Qsys TCL generators write their intermediate system files under `build/generated/qsys/inputs/`, not over authored inputs. Complete FPGA compilation/signoff still has a 900-second limit and is separate from generation/lint/replay acceptance.

The DOSBox output-test clean-build failure was caused by external SDL include paths being passed as ordinary `-I` paths. Its compile now marks those external includes as system headers while retaining `-Werror -pedantic` for project code; it does not globally suppress warnings.

The static inventory generator is `diagnostics/repository/deslop-viewer.py --refresh`, producing one self-contained `output/deslop-audit/viewer.html`. The original organization proposal remains under `.learning/` as historical rationale rather than a claim that every conditional gate has passed.
