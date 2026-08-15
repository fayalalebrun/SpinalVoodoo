# XLS Architecture

SpinalHDL and XLS are independent implementations of the Voodoo core.

## Native SpinalHDL

Production Scala under `src/voodoo/` owns the complete native implementation, including arithmetic, streams, state machines, caches, memories, and platform integration. Native simulation and the `SpinalVoodoo_de10` Quartus revision compile only RTL generated from that implementation.

## Pure XLS

The pure-XLS implementation lives under `xls/dslx/`. `xls/dslx/core_sim.x` composes the hosted command controller, rasterizer, pixel pipeline, texture sampler, texture cache, and memory interfaces as an XLS proc network.

`scripts/generate-xls-rtl --de10-proc` emits `CoreSimDe10.sv` and its schedule artifacts under `xls/build/core_sim_de10_rtl/`. The DE10 adapters under `hw/de10/xls/` connect that generated core to Avalon, Qsys, cache-bank memories, and scanout. The separate Quartus revision is `SpinalVoodoo_de10_xls`.

The pure-XLS replay, simulation, and DE10 targets are documented in `docs/pure-xls-coresim.md`.

## Separation Rule

Do not instantiate generated XLS datapaths from production SpinalHDL. Shared behavior must remain independently implemented and tested in both designs. Platform-only wrappers may be shared when they do not move core logic across the implementation boundary.

## Commands

```bash
nix develop "path:$PWD" -c make -C xls test
nix develop "path:$PWD" -c make native/sim/build
nix develop "path:$PWD" -c make de10/rtl
nix develop "path:$PWD" -c make de10/xls/check
nix develop "path:$PWD" -c make de10/xls/replay
```
