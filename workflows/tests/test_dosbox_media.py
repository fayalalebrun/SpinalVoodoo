import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class DosboxMediaTest(unittest.TestCase):
    def test_checked_write_flush_close_sync_seek_and_unclosed_errors(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / 'media-test'
            subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
                str(ROOT / 'software/integrations/dosbox/dosbox_media.cpp'),
                str(ROOT / 'software/integrations/dosbox/dosbox_media_test.cpp'),
                '-Wl,--wrap=fwrite,--wrap=fflush,--wrap=fclose,--wrap=fsync,--wrap=fseeko64',
                '-o', str(binary)], check=True)
            for phase in ('success', 'write', 'flush', 'close', 'sync', 'seek', 'unclosed'):
                with self.subTest(phase=phase):
                    subprocess.run([str(binary), phase], check=True, timeout=5)


if __name__ == '__main__':
    unittest.main()
