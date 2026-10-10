foreach {source destination} {
    *|pixels|pushCC_pushPtrGray* *|pixels|pushToPopGray_buffercc|buffers_0*
    *|pixels|popCC_ptrToPush* *|pixels|popToPushGray_buffercc|buffers_0*
} {
    set paths [get_timing_paths -setup -from [get_registers $source] -to [get_registers $destination] -npaths 11]
    if {[get_collection_size $paths] != 11} {error "Gray-pointer paths were cut from timing analysis"}
}
report_max_skew -multi_corner -npaths 20 -detail full_path -file output_files/mister_gray_skew.rpt
report_net_delay -multi_corner -nworst 100 -file output_files/mister_gray_net_delay.rpt
