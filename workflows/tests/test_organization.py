import importlib.machinery
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
loader = importlib.machinery.SourceFileLoader('de10_project', str(ROOT / 'workflows/build/prepare-de10-project'))
spec = importlib.util.spec_from_loader(loader.name, loader)
module = importlib.util.module_from_spec(spec)
loader.exec_module(module)


class OrganizationTest(unittest.TestCase):
    def test_private_project_resolves_inputs_without_modifying_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'hardware/platforms/de10/quartus'
            source.mkdir(parents=True)
            original = 'set_global_assignment -name VERILOG_FILE ../../../../build/generated/scala/de10/De10Top.v\n'
            (source / 'core.qsf').write_text(original)
            with patch.object(module, 'ROOT', root):
                destination = module.prepare(root / 'build/quartus/de10')
                self.assertIn(str(root / 'build/generated/scala/de10/De10Top.v'), (destination / 'core.qsf').read_text())
                self.assertEqual((source / 'core.qsf').read_text(), original)
                with self.assertRaises(ValueError):
                    module.prepare(destination)
                with self.assertRaises(ValueError):
                    module.prepare(root / 'external')

    def test_authored_file_lists_and_generators_use_new_boundaries(self):
        names = (ROOT / 'hardware/platforms/mister/quartus/xls-files.f').read_text().splitlines()
        for name in names:
            self.assertTrue(name.startswith(('hardware/', 'build/generated/')), name)
            if name.startswith('hardware/'):
                self.assertTrue((ROOT / name).is_file(), name)
        qsf = (ROOT / 'hardware/platforms/de10/quartus/SpinalVoodoo_de10_xls.qsf').read_text()
        self.assertNotIn('build/generated/build/generated', qsf)
        makefile = (ROOT / 'software/runtime/mister/Makefile').read_text()
        self.assertIn("sed 's/-I/-isystem /g'", makefile)
        self.assertIn('-Werror -pedantic', makefile)
        self.assertIn('build/host/mister', makefile)


if __name__ == '__main__':
    unittest.main()
