module VoodooXlsAlphaBlend(
  input wire [23:0] src_rgb,
  input wire [23:0] dest_rgb,
  input wire [23:0] before_fog_rgb,
  input wire [7:0] src_alpha,
  input wire [3:0] src_selector,
  input wire [3:0] dest_selector,
  input wire enable,
  output wire [23:0] out
);
  function automatic [7:0] priority_sel_8b_7way (input reg [6:0] sel, input reg [7:0] case0, input reg [7:0] case1, input reg [7:0] case2, input reg [7:0] case3, input reg [7:0] case4, input reg [7:0] case5, input reg [7:0] case6, input reg [7:0] default_value);
    begin
      casez (sel)
        7'b??????1: begin
          priority_sel_8b_7way = case0;
        end
        7'b?????10: begin
          priority_sel_8b_7way = case1;
        end
        7'b????100: begin
          priority_sel_8b_7way = case2;
        end
        7'b???1000: begin
          priority_sel_8b_7way = case3;
        end
        7'b??10000: begin
          priority_sel_8b_7way = case4;
        end
        7'b?100000: begin
          priority_sel_8b_7way = case5;
        end
        7'b1000000: begin
          priority_sel_8b_7way = case6;
        end
        7'b000_0000: begin
          priority_sel_8b_7way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_8b_7way = 8'dx;
        end
      endcase
    end
  endfunction
  // lint_off MULTIPLY
  function automatic [15:0] umul16b_8b_x_8b (input reg [7:0] lhs, input reg [7:0] rhs);
    begin
      umul16b_8b_x_8b = lhs * rhs;
    end
  endfunction
  // lint_on MULTIPLY
  wire [6:0] concat_780;
  wire [7:0] dest_alpha;
  wire [7:0] concat_785;
  wire [7:0] src_scale;
  wire [7:0] dest_scale;
  wire [7:0] src_scale__1;
  wire [7:0] dest_scale__1;
  wire [7:0] src_scale__2;
  wire [7:0] dest_scale__2;
  wire [15:0] umul_801;
  wire [15:0] umul_802;
  wire [15:0] umul_803;
  wire [15:0] umul_804;
  wire [15:0] umul_805;
  wire [15:0] umul_806;
  wire [16:0] wide;
  wire [16:0] wide__1;
  wire [16:0] wide__2;
  wire [16:0] wide__3;
  wire [16:0] wide__4;
  wire [16:0] wide__5;
  wire [16:0] add_821;
  wire [16:0] add_823;
  wire [16:0] add_825;
  wire [16:0] add_827;
  wire [16:0] add_829;
  wire [16:0] add_831;
  wire [16:0] add_833;
  wire [16:0] add_834;
  wire [16:0] add_835;
  wire [16:0] add_836;
  wire [16:0] add_837;
  wire [16:0] add_838;
  wire [7:0] src_value;
  wire [7:0] dest_value;
  wire [7:0] src_value__1;
  wire [7:0] dest_value__1;
  wire [7:0] src_value__2;
  wire [7:0] dest_value__2;
  wire [8:0] sum;
  wire [8:0] sum__1;
  wire [8:0] sum__2;
  wire [8:0] one_hot_871;
  wire eq_873;
  assign concat_780 = {src_selector == 4'hf | src_selector == 4'h7, src_selector == 4'h6, src_selector == 4'h5, src_selector == 4'h4 | src_selector == 4'h3, src_selector == 4'h2, src_selector == 4'h1, src_selector == 4'h0};
  assign dest_alpha = 8'hff;
  assign concat_785 = {dest_selector == 4'hf, dest_selector == 4'h7, dest_selector == 4'h6, dest_selector == 4'h5, dest_selector == 4'h4 | dest_selector == 4'h3, dest_selector == 4'h2, dest_selector == 4'h1, dest_selector == 4'h0};
  assign src_scale = priority_sel_8b_7way(concat_780, 8'h00, src_alpha, dest_rgb[23:16], dest_alpha, ~src_alpha, ~dest_rgb[23:16], 8'h00, dest_alpha);
  assign dest_scale = 8'h00 & {8{concat_785[0]}} | src_alpha & {8{concat_785[1]}} | src_rgb[23:16] & {8{concat_785[2]}} | dest_alpha & {8{concat_785[3]}} | ~src_alpha & {8{concat_785[4]}} | ~src_rgb[23:16] & {8{concat_785[5]}} | 8'h00 & {8{concat_785[6]}} | before_fog_rgb[23:16] & {8{concat_785[7]}};
  assign src_scale__1 = priority_sel_8b_7way(concat_780, 8'h00, src_alpha, dest_rgb[15:8], dest_alpha, ~src_alpha, ~dest_rgb[15:8], 8'h00, dest_alpha);
  assign dest_scale__1 = 8'h00 & {8{concat_785[0]}} | src_alpha & {8{concat_785[1]}} | src_rgb[15:8] & {8{concat_785[2]}} | dest_alpha & {8{concat_785[3]}} | ~src_alpha & {8{concat_785[4]}} | ~src_rgb[15:8] & {8{concat_785[5]}} | 8'h00 & {8{concat_785[6]}} | before_fog_rgb[15:8] & {8{concat_785[7]}};
  assign src_scale__2 = priority_sel_8b_7way(concat_780, 8'h00, src_alpha, dest_rgb[7:0], dest_alpha, ~src_alpha, ~dest_rgb[7:0], 8'h00, dest_alpha);
  assign dest_scale__2 = 8'h00 & {8{concat_785[0]}} | src_alpha & {8{concat_785[1]}} | src_rgb[7:0] & {8{concat_785[2]}} | dest_alpha & {8{concat_785[3]}} | ~src_alpha & {8{concat_785[4]}} | ~src_rgb[7:0] & {8{concat_785[5]}} | 8'h00 & {8{concat_785[6]}} | before_fog_rgb[7:0] & {8{concat_785[7]}};
  assign umul_801 = umul16b_8b_x_8b(src_rgb[23:16], src_scale);
  assign umul_802 = umul16b_8b_x_8b(dest_rgb[23:16], dest_scale);
  assign umul_803 = umul16b_8b_x_8b(src_rgb[15:8], src_scale__1);
  assign umul_804 = umul16b_8b_x_8b(dest_rgb[15:8], dest_scale__1);
  assign umul_805 = umul16b_8b_x_8b(src_rgb[7:0], src_scale__2);
  assign umul_806 = umul16b_8b_x_8b(dest_rgb[7:0], dest_scale__2);
  assign wide = {1'h0, umul_801};
  assign wide__1 = {1'h0, umul_802};
  assign wide__2 = {1'h0, umul_803};
  assign wide__3 = {1'h0, umul_804};
  assign wide__4 = {1'h0, umul_805};
  assign wide__5 = {1'h0, umul_806};
  assign add_821 = wide + 17'h0_0001;
  assign add_823 = wide__1 + 17'h0_0001;
  assign add_825 = wide__2 + 17'h0_0001;
  assign add_827 = wide__3 + 17'h0_0001;
  assign add_829 = wide__4 + 17'h0_0001;
  assign add_831 = wide__5 + 17'h0_0001;
  assign add_833 = add_821 + {9'h000, umul_801[15:8]};
  assign add_834 = add_823 + {9'h000, umul_802[15:8]};
  assign add_835 = add_825 + {9'h000, umul_803[15:8]};
  assign add_836 = add_827 + {9'h000, umul_804[15:8]};
  assign add_837 = add_829 + {9'h000, umul_805[15:8]};
  assign add_838 = add_831 + {9'h000, umul_806[15:8]};
  assign src_value = add_833[15:8];
  assign dest_value = add_834[15:8];
  assign src_value__1 = add_835[15:8];
  assign dest_value__1 = add_836[15:8];
  assign src_value__2 = add_837[15:8];
  assign dest_value__2 = add_838[15:8];
  assign sum = {1'h0, src_value} + {1'h0, dest_value};
  assign sum__1 = {1'h0, src_value__1} + {1'h0, dest_value__1};
  assign sum__2 = {1'h0, src_value__2} + {1'h0, dest_value__2};
  assign one_hot_871 = {concat_785[7:0] == 8'h00, concat_785[7] && concat_785[6:0] == 7'h00, concat_785[6] && concat_785[5:0] == 6'h00, concat_785[5] && concat_785[4:0] == 5'h00, concat_785[4] && concat_785[3:0] == 4'h0, concat_785[3] && concat_785[2:0] == 3'h0, concat_785[2] && concat_785[1:0] == 2'h0, concat_785[1] && !concat_785[0], concat_785[0]};
  assign eq_873 = concat_785 == one_hot_871[7:0];
  assign out = enable ? {sum[8] ? dest_alpha : sum[7:0], sum__1[8] ? dest_alpha : sum__1[7:0], sum__2[8] ? dest_alpha : sum__2[7:0]} : src_rgb;
endmodule
