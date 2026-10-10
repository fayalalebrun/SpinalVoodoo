module VoodooXlsFogWDepth(
  input wire [31:0] raw_w,
  output wire [15:0] out
);
  wire [15:0] upper;
  wire [15:0] reverse_169;
  wire [16:0] one_hot_170;
  wire [4:0] exponent;
  wire [29:0] inverted__2;
  wire [1:0] inverted__1;
  wire [2:0] sub_178;
  wire [31:0] inverted;
  wire [4:0] shift_amount;
  wire [31:0] shrl_182;
  wire [11:0] mantissa;
  wire [16:0] depth_raw;
  wire [15:0] clamped;
  assign upper = raw_w[29:14];
  assign reverse_169 = {upper[0], upper[1], upper[2], upper[3], upper[4], upper[5], upper[6], upper[7], upper[8], upper[9], upper[10], upper[11], upper[12], upper[13], upper[14], upper[15]};
  assign one_hot_170 = {reverse_169[15:0] == 16'h0000, reverse_169[15] && reverse_169[14:0] == 15'h0000, reverse_169[14] && reverse_169[13:0] == 14'h0000, reverse_169[13] && reverse_169[12:0] == 13'h0000, reverse_169[12] && reverse_169[11:0] == 12'h000, reverse_169[11] && reverse_169[10:0] == 11'h000, reverse_169[10] && reverse_169[9:0] == 10'h000, reverse_169[9] && reverse_169[8:0] == 9'h000, reverse_169[8] && reverse_169[7:0] == 8'h00, reverse_169[7] && reverse_169[6:0] == 7'h00, reverse_169[6] && reverse_169[5:0] == 6'h00, reverse_169[5] && reverse_169[4:0] == 5'h00, reverse_169[4] && reverse_169[3:0] == 4'h0, reverse_169[3] && reverse_169[2:0] == 3'h0, reverse_169[2] && reverse_169[1:0] == 2'h0, reverse_169[1] && !reverse_169[0], reverse_169[0]};
  assign exponent = {one_hot_170[16], one_hot_170[8] | one_hot_170[9] | one_hot_170[10] | one_hot_170[11] | one_hot_170[12] | one_hot_170[13] | one_hot_170[14] | one_hot_170[15], one_hot_170[4] | one_hot_170[5] | one_hot_170[6] | one_hot_170[7] | one_hot_170[12] | one_hot_170[13] | one_hot_170[14] | one_hot_170[15], one_hot_170[2] | one_hot_170[3] | one_hot_170[6] | one_hot_170[7] | one_hot_170[10] | one_hot_170[11] | one_hot_170[14] | one_hot_170[15], one_hot_170[1] | one_hot_170[3] | one_hot_170[5] | one_hot_170[7] | one_hot_170[9] | one_hot_170[11] | one_hot_170[13] | one_hot_170[15]};
  assign inverted__2 = ~raw_w[29:0];
  assign inverted__1 = 2'h3;
  assign sub_178 = 3'h4 - exponent[4:2];
  assign inverted = {inverted__2, inverted__1};
  assign shift_amount = {sub_178, ~exponent[1:0]};
  assign shrl_182 = inverted >> shift_amount;
  assign mantissa = shrl_182[11:0];
  assign depth_raw = {exponent, mantissa} + 17'h0_0001;
  assign clamped = depth_raw[16] ? 16'hffff : depth_raw[15:0];
  assign out = (upper == 16'h0000 ? 16'hf001 : clamped) & {16{~(raw_w[30] | raw_w[31])}};
endmodule
