if { ![ info exists qsys_name ] } {
  set qsys_name "de10_soc"
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

add_instance clk_0 clock_source
set_instance_parameter_value clk_0 clockFrequency {50000000.0}
set_instance_parameter_value clk_0 clockFrequencyKnown {1}
set_instance_parameter_value clk_0 resetSynchronousEdges {NONE}

add_instance hps_0 altera_hps
set_instance_parameter_value hps_0 F2S_Width {3}
set_instance_parameter_value hps_0 S2F_Width {2}
set_instance_parameter_value hps_0 LWH2F_Enable {true}
set_instance_parameter_value hps_0 HPS_PROTOCOL {DDR3}
set_instance_parameter_value hps_0 F2SDRAM_Type {Avalon-MM Bidirectional}
set_instance_parameter_value hps_0 F2SDRAM_Width {256}
set_instance_parameter_value hps_0 DAT_DATA_WIDTH {32}
set_instance_parameter_value hps_0 MEM_BANKADDR_WIDTH {3}
set_instance_parameter_value hps_0 MEM_CK_WIDTH {1}
set_instance_parameter_value hps_0 MEM_CLK_EN_WIDTH {1}
set_instance_parameter_value hps_0 MEM_CLK_FREQ {400.0}
set_instance_parameter_value hps_0 MEM_CLK_FREQ_MAX {800.0}
set_instance_parameter_value hps_0 MEM_COL_ADDR_WIDTH {10}
set_instance_parameter_value hps_0 MEM_CS_WIDTH {1}
set_instance_parameter_value hps_0 MEM_DQ_PER_DQS {8}
set_instance_parameter_value hps_0 MEM_DQ_WIDTH {32}
set_instance_parameter_value hps_0 MEM_DRV_STR {RZQ/6}
set_instance_parameter_value hps_0 MEM_IF_DM_PINS_EN {true}
set_instance_parameter_value hps_0 MEM_IF_DQSN_EN {true}
set_instance_parameter_value hps_0 MEM_ROW_ADDR_WIDTH {15}
set_instance_parameter_value hps_0 MEM_RTT_NOM {RZQ/6}
set_instance_parameter_value hps_0 MEM_RTT_WR {Dynamic ODT off}
set_instance_parameter_value hps_0 MEM_TCL {7}
set_instance_parameter_value hps_0 MEM_TFAW_NS {37.5}
set_instance_parameter_value hps_0 MEM_TINIT_US {500}
set_instance_parameter_value hps_0 MEM_TMRD_CK {4}
set_instance_parameter_value hps_0 MEM_TRAS_NS {35.0}
set_instance_parameter_value hps_0 MEM_TRCD_NS {13.75}
set_instance_parameter_value hps_0 MEM_TREFI_US {7.8}
set_instance_parameter_value hps_0 MEM_TRFC_NS {300.0}
set_instance_parameter_value hps_0 MEM_TRP_NS {13.75}
set_instance_parameter_value hps_0 MEM_TRRD_NS {7.5}
set_instance_parameter_value hps_0 MEM_TRTP_NS {7.5}
set_instance_parameter_value hps_0 MEM_TWR_NS {15.0}
set_instance_parameter_value hps_0 MEM_TWTR {4}
set_instance_parameter_value hps_0 MEM_WTCL {6}
set_instance_parameter_value hps_0 MEM_VOLTAGE {1.5V DDR3}

add_instance h2f_bridge altera_avalon_mm_bridge
add_instance fb_bridge altera_avalon_mm_bridge
add_instance tex_bridge altera_avalon_mm_bridge

set_instance_parameter_value h2f_bridge DATA_WIDTH {32}
set_instance_parameter_value h2f_bridge SYMBOL_WIDTH {8}
set_instance_parameter_value h2f_bridge HDL_ADDR_WIDTH {24}
set_instance_parameter_value h2f_bridge ADDRESS_WIDTH {24}

set_instance_parameter_value fb_bridge DATA_WIDTH {32}
set_instance_parameter_value fb_bridge SYMBOL_WIDTH {8}
set_instance_parameter_value fb_bridge HDL_ADDR_WIDTH {32}
set_instance_parameter_value fb_bridge ADDRESS_WIDTH {32}

set_instance_parameter_value tex_bridge DATA_WIDTH {32}
set_instance_parameter_value tex_bridge SYMBOL_WIDTH {8}
set_instance_parameter_value tex_bridge HDL_ADDR_WIDTH {32}
set_instance_parameter_value tex_bridge ADDRESS_WIDTH {32}

add_connection clk_0.clk hps_0.h2f_lw_axi_clock
add_connection clk_0.clk hps_0.h2f_axi_clock
add_connection clk_0.clk hps_0.f2h_axi_clock
add_connection clk_0.clk hps_0.f2h_sdram0_clock
add_connection clk_0.clk h2f_bridge.clk
add_connection clk_0.clk fb_bridge.clk
add_connection clk_0.clk tex_bridge.clk

add_connection clk_0.clk_reset h2f_bridge.reset
add_connection clk_0.clk_reset fb_bridge.reset
add_connection clk_0.clk_reset tex_bridge.reset

add_connection hps_0.h2f_axi_master h2f_bridge.s0
set_connection_parameter_value hps_0.h2f_axi_master/h2f_bridge.s0 baseAddress {0x00000000}
set_connection_parameter_value hps_0.h2f_axi_master/h2f_bridge.s0 defaultConnection {0}

add_connection fb_bridge.m0 hps_0.f2h_sdram0_data
set_connection_parameter_value fb_bridge.m0/hps_0.f2h_sdram0_data baseAddress {0x00000000}
set_connection_parameter_value fb_bridge.m0/hps_0.f2h_sdram0_data defaultConnection {0}

add_connection tex_bridge.m0 hps_0.f2h_sdram0_data
set_connection_parameter_value tex_bridge.m0/hps_0.f2h_sdram0_data baseAddress {0x00000000}
set_connection_parameter_value tex_bridge.m0/hps_0.f2h_sdram0_data defaultConnection {0}

add_interface memory conduit end
set_interface_property memory EXPORT_OF hps_0.memory

add_interface clk clock sink
set_interface_property clk EXPORT_OF clk_0.clk_in

add_interface reset reset sink
set_interface_property reset EXPORT_OF clk_0.clk_in_reset

add_interface h2f_reset reset source
set_interface_property h2f_reset EXPORT_OF hps_0.h2f_reset

add_interface h2f_mpu_events conduit end
set_interface_property h2f_mpu_events EXPORT_OF hps_0.h2f_mpu_events

add_interface h2f_avalon avalon start
set_interface_property h2f_avalon EXPORT_OF h2f_bridge.m0

add_interface fb_mem avalon end
set_interface_property fb_mem EXPORT_OF fb_bridge.s0

add_interface tex_mem avalon end
set_interface_property tex_mem EXPORT_OF tex_bridge.s0

validate_system

save_system hw/de10/qsys/${qsys_name}.qsys
