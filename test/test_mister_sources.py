import copy
import importlib.machinery
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
LOADER = importlib.machinery.SourceFileLoader("prepare_mister", str(ROOT / "scripts/prepare-mister-sources"))
SPEC = importlib.util.spec_from_loader(LOADER.name, LOADER)
MODULE = importlib.util.module_from_spec(SPEC)
LOADER.exec_module(MODULE)


class MisterSourcesTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "output")
        self.addCleanup(self.temp.cleanup)
        self.destination = Path(self.temp.name)
        self.source = json.loads((ROOT / "hw/mister/sources.json").read_text())["template"]
        self.checkout = self.destination / "template"

    def checkout_source(self):
        (self.checkout / ".git").mkdir(parents=True)
        (self.checkout / "LICENSE").write_text("fixture")

    def git_result(self, directory, *args):
        return {
            ("rev-parse", "--show-toplevel"): str(self.checkout.resolve()),
            ("rev-parse", "HEAD"): self.source["revision"],
            ("remote", "get-url", "origin"): self.source["url"],
            ("status", "--porcelain", "--untracked-files=all"): "",
            ("ls-files", "--others", "--exclude-standard", "-z"): "",
            ("diff", "--name-only", "-z", "HEAD"): "missing.v\0",
        }[args]

    def test_check_is_offline_and_checks_provenance(self):
        self.checkout_source()
        with patch.object(MODULE, "git", side_effect=self.git_result) as git:
            MODULE.prepare(self.destination, {"template": self.source}, check=True)
        self.assertEqual(git.call_count, 4)

    def test_dirty_revision_origin_and_license_fail_closed(self):
        self.checkout_source()
        for command, result in (
            (("rev-parse", "HEAD"), "0" * 40),
            (("remote", "get-url", "origin"), "https://example.invalid/repo"),
            (("status", "--porcelain", "--untracked-files=all"), " M sys/sys_top.v"),
        ):
            def run(directory, *args):
                return result if args == command else self.git_result(directory, *args)
            with self.subTest(command=command), patch.object(MODULE, "git", side_effect=run):
                with self.assertRaises(ValueError):
                    MODULE.verify(self.checkout, self.source)
        with patch.object(MODULE, "git", side_effect=self.git_result):
            with self.assertRaisesRegex(ValueError, "missing license"):
                MODULE.verify(self.checkout, dict(self.source, license="COPYING"))

    def test_upstream_crlf_blob_is_accepted_only_when_byte_identical(self):
        self.checkout_source()
        (self.checkout / "ChangeLog.md").write_bytes(b"upstream\r\n")
        for identical in (True, False):
            def run(directory, *args):
                values = {
                    ("status", "--porcelain", "--untracked-files=all"): " M ChangeLog.md",
                    ("ls-files", "--others", "--exclude-standard", "-z"): "",
                    ("diff", "--name-only", "-z", "HEAD"): "ChangeLog.md\0",
                    ("ls-tree", "HEAD", "--", "ChangeLog.md"): "100644 blob abc\tChangeLog.md",
                    ("hash-object", "--no-filters", "--", "ChangeLog.md"): "abc" if identical else "def",
                }
                return values[args] if args in values else self.git_result(directory, *args)
            with self.subTest(identical=identical), patch.object(MODULE, "git", side_effect=run):
                if identical:
                    MODULE.verify(self.checkout, self.source)
                else:
                    with self.assertRaisesRegex(ValueError, "modified checkout"):
                        MODULE.verify(self.checkout, self.source)

    def test_invalid_manifest_and_external_destination_do_not_run_git(self):
        for key, value in (("revision", "main"), ("url", "file:///tmp/repo"),
                           ("license", "../../LICENSE")):
            source = copy.copy(self.source)
            source[key] = value
            with self.subTest(key=key), patch.object(MODULE, "git") as git:
                with self.assertRaises(ValueError):
                    MODULE.prepare(self.destination, {"template": source})
                git.assert_not_called()
        with patch.object(MODULE, "git") as git:
            for destination, sources in ((Path("/tmp"), {"template": self.source}),
                                         (self.destination, {"../escape": self.source})):
                with self.assertRaises(ValueError):
                    MODULE.prepare(destination, sources)
            git.assert_not_called()

    def test_missing_source_check_and_failed_fetch_do_not_overwrite(self):
        with self.assertRaisesRegex(ValueError, "missing source"):
            MODULE.prepare(self.destination, {"template": self.source}, check=True)
        def fail_fetch(directory, *args):
            if args[0] == "fetch":
                raise subprocess.CalledProcessError(1, args)
            return ""
        with patch.object(MODULE, "git", side_effect=fail_fetch):
            with self.assertRaises(subprocess.CalledProcessError):
                MODULE.prepare(self.destination, {"template": self.source})
        self.assertTrue(self.checkout.is_dir())
        with patch.object(MODULE, "git") as git:
            with self.assertRaisesRegex(ValueError, "not a source checkout"):
                MODULE.prepare(self.destination, {"template": self.source})
            git.assert_not_called()

    def test_fetch_checks_out_only_pinned_commit(self):
        calls = []
        def run(directory, *args):
            calls.append(args)
            if args[0] == "init":
                (directory / ".git").mkdir()
                (directory / "LICENSE").write_text("fixture")
            if args[0] in ("rev-parse", "status") or args[:2] == ("remote", "get-url"):
                return self.git_result(directory, *args)
            return ""
        with patch.object(MODULE, "git", side_effect=run):
            MODULE.prepare(self.destination, {"template": self.source})
        self.assertIn(("fetch", "--depth=1", "origin", self.source["revision"]), calls)
        self.assertIn(("checkout", "--detach", "FETCH_HEAD"), calls)


if __name__ == "__main__":
    unittest.main()
