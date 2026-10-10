module VoodooXlsTmuLodSelect(
  input wire [15:0] base_lod,
  input wire [15:0] perspective_adjust,
  input wire [26:0] t_lod,
  output wire [11:0] out
);
  wire [15:0] add_252;
  wire [5:0] bit_slice_253;
  wire [5:0] lod_max_raw_squeezed;
  wire [5:0] lit_2048_squeezed;
  wire [9:0] add_259;
  wire [5:0] lod_max_squeezed;
  wire [5:0] lod_max_squeezed_const_lsb_bits;
  wire [15:0] lod;
  wire [7:0] clamped__1;
  wire [3:0] level;
  wire [3:0] dimension;
  wire [3:0] aspect;
  wire s_wider;
  wire [3:0] sub_295;
  wire [3:0] narrow;
  wire [3:0] width;
  wire [3:0] height;
  assign add_252 = base_lod + perspective_adjust;
  assign bit_slice_253 = t_lod[17:12];
  assign lod_max_raw_squeezed = t_lod[11:6];
  assign lit_2048_squeezed = 6'h20;
  assign add_259 = add_252[15:6] + {{4{bit_slice_253[5]}}, bit_slice_253};
  assign lod_max_squeezed = lod_max_raw_squeezed > lit_2048_squeezed ? lit_2048_squeezed : lod_max_raw_squeezed;
  assign lod_max_squeezed_const_lsb_bits = 6'h00;
  assign lod = {add_259, add_252[5:0]};
  assign clamped__1 = $signed(lod) < $signed({4'h0, t_lod[5:0], lod_max_squeezed_const_lsb_bits}) ? {4'h0, t_lod[5:2]} : ($signed(lod) > $signed({4'h0, lod_max_squeezed, lod_max_squeezed_const_lsb_bits}) ? {4'h0, lod_max_squeezed[5:2]} : add_259[9:2]);
  assign level = ($signed(clamped__1) > $signed(8'h08) ? 4'h8 : clamped__1[3:0]) & {4{~clamped__1[7]}};
  assign dimension = 4'h8 - level;
  assign aspect = {2'h0, t_lod[22:21]};
  assign s_wider = t_lod[20];
  assign sub_295 = dimension - aspect;
  assign narrow = sub_295 & {4{dimension >= aspect}};
  assign width = s_wider | ~(t_lod[21] | t_lod[22]) ? dimension : narrow;
  assign height = s_wider ? narrow : dimension;
  assign out = {level, width, height};
endmodule
