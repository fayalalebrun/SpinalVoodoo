# Focused remote keyboard

`diagnostics/board/mister-keyboard` opens a dedicated keyboard window. Only events received while that window is focused are forwarded over one SSH connection. Focus loss and window close release held keys. SSH disconnect or a two-second heartbeat timeout destroys the remote virtual keyboard, preventing stuck input. The remote helper opens only `/dev/uinput`; it does not change framebuffer memory, stop Main or program the FPGA.

Standard PC letters, digits, punctuation, modifiers, F1–F12, navigation and keypad keys are supported, including simultaneous held keys. This forwards physical key positions, not Unicode text or clipboard contents. Host compositor shortcuts remain owned by the host and are not guaranteed to reach the window. Unmapped multimedia/nonstandard keys are ignored.

## One-command launch

From this repository, run:

```sh
nix run .#mister-keyboard
```

This builds a self-contained ARM receiver, uploads it to a temporary board directory, opens live USB capture video with focused keyboard forwarding in the same window and removes the remote files on exit. Video defaults to `/dev/video0` using the existing capture card's 1280×720 MJPEG mode; override with `MISTER_VIDEO_DEVICE` or `--video-device`. Audio is not forwarded. The capture card must be connected and available to your user; another capture process cannot own it simultaneously. It defaults to `root@192.168.2.86` and the existing `~/.ssh/monumental` key when present. It uses the recorded board known-hosts file when running from this repository. Optional overrides are `MISTER_HOST` and `MISTER_SSH_KEY`. Main must remain running. The first Nix build can take time to prepare the ARM toolchain.

## Manual preparation

Build `diagnostics/board/mister-keyboard-receiver.c` with the existing ARM toolchain, retaining `-Wall -Wextra -Werror -std=gnu11`. Copy that binary into a private diagnostic directory on MiSTer. It requires access to `/dev/uinput`. If the ARM binary uses the private runtime interpreter, include the matching private loader and library path in `--remote-command`, as for the earlier release-key diagnostic. A host-built binary is not an ARM deployment.

## Launch

From a desktop terminal, with the usual SSH key/known-host configuration:

```sh
nix develop --command diagnostics/board/mister-keyboard root@MISTER_IP \
  --remote-command '/absolute/board/path/mister-keyboard-receiver'
```

Use `--identity /path/to/key` if necessary. The window opens after the receiver reports ready; focus it and play. Switch to another window to stop forwarding, or close it to disconnect. `SDL2_LIBRARY` can explicitly name the installed SDL2 shared library if its normal lookup fails. No global keyboard capture or desktop automation is used.

MiSTer Main must remain running: its existing Linux-keyboard-to-FPGA path handles the virtual keyboard. This is not a standalone-mode input implementation. Host compilation and unit checks pass; physical focus/held-key behavior and ARM receiver deployment still require board validation.
