import importlib.machinery
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
LOADER = importlib.machinery.SourceFileLoader("pipeline_stage", str(ROOT / "scripts/select-xls-pipeline-stage"))
SPEC = importlib.util.spec_from_loader(LOADER.name, LOADER)
MODULE = importlib.util.module_from_spec(SPEC)
LOADER.exec_module(MODULE)


class PipelineStageTest(unittest.TestCase):
    def setUp(self):
        self.old = "module __core_sim__hosted_pixel_texture_requests_0__0_next(input wire clk);\nwire old;\nendmodule"
        self.new = self.old.replace("wire old", "wire new")
        self.other = "module unrelated(input wire clk);\nendmodule\n"

    def test_selects_only_the_pipeline_and_is_idempotent(self):
        base = self.other + self.old + "\n" + self.other.replace("unrelated", "last")
        selected = MODULE.select(base, self.new + "\n" + self.other)
        self.assertEqual(selected, base.replace(self.old, self.new))
        self.assertEqual(MODULE.select(selected, self.new), selected)
        self.assertEqual(MODULE.select(self.old, self.new.replace("wire clk", "wire  clk")),
                         self.new.replace("wire clk", "wire  clk"))

    def test_selects_controller_without_changing_texture_pipeline(self):
        old = self.old.replace("hosted_pixel_texture_requests", "core_sim_hosted_controller")
        new = old.replace("wire old", "wire new")
        selected = MODULE.select(self.old + "\n" + old, new, "__core_sim__core_sim_hosted_controller_")
        self.assertEqual(selected, self.old + "\n" + new)

    def test_rejects_missing_duplicate_or_changed_interfaces(self):
        for base, replacement in ((self.other, self.new), (self.old, self.other),
                                  (self.old + "\n" + self.old, self.new),
                                  (self.old, self.new + "\n" + self.new),
                                  (self.old, self.new.replace("wire clk", "wire [1:0] clk")),
                                  (self.old, self.new.replace("requests_0", "requests_1"))):
            with self.assertRaises(ValueError):
                MODULE.select(base, replacement)


if __name__ == "__main__":
    unittest.main()
