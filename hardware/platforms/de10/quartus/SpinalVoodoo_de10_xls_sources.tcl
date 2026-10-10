rename set_global_assignment xls_set_global_assignment
proc set_global_assignment {args} {
  if {[lsearch -exact $args "../../../../build/generated/scala/de10/De10Top.v"] >= 0} {
    return
  }
  uplevel 1 [list xls_set_global_assignment {*}$args]
}
source SpinalVoodoo_de10.qsf
rename set_global_assignment {}
rename xls_set_global_assignment set_global_assignment
