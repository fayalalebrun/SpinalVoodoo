package require ::quartus::jtag
package require ::quartus::flow

if {[llength $quartus(args)] < 1} {
  puts "Usage: quartus_pgm -t program.tcl <sof-file>"
  qexit -error
}

set sof_file [lindex $quartus(args) 0]

if {![file exists $sof_file]} {
  puts "SOF file not found: $sof_file"
  qexit -error
}

set hardware_names [get_hardware_names]
if {[llength $hardware_names] == 0} {
  puts "No JTAG hardware found"
  qexit -error
}

set hardware_name [lindex $hardware_names 0]
set device_names [get_device_names -hardware_name $hardware_name]
if {[llength $device_names] == 0} {
  puts "No JTAG devices found on hardware: $hardware_name"
  qexit -error
}

set device_name [lindex $device_names 0]

puts "Programming $device_name on $hardware_name with $sof_file"
begin_memory_edit -hardware_name $hardware_name -device_name $device_name
end_memory_edit

execute_module -tool quartus_pgm -- "-c" "$hardware_name" "-m" "jtag" "-o" "p;$sof_file"

puts "Programming complete"
