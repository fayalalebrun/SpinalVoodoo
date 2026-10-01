set pixel_clock [get_clocks {emu|pixel_pll|*|divclk}]
set core_clock [get_clocks FPGA_CLK2_50]
if {[get_collection_size $pixel_clock] != 1 || [get_collection_size $core_clock] != 1} {
  error "Expected one MiSTer core clock and one pixel clock"
}
set other_clocks [remove_from_collection [get_clocks *] $pixel_clock]
set other_clocks [remove_from_collection $other_clocks $core_clock]
set_clock_groups -asynchronous -group $pixel_clock -group $other_clocks
set synchronizers [get_registers {*|core|*buffercc|buffers_0*}]
foreach {source destination} {
  *|pixels|pushCC_pushPtrGray* *|pixels|pushToPopGray_buffercc|buffers_0*
  *|pixels|popCC_ptrToPush* *|pixels|popToPushGray_buffercc|buffers_0*
} {
  set src [get_registers $source]
  set dst [get_registers $destination]
  if {[get_collection_size $src] != 11 || [get_collection_size $dst] != 11} {
    error "Missing MiSTer scanout Gray-pointer crossing: $source -> $destination"
  }
  set synchronizers [remove_from_collection $synchronizers $dst]
  set_max_delay -from $src -to $dst 10.0
  set_min_delay -from $src -to $dst -20.0
  set_net_delay -from $src -to $dst -max 10.0
  set_max_skew -from $src -to $dst 5.0
}
if {[get_collection_size $synchronizers] == 0} {error "Missing MiSTer scalar synchronizers"}
set_false_path -to $synchronizers
foreach pattern {
  LFB_EN LFB_FLT FREESCALE HDMI_PR lowlat cfg_done subcarrier
  vga_osd|info vga_osd|infoh[*] vga_osd|infow[*] vga_osd|osd_h[*] vga_osd|osd_w[*] vga_osd|osd_enable
} {
  set registers [get_registers $pattern]
  if {[get_collection_size $registers] == 0} {error "Missing MiSTer framework configuration: $pattern"}
  set_false_path -from $registers -to $pixel_clock
}
foreach pattern {vs_d0 vs_d1 vsd vs_r vs_old emu|hps_io|video_calc|dout*} {
  set registers [get_registers $pattern]
  if {[get_collection_size $registers] == 0} {error "Missing MiSTer framework video sampling: $pattern"}
  set_false_path -from $pixel_clock -to $registers
}
set scaler_reset [get_registers ascal|i_reset_na]
set framework_reset [get_registers reset_req]
if {[get_collection_size $scaler_reset] != 1 || [get_collection_size $framework_reset] != 1} {
  error "Missing MiSTer framework scaler reset synchronizer"
}
set_false_path -from $framework_reset -to $scaler_reset
foreach {source destination} {
  *|boundary|resetting *|pixels|*asyncAssertSyncDeassert_buffercc|buffers_*
  *|io_coreReset_buffercc|buffers_1 *|pixel_scan_io_newFrame_pulseCCByToggle|*asyncAssertSyncDeassert_buffercc|buffers_*
} {
  set src [get_registers $source]
  set dst [get_registers $destination]
  if {[get_collection_size $src] != 1 || [get_collection_size $dst] != 2} {
    error "Missing MiSTer asynchronous reset synchronizer: $source -> $destination"
  }
  set_false_path -from $src -to $dst
}
