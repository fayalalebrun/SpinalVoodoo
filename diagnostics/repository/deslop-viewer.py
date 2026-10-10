#!/usr/bin/env python3

import argparse
import csv
import gzip
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIELDS = ('path', 'kind', 'tracked', 'bytes', 'category', 'reason', 'direct_consumers')


def build_tree(source):
    categories, reasons, nodes, indexes = [], [], [], {}
    category_ids, reason_ids = {}, {}

    def intern(value, table, ids):
        if value not in ids:
            ids[value] = len(table)
            table.append(value)
        return ids[value]

    def directory(path):
        if not path:
            return -1
        if path not in indexes:
            parent, _, name = path.rpartition('/')
            parent_id = directory(parent)
            indexes[path] = len(nodes)
            nodes.append([name, parent_id, 'directory', 0, 0,
                          intern('MIXED-CONTAINER', categories, category_ids),
                          intern('Directory boundary; child classifications apply', reasons, reason_ids), ''])
        return indexes[path]

    with gzip.open(source, 'rt', newline='') as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != list(FIELDS):
            raise ValueError('Unexpected inventory columns')
        seen = set()
        for row in reader:
            path = row['path']
            if not path or path.startswith('/') or any(p in ('', '.', '..') for p in path.split('/')) or path in seen:
                raise ValueError('Invalid or duplicate inventory path')
            seen.add(path)
            size = int(row['bytes'])
            if size < 0 or row['tracked'] not in ('True', 'False') or not row['category'] or not row['reason']:
                raise ValueError('Invalid inventory metadata')
            parent, _, name = path.rpartition('/')
            parent_id = directory(parent)
            node = [name, parent_id, row['kind'], int(row['tracked'] == 'True'), size,
                    intern(row['category'], categories, category_ids),
                    intern(row['reason'], reasons, reason_ids), row['direct_consumers']]
            if path in indexes:
                nodes[indexes[path]] = node
            else:
                indexes[path] = len(nodes)
                nodes.append(node)
    return {'nodes': nodes, 'categories': categories, 'reasons': reasons,
            'snapshot': datetime.fromtimestamp(source.stat().st_mtime, timezone.utc).isoformat()}


def render_page(source, template, destination):
    data = json.dumps(build_tree(source), ensure_ascii=True, separators=(',', ':')).replace('<', '\\u003c')
    page = template.read_text()
    if page.count('__INVENTORY__') != 1:
        raise ValueError('Template requires exactly one inventory placeholder')
    temporary = destination.with_suffix('.tmp')
    temporary.write_text(page.replace('__INVENTORY__', data))
    temporary.replace(destination)


def main():
    parser = argparse.ArgumentParser(description='Generate a self-contained static classification tree')
    parser.add_argument('--refresh', action='store_true', help='Re-enumerate workspace before generation')
    options = parser.parse_args()
    if options.refresh:
        subprocess.run(['python3', '-B', str(ROOT / 'diagnostics/repository/deslop_inventory.py')], cwd=ROOT, check=True)
    output = ROOT / 'output/deslop-audit'
    destination = output / 'viewer.html'
    render_page(output / 'all-paths.csv.gz', ROOT / 'diagnostics/repository/deslop-viewer.html', destination)
    print(destination.as_uri())


if __name__ == '__main__':
    main()
