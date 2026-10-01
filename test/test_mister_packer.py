from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MisterPackerTest(unittest.TestCase):
    def test_real_macros_and_parameter_builder(self):
        header = (ROOT / 'emu/glide/glide2x/sst1/glide/src/fxglide.h').read_text(encoding='latin1')
        marker = header.index('  #define PACKER_WORKAROUND_SIZE 4')
        begin = header.rfind('\n#if ', 0, marker) + 1
        end = header.index('\n#endif', marker) + len('\n#endif')
        packer = header[begin:end] + '\n'
        self.assertIn('PACKER_BUGCHECK', packer)
        source = (ROOT / 'emu/glide/glide2x/sst1/glide/src/gglide.c').read_text(encoding='latin1')
        signature = 'GR_DDFUNC(_grRebuildDataList, void, ( void ))'
        begin = source.rindex(signature)
        end = source.index('} /* _grRebuildDataList */', begin) + len('} /* _grRebuildDataList */')
        function = source[begin:end] + '\n'
        self.assertIn('curTriSizeNoGradient', function)
        parent = ROOT / 'output/mister/packer-tests'
        parent.mkdir(parents=True, exist_ok=True)
        build = Path(tempfile.mkdtemp(dir=parent))
        (build / 'packer-header.inc').write_text(packer)
        (build / 'packer-list.inc').write_text(function)
        executable = build / 'packer-test'
        subprocess.run(['cc', '-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(build), str(ROOT / 'emu/mister/glide_packer_test.c'),
                        '-o', str(executable)], check=True, capture_output=True, text=True)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('512 parameter/CPU combinations', result.stdout)


if __name__ == '__main__':
    unittest.main()
