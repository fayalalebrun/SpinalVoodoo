`timescale 1ns/1ps
module de10_hdmi_pll (
  input wire refclk,
  input wire rst,
  output reg outclk_0 = 0,
  output wire locked
);
  assign locked = !rst;
  always #18.518518 outclk_0 = ~outclk_0;
endmodule
