module VoodooXlsColorCombine(
  input wire [23:0] iterated_rgb,
  input wire [7:0] iterated_alpha,
  input wire [7:0] iterated_z,
  input wire [23:0] texture_rgb,
  input wire [7:0] texture_alpha,
  input wire [23:0] color0_rgb,
  input wire [7:0] color0_alpha,
  input wire [23:0] color1_rgb,
  input wire [7:0] color1_alpha,
  input wire [1:0] rgb_sel,
  input wire local_sel,
  input wire local_override,
  input wire zero_other,
  input wire sub_local,
  input wire [2:0] factor_sel,
  input wire reverse_blend,
  input wire [1:0] add_mode,
  input wire invert_rgb,
  input wire [1:0] alpha_sel,
  input wire [1:0] alpha_local_sel,
  input wire alpha_zero_other,
  input wire alpha_sub_local,
  input wire [2:0] alpha_factor_sel,
  input wire alpha_reverse_blend,
  input wire [1:0] alpha_add_mode,
  input wire invert_alpha,
  output wire [31:0] out
);
  function automatic [7:0] priority_sel_8b_2way (input reg [1:0] sel, input reg [7:0] case0, input reg [7:0] case1, input reg [7:0] default_value);
    begin
      casez (sel)
        2'b?1: begin
          priority_sel_8b_2way = case0;
        end
        2'b10: begin
          priority_sel_8b_2way = case1;
        end
        2'b00: begin
          priority_sel_8b_2way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_8b_2way = 8'dx;
        end
      endcase
    end
  endfunction
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [17:0] smul18b_9b_x_10b (input reg [8:0] lhs, input reg [9:0] rhs);
    reg signed [8:0] signed_lhs;
    reg signed [9:0] signed_rhs;
    reg signed [17:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul18b_9b_x_10b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  function automatic [9:0] priority_sel_10b_2way (input reg [1:0] sel, input reg [9:0] case0, input reg [9:0] case1, input reg [9:0] default_value);
    begin
      casez (sel)
        2'b?1: begin
          priority_sel_10b_2way = case0;
        end
        2'b10: begin
          priority_sel_10b_2way = case1;
        end
        2'b00: begin
          priority_sel_10b_2way = default_value;
        end
        default: begin
          // Propagate X
          priority_sel_10b_2way = 10'dx;
        end
      endcase
    end
  endfunction
  wire [2:0] concat_1224;
  wire [23:0] other_rgb;
  wire [2:0] concat_1240;
  wire [23:0] local_rgb;
  wire [7:0] other_alpha;
  wire [8:0] sign_ext_1259;
  wire [7:0] local_alpha;
  wire [8:0] zeroed__3;
  wire [8:0] zeroed__4;
  wire [8:0] zeroed__5;
  wire [8:0] alpha_zeroed__1;
  wire [8:0] sub_1277;
  wire [8:0] sub_1278;
  wire [8:0] sub_1279;
  wire [8:0] sub_1281;
  wire [8:0] subtracted_squeezed;
  wire [8:0] subtracted__1_squeezed;
  wire [8:0] subtracted__2_squeezed;
  wire [8:0] alpha_subtracted_squeezed;
  wire [5:0] concat_1323;
  wire [4:0] concat_1330;
  wire [7:0] factor_pre;
  wire [7:0] factor_pre__1;
  wire [7:0] factor_pre__2;
  wire [7:0] alpha_factor_pre;
  wire [7:0] factor_base;
  wire [7:0] factor_base__1;
  wire [7:0] factor_base__2;
  wire [7:0] alpha_factor_base;
  wire [8:0] factor;
  wire [8:0] factor__1;
  wire [8:0] factor__2;
  wire [8:0] alpha_factor;
  wire [17:0] smul_1360;
  wire [17:0] smul_1362;
  wire [17:0] smul_1363;
  wire [17:0] smul_1365;
  wire [9:0] concat_1370;
  wire [1:0] concat_1378;
  wire [9:0] add_1379;
  wire [9:0] add_1380;
  wire [9:0] add_1381;
  wire [9:0] add_1382;
  wire [9:0] add_1383;
  wire [9:0] add_1384;
  wire [9:0] add_1386;
  wire [9:0] added__1_squeezed;
  wire [9:0] added__2_squeezed;
  wire [9:0] added__3_squeezed;
  wire [9:0] alpha_added__1_squeezed;
  wire [7:0] sel_1409;
  wire [7:0] sign_ext_1410;
  wire [7:0] sel_1411;
  wire [7:0] sign_ext_1412;
  wire [7:0] sel_1413;
  wire [7:0] sign_ext_1414;
  wire [7:0] sel_1415;
  wire [7:0] sign_ext_1416;
  wire [7:0] clamped;
  wire [7:0] clamped__1;
  wire [7:0] clamped__2;
  wire [7:0] alpha_clamped;
  wire [5:0] one_hot_1437;
  wire [6:0] one_hot_1443;
  wire [3:0] one_hot_1461;
  wire [3:0] one_hot_1467;
  wire [7:0] red;
  wire [7:0] green;
  wire [7:0] blue;
  wire [7:0] alpha_out;
  wire eq_1439;
  wire eq_1445;
  wire eq_1463;
  wire eq_1469;
  assign concat_1224 = {rgb_sel == 2'h2, rgb_sel == 2'h1, ~(rgb_sel[0] | rgb_sel[1])};
  assign other_rgb = iterated_rgb & {24{concat_1224[0]}} | texture_rgb & {24{concat_1224[1]}} | color1_rgb & {24{concat_1224[2]}};
  assign concat_1240 = {alpha_sel == 2'h2, alpha_sel == 2'h1, ~(alpha_sel[0] | alpha_sel[1])};
  assign local_rgb = local_override & texture_alpha[7] | local_sel ? color0_rgb : iterated_rgb;
  assign other_alpha = iterated_alpha & {8{concat_1240[0]}} | texture_alpha & {8{concat_1240[1]}} | color1_alpha & {8{concat_1240[2]}};
  assign sign_ext_1259 = {9{~zero_other}};
  assign local_alpha = priority_sel_8b_2way({alpha_local_sel == 2'h1, ~(alpha_local_sel[0] | alpha_local_sel[1])}, iterated_alpha, color0_alpha, iterated_z);
  assign zeroed__3 = {1'h0, other_rgb[23:16]} & sign_ext_1259;
  assign zeroed__4 = {1'h0, other_rgb[15:8]} & sign_ext_1259;
  assign zeroed__5 = {1'h0, other_rgb[7:0]} & sign_ext_1259;
  assign alpha_zeroed__1 = {1'h0, other_alpha} & {9{~alpha_zero_other}};
  assign sub_1277 = zeroed__3 - {1'h0, local_rgb[23:16]};
  assign sub_1278 = zeroed__4 - {1'h0, local_rgb[15:8]};
  assign sub_1279 = zeroed__5 - {1'h0, local_rgb[7:0]};
  assign sub_1281 = alpha_zeroed__1 - {1'h0, local_alpha};
  assign subtracted_squeezed = sub_local ? sub_1277 : zeroed__3;
  assign subtracted__1_squeezed = sub_local ? sub_1278 : zeroed__4;
  assign subtracted__2_squeezed = sub_local ? sub_1279 : zeroed__5;
  assign alpha_subtracted_squeezed = alpha_sub_local ? sub_1281 : alpha_zeroed__1;
  assign concat_1323 = {factor_sel == 3'h5, factor_sel == 3'h4, factor_sel == 3'h3, factor_sel == 3'h2, factor_sel == 3'h1, factor_sel == 3'h0};
  assign concat_1330 = {alpha_factor_sel == 3'h5 | alpha_factor_sel == 3'h4, alpha_factor_sel == 3'h3, alpha_factor_sel == 3'h2, alpha_factor_sel == 3'h1, alpha_factor_sel == 3'h0};
  assign factor_pre = 8'h00 & {8{concat_1323[0]}} | local_rgb[23:16] & {8{concat_1323[1]}} | subtracted_squeezed[7:0] & {8{~subtracted_squeezed[8]}} & {8{concat_1323[2]}} | local_alpha & {8{concat_1323[3]}} | texture_alpha & {8{concat_1323[4]}} | texture_rgb[23:16] & {8{concat_1323[5]}};
  assign factor_pre__1 = 8'h00 & {8{concat_1323[0]}} | local_rgb[15:8] & {8{concat_1323[1]}} | subtracted__1_squeezed[7:0] & {8{~subtracted__1_squeezed[8]}} & {8{concat_1323[2]}} | local_alpha & {8{concat_1323[3]}} | texture_alpha & {8{concat_1323[4]}} | texture_rgb[15:8] & {8{concat_1323[5]}};
  assign factor_pre__2 = 8'h00 & {8{concat_1323[0]}} | local_rgb[7:0] & {8{concat_1323[1]}} | subtracted__2_squeezed[7:0] & {8{~subtracted__2_squeezed[8]}} & {8{concat_1323[2]}} | local_alpha & {8{concat_1323[3]}} | texture_alpha & {8{concat_1323[4]}} | texture_rgb[7:0] & {8{concat_1323[5]}};
  assign alpha_factor_pre = 8'h00 & {8{concat_1330[0]}} | local_alpha & {8{concat_1330[1]}} | alpha_subtracted_squeezed[7:0] & {8{~alpha_subtracted_squeezed[8]}} & {8{concat_1330[2]}} | local_alpha & {8{concat_1330[3]}} | texture_alpha & {8{concat_1330[4]}};
  assign factor_base = reverse_blend ? factor_pre : ~factor_pre;
  assign factor_base__1 = reverse_blend ? factor_pre__1 : ~factor_pre__1;
  assign factor_base__2 = reverse_blend ? factor_pre__2 : ~factor_pre__2;
  assign alpha_factor_base = alpha_reverse_blend ? alpha_factor_pre : ~alpha_factor_pre;
  assign factor = {1'h0, factor_base} + 9'h001;
  assign factor__1 = {1'h0, factor_base__1} + 9'h001;
  assign factor__2 = {1'h0, factor_base__2} + 9'h001;
  assign alpha_factor = {1'h0, alpha_factor_base} + 9'h001;
  assign smul_1360 = smul18b_9b_x_10b(subtracted_squeezed, {1'h0, factor});
  assign smul_1362 = smul18b_9b_x_10b(subtracted__1_squeezed, {1'h0, factor__1});
  assign smul_1363 = smul18b_9b_x_10b(subtracted__2_squeezed, {1'h0, factor__2});
  assign smul_1365 = smul18b_9b_x_10b(alpha_subtracted_squeezed, {1'h0, alpha_factor});
  assign concat_1370 = {2'h0, local_alpha};
  assign concat_1378 = {add_mode == 2'h2, add_mode == 2'h1};
  assign add_1379 = smul_1360[17:8] + {2'h0, local_rgb[23:16]};
  assign add_1380 = smul_1360[17:8] + concat_1370;
  assign add_1381 = smul_1362[17:8] + {2'h0, local_rgb[15:8]};
  assign add_1382 = smul_1362[17:8] + concat_1370;
  assign add_1383 = smul_1363[17:8] + {2'h0, local_rgb[7:0]};
  assign add_1384 = smul_1363[17:8] + concat_1370;
  assign add_1386 = smul_1365[17:8] + concat_1370;
  assign added__1_squeezed = priority_sel_10b_2way(concat_1378, add_1379, add_1380, smul_1360[17:8]);
  assign added__2_squeezed = priority_sel_10b_2way(concat_1378, add_1381, add_1382, smul_1362[17:8]);
  assign added__3_squeezed = priority_sel_10b_2way(concat_1378, add_1383, add_1384, smul_1363[17:8]);
  assign alpha_added__1_squeezed = ~(alpha_add_mode[0] | alpha_add_mode[1]) ? smul_1365[17:8] : add_1386;
  assign sel_1409 = $signed(added__1_squeezed) > $signed(10'h0ff) ? 8'hff : added__1_squeezed[7:0];
  assign sign_ext_1410 = {8{~added__1_squeezed[9]}};
  assign sel_1411 = $signed(added__2_squeezed) > $signed(10'h0ff) ? 8'hff : added__2_squeezed[7:0];
  assign sign_ext_1412 = {8{~added__2_squeezed[9]}};
  assign sel_1413 = $signed(added__3_squeezed) > $signed(10'h0ff) ? 8'hff : added__3_squeezed[7:0];
  assign sign_ext_1414 = {8{~added__3_squeezed[9]}};
  assign sel_1415 = $signed(alpha_added__1_squeezed) > $signed(10'h0ff) ? 8'hff : alpha_added__1_squeezed[7:0];
  assign sign_ext_1416 = {8{~alpha_added__1_squeezed[9]}};
  assign clamped = sel_1409 & sign_ext_1410;
  assign clamped__1 = sel_1411 & sign_ext_1412;
  assign clamped__2 = sel_1413 & sign_ext_1414;
  assign alpha_clamped = sel_1415 & sign_ext_1416;
  assign one_hot_1437 = {concat_1330[4:0] == 5'h00, concat_1330[4] && concat_1330[3:0] == 4'h0, concat_1330[3] && concat_1330[2:0] == 3'h0, concat_1330[2] && concat_1330[1:0] == 2'h0, concat_1330[1] && !concat_1330[0], concat_1330[0]};
  assign one_hot_1443 = {concat_1323[5:0] == 6'h00, concat_1323[5] && concat_1323[4:0] == 5'h00, concat_1323[4] && concat_1323[3:0] == 4'h0, concat_1323[3] && concat_1323[2:0] == 3'h0, concat_1323[2] && concat_1323[1:0] == 2'h0, concat_1323[1] && !concat_1323[0], concat_1323[0]};
  assign one_hot_1461 = {concat_1240[2:0] == 3'h0, concat_1240[2] && concat_1240[1:0] == 2'h0, concat_1240[1] && !concat_1240[0], concat_1240[0]};
  assign one_hot_1467 = {concat_1224[2:0] == 3'h0, concat_1224[2] && concat_1224[1:0] == 2'h0, concat_1224[1] && !concat_1224[0], concat_1224[0]};
  assign red = invert_rgb ? ~(sel_1409 & sign_ext_1410) : clamped;
  assign green = invert_rgb ? ~(sel_1411 & sign_ext_1412) : clamped__1;
  assign blue = invert_rgb ? ~(sel_1413 & sign_ext_1414) : clamped__2;
  assign alpha_out = invert_alpha ? ~(sel_1415 & sign_ext_1416) : alpha_clamped;
  assign eq_1439 = concat_1330 == one_hot_1437[4:0];
  assign eq_1445 = concat_1323 == one_hot_1443[5:0];
  assign eq_1463 = concat_1240 == one_hot_1461[2:0];
  assign eq_1469 = concat_1224 == one_hot_1467[2:0];
  assign out = {red, green, blue, alpha_out};
endmodule
