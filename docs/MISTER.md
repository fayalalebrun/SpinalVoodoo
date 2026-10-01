# MiSTer integration

The FPGA runs the XLS renderer. ARM DOSBox-X and Glide share a checked runtime.
The computer service starts the selected saved profile and retires it before
covered native Main reset/core-switch operations. Main and the kernel stay unchanged.

The deployed computer uses frozen runtime09 and the signed
`SpinalVoodoo_game_fixes.rbf`. This is an experimental integration, not a general
MiSTer compatibility or public-release claim. Current source also contains
unsigned hardware experiments; do not substitute them for the signed core.

## Try the installed computer

1. On the host, run `nix run .#mister-keyboard` to open HDMI capture and focused
   keyboard forwarding. See [capture and keyboard](MISTER_REMOTE_KEYBOARD.md)
   for device and SSH settings. A directly connected display/keyboard also works.
2. In MiSTer's core browser, select `SpinalVoodoo_game_fixes.rbf`.
3. Wait for the DOS prompt. Bundle verification can take tens of seconds.
4. Use the native core-switch or reboot controls to leave the managed session.
   Closing the capture window only stops capture/input forwarding, not DOSBox.

The current saved profile is `interactive32`: dynamic CPU, 32 MiB RAM, audio off,
and a read-only shared directory. Profiles can change; see
[saved machine profiles](MISTER_MACHINE_PROFILES.md) for the native Scripts editor.
Mounts default to read-only. Only explicit raw-floppy/HDD `.img` writes use the
[checked image-ownership path](MISTER_WRITABLE_MEDIA.md). Directory/ISO/VHD writes,
hot eject, automatic repair and power-loss atomicity are not supported.

## Build and check

Run normal tools through `nix develop`. Use fresh build/package directories;
source preparation and packaging reject stale or existing outputs.

```sh
nix develop --command python3 -B workflows/build/prepare-mister-sources --check
nix develop --command make check
nix develop --command workflows/check/check-mister-platform
nix develop --command bash workflows/check/check-mister-glide

nix develop --command bash -c 'MISTER_DOSBOX_BUILD_DIR=output/mister/dosbox-local bash workflows/build/build-mister-dosbox'
nix develop --command python3 -B workflows/package/package-mister-runtime \
  --dosbox-build output/mister/dosbox-local --output output/mister/runtime-local
nix develop --command bash workflows/check/test-mister-arm output/mister/runtime-local
```

Pins live in `vendor/sources.json`; fetched sources live in `build/deps/mister/`.
Preparation does not overwrite modified checkouts. The ARM build workflow uses
an isolated `mister-arm` environment to prevent host include/library leakage.
Packaging includes the private ARM loader, dependencies, checksums and source/license
inventory. Do not deploy individual raw Nix build products to stock MiSTer.

A fresh computer installation uses the bundle's `install_computer.py`. It checks
Main/menu/core identities and refuses existing configuration, Scripts and startup
paths. Preserve an existing installation before an explicit migration. See the
[profile guide](MISTER_MACHINE_PROFILES.md) for paths and installation details.
`SHA256SUMS` checks integrity, not authenticity or FPGA timing compatibility.

Quartus needs both toolchains: `nix develop --command devenv shell -- ...`.
Complete compilation plus required timing signoff must fit within 900 seconds.
A generated RBF is not a timing-signed release.

## Interfaces and evidence

[ARCHITECTURE.md](../ARCHITECTURE.md) describes component boundaries, scanout,
renderer retirement and experimental-versus-signed hardware.
The [experiment journal](SCREAMER2_EXPERIMENT_JOURNAL.md) retains deployment
identities, measurements, failures and recovery records. This guide does not
repeat that history. Game compatibility, physical audio and arbitrary external
programmer coverage remain separate acceptance work.

The low-level mailbox and memory-ownership contracts follow. Normal DOSBox/Glide
clients use the shared runtime rather than creating their own mappings.

## Mailbox protocol and host API

All fields are little-endian. Offsets are relative to `0x3e000000`:

| Offset | Layout |
| --- | --- |
| `0x0000` | Magic `0x4d565331` (32 bits), publication sequence (32 bits) |
| `0x0008` | Command count, 0–256 (32 bits), reserved flags (32 bits, zero) |
| `0x0080` | Sequence [31:0], error [39:32], completed count [48:40], zero [63:49] |
| `0x0100` | Up to 256 commands, four 32-bit fields: opcode, address, value, mask |
| `0x2000` | 32-bit read result at each command's index; other entries are unspecified |
| `0x2800` | 512-bit key bitmap, then key-state sequence (32 bits) and zero (32 bits) |

