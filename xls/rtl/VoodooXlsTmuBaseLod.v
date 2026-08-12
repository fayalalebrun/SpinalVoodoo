module VoodooXlsTmuBaseLod(
  input wire [63:0] temp_lod,
  input wire [6:0] msb,
  input wire [7:0] log_fraction,
  output wire [15:0] out
);
  wire [5:0] add_93;
  wire [13:0] concat_98;
  assign add_93 = {1'h0, msb[6:2]} + 6'h37;
  assign concat_98 = {add_93, msb[1:0], log_fraction[7:2]};
  assign out = {{2{concat_98[13]}}, concat_98} & {16{temp_lod != 64'h0000_0000_0000_0000}};
endmodule
