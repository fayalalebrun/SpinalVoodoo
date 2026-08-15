module VoodooXlsTmuRecipMeta(
  input wire [47:0] w,
  input wire clamp_w,
  output wire [24:0] out
);
  wire [7:0] byte5;
  wire [7:0] byte4;
  wire [7:0] byte3;
  wire [7:0] byte2;
  wire [7:0] byte1;
  wire [7:0] byte0;
  wire [47:0] magnitude;
  wire [47:0] reverse_326;
  wire [48:0] one_hot_327;
  wire [5:0] encode_328;
  wire [47:0] normalized_narrowed;
  wire [2:0] sub_339;
  wire valid;
  wire clamp;
  assign byte5 = ~w[47:40] + {7'h00, w[39:0] == 40'h00_0000_0000};
  assign byte4 = ~w[39:32] + {7'h00, w[31:0] == 32'h0000_0000};
  assign byte3 = ~w[31:24] + {7'h00, w[23:0] == 24'h00_0000};
  assign byte2 = ~w[23:16] + {7'h00, w[15:0] == 16'h0000};
  assign byte1 = ~w[15:8] + {7'h00, w[7:0] == 8'h00};
  assign byte0 = -w[7:0];
  assign magnitude = w[47] ? {byte5, byte4, byte3, byte2, byte1, byte0} : w;
  assign reverse_326 = {magnitude[0], magnitude[1], magnitude[2], magnitude[3], magnitude[4], magnitude[5], magnitude[6], magnitude[7], magnitude[8], magnitude[9], magnitude[10], magnitude[11], magnitude[12], magnitude[13], magnitude[14], magnitude[15], magnitude[16], magnitude[17], magnitude[18], magnitude[19], magnitude[20], magnitude[21], magnitude[22], magnitude[23], magnitude[24], magnitude[25], magnitude[26], magnitude[27], magnitude[28], magnitude[29], magnitude[30], magnitude[31], magnitude[32], magnitude[33], magnitude[34], magnitude[35], magnitude[36], magnitude[37], magnitude[38], magnitude[39], magnitude[40], magnitude[41], magnitude[42], magnitude[43], magnitude[44], magnitude[45], magnitude[46], magnitude[47]};
  assign one_hot_327 = {reverse_326[47:0] == 48'h0000_0000_0000, reverse_326[47] && reverse_326[46:0] == 47'h0000_0000_0000, reverse_326[46] && reverse_326[45:0] == 46'h0000_0000_0000, reverse_326[45] && reverse_326[44:0] == 45'h0000_0000_0000, reverse_326[44] && reverse_326[43:0] == 44'h000_0000_0000, reverse_326[43] && reverse_326[42:0] == 43'h000_0000_0000, reverse_326[42] && reverse_326[41:0] == 42'h000_0000_0000, reverse_326[41] && reverse_326[40:0] == 41'h000_0000_0000, reverse_326[40] && reverse_326[39:0] == 40'h00_0000_0000, reverse_326[39] && reverse_326[38:0] == 39'h00_0000_0000, reverse_326[38] && reverse_326[37:0] == 38'h00_0000_0000, reverse_326[37] && reverse_326[36:0] == 37'h00_0000_0000, reverse_326[36] && reverse_326[35:0] == 36'h0_0000_0000, reverse_326[35] && reverse_326[34:0] == 35'h0_0000_0000, reverse_326[34] && reverse_326[33:0] == 34'h0_0000_0000, reverse_326[33] && reverse_326[32:0] == 33'h0_0000_0000, reverse_326[32] && reverse_326[31:0] == 32'h0000_0000, reverse_326[31] && reverse_326[30:0] == 31'h0000_0000, reverse_326[30] && reverse_326[29:0] == 30'h0000_0000, reverse_326[29] && reverse_326[28:0] == 29'h0000_0000, reverse_326[28] && reverse_326[27:0] == 28'h000_0000, reverse_326[27] && reverse_326[26:0] == 27'h000_0000, reverse_326[26] && reverse_326[25:0] == 26'h000_0000, reverse_326[25] && reverse_326[24:0] == 25'h000_0000, reverse_326[24] && reverse_326[23:0] == 24'h00_0000, reverse_326[23] && reverse_326[22:0] == 23'h00_0000, reverse_326[22] && reverse_326[21:0] == 22'h00_0000, reverse_326[21] && reverse_326[20:0] == 21'h00_0000, reverse_326[20] && reverse_326[19:0] == 20'h0_0000, reverse_326[19] && reverse_326[18:0] == 19'h0_0000, reverse_326[18] && reverse_326[17:0] == 18'h0_0000, reverse_326[17] && reverse_326[16:0] == 17'h0_0000, reverse_326[16] && reverse_326[15:0] == 16'h0000, reverse_326[15] && reverse_326[14:0] == 15'h0000, reverse_326[14] && reverse_326[13:0] == 14'h0000, reverse_326[13] && reverse_326[12:0] == 13'h0000, reverse_326[12] && reverse_326[11:0] == 12'h000, reverse_326[11] && reverse_326[10:0] == 11'h000, reverse_326[10] && reverse_326[9:0] == 10'h000, reverse_326[9] && reverse_326[8:0] == 9'h000, reverse_326[8] && reverse_326[7:0] == 8'h00, reverse_326[7] && reverse_326[6:0] == 7'h00, reverse_326[6] && reverse_326[5:0] == 6'h00, reverse_326[5] && reverse_326[4:0] == 5'h00, reverse_326[4] && reverse_326[3:0] == 4'h0, reverse_326[3] && reverse_326[2:0] == 3'h0, reverse_326[2] && reverse_326[1:0] == 2'h0, reverse_326[1] && !reverse_326[0], reverse_326[0]};
  assign encode_328 = {one_hot_327[32] | one_hot_327[33] | one_hot_327[34] | one_hot_327[35] | one_hot_327[36] | one_hot_327[37] | one_hot_327[38] | one_hot_327[39] | one_hot_327[40] | one_hot_327[41] | one_hot_327[42] | one_hot_327[43] | one_hot_327[44] | one_hot_327[45] | one_hot_327[46] | one_hot_327[47] | one_hot_327[48], one_hot_327[16] | one_hot_327[17] | one_hot_327[18] | one_hot_327[19] | one_hot_327[20] | one_hot_327[21] | one_hot_327[22] | one_hot_327[23] | one_hot_327[24] | one_hot_327[25] | one_hot_327[26] | one_hot_327[27] | one_hot_327[28] | one_hot_327[29] | one_hot_327[30] | one_hot_327[31] | one_hot_327[48], one_hot_327[8] | one_hot_327[9] | one_hot_327[10] | one_hot_327[11] | one_hot_327[12] | one_hot_327[13] | one_hot_327[14] | one_hot_327[15] | one_hot_327[24] | one_hot_327[25] | one_hot_327[26] | one_hot_327[27] | one_hot_327[28] | one_hot_327[29] | one_hot_327[30] | one_hot_327[31] | one_hot_327[40] | one_hot_327[41] | one_hot_327[42] | one_hot_327[43] | one_hot_327[44] | one_hot_327[45] | one_hot_327[46] | one_hot_327[47], one_hot_327[4] | one_hot_327[5] | one_hot_327[6] | one_hot_327[7] | one_hot_327[12] | one_hot_327[13] | one_hot_327[14] | one_hot_327[15] | one_hot_327[20] | one_hot_327[21] | one_hot_327[22] | one_hot_327[23] | one_hot_327[28] | one_hot_327[29] | one_hot_327[30] | one_hot_327[31] | one_hot_327[36] | one_hot_327[37] | one_hot_327[38] | one_hot_327[39] | one_hot_327[44] | one_hot_327[45] | one_hot_327[46] | one_hot_327[47], one_hot_327[2] | one_hot_327[3] | one_hot_327[6] | one_hot_327[7] | one_hot_327[10] | one_hot_327[11] | one_hot_327[14] | one_hot_327[15] | one_hot_327[18] | one_hot_327[19] | one_hot_327[22] | one_hot_327[23] | one_hot_327[26] | one_hot_327[27] | one_hot_327[30] | one_hot_327[31] | one_hot_327[34] | one_hot_327[35] | one_hot_327[38] | one_hot_327[39] | one_hot_327[42] | one_hot_327[43] | one_hot_327[46] | one_hot_327[47], one_hot_327[1] | one_hot_327[3] | one_hot_327[5] | one_hot_327[7] | one_hot_327[9] | one_hot_327[11] | one_hot_327[13] | one_hot_327[15] | one_hot_327[17] | one_hot_327[19] | one_hot_327[21] | one_hot_327[23] | one_hot_327[25] | one_hot_327[27] | one_hot_327[29] | one_hot_327[31] | one_hot_327[33] | one_hot_327[35] | one_hot_327[37] | one_hot_327[39] | one_hot_327[41] | one_hot_327[43] | one_hot_327[45] | one_hot_327[47]};
  assign normalized_narrowed = encode_328 >= 6'h30 ? 48'h0000_0000_0000 : magnitude << encode_328;
  assign sub_339 = 3'h2 - {1'h0, encode_328[5:4]};
  assign valid = ~(w[47] | magnitude == 48'h0000_0000_0000);
  assign clamp = clamp_w & w[47];
  assign out = {normalized_narrowed[46:31], sub_339, ~encode_328[3:0], valid, clamp};
endmodule
