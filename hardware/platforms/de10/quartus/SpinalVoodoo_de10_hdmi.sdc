create_clock -name ref_clk -period 20.000 [get_ports {clk}]
create_clock -period "10.0 MHz" [get_pins -compatibility_mode hdmi_i2c|out_clk] -name hdmi_sck
derive_pll_clocks
derive_clock_uncertainty
