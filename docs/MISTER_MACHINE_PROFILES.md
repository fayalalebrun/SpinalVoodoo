# Saved DOS machine profiles

Saved profiles are integrated with the exact-version hardware-breakpoint broker and boot guardian. Main and the kernel remain unchanged. Selecting the approved core through native OSD automatically starts the selected DOS machine; native departure and cold reboot retire it before the guarded reset boundary. The installed board candidate is experimental, not general ao486-equivalent compatibility.

A profile is declarative JSON, not a DOSBox configuration file or shell script:

```json
{
  "version": 1,
  "cpu": "normal",
  "cycles": "auto",
  "memory_mb": 16,
  "audio": "off",
  "drives": {
    "C": {"type": "directory", "path": "shared"},
    "D": {"type": "iso", "path": "cd/disc.iso"}
  }
}
```

Defaults boot to the internal DOS prompt. CPU accepts normal/dynamic/auto; cycles accepts auto/max or 100–1000000; memory accepts 8/16/32/64/128 MiB; audio accepts off/sb16/sbpro2. These are configuration values supported by the pinned DOSBox source, not workload performance recommendations or physical audio acceptance.

Media paths are relative to the configured games root. Absolute paths, traversal, escaping symlinks, command metacharacters, duplicate JSON fields, unknown options and missing media are rejected before emitting configuration. A/B support floppy IMG; C/D support raw HDD IMG/VHD; D/E support ISO; C/D/E support directory mounts. VHD here means a raw image named .vhd, not arbitrary dynamic VHD containers. Image filesystem/layout compatibility is still checked by DOSBox at mount time.

Mounts default to read-only. Raw floppy/HDD `.img` copies can explicitly set `"writable": true`; the launcher enforces exclusive ownership, checked child cleanup and persistent crash-recovery gates. Directory, ISO and VHD writes and hot eject are not supported. See [checked raw-image media](MISTER_WRITABLE_MEDIA.md) for scope, saved settings, evidence and recovery policy. No arbitrary autoexec or game autostart is accepted. The optional audio setting does not itself enable the MiSTer ALSA route; the existing audio-enabled runtime/core validation is still required.

`machine-profile.py` validates configuration without starting hardware. `computer_launch.py` snapshots the selected profile privately, generates configuration and preserves the retirement receipt descriptor through the runtime launcher.

Install a checksum-verified bundle using `python3 install_computer.py` on the board. It requires the manifest's exact Main, menu and signed game-fixes core hashes, and refuses existing configuration, Scripts or user-startup targets rather than overwriting them. Defaults are `/media/fat/spinalvoodoo-computer-config`, `Scripts/SpinalVoodoo_Computer`, and the stock `linux/user-startup.sh` hook. Existing installations require an explicit preserved migration, not rerunning the fresh installer.

From stock MENU, use F12 → System Settings → Scripts → SpinalVoodoo_Computer to select `default` or `dynamic32`. Select the approved `SpinalVoodoo_game_fixes` core normally. The selection applies on the next activation, not as an in-session hot change. Profiles live under `profiles/`; `selected` holds one validated profile name. The `configure` script opens the stock framebuffer terminal editor for saved profiles, CPU/cycles/memory/sound and saved drive attach/eject settings. Raw floppy/HDD drives offer `ro` or explicit `rw`; all other drives stay read-only. It can clone a profile under a new name; saved names are selected through the editor's profile list. Existing files are backed up before replacement, invalid settings/media are rejected, and Cancel does not write. This requires the normal stock `fb_terminal=1` setting. There is no in-core hot-mount menu: changes apply on the next core activation.

`default` boots a normal-core, 16-MiB, audio-off DOS prompt; `dynamic32` defaults to dynamic-core, 32-MiB and maximum cycles. No game is auto-started. Place owned media inside the configuration's `games/` directory, then use the editor's Media action to enter its drive, type and relative path. Missing files, incompatible drive/type combinations and escaping paths are rejected without changing the pending profile. A directory plus ISO mount is physically validated, including native keyboard entry and DOS4GW/Glide initialization. Configure the game's sound hardware to match its profile: an SB16-configured Tomb Raider copy stalls when the profile exposes no Sound Blaster. Disabling the copied game's sound advances initialization. Bounded board diagnostics then skip the observed intro movies with Escape and use longer held Enter selections to reach the title and Caves; actual renderer-memory snapshots and native retirement from that render-active session are retained. These are not whole-game, movement, physical-audio or HDMI-video acceptance.

Boot startup verifies the entire bundle before enrolling Main, so it can take tens of seconds. A `running:pid=` status records session launch, not proof that checksum verification and the DOS prompt are ready. Use the visible DOS prompt before typing. Frozen runtime09 is deployed with the native settings/media editor and bounded identity-checked start/stop/restart, including the stock Python 3.9 pidfd syscall fallback. Cold-boot and native-entry tests verify that the editor's saved dynamic-CPU, 32-MiB profile and read-only directory mount persist and apply; native departure retires that session without a DOS exit command. Board tests verify stopping in MENU, restarting a real mapped DOSBox session without reprogramming its active core, automatic replacement-session startup, and subsequent retirement back to MENU.

Tests cover settings, mounts, validation, snapshot/launch ordering, identity rejection, receipt-based retirement, real parent-death containment and pidfd stop identity/timeouts. Board evidence additionally covers native entry/profile selection/departure, cold reboot, boot enrollment and broker-crash recovery. Release evidence is indexed in `docs/SCREAMER2_EXPERIMENT_JOURNAL.md`. Runtime09 additionally passes disposable raw-floppy/HDD writes, native retirement/restart, guest files left open at departure, cold-reboot data persistence and actual broker-crash recovery gates. Exact file bytes and unchanged clean baselines are retained. These tests do not establish power-loss atomicity, automatic filesystem repair, directory/VHD writes, hot eject, arbitrary core/programmer coverage, physical audio or general game compatibility.
