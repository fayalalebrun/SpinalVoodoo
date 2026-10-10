import importlib.machinery
import importlib.util
import io
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[3]
loader = importlib.machinery.SourceFileLoader('mister_keyboard', str(ROOT / 'diagnostics/board/mister-keyboard'))
spec = importlib.util.spec_from_loader(loader.name, loader)
module = importlib.util.module_from_spec(spec)
loader.exec_module(module)


class VideoFramesTest(unittest.TestCase):
    def test_partial_frames_and_latest_complete_frame(self):
        frames = module.VideoFrames(3)
        self.assertIsNone(frames.feed(b'ab'))
        self.assertEqual(frames.feed(b'cdefg'), b'def')
        self.assertEqual(frames.feed(b'hi'), b'ghi')
        self.assertIsNone(frames.feed(b''))
        self.assertFalse(frames.pending)


class KeyboardTest(unittest.TestCase):
    def test_held_keys_repeat_and_focus_release(self):
        stream = io.BytesIO()
        keyboard = module.Keyboard(stream)
        keyboard.key(225, True)
        keyboard.key(4, True)
        keyboard.key(4, True)
        keyboard.key(999, True)
        keyboard.release()
        keyboard.release()
        self.assertEqual(list(struct.iter_unpack('<HB', stream.getvalue())), [(42, 1), (30, 1), (30, 0), (42, 0)])
        self.assertFalse(keyboard.held)

    def test_key_up_and_disconnect(self):
        stream = io.BytesIO()
        keyboard = module.Keyboard(stream)
        keyboard.key(82, True)
        keyboard.key(82, False)
        keyboard.key(82, False)
        self.assertEqual(list(struct.iter_unpack('<HB', stream.getvalue())), [(103, 1), (103, 0)])
        stream.close()
        with self.assertRaises(ValueError):
            keyboard.key(4, True)
        self.assertFalse(keyboard.held)


if __name__ == '__main__':
    unittest.main()
