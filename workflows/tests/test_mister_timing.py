import importlib.machinery
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
LOADER = importlib.machinery.SourceFileLoader("mister_timing", str(ROOT / "workflows/check/check-mister-timing"))
SPEC = importlib.util.spec_from_loader(LOADER.name, LOADER)
MODULE = importlib.util.module_from_spec(SPEC)
LOADER.exec_module(MODULE)


class MisterTimingTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "output")
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.project = root / "candidate"
        self.output = self.project / "output_files"
        self.output.mkdir(parents=True)
        (root / "candidate.status").write_text("0")
        (root / "candidate.elapsed").write_text("899")
        (self.output / "SpinalVoodoo.rbf").write_bytes(b"rbf")
        self.summary = "".join(f"Type : {model} 1100mV {temp}C Model {kind} 'FPGA_CLK2_50'\nSlack : 1.0\nTNS : 0.000\n\n"
                               for model in ("Slow", "Fast") for temp in ("100", "-40") for kind in ("Setup", "Hold"))
        (self.output / "SpinalVoodoo.sta.summary").write_text(self.summary)
        net = "; Name ; Slack ; Required ; Actual ; From ; To ;\n"
        for source, destination in (("pushCC_pushPtrGray", "pushToPopGray_buffercc"),
                                    ("popCC_ptrToPush", "popToPushGray_buffercc")):
            for bit in range(11):
                net += f"; -- ; 8.0 ; 10.0 ; 2.0 ; {source}[{bit}] ; {destination}|buffers_0[{bit}] ;\n"
        (self.output / "mister_gray_net_delay.rpt").write_text(net)
        (self.output / "mister_gray_skew.rpt").write_text(net)

    def test_accepts_complete_reports(self):
        result = MODULE.check(self.project)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["elapsed_seconds"], 899)
        self.assertEqual(result["net_delay_slack_ns"], 8.0)

    def test_rejects_timeout_failure_missing_corners_and_violations(self):
        cases = ((self.project.parent / "candidate.status", "124"),
                 (self.project.parent / "candidate.elapsed", "901"),
                 (self.output / "SpinalVoodoo.sta.summary", ""),
                 (self.output / "SpinalVoodoo.sta.summary", self.summary.replace("Slack : 1.0", "Slack : -0.1", 1)),
                 (self.output / "SpinalVoodoo.sta.summary", self.summary.replace("Slack : 1.0", "Slack : nan", 1)),
                 (self.output / "SpinalVoodoo.sta.summary", self.summary.replace("Fast", "Other")),
                 (self.output / "SpinalVoodoo.sta.summary", self.summary + self.summary),
                 (self.output / "mister_gray_skew.rpt", ""),
                 (self.output / "mister_gray_net_delay.rpt", "; Name ; Slack ;\n; -- ; 1.0 ;\n"))
        for path, text in cases:
            old = path.read_text()
            path.write_text(text)
            with self.assertRaises(ValueError):
                MODULE.check(self.project)
            path.write_text(old)

    def test_report_slacks_rejects_empty_negative_and_nonfinite_values(self):
        self.assertEqual(MODULE.report_slacks("; Slack ; 2.0 (MET) ;\n"), 2.0)
        for text in ("", "; Slack ; ;", "; Slack ; -0.1 (VIOLATED) ;", "; Slack ; inf ;", "; Name ; Slack ;\n; -- ; -1 ;"):
            with self.assertRaises(ValueError):
                MODULE.report_slacks(text)


if __name__ == "__main__":
    unittest.main()