Opcodes are 0 (no-op), 1 (MMIO write), 2 (MMIO read), 3 (frame publication),
4 (keyboard snapshot). No-op and snapshot fields are zero. MMIO addresses are
24-bit byte addresses, four-byte aligned, with a nonzero four-bit byte mask.
Frame address selects one of the three host slot bases or a validated renderer
buffer; value packs width [11:0],
height [23:12], format [28:24], enable [29], and zero [31:30]. Mask holds byte
stride [13:0], with its upper bits zero. Enabled frames require RGB565 format 4
or XRGB8888 format 6, nonzero dimensions within 1920×1200, four-byte-aligned
stride covering each row, and a last row contained within the 4 MiB slot.
Renderer descriptors instead require a 4 KiB-aligned base in
`0x3f400000..0x3f7fffff`, exactly 640×480 RGB565, and every row within that
4 MiB renderer window. The shared runtime derives these descriptors from GPU
registers; callers cannot submit raw frame commands through its exchange API.
The key index is the PS/2 scan code plus 256 for an extended key.

The host writes commands and count, orders those writes, then publishes a new
sequence. It must keep the batch immutable until the matching acknowledgment.
The sequence wraps modulo 2³²; unchanged sequences are not replayed. Errors are
1 (count/flags), 2 (command encoding), 3 (frame rejection/ack mismatch), and
4 (reset synchronization, no commands executed). A failed batch may already have
executed the reported number of commands; never automatically retry it.

`software/runtime/mister/mister_mailbox.h` exposes attach, exchange and detach. Attach rejects
an outstanding batch and probes an idle mailbox with an empty exchange. Exchange
validates input, serializes threads sharing the same object, uses ARM `dmb sy`
barriers, and polls with a monotonic deadline. A timeout or device error faults
the session; subsequent exchanges return `EPIPE` without changing shared memory.
Recovery requires explicit detach/attach after resolving the outstanding batch
or reset. Detach requires all callers to have stopped.

The caller must supply an eight-byte-aligned, uncached, exclusively owned mapping
of at least `0x3000` bytes. The library neither maps `/dev/mem` nor establishes
cross-process ownership or memory reservations. Barriers do not flush caches.
Do not create competing mailbox objects/processes for the same mapping. In the
local-scanout architecture, frame acknowledgment follows retirement of every
accepted DDR read from the previous descriptor. The previous slot may be reused
only if the new descriptor selects a different slot or disables presentation.
It does not mean that the newly submitted frame is already visible on HDMI.

## Reserved memory and process ownership

`software/runtime/mister/mister_memory.c` is separate from the mailbox transport. Initialize
its handle with `MISTER_MEMORY_INIT`. It accepts an exact 32 MiB `no-map`
reservation at `/proc/device-tree/reserved-memory/spinalvoodoo@3e000000`, empty
parent `ranges`, and one- or two-cell big-endian address/size properties.
If the DT reservation is absent, it accepts the verified stock MiSTer layout:
`memory/reg` must describe 1 GiB at zero, the command line must contain exactly one
`mem=511M` and one `memmap=513M$511M`, and every System RAM entry must be
`00000000-1fefffff`. Any reported resource overlapping the custom window rejects
the stock path. Malformed/redacted maps and malformed present DT properties fail
closed. This is deliberately not a general arbitrary-hole allocator.

This reservation gate needs no stock MiSTer boot/DTB modification. The matching custom core
must be selected exclusively. The managed computer service retires its guest
before covered Main reset/core-switch operations. Unmanaged clients must stop
before switching cores. Kernel exclusion alone does not protect against
another FPGA core or raw `/dev/mem` client. Stock framework scaler storage starts
at `0x20000000`, separate from the custom upper-32-MiB window.

Before direct runtime use, ensure `/run/lock` exists (`mkdir -p /run/lock` as root).
An owned, single-link, private regular file `/run/lock/spinalvoodoo.lock` is held
with nonblocking `flock` for the mapping lifetime. Never delete that lock file
while a client may hold it. `/dev/mem` is opened with `O_SYNC`, `O_CLOEXEC` and
`O_NOFOLLOW`, and mapped shared. These checks establish an advisory single-client
contract, not protection against another privileged program ignoring the lock.
The selected ARM kernel's actual mapping/cache attributes still require validation.

Opening an already-owned handle fails with `EALREADY` without losing ownership.
Always close the initialized handle after an open failure too: a failed descriptor
close followed by failed unmapping retains the mapping and lock for explicit
cleanup. Failed unmapping during normal close likewise retains ownership. Callers
must serialize handle lifetime operations and detach the mailbox before closing.
Host tests replace all physical-memory operations with workspace files and cover
reservation rejection, lock contention, reopening and injected cleanup failures.
