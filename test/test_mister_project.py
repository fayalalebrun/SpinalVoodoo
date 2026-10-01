import importlib.machinery
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
LOADER = importlib.machinery.SourceFileLoader("prepare_project", str(ROOT / "scripts/prepare-mister-project"))
SPEC = importlib.util.spec_from_loader(LOADER.name, LOADER)
MODULE = importlib.util.module_from_spec(SPEC)
LOADER.exec_module(MODULE)


class MisterProjectTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "output")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.destination = self.root / "output/project"
        self.template = self.root / "output/mister/sources/template"
        self.source = {"revision": "a" * 40, "url": "https://github.com/test/template.git", "license": "LICENSE"}
        self.files = {
            "hw/mister/sources.json": json.dumps({"template": self.source}),
            "hw/mister/xls-files.f": "xls/build/CoreSimDe10.sv\n",
            "xls/build/CoreSimDe10.sv": "original XLS\n",
            "output/mister/rtl/MisterCore.v": "platform\n",
            "hw/mister/SpinalVoodoo.sv": "shell\n",
            "hw/mister/SpinalVoodoo.qsf": "settings\n",
            "hw/mister/SpinalVoodoo.sdc": "timing\n",
            "hw/mister/report_timing.tcl": "reports\n",
            "hw/de10/qsys/generated/hdmi_pll/synthesis/de10_hdmi_pll.qip": "pll\n",
            "output/mister/sources/template/LICENSE": "license\n",
            "output/mister/sources/template/sys/sys.qip": "upstream\n",
            "output/mister/sources/template/sys/pll_q17.qip": "upstream PLL\n",
            "output/mister/sources/template/rtl/pll.qip": "template PLL\n",
        }
        for name, content in self.files.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
        (self.template / ".git").mkdir()
        (self.root / "scripts").mkdir()
        shutil.copy2(ROOT / "scripts/prepare-mister-sources", self.root / "scripts/prepare-mister-sources")
        self.addCleanup(patch.stopall)
        patch.object(MODULE, "ROOT", self.root).start()

    def git(self, command, **kwargs):
        return {("rev-parse", "--show-toplevel"): str(self.template),
                ("rev-parse", "HEAD"): self.source["revision"],
                ("remote", "get-url", "origin"): self.source["url"],
                ("status", "--porcelain", "--untracked-files=all"): ""}[tuple(command[3:])]

    def test_sources_are_preserved_and_compatibility_alias_is_project_only(self):
        with patch.object(MODULE.subprocess, "check_output", side_effect=self.git), \
             patch.object(MODULE.subprocess, "run") as run:
            MODULE.prepare(self.destination)
        self.assertEqual((self.destination / "sys/sys.qip").read_text(), "upstream\n")
        self.assertEqual((self.destination / "sys/pll_q24.qip").resolve(), self.template / "sys/pll_q17.qip")
        self.assertFalse((self.template / "sys/pll_q24.qip").exists())
        self.assertEqual((self.destination / "generated/CoreSimDe10.sv").read_text(), "original XLS\n")
        self.assertIn("generated/MisterCore.v", (self.destination / "files.qip").read_text())
        run.assert_called_once_with([str(self.root / "scripts/quartus-compat-xls-rtl"),
                                    str(self.destination / "generated/CoreSimDe10.sv"), "--ram-fifos"], check=True)
        for name, content in self.files.items():
            self.assertEqual((self.root / name).read_text(), content)
        with self.assertRaisesRegex(ValueError, "already exists"):
            MODULE.prepare(self.destination)

    def test_missing_input_and_external_destination_fail_before_project_creation(self):
        with self.assertRaisesRegex(ValueError, "inside the workspace"):
            MODULE.prepare(self.root / "external")
        (self.root / "output/mister/rtl/MisterCore.v").unlink()
        with patch.object(MODULE.subprocess, "check_output", side_effect=self.git):
            with self.assertRaisesRegex(ValueError, "missing generated input"):
                MODULE.prepare(self.destination)
        self.assertFalse(self.destination.exists())

    def test_dirty_framework_fails_without_creating_project(self):
        with patch.object(MODULE.subprocess, "check_output", return_value="wrong"):
            with self.assertRaises(ValueError):
                MODULE.prepare(self.destination)
        self.assertFalse(self.destination.exists())


if __name__ == "__main__":
    unittest.main()
