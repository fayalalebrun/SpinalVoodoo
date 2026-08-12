module VoodooXlsTriangleSetupS16(
  input wire [15:0] ax,
  input wire [15:0] ay,
  input wire [15:0] bx,
  input wire [15:0] by,
  input wire [15:0] cx,
  input wire [15:0] cy,
  input wire reverse,
  output wire [559:0] out
);
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [31:0] smul32b_16b_x_16b (input reg [15:0] lhs, input reg [15:0] rhs);
    reg signed [15:0] signed_lhs;
    reg signed [15:0] signed_rhs;
    reg signed [31:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul32b_16b_x_16b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [32:0] smul33b_17b_x_16b (input reg [16:0] lhs, input reg [15:0] rhs);
    reg signed [16:0] signed_lhs;
    reg signed [15:0] signed_rhs;
    reg signed [32:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul33b_17b_x_16b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [30:0] smul31b_17b_x_14b (input reg [16:0] lhs, input reg [13:0] rhs);
    reg signed [16:0] signed_lhs;
    reg signed [13:0] signed_rhs;
    reg signed [30:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul31b_17b_x_14b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  wire [16:0] sign_ext_1067;
  wire [16:0] sign_ext_1068;
  wire [16:0] sign_ext_1071;
  wire [16:0] sub_1073;
  wire [16:0] sign_ext_1074;
  wire [16:0] sign_ext_1075;
  wire [16:0] sub_1076;
  wire [16:0] sign_ext_1077;
  wire [16:0] sub_1078;
  wire [17:0] a_unadjusted_squeezed_squeezed;
  wire [16:0] sub_1085;
  wire [17:0] a_unadjusted__1_squeezed_squeezed;
  wire [16:0] sub_1087;
  wire [17:0] a_unadjusted__2_squeezed_squeezed;
  wire [16:0] sub_1089;
  wire [11:0] bit_slice_1095;
  wire [17:0] b_unadjusted_squeezed_squeezed;
  wire [31:0] smul_1097;
  wire [31:0] smul_1098;
  wire [17:0] b_unadjusted__1_squeezed_squeezed;
  wire [31:0] smul_1101;
  wire [31:0] smul_1102;
  wire [17:0] b_unadjusted__2_squeezed_squeezed;
  wire [31:0] smul_1105;
  wire [31:0] smul_1106;
  wire [17:0] a_squeezed_squeezed;
  wire [11:0] minimum_x__1;
  wire [12:0] floor__1_squeezed;
  wire [31:0] sub_1115;
  wire [17:0] a__1_squeezed_squeezed;
  wire [31:0] sub_1118;
  wire [17:0] a__2_squeezed_squeezed;
  wire [31:0] sub_1121;
  wire [16:0] a__3;
  wire [15:0] concat_1125;
  wire [12:0] add_1127;
  wire [17:0] b_squeezed_squeezed;
  wire [32:0] c_unadjusted_squeezed;
  wire [16:0] a__4;
  wire [17:0] b__1_squeezed_squeezed;
  wire [32:0] c_unadjusted__1_squeezed;
  wire [16:0] a__5;
  wire [17:0] b__2_squeezed_squeezed;
  wire [32:0] c_unadjusted__2_squeezed;
  wire [32:0] smul_1141;
  wire [12:0] y0_squeezed;
  wire [32:0] smul_1147;
  wire [32:0] smul_1151;
  wire [15:0] maximum_x;
  wire [29:0] bit_slice_1156;
  wire [16:0] b__3;
  wire [13:0] y_center__1;
  wire [32:0] c_squeezed;
  wire [29:0] bit_slice_1163;
  wire [16:0] b__4;
  wire [32:0] c__1_squeezed;
  wire [29:0] bit_slice_1168;
  wire [16:0] b__5;
  wire [32:0] c__2_squeezed;
  wire [11:0] bit_slice_1173;
  wire [11:0] bit_slice_1174;
  wire [30:0] smul_436_NarrowedMult_;
  wire top_left;
  wire [32:0] add_1178;
  wire [30:0] smul_444_NarrowedMult_;
  wire top_left__1;
  wire [32:0] add_1182;
  wire [30:0] smul_452_NarrowedMult_;
  wire top_left__2;
  wire [32:0] add_1186;
  wire [3:0] b_squeezed_const_lsb_bits;
  wire [12:0] floor_squeezed;
  wire [12:0] floor__2_squeezed;
  wire [30:0] add_1192;
  wire [32:0] biased_c_squeezed;
  wire [30:0] add_1195;
  wire [32:0] biased_c__1_squeezed;
  wire [30:0] add_1198;
  wire [32:0] biased_c__2_squeezed;
  wire [12:0] add_1202;
  wire [12:0] add_1204;
  wire [33:0] products;
  wire [39:0] biased_c;
  wire [33:0] products__1;
  wire [39:0] biased_c__1;
  wire [33:0] products__2;
  wire [39:0] biased_c__2;
  wire [12:0] x1_squeezed;
  wire [12:0] y1_squeezed;
  wire [16:0] x0;
  wire [16:0] x1;
  wire [16:0] y0;
  wire [16:0] y1;
  wire [34:0] add_1224;
  wire [34:0] add_1225;
  wire [34:0] add_1226;
  wire [35:0] a_squeezed;
  wire [35:0] b_squeezed;
  wire [35:0] a__1_squeezed;
  wire [35:0] b__1_squeezed;
  wire [35:0] a__2_squeezed;
  wire [35:0] b__2_squeezed;
  wire [39:0] start0;
  wire [39:0] start1;
  wire [39:0] start2;
  assign sign_ext_1067 = {{1{ay[15]}}, ay};
  assign sign_ext_1068 = {{1{by[15]}}, by};
  assign sign_ext_1071 = {{1{cy[15]}}, cy};
  assign sub_1073 = sign_ext_1067 - sign_ext_1068;
  assign sign_ext_1074 = {{1{bx[15]}}, bx};
  assign sign_ext_1075 = {{1{ax[15]}}, ax};
  assign sub_1076 = sign_ext_1068 - sign_ext_1071;
  assign sign_ext_1077 = {{1{cx[15]}}, cx};
  assign sub_1078 = sign_ext_1071 - sign_ext_1067;
  assign a_unadjusted_squeezed_squeezed = {{1{sub_1073[16]}}, sub_1073};
  assign sub_1085 = sign_ext_1074 - sign_ext_1075;
  assign a_unadjusted__1_squeezed_squeezed = {{1{sub_1076[16]}}, sub_1076};
  assign sub_1087 = sign_ext_1077 - sign_ext_1074;
  assign a_unadjusted__2_squeezed_squeezed = {{1{sub_1078[16]}}, sub_1078};
  assign sub_1089 = sign_ext_1075 - sign_ext_1077;
  assign bit_slice_1095 = ay[15:4];
  assign b_unadjusted_squeezed_squeezed = {{1{sub_1085[16]}}, sub_1085};
  assign smul_1097 = smul32b_16b_x_16b(ax, by);
  assign smul_1098 = smul32b_16b_x_16b(bx, ay);
  assign b_unadjusted__1_squeezed_squeezed = {{1{sub_1087[16]}}, sub_1087};
  assign smul_1101 = smul32b_16b_x_16b(bx, cy);
  assign smul_1102 = smul32b_16b_x_16b(cx, by);
  assign b_unadjusted__2_squeezed_squeezed = {{1{sub_1089[16]}}, sub_1089};
  assign smul_1105 = smul32b_16b_x_16b(cx, ay);
  assign smul_1106 = smul32b_16b_x_16b(ax, cy);
  assign a_squeezed_squeezed = reverse ? -a_unadjusted_squeezed_squeezed : a_unadjusted_squeezed_squeezed;
  assign minimum_x__1 = $signed(ax) < $signed(bx) ? ($signed(ax) < $signed(cx) ? ax[15:4] : cx[15:4]) : ($signed(bx) < $signed(cx) ? bx[15:4] : cx[15:4]);
  assign floor__1_squeezed = {{1{bit_slice_1095[11]}}, bit_slice_1095};
  assign sub_1115 = smul_1097 - smul_1098;
  assign a__1_squeezed_squeezed = reverse ? -a_unadjusted__1_squeezed_squeezed : a_unadjusted__1_squeezed_squeezed;
  assign sub_1118 = smul_1101 - smul_1102;
  assign a__2_squeezed_squeezed = reverse ? -a_unadjusted__2_squeezed_squeezed : a_unadjusted__2_squeezed_squeezed;
  assign sub_1121 = smul_1105 - smul_1106;
  assign a__3 = a_squeezed_squeezed[16:0];
  assign concat_1125 = {minimum_x__1, 4'h9};
  assign add_1127 = floor__1_squeezed + 13'h0001;
  assign b_squeezed_squeezed = reverse ? -b_unadjusted_squeezed_squeezed : b_unadjusted_squeezed_squeezed;
  assign c_unadjusted_squeezed = {{1{sub_1115[31]}}, sub_1115};
  assign a__4 = a__1_squeezed_squeezed[16:0];
  assign b__1_squeezed_squeezed = reverse ? -b_unadjusted__1_squeezed_squeezed : b_unadjusted__1_squeezed_squeezed;
  assign c_unadjusted__1_squeezed = {{1{sub_1118[31]}}, sub_1118};
  assign a__5 = a__2_squeezed_squeezed[16:0];
  assign b__2_squeezed_squeezed = reverse ? -b_unadjusted__2_squeezed_squeezed : b_unadjusted__2_squeezed_squeezed;
  assign c_unadjusted__2_squeezed = {{1{sub_1121[31]}}, sub_1121};
  assign smul_1141 = smul33b_17b_x_16b(a__3, concat_1125);
  assign y0_squeezed = ay[3:0] > 4'h8 ? add_1127 : floor__1_squeezed;
  assign smul_1147 = smul33b_17b_x_16b(a__4, concat_1125);
  assign smul_1151 = smul33b_17b_x_16b(a__5, concat_1125);
  assign maximum_x = $signed(ax) > $signed(bx) ? ($signed(ax) > $signed(cx) ? ax : cx) : ($signed(bx) > $signed(cx) ? bx : cx);
  assign bit_slice_1156 = smul_1141[32:3];
  assign b__3 = b_squeezed_squeezed[16:0];
  assign y_center__1 = {y0_squeezed, 1'h1};
  assign c_squeezed = reverse ? -c_unadjusted_squeezed : c_unadjusted_squeezed;
  assign bit_slice_1163 = smul_1147[32:3];
  assign b__4 = b__1_squeezed_squeezed[16:0];
  assign c__1_squeezed = reverse ? -c_unadjusted__1_squeezed : c_unadjusted__1_squeezed;
  assign bit_slice_1168 = smul_1151[32:3];
  assign b__5 = b__2_squeezed_squeezed[16:0];
  assign c__2_squeezed = reverse ? -c_unadjusted__2_squeezed : c_unadjusted__2_squeezed;
  assign bit_slice_1173 = maximum_x[15:4];
  assign bit_slice_1174 = cy[15:4];
  assign smul_436_NarrowedMult_ = smul31b_17b_x_14b(b__3, y_center__1);
  assign top_left = $signed(a_squeezed_squeezed) > $signed(18'h0_0000) | sub_1073 == 17'h0_0000 & $signed(b_squeezed_squeezed) > $signed(18'h0_0000);
  assign add_1178 = c_squeezed + 33'h1_ffff_ffff;
  assign smul_444_NarrowedMult_ = smul31b_17b_x_14b(b__4, y_center__1);
  assign top_left__1 = $signed(a__1_squeezed_squeezed) > $signed(18'h0_0000) | sub_1076 == 17'h0_0000 & $signed(b__1_squeezed_squeezed) > $signed(18'h0_0000);
  assign add_1182 = c__1_squeezed + 33'h1_ffff_ffff;
  assign smul_452_NarrowedMult_ = smul31b_17b_x_14b(b__5, y_center__1);
  assign top_left__2 = $signed(a__2_squeezed_squeezed) > $signed(18'h0_0000) | sub_1078 == 17'h0_0000 & $signed(b__2_squeezed_squeezed) > $signed(18'h0_0000);
  assign add_1186 = c__2_squeezed + 33'h1_ffff_ffff;
  assign b_squeezed_const_lsb_bits = 4'h0;
  assign floor_squeezed = {{1{bit_slice_1173[11]}}, bit_slice_1173};
  assign floor__2_squeezed = {{1{bit_slice_1174[11]}}, bit_slice_1174};
  assign add_1192 = {{1{bit_slice_1156[29]}}, bit_slice_1156} + smul_436_NarrowedMult_;
  assign biased_c_squeezed = top_left ? c_squeezed : add_1178;
  assign add_1195 = {{1{bit_slice_1163[29]}}, bit_slice_1163} + smul_444_NarrowedMult_;
  assign biased_c__1_squeezed = top_left__1 ? c__1_squeezed : add_1182;
  assign add_1198 = {{1{bit_slice_1168[29]}}, bit_slice_1168} + smul_452_NarrowedMult_;
  assign biased_c__2_squeezed = top_left__2 ? c__2_squeezed : add_1186;
  assign add_1202 = floor_squeezed + 13'h0001;
  assign add_1204 = floor__2_squeezed + 13'h0001;
  assign products = {add_1192, smul_1141[2:0]};
  assign biased_c = {{7{biased_c_squeezed[32]}}, biased_c_squeezed};
  assign products__1 = {add_1195, smul_1147[2:0]};
  assign biased_c__1 = {{7{biased_c__1_squeezed[32]}}, biased_c__1_squeezed};
  assign products__2 = {add_1198, smul_1151[2:0]};
  assign biased_c__2 = {{7{biased_c__2_squeezed[32]}}, biased_c__2_squeezed};
  assign x1_squeezed = maximum_x[3:0] == b_squeezed_const_lsb_bits ? floor_squeezed : add_1202;
  assign y1_squeezed = cy[3:0] > 4'h8 ? add_1204 : floor__2_squeezed;
  assign x0 = {{{1{minimum_x__1[11]}}, minimum_x__1}, b_squeezed_const_lsb_bits};
  assign x1 = {x1_squeezed, b_squeezed_const_lsb_bits};
  assign y0 = {y0_squeezed, b_squeezed_const_lsb_bits};
  assign y1 = {y1_squeezed, b_squeezed_const_lsb_bits};
  assign add_1224 = {{1{products[33]}}, products} + biased_c[34:0];
  assign add_1225 = {{1{products__1[33]}}, products__1} + biased_c__1[34:0];
  assign add_1226 = {{1{products__2[33]}}, products__2} + biased_c__2[34:0];
  assign a_squeezed = {{18{a_squeezed_squeezed[17]}}, a_squeezed_squeezed};
  assign b_squeezed = {{18{b_squeezed_squeezed[17]}}, b_squeezed_squeezed};
  assign a__1_squeezed = {{18{a__1_squeezed_squeezed[17]}}, a__1_squeezed_squeezed};
  assign b__1_squeezed = {{18{b__1_squeezed_squeezed[17]}}, b__1_squeezed_squeezed};
  assign a__2_squeezed = {{18{a__2_squeezed_squeezed[17]}}, a__2_squeezed_squeezed};
  assign b__2_squeezed = {{18{b__2_squeezed_squeezed[17]}}, b__2_squeezed_squeezed};
  assign start0 = {{5{add_1224[34]}}, add_1224};
  assign start1 = {{5{add_1225[34]}}, add_1225};
  assign start2 = {{5{add_1226[34]}}, add_1226};
  assign out = {{{3{x0[16]}}, x0}, {{3{x1[16]}}, x1}, {{3{y0[16]}}, y0}, {{3{y1[16]}}, y1}, a_squeezed, b_squeezed_const_lsb_bits, b_squeezed, b_squeezed_const_lsb_bits, biased_c, a__1_squeezed, b_squeezed_const_lsb_bits, b__1_squeezed, b_squeezed_const_lsb_bits, biased_c__1, a__2_squeezed, b_squeezed_const_lsb_bits, b__2_squeezed, b_squeezed_const_lsb_bits, biased_c__2, start0, start1, start2};
endmodule
