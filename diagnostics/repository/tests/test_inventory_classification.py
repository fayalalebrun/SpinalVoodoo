import ast
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]


class InventoryClassificationTest(unittest.TestCase):
    def setUp(self):
        tree = ast.parse((ROOT / 'diagnostics/repository/deslop_inventory.py').read_text())
        function = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == 'classify')
        namespace = dict(Path=Path, root=ROOT, archived={}, tracked=set(), cache_parts=set(), review_tools=set())
        exec(compile(ast.Module(body=[function], type_ignores=[]), 'classify', 'exec'), namespace)
        self.classify = namespace['classify']

    def test_migration_boundaries_preserve_dependency_and_evidence(self):
        for path, category in (
            ('build/deps/mister/template/sys/ascal.vhd', 'KEEP-DEPENDENCY'),
            ('build/generated/xls/rtl/core.v', 'GENERATED'),
            ('build/host/mister/test', 'REMOVE-CACHE'),
            ('output/mister/signed.rbf', 'ARCHIVE-EVIDENCE'),
            ('releases/generated-baseline/core.v', 'ARCHIVE-EVIDENCE'),
            ('experiments/organization-migration/moves.json', 'ARCHIVE-EVIDENCE'),
            ('hardware/shared/scala/Config.scala', 'KEEP-SOURCE'),
            ('software/runtime/mister/tests/test.c', 'KEEP-TEST'),
            ('verification/integration/test.sv', 'KEEP-TEST'),
            ('workflows/build/generate-xls-rtl', 'KEEP-TOOL'),
            ('unknown-local-file', 'REVIEW-LOCAL'),
            ('build/deps/mister/template/.git/config', 'KEEP-METADATA'),
        ):
            with self.subTest(path=path):
                self.assertEqual(self.classify(path)[0], category)
        self.assertIn('clean', self.classify('build/deps/mister/template/sys/ascal.vhd')[1])

    def test_known_containers_and_guest_binaries_are_not_unknown(self):
        for category, paths in (
            ('KEEP-DOC', ('.learning', 'docs')),
            ('KEEP-SOURCE', ('hardware', 'software', 'environment')),
            ('KEEP-TOOL', ('workflows', 'diagnostics')),
            ('KEEP-DEPENDENCY', ('vendor', 'vendor/patches', 'vendor/patches/xls', 'vendor/stb')),
            ('KEEP-TEST', ('verification', 'verification/integration')),
            ('KEEP-FIXTURE', ('verification/fixtures', 'traces', 'traces/logo')),
            ('MIXED-CONTAINER', ('build', 'releases', 'experiments')),
        ):
            for path in paths:
                with self.subTest(path=path):
                    self.assertEqual(self.classify(path, is_dir=True)[0], category)
        for name in ('df00sdk.exe', 'df02sdk.exe', 'test00.exe'):
            self.assertEqual(self.classify('vendor/glide/glide2x/sst1/glide/tests/' + name)[0], 'KEEP-TEST')
        self.assertEqual(self.classify('unknown-directory', is_dir=True)[0], 'REVIEW-LOCAL')
        self.assertEqual(self.classify('hardware')[0], 'REVIEW-LOCAL')
        self.assertEqual(self.classify('vendor/glide/unknown-local-file')[0], 'REVIEW-LOCAL')


if __name__ == '__main__':
    unittest.main()
