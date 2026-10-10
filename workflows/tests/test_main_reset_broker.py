import importlib.util
import copy
import json
import os
from pathlib import Path
import signal
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'software/runtime/mister'))
spec = importlib.util.spec_from_file_location('reset_broker', ROOT / 'software/runtime/mister/main_reset_broker.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ResetBrokerTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        binary = root / 'Main'
        binary.write_bytes(b'main')
        core = root / 'core.rbf'
        core.write_bytes(b'core')
        self.broker = module.Broker(binary, {'sites': [{'candidate_function': 'reset', 'address': '0x1000', 'width': 4}]},
                                    ['/private/launch'], core, 'SpinalVoodoo', root)
        self.broker.trace = Mock()
        self.broker.hbp = Mock()
        self.broker.status = Mock()
        self.broker.tasks[10] = {'stopped': False, 'ready': True, 'armed': 1, 'step': None}

    def test_manifest_validation(self):
        manifest = json.loads((ROOT / 'software/runtime/mister/main_control_manifest.json').read_text())
        module.validate_manifest(manifest)
        for field, value in (('address', '0x8bf41'), ('width', True), ('first_instruction_bytes', 'zz'), ('candidate_function', 'unknown')):
            candidate = copy.deepcopy(manifest)
            candidate['sites'][0][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                module.validate_manifest(candidate)
        manifest['sha256'] = 'invalid'
        with self.assertRaises(ValueError):
            module.validate_manifest(manifest)

    def trap(self, pc):
        def trace(request, tid, address=0, data=0):
            if request == 12:
                data._obj[15] = pc
                data._obj[16] = 0x20
        self.broker.trace.side_effect = trace
        self.broker.event(10, (signal.SIGTRAP << 8) | 0x7f)

    def test_receipt_failure_keeps_main_stopped_and_breakpoints_armed(self):
        session = self.broker.session = Mock()
        session.stop.return_value = False
        self.trap(0x1000)
        self.assertTrue(self.broker.tasks[10]['stopped'])
        self.assertIs(self.broker.session, session)
        self.broker.hbp.assert_not_called()
        self.assertFalse(any(call.args[0] == 7 for call in self.broker.trace.call_args_list))

    def test_retirement_before_hardware_step_and_rearm(self):
        self.broker.session = Mock()
        self.broker.session.stop.return_value = True
        self.trap(0x1000)
        self.assertIsNone(self.broker.session)
        self.assertEqual(self.broker.tasks[10]['step'], 0)
        self.broker.hbp.assert_any_call(10, 1, 0x1004)
        self.broker.trace.assert_any_call(7, 10, data=0)
        self.broker.hbp.reset_mock()
        self.trap(0x1004)
        self.assertIsNone(self.broker.tasks[10]['step'])
        self.broker.hbp.assert_any_call(10, 1, 0x1000)

    def test_unrecognized_trap_never_resumes(self):
        with self.assertRaisesRegex(RuntimeError, 'Unrecognized'):
            self.trap(0x2000)
        self.assertTrue(self.broker.tasks[10]['stopped'])
        self.broker.hbp.assert_not_called()

    def test_clone_tracked_before_parent_resumes(self):
        def trace(request, tid, address=0, data=0):
            if request == 0x4201:
                data._obj.value = 11
            if request == 7:
                self.assertIn(11, self.broker.tasks)
                self.assertFalse(self.broker.tasks[11]['ready'])
        self.broker.trace.side_effect = trace
        self.broker.event(10, (3 << 16) | (signal.SIGTRAP << 8) | 0x7f)
        self.assertFalse(self.broker.tasks[11]['ready'])
        self.broker.arm = Mock()
        self.broker.event(11, (128 << 16) | (signal.SIGTRAP << 8) | 0x7f)
        self.broker.arm.assert_called_once_with(11)

    def test_signal_delivery_preserved(self):
        self.broker.event(10, (signal.SIGUSR2 << 8) | 0x7f)
        self.broker.trace.assert_called_once_with(7, 10, data=signal.SIGUSR2)

    def test_exec_checks_retirement_and_reinstalls_breakpoints(self):
        def trace(request, tid, address=0, data=0):
            if request == 0x4201:
                data._obj.value = 10
        self.broker.trace.side_effect = trace
        self.broker.arm = Mock()
        self.broker.event(10, (4 << 16) | (signal.SIGTRAP << 8) | 0x7f)
        self.broker.arm.assert_called_once_with(10)
        self.broker.trace.assert_any_call(7, 10, data=0)


if __name__ == '__main__':
    unittest.main()
