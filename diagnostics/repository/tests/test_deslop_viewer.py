import csv
import gzip
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('deslop_viewer', ROOT / 'diagnostics/repository/deslop-viewer.py')
viewer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(viewer)


class DeslopViewerTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.source = self.base / 'inventory.csv.gz'
        self.rows = [
            ('src/deep/b.py', 'file', 'False', 90, 'REVIEW-LOCAL', 'Ownership unknown', ''),
            ('src', 'directory', 'False', 0, 'KEEP-SOURCE', 'Source container', ''),
            ('src/a.py', 'file', 'True', 12, 'KEEP-SOURCE', 'Maintained source', 'Makefile'),
            ('old.py', 'missing', 'True', 0, 'REMOVED-ARCHIVED', 'Verified archive', ''),
            ('</script><script>.py', 'file', 'False', 1, 'REVIEW-LOCAL', 'Literal name', ''),
        ]
        self.save()

    def save(self, fields=viewer.FIELDS):
        with gzip.open(self.source, 'wt', newline='') as stream:
            writer = csv.writer(stream)
            writer.writerow(fields)
            writer.writerows(self.rows)

    def test_tree_parents_precede_children_and_preserve_metadata(self):
        data = viewer.build_tree(self.source)
        paths = []
        for node in data['nodes']:
            name, parent = node[:2]
            self.assertLess(parent, len(paths))
            paths.append(name if parent < 0 else paths[parent] + '/' + name)
        self.assertEqual(set(paths), {row[0] for row in self.rows} | {'src/deep', '<'})
        source = data['nodes'][paths.index('src')]
        self.assertEqual(data['reasons'][source[6]], 'Source container')
        file = data['nodes'][paths.index('src/a.py')]
        self.assertEqual(file[3:5], [1, 12])
        self.assertEqual(file[7], 'Makefile')

    def test_repeated_classifications_are_interned(self):
        data = viewer.build_tree(self.source)
        self.assertEqual(data['categories'].count('KEEP-SOURCE'), 1)
        self.assertEqual(data['categories'].count('REVIEW-LOCAL'), 1)

    def test_invalid_inventory(self):
        for path in ('', '/etc', '../a', 'a//b', 'a/./b'):
            with self.subTest(path=path):
                self.rows = [(path, 'file', 'True', 0, 'KEEP', 'Reason', '')]
                self.save()
                with self.assertRaises(ValueError):
                    viewer.build_tree(self.source)
        self.save(fields=['wrong'])
        with self.assertRaises(ValueError):
            viewer.build_tree(self.source)

    def test_duplicate_paths_and_negative_sizes(self):
        self.rows.append(self.rows[0])
        self.save()
        with self.assertRaises(ValueError):
            viewer.build_tree(self.source)
        self.rows = [('a', 'file', 'True', -1, 'KEEP', 'Reason', '')]
        self.save()
        with self.assertRaises(ValueError):
            viewer.build_tree(self.source)

    def test_static_page_escapes_data_and_preserves_output_on_failure(self):
        destination = self.base / 'viewer.html'
        viewer.render_page(self.source, ROOT / 'diagnostics/repository/deslop-viewer.html', destination)
        page = destination.read_text()
        embedded = page.split('<script id="inventory" type="application/json">')[1].split('</script>')[0]
        self.assertNotIn('<', embedded)
        self.assertEqual(len(json.loads(embedded)['nodes']), 7)
        self.assertNotIn('fetch(', page)
        self.assertNotIn('__INVENTORY__', page)
        broken = self.base / 'broken.html'
        broken.write_text('no placeholder')
        with self.assertRaises(ValueError):
            viewer.render_page(self.source, broken, destination)
        self.assertEqual(destination.read_text(), page)


if __name__ == '__main__':
    unittest.main()
