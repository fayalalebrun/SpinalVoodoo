module VoodooXlsTexelDecode(
  input wire [15:0] texel,
  input wire [3:0] format,
  input wire [23:0] palette,
  input wire [7:0] ncc_y,
  input wire [26:0] ncc_i,
  input wire [26:0] ncc_q,
  output wire [31:0] out
);
  function automatic [31:0] priority_sel_32b_13way (input reg [12:0] sel, input reg [31:0] case0, input reg [31:0] case1, input reg [31:0] case2, input reg [31:0] case3, input reg [31:0] case4, input reg [31:0] case5, input reg [31:0] case6, input reg [31:0] case7, input reg [31:0] case8, input reg [31:0] case9, input reg [31:0] case10, input reg [31:0] case11, input reg [31:0] case12, input reg [31:0] default_value);
    begin
      casez (sel)
        13'b????????????1: begin
          priority_sel_32b_13way = case0;
        end
        13'b???????????10: begin
          priority_sel_32b_13way = case1;
        end
        13'b??????????100: begin
          priority_sel_32b_13way = case2;
        end
        13'b?????????1000: begin
          priority_sel_32b_13way = case3;
        end
        13'b????????10000: begin
          priority_sel_32b_13way = case4;
        end
        13'b???????100000: begin
          priority_sel_32b_13way = case5;
        end
        13'b??????1000000: begin
          priority_sel_32b_13way = case6;
        end
        13'b?????10000000: begin
          priority_sel_32b_13way = case7;
        end
        13'b????100000000: begin
          priority_sel_32b_13way = case8;
        end
        13'b???1000000000: begin
          priority_sel_32b_13way = case9;
        end
        13'b??10000000000: begin
          priority_sel_32b_13way = case10;
        end
        13'b?100000000000: begin
          priority_sel_32b_13way = case11;
        end
        13'b1000000000000: begin
          priority_sel_32b_13way = case12;
        end
        13'b0_0000_0000_0000: begin
          priority_sel_32b_13way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_32b_13way = 32'dx;
        end
      endcase
    end
  endfunction
  wire [8:0] ir;
  wire [8:0] ig;
  wire [8:0] ib;
  wire [9:0] concat_802;
  wire [9:0] add_807;
  wire [8:0] qr;
  wire [9:0] add_809;
  wire [8:0] qg;
  wire [9:0] add_811;
  wire [8:0] qb;
  wire [10:0] add_819;
  wire [10:0] add_820;
  wire [10:0] add_821;
  wire [7:0] nr;
  wire [7:0] ng;
  wire [7:0] nb;
  wire [7:0] texel_byte;
  wire [31:0] concat_910;
  assign ir = ncc_i[26:18];
  assign ig = ncc_i[17:9];
  assign ib = ncc_i[8:0];
  assign concat_802 = {2'h0, ncc_y};
  assign add_807 = concat_802 + {{1{ir[8]}}, ir};
  assign qr = ncc_q[26:18];
  assign add_809 = concat_802 + {{1{ig[8]}}, ig};
  assign qg = ncc_q[17:9];
  assign add_811 = concat_802 + {{1{ib[8]}}, ib};
  assign qb = ncc_q[8:0];
  assign add_819 = {{1{add_807[9]}}, add_807} + {{2{qr[8]}}, qr};
  assign add_820 = {{1{add_809[9]}}, add_809} + {{2{qg[8]}}, qg};
  assign add_821 = {{1{add_811[9]}}, add_811} + {{2{qb[8]}}, qb};
  assign nr = ($signed(add_819) > $signed(11'h0ff) ? 8'hff : add_819[7:0]) & {8{~add_819[10]}};
  assign ng = ($signed(add_820) > $signed(11'h0ff) ? 8'hff : add_820[7:0]) & {8{~add_820[10]}};
  assign nb = ($signed(add_821) > $signed(11'h0ff) ? 8'hff : add_821[7:0]) & {8{~add_821[10]}};
  assign texel_byte = texel[7:0];
  assign concat_910 = {texel[15:11], texel[15:13], texel[10:5], texel[10:9], texel[4:0], texel[4:2], 8'hff};
  assign out = priority_sel_32b_13way({format == 4'he, format == 4'hd, format == 4'hc, format == 4'hb, format == 4'ha, format == 4'h9, format == 4'h8, format == 4'h5, format == 4'h4, format == 4'h3, format == 4'h2, format == 4'h1, format == 4'h0}, {texel[7:5], texel[7:5], texel[7:6], texel[4:2], texel[4:2], texel[4:3], texel[1:0], texel[1:0], texel[1:0], texel[1:0], 8'hff}, {nr, ng, nb, 8'hff}, {texel_byte, texel_byte, texel_byte, texel_byte}, {texel_byte, texel_byte, texel_byte, 8'hff}, {texel[3:0], texel[3:0], texel[3:0], texel[3:0], texel[3:0], texel[3:0], texel[7:4], texel[7:4]}, {palette, 8'hff}, {texel[7:5], texel[7:5], texel[7:6], texel[4:2], texel[4:2], texel[4:3], texel[1:0], texel[1:0], texel[1:0], texel[1:0], texel[15:8]}, {nr, ng, nb, texel[15:8]}, concat_910, {texel[14:10], texel[14:12], texel[9:5], texel[9:7], texel[4:0], texel[4:2], {8{texel[15]}}}, {texel[11:8], texel[11:4], texel_byte, texel[3:0], texel[15:12], texel[15:12]}, {texel_byte, texel_byte, texel_byte, texel[15:8]}, {palette, texel[15:8]}, concat_910);
endmodule
