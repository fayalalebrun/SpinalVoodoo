module VoodooXlsTmuLodIndex(
  input wire [63:0] temp_lod,
  output wire [14:0] out
);
  wire [31:0] high;
  wire use_high;
  wire [31:0] low;
  wire [31:0] sel_112;
  wire [31:0] reverse_113;
  wire [32:0] one_hot_114;
  wire [5:0] encode_115;
  wire [1:0] sub_121;
  wire [6:0] msb;
  wire [2:0] add_130;
  wire [5:0] shift;
  wire [63:0] shrl_136;
  wire [7:0] index;
  assign high = temp_lod[63:32];
  assign use_high = high != 32'h0000_0000;
  assign low = temp_lod[31:0];
  assign sel_112 = use_high ? high : low;
  assign reverse_113 = {sel_112[0], sel_112[1], sel_112[2], sel_112[3], sel_112[4], sel_112[5], sel_112[6], sel_112[7], sel_112[8], sel_112[9], sel_112[10], sel_112[11], sel_112[12], sel_112[13], sel_112[14], sel_112[15], sel_112[16], sel_112[17], sel_112[18], sel_112[19], sel_112[20], sel_112[21], sel_112[22], sel_112[23], sel_112[24], sel_112[25], sel_112[26], sel_112[27], sel_112[28], sel_112[29], sel_112[30], sel_112[31]};
  assign one_hot_114 = {reverse_113[31:0] == 32'h0000_0000, reverse_113[31] && reverse_113[30:0] == 31'h0000_0000, reverse_113[30] && reverse_113[29:0] == 30'h0000_0000, reverse_113[29] && reverse_113[28:0] == 29'h0000_0000, reverse_113[28] && reverse_113[27:0] == 28'h000_0000, reverse_113[27] && reverse_113[26:0] == 27'h000_0000, reverse_113[26] && reverse_113[25:0] == 26'h000_0000, reverse_113[25] && reverse_113[24:0] == 25'h000_0000, reverse_113[24] && reverse_113[23:0] == 24'h00_0000, reverse_113[23] && reverse_113[22:0] == 23'h00_0000, reverse_113[22] && reverse_113[21:0] == 22'h00_0000, reverse_113[21] && reverse_113[20:0] == 21'h00_0000, reverse_113[20] && reverse_113[19:0] == 20'h0_0000, reverse_113[19] && reverse_113[18:0] == 19'h0_0000, reverse_113[18] && reverse_113[17:0] == 18'h0_0000, reverse_113[17] && reverse_113[16:0] == 17'h0_0000, reverse_113[16] && reverse_113[15:0] == 16'h0000, reverse_113[15] && reverse_113[14:0] == 15'h0000, reverse_113[14] && reverse_113[13:0] == 14'h0000, reverse_113[13] && reverse_113[12:0] == 13'h0000, reverse_113[12] && reverse_113[11:0] == 12'h000, reverse_113[11] && reverse_113[10:0] == 11'h000, reverse_113[10] && reverse_113[9:0] == 10'h000, reverse_113[9] && reverse_113[8:0] == 9'h000, reverse_113[8] && reverse_113[7:0] == 8'h00, reverse_113[7] && reverse_113[6:0] == 7'h00, reverse_113[6] && reverse_113[5:0] == 6'h00, reverse_113[5] && reverse_113[4:0] == 5'h00, reverse_113[4] && reverse_113[3:0] == 4'h0, reverse_113[3] && reverse_113[2:0] == 3'h0, reverse_113[2] && reverse_113[1:0] == 2'h0, reverse_113[1] && !reverse_113[0], reverse_113[0]};
  assign encode_115 = {one_hot_114[32], one_hot_114[16] | one_hot_114[17] | one_hot_114[18] | one_hot_114[19] | one_hot_114[20] | one_hot_114[21] | one_hot_114[22] | one_hot_114[23] | one_hot_114[24] | one_hot_114[25] | one_hot_114[26] | one_hot_114[27] | one_hot_114[28] | one_hot_114[29] | one_hot_114[30] | one_hot_114[31], one_hot_114[8] | one_hot_114[9] | one_hot_114[10] | one_hot_114[11] | one_hot_114[12] | one_hot_114[13] | one_hot_114[14] | one_hot_114[15] | one_hot_114[24] | one_hot_114[25] | one_hot_114[26] | one_hot_114[27] | one_hot_114[28] | one_hot_114[29] | one_hot_114[30] | one_hot_114[31], one_hot_114[4] | one_hot_114[5] | one_hot_114[6] | one_hot_114[7] | one_hot_114[12] | one_hot_114[13] | one_hot_114[14] | one_hot_114[15] | one_hot_114[20] | one_hot_114[21] | one_hot_114[22] | one_hot_114[23] | one_hot_114[28] | one_hot_114[29] | one_hot_114[30] | one_hot_114[31], one_hot_114[2] | one_hot_114[3] | one_hot_114[6] | one_hot_114[7] | one_hot_114[10] | one_hot_114[11] | one_hot_114[14] | one_hot_114[15] | one_hot_114[18] | one_hot_114[19] | one_hot_114[22] | one_hot_114[23] | one_hot_114[26] | one_hot_114[27] | one_hot_114[30] | one_hot_114[31], one_hot_114[1] | one_hot_114[3] | one_hot_114[5] | one_hot_114[7] | one_hot_114[9] | one_hot_114[11] | one_hot_114[13] | one_hot_114[15] | one_hot_114[17] | one_hot_114[19] | one_hot_114[21] | one_hot_114[23] | one_hot_114[25] | one_hot_114[27] | one_hot_114[29] | one_hot_114[31]};
  assign sub_121 = 2'h0 - {1'h0, encode_115[5]};
  assign msb = use_high ? {1'h0, ~encode_115} : {sub_121, ~encode_115[4:0]};
  assign add_130 = msb[5:3] + 3'h7;
  assign shift = {add_130, msb[2:0]} & {6{|msb[6:3]}};
  assign shrl_136 = temp_lod >> shift;
  assign index = shrl_136[7:0];
  assign out = {msb, index};
endmodule
