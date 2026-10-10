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
  wire [39:0] product1;
  wire [39:0] product0;
  wire [20:0] sign_ext_1525;
  wire [20:0] sign_ext_1526;
  wire [39:0] product1__1;
  wire [39:0] product0__1;
  wire [20:0] sign_ext_1529;
  wire [39:0] product1__2;
  wire [39:0] product0__2;
  wire [39:0] rhs;
  wire [20:0] sign_ext_1533;
  wire [20:0] sign_ext_1534;
  wire [20:0] sub_1535;
  wire [39:0] rhs__1;
  wire [20:0] sign_ext_1537;
  wire [20:0] sub_1538;
  wire [39:0] rhs__2;
  wire [20:0] sub_1540;
  wire [39:0] lhs;
  wire [39:0] not_1542;
  wire [20:0] sub_1543;
  wire [21:0] b_unadjusted_squeezed_squeezed;
  wire [39:0] lhs__1;
  wire [39:0] not_1546;
  wire [20:0] sub_1547;
  wire [21:0] b_unadjusted__1_squeezed_squeezed;
  wire [39:0] lhs__2;
  wire [39:0] not_1550;
  wire [20:0] sub_1551;
  wire [21:0] b_unadjusted__2_squeezed_squeezed;
  wire [21:0] a_unadjusted_squeezed_squeezed;
  wire [21:0] a_unadjusted__1_squeezed_squeezed;
  wire [21:0] a_unadjusted__2_squeezed_squeezed;
  wire [21:0] b_squeezed_squeezed;
  wire [21:0] b__1_squeezed_squeezed;
  wire [21:0] b__2_squeezed_squeezed;
  wire [8:0] sum0;
  wire [21:0] a_squeezed_squeezed;
  wire [8:0] sum0__1;
  wire [21:0] a__1_squeezed_squeezed;
  wire [8:0] sum0__2;
  wire [21:0] a__2_squeezed_squeezed;
  wire [8:0] sum1;
  wire top_left;
  wire [8:0] sum1__1;
  wire top_left__1;
  wire [8:0] sum1__2;
  wire top_left__2;
  wire [8:0] sum2;
  wire carry1;
  wire [8:0] sum2__1;
  wire carry1__1;
  wire [8:0] sum2__2;
  wire carry1__2;
  wire [8:0] sum3;
  wire carry2;
  wire [8:0] sum3__1;
  wire carry2__1;
  wire [8:0] sum3__2;
  wire carry2__2;
  wire [15:0] floor__1_squeezed;
  wire [15:0] minimum_x__1;
  wire [15:0] add_1707;
  wire carry3;
  wire carry3__1;
  wire carry3__2;
  wire [19:0] x_center;
  wire [15:0] y0_squeezed;
  wire [39:0] smul_1728;
  wire [16:0] y_center__1;
  wire [6:0] smul_838_TrailingBits_;
  wire carry4;
  wire [39:0] smul_1734;
  wire carry4__1;
  wire [39:0] smul_1738;
  wire carry4__2;
  wire [19:0] maximum_x;
  wire [36:0] smul_1744;
  wire [7:0] sum4__1;
  wire [36:0] smul_1752;
  wire [7:0] sum4__2;
  wire [36:0] smul_1760;
  wire [7:0] sum4__3;
  wire [3:0] smul_836_TrailingBits_;
  wire [15:0] floor_squeezed;
  wire [15:0] floor__2_squeezed;
  wire [36:0] add_1772;
  wire [7:0] add_1774;
  wire [7:0] add_1775;
  wire [7:0] add_1776;
  wire [7:0] add_1777;
  wire [7:0] add_1778;
  wire [36:0] add_1779;
  wire [7:0] add_1781;
  wire [7:0] add_1782;
  wire [7:0] add_1783;
  wire [7:0] add_1784;
  wire [7:0] add_1785;
  wire [36:0] add_1786;
  wire [7:0] add_1788;
  wire [7:0] add_1789;
  wire [7:0] add_1790;
  wire [7:0] add_1791;
  wire [7:0] add_1792;
  wire [15:0] add_1794;
  wire [15:0] add_1796;
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
  assign product1 = smul40b_20b_x_20b(bx, ay);
  assign product0 = smul40b_20b_x_20b(ax, by);
  assign sign_ext_1525 = {{1{bx[19]}}, bx};
  assign sign_ext_1526 = {{1{ax[19]}}, ax};
  assign product1__1 = smul40b_20b_x_20b(cx, by);
  assign product0__1 = smul40b_20b_x_20b(bx, cy);
  assign sign_ext_1529 = {{1{cx[19]}}, cx};
  assign product1__2 = smul40b_20b_x_20b(ax, cy);
  assign product0__2 = smul40b_20b_x_20b(cx, ay);
  assign rhs = reverse ? product0 : product1;
  assign sign_ext_1533 = {{1{ay[19]}}, ay};
  assign sign_ext_1534 = {{1{by[19]}}, by};
  assign sub_1535 = sign_ext_1525 - sign_ext_1526;
  assign rhs__1 = reverse ? product0__1 : product1__1;
  assign sign_ext_1537 = {{1{cy[19]}}, cy};
  assign sub_1538 = sign_ext_1529 - sign_ext_1525;
  assign rhs__2 = reverse ? product0__2 : product1__2;
  assign sub_1540 = sign_ext_1526 - sign_ext_1529;
  assign lhs = reverse ? product1 : product0;
  assign not_1542 = ~rhs;
  assign sub_1543 = sign_ext_1533 - sign_ext_1534;
  assign b_unadjusted_squeezed_squeezed = {{1{sub_1535[20]}}, sub_1535};
  assign lhs__1 = reverse ? product1__1 : product0__1;
  assign not_1546 = ~rhs__1;
  assign sub_1547 = sign_ext_1534 - sign_ext_1537;
  assign b_unadjusted__1_squeezed_squeezed = {{1{sub_1538[20]}}, sub_1538};
  assign lhs__2 = reverse ? product1__2 : product0__2;
  assign not_1550 = ~rhs__2;
  assign sub_1551 = sign_ext_1537 - sign_ext_1533;
  assign b_unadjusted__2_squeezed_squeezed = {{1{sub_1540[20]}}, sub_1540};
  assign a_unadjusted_squeezed_squeezed = {{1{sub_1543[20]}}, sub_1543};
  assign a_unadjusted__1_squeezed_squeezed = {{1{sub_1547[20]}}, sub_1547};
  assign a_unadjusted__2_squeezed_squeezed = {{1{sub_1551[20]}}, sub_1551};
  assign b_squeezed_squeezed = reverse ? -b_unadjusted_squeezed_squeezed : b_unadjusted_squeezed_squeezed;
  assign b__1_squeezed_squeezed = reverse ? -b_unadjusted__1_squeezed_squeezed : b_unadjusted__1_squeezed_squeezed;
  assign b__2_squeezed_squeezed = reverse ? -b_unadjusted__2_squeezed_squeezed : b_unadjusted__2_squeezed_squeezed;
  assign sum0 = {1'h0, lhs[7:0]} + {1'h0, not_1542[7:0]};
  assign a_squeezed_squeezed = reverse ? -a_unadjusted_squeezed_squeezed : a_unadjusted_squeezed_squeezed;
  assign sum0__1 = {1'h0, lhs__1[7:0]} + {1'h0, not_1546[7:0]};
  assign a__1_squeezed_squeezed = reverse ? -a_unadjusted__1_squeezed_squeezed : a_unadjusted__1_squeezed_squeezed;
  assign sum0__2 = {1'h0, lhs__2[7:0]} + {1'h0, not_1550[7:0]};
  assign a__2_squeezed_squeezed = reverse ? -a_unadjusted__2_squeezed_squeezed : a_unadjusted__2_squeezed_squeezed;
  assign sum1 = {1'h0, lhs[15:8]} + {1'h0, not_1542[15:8]};
  assign top_left = $signed(a_squeezed_squeezed) > $signed(22'h00_0000) | sub_1543 == 21'h00_0000 & $signed(b_squeezed_squeezed) > $signed(22'h00_0000);
  assign sum1__1 = {1'h0, lhs__1[15:8]} + {1'h0, not_1546[15:8]};
  assign top_left__1 = $signed(a__1_squeezed_squeezed) > $signed(22'h00_0000) | sub_1547 == 21'h00_0000 & $signed(b__1_squeezed_squeezed) > $signed(22'h00_0000);
  assign sum1__2 = {1'h0, lhs__2[15:8]} + {1'h0, not_1550[15:8]};
  assign top_left__2 = $signed(a__2_squeezed_squeezed) > $signed(22'h00_0000) | sub_1551 == 21'h00_0000 & $signed(b__2_squeezed_squeezed) > $signed(22'h00_0000);
  assign sum2 = {1'h0, lhs[23:16]} + {1'h0, not_1542[23:16]};
  assign carry1 = sum0[8] | sum0[7:0] == 8'hff & top_left;
  assign sum2__1 = {1'h0, lhs__1[23:16]} + {1'h0, not_1546[23:16]};
  assign carry1__1 = sum0__1[8] | sum0__1[7:0] == 8'hff & top_left__1;
  assign sum2__2 = {1'h0, lhs__2[23:16]} + {1'h0, not_1550[23:16]};
  assign carry1__2 = sum0__2[8] | sum0__2[7:0] == 8'hff & top_left__2;
  assign sum3 = {1'h0, lhs[31:24]} + {1'h0, not_1542[31:24]};
  assign carry2 = sum1[8] | sum1[7:0] == 8'hff & carry1;
  assign sum3__1 = {1'h0, lhs__1[31:24]} + {1'h0, not_1546[31:24]};
  assign carry2__1 = sum1__1[8] | sum1__1[7:0] == 8'hff & carry1__1;
  assign sum3__2 = {1'h0, lhs__2[31:24]} + {1'h0, not_1550[31:24]};
  assign carry2__2 = sum1__2[8] | sum1__2[7:0] == 8'hff & carry1__2;
  assign floor__1_squeezed = ay[19:4];
  assign minimum_x__1 = $signed(ax) < $signed(bx) ? ($signed(ax) < $signed(cx) ? ax[19:4] : cx[19:4]) : ($signed(bx) < $signed(cx) ? bx[19:4] : cx[19:4]);
  assign add_1707 = floor__1_squeezed + 16'h0001;
  assign carry3 = sum2[8] | sum2[7:0] == 8'hff & carry2;
  assign carry3__1 = sum2__1[8] | sum2__1[7:0] == 8'hff & carry2__1;
  assign carry3__2 = sum2__2[8] | sum2__2[7:0] == 8'hff & carry2__2;
  assign x_center = {minimum_x__1, 4'h9};
  assign y0_squeezed = ay[3:0] > 4'h8 ? add_1707 : floor__1_squeezed;
  assign smul_1728 = smul40b_22b_x_20b(a_squeezed_squeezed, x_center);
  assign y_center__1 = {y0_squeezed, 1'h1};
  assign smul_838_TrailingBits_ = 7'h00;
  assign carry4 = sum3[8] | sum3[7:0] == 8'hff & carry3;
  assign smul_1734 = smul40b_22b_x_20b(a__1_squeezed_squeezed, x_center);
  assign carry4__1 = sum3__1[8] | sum3__1[7:0] == 8'hff & carry3__1;
  assign smul_1738 = smul40b_22b_x_20b(a__2_squeezed_squeezed, x_center);
  assign carry4__2 = sum3__2[8] | sum3__2[7:0] == 8'hff & carry3__2;
  assign maximum_x = $signed(ax) > $signed(bx) ? ($signed(ax) > $signed(cx) ? ax : cx) : ($signed(bx) > $signed(cx) ? bx : cx);
  assign smul_1744 = smul37b_22b_x_17b(b_squeezed_squeezed, y_center__1);
  assign sum4__1 = lhs[39:32] + not_1542[39:32];
  assign smul_1752 = smul37b_22b_x_17b(b__1_squeezed_squeezed, y_center__1);
  assign sum4__2 = lhs__1[39:32] + not_1546[39:32];
  assign smul_1760 = smul37b_22b_x_17b(b__2_squeezed_squeezed, y_center__1);
  assign sum4__3 = lhs__2[39:32] + not_1550[39:32];
  assign smul_836_TrailingBits_ = 4'h0;
  assign floor_squeezed = maximum_x[19:4];
  assign floor__2_squeezed = cy[19:4];
  assign add_1772 = smul_1728[39:3] + smul_1744;
  assign add_1774 = sum4__1 + {smul_838_TrailingBits_, carry4};
  assign add_1775 = sum3[7:0] + {smul_838_TrailingBits_, carry3};
  assign add_1776 = sum2[7:0] + {smul_838_TrailingBits_, carry2};
  assign add_1777 = sum1[7:0] + {smul_838_TrailingBits_, carry1};
  assign add_1778 = sum0[7:0] + {smul_838_TrailingBits_, top_left};
  assign add_1779 = smul_1734[39:3] + smul_1752;
  assign add_1781 = sum4__2 + {smul_838_TrailingBits_, carry4__1};
  assign add_1782 = sum3__1[7:0] + {smul_838_TrailingBits_, carry3__1};
  assign add_1783 = sum2__1[7:0] + {smul_838_TrailingBits_, carry2__1};
  assign add_1784 = sum1__1[7:0] + {smul_838_TrailingBits_, carry1__1};
  assign add_1785 = sum0__1[7:0] + {smul_838_TrailingBits_, top_left__1};
  assign add_1786 = smul_1738[39:3] + smul_1760;
  assign add_1788 = sum4__3 + {smul_838_TrailingBits_, carry4__2};
  assign add_1789 = sum3__2[7:0] + {smul_838_TrailingBits_, carry3__2};
  assign add_1790 = sum2__2[7:0] + {smul_838_TrailingBits_, carry2__2};
  assign add_1791 = sum1__2[7:0] + {smul_838_TrailingBits_, carry1__2};
  assign add_1792 = sum0__2[7:0] + {smul_838_TrailingBits_, top_left__2};
  assign add_1794 = floor_squeezed + 16'h0001;
  assign add_1796 = floor__2_squeezed + 16'h0001;
  assign biased_c = {add_1774, add_1775, add_1776, add_1777, add_1778};
  assign biased_c__1 = {add_1781, add_1782, add_1783, add_1784, add_1785};
  assign biased_c__2 = {add_1788, add_1789, add_1790, add_1791, add_1792};
  assign x1_squeezed = maximum_x[3:0] == smul_836_TrailingBits_ ? floor_squeezed : add_1794;
  assign y1_squeezed = cy[3:0] > 4'h8 ? add_1796 : floor__2_squeezed;
  assign a_squeezed = {{14{a_squeezed_squeezed[21]}}, a_squeezed_squeezed};
  assign b_squeezed = {{14{b_squeezed_squeezed[21]}}, b_squeezed_squeezed};
  assign a__1_squeezed = {{14{a__1_squeezed_squeezed[21]}}, a__1_squeezed_squeezed};
  assign b__1_squeezed = {{14{b__1_squeezed_squeezed[21]}}, b__1_squeezed_squeezed};
  assign a__2_squeezed = {{14{a__2_squeezed_squeezed[21]}}, a__2_squeezed_squeezed};
  assign b__2_squeezed = {{14{b__2_squeezed_squeezed[21]}}, b__2_squeezed_squeezed};
  assign start0 = {add_1772, smul_1728[2:0]} + biased_c;
  assign start1 = {add_1779, smul_1734[2:0]} + biased_c__1;
  assign start2 = {add_1786, smul_1738[2:0]} + biased_c__2;
  assign out = {minimum_x__1, smul_836_TrailingBits_, x1_squeezed, smul_836_TrailingBits_, y0_squeezed, smul_836_TrailingBits_, y1_squeezed, smul_836_TrailingBits_, a_squeezed, smul_836_TrailingBits_, b_squeezed, smul_836_TrailingBits_, add_1774, add_1775, add_1776, add_1777, add_1778, a__1_squeezed, smul_836_TrailingBits_, b__1_squeezed, smul_836_TrailingBits_, add_1781, add_1782, add_1783, add_1784, add_1785, a__2_squeezed, smul_836_TrailingBits_, b__2_squeezed, smul_836_TrailingBits_, add_1788, add_1789, add_1790, add_1791, add_1792, start0, start1, start2};
endmodule
