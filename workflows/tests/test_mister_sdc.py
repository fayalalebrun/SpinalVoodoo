from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
proc get_clocks {pattern} {
    if {$pattern eq "*"} {return {pixel core hdmi}}
    if {$pattern eq "FPGA_CLK2_50"} {
        if {$::scenario == 1} {return {}}
        return core
    }
    return pixel
}
proc get_collection_size {collection} {llength $collection}
proc remove_from_collection {collection removed} {
    set result {}
    foreach element $collection {if {$element ni $removed} {lappend result $element}}
    return $result
}
proc get_registers {pattern} {
    if {$pattern eq "cfg_done" && $::scenario == 3} {return {}}
    if {[string first asyncAssertSyncDeassert $pattern] >= 0} {
        if {$::scenario == 4} {return {}}
        return [list ${pattern}0 ${pattern}1]
    }
    if {[string first pushToPop $pattern] >= 0} {return $::push}
    if {[string first popToPush $pattern] >= 0} {return $::pop}
    if {[string first pushPtrGray $pattern] >= 0 || [string first ptrToPush $pattern] >= 0} {
        if {$::scenario == 2} {return [lrange $::push 0 9]}
        return $::push
    }
    if {$pattern eq {*|core|*buffercc|buffers_0*}} {return [concat scalar $::push $::pop]}
    return [list $pattern]
}
foreach name {set_clock_groups set_max_delay set_min_delay set_net_delay set_max_skew set_false_path} {
    proc $name args [format {lappend ::calls(%s) $args} $name]
}
set push {}
set pop {}
for {set i 0} {$i < 11} {incr i} {lappend push push$i; lappend pop pop$i}
'''
CHECK = r'''
if {[lindex $calls(set_false_path) 0] ne {-to scalar}} {error "Gray paths were cut"}
set cuts $calls(set_false_path)
if {[llength $cuts] != 23} {error "missing scoped CDC exceptions"}
foreach cut [lrange $cuts 1 end] {
    if {[string first Gray $cut] >= 0} {error "Gray paths were cut"}
    if {[lsearch -exact $cut -from] < 0 || [lsearch -exact $cut -to] < 0} {error "unscoped CDC exception"}
}
if {{-from cfg_done -to pixel} ni $cuts || {-from pixel -to vs_r} ni $cuts ||
    {-from reset_req -to ascal|i_reset_na} ni $cuts} {error "missing framework crossings"}
if {[lindex $calls(set_min_delay) 0 end] != -20.0} {error "asynchronous Gray hold used clock phase"}
if {$calls(set_clock_groups) ne {{-asynchronous -group pixel -group hdmi}}} {error "Core/pixel clocks were cut"}
foreach name {set_max_delay set_min_delay set_net_delay set_max_skew} {
    if {[llength $calls($name)] != 2} {error "missing bidirectional Gray bounds"}
}
'''


class MisterSdcTest(unittest.TestCase):
    def test_gray_crossings_remain_timed_and_missing_endpoints_fail(self):
        source = (ROOT / "hardware/platforms/mister/quartus/SpinalVoodoo.sdc").read_text()
        for scenario in (0, 1, 2, 3, 4):
            script = f"set scenario {scenario}\n" + HARNESS
            script += "if {[catch {\n" + source + CHECK + "\n} message]} {puts stderr $message; exit 1}\n"
            result = subprocess.run(["tclsh"], input=script, text=True, capture_output=True)
            if scenario:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn({1: "Expected", 2: "Gray-pointer", 3: "framework", 4: "reset"}[scenario], result.stderr)
            else:
                self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
