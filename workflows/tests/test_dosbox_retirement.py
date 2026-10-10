import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class DosboxRetirementTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.temp.name) / 'input-test'
        subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
            '-I' + str(ROOT / 'software/runtime/mister'), '-I' + str(ROOT / 'build/deps/mister/dosbox-x/include'),
            str(ROOT / 'software/integrations/dosbox/dosbox_input.cpp'),
            str(ROOT / 'software/integrations/dosbox/dosbox_media.cpp'),
            str(ROOT / 'software/integrations/dosbox/dosbox_input_test.cpp'),
            '-Wl,--wrap=mister_runtime_acquire,--wrap=mister_runtime_release,--wrap=mister_runtime_exchange,--wrap=clock_gettime',
            '-o', str(cls.binary)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_child(self, fail=False, fence_fail=False):
        reader, writer = os.pipe()
        try:
            environment = {**os.environ, 'MISTER_SHUTDOWN_FD': str(writer)}
            environment.pop('MISTER_TEST_CLEANUP_FAILURE', None)
            environment.pop('MISTER_TEST_FENCE_FAILURE', None)
            if fence_fail:
                environment['MISTER_TEST_FENCE_FAILURE'] = '1'
            if fail:
                environment['MISTER_TEST_CLEANUP_FAILURE'] = '1'
            result = subprocess.run([str(self.binary)], env=environment, pass_fds=(writer,), capture_output=True, timeout=5)
            os.close(writer)
            writer = -1
            return result, os.read(reader, 64)
        finally:
            os.close(reader)
            if writer >= 0:
                os.close(writer)

    def test_controller_accepts_only_the_adapter_retirement_and_exit(self):
        spec = importlib.util.spec_from_file_location('retirement_controller', ROOT / 'software/runtime/mister/core_gate_controller.py')
        controller = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(controller)
        session = controller.Session([str(self.binary)])
        try:
            self.assertEqual(session.child.wait(timeout=5), 0)
            self.assertTrue(session.stop())
            self.assertTrue(session.retired)
        finally:
            if session.reader is not None:
                os.close(session.reader)

    def test_success_has_explicit_retirement_receipt(self):
        result, receipt = self.run_child()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(receipt, b'RETIRED\n')

    def test_checked_media_receipt_and_unclosed_stream_refusal(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / 'disk.img'
            image.write_bytes(bytes(1024))
            with image.open('r+b') as media:
                for unclosed in (False, True):
                    reader, writer = os.pipe()
                    try:
                        environment = {**os.environ, 'MISTER_SHUTDOWN_FD': str(writer), 'MISTER_MEDIA_FDS': str(media.fileno())}
                        if unclosed: environment['MISTER_TEST_MEDIA_UNCLOSED'] = '1'
                        result = subprocess.run([str(self.binary)], env=environment,
                            pass_fds=(writer, media.fileno()), capture_output=True, timeout=5)
                        os.close(writer)
                        writer = -1
                        self.assertEqual(result.returncode, 1 if unclosed else 0, result.stderr)
                        self.assertEqual(os.read(reader, 64), b'' if unclosed else b'MEDIAOK\n')
                    finally:
                        os.close(reader)
                        if writer >= 0: os.close(writer)

    def test_failed_renderer_fence_denies_receipt_and_clean_exit(self):
        result, receipt = self.run_child(fence_fail=True)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(receipt, b'')

    def test_invalid_receipt_descriptor_rejected_before_acquiring_memory(self):
        for value in ('1', '-1', '+4', ' 4', '3x', '999999999999999999999999', '99999'):
            with self.subTest(value=value):
                result = subprocess.run([str(self.binary)], env={**os.environ,
                    'MISTER_SHUTDOWN_FD': value, 'MISTER_TEST_INVALID_RECEIPT': '1'}, capture_output=True, timeout=5)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_cleanup_error_is_not_a_successful_exit_or_receipt(self):
        result, receipt = self.run_child(fail=True)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(receipt, b'')
        self.assertIn(b'MiSTer runtime cleanup', result.stderr)


if __name__ == '__main__':
    unittest.main()
