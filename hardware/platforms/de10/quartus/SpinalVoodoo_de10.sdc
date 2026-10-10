create_clock -name ref_clk -period 20.000 [get_ports {clk}]
derive_pll_clocks

# Core logic and HDMI scanout use explicit CDC synchronizers/FIFOs between
# independent PLL domains. Do not time those crossings as single-cycle paths.
set_clock_groups -asynchronous \
    -group [get_clocks {core_pll_0|pll_0|altera_pll_i|general[0].gpll~PLL_OUTPUT_COUNTER|divclk}] \
    -group [get_clocks {core_pll_0|pll_0|altera_pll_i|general[1].gpll~PLL_OUTPUT_COUNTER|divclk}] \
    -group [get_clocks {hdmi_pll_0|pll_0|altera_pll_i|general[0].gpll~PLL_OUTPUT_COUNTER|divclk}]

derive_clock_uncertainty
