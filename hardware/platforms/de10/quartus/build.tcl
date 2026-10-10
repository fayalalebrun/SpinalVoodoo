package require ::quartus::project
package require ::quartus::flow

set project_name "SpinalVoodoo_de10"

if {[llength $quartus(args)] >= 1} {
  set project_name [lindex $quartus(args) 0]
}

puts "Opening project ${project_name}"
project_open -revision ${project_name} ${project_name}

puts "Running compile flow"
execute_flow -compile

project_close
puts "Compile flow complete"
