package require ::quartus::project

set project_name "SpinalVoodoo_de10"
set revision_name "SpinalVoodoo_de10"

if {[llength $quartus(args)] >= 1} {
  set project_name [lindex $quartus(args) 0]
}
if {[llength $quartus(args)] >= 2} {
  set revision_name [lindex $quartus(args) 1]
}

set core_clock_name "core_pll_0|pll_0|altera_pll_i|general[0].gpll~PLL_OUTPUT_COUNTER|divclk"

project_open -revision $revision_name $project_name
create_timing_netlist
read_sdc
update_timing_netlist

set core_clock [get_clocks $core_clock_name]

puts "==== Fmax summary ===="
report_clock_fmax_summary

puts "==== Core setup paths ===="
report_timing -setup -from_clock $core_clock -to_clock $core_clock -npaths 5 -detail full_path

puts "==== Core hold paths ===="
report_timing -hold -from_clock $core_clock -to_clock $core_clock -npaths 5 -detail full_path

delete_timing_netlist
project_close
