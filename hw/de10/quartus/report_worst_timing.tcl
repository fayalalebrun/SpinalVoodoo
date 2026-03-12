package require ::quartus::project

set project_name "SpinalVoodoo_de10"
set revision_name "SpinalVoodoo_de10"

if {[llength $quartus(args)] >= 1} {
  set project_name [lindex $quartus(args) 0]
}
if {[llength $quartus(args)] >= 2} {
  set revision_name [lindex $quartus(args) 1]
}

project_open -revision $revision_name $project_name
create_timing_netlist
read_sdc
update_timing_netlist

puts "==== Worst setup paths ===="
report_timing -setup -npaths 3 -detail full_path

puts "==== Worst hold paths ===="
report_timing -hold -npaths 3 -detail full_path

delete_timing_netlist
project_close
