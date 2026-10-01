# SpinalVoodoo

An XLS/DSLX implementation of the 3dfx Voodoo Graphics renderer, with SpinalHDL platform/scanout glue, DE10-Nano and MiSTer integration, and a shared host runtime for Glide and DOSBox-X.

## Find the code

| Directory | Responsibility |
|---|---|
| `hardware/renderer/dslx/` | Rendering controller, rasterization, interpolation, texture/cache and pixel processing |
| `hardware/shared/{scala,rtl,tests}/` | Shared memory/scanout glue, handwritten RTL and hardware regressions |
| `hardware/platforms/{de10,mister}/` | Board-specific generators, shell, constraints and tests |
| `software/runtime/mister/` | Shared reserved-memory ownership and ordered command transport |
| `software/integrations/{glide,dosbox}/` | Owned frontend adapters, configuration and colocated tests |
| `software/simulation/xls/` | Hosted XLS bridge and simulation harnesses |
| `software/{trace,reference}/` | Shared trace format/replay helpers and independent oracle wrapper |
| `verification/{integration,fixtures}/` | Cross-component tests and preserved trace/reference inputs |
| `workflows/` | Build, check, package, run and board commands; Python workflow tests |
| `diagnostics/` | Profiling, board/recovery tools and repository inspection |
| `vendor/` | Glide, pinned 86Box, source pins/licenses and compiler patches |
| `environment/` | Nix toolchain definitions; root flake/devenv files remain entrypoints |
| `build/` | Generated RTL/Qsys, fetched working dependencies, simulation/build results and packages |
| `releases/`, `experiments/` | Release identities and evidence indexes, not disposable build state |
| `docs/` | Architecture, guides, references and experiment journal |

The Scala packages keep their existing names. DSLX is the renderer; Scala and handwritten RTL integrate it rather than implement duplicate renderers. Unit tests stay with their owners; integration tests span the boundaries.

See [ARCHITECTURE.md](ARCHITECTURE.md) for the maintained design and [the migration record](docs/ORGANIZATION.md) for exact provenance boundaries and validation limits. The [interactive organization proposal](.learning/organization-proposal.html) explains the intended layout; the generated static inventory is `output/deslop-audit/viewer.html`.

## Development and checks

```sh
git submodule update --init vendor/86box
nix develop
make check
make xls
make de10/xls/check
make de10/xls/replay
```

`make check` runs Python workflow/profiling/inventory suites, native runtime/frontend checks, DSLX tests and all Scala tests. RTL lint and integrated platform/real-Glide checks are explicit:

```sh
nix develop --command workflows/check/check-mister-platform
nix develop --command bash workflows/check/check-mister-glide
```

Quartus needs the nested environment, and every complete compile/signoff is bounded to 900 seconds:

```sh
nix develop --command devenv shell -- make de10/xls/bitstream
```

Normal generation writes into `build/generated/`; authored board input directories are not Quartus working projects. Private DE10 projects are prepared under `build/quartus/`. Never infer that an unsigned experimental checkout reproduces a preserved timing-signed release.

## Run workloads

Public Make target names stay stable. `make native/help`, `make dos/help`, `make tomb/help` and `make de10/help` list the supported command surface.

```sh
make native/trace/run/test00
make dos/xls/run/df00sdk
make dos/xls/dosbox
make tomb/prepare ARGS='--game-dir /path/to/TOMBRAID --patch /path/to/3dfx-patch.zip --iso /path/to/disc.iso'
make tomb/xls/run
```

The hosted Glide path is 32-bit. DOSBox-X must match that ABI. For the installed MiSTer computer, see [the operating guide](docs/MISTER.md),
[capture and keyboard](docs/MISTER_REMOTE_KEYBOARD.md), and
[saved profiles](docs/MISTER_MACHINE_PROFILES.md). The MiSTer ARM build is separate
from hosted DOSBox. Trace fixtures live in `verification/fixtures/traces/`.

## Evidence and releases

Historical performance results, failed candidates, signed RBFs and matching source/runtime snapshots remain under `output/` and `artifacts/`. They are preserved evidence, not uniformly regeneratable cache. No evidence is removed by the source reorganization. Current palette/arbiter candidates remain experimental pending full timing signoff and physical workload validation.

[The Screamer 2 journal](docs/SCREAMER2_EXPERIMENT_JOURNAL.md) records measured workloads, failures and deployment identities; [docs/PROFILING.md](docs/PROFILING.md) describes profiling limitations. Simulation throughput is not live-game FPS.
