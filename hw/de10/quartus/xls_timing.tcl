create_timing_netlist -force_dat
read_sdc
update_timing_netlist
report_clock_fmax_summary -panel_name "Clock Fmax Summary"
report_timing -setup -npaths 10 -detail summary -panel_name "Worst Setup Paths"
report_timing -hold -npaths 10 -detail summary -panel_name "Worst Hold Paths"
