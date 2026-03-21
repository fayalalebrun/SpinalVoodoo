if { ![ info exists qsys_name ] } {
  set qsys_name "de10_core_pll"
}

if { ![ info exists devicefamily ] } {
  set devicefamily "Cyclone V"
}

if { ![ info exists device ] } {
  set device "5CSEBA6U23I7"
}

package require -exact qsys 24.1

create_system $qsys_name

set_project_property DEVICE_FAMILY $devicefamily
set_project_property DEVICE $device

add_instance pll_0 altera_pll
set_instance_parameter_value pll_0 {gui_reference_clock_frequency} {50.0}
set_instance_parameter_value pll_0 {gui_use_locked} {1}
set_instance_parameter_value pll_0 {gui_number_of_clocks} {1}
set_instance_parameter_value pll_0 {gui_output_clock_frequency0} {50.0}
set_instance_parameter_value pll_0 {gui_operation_mode} {direct}
set_instance_parameter_value pll_0 {gui_feedback_clock} {Global Clock}
set_instance_property pll_0 AUTO_EXPORT {true}

save_system hw/de10/qsys/${qsys_name}.qsys
