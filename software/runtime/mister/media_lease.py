"""Raw-image ownership and durable recovery gates for cooperating launchers.

A lease requires checked child I/O before completion. It does not establish
power-loss atomicity or guest filesystem consistency. Crashes require recovery.
"""
import fcntl
import hashlib
import os
from pathlib import Path
import stat


def prepare_state(path):
    """Make the durable recovery directory and persist its parent entry."""
    path = Path(path)
    parent = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        info = os.fstat(parent)
        if info.st_uid != os.geteuid() or info.st_mode & 0o022:
            raise ValueError('Owned media-state parent not writable by others required')
        try:
            os.mkdir(path.name, 0o700, dir_fd=parent)
        except FileExistsError:
            pass
        os.fsync(parent)
    finally:
        os.close(parent)


class ImageLease:
    def __init__(self, games, relative, state, writable=True):
        self.fd = None
        self.lock_fd = None
        self.state_fd = None
        self.finished = False
        self.writable = writable
        games = Path(games).resolve(strict=True)
        parts = Path(relative).parts
        if not parts or Path(relative).is_absolute() or '..' in parts:
            raise ValueError('Relative image path required')
        if Path(relative).suffix.lower() != '.img':
            raise ValueError('Only raw .img images are supported')
        directory = None
        try:
            self.state_fd = os.open(state, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
            info = os.fstat(self.state_fd)
            # Stock MiSTer exFAT uses a fixed 0022 mask, not POSIX chmod.
            # Metadata may be readable, but only its owner may modify it.
            if info.st_uid != os.geteuid() or info.st_mode & 0o022:
                raise ValueError('Owned lease directory not writable by others required')
            directory = os.open(games, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
            for part in parts[:-1]:
                next_directory = os.open(part, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW,
                                         dir_fd=directory)
                os.close(directory)
                directory = next_directory
            access = os.O_RDWR if writable else os.O_RDONLY
            lock_mode = fcntl.LOCK_EX if writable else fcntl.LOCK_SH
            self.fd = os.open(parts[-1], access | os.O_NOFOLLOW | os.O_NONBLOCK,
                              dir_fd=directory)
            info = os.fstat(self.fd)
            if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1 or info.st_size <= 0 or info.st_size % 512:
                raise ValueError('Single-link nonempty sector-aligned regular image required')
            fcntl.flock(self.fd, lock_mode | fcntl.LOCK_NB)
            # Key by path: replacement of the inode must not bypass recovery.
            key = hashlib.sha256(str(Path(games).absolute() / Path(relative)).encode()).hexdigest()
            self.marker = key + '.dirty'
            self.lock_fd = os.open(key + '.lock', os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW,
                                   0o600, dir_fd=self.state_fd)
            lock_info = os.fstat(self.lock_fd)
            if (not stat.S_ISREG(lock_info.st_mode) or lock_info.st_nlink != 1 or
                    lock_info.st_uid != os.geteuid() or lock_info.st_mode & 0o022):
                raise ValueError('Regular lease lock required')
            fcntl.flock(self.lock_fd, lock_mode | fcntl.LOCK_NB)
            if not writable:
                try:
                    os.stat(self.marker, dir_fd=self.state_fd, follow_symlinks=False)
                except FileNotFoundError:
                    return
                raise FileExistsError('Unfinished image requires recovery: ' + str(relative))
            marker = os.open(self.marker, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                             0o600, dir_fd=self.state_fd)
            try:
                record = ('image=%s device=%d inode=%d\n' % (relative, info.st_dev, info.st_ino)).encode()
                if os.write(marker, record) != len(record):
                    raise OSError('Short lease marker write')
                os.fsync(marker)
            finally:
                os.close(marker)
            os.fsync(self.state_fd)
        except BaseException:
            self.close()
            raise
        finally:
            if directory is not None:
                os.close(directory)

    def complete(self, *, exit_status, receipt, eof):
        """After the child exits, check its receipt and sync kernel image writes.

        This API is not a checked DOSBox media-flush receipt. Integration must
        first check all guest/stdio writes, flushes and closes in the child.
        """
        if self.fd is None or self.finished:
            raise RuntimeError('Active unfinished image lease required')
        expected = b'MEDIAOK\n' if self.writable else b'RETIRED\n'
        if type(exit_status) is not int or exit_status != 0 or receipt != expected or eof is not True:
            raise ValueError('Successful child exit, exact receipt and EOF required')
        if not self.writable:
            self.finished = True
            return
        os.fsync(self.fd)
        os.unlink(self.marker, dir_fd=self.state_fd)
        try:
            os.fsync(self.state_fd)
        except OSError:
            # Restore the recovery gate before releasing ownership on failure.
            marker = os.open(self.marker, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW,
                             0o600, dir_fd=self.state_fd)
            os.close(marker)
            raise
        self.finished = True

    def close(self):
        # Never remove an unfinished marker, including exception/crash paths.
        for name in ('fd', 'lock_fd', 'state_fd'):
            descriptor = getattr(self, name, None)
            if descriptor is not None:
                os.close(descriptor)
                setattr(self, name, None)

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
