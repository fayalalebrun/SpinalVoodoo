# Checked raw-image media

Deployed frozen runtime09 supports explicit raw-floppy/HDD image writes. Runtime06 remains preserved and read-only. Board tests validate disposable images, not arbitrary game media or power-loss atomicity.

## Observed cleanup gap

The matched DOSBox source closes raw images in `imageDisk::~imageDisk()` with an unchecked `fclose`. `DOS_ShutdownFiles()` deletes file objects. `DOS_ShutdownDrives()` does not return a checked flush result. The MiSTer adapter sends `RETIRED` after GPU fence and runtime release in its exit handler. The broker requires this receipt, EOF and exit status zero.

These checks establish renderer retirement. They do not establish successful disk writes, checked stdio cleanup, or durable guest filesystems. A host `fsync` cannot recover data that remains in a failed child stdio buffer. Parent-death SIGKILL is containment, not a disk-flush receipt.

## Checked child cleanup and ownership

Maintained source preparation adds checked raw-image write/seek/close wrappers and a pre-drive guest-file Flush step. Failures latch permanently, including stdio flush, file sync, close and unclosed managed streams. A failed seek prevents later managed writes at an unknown position. The child sends `MEDIAOK` only after checked media cleanup, GPU fence and runtime release.

`computer_launch.py` uses `media_lease.py` for supported raw-image mounts, including read-only mounts. It retains locks, passes checked image descriptors to DOSBox, and uses `/proc/self/fd` paths so pathname replacement cannot redirect the child. The supervising launcher requires the correct child receipt, pipe EOF and exit status zero. It then synchronizes the images and recovery metadata before forwarding `RETIRED` to the unchanged broker. Parent-death SIGKILL contains a launcher crash; it never clears a recovery marker.

The lease:

- Opens a nonempty, sector-aligned raw `.img` through directory file descriptors. It rejects symlink components, special files and hard links.
- Takes nonblocking image and path-keyed sidecar locks: exclusive for writes, shared for read-only access.
- Writes and synchronizes a recovery marker before returning ownership.
- Leaves the marker after incomplete shutdown or a process crash. Replacing the image inode does not remove the path's recovery requirement.
- Requires successful exit, the exact checked `MEDIAOK` receipt and EOF before synchronizing kernel image writes and removing a writable marker. Read-only access also refuses unfinished writable markers.
- Restores the marker if the final directory synchronization fails.

Use one trusted, durable state directory for all cooperating clients. Do not use `/run` or `/tmp` for persistent recovery markers. The state directory and lock must belong to the service user. Group and other users must not have write access. Stock MiSTer exFAT uses a fixed `0022` permission mask; readable metadata is allowed because it contains paths and image identity, not secrets. The service must trust the state directory's parents and media root.

Locks are advisory. They do not prevent arbitrary host programs from writing the image or changing the media namespace. All supported readers and writers must use the same ownership mechanism. Direct host modification during a session remains outside the supported launcher's protection.

An unfinished marker requires explicit investigation and recovery. There is no automatic marker-clear command. Do not remove a marker merely because the old process is absent. Preserve a copy of the image before recovery. An image may contain incomplete guest filesystem updates even if all host writes reached storage.

## Saved settings

Omitted `writable` means read-only. Only a floppy or HDD raw `.img` can use `"writable": true`. Writable images must be nonempty, sector-aligned regular files with one link. Native Scripts → Configure → Media asks for `ro` or `rw` for these types. Settings apply on the next machine start, not during an active mount.

Example drive entry:

```json
{"A": {"type": "floppy", "path": "copies/save.img", "writable": true}}
```

Use a disposable or backed-up copy. Directory writes, ISO writes, writable VHDs, hot eject, filesystem repair, power-loss atomicity and arbitrary host programmers are not supported by this path. A successful receipt establishes checked host cleanup, not atomic guest filesystem transactions.

## Recovery policy and acceptance

An unfinished image stays unavailable for both read-only and writable mounts, including after reboot. Keep its marker and preserve the image for offline investigation. For a safe service recovery, select another profile with no image, or create a new image path from a known-clean preserved backup. Do not copy an unchecked crash image and call it clean. Automated marker clearing and automatic filesystem repair are intentionally absent.

Host tests cover error-latched writes/seeks/flushes/syncs/closes, managed streams left open, invalid or absent receipts, exclusive/shared contention, unsafe paths, inode replacement, directory sync failures, launcher crashes and restart gates. The new ARM build must match maintained source preparation. Matching runtime09 board tests pass actual floppy/HDD writes, native departure/restart, a running DOS program's file left open at departure, native cold reboot with independent saved-byte checks and post-boot append, and actual broker-crash containment/recovery gates. The failed images and their recovery markers remain preserved. The installed Scripts editor saves explicit write settings; its new write prompt was tested through the actual script target, not a new OSD navigation recording. Entry in these media tests uses stock Main's `load_core` FIFO; native entry was validated separately for the unchanged broker mechanism. Evidence and failed attempts are recorded in `docs/SCREAMER2_EXPERIMENT_JOURNAL.md`.
