module VoodooXlsTriangleSetup(
  input wire [19:0] ax,
  input wire [19:0] ay,
  input wire [19:0] bx,
  input wire [19:0] by,
  input wire [19:0] cx,
  input wire [19:0] cy,
  input wire reverse,
  output wire [559:0] out
);
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [39:0] smul40b_20b_x_20b (input reg [19:0] lhs, input reg [19:0] rhs);
    reg signed [19:0] signed_lhs;
    reg signed [19:0] signed_rhs;
    reg signed [39:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul40b_20b_x_20b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [39:0] smul40b_22b_x_20b (input reg [21:0] lhs, input reg [19:0] rhs);
    reg signed [21:0] signed_lhs;
    reg signed [19:0] signed_rhs;
    reg signed [39:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul40b_22b_x_20b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  // lint_off SIGNED_TYPE
  // lint_off MULTIPLY
  function automatic [36:0] smul37b_22b_x_17b (input reg [21:0] lhs, input reg [16:0] rhs);
    reg signed [21:0] signed_lhs;
    reg signed [16:0] signed_rhs;
    reg signed [36:0] signed_result;
    begin
      signed_lhs = $signed(lhs);
      signed_rhs = $signed(rhs);
      signed_result = signed_lhs * signed_rhs;
      smul37b_22b_x_17b = $unsigned(signed_result);
    end
  endfunction
  // lint_on MULTIPLY
  // lint_on SIGNED_TYPE
  wire [20:0] sign_ext_1032;
  wire [20:0] sign_ext_1033;
  wire [20:0] sign_ext_1035;
  wire [20:0] sign_ext_1036;
  wire [20:0] sign_ext_1037;
  wire [20:0] sub_1038;
  wire [20:0] sign_ext_1039;
  wire [20:0] sub_1040;
  wire [20:0] sub_1041;
  wire [20:0] sub_1042;
  wire [21:0] b_unadjusted_squeezed_squeezed;
  wire [20:0] sub_1044;
  wire [21:0] b_unadjusted__1_squeezed_squeezed;
  wire [20:0] sub_1046;
  wire [21:0] b_unadjusted__2_squeezed_squeezed;
  wire [21:0] a_unadjusted_squeezed_squeezed;
  wire [21:0] a_unadjusted__1_squeezed_squeezed;
  wire [21:0] a_unadjusted__2_squeezed_squeezed;
  wire [15:0] floor__1_squeezed;
  wire [21:0] b_squeezed_squeezed;
  wire [21:0] b__1_squeezed_squeezed;
  wire [21:0] b__2_squeezed_squeezed;
  wire [15:0] minimum_x__1;
  wire [15:0] add_1078;
  wire [21:0] a_squeezed_squeezed;
  wire [21:0] a__1_squeezed_squeezed;
  wire [21:0] a__2_squeezed_squeezed;
  wire [19:0] x_center;
  wire [15:0] y0_squeezed;
  wire [39:0] smul_1093;
  wire [39:0] smul_1094;
  wire [39:0] smul_1097;
  wire [39:0] smul_1098;
  wire [39:0] smul_1101;
  wire [39:0] smul_1102;
  wire [39:0] smul_1108;
  wire [16:0] y_center__1;
  wire [39:0] c_unadjusted;
  wire top_left;
  wire [39:0] smul_1112;
  wire [39:0] c_unadjusted__1;
  wire top_left__1;
  wire [39:0] smul_1115;
  wire [39:0] c_unadjusted__2;
  wire top_left__2;
  wire [19:0] maximum_x;
  wire [36:0] smul_1120;
  wire [36:0] smul_1124;
  wire [36:0] smul_1128;
  wire [3:0] smul_471_TrailingBits_;
  wire [15:0] floor_squeezed;
  wire [15:0] floor__2_squeezed;
  wire [36:0] add_1136;
  wire [39:0] c;
  wire [36:0] add_1140;
  wire [39:0] c__1;
  wire [36:0] add_1144;
  wire [39:0] c__2;
  wire [15:0] add_1149;
  wire [15:0] add_1151;
  wire [39:0] biased_c;
  wire [39:0] biased_c__1;
  wire [39:0] biased_c__2;
  wire [15:0] x1_squeezed;
  wire [15:0] y1_squeezed;
  wire [35:0] a_squeezed;
  wire [35:0] b_squeezed;
  wire [35:0] a__1_squeezed;
  wire [35:0] b__1_squeezed;
  wire [35:0] a__2_squeezed;
  wire [35:0] b__2_squeezed;
  wire [39:0] start0;
  wire [39:0] start1;
  wire [39:0] start2;
  assign sign_ext_1032 = {{1{bx[19]}}, bx};
  assign sign_ext_1033 = {{1{ax[19]}}, ax};
  assign sign_ext_1035 = {{1{cx[19]}}, cx};
  assign sign_ext_1036 = {{1{ay[19]}}, ay};
  assign sign_ext_1037 = {{1{by[19]}}, by};
  assign sub_1038 = sign_ext_1032 - sign_ext_1033;
  assign sign_ext_1039 = {{1{cy[19]}}, cy};
  assign sub_1040 = sign_ext_1035 - sign_ext_1032;
  assign sub_1041 = sign_ext_1033 - sign_ext_1035;
  assign sub_1042 = sign_ext_1036 - sign_ext_1037;
  assign b_unadjusted_squeezed_squeezed = {{1{sub_1038[20]}}, sub_1038};
  assign sub_1044 = sign_ext_1037 - sign_ext_1039;
  assign b_unadjusted__1_squeezed_squeezed = {{1{sub_1040[20]}}, sub_1040};
  assign sub_1046 = sign_ext_1039 - sign_ext_1036;
  assign b_unadjusted__2_squeezed_squeezed = {{1{sub_1041[20]}}, sub_1041};
  assign a_unadjusted_squeezed_squeezed = {{1{sub_1042[20]}}, sub_1042};
  assign a_unadjusted__1_squeezed_squeezed = {{1{sub_1044[20]}}, sub_1044};
  assign a_unadjusted__2_squeezed_squeezed = {{1{sub_1046[20]}}, sub_1046};
  assign floor__1_squeezed = ay[19:4];
  assign b_squeezed_squeezed = reverse ? -b_unadjusted_squeezed_squeezed : b_unadjusted_squeezed_squeezed;
  assign b__1_squeezed_squeezed = reverse ? -b_unadjusted__1_squeezed_squeezed : b_unadjusted__1_squeezed_squeezed;
  assign b__2_squeezed_squeezed = reverse ? -b_unadjusted__2_squeezed_squeezed : b_unadjusted__2_squeezed_squeezed;
  assign minimum_x__1 = $signed(ax) < $signed(bx) ? ($signed(ax) < $signed(cx) ? ax[19:4] : cx[19:4]) : ($signed(bx) < $signed(cx) ? bx[19:4] : cx[19:4]);
  assign add_1078 = floor__1_squeezed + 16'h0001;
  assign a_squeezed_squeezed = reverse ? -a_unadjusted_squeezed_squeezed : a_unadjusted_squeezed_squeezed;
  assign a__1_squeezed_squeezed = reverse ? -a_unadjusted__1_squeezed_squeezed : a_unadjusted__1_squeezed_squeezed;
  assign a__2_squeezed_squeezed = reverse ? -a_unadjusted__2_squeezed_squeezed : a_unadjusted__2_squeezed_squeezed;
  assign x_center = {minimum_x__1, 4'h9};
  assign y0_squeezed = ay[3:0] > 4'h8 ? add_1078 : floor__1_squeezed;
  assign smul_1093 = smul40b_20b_x_20b(ax, by);
  assign smul_1094 = smul40b_20b_x_20b(bx, ay);
  assign smul_1097 = smul40b_20b_x_20b(bx, cy);
  assign smul_1098 = smul40b_20b_x_20b(cx, by);
  assign smul_1101 = smul40b_20b_x_20b(cx, ay);
  assign smul_1102 = smul40b_20b_x_20b(ax, cy);
  assign smul_1108 = smul40b_22b_x_20b(a_squeezed_squeezed, x_center);
  assign y_center__1 = {y0_squeezed, 1'h1};
  assign c_unadjusted = smul_1093 - smul_1094;
  assign top_left = $signed(a_squeezed_squeezed) > $signed(22'h00_0000) | sub_1042 == 21'h00_0000 & $signed(b_squeezed_squeezed) > $signed(22'h00_0000);
  assign smul_1112 = smul40b_22b_x_20b(a__1_squeezed_squeezed, x_center);
  assign c_unadjusted__1 = smul_1097 - smul_1098;
  assign top_left__1 = $signed(a__1_squeezed_squeezed) > $signed(22'h00_0000) | sub_1044 == 21'h00_0000 & $signed(b__1_squeezed_squeezed) > $signed(22'h00_0000);
  assign smul_1115 = smul40b_22b_x_20b(a__2_squeezed_squeezed, x_center);
  assign c_unadjusted__2 = smul_1101 - smul_1102;
  assign top_left__2 = $signed(a__2_squeezed_squeezed) > $signed(22'h00_0000) | sub_1046 == 21'h00_0000 & $signed(b__2_squeezed_squeezed) > $signed(22'h00_0000);
  assign maximum_x = $signed(ax) > $signed(bx) ? ($signed(ax) > $signed(cx) ? ax : cx) : ($signed(bx) > $signed(cx) ? bx : cx);
  assign smul_1120 = smul37b_22b_x_17b(b_squeezed_squeezed, y_center__1);
  assign smul_1124 = smul37b_22b_x_17b(b__1_squeezed_squeezed, y_center__1);
  assign smul_1128 = smul37b_22b_x_17b(b__2_squeezed_squeezed, y_center__1);
  assign smul_471_TrailingBits_ = 4'h0;
  assign floor_squeezed = maximum_x[19:4];
  assign floor__2_squeezed = cy[19:4];
  assign add_1136 = smul_1108[39:3] + smul_1120;
  assign c = reverse ? -c_unadjusted : c_unadjusted;
  assign add_1140 = smul_1112[39:3] + smul_1124;
  assign c__1 = reverse ? -c_unadjusted__1 : c_unadjusted__1;
  assign add_1144 = smul_1115[39:3] + smul_1128;
  assign c__2 = reverse ? -c_unadjusted__2 : c_unadjusted__2;
  assign add_1149 = floor_squeezed + 16'h0001;
  assign add_1151 = floor__2_squeezed + 16'h0001;
  assign biased_c = c + {40{~top_left}};
  assign biased_c__1 = c__1 + {40{~top_left__1}};
  assign biased_c__2 = c__2 + {40{~top_left__2}};
  assign x1_squeezed = maximum_x[3:0] == smul_471_TrailingBits_ ? floor_squeezed : add_1149;
  assign y1_squeezed = cy[3:0] > 4'h8 ? add_1151 : floor__2_squeezed;
  assign a_squeezed = {{14{a_squeezed_squeezed[21]}}, a_squeezed_squeezed};
  assign b_squeezed = {{14{b_squeezed_squeezed[21]}}, b_squeezed_squeezed};
  assign a__1_squeezed = {{14{a__1_squeezed_squeezed[21]}}, a__1_squeezed_squeezed};
  assign b__1_squeezed = {{14{b__1_squeezed_squeezed[21]}}, b__1_squeezed_squeezed};
  assign a__2_squeezed = {{14{a__2_squeezed_squeezed[21]}}, a__2_squeezed_squeezed};
  assign b__2_squeezed = {{14{b__2_squeezed_squeezed[21]}}, b__2_squeezed_squeezed};
  assign start0 = {add_1136, smul_1108[2:0]} + biased_c;
  assign start1 = {add_1140, smul_1112[2:0]} + biased_c__1;
  assign start2 = {add_1144, smul_1115[2:0]} + biased_c__2;
  assign out = {minimum_x__1, smul_471_TrailingBits_, x1_squeezed, smul_471_TrailingBits_, y0_squeezed, smul_471_TrailingBits_, y1_squeezed, smul_471_TrailingBits_, a_squeezed, smul_471_TrailingBits_, b_squeezed, smul_471_TrailingBits_, biased_c, a__1_squeezed, smul_471_TrailingBits_, b__1_squeezed, smul_471_TrailingBits_, biased_c__1, a__2_squeezed, smul_471_TrailingBits_, b__2_squeezed, smul_471_TrailingBits_, biased_c__2, start0, start1, start2};
endmodule
